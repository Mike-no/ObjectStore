/**
 * ########################################################
 * 
 * @author: Michael De Angelis
 * @mat: 560049
 * @project: Sistemi Operativi e Laboratorio [SOL]
 * @A.A: 2018 / 2019
 * 
 * ########################################################
 */

#include "../include/client_op.h"

static int client_sock;
static struct sockaddr_un serv_addr;

int os_connect(char* name){
    if(name == NULL){
        errno = EINVAL;
        return -1;
    }

    struct timeval timeout;
    int opt;
    int connect_res;

    // Set the server address in the struct sockaddr_un
    memset(&serv_addr, 0, sizeof(serv_addr));
    strncpy(serv_addr.sun_path, __SOCK_PATH, strlen(__SOCK_PATH));
    serv_addr.sun_family = AF_UNIX;

    // Try to create the socket
    if((client_sock = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
        return -1;

    /**
     * The following segment of code aim to create a connection with
     * a timeout; if the connection doesn't take place within the set
     * limits, the function ends.
     */

    // Try to get the socket flags
    if((opt = fcntl(client_sock, F_GETFL, NULL)) == -1)
        return -1;    

    // Set the socket non-blocking
    if(fcntl(client_sock, F_SETFL, opt | O_NONBLOCK) == -1)
        return -1;

    memset(&timeout, 0, sizeof(timeout));
    timeout.tv_sec = TIMEOUT_SEC;
    timeout.tv_usec = 0;

    if((connect_res = connect(client_sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr))) == -1){
        if(errno == EINPROGRESS){
            fd_set wait_set;
            FD_ZERO(&wait_set);
            FD_SET(client_sock, &wait_set);
            // Wait for socket to be writable; return after given timeout
            connect_res = select(client_sock + 1, NULL, &wait_set, NULL, &timeout);
        }
    }
    else    // Connection was successfull immediately
        connect_res = 1;
    
    // Reset socket flags
    if(fcntl(client_sock, F_SETFL, opt & (~O_NONBLOCK) ) == -1)
        return -1;

    // An error occured in connect or select
    if(connect_res < 0)
        return -1;
    else if(connect_res == 0){  // Select timed out
        errno = ETIMEDOUT;
        return -1;
    }
    else{
        socklen_t len = sizeof(opt);
        
        // Check for errors in socket layer
        if(getsockopt(client_sock, SOL_SOCKET, SO_ERROR, &opt, &len) == -1)
            return -1;
        if(opt){    // There was an error
            errno = opt;
            return -1;
        }
    }

    // At this point the client is connected to the server, beginning of the registration phase
    // At this rate the socket is for-sure ready to be written; send the REGISTER operation
    char* request = NULL;
    size_t len = strlen(CLIENT_REGISTER_CMD) + strlen(name) + strlen(HEADER_TERM) + 1;
    
    // Send the request "REGISTER "name" \n""
    if((request = (char*)calloc(len, sizeof(char))) == NULL){
        close(client_sock);
        return -1;
    }
    strncpy(request, CLIENT_REGISTER_CMD, strlen(CLIENT_REGISTER_CMD) + 1);
    strncat(request, name, strlen(name));
    strncat(request, HEADER_TERM, strlen(HEADER_TERM));

    // Send the request "REGISTER "name" \n"" to the server then wait for the response
    if(!send_request(request, len - 1)){
        free(request);
        return -1;
    }
    free(request);

    // Get from the server and evaluate the response header
    int retval;
    if((retval = evaluate_response()) == 0)
        close(client_sock);
    return retval;
}

int os_store(char* name, void* block, size_t len){
    if(name == NULL || block == NULL){
        errno = EINVAL;
        close(client_sock);
        return -1;
    }

    char file_size[SIZE_MAX];
    sprintf(file_size, "%lu", len);

    char* request = NULL;
    size_t len_request = strlen(CLIENT_STORE_CMD) + strlen(name) + 1 + strlen(file_size) +
                        strlen(CLIENT_STORE_TERM) + len + 1;

    // Create the request "STORE "name" "len" \n "data""
    if((request = (char*)calloc(len_request, sizeof(char))) == NULL){
        close(client_sock);
        return -1;
    }
    strncpy(request, CLIENT_STORE_CMD, strlen(CLIENT_STORE_CMD) + 1);
    strncat(request, name, strlen(name));
    strncat(request, STR_DELIM, strlen(STR_DELIM));
    strncat(request, file_size, strlen(file_size));
    strncat(request, CLIENT_STORE_TERM, strlen(CLIENT_STORE_TERM));
    memcpy(request + strlen(request), block, len);

    // Send the request "STORE "name" "len" \n "data"" to the server then wait for the response
    if(!send_request(request, len_request - 1)){
        free(request);
        return -1;
    }
    free(request);

    // Get from the server and evaluate the response header
    return evaluate_response();
}

void* os_retrieve(char* name){
    if(name == NULL){
        errno = EINVAL;
        close(client_sock);
        return NULL;
    }

    char* request = NULL;
    size_t len = strlen(CLIENT_RETRIEVE_CMD) + strlen(name) + strlen(HEADER_TERM) + 1;

    // Create the request "RETRIEVE "name" \n"
    if((request = (char*)calloc(len, sizeof(char))) == NULL){
        close(client_sock);
        return NULL;
    }
    strncpy(request, CLIENT_RETRIEVE_CMD, strlen(CLIENT_RETRIEVE_CMD) + 1);
    strncat(request, name, strlen(name));
    strncat(request, HEADER_TERM, strlen(HEADER_TERM));

    // Send the request "RETRIEVE "name" \n" to the server then wait for the response
    if(!send_request(request, len - 1)){
        free(request);
        return NULL;
    }
    free(request);
    
    // Get the response header from the server; should be in the format "DATA "len" \n"
    int retval;
    char* response;
    if((retval = get_header(client_sock, &response)) == -1){    // Error occurred
        if(response != NULL)
            free(response);
        close(client_sock);
        return NULL;
    }
    else if(retval == -2){      // Connection Closed
        if(response != NULL)
            free(response);
        errno = ECONNABORTED;
        close(client_sock);
        return NULL;
    }
    else if(retval == 0){       // Select timed out (before allocate response, no need for free)
        errno = ETIMEDOUT;
        close(client_sock);
        return NULL;
    }

    // Check if the response is "DATA "len" \n"
    if(strncmp(response, SERVER_RETRIEVE, strlen(SERVER_RETRIEVE)) != 0){
        if(strncmp(response, SERVER_KO, strlen(SERVER_KO)) == 0){
            //fprintf(stderr, "Response : %s", response);
            free(response);
            // doesn't close the socket, to distinguish the cases where return NULL set errno = 0
            errno = 0;
            return NULL;
        }
        else{
            free(response);
            errno = EPROTO;
            close(client_sock);
            return NULL;
        }
    }

    char* saveptr;
    // Discards the command 
    strtok_r(response, STR_DELIM, &saveptr);
    // Save the length of the data that will be received
    long len_data = atol(strtok_r(NULL, STR_DELIM, &saveptr));
    // Must be the newline

    char* newline = strtok_r(NULL, STR_DELIM, &saveptr);
    if(newline && newline[0] != '\n'){
        free(response);
        errno = EPROTO;
        close(client_sock);
        return NULL;
    }

    free(response);

    char* data_retrieved = NULL;
    // + 1 for the space after \n in the header and before the data
    if((data_retrieved = (char*)calloc(len_data + 1, sizeof(char))) == NULL){
        close(client_sock);
        return NULL;
    }

    // + 1 to read the first space character
    if((retval = recv_all(client_sock, data_retrieved, len_data + 1, 0)) == -1){
        free(data_retrieved);
        close(client_sock);
        return NULL;
    }
    else if(retval == 0){
        free(data_retrieved);
        errno = ECONNABORTED;
        close(client_sock);
        return NULL;
    } 

    // Discard the space after the '\n' and before the data
    char* fdata = NULL;
    if((fdata = (char*)calloc(len_data, sizeof(char))) == NULL){
        free(data_retrieved);
        close(client_sock);
        return NULL;
    }
    memcpy(fdata, data_retrieved + 1, len_data);
    free(data_retrieved);

    return fdata;
}

int os_delete(char* name){
    if(name == NULL){
        errno = EINVAL;
        close(client_sock);
        return -1;
    }

    // Create the request "DELETE "name" \n"
    char* request = NULL;
    size_t len = strlen(CLIENT_DELETE_CMD) + strlen(name) + strlen(HEADER_TERM) + 1;

    if((request = (char*)calloc(len, sizeof(char))) == NULL){
        close(client_sock);
        return -1;
    }
    strncpy(request, CLIENT_DELETE_CMD, strlen(CLIENT_DELETE_CMD) + 1);
    strncat(request, name, strlen(name));
    strncat(request, HEADER_TERM, strlen(HEADER_TERM));

    // Send the request "DELETE "name" \n" to the server then wait for the response
    if(!send_request(request, len - 1)){
        free(request);
        return -1;
    }
    free(request);
    
    // Get from the server and evaluate the response header
    return evaluate_response();
}

int os_disconnect(){
    // Send the request "LEAVE \n" to the server then wait for the response
    if(!send_request(CLIENT_LEAVE_CMD, strlen(CLIENT_LEAVE_CMD)))
        return -1;

    // Get from the server and evaluate the response header
    int retval;
    if((retval =  evaluate_response()) == -1)
        return -1;
    else if(retval == 0){   // Shouldn't happen
        close(client_sock);
        exit(EXIT_FAILURE);
    }
    else{
        close(client_sock);
        return 1;
    }
}

int send_request(char* request, size_t len){
    int retval;
    // Error occurred
    if((retval = send_all(client_sock, request, len, 0)) == -1){
        close(client_sock);
        return 0;
    }
    else if(retval == 0){   // Connection closed 
        errno = ECONNABORTED;
        close(client_sock); 
        return 0;
    }

    return 1;   // header request sent
}

int evaluate_response(){
    // Get the response to evaluate it
    char* response = NULL;
    int retval;
    if((retval = get_header(client_sock, &response)) == -1){    // Error occurred
        if(response != NULL)
            free(response);
        close(client_sock);
        return -1;
    }
    else if(retval == -2){      // Connection closed
        if(response != NULL)
            free(response);
        errno = ECONNABORTED;
        close(client_sock);
        return -1;
    }
    else if(retval == 0){       // Select timed out (before allocate response, no need for free)
        errno = ETIMEDOUT;
        close(client_sock);
        return -1;
    }

    if(strncmp(response, SERVER_OK, strlen(SERVER_OK)) == 0){    // "OK \n" received
        //fprintf(stdout, "Response : %s", response);
        free(response);
        return 1;
    }
    else if(strncmp(response, SERVER_KO, strlen(SERVER_KO)) == 0){   // "KO "message" \n" received
        //fprintf(stdout, "Response : %s", response);
        free(response);
        return 0;
    }
    else{   // Protocol violation
        free(response);
        errno = EPROTO;
        close(client_sock);
        return -1;
    }
}

struct client_stat_t* exec_ftest(const char* name){
    size_t size = INIT_SIZE;
    int character = INIT_CHAR;
    struct client_stat_t* st;

    // Allocation and initialization of the client stat structure
    if((st = (struct client_stat_t*)malloc(sizeof(struct client_stat_t))) == NULL)
        return NULL;

    st->n_request = 0;
    st->ok_request = 0;
    st->ko_request = 0;

    for(int i = 0; i < N_OBJECTS_TEST; i++){   // For 20 objects
        char* obj;
        
        if((obj = (char*)calloc(size, sizeof(char))) == NULL){
            free(st);
            return NULL;
        }
        // First iteration will send 99 'a' plus '\0'
        memset(obj, (char)(character + i), size - 1);
        
        // Build the file name
        char* file_name = NULL;
        if((file_name = get_filename(name, i)) == NULL){
            free(st);
            free(obj);
            return NULL;
        }

        // Send the store request
        int retval;
        if((retval = os_store(file_name, obj, size)) == -1){    // Error occurred
            perror(CLIENT_OS_STORE_ERR);
            free(st);
            free(obj);
            free(file_name);
            exit(EXIT_FAILURE);
        }
        else if(retval == 0){   // KO response
            st->n_request++;
            st->ko_request++;
        }
        else{                   // OK response
            st->n_request++;
            st->ok_request++;
        }

        free(obj);
        free(file_name);

        // Increasing file size
        size += STEP_SIZE;
    }

    return st;
}

struct client_stat_t* exec_stest(const char* name){
    struct client_stat_t* st;

    // Allocation and initialization of the client stat structure
    if((st = (struct client_stat_t*)malloc(sizeof(struct client_stat_t))) == NULL)
        return NULL;

    st->n_request = 0;
    st->ok_request = 0;
    st->ko_request = 0;

    for(int i = 10; i < N_OBJECTS_TEST; i++){
        // Build the file name
        char* file_name = NULL;
        if((file_name = get_filename(name, i)) == NULL){
            free(st);
            return NULL;
        }

        char* ptr;
        if((ptr = os_retrieve(file_name)) == NULL && errno == 0){    // KO response
            st->n_request++;
            st->ko_request++;
        }
        else if(ptr == NULL){   // Error occurred
            perror(CLIENT_OS_RETRIEVE_ERR);
            free(st);
            free(file_name);
            exit(EXIT_FAILURE);
        }
        else{       // OK response
            st->n_request++;
            if(((strlen(ptr) + 1 - 100) % STEP_SIZE) == 0)
                st->ok_request++;
        }
        
        free(ptr);
        free(file_name);
    }

    return st;
}

struct client_stat_t* exec_ttest(const char* name){
    struct client_stat_t* st;

    // Allocation and initialization of the client stat structure
    if((st = (struct client_stat_t*)malloc(sizeof(struct client_stat_t))) == NULL)
        return NULL;

    st->n_request = 0;
    st->ok_request = 0;
    st->ko_request = 0;

    for(int i = 10; i < N_OBJECTS_TEST; i++){
        // Build the file name
        char* file_name = NULL;
        if((file_name = get_filename(name, i)) == NULL){
            free(st);
            return NULL;
        }

        int retval;
        if((retval = os_delete(file_name)) == -1){  // Error occurred
            perror(CLIENT_OS_DELETE_ERR);
            free(st);
            free(file_name);
            exit(EXIT_FAILURE);
        }
        else if(retval == 0){   // KO response
            st->n_request++;
            st->ko_request++;
        }
        else{                   // OK response
            st->n_request++;
            st->ok_request++;
        }

        free(file_name);
    }
    return st;
}

char* get_filename(const char* name, int nfile){
    char* file_name = NULL;
    char nfile_name[3];

    // +2 for the number of file-test
    // +1 for the string terminator
    if((file_name = (char*)calloc(strlen(name) + 3, sizeof(char))) == NULL)
        return NULL;

    // Build the file name
    strncpy(file_name, name, strlen(name) + 1);
    sprintf(nfile_name, "%d", nfile);
    strncat(file_name, nfile_name, strlen(nfile_name));

    return file_name;
}

void print_client_statistic(struct client_stat_t st, const char* name, int ntest){
    // Flush the buffer in order to not have a clean output video
    fflush(stdout);
    fflush(stderr);

    fprintf(stdout, "####################### %s ########################\n\n", name);
    fprintf(stdout, "Test battery : \t\t\t\t\t\t%d\n", ntest);
    fprintf(stdout, "Operations executed : \t\t\t\t%d\n", st.n_request);
    fprintf(stdout, "Operations terminated w/ OK : \t\t%d\n", st.ok_request);
    fprintf(stdout, "Operations terminated w/o OK [KO] : %d\n", st.ko_request);
    fprintf(stdout, "\n###########################################################\n\n");
}

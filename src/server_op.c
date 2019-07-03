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

#include "../include/server_op.h"

static struct server_analytics server_stat;
static volatile sig_atomic_t ending = 0;

// Custum hash table to keep tracking of the clients connected
hash_table_t* hash_table = NULL;

int initialize_server_env(pthread_t* signal_handler_th, struct sig_handler_t* handler_utils){
    struct stat st = {0};

    memset(&st, 0, sizeof(st));

    // Masking all signals
    if((sigemptyset(handler_utils->mask)) == -1)
        return 0;
    if((sigfillset(handler_utils->mask)) == -1)
        return 0;
    if((pthread_sigmask(SIG_BLOCK, handler_utils->mask, NULL)) != 0)
        return 0;
        
    // Spawn the signal-handler-thread
    if((pthread_create(signal_handler_th, NULL, &signal_handler, handler_utils)) != 0)
        return 0;
        
    // Try to delete the old file represent the socket (to bind later), no need to check the return value
    unlink(__SOCK_PATH); 

    // If not exists, create the server main folder
    if((stat(SERVER_ROOT, &st)) == -1){
        if((mkdir(SERVER_ROOT, FOLDER_MODE)) == -1)
            return 0;
    }

    // Initilize the analysis structure
    memset(&server_stat, 0, sizeof(server_stat));

    if(!set_root_info(SERVER_ROOT))
        return 0;
    server_stat.nconnected_client = 0; 
    if((pthread_mutex_init(&server_stat.mtx, NULL)) != 0)
        return 0;

    // Create the hash table
    if((hash_table = ht_create(SOMAXCONN)) == NULL)
        return 0;

    // Set the cleanup function
    if((atexit(&cleanup)) != 0)
        return 0;

    return 1;
}

void* signal_handler(void* arg){
    sigset_t* set = ((struct sig_handler_t*)arg)->mask;
    int signal_pipe = ((struct sig_handler_t*)arg)->signal_pipe;
    int sig;

    // Loop waiting for pending signal(s)
    while((sigwait(set, &sig)) == 0){
        // Handling the signal SIGINT and SIGUSR1
        switch(sig){
            // Shutdown completely the server
            case SIGINT: 
                close(signal_pipe);
                return NULL;

            // Prints server info
            case SIGUSR1:
                print_server_analytics();
                fflush(stdout);
                break;

            // For developing porpouse, can be used whenever we want
            case SIGTSTP:
                print_server_analytics();
                break;

            // Other signals 
            default:
                fprintf(stdout, "Signal %d masked\n", sig);
                break;
        }
    }
    // Should never be executed, protection statement 
    return NULL;
}

void spawn_thread(pthread_list_t* th_list, int sockfd){
    // Spawn the thread
    pthread_t thid;
    ec_sc_th((pthread_create(&thid, NULL, &connection_handler, (void*)(intptr_t)sockfd)), PTHREAD_CREATE_ERR);
    
    // Update the thread list structure by adding the new node in head
    pthread_list_t tmp = NULL;
    ec_null((tmp = (pthread_list_t)malloc(sizeof(pthread_node_t))), MALLOC_ERR);
    tmp->thid = thid;
    tmp->next = (*th_list);
    (*th_list) = tmp;
                    
    fprintf(stdout, "\t+++Client connected on socket %d\n", sockfd);
}

void set_ending(){
    ending = 1;
}

int is_ending_set(){
    return ending;
}

// #### Valgrind safe
int isdot(const char dir[]){
    int l = strlen(dir);
  
    if ((l > 0 && dir[l - 1] == '.'))   // Is dot
        return 1;
    
    return 0;   // Is not
}

// #### Valgrind safe
int set_root_info(const char* dir_path){
    // check if the argument is a directory
    struct stat st = {0};
    if((stat(dir_path, &st)) == -1)
        return 0; 
    
    DIR* directory;

    // Opening the directory to read its contents
    if((directory = opendir(dir_path)) == NULL)
        return 0;

    struct dirent* file;
    int res = 1;

    // Reading files
    while((errno = 0, file = readdir(directory)) != NULL){
        struct stat st_info = {0};
        char file_name[PATH_MAX];

        // Checks if the path is loo long
        if((strlen(dir_path) + strlen(file->d_name) + 1 + 1) > PATH_MAX){
            errno = E2BIG;
            closedir(directory);
            return 0;
        }

        // Create a usable path with the file name returned by readdir
        strncpy(file_name, dir_path, strlen(dir_path) + 1);
        strncat(file_name, "/", 2);
        strncat(file_name, file->d_name, strlen(file->d_name) + 1);

        if((stat(file_name, &st_info)) == -1){
            closedir(directory);
            return 0;
        }

        // Recursive call if the file identified is a directory
        if(S_ISDIR(st_info.st_mode)){
            if(!isdot(file_name))
                res = set_root_info(file_name);
        }
        else{   // Otherwise add the size of the regular file and increases the counter of teh files stored
            server_stat.nobject_stored++;
            server_stat.server_size += st_info.st_size;
        }
    }

    // Checks if readdir fails or ends succesfully 
    if(errno != 0){
        closedir(directory);
        return 0;
    }

    if((closedir(directory)) == -1)
        return 0;

    return (1 && res);
}

// #### Valgrind safe
void print_server_analytics(){
    // Flush the buffer in order to have a clean output video
    fflush(stdout);
    fflush(stderr);

    ec_sc_th((pthread_mutex_lock(&server_stat.mtx)), PTHREAD_LOCK_ERR);

    fprintf(stdout, "\nNumber of objects stored :   %d\n", server_stat.nobject_stored);
    fprintf(stdout, "Server size : \t\t     %ld Bytes\n", server_stat.server_size);
    fprintf(stdout, "Number of connected client : %d\n", server_stat.nconnected_client);
    
    ec_sc_th((pthread_mutex_unlock(&server_stat.mtx)), PTHREAD_UNLOCK_ERR);
}

// #### Valgrind safe
void cleanup(){
    unlink(__SOCK_PATH);
    ht_destroy(hash_table);
}

/**
 * For the following two functions: in base of the result of the send_all
 * operation print the relative result of the OK or KO strings write.
 */
void send_ok(int sockfd){
    int nsend;
    if((nsend = send_all(sockfd, SERVER_OK, strlen(SERVER_OK), 0)) == -1){
        perror(SEND_ERR);
        fprintf(stdout, "Send OK : error occurred in client on socket %d\n", sockfd);
    }
    else if(nsend == 0)
        fprintf(stdout, "Send OK : connection closed with client on socket %d\n", sockfd);
    //else
        //fprintf(stdout, "Sent OK to client on socket %d\n", sockfd);
}

void send_ko(int sockfd, char* ko_msg){
    int nsend;
    if((nsend = send_all(sockfd, ko_msg, strlen(ko_msg), 0)) == -1){
        perror(SEND_ERR);
        fprintf(stdout, "Send KO : error occurred in client on socket %d\n", sockfd);
    }
    else if(nsend == 0)
        fprintf(stdout, "Send KO : connection closed with client on socket %d\n", sockfd);
    //else
        //fprintf(stdout, "Sent KO to client on socket %d\n", sockfd);
}

void* connection_handler(void* arg){
    int data_socket = (intptr_t)arg;
    char* name = NULL;

    while(!is_ending_set()){
        char* request = NULL;
        int retval;

        // Get the header from the client connected (that starts the comunication)
        // Error(s) handling
        if((retval = get_header(data_socket, &request)) == -1){ // Error occured in get_header
            perror(GET_HEADER_ERR);
            fprintf(stderr, "Error occurred in client on socket %d with name %s\n", data_socket, name);
            exit_wrk_hd(data_socket, name, request)
        }
        else if(retval == -2){  // Conncection closed
            fprintf(stdout, "Connection closed with client on socket %d\n", data_socket);
            exit_wrk_hd(data_socket, name, request)
        }
        else if(retval == 0){   // Select timed out, no header sent
            fprintf(stdout, "Connection expired with client on socket %d\n", data_socket);
            exit_wrk_hd(data_socket, name, NULL);
        }

        //fprintf(stdout, "Client on socket %d request: %s", data_socket, request);

        // Tokening the header 
        struct msg_t* msg = NULL;
        if((msg = handle_msg(request)) == NULL){
            if(errno == EPROTO)
                fprintf(stderr, "Client on socket %d kicked : protocol violation\n", data_socket);
            else
                fprintf(stderr, "Cannot allocate memory to evaluate header : client on socket %d rejected\n", data_socket);
            perror(PROTOCOL_ERR);
            exit_wrk(msg, data_socket, name);
        }

        free(request);

        // else if nest to determine which request occurred
        if(strncmp(msg->request, CLIENT_REGISTER, strlen(CLIENT_REGISTER)) == 0){
            int retval;
            // Error(s) handling
            if((retval = server_register(data_socket, msg->name)) == -1){
                perror(SERVER_REGISTER_ERR);
                fprintf(stderr, "Registration : error occurred in client on socket %d\n", data_socket);
                exit_wrk(msg, data_socket, name);
            }
            // Client with name "name" currently connected
            else if(retval == 0){
                fprintf(stderr, "Registration : client's name already exists: client on socket %d\n", data_socket);
                send_ko(data_socket, SERVER_KO_REGISTER);
                exit_wrk(msg, data_socket, name);
            }
            else{
                if((name = (char*)calloc(strlen(msg->name) + 1, sizeof(char))) == NULL){
                    fprintf(stderr, "Registration : cannot allocate memory for client on socket %d\n", data_socket);
                    send_ko(data_socket, SERVER_KO_REG_MEM);
                    exit_wrk(msg, data_socket, name);
                }
                strncpy(name, msg->name, strlen(msg->name));
                free_header(msg);
                //fprintf(stdout, "Registration submitted successfully for client on socket %d\n", data_socket);
                send_ok(data_socket);
            }
        }
        else if(strncmp(msg->request, CLIENT_STORE, strlen(CLIENT_STORE)) == 0){
            int retval;
            // Error(s) handling
            if((retval = server_store(data_socket, name, msg->name, msg->len)) == -1){
                perror(SERVER_STORE_ERR);
                fprintf(stderr, "Store : error occurred in client on socket %d\n", data_socket);
                exit_wrk(msg, data_socket, name);
            }
            else if(retval == 0){
                fprintf(stderr, "Store : Missing byte(s) in client on socket %d\n", data_socket);
                send_ko(data_socket, SERVER_KO_STORE); // If fails the thread will exit in the next iteration
                free_header(msg);
            }
            else{
                free_header(msg);
                //fprintf(stdout, "Store submitted successfully for client on socket %d\n", data_socket);
                send_ok(data_socket);
            }
        }
        else if(strncmp(msg->request, CLIENT_RETRIEVE, strlen(CLIENT_RETRIEVE)) == 0){
            int retval;
            if((retval = server_retrieve(data_socket, name, msg->name)) == -1){
                perror(SERVER_RETRIEVE_ERR);
                fprintf(stderr, "Retrieve : error eccurred in client on socket %d\n", data_socket);
                exit_wrk(msg, data_socket, name);
            }
            else if(retval == 0){
                fprintf(stderr, "Retrieve : file \"%s\" doesn't exists for client on socket %d\n", msg->name, data_socket);
                send_ko(data_socket, SERVER_KO_RETRIEVE); 
                free_header(msg);
            }
            else{
                free_header(msg);
                //fprintf(stdout, "Retrieve submitted successfully for client on socket %d\n", data_socket);
            }
        }
        else if(strncmp(msg->request, CLIENT_DELETE, strlen(CLIENT_DELETE)) == 0){
            int retval;
            if((retval = server_delete(name, msg->name)) == -1){
                perror(SERVER_DELETE_ERR);
                fprintf(stdout, "Delete : error occurred in client on socket %d\n", data_socket);
                exit_wrk(msg, data_socket, name);
            }
            else if(retval == 0){
                fprintf(stderr, "Delete : file \"%s\" doesn't exists for client on socket %d\n", msg->name, data_socket);
                send_ko(data_socket, SERVER_KO_DELETE);
                free_header(msg);
            }
            else{
                free_header(msg);
                //fprintf(stdout, "Delete submitted successfully for client on socket %d\n", data_socket);
                send_ok(data_socket);
            }
        }
        else if(strncmp(msg->request, CLIENT_LEAVE, strlen(CLIENT_LEAVE)) == 0){
            server_leave(data_socket, name, msg);
            return NULL;
        }
        else{   // Shouldn't happen, still
            fprintf(stderr, "Protocol violation in client on socket %d\n", data_socket);
            send_ko(data_socket, SERVER_KO_PROTO);
            free_header(msg);
        }
    }
    exit_wrk_hd(data_socket, name, NULL);
}

void free_header(struct msg_t* msg){
    // Free first all the fields and then the msg structure
    if(msg != NULL){
        if(msg->request != NULL){
            free(msg->request);
        }
        if(msg->name != NULL){
            free(msg->name);
        }
        free(msg);
    }
}

void cleanup_thread(int sockfd, char* name){
    // pthread_exit before a proper client registration
    if(name == NULL){
        close(sockfd);
        return;
    }  

    int retval;
    if((retval = ht_remove(hash_table, name)) == -1)
        perror(HT_REMOVE_ERR);
    else if(retval == 0)
        fprintf(stderr, "User %s on socket %d doesn't exists in the hash table\n", name, sockfd);
    free(name);
    close(sockfd);

    ec_sc_th((pthread_mutex_lock(&server_stat.mtx)), PTHREAD_LOCK_ERR);

    server_stat.nconnected_client--;

    ec_sc_th((pthread_mutex_unlock(&server_stat.mtx)), PTHREAD_UNLOCK_ERR);
    
    fprintf(stdout, "\t---Client disconnected from socket %d\n", sockfd);
}

int server_register(int sockfd, char* name){
    // Try to insert an entry in the hash table for the new client

    int retval;
    if((retval = ht_insert(hash_table, name, sockfd)) == -1)
        return -1;  // Error(s) occurred
    else if(retval == 0)
        return 0;  // Client with the same name already present in the hash table

    // Check if exists the user folder, create if not
    struct stat st = {0};
    char client_folder[PATH_MAX];

    strncpy(client_folder, SERVER_ROOT, strlen(SERVER_ROOT) + 1);
    strncat(client_folder, "/", 2);
    strncat(client_folder, name, strlen(name) + 1);

    if((stat(client_folder, &st)) == -1){
        if((mkdir(client_folder, FOLDER_MODE)) == -1)
            return -1;
    }

    // Increases the number of current connected client 
    ec_sc_th((pthread_mutex_lock(&server_stat.mtx)), PTHREAD_LOCK_ERR);
    
    server_stat.nconnected_client++;

    ec_sc_th((pthread_mutex_unlock(&server_stat.mtx)), PTHREAD_UNLOCK_ERR);

    // Success
    return 1;
}

int server_store(int sockfd, char* name_client, char* name_file, long len){
    // Check if the client folder actually exists, if not, create it 
    struct stat st = {0};
    char client_folder[PATH_MAX];

    strncpy(client_folder, SERVER_ROOT, strlen(SERVER_ROOT) + 1);
    strncat(client_folder, "/", 2);
    strncat(client_folder, name_client, strlen(name_client) + 1);

    if((stat(client_folder, &st)) == -1){   // If the folder of the client doesn't exists (why?) create it
        if((mkdir(client_folder, FOLDER_MODE)) == -1)
            return -1;
    }

    // Check if a file "name_file" actually exists, if not, create it and proceed to write
    char client_file[PATH_MAX];

    strncpy(client_file, client_folder, strlen(client_folder) + 1);
    strncat(client_file, "/", 2);
    strncat(client_file, name_file, strlen(name_file) + 1);

    int overwritten = 1;
    int old_size = 0;
    if(stat(client_file, &st) == 0){
        overwritten = 0;
        old_size = st.st_size;
    }

    // + 1 for the space after the \n and before the data
    // + 1 for the string terminator in order to avoid undefined behavior
    char* data = NULL;
    if((data = (char*)calloc(len + 2, sizeof(char))) == NULL)
        return -1;

    // Open/create the file: aka O_RDWR | O_CREAT | O_TRUNC
    FILE* fd;
    if((fd = fopen(client_file, "wb+")) == NULL){
        free(data);
        return -1;
    }

    // Receive the data that will be stored
    int nread;
    if((nread = recv_all(sockfd, data, len + 1, 0)) == -1){
        free(data);
        fclose(fd);
        unlink(client_file);
        return -1;
    }
    else if(nread == 0){
        free(data);
        fclose(fd);
        unlink(client_file);
        return 0;
    }
    int nwrite = fwrite(data + 1, sizeof(char), len, fd);   // Writing the file
    if(nwrite != len){
        free(data);    
        fclose(fd);
        unlink(client_file);
        return 0;
    }

    free(data);
    
    // #################    STORE Alternative Version   #################

    /*
    // Read the data, no need for select, for protocol I know that there are data
    char data[DATA_EXCHANGE];
    size_t nread;
    size_t nwrite;
    size_t ntot = 0;
    memset(data, '\0', DATA_EXCHANGE);
    while(ntot < len && ((nread = recv(sockfd, data, DATA_EXCHANGE, 0)) > 0)){
        if(ntot == 0)   // Skip the space after the newline and before the data
            nwrite = fwrite(data + 1, sizeof(char), nread - 1, fd);
        else
            nwrite = fwrite(data, sizeof(char), nread, fd);
        ntot += nwrite;
        memset(data, '\0', DATA_EXCHANGE);   // "Reset" the buffer
    }
    // We don't want to have partial files
    if(nread == -1){
        fclose(fd);
        unlink(client_file);
        return -1;
    }
    // Same here, distinguishes between the case of error and the case of closing socket
    if(nread == 0 && ntot != len){
        fclose(fd);
        unlink(client_file);
        return 0;
    }
    */

    // ##################################################################

    if(fclose(fd) != 0) // Closing the file
        return -1;

    // Update the analysis structure
    ec_sc_th((pthread_mutex_lock(&server_stat.mtx)), PTHREAD_LOCK_ERR);

    server_stat.nobject_stored += overwritten;
    server_stat.server_size = server_stat.server_size - old_size + len;

    ec_sc_th((pthread_mutex_unlock(&server_stat.mtx)), PTHREAD_UNLOCK_ERR);

    return 1;
}

int server_retrieve(int sockfd, char* name_client, char* name_file){
    // Check if a file "name_file" actually exists, if not, return
    char client_file[PATH_MAX];
    struct stat st = {0};

    strncpy(client_file, SERVER_ROOT, strlen(SERVER_ROOT) + 1);
    strncat(client_file, "/", 2);
    strncat(client_file, name_client, strlen(name_client) + 1);
    strncat(client_file, "/", 2);
    strncat(client_file, name_file, strlen(name_file) + 1);

    if((stat(client_file, &st)) == -1)  // The file doesn't exists
        return 0;

    // Convert the file size (size_t) to string in order to append the size to the header 
    char file_size[SIZE_MAX];
    sprintf(file_size, "%lu", st.st_size);

    // Allocate the entire header
    char* data_buf = NULL;
    size_t data_buf_len = strlen(SERVER_RETRIEVE) + strlen(file_size) 
                + strlen(SERVER_RETRIEVE_TERM) + st.st_size;
    // +1 for the terminator (will not be written) in order to prevent undefined behavior of free
    data_buf = (char*)calloc(data_buf_len + 1, sizeof(char));
    if(data_buf == NULL)
        return -1;

    // Create the header until " \n "
    strncpy(data_buf, SERVER_RETRIEVE, strlen(SERVER_RETRIEVE));
    strncat(data_buf, file_size, strlen(file_size));
    strncat(data_buf, SERVER_RETRIEVE_TERM, strlen(SERVER_RETRIEVE_TERM));

    // Opens the file in order to read it
    FILE* fd = NULL;
    if((fd = fopen(client_file, "rb")) == NULL){
        free(data_buf);
        return -1;
    }

    /**
     * Since [7.19.8.1] provides no limitations, the values of size and nmemb 
     * can be up to the maximums provided by their type - SIZE_MAX [7.18.3.2],
     * as long as the storage pointed to by ptr has sufficient size. 
     */ 
    if((fread(data_buf + strlen(data_buf), sizeof(char), st.st_size, fd)) != st.st_size){
        free(data_buf);
        fclose(fd);
        return -1;
    }

    if((fclose(fd)) != 0){
        free(data_buf);
        return -1;
    }

    // Send the header with the data
    int retval;
    if((retval = send_all(sockfd, data_buf, data_buf_len, 0)) == -1){
        free(data_buf);
        return -1;
    }
    else if(retval == 0){       // Connection closed, missing byte(s) to the client
        free(data_buf);
        errno = ECONNABORTED;   // Distinguishes the case of closed socket and generic error(s)
        return -1;
    }

    free(data_buf);

    return 1;
}

int server_delete(char* name_client, char* name_file){
    // Check if a file "name_file" actually exists, if not, return
    char client_file[PATH_MAX];
    struct stat st = {0};

    strncpy(client_file, SERVER_ROOT, strlen(SERVER_ROOT) + 1);
    strncat(client_file, "/", 2);
    strncat(client_file, name_client, strlen(name_client) + 1);
    strncat(client_file, "/", 2);
    strncat(client_file, name_file, strlen(name_file) + 1);

    if((stat(client_file, &st)) == -1)  // The file doesn't exists
        return 0;

    // delete the file
    if((unlink(client_file)) == -1) // Cannot delete
        return -1;

    // Update the server's analysis struct
    ec_sc_th((pthread_mutex_lock(&server_stat.mtx)), PTHREAD_LOCK_ERR);

    server_stat.nobject_stored--;
    server_stat.server_size -= st.st_size;

    ec_sc_th((pthread_mutex_unlock(&server_stat.mtx)), PTHREAD_UNLOCK_ERR);

    return 1;
}

void server_leave(int sockfd, char* name, struct msg_t* msg){
    free_header(msg);
    send_ok(sockfd);
    cleanup_thread(sockfd, name);
}

void wait_thread_ending(pthread_list_t* th_list){
    pthread_list_t cur = NULL;

    // Join thread and free the list
    while((cur = (*th_list)) != NULL){
        (*th_list) = (*th_list)->next;
        pthread_join(cur->thid, NULL);
        free(cur);
    }
}

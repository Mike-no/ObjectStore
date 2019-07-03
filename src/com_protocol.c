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

#include "../include/com_protocol.h"

// #### Valgrind safe
int send_all(int sockfd, const void* buf, size_t len, int flags){
    char* ptr = (char*)buf;
    size_t bytes_left = len;
    // ssize_t because byte_sent could be negative
    ssize_t bytes_sent;
    while(bytes_left > 0){
        if((bytes_sent = send(sockfd, ptr, bytes_left, flags)) < 0){
            if(errno == EINTR){
                bytes_sent = 0;  // And call send() again
                continue;
            }
            else
                return -1;   // Error occured
        }
        else if(bytes_sent == 0)
            return 0;   // Socket closed
        ptr += bytes_sent;
        bytes_left -= bytes_sent;
    }
    return 1;
}

// #### Valgrind safe
int recv_all(int sockfd, void* buf, size_t len, int flags){
    char* ptr = (char*)buf;
    size_t bytes_left = len;
    // ssize_t because byte_sent could be negative
    ssize_t bytes_recv;
    while(bytes_left > 0){
        if((bytes_recv = recv(sockfd, ptr, bytes_left, flags)) < 0){
            if(errno == EINTR){
                bytes_recv = 0; // And call recv() again
                continue;
            }
            else
                return -1;   // Error occured
        }
        else if(bytes_recv == 0)
            return 0;   // Socket closed
        ptr += bytes_recv;
        bytes_left -= bytes_recv;
    }
    return len;
}

// #### Valgrind safe
int get_header(int sockfd, char** buf){
    // Set the timeout for the select
    struct timeval timeout;
    memset(&timeout, 0, sizeof(timeout));
    timeout.tv_sec = TIMEOUT_SEC;
    timeout.tv_usec = 0;

    fd_set rd_set;
    FD_ZERO(&rd_set);
    FD_SET(sockfd, &rd_set);

    int sel_rd;
    if((sel_rd = select(sockfd + 1, &rd_set, NULL, NULL, &timeout)) == -1)
        return -1;  // An error occurred in select
    else if(sel_rd == 0)
        return 0;  // Select timed out
        
    // #### can read from the socket
    char str[HEADER_NEWL_SIZE + 1];
    memset(str, '\0', HEADER_NEWL_SIZE + 1);

    // With MSG_PEEK the socket will not emptied
    int nread;
    if((nread = recv(sockfd, str, HEADER_NEWL_SIZE, MSG_PEEK)) == -1)
        return -1;
    else if(nread == 0)
        return -2;  // Connection closed

    // Look for the '\n' (newline)
    int found = 0;
    size_t nbytes;
    for(nbytes = 0; nbytes < strlen(str); nbytes++){
        if(str[nbytes] == '\n'){
            found = 1;
            nbytes++;   // get the number of bytes not the index
            break;
        }
    }

    /**
     * If '\n' was found allocate a buffer of nbytes + 1:
     * bytes until '\n' plus the space for the string terminator.
     */
    if(found){
        // If the header that will be read is null terminated
        if(nbytes < nread && str[nbytes] == '\0'){
            nbytes++;
            if(((*buf) = (char*)calloc(nbytes, sizeof(char))) == NULL)
                return -1;
        }
        else{   // If it isn't
            if(((*buf) = (char*)calloc(nbytes + 1, sizeof(char))) == NULL)
                return -1;
        }
        if((nread = recv_all(sockfd, *buf, nbytes, 0)) == -1)
            return -1;
        else if(nread == 0)
            return -2;  // Connection closed
        return 1;
    }
    else{   // If '\n' was not found we have a violation of the protocol
        errno = EPROTO;
        return -1;
    }
}

struct msg_t* handle_msg(char* buf){
    // Allocate the structure struct msg_t
    struct msg_t* msg = NULL;
    if((msg = (struct msg_t*)malloc(sizeof(struct msg_t))) == NULL)
        return NULL;
    memset(msg, 0, sizeof(struct msg_t));

    // Split the header
    char* saveptr;
    char* request = strtok_r(buf, STR_DELIM, &saveptr);
    char* name = strtok_r(NULL, STR_DELIM, &saveptr);
    char* len = strtok_r(NULL, STR_DELIM, &saveptr);
    char* newline = strtok_r(NULL, STR_DELIM, &saveptr);

    // Always present in an header
    if(request){
        if((msg->request = (char*)calloc(strlen(request) + 1, sizeof(char))) == NULL){
            free(msg);
            return NULL;
        }
        strncpy(msg->request, request, strlen(request));
    }

    /** 
     * The following code allows you to divide the header 
     * regardless of the type of request made
    */
    if(name && name[0] != '\n'){
        if((msg->name = (char*)calloc(strlen(name) + 1, sizeof(char))) == NULL){
            free(msg->request);
            free(msg);
            return NULL;
        }
        strncpy(msg->name, name, strlen(name));
    }

    if(len && len[0] != '\n')
        msg->len = atol(len);

    // If the last character isn't a newline the header isn't correct or complete
    if(newline && newline[0] != '\n'){
        errno = EPROTO;
        return NULL;
    }

    return msg;
}

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

#ifndef PROTOCOL_H_

#define PROTOCOL_H_

#define _POSIX_SOURCE
#define _POSIX_C_SOURCE  200112L

#include "error_control.h"
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/ioctl.h>
#include <sys/un.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <limits.h>

#define TIMEOUT_SEC             15
#define HEADER_NEWL_SIZE        512
#define DATA_EXCHANGE           1024
#define SIZE_MAX                24
#define __SOCK_PATH             "./objstore.sock"
#define CLIENT_REGISTER         "REGISTER"
#define CLIENT_STORE            "STORE"
#define CLIENT_RETRIEVE         "RETRIEVE"
#define CLIENT_DELETE           "DELETE"
#define CLIENT_LEAVE            "LEAVE"
#define SERVER_RETRIEVE         "DATA "
#define SERVER_RETRIEVE_TERM    " \n "
#define SERVER_OK               "OK \n"
#define SERVER_KO_REGISTER      "KO client's name already exists \n"
#define SERVER_KO_REG_MEM       "KO cannot allocate memory \n"
#define SERVER_KO_STORE         "KO file name already exists \n"
#define SERVER_KO_RETRIEVE      "KO file name not exists \n"
#define SERVER_KO_DELETE        "KO file name not exists \n"
#define SERVER_KO_PROTO         "KO protocol violation \n"
#define SERVER_KO               "KO "
#define HEADER_TERM             " \n"
#define STR_DELIM               " "
#define CLIENT_REGISTER_CMD     "REGISTER "
#define CLIENT_STORE_CMD        "STORE "
#define CLIENT_RETRIEVE_CMD     "RETRIEVE "
#define CLIENT_DELETE_CMD       "DELETE "
#define CLIENT_LEAVE_CMD        "LEAVE \n"
#define CLIENT_STORE_TERM       " \n "

// Struct that represent a request sent by the a client to the server
struct msg_t{
    char* request;
    char* name;
    long len;
};

/**
 * Call send function untill all data has been sent
 * @return:
 *      1 : all data sent
 *      0 : socket closed
 *      -1 : error occured, errno setted
 */
int send_all(int sockfd, const void* buf, size_t len, int flags);

/**
 * Call recv function untill all data has been received
 * @return:
 *      len : all data received
 *      0 : EOF, socket closed
 *      -1 : error occured, errno setted
 */
int recv_all(int sockfd, void* buf, size_t len, int flags);

/**
 * Get the header, if present, following the comunication protocol from
 * the socket "sockfd" and store it into "buf"
 * The string must be deallocated by the caller
 * @return:
 *      1 : header successfully got
 *      0 : select timed out, no header available
 *      -1 : error(s) occorred, errno setted
 *      -2 : connection closed
 */
int get_header(int sockfd, char** buf);

/**
 * Split the header received by insert each token in a struct msg_t structure.
 * @return:
 *      strcut msg_t* : if the subdivision has been carried out successfully
 *      NULL : if the header splitting doesn't match the protocol
 */
struct msg_t* handle_msg(char* buf);

#endif // PROTOCOL_H_

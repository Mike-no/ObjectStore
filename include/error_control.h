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

#ifndef ERROR_CONTROL_H_

#define ERROR_CONTROL_H_

#include <errno.h>

#define PIPE_SC_ERR              "pipe call"
#define SERVER_INIT              "initialize_server_env call"
#define SOCKET_ERR               "socket call"
#define BIND_ERR                 "bind call"
#define LISTEN_ERR               "listen call"
#define SELECT_ERR               "select call"
#define ACCEPT_ERR               "accept call"
#define PTHREAD_CREATE_ERR       "pthread_create call"
#define PTHREAD_LOCK_ERR         "pthread_mutex_lock call"
#define PTHREAD_UNLOCK_ERR       "pthread_mutex_unlock call"
#define GET_HEADER_ERR           "get_header call"
#define SEND_ERR                 "send/send_all call"
#define SPAWN_THREAD_ERR         "spawn_thread call"
#define PROTOCOL_ERR             "handle_masg call"
#define SHUTDOWN_ERR             "FATAL ERROR : cannot shutdown the socket"
#define CLOSE_ERR                "close call"
#define SERVER_REGISTER_ERR      "server_register call"
#define SERVER_STORE_ERR         "server_store call"
#define SERVER_DELETE_ERR        "server_delete call"
#define SERVER_RETRIEVE_ERR      "server_retrieve call"
#define CLIENT_OS_CONNECT_ERR    "os_connect call"
#define CLIENT_OS_STORE_ERR      "os_store call"
#define CLIENT_OS_RETRIEVE_ERR   "os_retrieve call"
#define CLIENT_OS_DELETE_ERR     "os_delete call"
#define CLIENT_OS_DISCONNECT_ERR "os_disconnect call"
#define MALLOC_ERR               "malloc call"
#define CALLOC_ERR               "calloc call"
#define WAIT_ERR                 "pthread_cond_wait call"
#define SIGN_ERR                 "pthread_cond_signal call"
#define HT_REMOVE_ERR            "ht_remove call"

#define system_call(fun, err_msg) if(fun == -1){ perror(err_msg); exit(EXIT_FAILURE); }
#define ec_sc_th(fun, err_msg) if(fun != 0){ perror(err_msg); exit(EXIT_FAILURE); }
#define ec_function_zero(fun, err_msg) if(fun == 0){ perror(err_msg); exit(EXIT_FAILURE); }
#define ec_null(fun, err_msg) if(fun == NULL){ perror(err_msg); exit(EXIT_FAILURE); }

#define exit_wrk_hd(data_socket, name, request){\
    if(request != NULL)\
        free(request);\
    cleanup_thread(data_socket, name);\
    return NULL;\
}

#define exit_wrk(msg, data_socket, name){\
    free_header(msg);\
    cleanup_thread(data_socket, name);\
    return NULL;\
}

#endif // ERROR_CONTROL_H_

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

#ifndef SERVER_H_

#define SERVER_H_

#include "com_protocol.h"
#include "error_control.h"
#include "hash_table.h"
#include <pthread.h>
#include <signal.h>
#include <dirent.h>

#define SERVER_ROOT "./data"
#define FOLDER_MODE 0777

/**
 * Structure to keep tracking of all the connection handler
 * threads that works with the clients.
 * No need to have a mutex because will be used only by the main server thread.
 */
struct pthread_node_s{
    pthread_t thid;
    struct pthread_node_s* next;
};
typedef struct pthread_node_s pthread_node_t;
typedef pthread_node_t* pthread_list_t;

/**
 * Struct to mantain server's statistics. The struct will be updated through
 * the lifecycle of the server by the various thread that handle the comunications
 * whit the clients, so, the struct should be accessed by one thread at a time.
 */
struct server_analytics{
    int nobject_stored;
    ssize_t server_size;
    int nconnected_client;
    pthread_mutex_t mtx;
};

// Used by the signal handler for the signal management
struct sig_handler_t{
    sigset_t* mask;
    int signal_pipe;
};

/**
 * Initialize the environment where the server will work:
 *      - masks all the signals and spawn a signal-handler-thread
 *      - delete the file that represent the socket (should not
 *        exists)
 *      - create the server main folder (if not exists)
 *      - Initialize struct server_analytics
 *      - set the cleanup function
 * @return:
 *      1 : environment initialized
 *      0 : environment not initialized, error(s) occured
 */
int initialize_server_env(pthread_t* signal_handler_th, struct sig_handler_t* handler_utils);

/**
 * Procedure executed by a signal-handler-thread that will take care of the
 * management of all the signals that the server receives.
 */
void* signal_handler(void* arg);

// Spawn the thread that handle the connection whit the client "data_socket"and update the thread list
void spawn_thread(pthread_list_t* th_list, int sockfd); 

// Set ending to begin the shutdown procedure
void set_ending();

// Return the status of the static variable ending
int is_ending_set();

// Simply check if the directory passed is "." or ".."
int isdot(const char dir[]);

/** 
 * Size of the server main folder and number of file stored
 * @return:
 *      1 : server size and number of files stored successfully calculated
 *      0 : error(s) occured
*/
int set_root_info(const char* dir_path);

// When SIGUSR1 is received the signal-handler-thread will call print_server_analytics
void print_server_analytics();

/**
 * At exit clean the server workspace:
 *      - delete the file that represent the socket 
 *      - free the hash table structure that keep tracking of the users
 */
void cleanup();

// Send "OK \n" to the client "sockfd"
void send_ok(int sockfd);

// Send "KO "ko_msg" \n" to the client "sockfd"
void send_ko(int sockfd, char* ko_msg);

/**
 * Each instance of this function, executed by all the client threads,
 * handle the comunications whit their own partner.
 * Implements the comunication protocol based on ASCII Header.
 */
void* connection_handler(void* arg);

// Free a msg structure and all it's fields
void free_header(struct msg_t* msg);

/**
 * At pthread_exit clean the thread handler's workspace:
 *      - remove the client from the connected clients list
 *      - decreases the number of connected clients
 *      - close data socket
 *      - prints the leave message
 */
void cleanup_thread(int sockfd, char* name);

/**
 * Registers a client to the server; if a client called "name" exists the
 * registration fails, otherwise, the client being registered.
 * If not exists, create a folder for the client's files.
 * @return:
 *      1 : the client has been registered
 *      0 : a client "name" already exist, registration failed
 *      -1 : error(s) occurred, errno setted
 */
int server_register(int sockfd, char* name);

/**
 * Stores a "len" amount of data in a file "name_file" in ./data/"name_client" folder
 * If the file altrady exists, overwrite it.
 * @return:
 *      1 : the file has been stored
 *      0 : file not stored, missing bytes
 *      -1 : error(s) occurred, errno setted
 */
int server_store(int sockfd, char* name_client, char* name_file, long len);

/**
 * Get, if exists, the file "name_file" from "./data/"name_client"" folder
 * the will be return with the format "DATA "len" \n data"
 * @return:
 *      1 : the file has been retrieved
 *      0 : the file doesn't exists in the client folder
 *      -1 : error(s) occurred, socket closed, errno setted
 */
int server_retrieve(int sockfd, char* name_client, char* name_file);

/**
 * If exists, delete the file "name" from the client folder ./data/"name_client"
 * @return:
 *      1 : the file has been deleted
 *      0 : the file doesn't exists in the client folder or if the file 
 *          can't be removed
 *      -1 : error(s) occurred, errno setted
 */
int server_delete(char* name_client, char* name_file);

/**
 * Close the connection with the client, free resources, send the 
 * "OK \n" message, then, call pthread_exit.
 */
void server_leave(int sockfd, char* name, struct msg_t* msg);

// Join all the connection thread that has been spawn during the execution
void wait_thread_ending(pthread_list_t* th_list);

#endif // SERVER_H_

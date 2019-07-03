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

#ifndef CLIENT_H_

#define CLIENT_H_

#include "com_protocol.h"
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>

#define N_OBJECTS_TEST   20
#define INIT_SIZE        100
#define STEP_SIZE        5258
#define INIT_CHAR        97

struct client_stat_t{
    int n_request;
    int ok_request;
    int ko_request;
};

// #################################################################################

/**
 * Following the comunication protocol the header (or header + date)
 * aren't NULL terminated ('\0'), so, the header that the client will send
 * will not.
 * The get_header function (see com_protocol.h / .c) and the composition
 * of the request header, anyway, will return a
 * null terminated header in order to operate with it without any kind of
 * errors in valgrind and/or undefined behavior.
 */

// #################################################################################

/**
 * Start the connection with the Object Store subscribing the
 * client with the given name.
 * @return:
 *      1 : connection established
 *      0 : connection refused, name already exists;
 *      -1 : error during the connection, errno setted
 */
int os_connect(char* name);

/**
 * Requires store of the object pointed by "block" into the Object Store;
 * the object has name "name" and length "len".
 * @return:
 *      1 : object stored
 *      0 : object not stored
 *      -1 : error during the store operations, errno setted
 */
int os_store(char* name, void* block, size_t len);

/**
 * Retrieve the objetc "name" (previously stored) from the Object Store.
 * @return: pointer to the requested object, NULL if the object doesn't
 *          exist.
 *          If an error occured return NULL and set errno.
 */
void* os_retrieve(char* name);

/**
 * Delete the object "name" (previously stored) from the Object Store.
 * @return:
 *      1 : object deleted
 *      0 : object not deleted
 *      -1 : error during the delete operations, errno setted 
 */
int os_delete(char* name);

/**
 * Close the connection with the Object Store.
 * @return:
 *      1 : connection closed
 *      0 : connection not closed
 *      -1 : error during the disconnect operations, socket closed, errno setted
 */
int os_disconnect();

/**
 * Send "request" of length "len" to the server
 * @return:
 *      1 : request successfully sent
 *      0 : request not sent
 */
int send_request(char* request, size_t len);

/**
 * @return:
 *      1 : is "OK \n"
 *      0 : is "KO "message" \n"
 *      -1 : no response received,
 *           error(s) occurred, errno setted
 */
int evaluate_response();

/**
 * Execute test type 1 of the project datasheet
 * @return:
 *      struct client_stat_t* : pointer to the structure with the
 *          results of the eleborations
 *      NULL : error(s) occurred, errno setted
 * If os_store return -1 the process exit.
 */
struct client_stat_t* exec_ftest(const char* name);

/**
 * Execute test type 2 of the project datasheet
 * @return:
 *      struct client_stat_t* : pointer to the structure with the
 *          results of the eleborations
 *      NULL : error(s) occurred, errno setted
 * If os_retrieve return -1 the process exit.
 */
struct client_stat_t* exec_stest(const char* name);

/**
 * Execute test type 3 of the project datasheet
 * @return:
 *      struct client_stat_t* : pointer to the structure with the
 *          results of the eleborations
 *      NULL : error(s) occurred, errno setted
 * If os_delete return -1 the process exit.
 */
struct client_stat_t* exec_ttest(const char* name);

/**
 * Given a name and an integer return the composition of the two that
 * will be used for the creation of a file in the server
 * @return:
 *      char* : string composed
 *      NULL : error(s) occurred 
 */
char* get_filename(const char* name, int nfile);

// Simply prints the fields of the structure
void print_client_statistic(struct client_stat_t st, const char* name, int ntest);

#endif // CLIENT_H_

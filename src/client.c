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

int main(int argc, char* argv[]){
    // Arguments name and test-number needed
    if(argc != 3){
        fprintf(stderr, "Use : %s < username > [ 1 2 3 ]\n", argv[0]);      
        exit(EXIT_FAILURE);
    }

    int retval;
    system_call((retval = os_connect(argv[1])), CLIENT_OS_CONNECT_ERR);     // Try to connect 
    if(retval == 0)
        exit(EXIT_FAILURE);

    struct client_stat_t* client_stat = NULL;

    int ntest = atoi(argv[2]);
    switch(ntest){
        case 1:
            if((client_stat = exec_ftest(argv[1])) == NULL)   // os_store of 20 blocks of data
                perror(CLIENT_OS_STORE_ERR);                  // then break; and close socket
            else
                print_client_statistic(*client_stat, argv[1], ntest);
            break;

        case 2:
            if((client_stat = exec_stest(argv[1])) == NULL)    // Retrieve some files and check of their integrity with os_retrieve
                perror(CLIENT_OS_RETRIEVE_ERR);                // then break; and close socket
            else
                print_client_statistic(*client_stat, argv[1], ntest);
            break;

        case 3:
            if((client_stat = exec_ttest(argv[1])) == NULL)    // Delete some files with os_delete
                perror(CLIENT_OS_DELETE_ERR);                  // then break; and close socket
            else
                print_client_statistic(*client_stat, argv[1], ntest);
            break;

        default:
            fprintf(stderr, "Use : %s <username > [ 1 2 3 ]\n", argv[0]);
            system_call(os_disconnect(), CLIENT_OS_DISCONNECT_ERR);
            exit(EXIT_FAILURE);
    }

    if(client_stat != NULL)
        free(client_stat);

    system_call((retval = os_disconnect()), CLIENT_OS_DISCONNECT_ERR);      // Should always work

    return 0;
}

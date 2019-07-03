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

int main(int argc, char* argv[]){
    // Initializes the Server
    sigset_t mask;
    pthread_t signal_handler_th;

    int signal_pipe[2];
    system_call(pipe(signal_pipe), PIPE_SC_ERR);

    struct sig_handler_t handler_utils = { &mask, signal_pipe[1] };

    ec_function_zero((initialize_server_env(&signal_handler_th, &handler_utils)), SERVER_INIT);

    // Creation of the listen socket of the Server
    int connection_socket = -1;
    system_call((connection_socket = socket(AF_UNIX, SOCK_STREAM, 0)), SOCKET_ERR);

    // Initializes a struct sockaddr_un and bind an address to the socket
    struct sockaddr_un serv_addr;
    memset(&serv_addr, '0', sizeof(serv_addr));
    serv_addr.sun_family = AF_UNIX;    
    strncpy(serv_addr.sun_path, __SOCK_PATH, strlen(__SOCK_PATH) + 1);
    system_call((bind(connection_socket, (struct sockaddr*)&serv_addr, sizeof(serv_addr))), BIND_ERR);
    // After the listen the socket will ready to accept incoming connection
    system_call((listen(connection_socket, SOMAXCONN)), LISTEN_ERR);

    fd_set set;
    fd_set rdset;
    FD_ZERO(&set);
    FD_ZERO(&rdset);
    FD_SET(connection_socket, &set);
    FD_SET(signal_pipe[0], &set);
    // Set the maximum descriptor 
    int fd_max = (connection_socket > signal_pipe[0]) ? connection_socket : signal_pipe[0];

    pthread_list_t th_list = NULL;

    fprintf(stdout, "####### Server online #######\n\n");

    // Accepts main loop
    while(!is_ending_set()){
        rdset = set;
        // Deciding if we must read from the connection socket or the signal pipe because a signal occurred
        system_call(select(fd_max + 1, &rdset, NULL, NULL, NULL), SELECT_ERR);
        for(int i = 0; i <= fd_max; i++){
            if(FD_ISSET(i, &rdset)){
                // A new connection request arrived
                if(i == connection_socket){
                    int data_socket;
                    // There is no reason to check EINTR because all signals are masked
                    system_call((data_socket = accept(connection_socket, (struct sockaddr*)NULL, NULL)), ACCEPT_ERR);
                    spawn_thread(&th_list, data_socket);    // Spawn a connection handler thread
                }
                // Time to shutdown
                if(i == signal_pipe[0]){
                    set_ending();
                    break;
                }
            }
        }
    }
    
    // Join all the connection handler threads
    wait_thread_ending(&th_list);
    // Join the signal handler thread
    pthread_join(signal_handler_th, NULL);

    close(connection_socket);

    fprintf(stdout, "\n\n####### Server offline #######\n");

    return 0;   // Call cleanup
}

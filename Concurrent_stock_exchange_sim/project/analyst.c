#define _XOPEN_SOURCE 600
#include <stdio.h>
#include "get_command.h"
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <sys/types.h>
#include <signal.h>
#include <errno.h>
#include "special_io.h"


void client_communication_analyst(command_client *cmd);

int main(int argc, char *argv[]) {

    command_client cmd;
    // struct sigaction sig;

    // sig.sa_flags = 0;
    // sig.sa_handler = SIG_IGN;
    // sigemptyset(&sig.sa_mask);
    
    // if(sigaction(SIGINT, &sig, NULL) == -1) {
    //     fprintf(stderr, "sigaction error.\n");
    //     exit(EXIT_FAILURE);
    // }
   
    get_arguments_clients(argc, argv, &cmd);

    client_communication_analyst(&cmd);


    return 0;
}


void client_communication_analyst(command_client *cmd) {

    int sock_fd;
    struct sockaddr_in sock_addr;
    char message[512];
    int num_read;
    int cnt = 0;
    char *status;
    char msg[512];

    sock_fd = socket(AF_INET, SOCK_STREAM, 0);

    if(sock_fd == -1) {
        fprintf(stderr, "socket is failed.\n");
        exit(EXIT_FAILURE);
    }

    memset(&sock_addr, 0, sizeof(sock_addr));

    sock_addr.sin_family = AF_INET;
    sock_addr.sin_port = htons(cmd->tcp_port);

    inet_pton(AF_INET, cmd->server_ip, &sock_addr.sin_addr.s_addr);

    if(connect(sock_fd, (struct sockaddr*) &sock_addr, sizeof(sock_addr)) == -1) {
        fprintf(stderr, "connect is failed.\n");
        exit(EXIT_FAILURE);
    }

    
    sprintf(message, "JOIN ANALYST %s\n", cmd->username);
    special_write(sock_fd, message, strlen(message));
    printf("[ANALYST %s] CONNECTED server=%s:%d.\n", cmd->username, cmd->server_ip, cmd->tcp_port);
    printf("[ANALYST %s] SENT JOIN\n", cmd->username);

    cnt = special_read(sock_fd, message, 512, &num_read);

    if(num_read > 0) {

        message[cnt] = '\0';

        printf("[ANALYST %s] RECEIVED %s", cmd->username, message);
    

        fd_set read_fds;
        int done = 0;

        while(1) {

            FD_ZERO(&read_fds);
            FD_SET(STDIN_FILENO, &read_fds);
            FD_SET(sock_fd, &read_fds);

            int num_ready = select(sock_fd + 1, &read_fds, NULL, NULL, NULL);

            if(num_ready == -1 && errno == EINTR) {

                fprintf(stderr, "select.\n");
                close(sock_fd);
                exit(EXIT_FAILURE);
            }

            if(FD_ISSET(STDIN_FILENO, &read_fds)) {
                if((status = fgets(message, 512, stdin)) == NULL && feof(stdin)) {
                    special_write(sock_fd, "QUIT\n", 5);
                    printf("[ANALYST %s] DISCONNECTED reason=QUIT\n", cmd->username);
                    done = 1;
                    // close(sock_fd);
                    // break; // ctrl - D
                }
                else if(status == NULL) {
                    fprintf(stderr, "fgets error.\n");
                    close(sock_fd);
                    break;
                }

                if(!done) {
                    special_write(sock_fd, message, strlen(message));
                    printf("[ANALYST %s] SENT %s", cmd->username, message);
                }
                
            }

            
            if(FD_ISSET(sock_fd, &read_fds)) {

                cnt = special_read(sock_fd, message, 512, &num_read);

                if(num_read <= 0) {
                    fprintf(stderr, "special read error.\n");
                    close(sock_fd);
                    break;
                }

                message[cnt] = '\0';

                if(strcmp(message, "SERVER SHUTDOWN\n") == 0) {
                    printf("[ANALYST %s] RECEIVED %s", cmd->username, message);
                    printf("[ANALYST %s] DISCONNECTED reason=shutdown\n", cmd->username);
                    close(sock_fd);
                    break; // ctrl - D
                }

                
                sprintf(msg, "ERR JOIN %s\n", cmd->username);
                if(strcmp(message, msg) == 0) {
                    printf("[TRADER %s] RECEIVED %s", cmd->username, message);
                    printf("[TRADER %s] DISCONNECTED reason=err_join\n", cmd->username);
                    close(sock_fd);
                    break; // err_join
                }

                printf("[ANALYST %s] RECEIVED %s", cmd->username, message);

            }
            if(done) {
                close(sock_fd);
                break; // ctrl - D
            }
        }
    }
    else {
        fprintf(stderr, "server could not be connected.\n");
        close(sock_fd);
    }

    
}
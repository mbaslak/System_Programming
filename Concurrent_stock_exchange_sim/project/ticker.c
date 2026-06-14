#define _GNU_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <string.h>
#include <signal.h>

#define MESSAGE_LINE 512

volatile sig_atomic_t done = 0;

void quit() {
    done = 1;
}


int main(int argc, char *argv[]) {

    struct sockaddr_in sock_addr;
    int sockfd;
    int rev;
    struct sigaction handler;
    char buf[MESSAGE_LINE];
    //socklen_t len;
    int udp_port;
    int enable = 1;

    if(argc != 2 || sscanf(argv[1], "%d", &udp_port) != 1) {
        fprintf(stderr, "Please, enter proper command line.\n");
        return 1;
    }

    handler.sa_handler = quit;
    sigemptyset(&handler.sa_mask);
    handler.sa_flags = 0;

    if(sigaction(SIGINT, &handler, NULL) == -1) {
        fprintf(stderr, "Sigaction is failed.\n");
        exit(EXIT_FAILURE);
    }

    memset(&sock_addr, 0, sizeof(sock_addr));

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    if(sockfd == -1) {
        fprintf(stderr, "Socket is failed.\n");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable)) == -1) {
        fprintf(stderr, "setsockopt 1 is failed.\n");
        return 1;
    }

    
    sock_addr.sin_addr.s_addr = INADDR_ANY;
    sock_addr.sin_family = AF_INET;
    sock_addr.sin_port = htons(udp_port);

    if(bind(sockfd, (struct sockaddr*)&sock_addr, sizeof(sock_addr)) < 0) {
        fprintf(stderr, "bind is failed.\n");
        return 1;
    }

    //len = sizeof(sock_addr);

    printf("[TICKER] LISTENING udp_port=%d\n", udp_port);

    while(!done) {

       // printf("mmm\n");
        
        rev = recvfrom(sockfd, buf, MESSAGE_LINE, 0, NULL, NULL);

        if(rev == -1 && !done) {
            fprintf(stderr, "recvfrom error.\n");
            break;
        }

        if(!done) {
            printf("[TICKER] RECEIVED %s", buf);
        }
        

    }

    close(sockfd);

    return 0;
}
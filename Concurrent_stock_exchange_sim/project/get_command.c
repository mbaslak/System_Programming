#define _GNU_SOURCE
#include "get_command.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

extern int optind, optopt, opterr;
extern char *optarg;

void get_arguments_clients(int argc, char *argv[], command_client *cmd) {

    if(argc != 4) {
        fprintf(stderr, "Invalid command line.\n");
        exit(EXIT_FAILURE);
    }

   
    strcpy(cmd->server_ip, argv[1]);
    strcpy(cmd->username, argv[3]);
    
    cmd->tcp_port = command_extract_for_digit(argv[2]);
    
}


void get_arguments_server(int argc, char *argv[], command_server *cmd) {

       
    
    
    int opt;
    int num;
    int flag_p = 0;
    int flag_u = 0;
    int flag_s = 0;
    int flag_l = 0;

    

    while((opt = getopt(argc, argv, ":p:u:s:l:x")) != -1) {

        switch(opt) {
            case 'p': 
                    if(flag_p == 1) {
                        fprintf(stderr, "You should use -p flag once\n");
                    }
                    else {
                        flag_p = 1;
                        num = command_extract_for_digit(optarg);
                        
                        if(num < 1024) {
                            fprintf(stderr, "The port number cannot be smaller than 1024.\n");
                            exit(EXIT_FAILURE);
                        }
                        cmd->tcp_port = num;
                    }break;
            case 's':
                    if(flag_s == 1) {
                        fprintf(stderr, "You should use -s flag once\n");
                    }
                    else {
                        flag_s = 1;
                        strcpy(cmd->stocks, optarg);
                    }break;
            case 'l':
                    if(flag_l == 1) {
                        fprintf(stderr, "You should use -l flag once\n");
                    }
                    else {
                        flag_l = 1;
                        strcpy(cmd->log_file, optarg);
                    }break;
            case 'u': 
                    if(flag_u == 1) {
                        fprintf(stderr, "You should use -u flag once\n");
                    }
                    else {
                        flag_u = 1;
                        num = command_extract_for_digit(optarg);
                        
                        if(num < 1024) {
                            fprintf(stderr, "The port number cannot be smaller than 1024.\n");
                            exit(EXIT_FAILURE);
                        }
                        cmd->udp_port = num;
                    }break;
           
            case '?':
                    fprintf(stderr, "Invalid argument.\n");
                    exit(EXIT_FAILURE); break;
            case ':':
                    fprintf(stderr, "Missing argument.\n");
                    exit(EXIT_FAILURE); break;
            
            default:
                    fprintf(stderr, "Unacceptable flag.\n");
                    exit(EXIT_FAILURE); break;
        }
    }

    if(flag_p == 0) {
        fprintf(stderr, "You should use p flag.\n");
        exit(EXIT_FAILURE);
    }
    if(flag_u == 0) {
        fprintf(stderr, "You should use -u flag.\n");
        exit(EXIT_FAILURE);
    }
    if(flag_s == 0) {
        fprintf(stderr, "You should use -s flag.\n");
        exit(EXIT_FAILURE);
    }
    if(flag_l == 0) {
        fprintf(stderr, "You should use -l flag.\n");
        exit(EXIT_FAILURE);
    }
   
}


int command_extract_for_digit(char *command) {

    int status;
    long int number;
    int flag = 0;
    while(!flag) {
        status = sscanf(command, "%ld", &number);
        if(status != 1) {
            fprintf(stderr, "Please, enter a integer number\n");
            exit(EXIT_FAILURE);
        }
        else flag = 1;
    }

    return number;
}

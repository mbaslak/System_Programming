#ifndef GET_COMMAND_H
#define GET_COMMAND_H

#define MAX_PATH_LENGTH 70
#define MAX_IP_LENGTH 16
#define MAX_NAME_LENGTH 30

typedef struct{
    int tcp_port;
    int udp_port;
    char stocks[MAX_PATH_LENGTH];
    char log_file[MAX_PATH_LENGTH];
    //int max_clients;
    //int timeout;
} command_server;

typedef struct{
    char server_ip[MAX_IP_LENGTH];
    int tcp_port;
    char username[MAX_NAME_LENGTH];
} command_client;


void get_arguments_server(int argc, char *argv[], command_server *cmd);
int command_extract_for_digit(char *command);
void get_arguments_clients(int argc, char *argv[], command_client *cmd);

#endif
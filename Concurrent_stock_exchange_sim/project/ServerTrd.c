#define _GNU_SOURCE
#define _POSIX_SOURCE
#include <stdio.h>
#include "get_command.h"
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <ctype.h>
#include "special_io.h"



#define BACKLOG 10
#define MESSAGE_SIZE 512
#define SYMBOL_SIZE 8
#define COMMUNICATION_ERROR -213


#define MAX_CLIENT 64

typedef struct {
    char symbol[SYMBOL_SIZE];
    int quantity;
} stock_t;

typedef struct{
    char symbol[SYMBOL_SIZE];
    int price;  // * 100
} stock_global_t;

typedef struct {
    int fd;
    char username[32];
    char type[16];
    char line_buf[512]; /* partial read buffer */
    char *tok_buf;
    stock_t *portfolio;
    int size;
    int cnt;
    int start;
} client_t;

stock_global_t *global_stock; // It should be freed.
int stock_size = 0;
volatile sig_atomic_t done = 0;

void shutdown_switch();
void initialize_global_stock(char *ingredients);
int server_communication(command_server *cmd, client_t *clients_fd, FILE *log);
void special_write(int fd, char *buf, int size);
void message_handling(client_t *client, int index, client_t *clients, FILE *log, int udp_fd, struct sockaddr_in *broadcast_addr);
void clean_portfolios(client_t *clients);

int main(int argc, char *argv[]) {

    command_server cmd;
    client_t *clients;
    FILE *log;
    int flag;
    struct sigaction quit_handling;
    //int live_clients = 0;

    quit_handling.sa_handler = shutdown_switch;
    sigemptyset(&quit_handling.sa_mask);
    quit_handling.sa_flags = 0;

    if(sigaction(SIGINT, &quit_handling, NULL) == -1) {
        fprintf(stderr, "Sigaction is failed.\n");
        exit(EXIT_FAILURE);
    }

    get_arguments_server(argc, argv, &cmd);

    initialize_global_stock(cmd.stocks);

    log = fopen(cmd.log_file, "w");

    if(log == NULL) {
        fprintf(stderr, "The log file could not be opened.\n");
        free(global_stock);
        exit(EXIT_FAILURE);
    }

    printf("[SERVER] SERVER_START tcp_port=%d udp_port=%d stocks=%d\n", cmd.tcp_port, cmd.udp_port, stock_size);
    
    fprintf(log, "[SERVER] SERVER_START tcp_port=%d udp_port=%d stocks=%d\n", cmd.tcp_port, cmd.udp_port, stock_size);
    clients = (client_t*)malloc(MAX_CLIENT * sizeof(client_t));

    for(int i = 0; i < MAX_CLIENT; i++) {
        clients[i].portfolio = NULL;
        clients[i].tok_buf = NULL;
        clients[i].cnt = 0;
        clients[i].start = 0;
    }

    flag = server_communication(&cmd, clients, log);

    if(flag == COMMUNICATION_ERROR) {
        clean_portfolios(clients);
        free(clients);
        free(global_stock);
        fclose(log);
        exit(EXIT_FAILURE);
    }

    // for(int i = 0; i < MAX_CLIENT; i++) {
    //     if(clients[i].fd != 0) live_clients++;
    // }

    
    clean_portfolios(clients);
   
    free(clients);
    free(global_stock);

    printf("[SERVER] CLEANUP_DONE clients=0\n");
    fprintf(log, "[SERVER] CLEANUP_DONE clients=0\n");

    fclose(log);
   
    return 0;
}

void clean_portfolios(client_t *clients) {

    for(int i = 0; i < MAX_CLIENT; i++) {

        if(clients[i].fd != 0 && strcmp(clients[i].type, "TRADER") == 0) {
            free(clients[i].portfolio);
        }
    }
}

void shutdown_switch() {

    done = 1;

}

void initialize_global_stock(char *stocks) {

    FILE *fp;
    char stc[SYMBOL_SIZE];
    int status;
    double price;
    char c;
    int counter = 0;
    int i;
    int flag;
    int size;

    fp = fopen(stocks, "r");

    if(fp == NULL) {
        fprintf(stderr, "The stocks file could not be opened.\n");
        exit(EXIT_FAILURE);
    }

    while((c = fgetc(fp)) != EOF) {
        if(c == '\n') counter++;
    }
    counter++;

    stock_size = counter;

    global_stock = (stock_global_t*)malloc(stock_size * sizeof(stock_global_t));

    rewind(fp);

    i = 0;
    while(i < counter) {
        if((status = fscanf(fp, "%s%lf", stc, &price)) != 2) {
            while((c = fgetc(fp)) != '\n');
            continue;
        }
        

        flag = 0;
        size = strlen(stc);
        for(int i = 0; i < size && !flag; i++) {
            if(isalnum(stc[i])) {
                if(!isupper(stc[i])) flag = 1;
            }
            else flag = 1;
        }
        if(flag) {
            while((c = fgetc(fp)) != '\n');
            continue;
        }

        strcpy(global_stock[i].symbol, stc);
        global_stock[i].price = (int) (price * 100);

        i++;

        if(i != counter) while((c = fgetc(fp)) != '\n');

    }

    
    
    fclose(fp);

}

int server_communication(command_server *cmd, client_t *clients, FILE *log) {

    int server_fd;
    int fd;
    int udp_fd;
    struct sockaddr_in server_addr;
    struct sockaddr_in client_addr;
    struct sockaddr_in broadcast_addr;
    struct sockaddr_in addr;
    socklen_t len;
    socklen_t client_addr_len;
    fd_set read_fds;
    //char message[MESSAGE_SIZE];
    int max_fd;
    int status;
    int client_fd;
    int read_var;
    int i;
    char ip[10];
    int enable = 1;
    char buf[MESSAGE_SIZE + 16];
    //char *tok;


    struct timeval tm;
    time_t last = 0;
    time_t remaining = 5;
    time_t past_time;
   
    len = sizeof(addr);
    
    for(i = 0; i < MAX_CLIENT; i++) {
        clients[i].fd = 0;
    }

    if((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        fprintf(stderr, "socket is failed.\n");
        return COMMUNICATION_ERROR;
    }

    if((udp_fd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
        fprintf(stderr, "socket is failed.\n");
        return COMMUNICATION_ERROR;
    }

    if(setsockopt(udp_fd, SOL_SOCKET, SO_BROADCAST, &enable, sizeof(enable)) == -1) {
        fprintf(stderr, "setsockopt 1 is failed.\n");
        return COMMUNICATION_ERROR;
    }

    if(setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable)) == -1) {
        fprintf(stderr, "setsockopt 2 is failed.\n");
        return COMMUNICATION_ERROR;
    }

    memset(&broadcast_addr, 0, sizeof(broadcast_addr));
    memset(&server_addr, 0, sizeof(server_addr));

    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(cmd->tcp_port);

    broadcast_addr.sin_family = AF_INET;
    broadcast_addr.sin_port = htons(cmd->udp_port);
    inet_pton(AF_INET, "255.255.255.255", &broadcast_addr.sin_addr.s_addr);

    if(bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        close(server_fd);
        close(udp_fd);
        fprintf(stderr, "bind 1 is failed.\n");
        return COMMUNICATION_ERROR;
    }


    // sendto(udp_fd, "merhaba", strlen("merhaba"), 0, (struct sockaddr*) &broadcast_addr, sizeof(broadcast_addr));

    if(listen(server_fd, BACKLOG) == -1) {
        fprintf(stderr, "listen is failed.\n");
        return COMMUNICATION_ERROR;
    }
    getsockname(server_fd, (struct sockaddr *)&addr, &len);
    last = time(NULL);
    while(1) {
       
        FD_ZERO(&read_fds);
        FD_SET(server_fd, &read_fds);
       
        max_fd = server_fd;

        for(i = 0; i < MAX_CLIENT; i++) {

            fd = clients[i].fd;
            if(clients[i].fd > 0)  {
                FD_SET(fd, &read_fds);
            }

            if(fd > max_fd) max_fd = fd;
        }


        past_time = time(NULL) - last;
        remaining = remaining - past_time;

        if(remaining <= 0) remaining = 5;

        last = time(NULL);

        tm.tv_sec = remaining;
        tm.tv_usec = 0;
        
        status = select(max_fd + 1, &read_fds, NULL, NULL, &tm);

        if(status == -1  && errno == EINTR) {
            if(done == 1) {
                fprintf(log, "[SERVER] SHUTDOWN  signal=SIGINT\n");
                printf("[SERVER] SHUTDOWN  signal=SIGINT\n");

                for(i = 0; i < MAX_CLIENT; i++) {
                    fd = clients[i].fd;
                    if(fd == 0) continue;
                    special_write(fd, "SERVER SHUTDOWN\n", 16);
                    close(fd);
                    
                }
                close(udp_fd);
                close(server_fd);
                break;

            }
            else {
                printf("errno : %d\n", errno);
                fprintf(stderr, "select is failed.\n");
                return COMMUNICATION_ERROR;
            }
            
        }
        else if(status == -1 && errno != EINTR) {
            printf("errno : %d\n", errno);
            fprintf(stderr, "select is failed.\n");
            return COMMUNICATION_ERROR; 
        }

        if(!done && status == 0) {
            int len = 0;
            len += sprintf(buf + len, "PRICE_UPDATE ");
            for(int i = 0; i < stock_size; i++) {
                len += sprintf(buf + len, "%s:%.2f ", global_stock[i].symbol, (float) global_stock[i].price / 100);
            }
            len += sprintf(buf + len, "\n");
            sendto(udp_fd, buf, strlen(buf), 0, (struct sockaddr *) &broadcast_addr, sizeof(broadcast_addr));

            printf("[SERVER] PRICE_BROADCAST trigger=PERIODIC\n");
            fprintf(log, "[SERVER] PRICE_BROADCAST trigger=PERIODIC\n");
        
        }

        if(!done && FD_ISSET(server_fd, &read_fds)) {

            client_addr_len = sizeof(client_addr);

            client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_addr_len);
            
            if(client_fd == -1) {
                fprintf(stderr, "accept is failed.\n");
                return COMMUNICATION_ERROR;
            }

            if(inet_ntop(AF_INET, &(client_addr.sin_addr), ip, INET_ADDRSTRLEN) == NULL) {
                fprintf(stderr, "inet_ntop error.\n");
                continue;
            }

            printf("[SERVER] CLIENT_CONNECTED. fd=%d ip=%s\n", client_fd, ip);
            fprintf(log, "[SERVER] CLIENT_CONNECTED fd=%d ip=%s\n", client_fd, ip);


            for(i = 0; i < MAX_CLIENT; i++) {

                if(clients[i].fd == 0) {
                    clients[i].fd = client_fd;
                    break;
                }
            }

            if(i == MAX_CLIENT) {
                special_write(client_fd, "ERR SERVER_FULL", 16);
                close(client_fd);
                fprintf(log, "[SERVER] REJECTED fd=%d ip=%s reason=SERVER_FULL\n", client_fd, ip);
                fflush(log);

            }

        }


        for(i = 0; i < MAX_CLIENT; i++) {

            fd = clients[i].fd;

            if(fd == 0) continue;

            if(FD_ISSET(fd, &read_fds)) {

                if(clients[i].cnt > MESSAGE_SIZE) {
                    special_write(clients[i].fd, "ERR TOOLONG\n", 12);
                    memset(clients[i].line_buf, 0, sizeof(clients[i].line_buf));
                    clients[i].cnt = 0;
                    continue;
                }
               
                read_var = read(fd, buf, MESSAGE_SIZE - 1);

                clients[i].cnt += read_var;

                if(read_var == 0) {
                    printf("[SERVER] CLIENT_DISCONNECTED username=%s reason=hangup\n", clients[i].username);
                    close(fd);
                    clients[i].fd = 0;

                    if(strcmp(clients[i].type, "TRADER") == 0 && clients[i].portfolio != NULL) {
                        free(clients[i].portfolio);
                        clients[i].portfolio = NULL;
                        
                    }

                    memset(clients[i].line_buf, 0, sizeof(clients[i].line_buf));
                    clients[i].cnt = 0;
                    
                    clients[i].start = 0;

                }
                else {

                    buf[read_var] = '\0';
                    clients[i].line_buf[clients[i].cnt] = '\0';
                    strcat(clients[i].line_buf, buf);

                    if(strchr(clients[i].line_buf, '\n') == NULL) continue;

                    //clients[i].line_buf[clients[i].cnt] = '\0';
                    //printf("the message from client %d: %s\n", fd, clients[i].line_buf);

                    if(strcmp(clients[i].line_buf, "\n") == 0) {
                        sprintf(buf, "ERR UNKNOWN %s\n", clients[i].line_buf);
                        special_write(clients[i].fd, buf, strlen(buf));
                    }

                    else {

                        clients[i].tok_buf = strtok(clients[i].line_buf, "\n");

                        while(clients[i].tok_buf != NULL) {

                            if(strlen(clients[i].tok_buf) > MESSAGE_SIZE) {
                                special_write(clients[i].fd, "ERR TOOLONG\n", 12);
                            }

                            else message_handling(&clients[i], i, clients, log, udp_fd, &broadcast_addr);
                            clients[i].tok_buf = strtok(NULL, "\n");
                        }
                    }

                    
                    memset(clients[i].line_buf, 0, sizeof(clients[i].line_buf));
                    clients[i].cnt = 0;
                    
                    //write(fd, message, strlen(message));
                }
                
                
            }
            
        }
       

    }

    return 0;
}

void message_handling(client_t *client, int index, client_t *clients, FILE *log, int udp_fd, struct sockaddr_in *broadcast_addr) {

    char key[30] = {0};
    char buf[MESSAGE_SIZE + 16];
    int status;
    int n = 0;
    int counter;
    char *ptr = client->tok_buf;
    int flag;
    int flg;
    int qty;
    int new_price;
    int k;
    int len = 0;

    sscanf(ptr, "%s%n", key, &n);
    ptr += n;

    if(strcmp(key, "JOIN") == 0) {
        sscanf(ptr, "%s%n", key, &n);
        ptr += n;
        if(strcmp(key, "TRADER") != 0 && strcmp(key, "ANALYST") != 0) {
            len += sprintf(buf + len, "ERR UNKNOWN %s\n", client->tok_buf);
            special_write(client->fd, buf, strlen(buf));
        }
        else {
            if(strcmp(key, "ANALYST") == 0) strcpy(client->type, "ANALYST");
            else if(strcmp(key, "TRADER") == 0) {
                strcpy(client->type, "TRADER");

                client->portfolio = (stock_t*)malloc(stock_size * sizeof(stock_t));
                client->size = 0;

                for(int i = 0; i < stock_size; i++) client->portfolio[i].quantity = 0;
            } 

            status = sscanf(ptr, "%s%n", key, &n);
            ptr += n;
            if(status != 1) {
                len += sprintf(buf + len, "ERR UNKNOWN %s\n", client->tok_buf);
                special_write(client->fd, buf, strlen(buf));
            }
            else {
                strcpy(client->username, key);

                counter = 0;
                for(int i = 0; i < MAX_CLIENT; i++) {
                    if(clients[i].fd != 0 && strcmp(client->username, clients[i].username) == 0) counter++;
                    if(counter > 1) break;
                }
                if(counter > 1) {
                    len += sprintf(buf + len, "ERR JOIN %s\n", client->username);
                    special_write(client->fd, buf, strlen(buf));
                    close(client->fd);
                    clients[index].fd = 0;

                    if(strcmp(client->type, "TRADER") == 0 && client->portfolio != NULL) {
                        free(client->portfolio);
                        client->portfolio = NULL;
                    }

                   
                    
                    printf("[SERVER] CLIENT_DISCONNECTED username=%s reason=err_join\n", client->username);
                    fprintf(log, "[SERVER] CLIENT_DISCONNECTED username=%s reason=err_join\n", client->username);
                    
                }
                else {
                    client->start = 1;
                    len += sprintf(buf + len, "OK JOIN %s\n", client->username);
                    special_write(client->fd, buf, strlen(buf));
                    printf("[SERVER] JOIN username=%s type=%s fd=%d\n", client->username, client->type, client->fd);
                    fprintf(log, "[SERVER] JOIN username=%s type=%s fd=%d\n", client->username, client->type, client->fd);

                }
            }

        }
    }
    else if(strcmp(key, "PRICE") == 0) {

        if(!(client->start)) {
            special_write(client->fd, "ERR NOT_JOINED\n", 15);
            return;
        }

        if(strcmp(client->type, "ANALYST") != 0) {
           
            special_write(client->fd, "ERR UNAUTHORIZED\n", 17);
        }
        else {
            sscanf(ptr, "%s%n", key, &n);
            ptr += n;

            flag = 0;
            for(int i = 0; i < stock_size && !flag; i++) {
                if(strcmp(key, global_stock[i].symbol) == 0) {
                    len += sprintf(buf + len, "OK PRICE %s %.2f\n", key, (float) global_stock[i].price / 100);
                    special_write(client->fd, buf, strlen(buf));
                    flag = 1;
    
                   
                }
            }
            if(flag == 0) {
                special_write(client->fd, "ERR UNKNOWN_SYMBOL\n", 19);
            }
        }
    }
    else if(strcmp(key, "REPORT") == 0) {

        if(!(client->start)) {
            special_write(client->fd, "ERR NOT_JOINED\n", 15);
            return;
        }

        if(strcmp(client->type, "ANALYST") != 0) {
            special_write(client->fd, "ERR UNAUTHORIZED\n", 17);

        }
        else {
            //special_write(client->fd, "OK REPORT ", 11);
            len += sprintf(buf + len, "OK REPORT ");
            for(int i = 0; i < stock_size; i++) {
                len += sprintf(buf + len, "%s:%.2f ", global_stock[i].symbol, (float)global_stock[i].price / 100);
                // special_write(client->fd, buf, strlen(buf));
                
            }
            len += sprintf(buf + len, "\n");
            special_write(client->fd, buf, strlen(buf));
        }

    }
    else if(strcmp(key, "LIST") == 0) {

        if(!(client->start)) {
            special_write(client->fd, "ERR NOT_JOINED\n", 15);
            return;
        }

        if(strcmp(client->type, "ANALYST") != 0) {
            special_write(client->fd, "ERR UNAUTHORIZED\n", 17);

        }
        else {
            //special_write(client->fd, "OK LIST,", 11);
            len += sprintf(buf + len, "OK LIST ");
            for(int i = 0; i < MAX_CLIENT; i++) {
                if(clients[i].fd != 0) {
                    len += sprintf(buf + len, "%s ", clients[i].username);
                    // special_write(client->fd, buf, strlen(buf));
                    
                }
            }
            len += sprintf(buf + len, "\n");
            special_write(client->fd, buf, strlen(buf));
        }


    }
    else if(strcmp(key, "QUIT") == 0) {

        if(!(client->start)) {
            special_write(client->fd, "ERR NOT_JOINED\n", 15);
            return;
        }

        special_write(client->fd, "OK QUIT\n", 8);
        
        if(strcmp(client->type, "TRADER") == 0) {
            free(client->portfolio);
            client->portfolio = NULL;
        }
        
        close(client->fd);
        client->fd = 0;

        memset(client->line_buf, 0, sizeof(client->line_buf));
        clients->cnt = 0;

        client->start = 0;

        printf("[SERVER] CLIENT_DISCONNECTED username=%s reason=QUIT\n", client->username);
        fprintf(log, "[SERVER] CLIENT_DISCONNECTED username=%s reason=QUIT\n", client->username);
    }
    else if(strcmp(key, "BUY") == 0) {

        if(!(client->start)) {
            special_write(client->fd, "ERR NOT_JOINED\n", 15);
            return;
        }

        if(strcmp(client->type, "TRADER") != 0) {
            special_write(client->fd, "ERR UNAUTHORIZED\n", 17);
        }
        else {
            status = sscanf(ptr, "%s%d%n", key, &qty, &n);
            ptr += n;
 
            if(status != 2 || qty < 0) {
                len += sprintf(buf + len, "ERR UNKNOWN %s\n", client->tok_buf);
                special_write(client->fd, buf, strlen(buf));
                return;
            }
            int old_price;
            flag = 0;
            for(int i = 0; i < stock_size && !flag; i++) {
                if(strcmp(key, global_stock[i].symbol) == 0) {
                    old_price = global_stock[i].price;
                    global_stock[i].price += qty * 1;
                    new_price= global_stock[i].price;
                    flag = 1;
                }
            }

            if(flag == 1) {
                flg = 0;
                for(int i = 0; i < client->size && !flg; i++) {
                   if(strcmp(key, client->portfolio[i].symbol) == 0) {
                        client->portfolio[i].quantity += qty;
                        flg = 1;
                    }
                }

                if(flg == 0) {
                    strcpy(client->portfolio[client->size].symbol, key);
                    client->portfolio[client->size].quantity += qty;
                    client->size++;
                }

                len += sprintf(buf + len, "OK BUY %s %d %.2f\n", key, qty, (float) new_price / 100);
                special_write(client->fd, buf, strlen(buf));
                printf("[SERVER]  BUY trader=%s symbol=%s qty=%d old_price=%.2f new_price=%.2f\n", client->username, key, qty,(float) old_price / 100, (float) new_price / 100);
                fprintf(log, "[SERVER]  BUY trader=%s symbol=%s qty=%d old_price=%.2f new_price=%.2f\n", client->username, key, qty, (float) old_price / 100, (float) new_price / 100);

                len = 0;
                len += sprintf(buf + len, "PRICE_UPDATE ");
                for(int i = 0; i < stock_size; i++) {
                    len += sprintf(buf + len, "%s:%.2f ", global_stock[i].symbol, (float) global_stock[i].price / 100);
                }
                len += sprintf(buf + len, "\n");
                sendto(udp_fd, buf, strlen(buf), 0, (struct sockaddr *) broadcast_addr, sizeof(*broadcast_addr));

                printf("[SERVER] PRICE_BROADCAST trigger=TRADE\n");
                fprintf(log, "[SERVER] PRICE_BROADCAST trigger=TRADE\n");
            }

            else {
                special_write(client->fd, "ERR UNKNOWN_SYMBOL\n", 19);
            }
        }

    }
    else if(strcmp(key, "SELL") == 0) {

        if(!(client->start)) {
            special_write(client->fd, "ERR NOT_JOINED\n", 15);
            return;
        }

        int old_price;
        
        if(strcmp(client->type, "TRADER") != 0) {
            special_write(client->fd, "ERR UNAUTHORIZED\n", 17);
        }
        else {
            status = sscanf(ptr, "%s%d%n", key, &qty, &n);
            ptr += n;
 
            if(status != 2 || qty < 0) {
                len += sprintf(buf + len, "ERR UNKNOWN %s\n", client->tok_buf);
                special_write(client->fd, buf, strlen(buf));
                return;
            }

            flag = 0;
            for(int i = 0; i < stock_size && !flag; i++) {
                if(strcmp(key, global_stock[i].symbol) == 0) {
                    if(global_stock[i].price - qty < 1) {
                        special_write(client->fd, "ERR INSUFFICIENT_SHARES\n", 24);
                        return;
                    }
                    // old_qty = global_stock[i].price;
                    // global_stock[i].price -= qty;
                    // new_price = global_stock[i].price;
                    k = i;
                    flag = 1;
                }
            }

            if(flag == 1) {
                flg = 0;
                for(int i = 0; i < client->size && !flg; i++) {
                   if(strcmp(key, client->portfolio[i].symbol) == 0) {
                        if(client->portfolio[i].quantity < qty) {
                            special_write(client->fd, "ERR INSUFFICIENT_SHARES\n", 24);
                            return;
                        }
                        client->portfolio[i].quantity -= qty;
                        flg = 1;
                    }
                }

                if(flg) {
                    old_price = global_stock[k].price;
                    global_stock[k].price -= qty;
                    new_price = global_stock[k].price;
                    
                    len += sprintf(buf + len, "OK SELL %s %d %.2f\n", key, qty, (float) new_price / 100);
                    special_write(client->fd, buf, strlen(buf));
                    printf("[SERVER] SELL trader=%s symbol=%s qty=%d old_price=%.2lf new_price=%.2lf\n", client->username, key, qty, (float) old_price / 100, (float) new_price / 100);
                    fprintf(log, "[SERVER] SELL trader=%s symbol=%s qty=%d old_price=%.2lf new_price=%.2lf\n", client->username, key, qty, (float) old_price / 100, (float) new_price / 100);

                    len = 0;
                    len += sprintf(buf + len, "PRICE_UPDATE ");
                    for(int i = 0; i < stock_size; i++) {
                        len += sprintf(buf + len, "%s:%.2f ", global_stock[i].symbol, (float) global_stock[i].price / 100);
                    }
                    len += sprintf(buf + len, "\n");
                    sendto(udp_fd, buf, strlen(buf), 0, (struct sockaddr *) broadcast_addr, sizeof(*broadcast_addr));

                    printf("[SERVER] PRICE_BROADCAST trigger=TRADE\n");
                    fprintf(log, "[SERVER] PRICE_BROADCAST trigger=TRADE\n");

                }
                else {
                    special_write(client->fd, "ERR INSUFFICIENT_SHARES\n", 24);
                }
                
            }

            else {
                special_write(client->fd, "ERR UNKNOWN_SYMBOL\n", 19);
            }
        }

    }
    else if(strcmp(key, "PORTFOLIO") == 0) {

        if(!(client->start)) {
            special_write(client->fd, "ERR NOT_JOINED\n", 15);
            return;
        }

        if(strcmp(client->type, "TRADER") != 0) {
            special_write(client->fd, "ERR UNAUTHORIZED\n", 17);

        }
        else {
            if(client->size == 0) {
                special_write(client->fd, "OK PORTFOLIO EMPTY\n", 19);
            }
            else {
                //special_write(client->fd, "OK PORTFOLIO\n", 14);
                len += sprintf(buf + len, "OK PORTFOLIO ");
                for(int i = 0; i < client->size; i++) {
                    len += sprintf(buf + len, "%s:%d ", client->portfolio[i].symbol, client->portfolio[i].quantity);
                    //special_write(client->fd, buf, strlen(buf));
                }
                len += sprintf(buf + len, "\n");
                special_write(client->fd, buf, strlen(buf));
            }
        }

        
        
    }
    else {
        len += sprintf(buf + len, "ERR UNKNOWN %s\n", client->tok_buf);
        special_write(client->fd, buf, strlen(buf));
    }


}



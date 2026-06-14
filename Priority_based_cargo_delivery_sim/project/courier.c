#define _GNU_SOURCE
#include "courier.h"
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "order_queue.h"
#include <string.h>
#include <pthread.h>
#include <stdatomic.h>
#include <time.h>
#include <math.h>
#include <signal.h>

extern int optind, opterr, optopt;
extern char *optarg;


order_queue *queue;
pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t not_empty = PTHREAD_COND_INITIALIZER;
pthread_cond_t finish = PTHREAD_COND_INITIALIZER;
atomic_int completed_orders = 0;
atomic_int cancelled_orders = 0;
atomic_int total_delivery_time = 0;
atomic_int is_completed = 0;
atomic_int finish_counter = 0;



void print_priority(priority prt) {

    switch(prt) {

        case EXPRESS: printf("EXPRESS"); break;
        case STANDARD: printf("STANDARD"); break;
        case ECONOMY: printf("ECONOMY"); break;
        default: printf("---");
    }
}


void* courier_thread(void *arg) {

    order item;
    courier_arg *c_arg = (courier_arg*)arg;
    struct timespec rq;
    struct timespec rm;
    
    c_arg->completed = 0;
    c_arg->total_time = 0;

    while(1) {    
        pthread_mutex_lock(&queue_mutex);

        while(queue->size == 0 && is_completed == 0) {
            atomic_fetch_add(&finish_counter, 1);
            pthread_cond_signal(&finish); // send a signal to the main thread.
            pthread_mutex_lock(&log_mutex);
            printf("[COURIER-%d] WAITING\n", c_arg->index);
            pthread_mutex_unlock(&log_mutex);

            pthread_cond_wait(&not_empty, &queue_mutex);
        }

        if(is_completed == 0) {
            read_order_list(&item);
            pthread_mutex_unlock(&queue_mutex);

            pthread_mutex_lock(&log_mutex);
            printf("[COURIER_%d] DELIVERY_START id = %d recipient = %s priority =", c_arg->index, item.id, item.name);
            print_priority(item.prt);
            printf("\n");
            pthread_mutex_unlock(&log_mutex);

            rq.tv_sec = item.duration * 500 / 1000;
            rq.tv_nsec = (item.duration * 500 - rq.tv_sec * pow(10, 3)) * pow(10, 6);

            nanosleep(&rq, &rm); // delivery operation

            atomic_fetch_add(&completed_orders, 1);
            atomic_fetch_add(&total_delivery_time, item.duration * 500);
            atomic_fetch_add(&cancelled_orders, -1);

            c_arg->completed++;
            c_arg->total_time += item.duration * 500;

            pthread_mutex_lock(&log_mutex);
            printf("[COURIER_%d] DELIVERY_COMPLETE id = %d recipient = %s duration = %dms\n", c_arg->index, item.id, item.name, item.duration * 500);
            pthread_mutex_unlock(&log_mutex);

        }
        else {
            pthread_mutex_unlock(&queue_mutex);
            pthread_mutex_lock(&log_mutex);
            printf("[COURIER-%d] SHIFT_OVER\n", c_arg->index);
            pthread_mutex_unlock(&log_mutex);
            break;
        }
    }

    return NULL;

}

void* signal_handler(void *arg) {

    sig_arg *s_arg = (sig_arg*)arg;
    int sig;

    sigwait(s_arg->set, &sig);

    atomic_fetch_add(&finish_counter, s_arg->num_courier - finish_counter);

    pthread_mutex_lock(&queue_mutex);
    pthread_cond_signal(&finish);
    pthread_mutex_unlock(&queue_mutex);

    return NULL;

}


void courier_main(int argc, char *argv[]) {

    command cmd;
    pthread_t *couriers;
    pthread_t signal_thread;
    courier_arg *args;
    sigset_t set;
    sig_arg s_arg;
    int num_order;

    sigemptyset(&set);
    sigaddset(&set, SIGINT);
    pthread_sigmask(SIG_BLOCK, &set, NULL);
    s_arg.set = &set;
   

    get_arguments(argc, argv, &cmd);

    s_arg.num_courier = cmd.num_couriers;

    couriers = (pthread_t*)malloc(cmd.num_couriers * sizeof(pthread_t));
    args = (courier_arg*)malloc(cmd.num_couriers * sizeof(courier_arg));

    read_file(cmd.input_file);

    num_order = queue->size;

    atomic_fetch_add(&cancelled_orders, queue->size);

    printf("[CARGOGTU] SHIFT_START couriers = %d, orders = %d\n", cmd.num_couriers, queue->size);
    pthread_create(&signal_thread, NULL, signal_handler, &s_arg);
    for(int i = 0; i < cmd.num_couriers; i++) {
        args[i].index = i + 1;
        pthread_create(&couriers[i], NULL, courier_thread, &args[i]);
    }

    pthread_detach(signal_thread);

    pthread_mutex_lock(&queue_mutex);
    while(finish_counter != cmd.num_couriers) {
        pthread_cond_wait(&finish, &queue_mutex);
    }
    atomic_fetch_add(&is_completed, 1);
    pthread_cond_broadcast(&not_empty);
    pthread_mutex_unlock(&queue_mutex);
    
    for(int i = 0; i < cmd.num_couriers; i++) {
        pthread_join(couriers[i], NULL);
    }

    printf("[CARGOGTU] SHIFT_END completed = %d cancelled = %d total_time = %d\n", completed_orders, cancelled_orders, total_delivery_time);

    print_summary(args, cmd.num_couriers, cmd.output_file, num_order);

    free(couriers);
    free(args);
    release_list();

    printf("[CARGOGTU] SHUTDOWN_COMPLETE\n");
}

void read_file(char *file_name) {

    FILE *fp;
    char line[MAX_LINE_LENGTH];
    int num_line;
    char priority[10];
    int id;
    char name[MAX_NAME_LENGTH];
    int duration;
    int i;
    order item;

    fp = fopen(file_name, "r");

    if(fp == NULL) {
        fprintf(stderr, "The order file could not be opened.\n");
        exit(EXIT_FAILURE);
    }

    num_line = 0;
    while(fgets(line, MAX_LINE_LENGTH, fp) != NULL) num_line++;

    initialize_order_list(num_line);

    rewind(fp);

    i = 0;
    while(i < num_line) {

        if(fscanf(fp, "%d%s%s%d", &id, name, priority, &duration) != 4) {
            i++;
            fgets(line, MAX_LINE_LENGTH, fp);
            continue;
        }
        
        i++;

        item.duration = duration;
        item.id = id;
        item.prt = return_priority(priority);
        strcpy(item.name, name);

        write_order_list(&item);

        //pthread_mutex_lock(&log_mutex);
        /*************************** */
        printf("[CARGOGTU] ORDER_QUEUED id = %d recipient = %s priority =", item.id, item.name);
        print_priority(item.prt);
        printf(" duration = %d\n", item.duration);
        /****************************** */
        //pthread_mutex_unlock(&log_mutex);

    }

    fclose(fp);
}

void get_arguments(int argc, char *argv[], command *cmd) {

       
    
    
    int opt;
    int flag_n = 0;
    int flag_i = 0;
    int flag_s = 0;
    

    while((opt = getopt(argc, argv, ":n:i:s:x")) != -1) {

        switch(opt) {
            case 'n': 
                    if(flag_n == 1) {
                        fprintf(stderr, "You should use -n flag once\n");
                    }
                    else {
                        flag_n = 1;
                        cmd->num_couriers = command_extract_for_digit(optarg);
                    }break;
            case 'i':
                    if(flag_i == 1) {
                        fprintf(stderr, "You should use -i flag once\n");
                    }
                    else {
                        flag_i = 1;
                        strcpy(cmd->input_file, optarg);
                    }break;
            case 's':
                    if(flag_s == 1) {
                        fprintf(stderr, "You should use -s flag once\n");
                    }
                    else {
                        flag_s = 1;
                        strcpy(cmd->output_file, optarg);
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

    if(flag_n == 0) {
        fprintf(stderr, "You should use n flag.\n");
        exit(EXIT_FAILURE);
    }
    if(flag_i == 0) {
        fprintf(stderr, "You should use -i flag.\n");
        exit(EXIT_FAILURE);
    }
    if(flag_s == 0) {
        fprintf(stderr, "You should use -s flag.\n");
        exit(EXIT_FAILURE);
    }
   

}

priority return_priority(char *prt) {

    priority prt_r;

    if(strcmp(prt, "EXPRESS") == 0) prt_r = EXPRESS;
    else if(strcmp(prt, "ECONOMY") == 0) prt_r = ECONOMY;
    else if(strcmp(prt, "STANDARD") == 0) prt_r = STANDARD;

    return prt_r;
    
}

void print_summary(courier_arg *arg, int num_courier, char *output_file, int num_order) { //array of arguments

    FILE *fp;

    fp = fopen(output_file, "w");

    if(fp == NULL) {
        fprintf(stderr, "The stat file could not be opened.\n");
        return;
    }

    fprintf(fp, "SHIFT_SUMMARY\n");
    fprintf(fp, "Total orders   : %d\n", num_order);
    fprintf(fp, "Completed   : %d\n", completed_orders);
    fprintf(fp, "Cancelled   : %d\n", cancelled_orders);
    fprintf(fp, "Total time   : %dms\n", total_delivery_time);
    fprintf(fp, "Avg per order   : %dms\n\n", (completed_orders == 0) ? 0 : (total_delivery_time / completed_orders));

    fprintf(fp, "COURIER_STATS\n");

    for(int i = 0; i < num_courier; i++) {
        fprintf(fp, "Courier-%d completed = %d total_time = %dms\n", arg[i].index, arg[i].completed, arg[i].total_time);
    }

    fclose(fp);
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
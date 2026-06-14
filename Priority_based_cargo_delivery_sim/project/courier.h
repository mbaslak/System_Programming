#ifndef COURIER_H
#define COURIER_H

#define _GNU_SOURCE
#define MAX_PATH_LENGTH 100
#include "order_queue.h"
#include <pthread.h>
#include <stdatomic.h>
#include <signal.h>

extern order_queue *queue;
extern pthread_mutex_t queue_mutex;
extern pthread_mutex_t log_mutex;
extern atomic_int completed_orders;
extern atomic_int cancelled_orders;
extern atomic_int total_delivery_time;
extern pthread_cond_t not_empty;
extern atomic_int is_completed;



typedef struct{
    int num_couriers; // It should be greater than or equal to 1.
    char input_file[MAX_PATH_LENGTH];
    char output_file[MAX_PATH_LENGTH];
} command;

typedef struct{
    int index;
    int completed;
    int total_time;
} courier_arg;

typedef struct {
    sigset_t *set;
    int num_courier;
} sig_arg;

void get_arguments(int argc, char *argv[], command *cmd);
int command_extract_for_digit(char *command);
void read_file(char *file_name);
void courier_main(int argc, char *argv[]);
priority return_priority(char *prt);
int command_extract_for_digit(char *command);
void print_priority(priority prt);
void* courier_thread(void *arg);
void print_summary(courier_arg *arg, int num_courier, char *output_file, int num_order);

#endif
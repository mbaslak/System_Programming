#ifndef ORDER_QUEUE_H
#define ORDER_QUEUE_H

#define MAX_NAME_LENGTH 32
#define MAX_LINE_LENGTH 70

typedef enum {
    EXPRESS, STANDARD, ECONOMY
} priority;

typedef struct {

    int id;
    char name[MAX_NAME_LENGTH];
    priority prt;
    int duration; // 1 unit = 500 milliseconds
} order;

typedef struct {
    order *orders;
    int size;
    int capacity;
    int front;
} order_queue; // priority queue

void read_order_list(order *item);
void initialize_order_list(int capacity);
void write_order_list(order *item);
void release_list();
void copy_order(order *to, order *from);
void swap_order(order *item_1, order *item_2);

#endif
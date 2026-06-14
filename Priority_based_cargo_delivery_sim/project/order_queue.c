#include "order_queue.h"
#include "courier.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>


void read_order_list(order *item) {
    
    if(queue->size == 0) {
        fprintf(stderr, "underflow - priority queue.\n");
        release_list();
        exit(EXIT_FAILURE);
    }

    item->duration = queue->orders[queue->front].duration;
    item->id = queue->orders[queue->front].id;
    item->prt = queue->orders[queue->front].prt;

    strcpy(item->name, queue->orders[queue->front].name);

    queue->front += 1;
    queue->size -= 1;

}

void initialize_order_list(int capacity) {

    queue = (order_queue*)malloc(sizeof(order_queue));

    queue->orders = (order*)malloc(capacity * sizeof(order));

    queue->capacity = capacity;

    queue->size = 0;

    queue->front = 0;    
}

void write_order_list(order *item) {

    int flag;

    if(queue->capacity == queue->size) {
        fprintf(stderr, "overflow - priority queue.\n");
        release_list();
        exit(EXIT_FAILURE);
    }

    copy_order(&queue->orders[queue->size], item);

    if(queue->size != 0) {

        flag = 0;
        int j = queue->size;
        for(int i = queue->size - 1; i >= queue->front && !flag; i--) {

            if(queue->orders[i].prt > queue->orders[j].prt) {
                swap_order(&queue->orders[i], &queue->orders[j]);
                j--;
            }
            else flag = 1;
        }

    }

    queue->size++;
}

void release_list() {
    
    free(queue->orders);
    free(queue);
}

void copy_order(order *to, order *from) {

    to->duration = from->duration;
    to->id = from->id;
    to->prt = from->prt;

    strcpy(to->name, from->name);

}

void swap_order(order *item_1, order *item_2) {

    order tmp;

    copy_order(&tmp, item_1);
    copy_order(item_1, item_2);
    copy_order(item_2, &tmp);
}
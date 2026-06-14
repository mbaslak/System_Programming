#include "special_io.h"
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#define MESSAGE_SIZE 512


void special_write(int fd, char *buf, int size) {

    int written_byte;

    while(1) {
        written_byte = write(fd, buf, size);

        if(written_byte <= 0) {
            if(errno == EINTR) continue;
            break;
        }
        

        if(written_byte == size) break;

        buf += written_byte;
        size -= written_byte;
    }
    
}

int special_read(int fd, char *buf, int size, int *read_var) {

    int j = 0;
    int cnt = 0;

    memset(buf, 0, MESSAGE_SIZE);

    do {
        *read_var = read(fd, buf + j, size); 
        if(*read_var == -1 || *read_var == 0) {
            if(errno == EINTR) continue;

            break;
        };
        j += *read_var;
        size -= *read_var;
        cnt += *read_var;
    }while(buf[cnt - 1] != '\n');

    

    return cnt;
}


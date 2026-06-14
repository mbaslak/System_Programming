#ifndef SPECIAL_IO_H
#define SPECIAL_IO_H


int special_read(int fd, char *buf, int size, int *read_var);
void special_write(int fd, char *buf, int size);

#endif
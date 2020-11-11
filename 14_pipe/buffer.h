#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <memory.h>
#include <sys/ioctl.h>
#include <linux/limits.h>
#include <unistd.h>

extern int errno;

struct buffer {
    char *buf;
    size_t buf_sz;
    size_t data_start; //first position with data
    size_t data_end;  //first position without data after data
};

int how_much_read_avail (int fd);
int alloc_buffer (char **buf, size_t buf_sz);
int struct_buffer_init (struct buffer *buf, size_t buf_sz);
ssize_t buf_write (int fd, struct buffer *buf);
ssize_t buf_read (int fd, struct buffer *buf);
int buf_defrag (struct buffer *buf);
#include "buffer.c"
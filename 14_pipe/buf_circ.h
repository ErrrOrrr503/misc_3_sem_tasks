#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <memory.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <stdbool.h>

struct buffer {
    char *buf;
    ssize_t buf_sz;
    ssize_t data_start; //first position with data
    ssize_t data_end;  //first position without data after data
    bool flag_loop; //data is looped, needed to distinguish empty buffer from absolutely full one
};

int how_much_read_avail (int fd);
int alloc_buffer (char **buf, size_t buf_sz);
int struct_buffer_init (struct buffer *buf, size_t buf_sz);
ssize_t buf_write (int fd, struct buffer *buf);
ssize_t buf_read (int fd, struct buffer *buf);
ssize_t data_amount (struct buffer * buf);
ssize_t write_retry (int fd, const void *buf, size_t count, size_t retries);
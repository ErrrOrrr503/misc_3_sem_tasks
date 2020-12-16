#include <stdlib.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>

#define TIMESTR_LEN_TARGET 100
struct time_shared {
    unsigned ver;
    char time[TIMESTR_LEN_TARGET];
};

char * check_mq_shmem_name_alloc (const char *raw_name);
void * shared_mem_init (const char *name, size_t size, int oflag, mode_t mode, int prot);
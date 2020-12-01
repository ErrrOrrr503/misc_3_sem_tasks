#define timestr_len_target 100
struct time_shared {
    sem_t sem;
    size_t len;
    char time[timestr_len_target];
};

char * check_mq_shmem_name_alloc (const char *raw_name);
void * shared_mem_init (const char *name, size_t size, int oflag, mode_t mode, int prot, int flags, off_t offset);

#include "shmem.c"
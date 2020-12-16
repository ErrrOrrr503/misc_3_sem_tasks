#include "common.h"

void * shared_mem_init (const char *name, size_t size, int oflag, mode_t mode, int prot)
{
    //init and map shmem
    int shm_fd = shm_open (name, oflag, mode);
    if (shm_fd == -1) {
        perror ("can't open shmem");
        return NULL;
    }
    if (oflag & O_CREAT) {
        if (ftruncate (shm_fd, size)) {
            perror ("truncating failed");
            close (shm_fd);
            return NULL;
        }
    }
    void *mem = (void *) mmap (NULL, size, prot, MAP_SHARED, shm_fd, 0);
    close (shm_fd); // no longer needed 
    if (mem == MAP_FAILED) {
        perror ("can't init shared memory");
        return NULL;
    }
    return mem;
}
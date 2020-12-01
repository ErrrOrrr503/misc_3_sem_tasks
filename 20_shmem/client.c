#include <fcntl.h>
#include <sys/stat.h>
#include <mqueue.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <linux/limits.h>
#include <signal.h>
#include <time.h>
#include <stdbool.h>
#include <sys/mman.h>
#include <semaphore.h>
#include "shmem.h"

extern int errno;

volatile bool g_flag_termination = 0;

void sig_handler (int sig);

int main (int argc, char *argv[])
{
    if (argc != 2) {
        printf ("usage: '%s' <shmem_name>\n", argv[0]);
        return 0;
    }
    struct sigaction act = {0};
    act.sa_handler = sig_handler; //new handler   
        // do not need sa_sigaction (used with SA_SIGINFO)
        // do not need to block other signals
        // do not need special flags
        // sa_restorer not even in posix
    int ret = sigaction (SIGINT, &act, NULL);
    ret |= sigaction (SIGTERM, &act, NULL);
    if (ret) {
        perror ("sigaction error");
        return -1;
    }
    //form shmem_name, as it must be '/somename' 
    char *shm_name = check_mq_shmem_name_alloc (argv[1]);
    if (shm_name == NULL)
        return -1;
    size_t shm_size = sizeof (struct time_shared);
    printf ("formed mq_name is: '%s'; size of shmem: '%lu'\n", shm_name, shm_size);
    struct time_shared *time_shared = (struct time_shared *) shared_mem_init (shm_name, shm_size, \
                                                                        O_RDWR, 0, PROT_READ | PROT_WRITE, MAP_SHARED, 0);
    if (time_shared == MAP_FAILED) {
        free (shm_name);
        printf ("Server not running?\n");
        return -1;
    }
    //semaphore inited by server, if not - servet is not running => will fail to open shmem;
    bool time_error_flag = 0;
    char str[timestr_len_target];
    while (1) {
        sem_wait (&time_shared->sem);
        if (!time_shared->len) {
            g_flag_termination = 1;
            time_error_flag = 1;
        }
        else
            strncpy (str, time_shared->time, timestr_len_target);
        sem_post (&time_shared->sem);
        if (g_flag_termination) {
            int ret = 0;
            if (time_error_flag) {
                printf ("Something wrong with server, exiting\n");
                ret = -1;
            }
            munmap (time_shared, shm_size);
            free (shm_name);
            return ret;
        }
        printf ("%s\n", str);
        sleep (1);
    }
    return 0;
}

void sig_handler (int sig)
{
    g_flag_termination = 1;
    printf ("\nterminated by signal: %d : %s\n", sig, strsignal (sig));
}
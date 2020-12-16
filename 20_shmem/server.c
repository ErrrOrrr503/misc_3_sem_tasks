#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <time.h>
#include <stdbool.h>
#include <sys/mman.h>
#include "common.h"

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
    if (sigaction (SIGINT, &act, NULL) || sigaction (SIGTERM, &act, NULL)) {
        perror ("sigaction error");
        return -1;
    }
    const char *shm_name = argv[1];
    size_t shm_size = sizeof (struct time_shared);
    printf ("size of shmem: '%lu'\n", shm_size);
    struct time_shared *time_shared = (struct time_shared *) shared_mem_init (shm_name, shm_size, \
                                                                        O_RDWR | O_CREAT | O_EXCL, 0622, PROT_READ | PROT_WRITE);
                                                                        // rw for server (user), ro for clients (others)
    if (time_shared == NULL) {
        if (errno == EEXIST)
            printf ("server already running?\n");
        return -1;
    }
    bool time_error_flag = 0;
    while (!g_flag_termination) {
        time_t tm = time (NULL);
        time_shared->ver++;
        if (!strftime (time_shared->time, TIMESTR_LEN_TARGET, "%c", localtime (&tm))) {
            time_error_flag = 1;
            break;
        }
        time_shared->ver++;
        sleep (1);
    }
    int ret = 0;
    if (time_error_flag) {
        printf ("Something wrong with time, exiting\n");
        ret = -1;
    }
    munmap (time_shared, shm_size);
    shm_unlink (shm_name);
    return ret;
}

void sig_handler (int sig)
{
    g_flag_termination = 1;
    printf ("\nterminated by signal: %d : %s\n", sig, strsignal (sig));
}
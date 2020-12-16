#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
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
    if (sigaction (SIGTERM, &act, NULL) || sigaction (SIGINT, &act, NULL)) {
        perror ("sigaction error");
        return -1;
    }
    const char *shm_name = argv[1];
    size_t shm_size = sizeof (struct time_shared);
    printf ("size of shmem: '%lu'\n", shm_size);
    struct time_shared *time_shared = (struct time_shared *) shared_mem_init (shm_name, shm_size, \
                                                                        O_RDWR, 0200, PROT_READ);
    if (time_shared == NULL) {
        printf ("Server not running?\n");
        return -1;
    }
    char str[TIMESTR_LEN_TARGET];
    while (!g_flag_termination) {
        unsigned ver = time_shared->ver;
        strncpy (str, time_shared->time, TIMESTR_LEN_TARGET);
        if (time_shared->ver == ver) { // if data read is correct then print, else repeat reading
            printf ("%s\n", str);
            sleep (1);
        }
    }
    munmap (time_shared, shm_size);
    return 0;
}

void sig_handler (int sig)
{
    g_flag_termination = 1;
    printf ("\nterminated by signal: %d : %s\n", sig, strsignal (sig));
}
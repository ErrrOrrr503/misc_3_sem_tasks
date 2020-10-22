#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <linux/limits.h>
#include <fcntl.h>
#include <signal.h>

extern int errno;

#define write_sz 65537 //number of bytes written to pipe

//conclusion: writing more bytes than pipe size causes sleeping.

void sigint_handler (int sig);

int main (int argc, char *argv[])
{
    signal (SIGINT, sigint_handler);
    if (argc > 1) {
		printf ("'%s' needs no options\n", argv[0]);
		return 1;
	}
    
    int pipe_fds[2] = {0};
    if (pipe (pipe_fds) == -1) {
        perror ("can't open pipe");
        return -1;
    }

    int pipe_sz = fcntl (pipe_fds[0], F_GETPIPE_SZ);
    if (pipe_sz == -1) {
        perror ("can't get pipe size");
        return -1;
    }
    printf ("pipe size according to fnctl: %u\n", pipe_sz);

    char zero[write_sz] = {0};
    printf ("writing %u to pipe; written %ld\n", write_sz, write (pipe_fds[1], zero, write_sz));
    return 0;
}

void sigint_handler (int sig)
{
    if (sig == SIGINT)
        printf ("\nChO spim b...\n");
    exit (0);
}
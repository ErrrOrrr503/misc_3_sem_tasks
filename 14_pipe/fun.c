#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <linux/limits.h>
#include <fcntl.h>
#include <signal.h>
#include <poll.h>
#include <time.h>
#include <sys/ioctl.h>

extern int errno;
int free_pipe_space (int pipefd);

int main () {
    while (1) {
        printf ("avail in stdin: '%d'\n", free_pipe_space (STDIN_FILENO));
        sleep (1);
    }
    return 0;
}



// info about how much can we read, no use
int free_pipe_space (int pipefd)
{
    int bytes_availible = 0;
    if (ioctl (pipefd, TIOCINQ, &bytes_availible) == -1) {
        perror ("can't determine amount of free pipe bytes (via ioctl FIONREAD)");
        return -1;
    }
    printf ("pipe_free_space: %d\n", bytes_availible);
    return bytes_availible;
}
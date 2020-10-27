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

time_t g_start_time = 0;

void sigint_handler (int sig);
int alloc_bufer (char **buf, size_t buf_sz);
int pipe_size (int pipefd);
int parent_code (int pipe_in_fds[2], int pipe_out_fds[2], int fd);

/*
Gets info from stdin and gzips it to output_file
*/

/* Questions:
    1) Is there a way to get free/occupied space of pipe input end
    2) Why poll() always returns POLLIN event even when eof is met?
*/
int main (int argc, char *argv[])
{
    g_start_time = time (NULL);
    signal (SIGINT, sigint_handler);
    if (argc != 2) {
		printf ("usage: '%s' output_file\n", argv[0]);
		return 1;
	}
    int fd = open (argv[1], O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd == -1) {
        perror (argv[1]);
        return -1;
    }
    
    int pipe_in_fds[2] = {0};
    int pipe_out_fds[2] = {0};
    if (pipe (pipe_in_fds) == -1 || pipe (pipe_out_fds) == -1) {
        perror ("can't open pipes");
        return -1;
    }

    int gzip_pid = fork ();
    if (gzip_pid == -1) {
        perror ("can't create child process");
        return -1;
    }
    if (!gzip_pid) {
        //child code
        close (pipe_in_fds[1]); // close "впись"
        dup2 (pipe_in_fds[0], STDIN_FILENO); // now STDIN is "выпись" pipe_in
        close (pipe_in_fds[0]); // close duplicate fd.

        close (pipe_out_fds[0]); // close "выпись"
        dup2 (pipe_out_fds[1], STDOUT_FILENO); // now STDIN is "впись" pipe_out
        close (pipe_out_fds[1]); // close duplicate fd.

        execlp ("gzip", "gzip", "-f", NULL); // -f to force gzip to work with stdin and stdout
        perror ("gzip failed");
        return -1;
    }
    else {
        //parent code
        return parent_code (pipe_in_fds, pipe_out_fds, fd);
    }
    return 0;
}

int parent_code (int pipe_in_fds[2], int pipe_out_fds[2], int fd)
{
    #define CLOSE_RETURN(X) \
            close (pipe_in_fds[1]);\
            close (pipe_out_fds[0]);\
            close (fd);\
            return X
    #define CLOSE_FREE_RETURN(X) \
            close (pipe_in_fds[1]);\
            close (pipe_out_fds[0]);\
            close (fd);\
            free (buf_in);\
            free (buf_out);\
            return X

    close (pipe_in_fds[0]); // close "выпись"
    close (pipe_out_fds[1]); // close "впись"
    int pipe_in_in_flags = fcntl (pipe_in_fds[1], F_GETFL); //make pipe_in input O_NONBLOCK
    pipe_in_in_flags |= O_NONBLOCK;
    if (fcntl (pipe_in_fds[1], F_SETFL, pipe_in_in_flags)){
        perror ("can't make pipe_in input O_NONBLOCK");
           CLOSE_RETURN(-1);
    }
    //init poll structs
    struct pollfd stdin_pipe_out_out[2] = {0};
    stdin_pipe_out_out[1].fd = STDIN_FILENO;
    stdin_pipe_out_out[1].events = POLLIN;
    stdin_pipe_out_out[0].fd = pipe_out_fds[0];
    stdin_pipe_out_out[0].events = POLLOUT | POLLHUP;
    //init buffers, sizes, flags and so on
    char eof_flag = 0;
    size_t totally_read = 0, totally_written = 0;
    int bytes_read = 0, bytes_written = 0, bytes_diff = 0;
        //determine min size of 2 pipes and allocate 2 buffers of that size
    int buf_sz = pipe_size (pipe_in_fds[1]);
    int tmp = pipe_size (pipe_out_fds[0]);
    if (tmp < buf_sz)
        buf_sz = tmp;
    char *buf_in = NULL, *buf_out = NULL;
    if (alloc_bufer (&buf_in, buf_sz) == -1 || alloc_bufer (&buf_out, buf_sz) == -1) {
        CLOSE_RETURN(-1);
    }
    //we sleep until we have nothing to do, on receiving work, redirect data to gzip (or dump gzip output to file) and print statistics, timeount is infinite.
    while (!eof_flag) {
        if (poll (stdin_pipe_out_out, 2, -1) == -1) {
            perror ("poll () crashed");
            CLOSE_FREE_RETURN(-1);
        }
        switch (stdin_pipe_out_out[0].revents) {
            case POLLHUP: // hang up (gzip crashed or ended)
            case POLLWRNORM: // same as pollout
            case POLLOUT: 
                //read & dump to file
                bytes_read = 0;
                bytes_written = 0;
                bytes_read = read (pipe_out_fds[0], buf_out, (size_t) buf_sz);
                if (!bytes_read) { //in pollout case,  != 0, else gzip already finished, eof.
                    close (pipe_out_fds[0]);
                    eof_flag = 1;
                    break;
                }
                bytes_written = write (fd, buf_out, (size_t) bytes_read);
                totally_written += bytes_written;
                break;
            case POLLNVAL:
            case POLLERR:
                printf ("something went wrong with gzip\n");
                CLOSE_FREE_RETURN(-1);
            default:
                break;
        }
        switch (stdin_pipe_out_out[1].revents) {
            case POLLRDNORM:
            case POLLIN:
                // read & redirect
                bytes_read = 0;
                bytes_written = 0;
                bytes_diff = 0; // difference between read and written bytes
                bytes_read = read (stdin_pipe_out_out[1].fd, buf_in, (size_t) buf_sz);
                if (!bytes_read) { //if nothing to read left, we must send eof (drop pipe) 
                    close (pipe_in_fds[1]);
                    stdin_pipe_out_out[1].events = 0; // don`t bother about stdin anymore. It works as after that gzip will process left data and hang up pipe_out
                    break;
                }
                bytes_written = write (pipe_in_fds[1], buf_in, (size_t) bytes_read);
                bytes_diff = bytes_read - bytes_written;
                if (bytes_diff)
                    lseek (stdin_pipe_out_out[1].fd, -bytes_diff, SEEK_CUR); // return on position from whitch we have`t sent data
                totally_read += bytes_read;
                break;
            case POLLHUP: //impossible for stdin (or pipe выпись)
            case POLLNVAL: 
            case POLLERR:
                printf ("something went wrong with STDIN\n"); // errors are not possible in this particular case, however...
                CLOSE_FREE_RETURN(-1);
            default:
                break;
        }
        stdin_pipe_out_out[0].revents = 0;
        stdin_pipe_out_out[1].revents = 0;
        //print_statistics
        printf ("[%ld] totally read: %lu, totally written: %lu, compression ratio: %g\n",\
                        time (NULL) - g_start_time, totally_read, totally_written, ((float) totally_written) / ((float) totally_read));
    }
    return 0;
}

void sigint_handler (int sig)
{
    if (sig == SIGINT)
        printf ("\nPoka...\n");
    exit (0);
}

#if 0
// info about how musc can we read, no use
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
#endif

int pipe_size (int pipefd) 
{
    int pipe_sz = fcntl (pipefd, F_GETPIPE_SZ);
    if (pipe_sz == -1 || pipe_sz == 0) {
        perror ("can't get pipe size");
        return -1;
    }
    return pipe_sz;
}

int alloc_bufer (char **buf, size_t buf_sz)
{
    if (buf_sz <= 0)
        return -1;
    *buf = (char *) calloc (buf_sz, sizeof (char));
    if (*buf == NULL) {
        perror ("calloc failed");
        return -1;
    }
    return 0;
}

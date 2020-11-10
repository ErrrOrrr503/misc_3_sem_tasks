#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <linux/limits.h>
#include <fcntl.h>
#include <poll.h>
#include <time.h>
#include <sys/ioctl.h>
#include <memory.h>

extern int errno;

struct buffer {
    char *buf;
    size_t buf_sz;
    size_t data_start; //first position with data
    size_t data_end;  //first position without data after data
};

time_t g_start_time = 0;

int pipe_size (int pipefd);
int parent_code (int pipe_in_fds[2], int pipe_out_fds[2], int fd);

int how_much_read_avail (int fd);
int alloc_buffer (char **buf, size_t buf_sz);

int struct_buffer_init (struct buffer *buf, size_t buf_sz);
int buffer_realloc_defrag (struct buffer *buf, size_t new_sz);
ssize_t buf_write (int fd, struct buffer *buf);
ssize_t buf_read (int fd, struct buffer *buf);
int buf_defrag (struct buffer *buf);

/*
Gets info from stdin and gzips it to output_file
*/

/* Questions:
    1) Why FIONREAD accepts int as an arg
    2) Why poll() always returns POLLIN event even when eof is met?
*/
int main (int argc, char *argv[])
{
    g_start_time = time (NULL);
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
            free (buf_out);\
            free (s_buf_in.buf);\
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
    struct pollfd pollfds[3] = {0};
    int pollfds_num = 3;
    struct pollfd *stdin_pfd = &pollfds[0];
    struct pollfd *pipe_read_pfd = &pollfds[1];
    struct pollfd *pipe_write_pfd = &pollfds[2];
    stdin_pfd->fd = STDIN_FILENO;
    stdin_pfd->events = POLLIN;
    pipe_read_pfd->fd = pipe_out_fds[0];
    pipe_read_pfd->events = POLLIN | POLLHUP;
    pipe_write_pfd->fd = pipe_in_fds[1];
    pipe_write_pfd->events = 0;
    //init buffers, sizes, flags and so on
    char eof_flag = 0;
    size_t totally_read = 0, totally_written = 0, totally_proceeded = 0, totally_saved = 0;
    int bytes_read = 0, bytes_written = 0;
        //determine min size of 2 pipes and allocate 2 buffers of that size (will definately work and pipes of different sizes... no reason for that)
    int buf_sz = pipe_size (pipe_in_fds[1]);
    int tmp = pipe_size (pipe_out_fds[0]);
    if (tmp < buf_sz)
        buf_sz = tmp;
    char *buf_out = NULL;
    struct buffer s_buf_in = {0};
    if (alloc_buffer (&buf_out, buf_sz) == -1 || struct_buffer_init (&s_buf_in, buf_sz) == -1) {
        CLOSE_RETURN(-1);
    }
    //we sleep while we have nothing to do, on receiving work, redirect data to gzip (or dump gzip output to file) and print statistics, timeount is infinite.
    while (!eof_flag) {
        if (poll (pollfds, pollfds_num, -1) == -1) {
            perror ("poll () crashed");
            CLOSE_FREE_RETURN(-1);
        }

        switch (pipe_read_pfd->revents) {
            case POLLHUP: // hang up (gzip crashed or ended)
            case POLLRDNORM: // same as pollout
            case POLLIN:
            case POLLIN | POLLHUP: // crutch... todo: switch to if from switch, less understandable, but will work more properly
                //read & dump to file
                bytes_read = 0;
                bytes_written = 0;
                bytes_read = read (pipe_read_pfd->fd, buf_out, (size_t) buf_sz);
                totally_proceeded += bytes_read;
                if (!bytes_read) { //in pollin case,  != 0, else gzip already finished, eof.
                    close (pipe_out_fds[0]);
                    eof_flag = 1;
                    break;
                }
                bytes_written = write (fd, buf_out, (size_t) bytes_read);
                totally_saved += bytes_written;
                break;
            case POLLNVAL:
            case POLLERR:
                printf ("something went wrong with gzip\n");
                CLOSE_FREE_RETURN(-1);
            default:
                break;
        }
        
        switch (stdin_pfd->revents) {
            case POLLRDNORM:
            case POLLIN:
                // read & redirect
                bytes_read = 0;
                bytes_read = buf_read (stdin_pfd->fd, &s_buf_in);
                if (bytes_read == -1) {
                    CLOSE_FREE_RETURN(-1);
                }                    
                if (!bytes_read) {
                    stdin_pfd->events = 0; //if no data left, don't bother about stdin
                    if (s_buf_in.data_end - s_buf_in.data_start == 0) { //moreover if bufer is empty, hang up pipe
                        close (pipe_write_pfd->fd);
                        pollfds_num--;
                    }
                    break;
                }
                if (s_buf_in.data_end - s_buf_in.data_start > 0)
                    pipe_write_pfd->events = POLLOUT;
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

        switch (pipe_write_pfd->revents) {
            case POLLWRNORM:
            case POLLOUT:
                bytes_written = buf_write (pipe_write_pfd->fd, &s_buf_in);
                if (bytes_written == -1) {
                    CLOSE_FREE_RETURN(-1);
                }
                if (s_buf_in.data_end - s_buf_in.data_start == 0) { //if buffer is empty
                    pipe_write_pfd->events = 0; //then no sense to track pipe_write
                    if (stdin_pfd->events == 0) { //moreover if there is no new data possible
                        close (pipe_write_pfd->fd); //hang up pipe. After that gzip will process left data and hang up pipe_out
                        pollfds_num--; //we can't even try to track closed fd (inval)
                    }
                }
                totally_written += bytes_written;
                break;
            case POLLHUP: //impossible
            case POLLNVAL: 
            case POLLERR:
                printf ("something went wrong with pipe write\n"); // errors are not possible in this particular case, however...
                CLOSE_FREE_RETURN(-1);
            default:
                break;
        }
        //clear happened events
        pipe_read_pfd->revents = 0;
        pipe_write_pfd->revents = 0;
        stdin_pfd->revents = 0;
        //print_statistics
        printf ("[%ld] totally :: read: %lu, written: %lu, proceeded: %lu, saved: %lu, compression ratio: %g\n",\
                        time (NULL) - g_start_time, totally_read, totally_written, totally_proceeded, totally_saved,\
                        ((float) totally_saved) / ((float) totally_read));
    }
    free (buf_out);
    free (s_buf_in.buf);
    close (fd);
    return 0;
}

int how_much_read_avail (int fd)
{
    int bytes_availible = 0;
    if (ioctl (fd, TIOCINQ, &bytes_availible) == -1) {
        perror ("can't determine amount of free pipe bytes (via ioctl FIONREAD)");
        return -1;
    }
    printf ("fd_read_avail: %d\n", bytes_availible);
    return bytes_availible;
}

int pipe_size (int pipefd) 
{
    int pipe_sz = fcntl (pipefd, F_GETPIPE_SZ);
    if (pipe_sz == -1 || pipe_sz == 0) {
        perror ("can't get pipe size");
        return -1;
    }
    return pipe_sz;
}

int alloc_buffer (char **buf, size_t buf_sz)
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

int buf_defrag (struct buffer *buf)
{
    //plain
    size_t data_len = buf->data_end - buf->data_start;
    for (size_t i = 0; (i < data_len) && (buf->data_start > 0); i++) {
        buf->buf[i] = buf->buf[i + buf->data_start];
    }
    buf->data_start = 0;
    buf->data_end = data_len;
    return 0;
}

ssize_t buf_read (int fd, struct buffer *buf)
{
    int read_avail = how_much_read_avail (fd);
    if (read_avail < 0)
        return -1;
    if (read_avail == 0)
        return 0;
    if ((buf->buf_sz - (buf->data_end - buf->data_start)) < (size_t) read_avail) { //if free space is less than needed
        if (buffer_realloc_defrag (buf, (buf->data_end - buf->data_start) + (size_t) read_avail))
            return -1;
    }
    if (buf->buf_sz - buf->data_end < (size_t) read_avail) {
        buf_defrag (buf);
    }
    ssize_t bytes_read = read (fd, &buf->buf[buf->data_end], read_avail);
    if (bytes_read != read_avail)
        return -1;
    buf->data_end += bytes_read;
    return bytes_read;
}

ssize_t buf_write (int fd, struct buffer *buf)
{
    ssize_t bytes_written = write (fd, &buf->buf[buf->data_start], buf->data_end - buf->data_start);
    if (bytes_written == -1) {
        perror ("can't write to fd");
        return -1;
    }
    buf->data_start += bytes_written;
    return bytes_written;
}

int buffer_realloc_defrag (struct buffer *buf, size_t new_sz)
{
    char *newbuf = NULL;
    if (alloc_buffer (&newbuf, new_sz) == -1)
        return -1;
    size_t data_len = buf->data_end - buf->data_start;
    memcpy (newbuf, &buf->buf[buf->data_start], data_len);
    free (buf->buf);
    buf->buf = newbuf;
    buf->data_start = 0;
    buf->data_end = data_len;
    buf->buf_sz = new_sz;
    return 0;
}

int struct_buffer_init (struct buffer *buf, size_t buf_sz)
{
    if (alloc_buffer (&buf->buf, buf_sz))
        return -1;
    buf->buf_sz = buf_sz;
    buf->data_start = 0;
    buf->data_end = 0;
    return 0;
}
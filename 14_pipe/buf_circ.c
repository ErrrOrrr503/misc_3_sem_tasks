#include "buf_circ.h"

int how_much_read_avail (int fd)
{
    // int is not that much according the standart, however 2^63 (on x64 int is 64 bit) is quite many bytes
    int bytes_available = 0;
    if (ioctl (fd, TIOCINQ, &bytes_available) == -1) {
        perror ("can't determine amount of free pipe bytes (via ioctl FIONREAD)");
        return -1;
    }
    return bytes_available;
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

ssize_t buf_read (int fd, struct buffer *buf)
{
    #define MIN(a,b) (((a) < (b)) ? (a) : (b))

    int read_avail = how_much_read_avail (fd);
    if (read_avail < 0)
        return -1;
    if (read_avail == 0)
        return 0;
    
    ssize_t read_amount_end = 0;
    ssize_t read_amount_start = 0;
    if (buf->flag_loop) { //data looped
        read_amount_start = 0;
        read_amount_end = MIN((ssize_t) read_avail, buf->data_start - buf->data_end);
    }
    else { //data not looped
        read_amount_end = MIN((ssize_t) read_avail, buf->buf_sz - buf->data_end);
        read_amount_start = MIN((ssize_t) read_avail - read_amount_end, buf->data_start);
    }

    ssize_t bytes_read = 0;
    if (read_amount_end)
        bytes_read += read (fd, &buf->buf[buf->data_end], read_amount_end);
    if (read_amount_start)
        bytes_read += read (fd, buf->buf, read_amount_start);
    if (bytes_read != read_amount_end + read_amount_start)
        return -1;
    if (read_amount_start) {
        buf->data_end = read_amount_start;
        buf->flag_loop = 1;
    }
    else
        buf->data_end += read_amount_end;
    return bytes_read;
}

ssize_t buf_write (int fd, struct buffer *buf)
{
    if (buf->flag_loop) { //data looped
        ssize_t bytes_written_end = 0;
        ssize_t bytes_written_start = 0; 
        if (buf->buf_sz - buf->data_start)
            bytes_written_end = write_retry (fd, &buf->buf[buf->data_start], buf->buf_sz - buf->data_start, 100); //write end
        if (bytes_written_end == buf->buf_sz - buf->data_start) { //if we have written all the end, write start
            if (buf->data_end)
                bytes_written_start = write_retry (fd, buf->buf, buf->data_end, 100);
        }
        if (bytes_written_start) {
            buf->data_start = bytes_written_start;
            buf->flag_loop = 0;
        }
        else
            buf->data_start += bytes_written_end;
        return bytes_written_end + bytes_written_start;
    }
    else {
        ssize_t bytes_written = write_retry (fd, &buf->buf[buf->data_start], buf->data_end - buf->data_start, 100);
        if (bytes_written == -1) {
            perror ("can't write to fd");
            return -1;
        }
        buf->data_start += bytes_written;
        return bytes_written;
    }
}

int struct_buffer_init (struct buffer *buf, size_t buf_sz)
{
    if (alloc_buffer (&buf->buf, buf_sz))
        return -1;
    buf->buf_sz = buf_sz;
    buf->data_start = 0;
    buf->data_end = 0;
    buf->flag_loop = 0;
    return 0;
}

ssize_t data_amount (struct buffer * buf)
{
    if (buf->flag_loop)
        return buf->buf_sz - (buf->data_end - buf->data_start);
    else
        return buf->data_end - buf->data_start;
}

ssize_t write_retry (int fd, const void *buf, size_t count, size_t retries)
{
    ssize_t written = 0;
    for (
        size_t i = 0;
        i < retries
        && (written = write (fd, buf, count)) == -1
        && errno == EAGAIN;
        i++
        ) ;
    return written;
}
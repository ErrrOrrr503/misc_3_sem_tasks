#include "buffer.h"

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
    
    if ((buf->buf_sz - buf->data_end < (size_t) read_avail) && (buf->data_start > 0)) {
        buf_defrag (buf);
    }
    ssize_t read_amount = (ssize_t) read_avail;
    if (buf->buf_sz - buf->data_end < (size_t) read_amount)
        read_amount = buf->buf_sz - buf->data_end;
    if (!read_amount)
        return 0;
    ssize_t bytes_read = read (fd, &buf->buf[buf->data_end], read_amount);
    if (bytes_read != read_amount)
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

int struct_buffer_init (struct buffer *buf, size_t buf_sz)
{
    if (alloc_buffer (&buf->buf, buf_sz))
        return -1;
    buf->buf_sz = buf_sz;
    buf->data_start = 0;
    buf->data_end = 0;
    return 0;
}

size_t data_amount (struct buffer * buf)
{
    return buf->data_end - buf->data_start;
}
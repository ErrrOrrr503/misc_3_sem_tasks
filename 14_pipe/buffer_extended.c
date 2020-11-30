int how_much_read_avail (int fd)
{
    int bytes_availible = 0;
    if (ioctl (fd, TIOCINQ, &bytes_availible) == -1) {
        perror ("can't determine amount of free pipe bytes (via ioctl FIONREAD)");
        return -1;
    }
    return bytes_availible;
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
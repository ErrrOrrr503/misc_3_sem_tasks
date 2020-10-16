#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <linux/limits.h>
#include <fcntl.h>

extern int errno;

int read_counter (int fd, long unsigned *num);
int write_counter (int fd, long unsigned num);

int main (int argc, char *argv[])
{
	if (argc > 1) {
		printf ("no options needed for '%s'\n", argv[0]);
		return 1;
	}
	int fd = open ("counter.txt", O_RDWR | O_CREAT, 0600); //user rw
	if (fd == -1) {
		perror ("can't open counter file");
		return -1;
	}
	long unsigned num = 0;
	if (read_counter (fd, &num)) {
		close (fd);
		return -2;
	}
	//now we have counter.txt opened, locked and read
	if (write_counter (fd, num + 1)) {
		close (fd);
		return -2;
	}
	close (fd);
	return 0;
}

int read_counter (int fd, long unsigned *num)
{
	struct flock fl = {0};
	fl.l_type = F_WRLCK;    //Type of blocking lock found.
	fl.l_whence = SEEK_SET;
	fl.l_start = 0; //Start of the blocking lock.
	fl.l_len = 0;   // lock the whole file (even if it will be truncated)
	fl.l_pid = getpid ();    //Process ID of the process that holds the blocking lock.

	if (fcntl (fd, F_SETLKW, &fl) == -1) { //wait when other process will release lock and then lock
		perror ("can't lock file");
		return -1;
	}
	//now successfully locked
	char str[80] = {0};
	if (read (fd, str, 80) == -1) {
		perror ("can't read counter file");
		return -1;
	}
	sscanf (str, "%lu", num);
	return 0;
}

int write_counter (int fd, long unsigned num)
{
	if (fd == -1)
		return -2;
	if (ftruncate (fd, 0) == -1 || lseek (fd, 0, SEEK_SET) == -1) {
		perror ("can't truncate the counter file");
		return -1;
	}
	char str[80] = {0}; // enough to store 2^64 (dec);
	sprintf (str, "%lu", num);
	write (fd, str, strlen (str));
	//unlock
	struct flock fl = {0};
	fl.l_type = F_UNLCK;    //Type of blocking lock found.
	fl.l_whence = SEEK_SET;
	fl.l_start = 0; //Start of the blocking lock.
	fl.l_len = 0;   // unlock the whole file (even if it will be truncated)
	fl.l_pid = getpid ();    //Process ID of the process that holds the blocking lock.

	if (fcntl (fd, F_SETLK, &fl) == -1) {
		perror ("can't unlock file");
		return -1;
	}
	//now successfully locked
	return 0;
}
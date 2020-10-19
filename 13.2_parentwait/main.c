#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <pwd.h>
#include <linux/limits.h>
#include <sys/wait.h>
#include <poll.h>
#include <sys/syscall.h>

/*
Questions: Why we need 2 different poll events for all exept High prio and High prio is readable?
*/

int print_p_info_small (const char* description);

int main (int argc, char *argv[])
{
	if (argc > 1) {
		printf ("'%s' needs no options\n", argv[0]);
		return 1;
	}
	int child_id = fork ();
	if (child_id == -1) {
		perror ("can't create child process");
		return -1;
	}
	if (!child_id) {
		//child code
		int ppidfd = syscall (SYS_pidfd_open, getppid (), 0); // SYS_pidfd_open is same as __NR_pidfd_open that is 434 on most systems
		if (ppidfd == -1) {
			perror ("can't init parent pidfd");
			return -1;
		}
		print_p_info_small ("child");
		struct pollfd parent_pfd = {0};
		parent_pfd.fd = ppidfd;
		parent_pfd.events = POLLIN | POLLPRI; // when patent terminated, ppidfd becomes readable. POLLIN: data exept high-priority is readable. POLLPRI: data of high prio is readable. So, apply both
		if (poll (&parent_pfd, 1, -1) == -1) {  //wait until (-1 no timeout) parent termination is reported
			perror ("an error during waiting parent to exit");
			close (ppidfd);
			return -1;
		}
		print_p_info_small ("child, parent finished");
		return 0;
	}
	else {
		//parent code
		print_p_info_small ("parent");
		sleep (1);
		return 0;
	}
	//both code
	return 0;
}

int print_p_info_small (const char* description)
{
	printf ("[%s] PID: %d	PPID: %d\n", description, getpid (), getppid ());
	return 0;
}
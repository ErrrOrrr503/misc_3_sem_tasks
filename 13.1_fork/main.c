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
/*
Questions: is it really enough? shall we implement for example waitpid with
WUNTRACED and WCONTINUED and "trace" child state until it is terminated?
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
		print_p_info_small ("child");
		#ifdef SIG
		kill (getpid (), SIGTERM); // suicide:)
		#endif
		return 153;
	}
	else {
		//parent code
		int wstatus = 0;
		if (waitpid (-1, &wstatus, 0) == -1) { //wait for termination of any (-1) child process
			perror ("somewhat error on waiting child to exit");
			return -1;
		}
		print_p_info_small ("parent");
		if (WIFEXITED (wstatus))
			printf ("child exited with code: %d\n", WEXITSTATUS (wstatus));
		if (WIFSIGNALED (wstatus)) {
			printf ("child terminated by signal: %d : %s\n", WTERMSIG (wstatus), strsignal (WTERMSIG (wstatus)));
			if (WCOREDUMP (wstatus))
				printf ("child performed a core dump\n");
		}
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
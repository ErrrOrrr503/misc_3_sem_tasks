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
#include <grp.h>
#include <sched.h>
#include <linux/limits.h>
#include <sys/capability.h>
#include <sys/resource.h>
#include <fcntl.h>

extern int errno;

int print_getpwuid (uid_t uid, const char *uid_descr);
int print_capabilities (pid_t pid);
int print_subgroups ();
int print_sched_details ();
int print_cpu_affinity ();
int print_rlimit (int resource, const char *res_name);
int print_limits ();
int print_proc_status (pid_t pid);

int main (int argc, char *argv[])
{
	if (argc > 1) {
		printf ("no options needed\n");
		return 1;
	}
	printf ("'%s':\n", argv[0]);
	//current working directory
	char cwd_name[PATH_MAX] = {0};
	if (getcwd (cwd_name, PATH_MAX) == NULL) {
		perror (NULL);
		return -1;
	}
	printf ("working dir is: '%s'\n", cwd_name);
	//umask
	printf ("umask is: '%04o'\n", umask (0777)); //syscall is always successful
	//process uids and gids: real, effective and saved
	printf ("\nuids and gids of the process:\n");
	uid_t euid = 0, ruid  = 0, suid = 0;
	gid_t egid = 0, rgid  = 0, sgid = 0;
	if (getresuid (&euid, &ruid, &suid) != 0 || getresgid (&egid, &rgid, &sgid) != 0) {
		perror ("can't read uid&gid ");
		return -1;
	}
	printf ("%-6s%12s%12s%12s\n", "", "effective:", "real:", "saved:" );
	printf ("%-6s%12d%12d%12d\n", "uid:", euid, ruid, suid);
	printf ("%-6s%12d%12d%12d\n", "gid:", egid, rgid, sgid);
	//user details from password database
	print_getpwuid (euid, "\neffective user id");
	if (ruid != euid)
		print_getpwuid (ruid, "\nreal user id");
	if (suid != euid)
		print_getpwuid (suid, "\nsaved user id");
	print_subgroups ();
	print_capabilities (0);
	print_sched_details ();
	print_cpu_affinity ();
	//affinity test
	cpu_set_t coremask;
	CPU_ZERO (&coremask);
	CPU_SET (11, &coremask);
	sched_setaffinity (0, sizeof(cpu_set_t), &coremask);
	print_cpu_affinity ();
	//
	print_limits ();
	print_proc_status (0);
	return 0;
}

int print_getpwuid (uid_t uid, const char *uid_descr)
{
	printf ("%s details:\n", uid_descr);
	struct passwd *pwd_uid = getpwuid (uid);
	if (pwd_uid == NULL) {
		perror ("can't read details from password database");
		return -1;
	}
	printf ("username: '%s'\n", pwd_uid->pw_name);
	printf ("password: '%s'\n", pwd_uid->pw_passwd);
	printf ("user information: '%s'\n", pwd_uid->pw_gecos);
	printf ("user home dir: '%s'\n", pwd_uid->pw_dir);
	printf ("user default shell: '%s'\n", pwd_uid->pw_shell);
	return 0;
}

int print_subgroups ()
{
	printf ("\nsubgroups info:\n");
	gid_t group_list[NGROUPS_MAX] = {0};
	int group_num = getgroups (NGROUPS_MAX, group_list);
	if (group_num == -1) {
		perror ("can't get group list");
		return -1;
	}
	printf ("%-6s%20s\n", "gid:", "group_name:");
	for (int i = 0; i < group_num; i++) {
		struct group *group = getgrgid (group_list[i]);
		if (group == NULL) {
			perror ("can't get group name from gid");
			return -1;
		}
		printf ("%-6d%20s\n", group_list[i], group->gr_name);
	}
	return 0;
}

int print_capabilities (pid_t pid)
{
	//prints capatibiliries (effective, inherited and permitted) of process referred to by pid.
	//if pid is 0 capabilities of the caller process are printed
	printf ("\nprocess capabilities:\n");
	cap_t cap = cap_init ();
	cap = cap_get_pid (pid);
	if (cap == NULL) {
		perror ("can't get pid capabilities");
		cap_free (cap);
		return -1;
	}
	char *cap_str = cap_to_text (cap, NULL);
	if (cap_str == NULL) {
		perror ("can't convert capabilities to text");
		cap_free (cap);
		return -1;
	}
	printf ("%s\n", cap_str);
	cap_free (cap);
	cap_free (cap_str);
	return 0;
}

int print_sched_details ()
{
	//prio section
	printf ("\npriority details:\n");
	int prio = 0;
	prio = getpriority (PRIO_PROCESS, 0);
	if (prio == -1)
		perror ("can't get process priority");
	else
		printf ("%-20s%6d\n", "process_prio:", prio);
	prio = getpriority (PRIO_USER, getuid ());
	if (prio == -1)
		perror ("can't get user priority");
	else
		printf ("%-20s%6d\n", "user_prio:", prio);
	prio = getpriority (PRIO_PGRP, getpgrp ());
	if (prio == -1)
		perror ("can't get process group priority");
	else
		printf ("%-20s%6d\n", "process_grp_prio:", prio);
	return 0;
}

int print_cpu_affinity ()
{
	cpu_set_t cpu_set;
	printf ("\ncpu affinity details:\n");
	if (sched_getaffinity (0, sizeof (cpu_set_t), &cpu_set) != 0) {
		perror ("can't get cpu_affinity");
		return -1;
	}
	printf ("affiliated cpus: ");
	for (int i = 0; i < CPU_SETSIZE; i++) {
		if (CPU_ISSET (i, &cpu_set)) {
			printf ("%d ", i);
		}
	}
	printf ("\n");
	return 0;
}

int print_limits ()
{
	printf ("\nprocess limits:\n");
	printf ("%-20s%10s%10s\n", "resource name:", "soft:", "hard:");
	print_rlimit (RLIMIT_CORE, "core_file_size");
	print_rlimit (RLIMIT_CPU, "cpu_time");
	print_rlimit (RLIMIT_DATA, "data_segment");
	print_rlimit (RLIMIT_FSIZE, "regular_file_size");
	print_rlimit (RLIMIT_NOFILE, "amount_of_fds");
	print_rlimit (RLIMIT_STACK, "stack_size");
	print_rlimit (RLIMIT_AS, "total_mem");
	return 0;
}

int print_rlimit (int resource, const char *res_name)
{
	printf ("%-20s", res_name);
	struct rlimit rlp = {0};
	if (getrlimit (resource, &rlp) == -1) {
		perror ("can't get the resource limits");
		return -1;
	}
	if (rlp.rlim_cur == RLIM_INFINITY)
		printf ("%10s", "unlimited");
	else
		printf ("%10lu", rlp.rlim_cur);
	if (rlp.rlim_max == RLIM_INFINITY)
		printf ("%10s\n", "unlimited");
	else
		printf ("%10lu\n", rlp.rlim_max);
	return 0;
}

int print_proc_status (pid_t pid)
{
	char filename[PATH_MAX] = "/proc/", pidname[10];
	if (pid)
		sprintf (pidname, "%u", pid);
	else 
		sprintf (pidname, "self");
	strcat (filename, pidname);
	strcat (filename, "/status");
	printf ("\nraw %s info:\n", filename);
	int fd = open (filename, O_RDONLY);
	if (fd == -1) {
		perror ("can't open /proc/<pid>/status");
		return -1;
	}
	size_t bs = 32768;
	char *buffer = (char *) calloc (bs, sizeof (char));
	ssize_t read_bytes = 0;
	while ((read_bytes = read (fd, buffer, bs)) != 0) {
		if (read_bytes == (ssize_t) -1) {
			perror ("can't read /proc/<pid>/status");
			return -1;
		}
		write (STDOUT_FILENO, buffer, read_bytes);
	}
	free (buffer);
	return 0;
}
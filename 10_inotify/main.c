#include <sys/inotify.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <linux/limits.h>

extern int errno;

volatile char g_termination_flag = 0;

void sig_handler (int sig);
int print_event_info (const char * wdir, const struct inotify_event *event);

int main (int argc, char *argv[])
{
    if (argc != 2) {
        printf("usage: <%s> <tracked directory>\n", argv[0]);
        return -1;
    }
    // signal handler init
    struct sigaction act = {0};
    act.sa_handler = sig_handler; //new handler   
        // do not need sa_sigaction (used with SA_SIGINFO)
        // do not need to block other signals
        // do not need special flags
        // sa_restorer not even in posix
    int ret = sigaction (SIGINT, &act, NULL);
    ret |= sigaction (SIGTERM, &act, NULL);
    if (ret) {
        perror ("sigaction error");
        return -1;
    }
    // inotify init
    int inot = inotify_init ();
    if (inot == -1) {
        perror ("can't init inotify instance");
        return -1;
    }
    int dir_inot = inotify_add_watch (inot, argv[1], IN_CREATE | IN_DELETE | IN_MODIFY | IN_ATTRIB | IN_ACCESS | IN_OPEN | IN_DELETE_SELF | IN_MOVE_SELF | IN_MOVED_FROM | IN_MOVED_TO);
    if (dir_inot == -1) {
        perror ("can't add dir to watch list");
        return -1;
    }
    // main cycle: reporting about events
    size_t buf_sz = sizeof (struct inotify_event) + NAME_MAX;
    struct inotify_event *event = (struct inotify_event *) calloc (buf_sz, sizeof (char)); // allocate enough for struct with name[]
    if (event == NULL) {
        perror ("can't allocate buffer");
        close (dir_inot);
        close (inot);
        return -1;
    }
    while (1) {
        if (read (inot, event, buf_sz) == -1) {
            close (dir_inot);
            close (inot);
            free (event);
            if (g_termination_flag)
                return 0;
            perror ("can't read inotify event");;
            return -1;
        }
        print_event_info (argv[1], event);
    }   
}

void sig_handler (int sig)
{
    g_termination_flag = 1;
    printf ("\n");
}

int print_event_info (const char * wdir, const struct inotify_event *event)
{
    printf ("'%s' : ", wdir);
    if (event->mask & IN_UNMOUNT) {
        printf ("fs was unmounted\n");
        return 0;
    }
    if (event->mask & IN_MOVE_SELF) {
        printf ("moved\n");
        return 0;
    }
    if (event->mask & IN_DELETE_SELF) {
        printf ("deleted\n");
        return 0;
    }
    if (event->len) {
        printf ("'%s' ", event->name);
        if (event->mask & IN_ISDIR)
            printf ("[dir] ");
        else
            printf ("[file] ");
    }
    if (event->mask & IN_CREATE)
        printf ("created");
    if (event->mask & IN_DELETE)
        printf ("deleted");
    if (event->mask & IN_MODIFY)
        printf ("modified");
    if (event->mask & IN_ATTRIB)
        printf ("metadata modified");
    if (event->mask & IN_ACCESS)
        printf ("accessed");
    if (event->mask & IN_OPEN)
        printf ("opened");
    if (event->mask & IN_MOVED_FROM)
        printf ("moved outside");
    if (event->mask & IN_MOVED_TO)
        printf ("moved inside");
    printf ("\n");
    return 0;
}
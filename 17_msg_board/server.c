#include <fcntl.h>
#include <sys/stat.h>
#include <mqueue.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <linux/limits.h>
#include <signal.h>

extern int errno;

volatile char g_flag_termination = 0;

char * check_mq_name_alloc (const char *raw_name);
ssize_t mq_receive_alloc (mqd_t mq, char **buf, unsigned *prio_received);
void sig_handler (int sig);

int main (int argc, char *argv[])
{
    if (argc != 2) {
        printf ("usage: '%s' <msg_queue_name>\n", argv[0]);
        return 0;
    }
    struct sigaction act = {0};
    act.sa_handler = sig_handler; //new handler   
        // do not need sa_sigaction (used with SA_SIGINFO)
        // do not need to block other signals
        // do not need special flags
        // sa_restorer not even in posix
    int ret = sigaction (SIGINT, &act, NULL);
    ret |= sigaction (SIGTERM, &act, NULL);
    if (ret) {
        perror ("sigacrion error");
        return -1;
    }
    //form mq_name, as it must be '/somename' 
    char *mq_name = check_mq_name_alloc (argv[1]);
    if (mq_name == NULL)
        return -1;
    printf ("formed mq_name is: '%s'\n", mq_name);
    //create queue
    mqd_t mq = mq_open (mq_name, O_RDONLY | O_CREAT | O_EXCL, 0644, NULL); // 0644 :rdwr for user (and server process) wronly for others. NULL: default params
    if (mq == -1) {
        perror ("can't open or create msg_queue");
        free (mq_name);
        return -1;
    }
    // receive msg
    while (1) {
        unsigned prio = 0;
        char *buf = NULL;
        ssize_t msg_sz = mq_receive_alloc (mq, &buf, &prio);
        if (msg_sz == -1 && !g_flag_termination) {
            perror ("can't receive msg");
            g_flag_termination = 1;
        }
        if (g_flag_termination) {
            mq_close (mq);
            mq_unlink (mq_name);
            if (buf != NULL)
                free (buf);
            free (mq_name);
            return -1;
        }
        //print msg
        printf ("received :'%s' size '%ld' of '%u' priority\n", buf, msg_sz, prio);
        free (buf);
    }
    return 0;
}

char * check_mq_name_alloc (const char *raw_name)
{
    //function checks queue name and forms correct one with (maybe) adding first '/'
    if (raw_name[0] == 0)
        return NULL;
    //check '/'
    size_t i = 1;
    for (; raw_name[i] != 0; i++) {
        if (raw_name[i] == '/') {
            printf ("wrong queue name, read man 7 mq_overview\n");
            return NULL;
        }
    }
    size_t raw_name_len = i;
    //check length
    if (raw_name_len + 1 > NAME_MAX) { // +1 to include '\0'
        printf ("queue name too long, read man 7 mq_overview\n");
        return NULL;
    }
    //form correct name
    char *mq_name = (char *) calloc (raw_name_len + 2, sizeof (char)); // 1 for '\0', one for '/'
    if (mq_name == NULL) {
        printf ("can't allocate memory for mq_name\n");
        return NULL;
    }
    mq_name[0] = '/';
    if (raw_name[0] != '/')
        strcpy (&mq_name[1], raw_name);
    else 
        strcpy (mq_name, raw_name);
    return mq_name;
}

ssize_t mq_receive_alloc (mqd_t mq, char **buf, unsigned *prio_received)
{
    struct mq_attr mq_attributes = {0};
    if (mq_getattr (mq, &mq_attributes)) {
        return -1;
    }
    *buf = (char *) calloc (mq_attributes.mq_msgsize + 1, sizeof (char)); // +1 for '\0'
    if (*buf == NULL) {
        return -1;
    }
    ssize_t msg_sz = mq_receive (mq, *buf, (size_t) mq_attributes.mq_msgsize, prio_received);
    if (msg_sz == -1) {
        return -1;
    }
    return msg_sz;
}

void sig_handler (int sig)
{
    g_flag_termination = 1;
    printf ("\nterminated by signal: %d : %s\n", sig, strsignal (sig));
}
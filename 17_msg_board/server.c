#include <mqueue.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <stdbool.h>

volatile bool g_flag_termination = 0;

char * mq_receive_alloc (mqd_t mq, ssize_t *msg_len, unsigned *prio_received);
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
    if (sigaction (SIGINT, &act, NULL) || sigaction (SIGTERM, &act, NULL)) {
        perror ("sigaction error");
        return -1;
    }
    const char *mq_name = argv[1];
    //create queue
    mqd_t mq = mq_open (mq_name, O_RDONLY | O_CREAT | O_EXCL, 0644, NULL); // 0644 :rdwr for user (and server process) wronly for others. NULL: default params
    if (mq == -1) {
        perror ("can't open or create msg_queue");
        return -1;
    }
    // receive msg
    char *msg = NULL;
    int ret = 0;
    while (!g_flag_termination) {
        unsigned prio = 0;
        ssize_t msg_len = 0;
        msg = mq_receive_alloc (mq, &msg_len, &prio);
        if (msg == NULL && !g_flag_termination) {
            perror ("can't receive msg");
            ret = -1;
            break;
        }
        //print msg
        printf ("received :'%s' size '%ld' of '%u' priority\n", msg, msg_len, prio); // msg is null terminated
    }
    mq_close (mq);
    mq_unlink (mq_name);
    if (msg != NULL)
        free (msg);
    return ret;
}

char * mq_receive_alloc (mqd_t mq, ssize_t *msg_len, unsigned *prio_received)
{
    static char * buf = NULL;
    static size_t buf_sz = 1;
    ssize_t msg_sz = mq_receive (mq, buf, buf_sz - 1, prio_received);
    if (msg_sz == -1 && errno == EMSGSIZE) { // then realloc buf and retry
        struct mq_attr mq_attributes = {0};
        if (mq_getattr (mq, &mq_attributes)) {
            return NULL;
        }
        if (buf != NULL)
            free (buf);
        buf_sz = mq_attributes.mq_msgsize + 1; // +1 for '\0'
        buf = (char *) calloc (buf_sz, sizeof (char));
        msg_sz = mq_receive (mq, buf, mq_attributes.mq_msgsize, prio_received);
    }
    if (msg_sz == -1) {
        return NULL;
    }
    buf[msg_sz] = 0; // null termination
    *msg_len = msg_sz;
    return buf;
}

void sig_handler (int sig)
{
    g_flag_termination = 1;
    printf ("\nterminated by signal: %d : %s\n", sig, strsignal (sig));
}
#include <mqueue.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

int check_mq_send (mqd_t mq, const char *msg, size_t sz, unsigned prio);

int main (int argc, char *argv[])
{
    if (argc != 3) {
        printf ("usage: '%s' <msg_queue_name> <message>\n", argv[0]);
        return 0;
    }
    const char *mq_name = argv[1];
    mqd_t mq = mq_open (mq_name, O_WRONLY);
    if (mq == -1){
        perror ("can't open msg_queue");
        return -1;
    }
    // send msg
    int ret = check_mq_send (mq, argv[2], 0, 0); //0 sor sz: automatic in this case ; 0 for prio: lowest
    mq_close (mq);
    return ret;
}

int check_mq_send (mqd_t mq, const char *msg, size_t sz, unsigned prio)
{
    //wrap for mq_send that checks limits. If sz == 0, msg len is calculated via strlen
    if (!sz)
        sz = strlen (msg);
    if (mq_send (mq, msg, sz, prio)) {
        perror ("can't send message");
        return -1;
    }
    return 0;
}
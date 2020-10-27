#include <fcntl.h>
#include <sys/stat.h>
#include <mqueue.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <linux/limits.h>

extern int errno;

char * check_mq_name_alloc (const char *raw_name);
int check_mq_send (mqd_t mq, const char *msg, size_t sz, unsigned prio);

int main (int argc, char *argv[])
{
    if (argc != 3) {
        printf ("usage: '%s' <msg_queue_name> <message>\n", argv[0]);
        return 0;
    }
    //form mq_name, as it must be '/somename' 
    char *mq_name = check_mq_name_alloc (argv[1]);
    if (mq_name == NULL)
        return -1;
    printf ("formed mq_name is: '%s'\n", mq_name);
    //open or create queue
    mqd_t mq = mq_open (mq_name, O_WRONLY);
    if (mq == -1){
        perror ("can't open msg_queue");
        free (mq_name);
        return -1;
    }
    // send msg
    int ret = check_mq_send (mq, argv[2], 0, 0); //0 sor sz: automatic; 0 for prio: lowest
    free (mq_name);
    return ret;
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
    if (raw_name_len > NAME_MAX) {
        printf ("queue name too long, read man 7 mq_overview\n");
        return NULL;
    }
    //form correct name
    char *mq_name = (char *) calloc (raw_name_len + 1, sizeof (char));
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

int check_mq_send (mqd_t mq, const char *msg, size_t sz, unsigned prio)
{
    //wrap for mq_send that chaecks limits. If sz == 0, full msg len is used instead
    struct mq_attr mq_attributes = {0};
    if (mq_getattr (mq, &mq_attributes)) {
        perror ("can't get msg_queue attributes");
        return -1;
    }
    if (!sz)
        sz = strlen (msg);
    if (sz > (size_t) mq_attributes.mq_msgsize) {
        printf ("msg size of '%lu' exceeds maximum of '%ld'\n", sz, mq_attributes.mq_msgsize);
        return -1;
    }
    if (prio > sysconf(_SC_MQ_PRIO_MAX) - 1) {
        printf ("prio exceedxs maximum of '%ld'\n", sysconf(_SC_MQ_PRIO_MAX) - 1);
        return -1;
    }
    if (mq_send (mq, msg, sz, prio)) {
        perror ("can't send message");
        return -1;
    }
    return 0;
}
#include <mqueue.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

int main (int argc, char *argv[])
{
    if (argc != 2) {
        printf ("usage: '%s' <msg_queue_name>\n", argv[0]);
        return 0;
    }
    char mq_was_opened_flag = 0;
    //form mq_name, as it must be '/somename' 
    const char *mq_name = argv[1];
    if (mq_name == NULL)
        return -1;
    printf ("formed mq_name is: '%s'\n", mq_name);
    //open or create queue
    mqd_t mq = mq_open (mq_name, O_RDONLY | O_CREAT | O_EXCL, 0600, NULL); // 0666 :rdwr for user/group/others. NULL: default params
    if (mq == -1 && errno == EEXIST) {
        printf ("msg_gueue '%s' already existed\n", mq_name);
        mq = mq_open (mq_name, O_RDONLY);
        mq_was_opened_flag = 1;
    }
    if (mq == -1){
        perror ("can't open or create msg_queue");
        return -1;
    }
    //get and print info
    struct mq_attr mq_attributes = {0};
    mq_getattr (mq, &mq_attributes); // errors are imposible here

    if (mq_attributes.mq_flags)
        printf ("mq_flags is: O_NONBLOCK\n");
    else 
        printf ("mq_flags is: 0\n");
    printf ("mq_maxmsg: maximum amount of messages in queue is: %ld\n", mq_attributes.mq_maxmsg);
    printf ("mq_msgsize: maximum message size is: %ld\n", mq_attributes.mq_msgsize);
    printf ("mq_curmsgs: amount of messages currently in queue is: %ld\n", mq_attributes.mq_curmsgs);

    mq_close (mq);
    if (!mq_was_opened_flag) {
        mq_unlink (mq_name); //errors are impossible here
    }
    return 0;
}
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

int main (int argc, char *argv[])
{
    if (argc != 2) {
        printf ("usage: '%s' <msg_queue_name>\n", argv[0]);
        return 0;
    }
    char mq_was_opened_flag = 0;
    //form mq_name, as it must be '/somename' 
    char *mq_name = check_mq_name_alloc (argv[1]);
    if (mq_name == NULL)
        return -1;
    printf ("formed mq_name is: '%s'\n", mq_name);
    //open or create queue
    mqd_t mq = mq_open (mq_name, O_RDWR | O_CREAT | O_EXCL, 0666, NULL); // 0666 :rdwr for user/group/others. NULL: default params
    if (mq == -1 && errno == EEXIST) {
        printf ("msg_gueue '%s' already existed\n", mq_name);
        mq = mq_open (mq_name, O_RDWR);
        mq_was_opened_flag = 1;
    }
    if (mq == -1){
        perror ("can't open or create msg_queue");
        return -1;
    }
    //get and print info
    struct mq_attr mq_attributes = {0};
    mq_getattr (mq, &mq_attributes); // error is imposible here

    if (mq_attributes.mq_flags)
        printf ("mq_flags is: O_NONBLOCK\n");
    else 
        printf ("mq_flags is: 0\n");
    printf ("mq_maxmsg: maximum amount of messages in queue is: %ld\n", mq_attributes.mq_maxmsg);
    printf ("mq_msgsize: maximum message size is: %ld\n", mq_attributes.mq_msgsize);
    printf ("mq_curmsgs: amount of messages currently in queue is: %ld\n", mq_attributes.mq_curmsgs);

    mq_close (mq);
    free (mq_name);
    if (!mq_was_opened_flag) {
        mq_unlink (mq_name); //errors are impossible here
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
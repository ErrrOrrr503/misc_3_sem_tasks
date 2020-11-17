#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <semaphore.h>
#include <pthread.h>
#include <time.h>
#include <fcntl.h>
#include <sys/stat.h>

// NOTE: as progress is a pointer, (volatile) is, it seems, nesessary

extern int errno;

typedef struct {
    sem_t *s;
    const char *sem_name;
    volatile unsigned *progress;
    unsigned long subtarget;
} thread_arg;

void *thread (void *arg_) {
    thread_arg *arg = (thread_arg *) arg_;
    sem_t *sem_mut = sem_open (arg->sem_name, 0); 
    if (sem_mut == SEM_FAILED)
        return NULL;
    for (unsigned int subprogress = 0; subprogress < arg->subtarget; subprogress++) { //subturget is const, no mutex/atomic needed
        // do magic of reptiles
        struct timespec st = {0, 20000000};
        nanosleep (&st, NULL);
        // report progress and report main about it
        sem_wait (sem_mut);
            *(arg->progress) += 1;
        sem_post (sem_mut);
        sem_post (arg->s);
    }
    sem_close (sem_mut);
    return NULL;
}

int main (int argc, char *argv[])
{
    //init options
    if (argc != 3) {
        printf ("usage: %s <target_val> <thread_amount>\n", argv[0]);
        return -1;
    }
    int thread_amount = 0;
    unsigned target = 0;
    sscanf (argv[2], "%d", &thread_amount);
    sscanf (argv[1], "%u", &target);
    if (thread_amount <= 0) {
        printf ("<thread_amount> must be an integer positive number\n");
        return -1;
    }
    if (target == 0) {
        printf ("<target> must be an integer positive number\n");
        return -1;
    }
    if (target < (unsigned) thread_amount) {
        printf ("useless to count on more than <target> threads, setting thread_amount to %d\n", target);
        thread_amount = target;
    }
    printf ("counting to: %u\non %d threads\n", target, thread_amount);

    //init arguments and variables
    thread_arg arg_common = {0}, arg_rest = {0};
    sem_t s;
    sem_init (&s, 0, 0); //o, 0: sem s with initial value 0, shared between threads, but not between processes, no errors available here
    unsigned progress = 0;
    arg_common.s = arg_rest.s = &s;
    arg_common.progress = arg_rest.progress = &progress;
    arg_common.subtarget = target / thread_amount;
    arg_rest.subtarget = arg_common.subtarget + 1;
    
    // init named semaphore
    const char sem_name[] = "/semaphore";
    sem_t *sem_mut = sem_open (sem_name, O_CREAT, 0600, 1); //sem with start walue 1 to enable threads start counting, rights are rw for user
    if (sem_mut == SEM_FAILED) {
        sem_destroy (&s);
        perror ("can't open semaphore");
    }
    arg_common.sem_name = sem_name;
    arg_rest.sem_name = sem_name;

    //init threads
    thread_arg *arg = &arg_common;
    pthread_t *thread_ids = (pthread_t *) calloc ((size_t) thread_amount, sizeof (pthread_t));
    for (int i = 0; i < thread_amount; i++) {
        if (i == thread_amount - (int) (target % thread_amount))
            arg = &arg_rest;
        if (errno = pthread_create (&thread_ids[i], NULL , thread, arg)) { // create new thread with "thread" routine and default attributes
            perror ("can't create new thread");
            free (thread_ids);
            sem_destroy (&s);
            sem_close (sem_mut);
            sem_unlink (sem_name);
            return -1;
        }
    }

    //report progress status
    unsigned temp = 0;
    sem_wait (sem_mut);
        unsigned prev = progress; // for correct display of 0
    sem_post (sem_mut);
    printf ("Progress is %u\n", prev);
    while (temp < target) {
        sem_wait (&s);
        sem_wait (sem_mut);
            temp = progress;
        sem_post (sem_mut);
        if (temp > prev)
            printf ("Progress is %u\n", temp);
        prev = temp;
    }
    for (int i = 0; i < thread_amount; i++) {
        if (errno = pthread_join (thread_ids[i], NULL)) {
            perror ("error while thread terminating");
            free (thread_ids);
            sem_destroy (&s);
            sem_close (sem_mut);
            sem_unlink (sem_name);
            return -1;
        }
    }
    sem_destroy (&s);
    sem_close (sem_mut);
    sem_unlink (sem_name);
    free (thread_ids);
    return 0;
}
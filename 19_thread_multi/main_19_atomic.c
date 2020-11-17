#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <semaphore.h>
#include <stdatomic.h>
#include <pthread.h>
#include <time.h>

extern int errno;

typedef struct {
    sem_t *s;
    atomic_uint *progress;
    unsigned long subtarget;
} thread_arg;

void *thread (void *arg_) {
    thread_arg *arg = (thread_arg *) arg_;
    for (unsigned int subprogress = 0; subprogress < arg->subtarget; subprogress++) { //subturget is const, no mutex/atomic needed
        // do magic of reptiles
        struct timespec st = {0, 20000000};
        nanosleep (&st, NULL);
        // report progress and report main about it
        atomic_fetch_add (arg->progress, 1);
        sem_post (arg->s);
    }
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
    atomic_uint progress;
    atomic_init (&progress, 0);
    arg_common.s = arg_rest.s = &s;
    arg_common.progress = arg_rest.progress = &progress;
    arg_common.subtarget = target / thread_amount;
    arg_rest.subtarget = arg_common.subtarget + 1;

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
            return -1;
        }
    }

    //report progress status
    unsigned temp = 0;
    unsigned prev = atomic_load (&progress); // for correct display of 0
    printf ("Progress is %u\n", prev);
    while (temp < target) {
        sem_wait (&s);
        temp = atomic_load (&progress);
        if (temp > prev)
            printf ("Progress is %u\n", temp);
        prev = temp;
    }
    for (int i = 0; i < thread_amount; i++) {
        if (errno = pthread_join (thread_ids[i], NULL)) {
            perror ("error while thread terminating");
            free (thread_ids);
            sem_destroy (&s);
            return -1;
        }
    }
    sem_destroy (&s);
    free (thread_ids);
    return 0;
}
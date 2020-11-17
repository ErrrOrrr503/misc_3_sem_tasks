#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <semaphore.h>
#include <pthread.h>
#include <time.h>

// NOTE: as progress is a pointer, (volatile) is, it seems, nesessary

extern int errno;

typedef struct {
    sem_t *s;
    pthread_mutex_t *mutex;
    volatile unsigned *progress;
    unsigned subtarget;
    unsigned start;
    long double result;
} thread_arg;

void *thread (void *arg_) {
    thread_arg *arg = (thread_arg *) arg_;
    long double subfact = 1;
    for (unsigned int subprogress = arg->start; subprogress < arg->subtarget + arg->start; subprogress++) { //subturget is const, no mutex/atomic needed
        // do magic of reptiles
        struct timespec st = {0, 20000000};
        nanosleep (&st, NULL);
        subfact *= (long double) (subprogress + 1);
        // report progress and report main about it
        pthread_mutex_lock (arg->mutex);
            *(arg->progress) += 1;
        pthread_mutex_unlock (arg->mutex);
        sem_post (arg->s);
    }
    pthread_mutex_lock (arg->mutex);
        arg->result = subfact;
    pthread_mutex_unlock (arg->mutex);
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
    sem_t s;
    sem_init (&s, 0, 0); //0, 0: sem s with initial value 0, shared between threads, but not between processes, no errors available here
    unsigned progress = 0;
    thread_arg *args = calloc (thread_amount, sizeof (thread_arg));
    
    // init mutex
    pthread_mutex_t mutex;
    if (errno = pthread_mutex_init (&mutex, NULL)) {// init mutex with default params
        perror ("can't init mutex");
        sem_destroy (&s);
        free (args);
        return -1;
    }

    //init threads and map job
    unsigned proceded = 0;
    pthread_t *thread_ids = (pthread_t *) calloc ((size_t) thread_amount, sizeof (pthread_t));
    for (int i = 0; i < thread_amount; i++) {
        //init arg
        args[i].s = &s;
        args[i].progress = &progress;
        args[i].subtarget = target / thread_amount;
        args[i].mutex = &mutex;
        args[i].start = proceded;
        if (i == thread_amount - (int) (target % thread_amount))
            args[i].subtarget++;
        proceded += args[i].subtarget;
        // create new thread with "thread" routine and default attributes
        if (errno = pthread_create (&thread_ids[i], NULL , thread, &args[i])) {
            perror ("can't create new thread");
            free (thread_ids);
            pthread_mutex_destroy (&mutex);
            sem_destroy (&s);
            free (args);
            return -1;
        }
    }

    //report progress status
    unsigned temp = 0;
    pthread_mutex_lock (&mutex);
        unsigned prev = progress; // for correct display of 0
    pthread_mutex_unlock (&mutex);
    printf ("Progress is %u\n", prev);
    while (temp < target) {
        sem_wait (&s);
        pthread_mutex_lock (&mutex);
            temp = progress;
        pthread_mutex_unlock (&mutex);
        if (temp > prev)
            printf ("Progress is %u\n", temp);
        prev = temp;
    }

    // reduce result and join threads
    long double result = 1;
    for (int i = 0; i < thread_amount; i++) {
        if (errno = pthread_join (thread_ids[i], NULL)) {
            perror ("error while thread terminating");
            free (thread_ids);
            pthread_mutex_destroy (&mutex);
            sem_destroy (&s);
            free (args);
            return -1;
        }
        pthread_mutex_lock (&mutex);
            result *= args[i].result;
        pthread_mutex_unlock (&mutex);
    }

    printf ("RESULT: %Lf\n", result);
    pthread_mutex_destroy (&mutex);
    sem_destroy (&s);
    free (thread_ids);
    free (args);
    return 0;
}
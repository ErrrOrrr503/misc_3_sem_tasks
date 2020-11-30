#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <semaphore.h>
#include <stdatomic.h>
#include <pthread.h>
#include <time.h>

typedef struct {
    sem_t s;
    atomic_uint progress;
} thread_arg;

void *thread (void *arg_) {
    thread_arg *arg = (thread_arg *) arg_;
    while (arg->progress < 100) {
        struct timespec st = {0, 200000};
        nanosleep (&st, NULL);
        atomic_fetch_add (&arg->progress, 1);
        sem_post (&arg->s);
    }
    return NULL;
}

int main ()
{
    int errno; //pthread funcs are not alowed to set errno
    thread_arg arg = {0};
    sem_init (&arg.s, 0, 0); //o, 0: sem s with initial value 0, shared between threads, but not between processes, no errors available here
    atomic_init (&arg.progress, 0);

    pthread_t thread_id = 0;
    if (errno = pthread_create (&thread_id, NULL , thread, &arg)) { // create new thread with "thread" routine and default attributes
        perror ("can't create new thread");
        return -1;
    }

    unsigned temp = 0;
    temp = atomic_load (&arg.progress); //correct display of 0
    printf ("Progress is %u%\%\n", temp);
    while (temp < 100) {
        sem_wait (&arg.s);
        temp = atomic_load (&arg.progress);
        printf ("Progress is %u%\%\n", temp);
    }
    if (errno = pthread_join (thread_id, NULL)) {
        perror ("error while thread terminating");
        return -1;
    }
    return 0;
}
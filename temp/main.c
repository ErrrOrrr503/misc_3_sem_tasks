#include <sys/mman.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <errno.h>
#include <string.h>
#include <sys/wait.h>
#include <semaphore.h>

extern int errno;

int main ()
{
    //ro rw or wo, do ot use other options
    //create mapping for one page (min size of mapping)
    void *p = mmap (NULL,
        (size_t) sysconf (_SC_PAGE_SIZE),
        PROT_READ | PROT_WRITE,
        MAP_ANONYMOUS | MAP_SHARED,
        -1,
        0
    );
    if (p == MAP_FAILED) {
        perror ("mmap_failed");
        return -1;
    }
    printf ("%p\n", p);
    struct {
        sem_t s;
        int flag;
        double a, b;
    } *buf = p;
    sem_init (buf->s, 1, 1); // shared!
    buf->a = 3485930;
    buf->b = 3749;

    pid_t child_id = fork ();
    if (child_id == -1) {
        perror (NULL);
        return -1;
    }
    if (!child_id) {
        while (1) {
            sem_wait (&buf->s);
            buf->a++;
            buf->b--;
            if (!buf->flag)
                break;
            sem_post (&buf->s);
        }
        return 0;
    }
    for (int i = 0; i < )
    printf ("%lf %lf\n", buf->a, buf->b);
    waitpid (-1, NULL, 0);
    printf ("%lf %lf\n", buf->a, buf->b);
    return 0;
}
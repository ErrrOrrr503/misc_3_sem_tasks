//#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
//#include <pwd.h>
#include <linux/limits.h>
#include <dlfcn.h>
#include <sys/wait.h>

const char dlname[] = "./tmp.so";
const char funcname[] = "equasion";

int compiler_launch (int pipe_fds[2]);
int send_code (int pipe_fds[2], const char *func);
int compile_check ();
int extract_equasion (long double (**equasion) (long double), void **lib);
long double integrate_print (long double (*equasion) (long double), long double a, long double b, long double step);
int check_args (int argc, char *argv[], long double *a, long double *b, long double *step);

int main (int argc, char *argv[])
{
    long double a = 0, b = 0, step = 0;
    if (check_args (argc, argv, &a, &b, &step))
        return -1;
    int pipe_fds[2] = {-1};
    if (pipe (pipe_fds) == -1) {
        perror ("can't init pipe");
        return -1;
    }
    pid_t child_pid = fork ();
    if (child_pid == -1) {
        perror ("can't create child process");
        return -1;
    }
    if (!child_pid) {
        //child code
        return compiler_launch (pipe_fds);
    }
    //send src and close everything that can be closed
    if (send_code (pipe_fds, argv[1])) 
        return -1;
    //check if compiler finished correctly
    if (compile_check ())
        return -1;
    
    //open lib and load function
    long double (*equasion) (long double);
    void *lib = NULL;
    if (extract_equasion (&equasion, &lib))
        return -1;
    long double res = integrate_print (equasion, a, b, step);
    printf ("res: %Lg\n", res);
    dlclose (lib);
    return 0;
}

int compiler_launch (int pipe_fds[2])
{
    close (pipe_fds[1]);
    dup2 (pipe_fds[0], STDIN_FILENO);
    close (pipe_fds[0]);
    execlp (
        "gcc",
        "gcc", "-Wall", "-Wextra", "-Werror", // common, -Werror needet to avoid junk functions
        "-fPIC", "-fPIE", "-shared", // shared lib
        "-lm", // link with math library
        "-o", dlname, "-xc", "-", // read stdin and save compiled lib to .so
        NULL  //end of args
    );
    perror ("compiler command failed");
    return -1;
}

int send_code (int pipe_fds[2], const char *func)
{
    close (pipe_fds[0]);
    FILE *src = fdopen (pipe_fds[1], "w");
    if (src == NULL) {
        perror ("something wrong with pipe, can't open for writing");
        close (pipe_fds[1]);
        return -1;
    }
    fprintf (
        src, 
        "\
        #include <math.h> \n\
        long double %s (long double x) \n\
        { \n\
            return %s; \n\
        } \
        ",
        funcname,
        func
    );
    fclose (src);
    close (pipe_fds[1]);
    return 0;
}

int compile_check ()
{
    int wstatus = 0;
    if (waitpid (-1, &wstatus, 0) == -1) { //wait for termination of compiler child process
			perror ("somewhat error on waiting child to exit");
			return -1;
	}
	if (!WIFEXITED (wstatus) || WEXITSTATUS (wstatus)) {
        printf ("compiler failed with code: %d\n", WEXITSTATUS (wstatus));
        return -1;
    }
    return 0;
}

int extract_equasion (long double (**equasion) (long double), void **lib)
{
    *lib = dlopen (dlname, RTLD_LAZY);
    if (*lib == NULL) {
        printf ("@dlopen:%s\n", dlerror ());
        return -1;
    }
    *equasion = (long double (*) (long double)) dlsym (*lib, funcname);
    if (dlerror () != NULL) {
        printf ("@dlsym:%s\n", dlerror ());
        dlclose (*lib);
        return -1;
    }
    return 0;
}

long double integrate_print (long double (*equasion) (long double), long double a, long double b, long double step)
{
    long double res = 0;
    for ( ; a < b; a += step) {
        res += equasion (a) * step;
    }
    return res;
}

int check_args (int argc, char *argv[], long double *a, long double *b, long double *step)
{
    if (argc != 5) {
        printf ("usage: %s \"[c-funcltion](x)>\" <start> <end> <step>\nfor example: integrate \"sin(x)\" 0 1 0.001\n", argv[0]);
        return -1;
    }
    if (sscanf (argv[2], "%Lg", a) != 1 || sscanf (argv[3], "%Lg", b) != 1 || sscanf (argv[4], "%Lg", step) != 1) {
        printf ("incorrect input, not numbers?\n");
        return -1;
    }
    if (*a > *b) {
        printf ("incorrect integration limits: a must be less than b\n");
        return -1;
    }
    if (*step < 0.000001L) {
        printf ("incorrect step: must be at least 1e-6\n");
        return -1;
    }
    return 0;
}
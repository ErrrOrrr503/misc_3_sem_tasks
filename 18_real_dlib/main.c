//#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <linux/limits.h>
#include <dlfcn.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <inttypes.h>

//using getopt as it is in posix

bool g_flag_verbose = 0;

const char dlname[] = "./tmp.so";
const char funcname[] = "equation";

typedef long double (* equation) (long double, long double, uint64_t);

static int compiler_launch (int pipe_fds[2]);
static int send_code (int pipe_fds[2], const char *func);
static int compile_check ();
static int extract_equation (equation *equ, void **lib);
static int check_args (int argc, char * const argv[], const char ** pfunc, long double *begin, long double *end, uint64_t *prec);

int main (int argc, char *argv[])
{
    long double begin = 0, end = 0;
    uint64_t iterations = 0;
    const char * func;
    if (check_args (argc, argv, &func, &begin, &end, &iterations))
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
    close (pipe_fds[0]);
    //send src and close everything that can be closed
    if (send_code (pipe_fds, func)) {
        close (pipe_fds[1]);
        return -1;
    }
    close (pipe_fds[1]);
    //check if compiler finished correctly
    if (compile_check ())
        return -1;
    //open lib and load function
    equation equ;
    void *lib = NULL;
    if (extract_equation (&equ, &lib))
        return -1;
    long double res = equ (begin, end, iterations);
    printf ("res: %Lg\n", res);
    dlclose (lib);
    return 0;
}

static int compiler_launch (int pipe_fds[2])
{
    close (pipe_fds[1]);
    dup2 (pipe_fds[0], STDIN_FILENO);
    close (pipe_fds[0]);
    execlp (
        "gcc",
        "gcc", "-Wall", "-Wextra", "-Werror", // common, -Werror needed to avoid junk functions
        "-fPIC", "-fPIE", "-shared", // shared lib
        "-lm", // link with math library
        "-o", dlname, "-xc", "-", // read stdin and save compiled lib to .so
        NULL  //end of args
    );
    perror ("compiler command failed");
    return -1;
}

static int send_code (int pipe_fds[2], const char *func)
{
    FILE *src = fdopen (pipe_fds[1], "w");
    if (src == NULL) {
        perror ("something wrong with pipe, can't open for writing");
        return -1;
    }
    const char *code = \
        "\
        #include <math.h> \n\
        #include <inttypes.h> \n\
        long double %s (long double begin, long double end, uint64_t iterations) \n\
        { \n\
            long double res = 0; \n\
            long double x = begin; \n\
            for (uint64_t i = 0; i <= iterations; i++) { \n\
                x = begin +  i * (end - begin) /  iterations; \n\
                res += %s * (end - begin) /  iterations; \n\
            } \n\
            return res; \n\
        } \n\
        \n";
    fprintf (src, code, funcname, func);
    if (g_flag_verbose) {
        printf ("generated code:\n");
        printf (code, funcname, func);
    }
    fclose (src);
    return 0;
}

static int compile_check ()
{
    int wstatus = 0;
    if (waitpid (-1, &wstatus, 0) == -1) { //wait for termination of compiler child process
			perror ("somewhat error on waiting child to exit");
			return -1;
	}
	if (WIFEXITED (wstatus)) {
        if (WEXITSTATUS (wstatus)) {
            printf ("compiler failed with code: %d\n", WEXITSTATUS (wstatus));
            return -1;
        }
    }
    else {
        //WIFSIGNALED
        printf ("compiler terminated by signal: %d : %s\n", WTERMSIG (wstatus), strsignal (WTERMSIG (wstatus)));
        return -1;
    }
    return 0;
}

static int extract_equation (equation *equ, void **lib)
{
    *lib = dlopen (dlname, RTLD_LAZY);
    if (*lib == NULL) {
        printf ("@dlopen:%s\n", dlerror ());
        return -1;
    }
    *equ = (equation) dlsym (*lib, funcname);
    if (dlerror () != NULL) {
        printf ("@dlsym:%s\n", dlerror ());
        dlclose (*lib);
        return -1;
    }
    return 0;
}

static int check_args (int argc, char * const argv[], const char ** pfunc, long double *begin, long double *end, uint64_t *iterations)
{
    long double prec = 0;
    bool flag_stepping = 0;
    int ret = 0;
    uint8_t uninit_mask = 0b1111;
    while ((ret = getopt (argc, argv, "vf:b:e:p:s:")) != -1) {
        switch (ret) {
            case 'f':
                *pfunc = optarg;
                uninit_mask &= 0b0111;
                break;
            case 'b':
                if (sscanf (optarg, "%Lg", begin) != 1) {
                    printf ("incorrect input: begin in not a number\n");
                    return -1;
                }
                uninit_mask &= 0b1011;
                break;
            case 'e':
                if (sscanf (optarg, "%Lg", end) != 1) {
                    printf ("incorrect input: end in not a number\n");
                    return -1;
                }
                uninit_mask &= 0b1101;
                break;
            case 's':
                flag_stepping = 1;
            case 'p':
                if (sscanf (optarg, "%Lg", &prec) != 1) {
                    printf ("incorrect input: precision in not a number\n");
                    return -1;
                }
                uninit_mask &= 0b1110;
                break;
            case 'v':
                g_flag_verbose = 1;
                break;
            default:
                printf ("usage: %s -f \"[c-function](x)>\" -b <begin> -e <end> [-p <precision>] or [-s <step>]\n", argv[0]);
                return -1;
        }
    }
    if (uninit_mask) {
        printf ("usage: %s -f \"[c-function](x)>\" -b <begin> -e <end> [-p <precision>] or [-s <step>]\n", argv[0]);
        return -1;
    }
    if (*end < *begin) {
        printf ("incorrect integration limits: begin: %Lg must be less than end: %Lg\n", *begin, *end);
        return -1;
    }
    if (flag_stepping)
        prec = (*end - *begin) / prec; // now prec is somewhat num of iterations
    if (prec < 1 || prec > UINT64_MAX - 1) {
        printf ("incorrect precision: %Lg\n", prec);
        return -1;
    }
    *iterations = (uint64_t) prec;
    if (g_flag_verbose)
        printf ("func: %s\nbegin: %Lg, end: %Lg\niterations: %" PRIu64 "\n", *pfunc, *begin, *end, *iterations);
    return 0;
}
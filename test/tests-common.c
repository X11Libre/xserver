#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "tests-common.h"

void
run_test_in_child(const testfunc_t* (*suite)(void), const char *funcname)
{
    int cpid;
    int csts;
    int exit_code = -1;
    const testfunc_t *func = suite();

    printf("\n---------------------\n%s...\n", funcname);

    while (*func)
    {
        fflush(stdout);
        cpid = fork();
        if (cpid) {
            waitpid(cpid, &csts, 0);
            if (WIFEXITED(csts))
                exit_code = WEXITSTATUS(csts);
            else
                exit_code = WIFSIGNALED(csts) ? 128 + WTERMSIG(csts) : 1;

            if (exit_code != 0) {
                printf(" FAIL\n");
                exit(exit_code);
            }
        } else {
            testfunc_t f = *func;
            f();
            exit(0);
        }
        func++;
    }
    printf(" Pass\n");
}

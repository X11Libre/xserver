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
        cpid = fork();
        if (cpid) {
            waitpid(cpid, &csts, 0);
            if (!WIFEXITED(csts))
                goto child_failed;
            exit_code = WEXITSTATUS(csts);
            if (exit_code != 0) {
    child_failed:
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

#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>

#include "tests-common.h"

void
run_test_in_child(const testfunc_t* (*suite)(void), const char *funcname)
{
    int cpid;
    int csts;
    int exit_code = -1;
    const testfunc_t *func = suite();

    char *xlibre_testenv = getenv("XLIBRE_TEST");

    if (xlibre_testenv != NULL) {
        bool run_this_test = false;
        char *tok_r;
        char *tok;

        xlibre_testenv = strdup(xlibre_testenv);
        tok = strtok_r(xlibre_testenv, ",", &tok_r);

        while (tok != NULL) {
            if (strcmp(tok, funcname) == 0) {
                run_this_test = true;
                break;
            }
            tok = strtok_r(NULL, ",", &tok_r);
        }

        free(xlibre_testenv);

        if (!run_this_test) {
            return; /* not selected via XLIBRE_TEST, skip it */
        }
    }

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

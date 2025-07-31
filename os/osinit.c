/***********************************************************

Copyright 1987, 1998  The Open Group

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation.

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from The Open Group.

Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the name of Digital not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.

DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

******************************************************************/

#include <dix-config.h>

#include <errno.h>
#include <stdio.h>
#include <signal.h>
#include <X11/X.h>
#include <X11/Xos.h>
#ifdef HAVE_DLFCN_H
#include <dlfcn.h>
#endif
#if defined(HAVE_BACKTRACE) && defined(HAVE_EXECINFO_H)
#include <execinfo.h>
#endif

#include "dix/dix_priv.h"
#include "os/busfault.h"
#include "os/log_priv.h"
#include "os/osdep.h"
#include "os/serverlock.h"

#include "misc.h"
#include "os.h"
#include "opaque.h"
#include "dixstruct.h"
#include "dixstruct_priv.h"

#if !defined(WIN32)
#include <sys/resource.h>
#endif

/* The actual user defined max number of clients */
int LimitClients = DIX_LIMITCLIENTS;

static OsSigWrapperPtr OsSigWrapper = NULL;

OsSigWrapperPtr
OsRegisterSigWrapper(OsSigWrapperPtr newSigWrapper)
{
    OsSigWrapperPtr oldSigWrapper = OsSigWrapper;

    OsSigWrapper = newSigWrapper;

    return oldSigWrapper;
}

/*
 * OsSigHandler --
 *    Catch unexpected signals and exit or continue cleanly.
 */
#if !defined(WIN32)
static void
#ifdef SA_SIGINFO
OsSigHandler(int signo, siginfo_t * sip, void *unused)
#else
OsSigHandler(int signo)
#endif
{
#ifdef RTLD_DI_SETSIGNAL
# define SIGNAL_FOR_RTLD_ERROR SIGQUIT
    if (signo == SIGNAL_FOR_RTLD_ERROR) {
        const char *dlerr = dlerror();

        if (dlerr)
            LogMessageVerb(X_ERROR, 1, "Dynamic loader error: %s\n", dlerr);
    }
#endif                          /* RTLD_DI_SETSIGNAL */

    if (OsSigWrapper != NULL) {
        if (OsSigWrapper(signo) == 0) {
            /* ddx handled signal and wants us to continue */
            return;
        }
    }

    /* log, cleanup, and abort */
    xorg_backtrace();

#ifdef SA_SIGINFO
    if (sip->si_code == SI_USER) {
        ErrorF("Received signal %u sent by process %u, uid %u\n", signo,
               sip->si_pid, sip->si_uid);
    }
    else {
        switch (signo) {
        case SIGSEGV:
        case SIGBUS:
        case SIGILL:
        case SIGFPE:
            ErrorF("%s at address %p\n", strsignal(signo), sip->si_addr);
        }
    }
#endif

    if (signo != SIGQUIT)
        CoreDump = TRUE;

    FatalError("Caught signal %d (%s). Server aborting\n",
               signo, strsignal(signo));
}
#endif /* !WIN32 */

void
OsInit(void)
{
    static Bool been_here = FALSE;

    if (!been_here) {
#if !defined(WIN32)
        struct sigaction act, oact;
        int i;

        int siglist[] = { SIGSEGV, SIGQUIT, SIGILL, SIGFPE, SIGBUS,
            SIGABRT,
            SIGSYS,
            SIGXCPU,
            SIGXFSZ,
#ifdef SIGEMT
            SIGEMT,
#endif
            0 /* must be last */
        };
        sigemptyset(&act.sa_mask);
#ifdef SA_SIGINFO
        act.sa_sigaction = OsSigHandler;
        act.sa_flags = SA_SIGINFO;
#else
        act.sa_handler = OsSigHandler;
        act.sa_flags = 0;
#endif
        for (i = 0; siglist[i] != 0; i++) {
            if (sigaction(siglist[i], &act, &oact)) {
                ErrorF("failed to install signal handler for signal %d: %s\n",
                       siglist[i], strerror(errno));
            }
        }
#endif /* !WIN32 */
        busfault_init();
        server_poll = ospoll_create();
        if (!server_poll)
            FatalError("failed to allocate poll structure");

#if defined(HAVE_BACKTRACE) && defined(HAVE_EXECINFO_H)
        /*
         * initialize the backtracer, since the ctor calls dlopen(), which
         * calls malloc(), which isn't signal-safe.
         */
        do {
            void *array;

            backtrace(&array, 1);
        } while (0);
#endif

#ifdef RTLD_DI_SETSIGNAL
        /* Tell runtime linker to send a signal we can catch instead of SIGKILL
         * for failures to load libraries/modules at runtime so we can clean up
         * after ourselves.
         */
        {
            int failure_signal = SIGNAL_FOR_RTLD_ERROR;

            dlinfo(RTLD_SELF, RTLD_DI_SETSIGNAL, &failure_signal);
        }
#endif

#if !defined(WIN32)
        if (getpgrp() == 0)
            setpgid(0, 0);
#endif
        LockServer();
        been_here = TRUE;
    }
    TimerInit();
    OsVendorInit();
    OsResetSignals();
    /*
     * No log file by default.  OsVendorInit() should call LogInit() with the
     * log file name if logging to a file is desired.
     */
    LogInit(NULL, NULL);
    SmartScheduleInit();
}

void
OsCleanup(Bool terminating)
{
    if (terminating) {
        UnlockServer();
    }
}

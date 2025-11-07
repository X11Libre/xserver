/*

Copyright 1987, 1998  The Open Group

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation.

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall
not be used in advertising or otherwise to promote the sale, use or
other dealings in this Software without prior written authorization
from The Open Group.

Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts,
Copyright 1994 Quarterdeck Office Systems.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the names of Digital and
Quarterdeck not be used in advertising or publicity pertaining to
distribution of the software without specific, written prior
permission.

DIGITAL AND QUARTERDECK DISCLAIM ALL WARRANTIES WITH REGARD TO THIS
SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
FITNESS, IN NO EVENT SHALL DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT
OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE
OR PERFORMANCE OF THIS SOFTWARE.

*/

#include <dix-config.h>

#if defined(WIN32)
#include <X11/Xwinsock.h>
#endif
#include <X11/Xos.h>
#include <stdio.h>
#include <time.h>
#if !defined(WIN32) || !defined(__MINGW32__)
#include <sys/time.h>
#include <sys/resource.h>
#endif
#include <X11/X.h>

#include "os/mathx_priv.h"
#include "os/Xtrans.h"

#include <libgen.h>

#include "input.h"
#include "dixfont.h"
#include <X11/fonts/libxfont2.h>
#include "osdep.h"

#ifdef XDMCP
#include "xdmcp.h"
#endif

#include "extension.h"
#include <signal.h>
#ifndef WIN32
#include <sys/wait.h>
#endif
#if !defined(WIN32)
#include <sys/resource.h>
#endif
#include <sys/stat.h>
#include <ctype.h>              /* for isspace */
#include <stdarg.h>
#include <stdlib.h>             /* for calloc() */

#if defined(TCPCONN)
#ifndef WIN32
#include <netdb.h>
#endif
#endif

#include "dix/dix_priv.h"
#include "dix/extension_priv.h"
#include "dix/input_priv.h"
#include "dix/settings_priv.h"
#include "dix/screensaver_priv.h"
#include "miext/extinit_priv.h"
#include "os/audit_priv.h"
#include "os/auth.h"
#include "os/bug_priv.h"
#include "os/cmdline.h"
#include "os/client_priv.h"
#include "os/ddx_priv.h"
#include "os/log_priv.h"
#include "os/osdep.h"
#include "os/serverlock.h"
#include "os/xhostname.h"
#include "present/present_priv.h"
#include "Xext/xf86bigfontsrv.h" /* XF86BigfontCleanup() */
#include "xkb/xkbsrv_priv.h"

#include "dixstruct.h"
#include "picture.h"
#include "miinitext.h"
#include "dixstruct_priv.h"
#include "dpmsproc.h"

#define X_INCLUDE_NETDB_H
#include <X11/Xos_r.h>

#include <errno.h>

Bool CoreDump;

Bool enableIndirectGLX = FALSE;

#ifdef XINERAMA
Bool PanoramiXExtensionDisabledHack = FALSE;
#endif /* XINERAMA */

sig_atomic_t inSignalContext = FALSE;

#ifdef MONOTONIC_CLOCK
static clockid_t clockid;
#endif

OsSigHandlerPtr
OsSignal(int sig, OsSigHandlerPtr handler)
{
#if defined(WIN32)
    return signal(sig, handler);
#else
    struct sigaction act, oact;

    sigemptyset(&act.sa_mask);
    if (handler != SIG_IGN)
        sigaddset(&act.sa_mask, sig);
    act.sa_flags = 0;
    act.sa_handler = handler;
    if (sigaction(sig, &act, &oact))
        perror("sigaction");
    return oact.sa_handler;
#endif
}

/* Force connections to close and then exit on SIGTERM, SIGINT */

void
GiveUp(int sig)
{
    int olderrno = errno;

    dispatchException |= DE_TERMINATE;
    isItTimeToYield = TRUE;
    errno = olderrno;
}

#ifdef MONOTONIC_CLOCK
void
ForceClockId(clockid_t forced_clockid)
{
    struct timespec tp;

    BUG_RETURN (clockid);

    clockid = forced_clockid;

    if (clock_gettime(clockid, &tp) != 0) {
        FatalError("Forced clock id failed to retrieve current time: %s\n",
                   strerror(errno));
        return;
    }
}
#endif

#if (defined WIN32 && defined __MINGW32__)
CARD32
GetTimeInMillis(void)
{
    return GetTickCount();
}
CARD64
GetTimeInMicros(void)
{
    return (CARD64) GetTickCount() * 1000;
}
#else
CARD32
GetTimeInMillis(void)
{
    struct timeval tv;

#ifdef MONOTONIC_CLOCK
    struct timespec tp;

    if (!clockid) {
#ifdef CLOCK_MONOTONIC_COARSE
        if (clock_getres(CLOCK_MONOTONIC_COARSE, &tp) == 0 &&
            (tp.tv_nsec / 1000) <= 1000 &&
            clock_gettime(CLOCK_MONOTONIC_COARSE, &tp) == 0)
            clockid = CLOCK_MONOTONIC_COARSE;
        else
#endif
        if (clock_gettime(CLOCK_MONOTONIC, &tp) == 0)
            clockid = CLOCK_MONOTONIC;
        else
            clockid = ~0L;
    }
    if (clockid != ~0L && clock_gettime(clockid, &tp) == 0)
        return (tp.tv_sec * 1000) + (tp.tv_nsec / 1000000L);
#endif

    X_GETTIMEOFDAY(&tv);
    return (tv.tv_sec * 1000) + (tv.tv_usec / 1000);
}

CARD64
GetTimeInMicros(void)
{
    struct timeval tv;
#ifdef MONOTONIC_CLOCK
    struct timespec tp;
    static clockid_t uclockid;

    if (!uclockid) {
        if (clock_gettime(CLOCK_MONOTONIC, &tp) == 0)
            uclockid = CLOCK_MONOTONIC;
        else
            uclockid = ~0L;
    }
    if (uclockid != ~0L && clock_gettime(uclockid, &tp) == 0)
        return (CARD64) tp.tv_sec * (CARD64)1000000 + tp.tv_nsec / 1000;
#endif

    X_GETTIMEOFDAY(&tv);
    return (CARD64) tv.tv_sec * (CARD64)1000000 + (CARD64) tv.tv_usec;
}
#endif

void
UseMsg(void)
{
    ErrorF("use: X [:<display>] [option]\n");
    ErrorF("-a #                   default pointer acceleration (factor)\n");
    ErrorF("-ac                    disable access control restrictions\n");
    ErrorF("-audit int             set audit trail level\n");
    ErrorF("-auth file             select authorization file\n");
    ErrorF("-br                    create root window with black background\n");
    ErrorF("+bs                    enable any backing store support\n");
    ErrorF("-bs                    disable any backing store support\n");
    ErrorF("+byteswappedclients    Allow clients with endianess different to that of the server\n");
    ErrorF("-byteswappedclients    Prohibit clients with endianess different to that of the server\n");
    ErrorF("-c                     turns off key-click\n");
    ErrorF("c #                    key-click volume (0-100)\n");
    ErrorF("-cc int                default color visual class\n");
    ErrorF("-nocursor              disable the cursor\n");
    ErrorF("-core                  generate core dump on fatal error\n");
    ErrorF("-displayfd fd          file descriptor to write display number to when ready to connect\n");
    ErrorF("-dpi int               screen resolution in dots per inch\n");
#ifdef DPMSExtension
    ErrorF("-dpms                  disables VESA DPMS monitor control\n");
#endif
    ErrorF
        ("-deferglyphs [none|all|16] defer loading of [no|all|16-bit] glyphs\n");
    ErrorF("-f #                   bell base (0-100)\n");
    ErrorF("-fakescreenfps #       fake screen default fps (1-600)\n");
    ErrorF("-fp string             default font path\n");
    ErrorF("-help                  prints message with these options\n");
    ErrorF("+iglx                  Allow creating indirect GLX contexts\n");
    ErrorF("-iglx                  Prohibit creating indirect GLX contexts (default)\n");
    ErrorF("-I                     ignore all remaining arguments\n");
#ifdef CONFIG_NAMESPACE
    ErrorF("-namespace <conf>      Enable NAMESPACE extension with given config file\n");
#endif /* CONFIG_NAMESPACE */
    LockServerUseMsg();
    ErrorF("-maxclients n          set maximum number of clients (power of two)\n");
    ErrorF("-nolisten string       don't listen on protocol\n");
    ErrorF("-listen string         listen on protocol\n");
    ErrorF("-background [none]     create root window with no background\n");
    ErrorF("-p #                   screen-saver pattern duration (minutes)\n");
    ErrorF("-pn                    accept failure to listen on all ports\n");
    ErrorF("-nopn                  reject failure to listen on all ports\n");
    ErrorF("-r                     turns off auto-repeat\n");
    ErrorF("r                      turns on auto-repeat \n");
    ErrorF("-render [default|mono|gray|color] set render color alloc policy\n");
    ErrorF("-retro                 start with classic stipple and cursor\n");
    ErrorF("-s #                   screen-saver timeout (minutes)\n");
    ErrorF("-seat string           seat to run on\n");
    ErrorF("-t #                   default pointer threshold (pixels/t)\n");
    ErrorF("-terminate [delay]     terminate at server reset (optional delay in sec)\n");
    ErrorF("-tst                   disable testing extensions\n");
    ErrorF("ttyxx                  server started from init on /dev/ttyxx\n");
    ErrorF("v                      video blanking for screen-saver\n");
    ErrorF("-v                     screen-saver without video blanking\n");
    ErrorF("-wr                    create root window with white background\n");
    ErrorF("-maxbigreqsize         set maximal bigrequest size \n");
#ifdef XINERAMA
    ErrorF("+xinerama              Enable XINERAMA extension\n");
    ErrorF("-xinerama              Disable XINERAMA extension\n");
#endif /* XINERAMA */
    ErrorF("-dumbSched             Disable smart scheduling and threaded input, enable old behavior\n");
    ErrorF("-schedInterval int     Set scheduler interval in msec\n");
    ErrorF("+extension name        Enable extension\n");
    ErrorF("-extension name        Disable extension\n");
    ListStaticExtensions();
#ifdef XDMCP
    XdmcpUseMsg();
#endif
    XkbUseMsg();
    ddxUseMsg();
}

/*  This function performs a rudimentary sanity check
 *  on the display name passed in on the command-line,
 *  since this string is used to generate filenames.
 *  It is especially important that the display name
 *  not contain a "/" and not start with a "-".
 *                                            --kvajk
 */
static int
VerifyDisplayName(const char *d)
{
    int i;
    int period_found = FALSE;
    int after_period = 0;

    if (d == (char *) 0)
        return 0;               /*  null  */
    if (*d == '\0')
        return 0;               /*  empty  */
    if (*d == '-')
        return 0;               /*  could be confused for an option  */
    if (*d == '.')
        return 0;               /*  must not equal "." or ".."  */
    if (strchr(d, '/') != (char *) 0)
        return 0;               /*  very important!!!  */

    /* Since we run atoi() on the display later, only allow
       for digits, or exception of :0.0 and similar (two decimal points max)
       */
    for (i = 0; i < strlen(d); i++) {
        if (!isdigit((unsigned char)d[i])) {
            if (d[i] != '.' || period_found)
                return 0;
            period_found = TRUE;
        } else if (period_found)
            after_period++;

        if (after_period > 2)
            return 0;
    }

    /* don't allow for :0. */
    if (period_found && after_period == 0)
        return 0;

    if (atol(d) > INT_MAX)
        return 0;

    return 1;
}

static const char *defaultNoListenList[] = {
#ifndef LISTEN_TCP
    "tcp",
#endif
#ifndef LISTEN_UNIX
    "unix",
#endif
#ifndef LISTEN_LOCAL
    "local",
#endif
    NULL
};

/*
 * This function parses the command line. Handles device-independent fields
 * and allows ddx to handle additional fields.  It is not allowed to modify
 * argc or any of the strings pointed to by argv.
 */
void
ProcessCommandLine(int argc, char *argv[])
{
    int i, skip;

    defaultKeyboardControl.autoRepeat = TRUE;

#ifdef NO_PART_NET
    PartialNetwork = FALSE;
#else
    PartialNetwork = TRUE;
#endif

    for (i = 0; defaultNoListenList[i] != NULL; i++) {
        if (_XSERVTransNoListen(defaultNoListenList[i]))
                    ErrorF("Failed to disable listen for %s transport",
                           defaultNoListenList[i]);
    }
    dixSettingSeatId = getenv("XDG_SEAT");

#ifdef CONFIG_SYSLOG
    xorgSyslogIdent = getenv("SYSLOG_IDENT");
    if (!xorgSyslogIdent)
        xorgSyslogIdent = strdup(basename(argv[0]));
#endif

    for (i = 1; i < argc; i++) {
        /* call ddx first, so it can peek/override if it wants */
        if ((skip = ddxProcessArgument(argc, argv, i))) {
            i += (skip - 1);
        }
        else if (argv[i][0] == ':') {
            /* initialize display */
            display = argv[i];
            explicit_display = TRUE;
            display++;
            if (!VerifyDisplayName(display)) {
                ErrorF("Bad display name: %s\n", display);
                UseMsg();
                FatalError("Bad display name, exiting: %s\n", display);
            }
        }
        else if (strcmp(argv[i], "-a") == 0) {
            if (++i < argc)
                defaultPointerControl.num = atoi(argv[i]);
            else
                UseMsg();
        }
        else if (strcmp(argv[i], "-ac") == 0) {
            defeatAccessControl = TRUE;
        }
        else if (strcmp(argv[i], "-audit") == 0) {
            if (++i < argc)
                auditTrailLevel = atoi(argv[i]);
            else
                UseMsg();
        }
        else if (strcmp(argv[i], "-auth") == 0) {
            if (++i < argc)
                InitAuthorization(argv[i]);
            else
                UseMsg();
        }
        else if (strcmp(argv[i], "-byteswappedclients") == 0) {
            dixSettingAllowByteSwappedClients = FALSE;
        } else if (strcmp(argv[i], "+byteswappedclients") == 0) {
            dixSettingAllowByteSwappedClients = TRUE;
        }
        else if (strcmp(argv[i], "-br") == 0);  /* default */
        else if (strcmp(argv[i], "+bs") == 0)
            enableBackingStore = TRUE;
        else if (strcmp(argv[i], "-bs") == 0)
            disableBackingStore = TRUE;
        else if (strcmp(argv[i], "c") == 0) {
            if (++i < argc)
                defaultKeyboardControl.click = atoi(argv[i]);
            else
                UseMsg();
        }
        else if (strcmp(argv[i], "-c") == 0) {
            defaultKeyboardControl.click = 0;
        }
        else if (strcmp(argv[i], "-cc") == 0) {
            if (++i < argc)
                defaultColorVisualClass = atoi(argv[i]);
            else
                UseMsg();
        }
        else if (strcmp(argv[i], "-core") == 0) {
#if !defined(WIN32) || !defined(__MINGW32__)
            struct rlimit core_limit;

            getrlimit(RLIMIT_CORE, &core_limit);
            core_limit.rlim_cur = core_limit.rlim_max;
            setrlimit(RLIMIT_CORE, &core_limit);
#endif
            CoreDump = TRUE;
        }
        else if (strcmp(argv[i], "-nocursor") == 0) {
            EnableCursor = FALSE;
        }
        else if (strcmp(argv[i], "-dpi") == 0) {
            if (++i < argc)
                monitorResolution = atoi(argv[i]);
            else
                UseMsg();
        }
        else if (strcmp(argv[i], "-displayfd") == 0) {
            if (++i < argc) {
                displayfd = atoi(argv[i]);
                DisableServerLock();
            }
            else
                UseMsg();
        }
#ifdef DPMSExtension
        else if (strcmp(argv[i], "dpms") == 0)
            /* ignored for compatibility */ ;
        else if (strcmp(argv[i], "-dpms") == 0)
            DPMSDisabledSwitch = TRUE;
#endif
        else if (strcmp(argv[i], "-deferglyphs") == 0) {
            if (++i >= argc || !xfont2_parse_glyph_caching_mode(argv[i]))
                UseMsg();
        }
        else if (strcmp(argv[i], "-f") == 0) {
            if (++i < argc)
                defaultKeyboardControl.bell = atoi(argv[i]);
            else
                UseMsg();
        }
        else if (strcmp(argv[i], "-fakescreenfps") == 0) {
            if (++i < argc) {
                FakeScreenFps = (uint32_t) atoi(argv[i]);
                if (FakeScreenFps < 1 || FakeScreenFps > 600)
                    FatalError("fakescreenfps must be an integer in [1;600] range\n");
            }
            else
                UseMsg();
        }
        else if (strcmp(argv[i], "-fp") == 0) {
            if (++i < argc) {
                defaultFontPath = argv[i];
            }
            else
                UseMsg();
        }
        else if (strcmp(argv[i], "-help") == 0) {
            UseMsg();
            exit(0);
        }
        else if (strcmp(argv[i], "+iglx") == 0)
            enableIndirectGLX = TRUE;
        else if (strcmp(argv[i], "-iglx") == 0)
            enableIndirectGLX = FALSE;
        else if ((skip = XkbProcessArguments(argc, argv, i)) != 0) {
            if (skip > 0)
                i += skip - 1;
            else
                UseMsg();
        }
#ifdef LOCK_SERVER
        else if (strcmp(argv[i], "-nolock") == 0) {
#if !defined(WIN32)
            if (getuid() != 0)
                ErrorF
                    ("Warning: the -nolock option can only be used by root\n");
            else
#endif
                DisableServerLock();
        }
#endif
        else if ( strcmp( argv[i], "-maxclients") == 0)
        {
            if (++i < argc) {
                LimitClients = atoi(argv[i]);
                if (LimitClients != 64 &&
                    LimitClients != 128 &&
                    LimitClients != 256 &&
                    LimitClients != 512 &&
                    LimitClients != 1024 &&
                    LimitClients != 2048) {
                    FatalError("maxclients must be one of 64, 128, 256, 512, 1024 or 2048\n");
                }
            } else
                UseMsg();
        }
        else if (strcmp(argv[i], "-nolisten") == 0) {
            if (++i < argc) {
                if (_XSERVTransNoListen(argv[i]))
                    ErrorF("Failed to disable listen for %s transport",
                           argv[i]);
            }
            else
                UseMsg();
        }
        else if (strcmp(argv[i], "-listen") == 0) {
            if (++i < argc) {
                if (_XSERVTransListen(argv[i]))
                    ErrorF("Failed to enable listen for %s transport",
                           argv[i]);
            }
            else
                UseMsg();
        }
        else if (strcmp(argv[i],"-noreset") == 0){
            ErrorF("Argument -noreset is removed in XLibre (for more context: https://github.com/orgs/X11Libre/discussions/424 )\n");
        }
        else if(strcmp(argv[i],"-reset") == 0){
            ErrorF("Argument -reset is removed in XLibre (for more context: https://github.com/orgs/X11Libre/discussions/424 )\n");
        }
        else if (strcmp(argv[i], "-p") == 0) {
            if (++i < argc)
                defaultScreenSaverInterval = ((CARD32) atoi(argv[i])) *
                    MILLI_PER_MIN;
            else
                UseMsg();
        }
        else if (strcmp(argv[i], "-pogo") == 0) {
            dispatchException = DE_TERMINATE;
        }
        else if (strcmp(argv[i], "-pn") == 0)
            PartialNetwork = TRUE;
        else if (strcmp(argv[i], "-nopn") == 0)
            PartialNetwork = FALSE;
        else if (strcmp(argv[i], "r") == 0)
            defaultKeyboardControl.autoRepeat = TRUE;
        else if (strcmp(argv[i], "-r") == 0)
            defaultKeyboardControl.autoRepeat = FALSE;
        else if (strcmp(argv[i], "-retro") == 0)
            party_like_its_1989 = TRUE;
        else if (strcmp(argv[i], "-s") == 0) {
            if (++i < argc)
                defaultScreenSaverTime = ((CARD32) atoi(argv[i])) *
                    MILLI_PER_MIN;
            else
                UseMsg();
        }
        else if (strcmp(argv[i], "-seat") == 0) {
            if (++i < argc)
                dixSettingSeatId = argv[i];
            else
                UseMsg();
        }
        else if (strcmp(argv[i], "-t") == 0) {
            if (++i < argc)
                defaultPointerControl.threshold = atoi(argv[i]);
            else
                UseMsg();
        }
        else if (strcmp(argv[i], "-terminate") == 0) {
            dispatchExceptionAtReset = DE_TERMINATE;
            terminateDelay = -1;
            if ((i + 1 < argc) && (isdigit((unsigned char)*argv[i + 1])))
               terminateDelay = atoi(argv[++i]);
            terminateDelay = MAX(0, terminateDelay);
        }
        else if (strcmp(argv[i], "-tst") == 0) {
            noTestExtensions = TRUE;
        }
        else if (strcmp(argv[i], "v") == 0)
            defaultScreenSaverBlanking = PreferBlanking;
        else if (strcmp(argv[i], "-v") == 0)
            defaultScreenSaverBlanking = DontPreferBlanking;
        else if (strcmp(argv[i], "-wr") == 0)
            whiteRoot = TRUE;
        else if (strcmp(argv[i], "-background") == 0) {
            if (++i < argc) {
                if (!strcmp(argv[i], "none"))
                    bgNoneRoot = TRUE;
                else
                    UseMsg();
            }
        }
        else if (strcmp(argv[i], "-maxbigreqsize") == 0) {
            if (++i < argc) {
                long reqSizeArg = atol(argv[i]);

                /* Request size > 128MB does not make much sense... */
                if (reqSizeArg > 0L && reqSizeArg < 128L) {
                    maxBigRequestSize = (reqSizeArg * 1048576L) - 1L;
                }
                else {
                    UseMsg();
                }
            }
            else {
                UseMsg();
            }
        }
#ifdef CONFIG_NAMESPACE
        else if (strcmp(argv[i], "-namespace") == 0) {
            if (++i < argc) {
                namespaceConfigFile = argv[i];
                noNamespaceExtension = FALSE;
            }
            else
                UseMsg();
        }
#endif
#ifdef XINERAMA
        else if (strcmp(argv[i], "+xinerama") == 0) {
            noPanoramiXExtension = FALSE;
        }
        else if (strcmp(argv[i], "-xinerama") == 0) {
            noPanoramiXExtension = TRUE;
        }
        else if (strcmp(argv[i], "-disablexineramaextension") == 0) {
            PanoramiXExtensionDisabledHack = TRUE;
        }
#endif /* XINERAMA */
        else if (strcmp(argv[i], "-I") == 0) {
            /* ignore all remaining arguments */
            break;
        }
        else if (strncmp(argv[i], "tty", 3) == 0) {
            /* init supplies us with this useless information */
        }
#ifdef XDMCP
        else if ((skip = XdmcpOptions(argc, argv, i)) != i) {
            i = skip - 1;
        }
#endif
        else if (strcmp(argv[i], "-dumbSched") == 0) {
            InputThreadEnable = FALSE;
#ifdef HAVE_SETITIMER
            SmartScheduleSignalEnable = FALSE;
#endif
        }
        else if (strcmp(argv[i], "-schedInterval") == 0) {
            if (++i < argc) {
                SmartScheduleInterval = atoi(argv[i]);
                SmartScheduleSlice = SmartScheduleInterval;
            }
            else
                UseMsg();
        }
        else if (strcmp(argv[i], "-schedMax") == 0) {
            if (++i < argc) {
                SmartScheduleMaxSlice = atoi(argv[i]);
            }
            else
                UseMsg();
        }
        else if (strcmp(argv[i], "-render") == 0) {
            if (++i < argc) {
                int policy = PictureParseCmapPolicy(argv[i]);

                if (policy != PictureCmapPolicyInvalid)
                    PictureCmapPolicy = policy;
                else
                    UseMsg();
            }
            else
                UseMsg();
        }
        else if (strcmp(argv[i], "+extension") == 0) {
            if (++i < argc) {
                if (!EnableDisableExtension(argv[i], TRUE))
                    EnableDisableExtensionError(argv[i], TRUE);
            }
            else
                UseMsg();
        }
        else if (strcmp(argv[i], "-extension") == 0) {
            if (++i < argc) {
                if (!EnableDisableExtension(argv[i], FALSE))
                    EnableDisableExtensionError(argv[i], FALSE);
            }
            else
                UseMsg();
        }
#ifdef CONFIG_SYSLOG
        else if (ProcessCmdLineMultiInt(argc, argv, &i, "-syslogverbose", &xorgSyslogVerbosity));
#endif
        else {
            ErrorF("Unrecognized option: %s\n", argv[i]);
            UseMsg();
            FatalError("Unrecognized option: %s\n", argv[i]);
        }
    }
}

/* Implement a simple-minded font authorization scheme.  The authorization
   name is "hp-hostname-1", the contents are simply the host name. */
int
set_font_authorizations(char **authorizations, int *authlen, void *client)
{
#define AUTHORIZATION_NAME "hp-hostname-1"
#if defined(TCPCONN)
    static char *result = NULL;
    static char *p = NULL;

    if (p == NULL) {
        unsigned int len;

#if defined(HAVE_GETADDRINFO)
        struct addrinfo hints, *ai = NULL;
#else
        struct hostent *host;

#ifdef XTHREADS_NEEDS_BYNAMEPARAMS
        _Xgethostbynameparams hparams;
#endif
#endif

        struct xhostname hn;
        xhostname(&hn);

        char *hnameptr = NULL;
#if defined(HAVE_GETADDRINFO)
        memset(&hints, 0, sizeof(hints));
        hints.ai_flags = AI_CANONNAME;
        if (getaddrinfo(hn.name, NULL, &hints, &ai) == 0) {
            hnameptr = ai->ai_canonname;
        }
        else {
            hnameptr = hn.name;
        }
#else
        host = _XGethostbyname(hn.name, hparams);
        if (host == NULL)
            hnameptr = hn.name;
        else
            hnameptr = host->h_name;
#endif

        len = strlen(hnameptr) + 1;
        result = calloc(1, len + sizeof(AUTHORIZATION_NAME) + 4);
        if (!result)
            return 0;

        p = result;
        *p++ = sizeof(AUTHORIZATION_NAME) >> 8;
        *p++ = sizeof(AUTHORIZATION_NAME) & 0xff;
        *p++ = (len) >> 8;
        *p++ = (len & 0xff);

        memcpy(p, AUTHORIZATION_NAME, sizeof(AUTHORIZATION_NAME));
        p += sizeof(AUTHORIZATION_NAME);
        memcpy(p, hnameptr, len);
        p += len;
#if defined(HAVE_GETADDRINFO)
        if (ai) {
            freeaddrinfo(ai);
        }
#endif
    }
    *authlen = p - result;
    *authorizations = result;
    return 1;
#else                           /* TCPCONN */
    return 0;
#endif                          /* TCPCONN */
}

void
SmartScheduleStopTimer(void)
{
#ifdef HAVE_SETITIMER
    struct itimerval timer;

    if (!SmartScheduleSignalEnable)
        return;
    timer.it_interval.tv_sec = 0;
    timer.it_interval.tv_usec = 0;
    timer.it_value.tv_sec = 0;
    timer.it_value.tv_usec = 0;
    (void) setitimer(ITIMER_REAL, &timer, 0);
#endif
}

void
SmartScheduleStartTimer(void)
{
#ifdef HAVE_SETITIMER
    struct itimerval timer;

    if (!SmartScheduleSignalEnable)
        return;
    timer.it_interval.tv_sec = 0;
    timer.it_interval.tv_usec = SmartScheduleInterval * 1000;
    timer.it_value.tv_sec = 0;
    timer.it_value.tv_usec = SmartScheduleInterval * 1000;
    setitimer(ITIMER_REAL, &timer, 0);
#endif
}

#ifdef HAVE_SETITIMER
static void
SmartScheduleTimer(int sig)
{
    SmartScheduleTime += SmartScheduleInterval;
}

static int
SmartScheduleEnable(void)
{
    int ret = 0;
    struct sigaction act;

    if (!SmartScheduleSignalEnable)
        return 0;

    memset((char *) &act, 0, sizeof(struct sigaction));

    /* Set up the timer signal function */
    act.sa_flags = SA_RESTART;
    act.sa_handler = SmartScheduleTimer;
    sigemptyset(&act.sa_mask);
    sigaddset(&act.sa_mask, SIGALRM);
    ret = sigaction(SIGALRM, &act, 0);
    return ret;
}

static int
SmartSchedulePause(void)
{
    int ret = 0;
    struct sigaction act;

    if (!SmartScheduleSignalEnable)
        return 0;

    memset((char *) &act, 0, sizeof(struct sigaction));

    act.sa_handler = SIG_IGN;
    sigemptyset(&act.sa_mask);
    ret = sigaction(SIGALRM, &act, 0);
    return ret;
}
#endif

void
SmartScheduleInit(void)
{
#ifdef HAVE_SETITIMER
    if (SmartScheduleEnable() < 0) {
        perror("sigaction for smart scheduler");
        SmartScheduleSignalEnable = FALSE;
    }
#endif
}

#ifdef HAVE_SIGPROCMASK
static sigset_t PreviousSignalMask;
static int BlockedSignalCount;
#endif

void
OsBlockSignals(void)
{
#ifdef HAVE_SIGPROCMASK
    if (BlockedSignalCount++ == 0) {
        sigset_t set;

        sigemptyset(&set);
        sigaddset(&set, SIGALRM);
        sigaddset(&set, SIGVTALRM);
#ifdef SIGWINCH
        sigaddset(&set, SIGWINCH);
#endif
        sigaddset(&set, SIGTSTP);
        sigaddset(&set, SIGTTIN);
        sigaddset(&set, SIGTTOU);
        sigaddset(&set, SIGCHLD);
        xthread_sigmask(SIG_BLOCK, &set, &PreviousSignalMask);
    }
#endif
}

void
OsReleaseSignals(void)
{
#ifdef HAVE_SIGPROCMASK
    if (--BlockedSignalCount == 0) {
        xthread_sigmask(SIG_SETMASK, &PreviousSignalMask, 0);
    }
#endif
}

void
OsResetSignals(void)
{
#ifdef HAVE_SIGPROCMASK
    while (BlockedSignalCount > 0)
        OsReleaseSignals();
    input_force_unlock();
#endif
}

/*
 * Pending signals may interfere with core dumping. Provide a
 * mechanism to block signals when aborting.
 */

void
OsAbort(void)
{
#ifndef __APPLE__
    OsBlockSignals();
#endif
#if !defined(WIN32)
    /* abort() raises SIGABRT, so we have to stop handling that to prevent
     * recursion
     */
    OsSignal(SIGABRT, SIG_DFL);
#endif
    abort();
}

#if !defined(WIN32)
/*
 * "safer" versions of system(3), popen(3) and pclose(3) which give up
 * all privs before running a command.
 *
 * This is based on the code in FreeBSD 2.2 libc.
 *
 * XXX It'd be good to redirect stderr so that it ends up in the log file
 * as well.  As it is now, xkbcomp messages don't end up in the log file.
 */

static struct pid {
    struct pid *next;
    FILE *fp;
    int pid;
} *pidlist;

void *
Popen(const char *command, const char *type)
{
    struct pid *cur;
    FILE *iop;
    int pdes[2], pid;

    if (command == NULL || type == NULL)
        return NULL;

    if ((*type != 'r' && *type != 'w') || type[1])
        return NULL;

    if ((cur = calloc(1, sizeof(struct pid))) == NULL)
        return NULL;

    if (pipe(pdes) < 0) {
        free(cur);
        return NULL;
    }

    /* Ignore the smart scheduler while this is going on */
#ifdef HAVE_SETITIMER
    if (SmartSchedulePause() < 0) {
        close(pdes[0]);
        close(pdes[1]);
        free(cur);
        perror("signal");
        return NULL;
    }
#endif

    switch (pid = fork()) {
    case -1:                   /* error */
        close(pdes[0]);
        close(pdes[1]);
        free(cur);
#ifdef HAVE_SETITIMER
        if (SmartScheduleEnable() < 0)
            perror("signal");
#endif
        return NULL;
    case 0:                    /* child */
        if (setgid(getgid()) == -1)
            _exit(127);
        if (setuid(getuid()) == -1)
            _exit(127);
        if (*type == 'r') {
            if (pdes[1] != 1) {
                /* stdout */
                dup2(pdes[1], 1);
                close(pdes[1]);
            }
            close(pdes[0]);
        }
        else {
            if (pdes[0] != 0) {
                /* stdin */
                dup2(pdes[0], 0);
                close(pdes[0]);
            }
            close(pdes[1]);
        }
        execl("/bin/sh", "sh", "-c", command, (char *) NULL);
        _exit(127);
    }

    /* Avoid EINTR during stdio calls */
    OsBlockSignals();

    /* parent */
    if (*type == 'r') {
        iop = fdopen(pdes[0], type);
        close(pdes[1]);
    }
    else {
        iop = fdopen(pdes[1], type);
        close(pdes[0]);
    }

    cur->fp = iop;
    cur->pid = pid;
    cur->next = pidlist;
    pidlist = cur;

    DebugF("Popen: `%s', fp = %p\n", command, iop);

    return iop;
}

/* fopen that drops privileges */
void *
Fopen(const char *file, const char *type)
{
    FILE *iop;
    int ruid, euid;

    ruid = getuid();
    euid = geteuid();

    if (seteuid(ruid) == -1) {
        return NULL;
    }
    iop = fopen(file, type);

    if (seteuid(euid) == -1) {
        fclose(iop);
        return NULL;
    }
    return iop;
}

int
Pclose(void *iop)
{
    struct pid *cur, *last;
    int pstat;
    int pid;

    DebugF("Pclose: fp = %p\n", iop);
    fclose(iop);

    for (last = NULL, cur = pidlist; cur; last = cur, cur = cur->next)
        if (cur->fp == iop)
            break;
    if (cur == NULL)
        return -1;

    do {
        pid = waitpid(cur->pid, &pstat, 0);
    } while (pid == -1 && errno == EINTR);

    if (last == NULL)
        pidlist = cur->next;
    else
        last->next = cur->next;
    free(cur);

    /* allow EINTR again */
    OsReleaseSignals();

#ifdef HAVE_SETITIMER
    if (SmartScheduleEnable() < 0) {
        perror("signal");
        return -1;
    }
#endif

    return pid == -1 ? -1 : pstat;
}

int
Fclose(void *iop)
{
    return fclose(iop);
}

#endif                          /* !WIN32 */

#ifdef WIN32

#include <X11/Xwindows.h>

const char *
Win32TempDir(void)
{
    static char buffer[PATH_MAX];

    if (GetTempPath(sizeof(buffer), buffer)) {
        int len;

        buffer[sizeof(buffer) - 1] = 0;
        len = strlen(buffer);
        if (len > 0)
            if (buffer[len - 1] == '\\')
                buffer[len - 1] = 0;
        return buffer;
    }
    if (getenv("TEMP") != NULL)
        return getenv("TEMP");
    else if (getenv("TMP") != NULL)
        return getenv("TMP");
    else
        return "/tmp";
}
#endif

Bool
PrivsElevated(void)
{
    static Bool privsTested = FALSE;
    static Bool privsElevated = TRUE;

    if (!privsTested) {
#if defined(WIN32)
        privsElevated = FALSE;
#else
        if ((getuid() != geteuid()) || (getgid() != getegid())) {
            privsElevated = TRUE;
        }
        else {
#if defined(HAVE_ISSETUGID)
            privsElevated = issetugid();
#elif defined(HAVE_GETRESUID)
            uid_t ruid, euid, suid;
            gid_t rgid, egid, sgid;

            if ((getresuid(&ruid, &euid, &suid) == 0) &&
                (getresgid(&rgid, &egid, &sgid) == 0)) {
                privsElevated = (euid != suid) || (egid != sgid);
            }
            else {
                printf("Failed getresuid or getresgid");
                /* Something went wrong, make defensive assumption */
                privsElevated = TRUE;
            }
#else
            if (getuid() == 0) {
                /* running as root: uid==euid==0 */
                privsElevated = FALSE;
            }
            else {
                /*
                 * If there are saved ID's the process might still be privileged
                 * even though the above test succeeded. If issetugid() and
                 * getresgid() aren't available, test this by trying to set
                 * euid to 0.
                 */
                unsigned int oldeuid;

                oldeuid = geteuid();

                if (seteuid(0) != 0) {
                    privsElevated = FALSE;
                }
                else {
                    if (seteuid(oldeuid) != 0) {
                        FatalError("Failed to drop privileges.  Exiting\n");
                    }
                    privsElevated = TRUE;
                }
            }
#endif
        }
#endif
        privsTested = TRUE;
    }
    return privsElevated;
}

/*
 * CheckUserParameters: check for long command line arguments and long
 * environment variables.  By default, these checks are only done when
 * the server's euid != ruid.  In 3.3.x, these checks were done in an
 * external wrapper utility.
 */

/* Check args and env only if running setuid (euid == 0 && euid != uid) ? */
#ifndef CHECK_EUID
#ifndef WIN32
#define CHECK_EUID 1
#else
#define CHECK_EUID 0
#endif
#endif

#define MAX_ARG_LENGTH          128
#define MAX_ENV_LENGTH          256
#define MAX_ENV_PATH_LENGTH     2048    /* Limit for *PATH and TERMCAP */

#define checkPrintable(c) (((c) & 0x7f) >= 0x20 && ((c) & 0x7f) != 0x7f)

enum BadCode {
    NotBad = 0,
    UnsafeArg,
    ArgTooLong,
    UnprintableArg,
    InternalError
};

void
CheckUserParameters(int argc, char **argv, char **envp)
{
    enum BadCode bad = NotBad;
    int i = 0, j;
    char *a = NULL;

#if CHECK_EUID
    if (PrivsElevated())
#endif
    {
        /* Check each argv[] */
        for (i = 1; i < argc; i++) {
            if (strcmp(argv[i], "-fp") == 0) {
                i++;            /* continue with next argument. skip the length check */
                if (i >= argc)
                    break;
            }
            else {
                if (strlen(argv[i]) > MAX_ARG_LENGTH) {
                    bad = ArgTooLong;
                    break;
                }
            }
            a = argv[i];
            while (*a) {
                if (checkPrintable(*a) == 0) {
                    bad = UnprintableArg;
                    break;
                }
                a++;
            }
            if (bad)
                break;
        }
        if (!bad) {
            /* Check each envp[] */
            for (i = 0; envp[i]; i++) {

                /* Check for bad environment variables and values */
                while (envp[i] && (strncmp(envp[i], "LD", 2) == 0)) {
                    for (j = i; envp[j]; j++) {
                        envp[j] = envp[j + 1];
                    }
                }
                if (envp[i] && (strlen(envp[i]) > MAX_ENV_LENGTH)) {
                    for (j = i; envp[j]; j++) {
                        envp[j] = envp[j + 1];
                    }
                    i--;
                }
            }
        }
    }
    switch (bad) {
    case NotBad:
        return;
    case UnsafeArg:
        ErrorF("Command line argument number %d is unsafe\n", i);
        break;
    case ArgTooLong:
        ErrorF("Command line argument number %d is too long\n", i);
        break;
    case UnprintableArg:
        ErrorF("Command line argument number %d contains unprintable"
               " characters\n", i);
        break;
    case InternalError:
        ErrorF("Internal Error\n");
        break;
    default:
        ErrorF("Unknown error\n");
        break;
    }
    FatalError("X server aborted because of unsafe environment\n");
}

/*
 * CheckUserAuthorization: check if the user is allowed to start the
 * X server.  This usually means some sort of PAM checking, and it is
 * usually only done for setuid servers (uid != euid).
 */

#ifdef USE_PAM
#include <security/pam_appl.h>
#include <security/pam_misc.h>
#include <pwd.h>
#endif                          /* USE_PAM */

void
CheckUserAuthorization(void)
{
#ifdef USE_PAM
    static struct pam_conv conv = {
        misc_conv,
        NULL
    };

    pam_handle_t *pamh = NULL;
    struct passwd *pw;
    int retval;

    if (getuid() != geteuid()) {
        pw = getpwuid(getuid());
        if (pw == NULL)
            FatalError("getpwuid() failed for uid %d\n", getuid());

        retval = pam_start("xserver", pw->pw_name, &conv, &pamh);
        if (retval != PAM_SUCCESS)
            FatalError("pam_start() failed.\n"
                       "\tMissing or mangled PAM config file or module?\n");

        retval = pam_authenticate(pamh, 0);
        if (retval != PAM_SUCCESS) {
            pam_end(pamh, retval);
            FatalError("PAM authentication failed, cannot start X server.\n"
                       "\tPerhaps you do not have console ownership?\n");
        }

        retval = pam_acct_mgmt(pamh, 0);
        if (retval != PAM_SUCCESS) {
            pam_end(pamh, retval);
            FatalError("PAM authentication failed, cannot start X server.\n"
                       "\tPerhaps you do not have console ownership?\n");
        }

        /* this is not a session, so do not do session management */
        pam_end(pamh, PAM_SUCCESS);
    }
#endif
}

#if !defined(WIN32)
/* Move a file descriptor out of the way of our select mask; this
 * is useful for file descriptors which will never appear in the
 * select mask to avoid reducing the number of clients that can
 * connect to the server
 */
int
os_move_fd(int fd)
{
    int newfd;

#ifdef F_DUPFD_CLOEXEC
    newfd = fcntl(fd, F_DUPFD_CLOEXEC, MAXCLIENTS);
#else
    newfd = fcntl(fd, F_DUPFD, MAXCLIENTS);
#endif
    if (newfd < 0)
        return fd;
#ifndef F_DUPFD_CLOEXEC
    fcntl(newfd, F_SETFD, FD_CLOEXEC);
#endif
    close(fd);
    return newfd;
}
#endif

void
AbortServer(void)
{
#ifdef XF86BIGFONT
    XF86BigfontCleanup();
#endif
    CloseWellKnownConnections();
    UnlockServer();
    AbortDevices();
    ddxGiveUp(EXIT_ERR_ABORT);
    fflush(stderr);
    if (CoreDump)
        OsAbort();
    exit(1);
}

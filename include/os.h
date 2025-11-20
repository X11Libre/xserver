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

#ifndef OS_H
#define OS_H

#include "callback.h"
#include "misc.h"

#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#ifdef MONOTONIC_CLOCK
#include <time.h>
#endif

#include <X11/Xfuncproto.h>

/*
 * @brief macro for specifying non-null arguments
 *
 * part of public SDK / driver API
 */
#ifndef _X_ATTRIBUTE_NONNULL_ARG
#define _X_ATTRIBUTE_NONNULL_ARG(...) __attribute__((nonnull(__VA_ARGS__)))
#endif

#ifndef _X_ATTRIBUTE_VPRINTF
# if defined(__GNUC__) && (__GNUC__ >= 2) && (!defined(__APPLE__)) && (!defined(__FreeBSD__))
#  define _X_ATTRIBUTE_VPRINTF(fmt, firstarg) \
          __attribute__((__format__(gnu_printf, fmt, firstarg)))
# else
#  define _X_ATTRIBUTE_VPRINTF(fmt, firstarg) _X_ATTRIBUTE_PRINTF(fmt,firstarg)
# endif
#endif

#define SCREEN_SAVER_ON   0
#define SCREEN_SAVER_OFF  1
#define SCREEN_SAVER_FORCER 2
#define SCREEN_SAVER_CYCLE  3

#ifndef MAX_REQUEST_SIZE
#define MAX_REQUEST_SIZE 65535
#endif

typedef struct _FontPathRec *FontPathPtr;
typedef struct _NewClientRec *NewClientPtr;

#ifndef xnfalloc
#define xnfalloc(size) XNFalloc((unsigned long)(size))
#define xnfcalloc(_num, _size) XNFcallocarray((_num), (_size))
#define xnfrealloc(ptr, size) XNFrealloc((void *)(ptr), (unsigned long)(size))

#define xstrdup(s) Xstrdup(s)
#define xnfstrdup(s) XNFstrdup(s)
#endif

#include <stdio.h>
#include <stdarg.h>

int ReadFdFromClient(ClientPtr client);

extern _X_EXPORT int WriteToClient(ClientPtr /*who */ , int /*count */ ,
                                   const void * /*buf */ );

typedef void (*NotifyFdProcPtr)(int fd, int ready, void *data);

#include "fd_notify.h"

extern _X_EXPORT Bool SetNotifyFd(int fd, NotifyFdProcPtr notify_fd, int mask, void *data);

static inline void RemoveNotifyFd(int fd)
{
    (void) SetNotifyFd(fd, NULL, X_NOTIFY_NONE, NULL);
}

extern _X_EXPORT void IgnoreClient(ClientPtr /*client */ );

extern _X_EXPORT void AttendClient(ClientPtr /*client */ );

extern _X_EXPORT CARD32 GetTimeInMillis(void);
extern _X_EXPORT CARD64 GetTimeInMicros(void);

extern _X_EXPORT void AdjustWaitForDelay(void *waitTime, int newdelay);

typedef struct _OsTimerRec *OsTimerPtr;

typedef CARD32 (*OsTimerCallback) (OsTimerPtr timer,
                                   CARD32 time,
                                   void *arg);

#define TimerAbsolute (1<<0)
#define TimerForceOld (1<<1)

extern _X_EXPORT OsTimerPtr TimerSet(OsTimerPtr timer,
                                     int flags,
                                     CARD32 millis,
                                     OsTimerCallback func,
                                     void *arg);

extern _X_EXPORT void TimerCancel(OsTimerPtr /* pTimer */ );
extern _X_EXPORT void TimerFree(OsTimerPtr /* pTimer */ );

// FIXME: drop use in sisusb driver!
extern _X_EXPORT void GiveUp(int /*sig */ );

/*
 * This function malloc(3)s buffer, terminating the server if there is not
 * enough memory.
 */
extern _X_EXPORT void *
XNFalloc(unsigned long /*amount */ ) __attribute__((returns_nonnull));

/*
 * This function calloc(3)s buffer, terminating the server if there is not
 * enough memory.
 */
extern _X_EXPORT void *
XNFcalloc(unsigned long /*amount */ ) _X_DEPRECATED;

/*
 * This function calloc(3)s buffer, terminating the server if there is not
 * enough memory or the arguments overflow when multiplied
 */
extern _X_EXPORT void *
XNFcallocarray(size_t nmemb, size_t size) __attribute__((returns_nonnull));

/*
 * This function realloc(3)s passed buffer, terminating the server if there is
 * not enough memory.
 */
extern _X_EXPORT void *
XNFrealloc(void * /*ptr */ , unsigned long /*amount */ );

/*
 * This function strdup(3)s passed string. The only difference from the library
 * function that it is safe to pass NULL, as NULL will be returned.
 */
extern _X_EXPORT char *
Xstrdup(const char *s);

/*
 * This function strdup(3)s passed string, terminating the server if there is
 * not enough memory. If NULL is passed to this function, NULL is returned.
 */
extern _X_EXPORT char *
XNFstrdup(const char *s);

/* Include new X*asprintf API */
#include "Xprintf.h"

typedef int (*OsSigWrapperPtr) (int /* sig */ );

extern _X_EXPORT OsSigWrapperPtr
OsRegisterSigWrapper(OsSigWrapperPtr newWrap);

Bool PrivsElevated(void);

int GetClientFd(ClientPtr);

/* stuff for FlushCallback */
extern _X_EXPORT CallbackListPtr FlushCallback;

extern _X_EXPORT int
TimeSinceLastInputEvent(void);

/* Function fallbacks provided by AC_REPLACE_FUNCS in configure.ac */

#ifndef HAVE_REALLOCARRAY
#define reallocarray xreallocarray
extern _X_EXPORT void *
reallocarray(void *optr, size_t nmemb, size_t size);
#endif

#ifndef HAVE_STRCASESTR
#define strcasestr xstrcasestr
extern _X_EXPORT char *
xstrcasestr(const char *s, const char *find);
#endif

#ifndef HAVE_STRLCPY
extern _X_EXPORT size_t
strlcpy(char *dst, const char *src, size_t siz);
extern _X_EXPORT size_t
strlcat(char *dst, const char *src, size_t siz);
#endif

#ifndef HAVE_STRNDUP
extern _X_EXPORT char *
strndup(const char *str, size_t n);
#endif

// drop from driver's AC
#ifndef HAVE_TIMINGSAFE_MEMCMP
extern _X_EXPORT int
timingsafe_memcmp(const void *b1, const void *b2, size_t len);
#endif

/* Flags for log messages. */
typedef enum {
    X_PROBED,                   /* Value was probed */
    X_CONFIG,                   /* Value was given in the config file */
    X_DEFAULT,                  /* Value is a default */
    X_CMDLINE,                  /* Value was given on the command line */
    X_NOTICE,                   /* Notice */
    X_ERROR,                    /* Error message */
    X_WARNING,                  /* Warning message */
    X_INFO,                     /* Informational message */
    X_NONE,                     /* No prefix */
    X_NOT_IMPLEMENTED,          /* Not implemented */
    X_DEBUG,                    /* Debug message */
    X_UNKNOWN = -1              /* unknown -- this must always be last */
} MessageType;

extern _X_EXPORT void
LogVMessageVerb(MessageType type, int verb, const char *format, va_list args)
_X_ATTRIBUTE_PRINTF(3, 0);
extern _X_EXPORT void
LogMessageVerb(MessageType type, int verb, const char *format, ...)
_X_ATTRIBUTE_PRINTF(3, 4);
extern _X_EXPORT void
LogMessage(MessageType type, const char *format, ...)
_X_ATTRIBUTE_PRINTF(2, 3);

void
LogHdrMessageVerb(MessageType type, int verb,
                  const char *msg_format, va_list msg_args,
                  const char *hdr_format, ...)
_X_ATTRIBUTE_PRINTF(3, 0)
_X_ATTRIBUTE_PRINTF(5, 6);

extern _X_EXPORT void
FatalError(const char *f, ...)
_X_ATTRIBUTE_PRINTF(1, 2)
    _X_NORETURN;

extern _X_EXPORT void
ErrorF(const char *f, ...)
_X_ATTRIBUTE_PRINTF(1, 2);

extern _X_EXPORT void
xorg_backtrace(void);

/* should not be used anymore, just for backwards compat with drivers */
#define LogVMessageVerbSigSafe(...) LogVMessageVerb(__VA_ARGS__)
#define LogMessageVerbSigSafe(...) LogMessageVerb(__VA_ARGS__)
#define ErrorFSigSafe(...) ErrorF(__VA_ARGS__)
#define VErrorFSigSafe(...) VErrorF(__VA_ARGS__)
#define VErrorF(...) LogVMessageVerb(X_NONE, -1, __VA_ARGS__)

/* only for backwards compat with drivers that haven't kept up yet
   (xf86-video-intel)

   @todo revise after next stable release
*/
_X_DEPRECATED
static inline int System(const char* cmdline)
{
    return system(cmdline);
}

#endif                          /* OS_H */

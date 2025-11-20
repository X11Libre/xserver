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
#ifndef _OSDEP_H_
#define _OSDEP_H_ 1

#include <dix-config.h>

#include <X11/Xdefs.h>

#include <limits.h>
#include <signal.h>
#include <stddef.h>
#include <X11/Xos.h>
#include <X11/Xmd.h>
#include <X11/Xdefs.h>

/*
 * return the least significant bit in x which is set
 *
 * This works on 1's complement and 2's complement machines.
 * If you care about the extra instruction on 2's complement
 * machines, change to ((x) & (-(x)))
 */
#define lowbit(x) ((x) & (~(x) + 1))

#ifndef __has_builtin
# define __has_builtin(x) 0     /* Compatibility with older compilers */
#endif

#define MILLI_PER_MIN (1000 * 60)
#define MILLI_PER_SECOND (1000)

#include "dix.h"
#include "ospoll.h"

extern struct ospoll    *server_poll;

Bool
listen_to_client(ClientPtr client);

extern Bool NewOutputPending;

/* for platforms lacking arc4random_buf() libc function */
#ifndef HAVE_ARC4RANDOM_BUF
static inline void arc4random_buf(void *buf, size_t nbytes)
{
    int fd = open("/dev/urandom", O_RDONLY);
    read(fd, buf, nbytes);
    close(fd);
}
#endif /* HAVE_ARC4RANDOM_BUF */

/* OsTimer functions */
void TimerInit(void);

/* must be exported for backwards compatibility with legacy nvidia390,
 * not for use in maintained drivers
 */
_X_EXPORT Bool TimerForce(OsTimerPtr);

#ifdef WIN32
#include <X11/Xwinsock.h>
#include <signal.h>

typedef _sigset_t sigset_t;

#undef CreateWindow

const char *Win32TempDir(void);

static inline void Fclose(void *f) { fclose(f); }
static inline void *Fopen(const char *a, const char *b) { return fopen(a,b); }

#else /* WIN32 */

void *Popen(const char *, const char *);
void *Fopen(const char *, const char *);
int Fclose(void *f);
int Pclose(void *f);

#endif /* WIN32 */

/* clone fd so it gets out of our select mask */
int os_move_fd(int fd);

/* set signal mask - either on current thread or whole process,
   depending on whether multithreading is used */
int xthread_sigmask(int how, const sigset_t *set, sigset_t *oldest);

typedef void (*OsSigHandlerPtr) (int sig);

/* install signal handler */
OsSigHandlerPtr OsSignal(int sig, OsSigHandlerPtr handler);

void OsInit(void);

_X_EXPORT /* needed by the int10 module, but should not be used by OOT drivers */
void OsBlockSignals(void);

_X_EXPORT /* needed by the int10 module, but should not be used by OOT drivers */
void OsReleaseSignals(void);

void OsResetSignals(void);
void OsAbort(void) _X_NORETURN;
void AbortServer(void) _X_NORETURN;

void MakeClientGrabPervious(ClientPtr client);
void MakeClientGrabImpervious(ClientPtr client);

int OnlyListenToOneClient(ClientPtr client);

void ListenToAllClients(void);

/* allow DDX to force using another clock */
void ForceClockId(clockid_t forced_clockid);

Bool WaitForSomething(Bool clients_are_ready);
void CloseDownConnection(ClientPtr client);

extern int LimitClients;
extern Bool PartialNetwork;

extern Bool CoreDump;
extern Bool NoListenAll;

/*
 * This function reallocarray(3)s passed buffer, terminating the server if
 * there is not enough memory or the arguments overflow when multiplied.
 */
void *XNFreallocarray(void *ptr, size_t nmemb, size_t size);

#if __has_builtin(__builtin_popcountl)
# define Ones __builtin_popcountl
#else
/*
 * Count the number of bits set to 1 in a 32-bit word.
 * Algorithm from MIT AI Lab Memo 239: "HAKMEM", ITEM 169.
 * https://dspace.mit.edu/handle/1721.1/6086
 */
static inline int
Ones(unsigned long mask)
{
    unsigned long y;

    y = (mask >> 1) & 033333333333;
    y = mask - y - ((y >> 1) & 033333333333);
    return (((y + (y >> 3)) & 030707070707) % 077);
}
#endif

/* static assert for protocol structure sizes */
#ifndef __size_assert
#define __size_assert(what, howmuch) \
  typedef char what##_size_wrong_[( !!(sizeof(what) == howmuch) )*2-1 ]
#endif

/*
 * like strlen(), but checking for NULL and return 0 in this case
 */
static inline size_t x_safe_strlen(const char *str) {
    return (str ? strlen(str) : 0);
}

enum ExitCode {
    EXIT_NO_ERROR = 0,
    EXIT_ERR_ABORT = 1,
    EXIT_ERR_CONFIGURE = 2,
    EXIT_ERR_DRIVERS = 3,
};

extern sig_atomic_t inSignalContext;

/* run timers that are expired at timestamp `now` */
void DoTimers(CARD32 now);

/**
 * Calculate padding needed to bring the number of bytes to an even
 * multiple of 4.
 * @param bytes The minimum number of bytes needed.
 * @return The bytes of padding needed to arrive at the closest multiple of 4
 * that is equal or higher than bytes.
 */
static inline int padding_for_int32(const int bytes)
{
    return ((-bytes) & 3);
}

/**
 * Calculate the number of bytes (in multiples of 4) needed to hold bytes.
 * @param bytes The minimum number of bytes needed.
 * @return The closest multiple of 4 that is equal or higher than bytes.
 */
static inline int pad_to_int32(const int bytes)
{
    return (((bytes) + 3) & ~3);
}

/**
 * Calculate the number of 4-byte units needed to hold the given number of
 * bytes.
 * @param bytes The minimum number of bytes needed.
 * @return The number of 4-byte units needed to hold bytes.
 */
static inline CARD32 bytes_to_int32(const size_t bytes)
{
    return (CARD32)(((bytes) + 3) >> 2);
}

/**
 * Calculate the number of bytes needed to hold bits.
 * @param bits The minimum number of bits needed.
 * @return The number of bytes needed to hold bits.
 */
static inline int
bits_to_bytes(const int bits)
{
    return ((bits + 7) >> 3);
}

#endif                          /* _OSDEP_H_ */

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

#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#ifdef MONOTONIC_CLOCK
#include <time.h>
#endif

#include <X11/Xfuncproto.h>

#include "xlibre_ptrtypes.h"
#include "callback.h"
#include "misc.h"

/*
 * @brief macro for specifying non-null arguments
 *
 * part of public SDK / driver API
 */
#ifndef _X_ATTRIBUTE_NONNULL_ARG
#define _X_ATTRIBUTE_NONNULL_ARG(...) __attribute__((nonnull(__VA_ARGS__)))
#endif

#ifndef _X_ATTRIBUTE_VPRINTF
# if defined(__GNUC__) && (__GNUC__ >= 2) && !defined(__clang__)
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

typedef struct _NewClientRec *NewClientPtr;

#ifndef xnfalloc
#define xnfalloc(size) XNFalloc((unsigned long)(size))
#define xnfcalloc(_num, _size) XNFcallocarray((_num), (_size))
#define xnfrealloc(ptr, size) XNFrealloc((void *)(ptr), (unsigned long)(size))

#define xstrdup(s) Xstrdup((s))
#define xnfstrdup(s) XNFstrdup((s))
#endif

#include "alloc.h"
#include "Xprintf.h"
#include "client_io.h"
#include "fd_notify.h"
#include "logging.h"
#include "notify_fd.h"
#include "timer.h"
#include "fallback_funcs.h"

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
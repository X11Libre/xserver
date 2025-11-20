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

Copyright 1992, 1993 Data General Corporation;
Copyright 1992, 1993 OMRON Corporation

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that the
above copyright notice appear in all copies and that both that copyright
notice and this permission notice appear in supporting documentation, and that
neither the name OMRON or DATA GENERAL be used in advertising or publicity
pertaining to distribution of the software without specific, written prior
permission of the party whose name is to be used.  Neither OMRON or
DATA GENERAL make any representation about the suitability of this software
for any purpose.  It is provided "as is" without express or implied warranty.

OMRON AND DATA GENERAL EACH DISCLAIM ALL WARRANTIES WITH REGARD TO THIS
SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS,
IN NO EVENT SHALL OMRON OR DATA GENERAL BE LIABLE FOR ANY SPECIAL, INDIRECT
OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
OF THIS SOFTWARE.

******************************************************************/
#ifndef MISC_H
#define MISC_H 1
/*
 *  X internal definitions
 *
 */

#include <X11/Xosdefs.h>
#include <X11/Xfuncproto.h>
#include <X11/Xmd.h>
#include <X11/X.h>
#include <X11/Xdefs.h>

#include <stddef.h>
#include <stdint.h>
#include <pthread.h>

#ifndef MAXSCREENS
#define MAXSCREENS	16
#endif
#ifndef MAXGPUSCREENS
#define MAXGPUSCREENS	16
#endif
#define MAXFORMATS	8
#ifndef MAXDEVICES
#define MAXDEVICES	256      /* input devices */
#endif
#define GPU_SCREEN_OFFSET 256

/* 128 event opcodes for core + extension events, excluding GE */
#define MAXEVENTS       128
#define EXTENSION_EVENT_BASE 64
#define EXTENSION_BASE 128

typedef uint32_t ATOM;

/* @brief generic X return code
 *
 * this type is should be used instead of plain int for all functions
 * returning and X error code (that's possibly sent to the client),
 * in order to make return value semantics clear to the humen reader.
 *
 * part of public SDK / driver API.
 */
typedef int XRetCode;

#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif

#include "os.h"                 /* for ALLOCATE_LOCAL and DEALLOCATE_LOCAL */
#include <X11/Xfuncs.h>         /* for bcopy, bzero, and bcmp */

#define NullBox ((BoxPtr)0)

#undef min
#undef max
/* @deprecated */
#define min(a, b) (((a) < (b)) ? (a) : (b))
#define max(a, b) (((a) > (b)) ? (a) : (b))
/* abs() is a function, not a macro; include the file declaring
 * it in case we haven't done that yet.
 */
/* this assumes b > 0 */
#define modulus(a, b, d)    if (((d) = (a) % (b)) < 0) (d) += (b)

/* XXX Not for modules */
#include <limits.h>
#if !defined(MAXSHORT) || !defined(MINSHORT) || \
    !defined(MAXINT) || !defined(MININT)
/*
 * Some implementations #define these through <math.h>, so preclude
 * #include'ing it later.
 */

#include <math.h>
#undef MAXSHORT
#define MAXSHORT SHRT_MAX
#undef MINSHORT
#define MINSHORT SHRT_MIN
#undef MAXINT
#define MAXINT INT_MAX
#undef MININT
#define MININT INT_MIN

#include <assert.h>
#include <ctype.h>
#include <stdio.h>              /* for fopen, etc... */

#endif

/* some macros to help swap requests, replies, and events */

#define LengthRestS(stuff) \
    ((client->req_len << 1) - (sizeof(*stuff) >> 1))

#define SwapRestS(stuff) \
    SwapShorts((short *)(stuff + 1), LengthRestS(stuff))

#if defined(__GNUC__) && ((__GNUC__ > 4) || (__GNUC__ == 4 && __GNUC_MINOR__ >= 3))
void __attribute__ ((error("wrong sized variable passed to swap")))
wrong_size(void);
#else
static inline void
wrong_size(void)
{
}
#endif

#if !(defined(__GNUC__))
static inline int
__builtin_constant_p(int x)
{
    return 0;
}
#endif

static inline uint64_t
bswap_64(uint64_t x)
{
    return (((x & 0xFF00000000000000ull) >> 56) |
            ((x & 0x00FF000000000000ull) >> 40) |
            ((x & 0x0000FF0000000000ull) >> 24) |
            ((x & 0x000000FF00000000ull) >>  8) |
            ((x & 0x00000000FF000000ull) <<  8) |
            ((x & 0x0000000000FF0000ull) << 24) |
            ((x & 0x000000000000FF00ull) << 40) |
            ((x & 0x00000000000000FFull) << 56));
}

#define swapll(x) do { \
		if (sizeof(*(x)) != 8) \
			wrong_size(); \
		*(x) = bswap_64(*(x));          \
	} while (0)

static inline uint32_t
bswap_32(uint32_t x)
{
    return (((x & 0xFF000000) >> 24) |
            ((x & 0x00FF0000) >> 8) |
            ((x & 0x0000FF00) << 8) |
            ((x & 0x000000FF) << 24));
}

#define swapl(x) do { \
		if (sizeof(*(x)) != 4) \
			wrong_size(); \
		*(x) = bswap_32(*(x)); \
	} while (0)

static inline uint16_t
bswap_16(uint16_t x)
{
    return (((x & 0xFF00) >> 8) |
            ((x & 0x00FF) << 8));
}

#define swaps(x) do { \
		if (sizeof(*(x)) != 2) \
			wrong_size(); \
		*(x) = bswap_16(*(x)); \
	} while (0)

/* copy 32-bit value from src to dst byteswapping on the way */
#define cpswapl(src, dst) do { \
		if (sizeof((src)) != 4 || sizeof((dst)) != 4) \
			wrong_size(); \
		(dst) = bswap_32((src)); \
	} while (0)

/* copy short from src to dst byteswapping on the way */
#define cpswaps(src, dst) do { \
		if (sizeof((src)) != 2 || sizeof((dst)) != 2) \
			wrong_size(); \
		(dst) = bswap_16((src)); \
	} while (0)

extern _X_EXPORT void SwapShorts(short *list, unsigned long count);

typedef struct _xPoint *DDXPointPtr;
typedef struct pixman_box16 *BoxPtr;
typedef struct _xEvent *xEventPtr;
typedef struct _xRectangle *xRectanglePtr;
typedef struct _GrabRec *GrabPtr;

typedef unsigned long x_server_generation_t;

extern _X_EXPORT unsigned long globalSerialNumber;
extern _X_EXPORT x_server_generation_t serverGeneration;

#endif                          /* MISC_H */

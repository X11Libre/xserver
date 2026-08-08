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

#ifndef CLIENT_IO_H
#define CLIENT_IO_H

#include <X11/Xfuncproto.h>

#include "xlibre_ptrtypes.h"

extern _X_EXPORT int ReadFdFromClient(ClientPtr client);

/**
 * @brief write @p count bytes from @p buf into the client's output buffer
 *
 * @deprecated Legacy entry point, kept for ABI compatibility. Drivers and
 *             external modules should not write to clients directly; this
 *             remains exported only for existing out-of-tree users. In-tree
 *             code uses the internal dixWriteToClient() worker instead.
 *
 * @param who    the client to write to
 * @param count  number of bytes to write
 * @param buf    data to write
 * @return       number of bytes buffered, 0 on no-op, -1 on error
 */
extern _X_EXPORT int WriteToClient(ClientPtr /*who */ , int /*count */ ,
                                   const void * /*buf */ );

extern _X_EXPORT void IgnoreClient(ClientPtr /*client */ );

extern _X_EXPORT void AttendClient(ClientPtr /*client */ );

extern _X_EXPORT int GetClientFd(ClientPtr);

/* Signal handling */
typedef int (*OsSigWrapperPtr) (int /* sig */ );

extern _X_EXPORT OsSigWrapperPtr
OsRegisterSigWrapper(OsSigWrapperPtr newWrap);

extern _X_EXPORT Bool
PrivsElevated(void);

/* stuff for FlushCallback */
extern _X_EXPORT CallbackListPtr FlushCallback;

extern _X_EXPORT int
TimeSinceLastInputEvent(void);

#endif /* CLIENT_IO_H */
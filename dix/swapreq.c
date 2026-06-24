/************************************************************

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

********************************************************/

#include <dix-config.h>

#include <X11/X.h>
#include <X11/Xproto.h>
#include <X11/Xprotostr.h>

#include "dix/dix_priv.h"
#include "dix/reqhandlers_priv.h"
#include "include/misc.h"

#include "dixstruct.h"
#include "extnsionst.h"         /* for SendEvent */
#include "swapreq.h"

/* Thanks to Jack Palevich for testing and subsequently rewriting all this */

/* Byte swap a list of shorts */
void
SwapShorts(short *list, unsigned long count)
{
    while (count >= 16) {
        swaps(list + 0);
        swaps(list + 1);
        swaps(list + 2);
        swaps(list + 3);
        swaps(list + 4);
        swaps(list + 5);
        swaps(list + 6);
        swaps(list + 7);
        swaps(list + 8);
        swaps(list + 9);
        swaps(list + 10);
        swaps(list + 11);
        swaps(list + 12);
        swaps(list + 13);
        swaps(list + 14);
        swaps(list + 15);
        list += 16;
        count -= 16;
    }
    if (count != 0) {
        do {
            swaps(list);
            list++;
        } while (--count != 0);
    }
}

void _X_COLD
SwapConnClientPrefix(xConnClientPrefix * pCCP)
{
    swaps(&pCCP->majorVersion);
    swaps(&pCCP->minorVersion);
    swaps(&pCCP->nbytesAuthProto);
    swaps(&pCCP->nbytesAuthString);
}

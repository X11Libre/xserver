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

int _X_COLD
SProcCreateColormap(ClientPtr client)
{
    REQUEST(xCreateColormapReq);
    REQUEST_SIZE_MATCH(xCreateColormapReq);
    swapl(&stuff->mid);
    swapl(&stuff->window);
    swapl(&stuff->visual);
    return ((*ProcVector[X_CreateColormap]) (client));
}

int _X_COLD
SProcCopyColormapAndFree(ClientPtr client)
{
    REQUEST(xCopyColormapAndFreeReq);
    REQUEST_SIZE_MATCH(xCopyColormapAndFreeReq);
    swapl(&stuff->mid);
    swapl(&stuff->srcCmap);
    return ((*ProcVector[X_CopyColormapAndFree]) (client));
}

int _X_COLD
SProcAllocNamedColor(ClientPtr client)
{
    REQUEST(xAllocNamedColorReq);
    REQUEST_AT_LEAST_SIZE(xAllocNamedColorReq);
    swapl(&stuff->cmap);
    swaps(&stuff->nbytes);
    return ((*ProcVector[X_AllocNamedColor]) (client));
}

int _X_COLD
SProcAllocColorCells(ClientPtr client)
{
    REQUEST(xAllocColorCellsReq);
    REQUEST_SIZE_MATCH(xAllocColorCellsReq);
    swapl(&stuff->cmap);
    swaps(&stuff->colors);
    swaps(&stuff->planes);
    return ((*ProcVector[X_AllocColorCells]) (client));
}

int _X_COLD
SProcAllocColorPlanes(ClientPtr client)
{
    REQUEST(xAllocColorPlanesReq);
    REQUEST_SIZE_MATCH(xAllocColorPlanesReq);
    swapl(&stuff->cmap);
    swaps(&stuff->colors);
    swaps(&stuff->red);
    swaps(&stuff->green);
    swaps(&stuff->blue);
    return ((*ProcVector[X_AllocColorPlanes]) (client));
}

int _X_COLD
SProcFreeColors(ClientPtr client)
{
    REQUEST(xFreeColorsReq);
    REQUEST_AT_LEAST_SIZE(xFreeColorsReq);
    swapl(&stuff->cmap);
    swapl(&stuff->planeMask);
    SwapRestL(stuff);
    return ((*ProcVector[X_FreeColors]) (client));
}

void _X_COLD
SwapColorItem(xColorItem * pItem)
{
    swapl(&pItem->pixel);
    swaps(&pItem->red);
    swaps(&pItem->green);
    swaps(&pItem->blue);
}

int _X_COLD
SProcStoreColors(ClientPtr client)
{
    xColorItem *pItem;

    REQUEST(xStoreColorsReq);
    REQUEST_AT_LEAST_SIZE(xStoreColorsReq);
    swapl(&stuff->cmap);
    pItem = (xColorItem *) &stuff[1];
    for (long count = ((client->req_len << 2) - sizeof(xStoreColorsReq)) / sizeof(xColorItem); --count >= 0;)
        SwapColorItem(pItem++);
    return ((*ProcVector[X_StoreColors]) (client));
}

int _X_COLD
SProcStoreNamedColor(ClientPtr client)
{
    REQUEST(xStoreNamedColorReq);
    REQUEST_AT_LEAST_SIZE(xStoreNamedColorReq);
    swapl(&stuff->cmap);
    swapl(&stuff->pixel);
    swaps(&stuff->nbytes);
    return ((*ProcVector[X_StoreNamedColor]) (client));
}

int _X_COLD
SProcCreateCursor(ClientPtr client)
{
    REQUEST(xCreateCursorReq);
    REQUEST_SIZE_MATCH(xCreateCursorReq);
    swapl(&stuff->cid);
    swapl(&stuff->source);
    swapl(&stuff->mask);
    swaps(&stuff->foreRed);
    swaps(&stuff->foreGreen);
    swaps(&stuff->foreBlue);
    swaps(&stuff->backRed);
    swaps(&stuff->backGreen);
    swaps(&stuff->backBlue);
    swaps(&stuff->x);
    swaps(&stuff->y);
    return ((*ProcVector[X_CreateCursor]) (client));
}

int _X_COLD
SProcRecolorCursor(ClientPtr client)
{
    REQUEST(xRecolorCursorReq);
    REQUEST_SIZE_MATCH(xRecolorCursorReq);
    swapl(&stuff->cursor);
    swaps(&stuff->foreRed);
    swaps(&stuff->foreGreen);
    swaps(&stuff->foreBlue);
    swaps(&stuff->backRed);
    swaps(&stuff->backGreen);
    swaps(&stuff->backBlue);
    return ((*ProcVector[X_RecolorCursor]) (client));
}

int _X_COLD
SProcQueryBestSize(ClientPtr client)
{
    REQUEST(xQueryBestSizeReq);
    REQUEST_SIZE_MATCH(xQueryBestSizeReq);
    swapl(&stuff->drawable);
    swaps(&stuff->width);
    swaps(&stuff->height);
    return ((*ProcVector[X_QueryBestSize]) (client));
}

int _X_COLD
SProcChangeKeyboardMapping(ClientPtr client)
{
    REQUEST(xChangeKeyboardMappingReq);
    REQUEST_AT_LEAST_SIZE(xChangeKeyboardMappingReq);
    SwapRestL(stuff);
    return ((*ProcVector[X_ChangeKeyboardMapping]) (client));
}

int _X_COLD
SProcChangeKeyboardControl(ClientPtr client)
{
    REQUEST(xChangeKeyboardControlReq);
    REQUEST_AT_LEAST_SIZE(xChangeKeyboardControlReq);
    swapl(&stuff->mask);
    SwapRestL(stuff);
    return ((*ProcVector[X_ChangeKeyboardControl]) (client));
}

int _X_COLD
SProcChangePointerControl(ClientPtr client)
{
    REQUEST(xChangePointerControlReq);
    REQUEST_SIZE_MATCH(xChangePointerControlReq);
    swaps(&stuff->accelNum);
    swaps(&stuff->accelDenum);
    swaps(&stuff->threshold);
    return ((*ProcVector[X_ChangePointerControl]) (client));
}

int _X_COLD
SProcChangeHosts(ClientPtr client)
{
    REQUEST(xChangeHostsReq);
    REQUEST_AT_LEAST_SIZE(xChangeHostsReq);
    swaps(&stuff->hostLength);
    return ((*ProcVector[X_ChangeHosts]) (client));
}

void _X_COLD
SwapConnClientPrefix(xConnClientPrefix * pCCP)
{
    swaps(&pCCP->majorVersion);
    swaps(&pCCP->minorVersion);
    swaps(&pCCP->nbytesAuthProto);
    swaps(&pCCP->nbytesAuthString);
}

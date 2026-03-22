/*
 * Copyright (c) 2006, Oracle and/or its affiliates.
 * Copyright 2010 Red Hat, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Copyright © 2002 Keith Packard
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Keith Packard not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Keith Packard makes no
 * representations about the suitability of this software for any purpose.  It
 * is provided "as is" without express or implied warranty.
 *
 * KEITH PACKARD DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL KEITH PACKARD BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#include <dix-config.h>

#include "dix/cursor_priv.h"
#include "dix/dix_priv.h"
#include "dix/input_priv.h"
#include "dix/request_priv.h"
#include "dix/rpcbuf_priv.h"
#include "dix/screen_hooks_priv.h"
#include "dix/screenint_priv.h"

#include "xfixesint.h"
#include "scrnintstr.h"
#include "cursorstr.h"
#include "servermd.h"
#include "mipointer.h"
#include "inputstr.h"
#include "windowstr.h"
#include "xace.h"
#include "list.h"
#include "xibarriers.h"

static RESTYPE CursorClientType;
static RESTYPE CursorHideCountType;
static RESTYPE CursorWindowType;

static DevPrivateKeyRec CursorScreenPrivateKeyRec;

#define CursorScreenPrivateKey (&CursorScreenPrivateKeyRec)

static void deleteCursorHideCountsForScreen(ScreenPtr pScreen);

#define VERIFY_CURSOR(pCursor, cursor, client, access)			\
    do {								\
	int err;							\
	err = dixLookupResourceByType((void **) &pCursor, cursor,	\
				      X11_RESTYPE_CURSOR, client, access);	\
	if (err != Success) {						\
	    client->errorValue = cursor;				\
	    return err;							\
	}								\
    } while (0)

/*
 * There is a global list of windows selecting for cursor events
 */

typedef struct _CursorEvent *CursorEventPtr;

typedef struct _CursorEvent {
    CursorEventPtr next;
    CARD32 eventMask;
    ClientPtr pClient;
    WindowPtr pWindow;
    XID clientResource;
} CursorEventRec;

static CursorEventPtr cursorEvents;

/*
 * Each screen has a list of clients which have requested
 * that the cursor be hid, and the number of times each
 * client has requested.
*/

typedef struct _CursorHideCountRec *CursorHideCountPtr;

typedef struct _CursorHideCountRec {
    CursorHideCountPtr pNext;
    ClientPtr pClient;
    ScreenPtr pScreen;
    int hideCount;
    XID resource;
} CursorHideCountRec;

/*
 * Wrap DisplayCursor to catch cursor change events
 */

typedef struct _CursorScreen {
    DisplayCursorProcPtr DisplayCursor;
    CursorHideCountPtr pCursorHideCounts;
} CursorScreenRec, *CursorScreenPtr;

#define GetCursorScreen(s) ((CursorScreenPtr)dixLookupPrivate(&(s)->devPrivates, CursorScreenPrivateKey))
#define Wrap(as,s,elt,func)	(((as)->elt = (s)->elt), (s)->elt = func)
#define Unwrap(as,s,elt,backup)	(((backup) = (s)->elt), (s)->elt = (as)->elt)

/* The cursor doesn't show up until the first XDefineCursor() */
Bool CursorVisible = FALSE;
Bool EnableCursor = TRUE;

static CursorPtr
CursorForDevice(DeviceIntPtr pDev)
{
    if (pDev && pDev->spriteInfo && pDev->spriteInfo->sprite) {
        if (pDev->spriteInfo->anim.pCursor)
            return pDev->spriteInfo->anim.pCursor;
        return pDev->spriteInfo->sprite->current;
    }

    return NULL;
}

static CursorPtr
CursorForClient(ClientPtr client)
{
    return CursorForDevice(PickPointer(client));
}

static Bool
CursorDisplayCursor(DeviceIntPtr pDev, ScreenPtr pScreen, CursorPtr pCursor)
{
    CursorScreenPtr cs = GetCursorScreen(pScreen);
    CursorPtr pOldCursor = CursorForDevice(pDev);
    Bool ret;
    DisplayCursorProcPtr backupProc;

    Unwrap(cs, pScreen, DisplayCursor, backupProc);

    CursorVisible = CursorVisible && EnableCursor;

    if (cs->pCursorHideCounts != NULL || !CursorVisible) {
        ret = (*pScreen->DisplayCursor) (pDev, pScreen, NullCursor);
    }
    else {
        ret = (*pScreen->DisplayCursor) (pDev, pScreen, pCursor);
    }

    if (pCursor != pOldCursor) {
        CursorEventPtr e;

        UpdateCurrentTimeIf();
        for (e = cursorEvents; e; e = e->next) {
            if ((e->eventMask & XFixesDisplayCursorNotifyMask)) {
                xXFixesCursorNotifyEvent ev = {
                    .type = XFixesEventBase + XFixesCursorNotify,
                    .subtype = XFixesDisplayCursorNotify,
                    .window = e->pWindow->drawable.id,
                    .cursorSerial = pCursor ? pCursor->serialNumber : 0,
                    .timestamp = currentTime.milliseconds,
                    .name = pCursor ? pCursor->name : None
                };
                WriteEventsToClient(e->pClient, 1, (xEvent *) &ev);
            }
        }
    }
    Wrap(cs, pScreen, DisplayCursor, backupProc);

    return ret;
}

static void CursorScreenClose(CallbackListPtr *pcbl, ScreenPtr pScreen, void *unused)
{
    CursorScreenPtr cs = GetCursorScreen(pScreen);

    _X_UNUSED DisplayCursorProcPtr display_proc;

    dixScreenUnhookClose(pScreen, CursorScreenClose);
    Unwrap(cs, pScreen, DisplayCursor, display_proc);
    deleteCursorHideCountsForScreen(pScreen);
}

#define CursorAllEvents (XFixesDisplayCursorNotifyMask)

static int
XFixesSelectCursorInput(ClientPtr pClient, WindowPtr pWindow, CARD32 eventMask)
{
    CursorEventPtr *prev, e;
    void *val;
    int rc;

    for (prev = &cursorEvents; (e = *prev); prev = &e->next) {
        if (e->pClient == pClient && e->pWindow == pWindow) {
            break;
        }
    }
    if (!eventMask) {
        if (e) {
            FreeResource(e->clientResource, 0);
        }
        return Success;
    }
    if (!e) {
        e = calloc(1, sizeof(CursorEventRec));
        if (!e)
            return BadAlloc;

        e->pClient = pClient;
        e->pWindow = pWindow;
        e->clientResource = FakeClientID(pClient->index);

        /*
         * Add a resource hanging from the window to
         * catch window destroy
         */
        rc = dixLookupResourceByType(&val, pWindow->drawable.id,
                                     CursorWindowType, serverClient,
                                     DixGetAttrAccess);
        if (rc != Success)
            if (!AddResource(pWindow->drawable.id, CursorWindowType,
                             (void *) pWindow)) {
                free(e);
                return BadAlloc;
            }

        if (!AddResource(e->clientResource, CursorClientType, (void *) e))
            return BadAlloc;

        *prev = e;
    }
    e->eventMask = eventMask;
    return Success;
}

int
ProcXFixesSelectCursorInput(ClientPtr client)
{
    X_REQUEST_HEAD_STRUCT(xXFixesSelectCursorInputReq);
    X_REQUEST_FIELD_CARD32(window);
    X_REQUEST_FIELD_CARD32(eventMask);

    WindowPtr pWin;
    int rc;

    rc = dixLookupWindow(&pWin, stuff->window, client, DixGetAttrAccess);
    if (rc != Success)
        return rc;
    if (stuff->eventMask & ~CursorAllEvents) {
        client->errorValue = stuff->eventMask;
        return BadValue;
    }
    return XFixesSelectCursorInput(client, pWin, stuff->eventMask);
}

static int
GetBit(unsigned char *line, int x)
{
    unsigned char mask;

    if (screenInfo.bitmapBitOrder == LSBFirst)
        mask = (1 << (x & 7));
    else
        mask = (0x80 >> (x & 7));
    /* XXX assumes byte order is host byte order */
    line += (x >> 3);
    if (*line & mask)
        return 1;
    return 0;
}

void _X_COLD
SXFixesCursorNotifyEvent(xXFixesCursorNotifyEvent * from,
                         xXFixesCursorNotifyEvent * to)
{
    to->type = from->type;
    cpswaps(from->sequenceNumber, to->sequenceNumber);
    cpswapl(from->window, to->window);
    cpswapl(from->cursorSerial, to->cursorSerial);
    cpswapl(from->timestamp, to->timestamp);
    cpswapl(from->name, to->name);
}

static void
CopyCursorToImage(CursorPtr pCursor, CARD32 *image)
{
    int width = pCursor->bits->width;
    int height = pCursor->bits->height;
    int npixels = width * height;

    if (pCursor->bits->argb)
        memcpy(image, pCursor->bits->argb, npixels * sizeof(CARD32));
    else
    {
        unsigned char *srcLine = pCursor->bits->source;
        unsigned char *mskLine = pCursor->bits->mask;
        int stride = BitmapBytePad(width);
        int x, y;
        CARD32 fg, bg;

        fg = (0xff000000 |
              ((pCursor->foreRed & 0xff00) << 8) |
              (pCursor->foreGreen & 0xff00) | (pCursor->foreBlue >> 8));
        bg = (0xff000000 |
              ((pCursor->backRed & 0xff00) << 8) |
              (pCursor->backGreen & 0xff00) | (pCursor->backBlue >> 8));
        for (y = 0; y < height; y++) {
            for (x = 0; x < width; x++) {
                if (GetBit(mskLine, x)) {
                    if (GetBit(srcLine, x))
                        *image++ = fg;
                    else
                        *image++ = bg;
                }
                else
                    *image++ = 0;
            }
            srcLine += stride;
            mskLine += stride;
        }
    }
}

int
ProcXFixesGetCursorImage(ClientPtr client)
{
    X_REQUEST_HEAD_STRUCT(xXFixesGetCursorImageReq);

    CursorPtr pCursor;
    int npixels, width, height, rc, x, y;

    pCursor = CursorForClient(client);
    if (!pCursor)
        return BadCursor;
    rc = XaceHookResourceAccess(client, pCursor->id, X11_RESTYPE_CURSOR,
                  pCursor, X11_RESTYPE_NONE, NULL, DixReadAccess);
    if (rc != Success)
        return rc;
    GetSpritePosition(PickPointer(client), &x, &y);
    width = pCursor->bits->width;
    height = pCursor->bits->height;
    npixels = width * height;

    x_rpcbuf_t rpcbuf = { .swapped = client->swapped, .err_clear = TRUE };

    CARD32 *image = x_rpcbuf_reserve(&rpcbuf, npixels * sizeof(CARD32));
    if (!image)
        return BadAlloc;

    CopyCursorToImage(pCursor, image);
    if (client->swapped)
        SwapLongs(image, npixels);

    xXFixesGetCursorImageReply reply = {
        .width = width,
        .height = height,
        .x = x,
        .y = y,
        .xhot = pCursor->bits->xhot,
        .yhot = pCursor->bits->yhot,
        .cursorSerial = pCursor->serialNumber,
    };

    if (client->swapped) {
        swaps(&reply.x);
        swaps(&reply.y);
        swaps(&reply.width);
        swaps(&reply.height);
        swaps(&reply.xhot);
        swaps(&reply.yhot);
        swapl(&reply.cursorSerial);
    }

    return X_SEND_REPLY_WITH_RPCBUF(client, reply, rpcbuf);
}

int
ProcXFixesSetCursorName(ClientPtr client)
{
    CursorPtr pCursor;
    char *tchar;
    Atom atom;

    X_REQUEST_HEAD_AT_LEAST(xXFixesSetCursorNameReq);
    X_REQUEST_FIELD_CARD32(cursor);
    X_REQUEST_FIELD_CARD16(nbytes);
    REQUEST_FIXED_SIZE(xXFixesSetCursorNameReq, stuff->nbytes);

    VERIFY_CURSOR(pCursor, stuff->cursor, client, DixSetAttrAccess);
    tchar = (char *) &stuff[1];
    atom = MakeAtom(tchar, stuff->nbytes, TRUE);
    if (atom == BAD_RESOURCE)
        return BadAlloc;

    pCursor->name = atom;
    return Success;
}

int
ProcXFixesGetCursorName(ClientPtr client)
{
    X_REQUEST_HEAD_STRUCT(xXFixesGetCursorNameReq);
    X_REQUEST_FIELD_CARD32(cursor);

    CursorPtr pCursor;
    const char *str;

    VERIFY_CURSOR(pCursor, stuff->cursor, client, DixGetAttrAccess);
    if (pCursor->name)
        str = NameForAtom(pCursor->name);
    else
        str = "";

    x_rpcbuf_t rpcbuf = { .swapped = client->swapped, .err_clear = TRUE };
    x_rpcbuf_write_string_pad(&rpcbuf, str);

    xXFixesGetCursorNameReply reply = {
        .atom = pCursor->name,
        .nbytes = strlen(str)
    };
    if (client->swapped) {
        swapl(&reply.atom);
        swaps(&reply.nbytes);
    }

    return X_SEND_REPLY_WITH_RPCBUF(client, reply, rpcbuf);
}

int
ProcXFixesGetCursorImageAndName(ClientPtr client)
{
    X_REQUEST_HEAD_STRUCT(xXFixesGetCursorImageAndNameReq);

    CursorPtr pCursor;
    int npixels;
    const char *name;
    int width, height;
    int rc, x, y;

    pCursor = CursorForClient(client);
    if (!pCursor)
        return BadCursor;
    rc = XaceHookResourceAccess(client, pCursor->id, X11_RESTYPE_CURSOR,
                  pCursor, X11_RESTYPE_NONE, NULL, DixReadAccess | DixGetAttrAccess);
    if (rc != Success)
        return rc;
    GetSpritePosition(PickPointer(client), &x, &y);
    width = pCursor->bits->width;
    height = pCursor->bits->height;
    npixels = width * height;
    name = pCursor->name ? NameForAtom(pCursor->name) : "";

    x_rpcbuf_t rpcbuf = { .swapped = client->swapped, .err_clear = TRUE };

    CARD32 *image = x_rpcbuf_reserve(&rpcbuf, npixels * sizeof(CARD32));
    if (!image)
        return BadAlloc;

    CopyCursorToImage(pCursor, image);
    if (client->swapped)
        SwapLongs(image, npixels);

    x_rpcbuf_write_string_pad(&rpcbuf, name);

    if (rpcbuf.error)
        return BadAlloc;

    xXFixesGetCursorImageAndNameReply reply = {
        .width = width,
        .height = height,
        .x = x,
        .y = y,
        .xhot = pCursor->bits->xhot,
        .yhot = pCursor->bits->yhot,
        .cursorSerial = pCursor->serialNumber,
        .cursorName = pCursor->name,
        .nbytes = strlen(name),
    };

    if (client->swapped) {
        swaps(&reply.x);
        swaps(&reply.y);
        swaps(&reply.width);
        swaps(&reply.height);
        swaps(&reply.xhot);
        swaps(&reply.yhot);
        swapl(&reply.cursorSerial);
        swapl(&reply.cursorName);
        swaps(&reply.nbytes);
    }

    return X_SEND_REPLY_WITH_RPCBUF(client, reply, rpcbuf);
}

/*
 * Find every cursor reference in the system, ask testCursor
 * whether it should be replaced with a reference to pCursor.
 */

typedef Bool (*TestCursorFunc) (CursorPtr pOld, void *closure);

typedef struct {
    RESTYPE type;
    TestCursorFunc testCursor;
    CursorPtr pNew;
    void *closure;
} ReplaceCursorLookupRec, *ReplaceCursorLookupPtr;

static const RESTYPE CursorRestypes[] = {
    X11_RESTYPE_WINDOW,
    X11_RESTYPE_PASSIVEGRAB,
    X11_RESTYPE_CURSOR
};

static Bool
ReplaceCursorLookup(void *value, XID id, void *closure)
{
    ReplaceCursorLookupPtr rcl = (ReplaceCursorLookupPtr) closure;
    WindowPtr pWin;
    GrabPtr pGrab;
    CursorPtr pCursor = 0, *pCursorRef = 0;
    XID cursor = 0;

    switch (rcl->type) {
    case X11_RESTYPE_WINDOW:
        pWin = (WindowPtr) value;
        if (pWin->optional) {
            pCursorRef = &pWin->optional->cursor;
            pCursor = *pCursorRef;
        }
        break;
    case X11_RESTYPE_PASSIVEGRAB:
        pGrab = (GrabPtr) value;
        pCursorRef = &pGrab->cursor;
        pCursor = *pCursorRef;
        break;
    case X11_RESTYPE_CURSOR:
        pCursorRef = 0;
        pCursor = (CursorPtr) value;
        cursor = id;
        break;
    }
    if (pCursor && pCursor != rcl->pNew) {
        if ((*rcl->testCursor) (pCursor, rcl->closure)) {
            CursorPtr curs = RefCursor(rcl->pNew);
            /* either redirect reference or update resource database */
            if (pCursorRef)
                *pCursorRef = curs;
            else
                ChangeResourceValue(id, X11_RESTYPE_CURSOR, curs);
            FreeCursor(pCursor, cursor);
        }
    }
    return FALSE;               /* keep walking */
}

static void
ReplaceCursor(CursorPtr pCursor, TestCursorFunc testCursor, void *closure)
{
    int clientIndex;
    int resIndex;
    ReplaceCursorLookupRec rcl;

    /*
     * Cursors exist only in the resource database, windows and grabs.
     * All of these are always pointed at by the resource database.  Walk
     * the whole thing looking for cursors
     */
    rcl.testCursor = testCursor;
    rcl.pNew = pCursor;
    rcl.closure = closure;

    /* for each client */
    for (clientIndex = 0; clientIndex < currentMaxClients; clientIndex++) {
        if (!clients[clientIndex])
            continue;
        for (resIndex = 0; resIndex < ARRAY_SIZE(CursorRestypes); resIndex++) {
            rcl.type = CursorRestypes[resIndex];
            /*
             * This function walks the entire client resource database
             */
            LookupClientResourceComplex(clients[clientIndex],
                                        rcl.type,
                                        ReplaceCursorLookup, (void *) &rcl);
        }
    }
    /* this "knows" that WindowHasNewCursor doesn't depend on its argument */
    WindowHasNewCursor(dixGetMasterScreen()->root);
}

static Bool
TestForCursor(CursorPtr pCursor, void *closure)
{
    return (pCursor == (CursorPtr) closure);
}

int
ProcXFixesChangeCursor(ClientPtr client)
{
    CursorPtr pSource, pDestination;

    X_REQUEST_HEAD_STRUCT(xXFixesChangeCursorReq);
    X_REQUEST_FIELD_CARD32(source);
    X_REQUEST_FIELD_CARD32(destination);

    VERIFY_CURSOR(pSource, stuff->source, client,
                  DixReadAccess | DixGetAttrAccess);
    VERIFY_CURSOR(pDestination, stuff->destination, client,
                  DixWriteAccess | DixSetAttrAccess);

    ReplaceCursor(pSource, TestForCursor, (void *) pDestination);
    return Success;
}

static Bool
TestForCursorName(CursorPtr pCursor, void *closure)
{
    Atom *pName = closure;

    return pCursor->name == *pName;
}

int
ProcXFixesChangeCursorByName(ClientPtr client)
{
    X_REQUEST_HEAD_AT_LEAST(xXFixesChangeCursorByNameReq);
    X_REQUEST_FIELD_CARD32(source);
    X_REQUEST_FIELD_CARD16(nbytes);
    REQUEST_FIXED_SIZE(xXFixesChangeCursorByNameReq, stuff->nbytes);

    CursorPtr pSource;
    Atom name;
    char *tchar;

    VERIFY_CURSOR(pSource, stuff->source, client,
                  DixReadAccess | DixGetAttrAccess);
    tchar = (char *) &stuff[1];
    name = MakeAtom(tchar, stuff->nbytes, FALSE);
    if (name)
        ReplaceCursor(pSource, TestForCursorName, &name);
    return Success;
}

/*
 * Routines for manipulating the per-screen hide counts list.
 * This list indicates which clients have requested cursor hiding
 * for that screen.
 */

/* Return the screen's hide-counts list element for the given client */
static CursorHideCountPtr
findCursorHideCount(ClientPtr pClient, ScreenPtr pScreen)
{
    CursorScreenPtr cs = GetCursorScreen(pScreen);
    CursorHideCountPtr pChc;

    for (pChc = cs->pCursorHideCounts; pChc != NULL; pChc = pChc->pNext) {
        if (pChc->pClient == pClient) {
            return pChc;
        }
    }

    return NULL;
}

static int
createCursorHideCount(ClientPtr pClient, ScreenPtr pScreen)
{
    CursorScreenPtr cs = GetCursorScreen(pScreen);
    CursorHideCountPtr pChc = calloc(1, sizeof(CursorHideCountRec));
    if (pChc == NULL) {
        return BadAlloc;
    }
    pChc->pClient = pClient;
    pChc->pScreen = pScreen;
    pChc->hideCount = 1;
    pChc->resource = FakeClientID(pClient->index);
    pChc->pNext = cs->pCursorHideCounts;
    cs->pCursorHideCounts = pChc;

    /*
     * Create a resource for this element so it can be deleted
     * when the client goes away.
     */
    if (!AddResource(pChc->resource, CursorHideCountType, (void *) pChc))
        return BadAlloc;

    return Success;
}

/*
 * Delete the given hide-counts list element from its screen list.
 */
static void
deleteCursorHideCount(CursorHideCountPtr pChcToDel, ScreenPtr pScreen)
{
    CursorScreenPtr cs = GetCursorScreen(pScreen);
    CursorHideCountPtr pChc, pNext;
    CursorHideCountPtr pChcLast = NULL;

    pChc = cs->pCursorHideCounts;
    while (pChc != NULL) {
        pNext = pChc->pNext;
        if (pChc == pChcToDel) {
            free(pChc);
            if (pChcLast == NULL) {
                cs->pCursorHideCounts = pNext;
            }
            else {
                pChcLast->pNext = pNext;
            }
            return;
        }
        pChcLast = pChc;
        pChc = pNext;
    }
}

/*
 * Delete all the hide-counts list elements for this screen.
 */
static void
deleteCursorHideCountsForScreen(ScreenPtr pScreen)
{
    CursorScreenPtr cs = GetCursorScreen(pScreen);
    CursorHideCountPtr pChc, pTmp;

    pChc = cs->pCursorHideCounts;
    while (pChc != NULL) {
        pTmp = pChc->pNext;
        FreeResource(pChc->resource, 0);
        pChc = pTmp;
    }
    cs->pCursorHideCounts = NULL;
}

int
ProcXFixesHideCursor(ClientPtr client)
{
    X_REQUEST_HEAD_STRUCT(xXFixesHideCursorReq);
    X_REQUEST_FIELD_CARD32(window);

    WindowPtr pWin;
    CursorHideCountPtr pChc;
    int ret;

    ret = dixLookupResourceByType((void **) &pWin, stuff->window, X11_RESTYPE_WINDOW,
                                  client, DixGetAttrAccess);
    if (ret != Success) {
        client->errorValue = stuff->window;
        return ret;
    }

    /*
     * Has client hidden the cursor before on this screen?
     * If so, just increment the count.
     */

    pChc = findCursorHideCount(client, pWin->drawable.pScreen);
    if (pChc != NULL) {
        pChc->hideCount++;
        return Success;
    }

    /*
     * This is the first time this client has hid the cursor
     * for this screen.
     */
    ret = dixCallScreenAccessCallback(client, pWin->drawable.pScreen, DixHideAccess);
    if (ret != Success)
        return ret;

    ret = createCursorHideCount(client, pWin->drawable.pScreen);

    if (ret == Success) {
        DeviceIntPtr dev;

        for (dev = inputInfo.devices; dev; dev = dev->next) {
            if (InputDevIsMaster(dev) && IsPointerDevice(dev))
                CursorDisplayCursor(dev, pWin->drawable.pScreen,
                                    CursorForDevice(dev));
        }
    }

    return ret;
}

int
ProcXFixesShowCursor(ClientPtr client)
{
    X_REQUEST_HEAD_STRUCT(xXFixesShowCursorReq);
    X_REQUEST_FIELD_CARD32(window);

    WindowPtr pWin;
    CursorHideCountPtr pChc;
    int rc;

    rc = dixLookupResourceByType((void **) &pWin, stuff->window, X11_RESTYPE_WINDOW,
                                 client, DixGetAttrAccess);
    if (rc != Success) {
        client->errorValue = stuff->window;
        return rc;
    }

    /*
     * Has client hidden the cursor on this screen?
     * If not, generate an error.
     */
    pChc = findCursorHideCount(client, pWin->drawable.pScreen);
    if (pChc == NULL) {
        return BadMatch;
    }

    rc = dixCallScreenAccessCallback(client, pWin->drawable.pScreen, DixShowAccess);
    if (rc != Success)
        return rc;

    pChc->hideCount--;
    if (pChc->hideCount <= 0) {
        FreeResource(pChc->resource, 0);
    }

    return Success;
}

static int
CursorFreeClient(void *data, XID id)
{
    CursorEventPtr old = (CursorEventPtr) data;
    CursorEventPtr *prev, e;

    for (prev = &cursorEvents; (e = *prev); prev = &e->next) {
        if (e == old) {
            *prev = e->next;
            free(e);
            break;
        }
    }
    return 1;
}

static int
CursorFreeHideCount(void *data, XID id)
{
    CursorHideCountPtr pChc = (CursorHideCountPtr) data;
    ScreenPtr pScreen = pChc->pScreen;
    DeviceIntPtr dev;

    deleteCursorHideCount(pChc, pChc->pScreen);
    for (dev = inputInfo.devices; dev; dev = dev->next) {
        if (InputDevIsMaster(dev) && IsPointerDevice(dev))
            CursorDisplayCursor(dev, pScreen, CursorForDevice(dev));
    }

    return 1;
}

static int
CursorFreeWindow(void *data, XID id)
{
    WindowPtr pWindow = (WindowPtr) data;
    CursorEventPtr e, next;

    for (e = cursorEvents; e; e = next) {
        next = e->next;
        if (e->pWindow == pWindow) {
            FreeResource(e->clientResource, 0);
        }
    }
    return 1;
}

int
ProcXFixesCreatePointerBarrier(ClientPtr client)
{
    X_REQUEST_HEAD_AT_LEAST(xXFixesCreatePointerBarrierReq);

    X_REQUEST_FIELD_CARD16(num_devices);
    X_REQUEST_FIELD_CARD32(barrier);
    X_REQUEST_FIELD_CARD32(window);
    X_REQUEST_FIELD_CARD16(x1);
    X_REQUEST_FIELD_CARD16(y1);
    X_REQUEST_FIELD_CARD16(x2);
    X_REQUEST_FIELD_CARD16(y2);
    X_REQUEST_FIELD_CARD32(directions);
    X_REQUEST_REST_COUNT_CARD16(stuff->num_devices);

    LEGAL_NEW_RESOURCE(stuff->barrier, client);

    return XICreatePointerBarrier(client, stuff);
}

int
ProcXFixesDestroyPointerBarrier(ClientPtr client)
{
    X_REQUEST_HEAD_STRUCT(xXFixesDestroyPointerBarrierReq);
    X_REQUEST_FIELD_CARD32(barrier);
    return XIDestroyPointerBarrier(client, stuff);
}

Bool
XFixesCursorInit(void)
{
    if (party_like_its_1989)
        CursorVisible = EnableCursor;
    else
        CursorVisible = FALSE;

    if (!dixRegisterPrivateKey(&CursorScreenPrivateKeyRec, PRIVATE_SCREEN, sizeof(CursorScreenRec)))
        return FALSE;

    DIX_FOR_EACH_SCREEN({
        CursorScreenPtr cs = GetCursorScreen(walkScreen);
        dixScreenHookClose(walkScreen, CursorScreenClose);
        Wrap(cs, walkScreen, DisplayCursor, CursorDisplayCursor);
        cs->pCursorHideCounts = NULL;
    });

    CursorClientType = CreateNewResourceType(CursorFreeClient,
                                             "XFixesCursorClient");
    CursorHideCountType = CreateNewResourceType(CursorFreeHideCount,
                                                "XFixesCursorHideCount");
    CursorWindowType = CreateNewResourceType(CursorFreeWindow,
                                             "XFixesCursorWindow");

    return CursorClientType && CursorHideCountType && CursorWindowType;
}

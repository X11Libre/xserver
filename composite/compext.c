/*
 * Copyright (c) 2006, Oracle and/or its affiliates.
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
 * Copyright © 2003 Keith Packard
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

#include "dix/dix_priv.h"
#include "dix/request_priv.h"
#include "miext/extinit_priv.h"
#include "Xext/panoramiXsrv.h"

#include "compint.h"
#include "xace.h"
#include "protocol-versions.h"

static CARD8 CompositeReqCode;
static DevPrivateKeyRec CompositeClientPrivateKeyRec;

static int compositeUseXinerama = 0;

#define CompositeClientPrivateKey (&CompositeClientPrivateKeyRec)
RESTYPE CompositeClientWindowType;
RESTYPE CompositeClientSubwindowsType;
RESTYPE CompositeClientOverlayType;

typedef struct _CompositeClient {
    int major_version;
    int minor_version;
} CompositeClientRec, *CompositeClientPtr;

#define GetCompositeClient(pClient) ((CompositeClientPtr) \
    dixLookupPrivate(&(pClient)->devPrivates, CompositeClientPrivateKey))

static int
FreeCompositeClientWindow(void *value, XID ccwid)
{
    WindowPtr pWin = value;

    compFreeClientWindow(pWin, ccwid);
    return Success;
}

static int
FreeCompositeClientSubwindows(void *value, XID ccwid)
{
    WindowPtr pWin = value;

    compFreeClientSubwindows(pWin, ccwid);
    return Success;
}

static int
FreeCompositeClientOverlay(void *value, XID ccwid)
{
    CompOverlayClientPtr pOc = (CompOverlayClientPtr) value;

    compFreeOverlayClient(pOc);
    return Success;
}

static int
ProcCompositeQueryVersion(ClientPtr client)
{
    REQUEST_HEAD_STRUCT(xCompositeQueryVersionReq);
    REQUEST_FIELD_CARD32(majorVersion);
    REQUEST_FIELD_CARD32(minorVersion);

    CompositeClientPtr pCompositeClient = GetCompositeClient(client);

    xCompositeQueryVersionReply rep = { 0 };

    if (stuff->majorVersion < SERVER_COMPOSITE_MAJOR_VERSION) {
        rep.majorVersion = stuff->majorVersion;
        rep.minorVersion = stuff->minorVersion;
    }
    else {
        rep.majorVersion = SERVER_COMPOSITE_MAJOR_VERSION;
        rep.minorVersion = SERVER_COMPOSITE_MINOR_VERSION;
    }
    pCompositeClient->major_version = rep.majorVersion;
    pCompositeClient->minor_version = rep.minorVersion;

    REPLY_FIELD_CARD32(majorVersion);
    REPLY_FIELD_CARD32(minorVersion);
    REPLY_SEND();
    return Success;
}

#define VERIFY_WINDOW(pWindow, wid, client, mode)			\
    do {								\
	int err;							\
	err = dixLookupResourceByType((void **) &pWindow, wid,	\
				      X11_RESTYPE_WINDOW, client, mode);\
	if (err != Success) {						\
	    client->errorValue = wid;					\
	    return err;							\
	}								\
    } while (0)

static int
SingleCompositeRedirectWindow(ClientPtr client, xCompositeRedirectWindowReq *stuff)
{
    WindowPtr pWin;

    VERIFY_WINDOW(pWin, stuff->window, client,
                  DixSetAttrAccess | DixManageAccess | DixBlendAccess);

    return compRedirectWindow(client, pWin, stuff->update);
}

static int
SingleRedirectSubwindows(ClientPtr client, xCompositeRedirectSubwindowsReq *stuff)
{
    WindowPtr pWin;

    VERIFY_WINDOW(pWin, stuff->window, client,
                  DixSetAttrAccess | DixManageAccess | DixBlendAccess);

    return compRedirectSubwindows(client, pWin, stuff->update);
}

static int
SingleCompositeUnredirectWindow(ClientPtr client, xCompositeUnredirectWindowReq *stuff)
{
    WindowPtr pWin;

    VERIFY_WINDOW(pWin, stuff->window, client,
                  DixSetAttrAccess | DixManageAccess | DixBlendAccess);

    return compUnredirectWindow(client, pWin, stuff->update);
}

static int
SingleCompositeUnredirectSubwindows(ClientPtr client, xCompositeUnredirectSubwindowsReq* stuff)
{
    WindowPtr pWin;

    VERIFY_WINDOW(pWin, stuff->window, client,
                  DixSetAttrAccess | DixManageAccess | DixBlendAccess);

    return compUnredirectSubwindows(client, pWin, stuff->update);
}

static int
ProcCompositeCreateRegionFromBorderClip(ClientPtr client)
{
    REQUEST_HEAD_STRUCT(xCompositeCreateRegionFromBorderClipReq);
    REQUEST_FIELD_CARD32(region);
    REQUEST_FIELD_CARD32(window);

    WindowPtr pWin;

    VERIFY_WINDOW(pWin, stuff->window, client, DixGetAttrAccess);
    LEGAL_NEW_RESOURCE(stuff->region, client);

    CompWindowPtr cw = GetCompWindow(pWin);

    RegionPtr pBorderClip = (cw ? &cw->borderClip : &pWin->borderClip);

    RegionPtr pRegion = XFixesRegionCopy(pBorderClip);
    if (!pRegion)
        return BadAlloc;

    RegionTranslate(pRegion, -pWin->drawable.x, -pWin->drawable.y);

    if (!AddResource(stuff->region, RegionResType, (void *) pRegion))
        return BadAlloc;

    return Success;
}

static int
SingleCompositeNameWindowPixmap(ClientPtr client, xCompositeNameWindowPixmapReq *stuff)
{
    WindowPtr pWin;

    VERIFY_WINDOW(pWin, stuff->window, client, DixGetAttrAccess);

    ScreenPtr pScreen = pWin->drawable.pScreen;

    if (!pWin->viewable)
        return BadMatch;

    LEGAL_NEW_RESOURCE(stuff->pixmap, client);

    CompWindowPtr cw = GetCompWindow(pWin);
    if (!cw)
        return BadMatch;

    PixmapPtr pPixmap = pScreen->GetWindowPixmap(pWin);
    if (!pPixmap)
        return BadMatch;

    /* security creation/labeling check */
    int rc = XaceHookResourceAccess(client,
                                    stuff->pixmap,
                                    X11_RESTYPE_PIXMAP,
                                    pPixmap,
                                    X11_RESTYPE_WINDOW,
                                    pWin,
                                    DixCreateAccess);
    if (rc != Success)
        return rc;

    ++pPixmap->refcnt;

    if (!AddResource(stuff->pixmap, X11_RESTYPE_PIXMAP, (void *) pPixmap))
        return BadAlloc;

    if (pScreen->NameWindowPixmap) {
        rc = pScreen->NameWindowPixmap(pWin, pPixmap, stuff->pixmap);
        if (rc != Success) {
            FreeResource(stuff->pixmap, X11_RESTYPE_NONE);
            return rc;
        }
    }

    return Success;
}

static int
SingleCompositeGetOverlayWindow(ClientPtr client, xCompositeGetOverlayWindowReq *stuff)
{
    WindowPtr pWin;

    VERIFY_WINDOW(pWin, stuff->window, client, DixGetAttrAccess);
    ScreenPtr pScreen = pWin->drawable.pScreen;

    /*
     * Create an OverlayClient structure to mark this client's
     * interest in the overlay window
     */
    CompOverlayClientPtr pOc = compCreateOverlayClient(pScreen, client);
    if (pOc == NULL)
        return BadAlloc;

    /*
     * Make sure the overlay window exists
     */
    CompScreenPtr cs = GetCompScreen(pScreen);
    if (cs->pOverlayWin == NULL)
        if (!compCreateOverlayWindow(pScreen)) {
            FreeResource(pOc->resource, X11_RESTYPE_NONE);
            return BadAlloc;
        }

    int rc = XaceHookResourceAccess(client,
                                    cs->pOverlayWin->drawable.id,
                                    X11_RESTYPE_WINDOW,
                                    cs->pOverlayWin, X11_RESTYPE_NONE,
                                    NULL,
                                    DixGetAttrAccess);
    if (rc != Success) {
        FreeResource(pOc->resource, X11_RESTYPE_NONE);
        return rc;
    }

    xCompositeGetOverlayWindowReply rep = {
        .overlayWin = cs->pOverlayWin->drawable.id
    };

    REPLY_FIELD_CARD32(overlayWin);
    REPLY_SEND();
    return Success;
}

static int
SingleCompositeReleaseOverlayWindow(ClientPtr client, xCompositeReleaseOverlayWindowReq *stuff)
{
    WindowPtr pWin;

    VERIFY_WINDOW(pWin, stuff->window, client, DixGetAttrAccess);

    /*
     * Has client queried a reference to the overlay window
     * on this screen? If not, generate an error.
     */
    CompOverlayClientPtr pOc = compFindOverlayClient(pWin->drawable.pScreen, client);
    if (pOc == NULL)
        return BadMatch;

    /* The delete function will free the client structure */
    FreeResource(pOc->resource, X11_RESTYPE_NONE);

    return Success;
}

static int ProcCompositeRedirectWindow(ClientPtr client);
static int ProcCompositeRedirectSubwindows(ClientPtr client);
static int ProcCompositeUnredirectWindow(ClientPtr client);
static int ProcCompositeReleaseOverlayWindow(ClientPtr client);
static int ProcCompositeUnredirectSubwindows(ClientPtr client);
static int ProcCompositeNameWindowPixmap(ClientPtr client);
static int ProcCompositeGetOverlayWindow(ClientPtr client);

static int
ProcCompositeDispatch(ClientPtr client)
{
    REQUEST(xReq);
    switch (stuff->data) {
        case X_CompositeQueryVersion:
            return ProcCompositeQueryVersion(client);
        case X_CompositeRedirectWindow:
            return ProcCompositeRedirectWindow(client);
        case X_CompositeRedirectSubwindows:
            return ProcCompositeRedirectSubwindows(client);
        case X_CompositeUnredirectWindow:
            return ProcCompositeUnredirectWindow(client);
        case X_CompositeUnredirectSubwindows:
            return ProcCompositeUnredirectSubwindows(client);
        case X_CompositeCreateRegionFromBorderClip:
            return ProcCompositeCreateRegionFromBorderClip(client);
        case X_CompositeNameWindowPixmap:
            return ProcCompositeNameWindowPixmap(client);
        case X_CompositeGetOverlayWindow:
            return ProcCompositeGetOverlayWindow(client);
        case X_CompositeReleaseOverlayWindow:
            return ProcCompositeReleaseOverlayWindow(client);
        default:
            return BadRequest;
    }
}

/** @see GetDefaultBytes */
static SizeType coreGetWindowBytes;

static void
GetCompositeWindowBytes(void *value, XID id, ResourceSizePtr size)
{
    WindowPtr window = value;

    /* call down */
    coreGetWindowBytes(value, id, size);

    /* account for redirection */
    if (window->redirectDraw != RedirectDrawNone)
    {
        SizeType pixmapSizeFunc = GetResourceTypeSizeFunc(X11_RESTYPE_PIXMAP);
        ResourceSizeRec pixmapSize = { 0, 0 };
        ScreenPtr screen = window->drawable.pScreen;
        PixmapPtr pixmap = screen->GetWindowPixmap(window);
        pixmapSizeFunc(pixmap, pixmap->drawable.id, &pixmapSize);
        size->pixmapRefSize += pixmapSize.pixmapRefSize;
    }
}

void
CompositeExtensionInit(void)
{
    ExtensionEntry *extEntry;
    int s;

    /* Assume initialization is going to fail */
    noCompositeExtension = TRUE;

    for (s = 0; s < screenInfo.numScreens; s++) {
        ScreenPtr pScreen = screenInfo.screens[s];
        VisualPtr vis;

        /* Composite on 8bpp pseudocolor root windows appears to fail, so
         * just disable it on anything pseudocolor for safety.
         */
        for (vis = pScreen->visuals; vis->vid != pScreen->rootVisual; vis++);
        if ((vis->class | DynamicClass) == PseudoColor)
            return;

        /* Ensure that Render is initialized, which is required for automatic
         * compositing.
         */
        if (GetPictureScreenIfSet(pScreen) == NULL)
            return;
    }

    CompositeClientWindowType = CreateNewResourceType
        (FreeCompositeClientWindow, "CompositeClientWindow");
    if (!CompositeClientWindowType)
        return;

    coreGetWindowBytes = GetResourceTypeSizeFunc(X11_RESTYPE_WINDOW);
    SetResourceTypeSizeFunc(X11_RESTYPE_WINDOW, GetCompositeWindowBytes);

    CompositeClientSubwindowsType = CreateNewResourceType
        (FreeCompositeClientSubwindows, "CompositeClientSubwindows");
    if (!CompositeClientSubwindowsType)
        return;

    CompositeClientOverlayType = CreateNewResourceType
        (FreeCompositeClientOverlay, "CompositeClientOverlay");
    if (!CompositeClientOverlayType)
        return;

    if (!dixRegisterPrivateKey(&CompositeClientPrivateKeyRec, PRIVATE_CLIENT,
                               sizeof(CompositeClientRec)))
        return;

    for (s = 0; s < screenInfo.numScreens; s++)
        if (!compScreenInit(screenInfo.screens[s]))
            return;

    extEntry = AddExtension(COMPOSITE_NAME, 0, 0,
                            ProcCompositeDispatch, ProcCompositeDispatch,
                            NULL, StandardMinorOpcode);
    if (!extEntry)
        return;
    CompositeReqCode = (CARD8) extEntry->base;

    /* Initialization succeeded */
    noCompositeExtension = FALSE;
}

static int
ProcCompositeRedirectWindow(ClientPtr client)
{
    REQUEST_HEAD_STRUCT(xCompositeRedirectWindowReq);
    REQUEST_FIELD_CARD32(window);

#ifdef XINERAMA
    if (!compositeUseXinerama)
        return SingleCompositeRedirectWindow(client, stuff);

    PanoramiXRes *win;
    int rc = 0, j;

    if ((rc = dixLookupResourceByType((void **) &win, stuff->window, XRT_WINDOW,
                                      client, DixUnknownAccess))) {
        client->errorValue = stuff->window;
        return rc;
    }

    FOR_NSCREENS_FORWARD(j) {
        stuff->window = win->info[j].id;
        rc = SingleCompositeRedirectWindow(client, stuff);
        if (rc != Success)
            break;
    }

    return rc;
#else
    return SingleCompositeRedirectWindow(client, stuff);
#endif /* XINERAMA */
}

static int
ProcCompositeRedirectSubwindows(ClientPtr client)
{
    REQUEST_HEAD_STRUCT(xCompositeRedirectSubwindowsReq);
    REQUEST_FIELD_CARD32(window);

#ifdef XINERAMA
    if (!compositeUseXinerama)
        return SingleRedirectSubwindows(client, stuff);

    PanoramiXRes *win;
    int rc = 0, j;

    if ((rc = dixLookupResourceByType((void **) &win, stuff->window, XRT_WINDOW,
                                      client, DixUnknownAccess))) {
        client->errorValue = stuff->window;
        return rc;
    }

    FOR_NSCREENS_FORWARD(j) {
        stuff->window = win->info[j].id;
        rc = SingleRedirectSubwindows(client, stuff);
        if (rc != Success)
            break;
    }

    return rc;
#else
    return SingleRedirectSubwindows(client, stuff);
#endif /* XINERAMA */
}

static int
ProcCompositeUnredirectWindow(ClientPtr client)
{
    REQUEST_HEAD_STRUCT(xCompositeUnredirectWindowReq);
    REQUEST_FIELD_CARD32(window);

#ifdef XINERAMA
    if (!compositeUseXinerama)
        return SingleCompositeUnredirectWindow(client, stuff);

    PanoramiXRes *win;
    int rc = 0, j;

    if ((rc = dixLookupResourceByType((void **) &win, stuff->window, XRT_WINDOW,
                                      client, DixUnknownAccess))) {
        client->errorValue = stuff->window;
        return rc;
    }

    FOR_NSCREENS_FORWARD(j) {
        stuff->window = win->info[j].id;
        rc = SingleCompositeUnredirectWindow(client, stuff);
        if (rc != Success)
            break;
    }

    return rc;
#else
    return SingleCompositeUnredirectWindow(client, stuff);
#endif /* XINERAMA */
}

static int
ProcCompositeUnredirectSubwindows(ClientPtr client)
{
    REQUEST_HEAD_STRUCT(xCompositeUnredirectSubwindowsReq);
    REQUEST_FIELD_CARD32(window);

#ifdef XINERAMA
    if (!compositeUseXinerama)
        return SingleCompositeUnredirectSubwindows(client, stuff);

    PanoramiXRes *win;
    int rc = 0, j;

    if ((rc = dixLookupResourceByType((void **) &win, stuff->window, XRT_WINDOW,
                                      client, DixUnknownAccess))) {
        client->errorValue = stuff->window;
        return rc;
    }

    FOR_NSCREENS_FORWARD(j) {
        stuff->window = win->info[j].id;
        rc = SingleCompositeUnredirectSubwindows(client, stuff);
        if (rc != Success)
            break;
    }

    return rc;
#else
    return SingleCompositeUnredirectSubwindows(client, stuff);
#endif /* XINERAMA */
}

static int
ProcCompositeNameWindowPixmap(ClientPtr client)
{
    REQUEST_HEAD_STRUCT(xCompositeNameWindowPixmapReq);
    REQUEST_FIELD_CARD32(window);
    REQUEST_FIELD_CARD32(pixmap);

#ifdef XINERAMA
    if (!compositeUseXinerama)
        return SingleCompositeNameWindowPixmap(client, stuff);

    WindowPtr pWin;
    CompWindowPtr cw;
    PixmapPtr pPixmap;
    int rc;
    PanoramiXRes *win, *newPix;
    int i;

    if ((rc = dixLookupResourceByType((void **) &win, stuff->window, XRT_WINDOW,
                                      client, DixUnknownAccess))) {
        client->errorValue = stuff->window;
        return rc;
    }

    LEGAL_NEW_RESOURCE(stuff->pixmap, client);

    if (!(newPix = calloc(1, sizeof(PanoramiXRes))))
        return BadAlloc;

    newPix->type = XRT_PIXMAP;
    newPix->u.pix.shared = FALSE;
    panoramix_setup_ids(newPix, client, stuff->pixmap);

    FOR_NSCREENS_BACKWARD(i) {
        rc = dixLookupResourceByType((void **) &pWin, win->info[i].id,
                                     X11_RESTYPE_WINDOW, client,
                                     DixGetAttrAccess);
        if (rc != Success) {
            client->errorValue = stuff->window;
            free(newPix);
            return rc;
        }

        if (!pWin->viewable) {
            free(newPix);
            return BadMatch;
        }

        cw = GetCompWindow(pWin);
        if (!cw) {
            free(newPix);
            return BadMatch;
        }

        pPixmap = (*pWin->drawable.pScreen->GetWindowPixmap) (pWin);
        if (!pPixmap) {
            free(newPix);
            return BadMatch;
        }

        if (!AddResource(newPix->info[i].id, X11_RESTYPE_PIXMAP, (void *) pPixmap))
            return BadAlloc;

        ++pPixmap->refcnt;
    }

    if (!AddResource(stuff->pixmap, XRT_PIXMAP, (void *) newPix))
        return BadAlloc;

    return Success;
#else
    return SingleCompositeNameWindowPixmap(client, stuff);
#endif /* XINERAMA */
}

static int
ProcCompositeGetOverlayWindow(ClientPtr client)
{
    REQUEST_HEAD_STRUCT(xCompositeGetOverlayWindowReq);
    REQUEST_FIELD_CARD32(window);

#ifdef XINERAMA
    if (!compositeUseXinerama)
        return SingleCompositeGetOverlayWindow(client, stuff);

    WindowPtr pWin;
    ScreenPtr pScreen;
    CompScreenPtr cs;
    CompOverlayClientPtr pOc;
    int rc;
    PanoramiXRes *win, *overlayWin = NULL;
    int i;

    if ((rc = dixLookupResourceByType((void **) &win, stuff->window, XRT_WINDOW,
                                      client, DixUnknownAccess))) {
        client->errorValue = stuff->window;
        return rc;
    }

    cs = GetCompScreen(screenInfo.screens[0]);
    if (!cs->pOverlayWin) {
        if (!(overlayWin = calloc(1, sizeof(PanoramiXRes))))
            return BadAlloc;

        overlayWin->type = XRT_WINDOW;
        overlayWin->u.win.root = FALSE;
    }

    FOR_NSCREENS_BACKWARD(i) {
        rc = dixLookupResourceByType((void **) &pWin, win->info[i].id,
                                     X11_RESTYPE_WINDOW, client,
                                     DixGetAttrAccess);
        if (rc != Success) {
            client->errorValue = stuff->window;
            free(overlayWin);
            return rc;
        }
        pScreen = pWin->drawable.pScreen;

        /*
         * Create an OverlayClient structure to mark this client's
         * interest in the overlay window
         */
        pOc = compCreateOverlayClient(pScreen, client);
        if (pOc == NULL) {
            free(overlayWin);
            return BadAlloc;
        }

        /*
         * Make sure the overlay window exists
         */
        cs = GetCompScreen(pScreen);
        if (cs->pOverlayWin == NULL)
            if (!compCreateOverlayWindow(pScreen)) {
                FreeResource(pOc->resource, X11_RESTYPE_NONE);
                free(overlayWin);
                return BadAlloc;
            }

        rc = XaceHookResourceAccess(client,
                      cs->pOverlayWin->drawable.id,
                      X11_RESTYPE_WINDOW, cs->pOverlayWin, X11_RESTYPE_NONE, NULL,
                      DixGetAttrAccess);
        if (rc != Success) {
            FreeResource(pOc->resource, X11_RESTYPE_NONE);
            free(overlayWin);
            return rc;
        }
    }

    if (overlayWin) {
        FOR_NSCREENS_BACKWARD(i) {
            cs = GetCompScreen(screenInfo.screens[i]);
            overlayWin->info[i].id = cs->pOverlayWin->drawable.id;
        }

        AddResource(overlayWin->info[0].id, XRT_WINDOW, overlayWin);
    }

    cs = GetCompScreen(screenInfo.screens[0]);

    xCompositeGetOverlayWindowReply rep = {
        .overlayWin = cs->pOverlayWin->drawable.id
    };

    REPLY_FIELD_CARD32(overlayWin);
    REPLY_SEND();
    return Success;
#else
    return SingleCompositeGetOverlayWindow(client, stuff);
#endif /* XINERAMA */
}

static int
ProcCompositeReleaseOverlayWindow(ClientPtr client)
{
    REQUEST_HEAD_STRUCT(xCompositeReleaseOverlayWindowReq);
    REQUEST_FIELD_CARD32(window);

#ifdef XINERAMA
    if (!compositeUseXinerama)
        return SingleCompositeReleaseOverlayWindow(client, stuff);

    WindowPtr pWin;
    CompOverlayClientPtr pOc;
    PanoramiXRes *win;
    int i, rc;

    if ((rc = dixLookupResourceByType((void **) &win, stuff->window, XRT_WINDOW,
                                      client, DixUnknownAccess))) {
        client->errorValue = stuff->window;
        return rc;
    }

    FOR_NSCREENS_BACKWARD(i) {
        if ((rc = dixLookupResourceByType((void **) &pWin, win->info[i].id,
                                          XRT_WINDOW, client,
                                          DixUnknownAccess))) {
            client->errorValue = stuff->window;
            return rc;
        }

        /*
         * Has client queried a reference to the overlay window
         * on this screen? If not, generate an error.
         */
        pOc = compFindOverlayClient(pWin->drawable.pScreen, client);
        if (pOc == NULL)
            return BadMatch;

        /* The delete function will free the client structure */
        FreeResource(pOc->resource, X11_RESTYPE_NONE);
    }

    return Success;
#else
    return SingleCompositeReleaseOverlayWindow(client, stuff);
#endif /* XINERAMA */
}

#ifdef XINERAMA
void
PanoramiXCompositeInit(void)
{
    compositeUseXinerama = 1;
}

void
PanoramiXCompositeReset(void)
{
    compositeUseXinerama = 0;
}
#endif /* XINERAMA */

/*
 *
Copyright (c) 1992  X Consortium

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
X CONSORTIUM BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of the X Consortium shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from the X Consortium.
 *
 * Author:  Keith Packard, MIT X Consortium
 */

#include <dix-config.h>

#include <stdio.h>
#include <X11/X.h>
#include <X11/Xproto.h>
#include <X11/extensions/saverproto.h>

#include "dix/colormap_priv.h"
#include "dix/cursor_priv.h"
#include "dix/dix_priv.h"
#include "dix/dixgrabs_priv.h"
#include "dix/request_priv.h"
#include "dix/screensaver_priv.h"
#include "dix/window_priv.h"
#include "miext/extinit_priv.h"
#include "os/osdep.h"
#include "os/screensaver.h"
#include "Xext/panoramiX.h"
#include "Xext/panoramiXsrv.h"

#include "misc.h"
#include "os.h"
#include "windowstr.h"
#include "scrnintstr.h"
#include "pixmapstr.h"
#include "extnsionst.h"
#include "dixstruct.h"
#include "resource.h"
#include "gcstruct.h"
#include "cursorstr.h"
#include "xace.h"
#include "inputstr.h"
#ifdef DPMSExtension
#include <X11/extensions/dpmsconst.h>
#include "dpmsproc.h"
#endif
#include "protocol-versions.h"

Bool noScreenSaverExtension = FALSE;

static int ScreenSaverEventBase = 0;

static Bool ScreenSaverHandle(ScreenPtr pScreen, int xstate, Bool force);
static Bool CreateSaverWindow(ScreenPtr pScreen);
static Bool DestroySaverWindow(ScreenPtr pScreen);
static void UninstallSaverColormap(ScreenPtr pScreen);
static void CheckScreenPrivate(ScreenPtr pScreen);
static void SScreenSaverNotifyEvent(xScreenSaverNotifyEvent *from,
                                    xScreenSaverNotifyEvent *to);

static RESTYPE SuspendType;     /* resource type for suspension records */

typedef struct _ScreenSaverSuspension *ScreenSaverSuspensionPtr;

/* List of clients that are suspending the screensaver. */
static ScreenSaverSuspensionPtr suspendingClients = NULL;

/*
 * clientResource is a resource ID that's added when the record is
 * allocated, so the record is freed and the screensaver resumed when
 * the client disconnects. count is the number of times the client has
 * requested the screensaver be suspended.
 */
typedef struct _ScreenSaverSuspension {
    ScreenSaverSuspensionPtr next;
    ClientPtr pClient;
    XID clientResource;
    int count;
} ScreenSaverSuspensionRec;

static int ScreenSaverFreeSuspend(void *value, XID id);

/*
 * each screen has a list of clients requesting
 * ScreenSaverNotify events.  Each client has a resource
 * for each screen it selects ScreenSaverNotify input for,
 * this resource is used to delete the ScreenSaverNotifyRec
 * entry from the per-screen queue.
 */

static RESTYPE SaverEventType;  /* resource type for event masks */

typedef struct _ScreenSaverEvent *ScreenSaverEventPtr;

typedef struct _ScreenSaverEvent {
    ScreenSaverEventPtr next;
    ClientPtr client;
    ScreenPtr screen;
    XID resource;
    CARD32 mask;
} ScreenSaverEventRec;

static int ScreenSaverFreeEvents(void * value, XID id);

static Bool setEventMask(ScreenPtr      pScreen,
                         ClientPtr      client,
                         unsigned long  mask);

static unsigned long getEventMask(ScreenPtr     pScreen,
                                  ClientPtr     client);

/*
 * when a client sets the screen saver attributes, a resource is
 * kept to be freed when the client exits
 */

static RESTYPE AttrType;        /* resource type for attributes */

typedef struct _ScreenSaverAttr {
    ScreenPtr screen;
    ClientPtr client;
    XID resource;
    short x, y;
    unsigned short width, height, borderWidth;
    unsigned char class;
    unsigned char depth;
    VisualID visual;
    CursorPtr pCursor;
    PixmapPtr pBackgroundPixmap;
    PixmapPtr pBorderPixmap;
    Colormap colormap;
    unsigned long mask;         /* no pixmaps or cursors */
    unsigned long *values;
} ScreenSaverAttrRec, *ScreenSaverAttrPtr;

static int ScreenSaverFreeAttr(void *value, XID id);

static void FreeAttrs(ScreenSaverAttrPtr pAttr);

static void FreeScreenAttr(ScreenSaverAttrPtr pAttr);

static void
SendScreenSaverNotify(ScreenPtr pScreen,
                      int       state,
                      Bool      forced);

typedef struct _ScreenSaverScreenPrivate {
    ScreenSaverEventPtr events;
    ScreenSaverAttrPtr attr;
    Bool hasWindow;
    Colormap installedMap;
} ScreenSaverScreenPrivateRec, *ScreenSaverScreenPrivatePtr;

static ScreenSaverScreenPrivatePtr MakeScreenPrivate(ScreenPtr pScreen);

static DevPrivateKeyRec ScreenPrivateKeyRec;

#define ScreenPrivateKey (&ScreenPrivateKeyRec)

#define GetScreenPrivate(s) ((ScreenSaverScreenPrivatePtr) \
    dixLookupPrivate(&(s)->devPrivates, ScreenPrivateKey))
#define SetScreenPrivate(s,v) \
    dixSetPrivate(&(s)->devPrivates, ScreenPrivateKey, v);
#define SetupScreen(s)	ScreenSaverScreenPrivatePtr pPriv = (s ? GetScreenPrivate(s) : NULL)

static void
CheckScreenPrivate(ScreenPtr pScreen)
{
    SetupScreen(pScreen);

    if (!pPriv)
        return;
    if (!pPriv->attr && !pPriv->events &&
        !pPriv->hasWindow && pPriv->installedMap == None) {
        free(pPriv);
        SetScreenPrivate(pScreen, NULL);
        pScreen->screensaver.ExternalScreenSaver = NULL;
    }
}

static ScreenSaverScreenPrivatePtr
MakeScreenPrivate(ScreenPtr pScreen)
{
    SetupScreen(pScreen);

    if (pPriv)
        return pPriv;
    pPriv = calloc(1, sizeof(ScreenSaverScreenPrivateRec));
    if (!pPriv)
        return 0;
    pPriv->events = 0;
    pPriv->attr = 0;
    pPriv->hasWindow = FALSE;
    pPriv->installedMap = None;
    SetScreenPrivate(pScreen, pPriv);
    pScreen->screensaver.ExternalScreenSaver = ScreenSaverHandle;
    return pPriv;
}

static unsigned long
getEventMask(ScreenPtr pScreen, ClientPtr client)
{
    SetupScreen(pScreen);
    ScreenSaverEventPtr pEv;

    if (!pPriv)
        return 0;
    for (pEv = pPriv->events; pEv; pEv = pEv->next)
        if (pEv->client == client)
            return pEv->mask;
    return 0;
}

static Bool
setEventMask(ScreenPtr pScreen, ClientPtr client, unsigned long mask)
{
    SetupScreen(pScreen);
    ScreenSaverEventPtr pEv, *pPrev;

    if (getEventMask(pScreen, client) == mask)
        return TRUE;
    if (!pPriv) {
        pPriv = MakeScreenPrivate(pScreen);
        if (!pPriv)
            return FALSE;
    }
    for (pPrev = &pPriv->events; (pEv = *pPrev) != 0; pPrev = &pEv->next)
        if (pEv->client == client)
            break;
    if (mask == 0) {
        FreeResource(pEv->resource, SaverEventType);
        *pPrev = pEv->next;
        free(pEv);
        CheckScreenPrivate(pScreen);
    }
    else {
        if (!pEv) {
            pEv = calloc(1, sizeof(ScreenSaverEventRec));
            if (!pEv) {
                CheckScreenPrivate(pScreen);
                return FALSE;
            }
            *pPrev = pEv;
            pEv->next = NULL;
            pEv->client = client;
            pEv->screen = pScreen;
            pEv->resource = FakeClientID(client->index);
            if (!AddResource(pEv->resource, SaverEventType, (void *) pEv))
                return FALSE;
        }
        pEv->mask = mask;
    }
    return TRUE;
}

static void
FreeAttrs(ScreenSaverAttrPtr pAttr)
{
    CursorPtr pCursor;

    dixDestroyPixmap(pAttr->pBackgroundPixmap, 0);
    dixDestroyPixmap(pAttr->pBorderPixmap, 0);
    if ((pCursor = pAttr->pCursor) != 0)
        FreeCursor(pCursor, (Cursor) 0);
}

static void
FreeScreenAttr(ScreenSaverAttrPtr pAttr)
{
    FreeAttrs(pAttr);
    free(pAttr->values);
    free(pAttr);
}

static int
ScreenSaverFreeEvents(void *value, XID id)
{
    ScreenSaverEventPtr pOld = (ScreenSaverEventPtr) value;
    ScreenPtr pScreen = pOld->screen;

    SetupScreen(pScreen);
    ScreenSaverEventPtr pEv, *pPrev;

    if (!pPriv)
        return TRUE;
    for (pPrev = &pPriv->events; (pEv = *pPrev) != 0; pPrev = &pEv->next)
        if (pEv == pOld)
            break;
    if (!pEv)
        return TRUE;
    *pPrev = pEv->next;
    free(pEv);
    CheckScreenPrivate(pScreen);
    return TRUE;
}

static int
ScreenSaverFreeAttr(void *value, XID id)
{
    ScreenSaverAttrPtr pOldAttr = (ScreenSaverAttrPtr) value;
    ScreenPtr pScreen = pOldAttr->screen;

    SetupScreen(pScreen);

    if (!pPriv)
        return TRUE;
    if (pPriv->attr != pOldAttr)
        return TRUE;
    FreeScreenAttr(pOldAttr);
    pPriv->attr = NULL;
    if (pPriv->hasWindow) {
        dixSaveScreens(serverClient, SCREEN_SAVER_FORCER, ScreenSaverReset);
        dixSaveScreens(serverClient, SCREEN_SAVER_FORCER, ScreenSaverActive);
    }
    CheckScreenPrivate(pScreen);
    return TRUE;
}

static int
ScreenSaverFreeSuspend(void *value, XID id)
{
    ScreenSaverSuspensionPtr data = (ScreenSaverSuspensionPtr) value;
    ScreenSaverSuspensionPtr *prev, this;

    /* Unlink and free the suspension record for the client */
    for (prev = &suspendingClients; (this = *prev); prev = &this->next) {
        if (this == data) {
            *prev = this->next;
            free(this);
            break;
        }
    }

    /* Re-enable the screensaver if this was the last client suspending it. */
    if (screenSaverSuspended && suspendingClients == NULL) {
        screenSaverSuspended = FALSE;

        /* The screensaver could be active, since suspending it (by design)
           doesn't prevent it from being forceably activated */
#ifdef DPMSExtension
        if (screenIsSaved != SCREEN_SAVER_ON && DPMSPowerLevel == DPMSModeOn)
#else
        if (screenIsSaved != SCREEN_SAVER_ON)
#endif
        {
            DeviceIntPtr dev;
            UpdateCurrentTimeIf();
            nt_list_for_each_entry(dev, inputInfo.devices, next)
                NoticeTime(dev, currentTime);
            SetScreenSaverTimer();
        }
    }

    return Success;
}

static void
SendScreenSaverNotify(ScreenPtr pScreen, int state, Bool forced)
{
    ScreenSaverScreenPrivatePtr pPriv;
    ScreenSaverEventPtr pEv;
    unsigned long mask;
    int kind;

    UpdateCurrentTimeIf();
    mask = ScreenSaverNotifyMask;
    if (state == ScreenSaverCycle)
        mask = ScreenSaverCycleMask;
    pPriv = GetScreenPrivate(pScreen);
    if (!pPriv)
        return;
    if (pPriv->attr)
        kind = ScreenSaverExternal;
    else if (ScreenSaverBlanking != DontPreferBlanking)
        kind = ScreenSaverBlanked;
    else
        kind = ScreenSaverInternal;
    for (pEv = pPriv->events; pEv; pEv = pEv->next) {
        if (pEv->mask & mask) {
            xScreenSaverNotifyEvent ev = {
                .type = ScreenSaverNotify + ScreenSaverEventBase,
                .state = state,
                .timestamp = currentTime.milliseconds,
                .root = pScreen->root->drawable.id,
                .window = pScreen->screensaver.wid,
                .kind = kind,
                .forced = forced
            };
            WriteEventsToClient(pEv->client, 1, (xEvent *) &ev);
        }
    }
}

static void _X_COLD
SScreenSaverNotifyEvent(xScreenSaverNotifyEvent * from,
                        xScreenSaverNotifyEvent * to)
{
    to->type = from->type;
    to->state = from->state;
    cpswaps(from->sequenceNumber, to->sequenceNumber);
    cpswapl(from->timestamp, to->timestamp);
    cpswapl(from->root, to->root);
    cpswapl(from->window, to->window);
    to->kind = from->kind;
    to->forced = from->forced;
}

static void
UninstallSaverColormap(ScreenPtr pScreen)
{
    SetupScreen(pScreen);
    ColormapPtr pCmap;
    int rc;

    if (pPriv && pPriv->installedMap != None) {
        rc = dixLookupResourceByType((void **) &pCmap, pPriv->installedMap,
                                     X11_RESTYPE_COLORMAP, serverClient,
                                     DixUninstallAccess);
        if (rc == Success)
            (*pCmap->pScreen->UninstallColormap) (pCmap);
        pPriv->installedMap = None;
        CheckScreenPrivate(pScreen);
    }
}

static Bool
CreateSaverWindow(ScreenPtr pScreen)
{
    SetupScreen(pScreen);
    ScreenSaverStuffPtr pSaver;
    ScreenSaverAttrPtr pAttr;
    WindowPtr pWin;
    int result;
    unsigned long mask;
    Colormap wantMap;
    ColormapPtr pCmap;

    pSaver = &pScreen->screensaver;
    if (pSaver->pWindow) {
        pSaver->pWindow = NullWindow;
        FreeResource(pSaver->wid, X11_RESTYPE_NONE);
        if (pPriv) {
            UninstallSaverColormap(pScreen);
            pPriv->hasWindow = FALSE;
            CheckScreenPrivate(pScreen);
        }
    }

    if (!pPriv || !(pAttr = pPriv->attr))
        return FALSE;

    pPriv->installedMap = None;

    if (GrabInProgress && GrabInProgress != pAttr->client->index)
        return FALSE;

    pWin = dixCreateWindow(pSaver->wid, pScreen->root,
                        pAttr->x, pAttr->y, pAttr->width, pAttr->height,
                        pAttr->borderWidth, pAttr->class,
                        pAttr->mask, (XID *) pAttr->values,
                        pAttr->depth, serverClient, pAttr->visual, &result);
    if (!pWin)
        return FALSE;

    if (!AddResource(pWin->drawable.id, X11_RESTYPE_WINDOW, pWin))
        return FALSE;

    mask = 0;
    if (pAttr->pBackgroundPixmap) {
        pWin->backgroundState = BackgroundPixmap;
        pWin->background.pixmap = pAttr->pBackgroundPixmap;
        pAttr->pBackgroundPixmap->refcnt++;
        mask |= CWBackPixmap;
    }
    if (pAttr->pBorderPixmap) {
        pWin->borderIsPixel = FALSE;
        pWin->border.pixmap = pAttr->pBorderPixmap;
        pAttr->pBorderPixmap->refcnt++;
        mask |= CWBorderPixmap;
    }
    if (pAttr->pCursor) {
        if (!MakeWindowOptional(pWin)) {
            FreeResource(pWin->drawable.id, X11_RESTYPE_NONE);
            return FALSE;
        }
        CursorPtr cursor = RefCursor(pAttr->pCursor);
        if (pWin->optional->cursor)
            FreeCursor(pWin->optional->cursor, (Cursor) 0);
        pWin->optional->cursor = cursor;
        pWin->cursorIsNone = FALSE;
        CheckWindowOptionalNeed(pWin);
        mask |= CWCursor;
    }
    if (mask)
        (*pScreen->ChangeWindowAttributes) (pWin, mask);

    if (pAttr->colormap != None)
        (void) ChangeWindowAttributes(pWin, CWColormap, &pAttr->colormap,
                                      serverClient);

    MapWindow(pWin, serverClient);

    pPriv->hasWindow = TRUE;
    pSaver->pWindow = pWin;

    /* check and install our own colormap if it isn't installed now */
    wantMap = wColormap(pWin);
    if (wantMap == None || IsMapInstalled(wantMap, pWin))
        return TRUE;

    result = dixLookupResourceByType((void **) &pCmap, wantMap, X11_RESTYPE_COLORMAP,
                                     serverClient, DixInstallAccess);
    if (result != Success)
        return TRUE;

    pPriv->installedMap = wantMap;

    (*pCmap->pScreen->InstallColormap) (pCmap);

    return TRUE;
}

static Bool
DestroySaverWindow(ScreenPtr pScreen)
{
    SetupScreen(pScreen);
    ScreenSaverStuffPtr pSaver;

    if (!pPriv || !pPriv->hasWindow)
        return FALSE;

    pSaver = &pScreen->screensaver;
    if (pSaver->pWindow) {
        pSaver->pWindow = NullWindow;
        FreeResource(pSaver->wid, X11_RESTYPE_NONE);
    }
    pPriv->hasWindow = FALSE;
    CheckScreenPrivate(pScreen);
    UninstallSaverColormap(pScreen);
    return TRUE;
}

static Bool
ScreenSaverHandle(ScreenPtr pScreen, int xstate, Bool force)
{
    int state = 0;
    Bool ret = FALSE;
    ScreenSaverScreenPrivatePtr pPriv;

    switch (xstate) {
    case SCREEN_SAVER_ON:
        state = ScreenSaverOn;
        ret = CreateSaverWindow(pScreen);
        break;
    case SCREEN_SAVER_OFF:
        state = ScreenSaverOff;
        ret = DestroySaverWindow(pScreen);
        break;
    case SCREEN_SAVER_CYCLE:
        state = ScreenSaverCycle;
        pPriv = GetScreenPrivate(pScreen);
        if (pPriv && pPriv->hasWindow)
            ret = TRUE;

    }
#ifdef XINERAMA
    if (noPanoramiXExtension || !pScreen->myNum)
#endif /* XINERAMA */
        SendScreenSaverNotify(pScreen, state, force);
    return ret;
}

static int
ProcScreenSaverQueryVersion(ClientPtr client)
{
    X_REQUEST_HEAD_STRUCT(xScreenSaverQueryVersionReq);

    xScreenSaverQueryVersionReply reply = {
        .majorVersion = SERVER_SAVER_MAJOR_VERSION,
        .minorVersion = SERVER_SAVER_MINOR_VERSION
    };

    REPLY_FIELD_CARD16(majorVersion);
    REPLY_FIELD_CARD16(minorVersion);
    return X_SEND_REPLY_SIMPLE(client, reply);
}

static int
ProcScreenSaverQueryInfo(ClientPtr client)
{
    X_REQUEST_HEAD_STRUCT(xScreenSaverQueryInfoReq);
    X_REQUEST_FIELD_CARD32(drawable);

    int rc;
    ScreenSaverStuffPtr pSaver;
    DrawablePtr pDraw;
    CARD32 lastInput;
    ScreenSaverScreenPrivatePtr pPriv;

    rc = dixLookupDrawable(&pDraw, stuff->drawable, client, 0,
                           DixGetAttrAccess);
    if (rc != Success)
        return rc;
    rc = dixCallScreensaverAccessCallback(client, pDraw->pScreen, DixGetAttrAccess);
    if (rc != Success)
        return rc;

    pSaver = &pDraw->pScreen->screensaver;
    pPriv = GetScreenPrivate(pDraw->pScreen);

    UpdateCurrentTime();
    lastInput = GetTimeInMillis() - LastEventTime(XIAllDevices).milliseconds;

    xScreenSaverQueryInfoReply reply = {
        .window = pSaver->wid,
        .idle = lastInput,
        .eventMask = getEventMask(pDraw->pScreen, client),
    };

    if (screenIsSaved != SCREEN_SAVER_OFF) {
        reply.state = ScreenSaverOn;
        if (ScreenSaverTime)
            reply.tilOrSince = lastInput - ScreenSaverTime;
    }
    else {
        if (ScreenSaverTime) {
            reply.state = ScreenSaverOff;
            if (ScreenSaverTime >= lastInput)
                reply.tilOrSince = ScreenSaverTime - lastInput;
        }
        else {
            reply.state = ScreenSaverDisabled;
        }
    }

    if (pPriv && pPriv->attr)
        reply.kind = ScreenSaverExternal;
    else if (ScreenSaverBlanking != DontPreferBlanking)
        reply.kind = ScreenSaverBlanked;
    else
        reply.kind = ScreenSaverInternal;

    REPLY_FIELD_CARD32(window);
    REPLY_FIELD_CARD32(tilOrSince);
    REPLY_FIELD_CARD32(idle);
    REPLY_FIELD_CARD32(eventMask);

    return X_SEND_REPLY_SIMPLE(client, reply);
}

static int
ProcScreenSaverSelectInput(ClientPtr client)
{
    X_REQUEST_HEAD_STRUCT(xScreenSaverSelectInputReq);
    X_REQUEST_FIELD_CARD32(drawable);
    X_REQUEST_FIELD_CARD32(eventMask);

    DrawablePtr pDraw;
    int rc;
    rc = dixLookupDrawable(&pDraw, stuff->drawable, client, 0,
                           DixGetAttrAccess);
    if (rc != Success)
        return rc;

    rc = dixCallScreensaverAccessCallback(client, pDraw->pScreen, DixSetAttrAccess);
    if (rc != Success)
        return rc;

    if (!setEventMask(pDraw->pScreen, client, stuff->eventMask))
        return BadAlloc;
    return Success;
}

static int
ScreenSaverSetAttributes(ClientPtr client, xScreenSaverSetAttributesReq *stuff)
{
    DrawablePtr pDraw;
    WindowPtr pParent;
    ScreenPtr pScreen;
    ScreenSaverScreenPrivatePtr pPriv = 0;
    ScreenSaverAttrPtr pAttr = 0;
    int ret, len, class, bw, depth;
    unsigned long visual;
    int idepth, ivisual;
    Bool fOK;
    DepthPtr pDepth;
    WindowOptPtr ancwopt;
    unsigned int *pVlist;
    unsigned long *values = 0;
    unsigned long tmask, imask;
    unsigned long val;
    Pixmap pixID;
    PixmapPtr pPixmap;
    Cursor cursorID;
    CursorPtr pCursor;
    Colormap cmap;
    ColormapPtr pCmap;

    ret = dixLookupDrawable(&pDraw, stuff->drawable, client, 0,
                            DixGetAttrAccess);
    if (ret != Success)
        return ret;
    pScreen = pDraw->pScreen;
    pParent = pScreen->root;

    ret = dixCallScreensaverAccessCallback(client, pScreen, DixSetAttrAccess);
    if (ret != Success)
        return ret;

    len = client->req_len - bytes_to_int32(sizeof(xScreenSaverSetAttributesReq));
    if (Ones(stuff->mask) != len)
        return BadLength;
    if (!stuff->width || !stuff->height) {
        client->errorValue = 0;
        return BadValue;
    }
    switch (class = stuff->c_class) {
    case CopyFromParent:
    case InputOnly:
    case InputOutput:
        break;
    default:
        client->errorValue = class;
        return BadValue;
    }
    bw = stuff->borderWidth;
    depth = stuff->depth;
    visual = stuff->visualID;

    /* copied directly from dixCreateWindow */

    if (class == CopyFromParent)
        class = pParent->drawable.class;

    if ((class != InputOutput) && (class != InputOnly)) {
        client->errorValue = class;
        return BadValue;
    }

    if ((class != InputOnly) && (pParent->drawable.class == InputOnly))
        return BadMatch;

    if ((class == InputOnly) && ((bw != 0) || (depth != 0)))
        return BadMatch;

    if ((class == InputOutput) && (depth == 0))
        depth = pParent->drawable.depth;
    ancwopt = pParent->optional;
    if (!ancwopt)
        ancwopt = FindWindowWithOptional(pParent)->optional;
    if (visual == CopyFromParent)
        visual = ancwopt->visual;

    /* Find out if the depth and visual are acceptable for this Screen */
    if ((visual != ancwopt->visual) || (depth != pParent->drawable.depth)) {
        fOK = FALSE;
        for (idepth = 0; idepth < pScreen->numDepths; idepth++) {
            pDepth = (DepthPtr) &pScreen->allowedDepths[idepth];
            if ((depth == pDepth->depth) || (depth == 0)) {
                for (ivisual = 0; ivisual < pDepth->numVids; ivisual++) {
                    if (visual == pDepth->vids[ivisual]) {
                        fOK = TRUE;
                        break;
                    }
                }
            }
        }
        if (fOK == FALSE)
            return BadMatch;
    }

    if (((stuff->mask & (CWBorderPixmap | CWBorderPixel)) == 0) &&
        (class != InputOnly) && (depth != pParent->drawable.depth)) {
        return BadMatch;
    }

    if (((stuff->mask & CWColormap) == 0) &&
        (class != InputOnly) &&
        ((visual != ancwopt->visual) || (ancwopt->colormap == None))) {
        return BadMatch;
    }

    /* end of errors from dixCreateWindow */

    pPriv = GetScreenPrivate(pScreen);
    if (pPriv && pPriv->attr) {
        if (pPriv->attr->client != client)
            return BadAccess;
    }
    if (!pPriv) {
        pPriv = MakeScreenPrivate(pScreen);
        if (!pPriv)
            return FALSE;
    }
    pAttr = calloc(1, sizeof(ScreenSaverAttrRec));
    if (!pAttr) {
        ret = BadAlloc;
        goto bail;
    }
    /* over allocate for override redirect */
    pAttr->values = values = calloc(len + 1, sizeof(unsigned long));
    if (!values) {
        ret = BadAlloc;
        goto bail;
    }
    pAttr->screen = pScreen;
    pAttr->client = client;
    pAttr->x = stuff->x;
    pAttr->y = stuff->y;
    pAttr->width = stuff->width;
    pAttr->height = stuff->height;
    pAttr->borderWidth = stuff->borderWidth;
    pAttr->class = stuff->c_class;
    pAttr->depth = depth;
    pAttr->visual = visual;
    pAttr->colormap = None;
    pAttr->pCursor = NullCursor;
    pAttr->pBackgroundPixmap = NullPixmap;
    pAttr->pBorderPixmap = NullPixmap;
    /*
     * go through the mask, checking the values,
     * looking up pixmaps and cursors and hold a reference
     * to them.
     */
    pAttr->mask = tmask = stuff->mask | CWOverrideRedirect;
    pVlist = (unsigned int *) (stuff + 1);
    while (tmask) {
        imask = lowbit(tmask);
        tmask &= ~imask;
        switch (imask) {
        case CWBackPixmap:
            pixID = (Pixmap) * pVlist;
            if (pixID == None) {
                *values++ = None;
            }
            else if (pixID == ParentRelative) {
                if (depth != pParent->drawable.depth) {
                    ret = BadMatch;
                    goto PatchUp;
                }
                *values++ = ParentRelative;
            }
            else {
                ret =
                    dixLookupResourceByType((void **) &pPixmap, pixID,
                                            X11_RESTYPE_PIXMAP, client, DixReadAccess);
                if (ret == Success) {
                    if ((pPixmap->drawable.depth != depth) ||
                        (pPixmap->drawable.pScreen != pScreen)) {
                        ret = BadMatch;
                        goto PatchUp;
                    }
                    pAttr->pBackgroundPixmap = pPixmap;
                    pPixmap->refcnt++;
                    pAttr->mask &= ~CWBackPixmap;
                }
                else {
                    client->errorValue = pixID;
                    goto PatchUp;
                }
            }
            break;
        case CWBackPixel:
            *values++ = (CARD32) *pVlist;
            break;
        case CWBorderPixmap:
            pixID = (Pixmap) * pVlist;
            if (pixID == CopyFromParent) {
                if (depth != pParent->drawable.depth) {
                    ret = BadMatch;
                    goto PatchUp;
                }
                *values++ = CopyFromParent;
            }
            else {
                ret =
                    dixLookupResourceByType((void **) &pPixmap, pixID,
                                            X11_RESTYPE_PIXMAP, client, DixReadAccess);
                if (ret == Success) {
                    if ((pPixmap->drawable.depth != depth) ||
                        (pPixmap->drawable.pScreen != pScreen)) {
                        ret = BadMatch;
                        goto PatchUp;
                    }
                    pAttr->pBorderPixmap = pPixmap;
                    pPixmap->refcnt++;
                    pAttr->mask &= ~CWBorderPixmap;
                }
                else {
                    client->errorValue = pixID;
                    goto PatchUp;
                }
            }
            break;
        case CWBorderPixel:
            *values++ = (CARD32) *pVlist;
            break;
        case CWBitGravity:
            val = (CARD8) *pVlist;
            if (val > StaticGravity) {
                ret = BadValue;
                client->errorValue = val;
                goto PatchUp;
            }
            *values++ = val;
            break;
        case CWWinGravity:
            val = (CARD8) *pVlist;
            if (val > StaticGravity) {
                ret = BadValue;
                client->errorValue = val;
                goto PatchUp;
            }
            *values++ = val;
            break;
        case CWBackingStore:
            val = (CARD8) *pVlist;
            if ((val != NotUseful) && (val != WhenMapped) && (val != Always)) {
                ret = BadValue;
                client->errorValue = val;
                goto PatchUp;
            }
            *values++ = val;
            break;
        case CWBackingPlanes:
            *values++ = (CARD32) *pVlist;
            break;
        case CWBackingPixel:
            *values++ = (CARD32) *pVlist;
            break;
        case CWSaveUnder:
            val = (BOOL) * pVlist;
            if ((val != xTrue) && (val != xFalse)) {
                ret = BadValue;
                client->errorValue = val;
                goto PatchUp;
            }
            *values++ = val;
            break;
        case CWEventMask:
            *values++ = (CARD32) *pVlist;
            break;
        case CWDontPropagate:
            *values++ = (CARD32) *pVlist;
            break;
        case CWOverrideRedirect:
            if (!(stuff->mask & CWOverrideRedirect))
                pVlist--;
            else {
                val = (BOOL) * pVlist;
                if ((val != xTrue) && (val != xFalse)) {
                    ret = BadValue;
                    client->errorValue = val;
                    goto PatchUp;
                }
            }
            *values++ = xTrue;
            break;
        case CWColormap:
            cmap = (Colormap) * pVlist;
            ret = dixLookupResourceByType((void **) &pCmap, cmap, X11_RESTYPE_COLORMAP,
                                          client, DixUseAccess);
            if (ret != Success) {
                client->errorValue = cmap;
                goto PatchUp;
            }
            if (pCmap->pVisual->vid != visual || pCmap->pScreen != pScreen) {
                ret = BadMatch;
                goto PatchUp;
            }
            pAttr->colormap = cmap;
            pAttr->mask &= ~CWColormap;
            break;
        case CWCursor:
            cursorID = (Cursor) * pVlist;
            if (cursorID == None) {
                *values++ = None;
            }
            else {
                ret = dixLookupResourceByType((void **) &pCursor, cursorID,
                                              X11_RESTYPE_CURSOR, client, DixUseAccess);
                if (ret != Success) {
                    client->errorValue = cursorID;
                    goto PatchUp;
                }
                pAttr->pCursor = RefCursor(pCursor);
                pAttr->mask &= ~CWCursor;
            }
            break;
        default:
            ret = BadValue;
            client->errorValue = stuff->mask;
            goto PatchUp;
        }
        pVlist++;
    }
    if (pPriv->attr)
        FreeResource(pPriv->attr->resource, AttrType);
    pPriv->attr = pAttr;
    pAttr->resource = FakeClientID(client->index);
    if (!AddResource(pAttr->resource, AttrType, (void *) pAttr))
        return BadAlloc;
    return Success;
 PatchUp:
    FreeAttrs(pAttr);
 bail:
    CheckScreenPrivate(pScreen);
    if (pAttr)
        free(pAttr->values);
    free(pAttr);
    return ret;
}

static int
ScreenSaverUnsetAttributes(ClientPtr client, Drawable drawable)
{
    DrawablePtr pDraw;
    ScreenSaverScreenPrivatePtr pPriv;
    int rc;

    rc = dixLookupDrawable(&pDraw, drawable, client, 0, DixGetAttrAccess);
    if (rc != Success)
        return rc;
    pPriv = GetScreenPrivate(pDraw->pScreen);
    if (pPriv && pPriv->attr && pPriv->attr->client == client) {
        FreeResource(pPriv->attr->resource, AttrType);
        FreeScreenAttr(pPriv->attr);
        pPriv->attr = NULL;
        CheckScreenPrivate(pDraw->pScreen);
    }
    return Success;
}

static int
ProcScreenSaverSetAttributes(ClientPtr client)
{
    X_REQUEST_HEAD_AT_LEAST(xScreenSaverSetAttributesReq);
    X_REQUEST_FIELD_CARD32(drawable);
    X_REQUEST_FIELD_CARD16(x);
    X_REQUEST_FIELD_CARD16(y);
    X_REQUEST_FIELD_CARD16(width);
    X_REQUEST_FIELD_CARD16(height);
    X_REQUEST_FIELD_CARD16(borderWidth);
    X_REQUEST_FIELD_CARD32(visualID);
    X_REQUEST_FIELD_CARD32(mask);
    X_REQUEST_REST_CARD32();

#ifdef XINERAMA
    if (!noPanoramiXExtension) {
        PanoramiXRes *draw;
        PanoramiXRes *backPix = NULL;
        PanoramiXRes *bordPix = NULL;
        PanoramiXRes *cmap = NULL;
        int status, len;
        int pback_offset = 0, pbord_offset = 0, cmap_offset = 0;
        XID orig_visual, tmp;

        status = dixLookupResourceByClass((void **) &draw, stuff->drawable,
                                          XRC_DRAWABLE, client, DixWriteAccess);
        if (status != Success)
            return (status == BadValue) ? BadDrawable : status;

        len =
            client->req_len -
            bytes_to_int32(sizeof(xScreenSaverSetAttributesReq));
        if (Ones(stuff->mask) != len)
            return BadLength;

        if ((Mask) stuff->mask & CWBackPixmap) {
            pback_offset = Ones((Mask) stuff->mask & (CWBackPixmap - 1));
            tmp = *((CARD32 *) &stuff[1] + pback_offset);
            if ((tmp != None) && (tmp != ParentRelative)) {
                status = dixLookupResourceByType((void **) &backPix, tmp,
                                                 XRT_PIXMAP, client,
                                                 DixReadAccess);
                if (status != Success)
                    return status;
            }
        }

        if ((Mask) stuff->mask & CWBorderPixmap) {
            pbord_offset = Ones((Mask) stuff->mask & (CWBorderPixmap - 1));
            tmp = *((CARD32 *) &stuff[1] + pbord_offset);
            if (tmp != CopyFromParent) {
                status = dixLookupResourceByType((void **) &bordPix, tmp,
                                                 XRT_PIXMAP, client,
                                                 DixReadAccess);
                if (status != Success)
                    return status;
            }
        }

        if ((Mask) stuff->mask & CWColormap) {
            cmap_offset = Ones((Mask) stuff->mask & (CWColormap - 1));
            tmp = *((CARD32 *) &stuff[1] + cmap_offset);
            if (tmp != CopyFromParent) {
                status = dixLookupResourceByType((void **) &cmap, tmp,
                                                 XRT_COLORMAP, client,
                                                 DixReadAccess);
                if (status != Success)
                    return status;
            }
        }

        orig_visual = stuff->visualID;

        XINERAMA_FOR_EACH_SCREEN_BACKWARD({
            stuff->drawable = draw->info[walkScreenIdx].id;
            if (backPix)
                *((CARD32 *) &stuff[1] + pback_offset) = backPix->info[walkScreenIdx].id;
            if (bordPix)
                *((CARD32 *) &stuff[1] + pbord_offset) = bordPix->info[walkScreenIdx].id;
            if (cmap)
                *((CARD32 *) &stuff[1] + cmap_offset) = cmap->info[walkScreenIdx].id;

            if (orig_visual != CopyFromParent)
                stuff->visualID = PanoramiXTranslateVisualID(walkScreenIdx, orig_visual);

            status = ScreenSaverSetAttributes(client, stuff);
        });

        return status;
    }
#endif /* XINERAMA */

    return ScreenSaverSetAttributes(client, stuff);
}

static int
ProcScreenSaverUnsetAttributes(ClientPtr client)
{
    X_REQUEST_HEAD_STRUCT(xScreenSaverUnsetAttributesReq);
    X_REQUEST_FIELD_CARD32(drawable);

#ifdef XINERAMA
    if (!noPanoramiXExtension) {
        PanoramiXRes *draw;
        int rc, i;


        rc = dixLookupResourceByClass((void **) &draw, stuff->drawable,
                                      XRC_DRAWABLE, client, DixWriteAccess);
        if (rc != Success)
            return (rc == BadValue) ? BadDrawable : rc;

        for (i = PanoramiXNumScreens - 1; i > 0; i--) {
            ScreenSaverUnsetAttributes(client, draw->info[i].id);
        }

        stuff->drawable = draw->info[0].id;
    }
#endif /* XINERAMA */

    return ScreenSaverUnsetAttributes(client, stuff->drawable);
}

static int
ProcScreenSaverSuspend(ClientPtr client)
{
    X_REQUEST_HEAD_STRUCT(xScreenSaverSuspendReq);
    X_REQUEST_FIELD_CARD32(suspend);

    ScreenSaverSuspensionPtr *prev, this;
    BOOL suspend;
    /*
     * Old versions of XCB encode suspend as 1 byte followed by three
     * pad bytes (which are always cleared), instead of a 4 byte
     * value. Be compatible by just checking for a non-zero value in
     * all 32-bits.
     */
    suspend = stuff->suspend != 0;

    /* Check if this client is suspending the screensaver */
    for (prev = &suspendingClients; (this = *prev); prev = &this->next)
        if (this->pClient == client)
            break;

    if (this) {
        if (suspend == TRUE)
            this->count++;
        else if (--this->count == 0)
            FreeResource(this->clientResource, X11_RESTYPE_NONE);

        return Success;
    }

    /* If we get to this point, this client isn't suspending the screensaver */
    if (suspend == FALSE)
        return Success;

    /*
     * Allocate a suspension record for the client, and stop the screensaver
     * if it isn't already suspended by another client. We attach a resource ID
     * to the record, so the screensaver will be re-enabled and the record freed
     * if the client disconnects without reenabling it first.
     */
    this = calloc(1, sizeof(ScreenSaverSuspensionRec));

    if (!this)
        return BadAlloc;

    this->next = NULL;
    this->pClient = client;
    this->count = 1;
    this->clientResource = FakeClientID(client->index);

    if (!AddResource(this->clientResource, SuspendType, (void *) this)) {
        free(this);
        return BadAlloc;
    }

    *prev = this;
    if (!screenSaverSuspended) {
        screenSaverSuspended = TRUE;
        FreeScreenSaverTimer();
    }

    return Success;
}

static int
ProcScreenSaverDispatch(ClientPtr client)
{
    REQUEST(xReq);
    switch (stuff->data) {
        case X_ScreenSaverQueryVersion:
            return ProcScreenSaverQueryVersion(client);
        case X_ScreenSaverQueryInfo:
            return ProcScreenSaverQueryInfo(client);
        case X_ScreenSaverSelectInput:
            return ProcScreenSaverSelectInput(client);
        case X_ScreenSaverSetAttributes:
            return ProcScreenSaverSetAttributes(client);
        case X_ScreenSaverUnsetAttributes:
            return ProcScreenSaverUnsetAttributes(client);
        case X_ScreenSaverSuspend:
            return ProcScreenSaverSuspend(client);
        default:
            return BadRequest;
    }
}

void
ScreenSaverExtensionInit(void)
{
    ExtensionEntry *extEntry;

    if (!dixRegisterPrivateKey(&ScreenPrivateKeyRec, PRIVATE_SCREEN, 0))
        return;

    AttrType = CreateNewResourceType(ScreenSaverFreeAttr, "SaverAttr");
    SaverEventType = CreateNewResourceType(ScreenSaverFreeEvents, "SaverEvent");
    SuspendType = CreateNewResourceType(ScreenSaverFreeSuspend, "SaverSuspend");

    DIX_FOR_EACH_SCREEN({
        SetScreenPrivate(walkScreen, NULL);
    });

    if (AttrType && SaverEventType && SuspendType &&
        (extEntry = AddExtension(ScreenSaverName, ScreenSaverNumberEvents, 0,
                                 ProcScreenSaverDispatch,
                                 ProcScreenSaverDispatch, NULL,
                                 StandardMinorOpcode))) {
        ScreenSaverEventBase = extEntry->eventBase;
        EventSwapVector[ScreenSaverEventBase] =
            (EventSwapPtr) SScreenSaverNotifyEvent;
    }
}

/*
 * Copyright © 2002 Keith Packard
 * Copyright 2013 Red Hat, Inc.
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

#include <X11/Xproto.h>
#include <X11/extensions/damageproto.h>

#include "dix/dix_priv.h"
#include "dix/request_priv.h"
#include "dix/screenint_priv.h"
#include "include/pixmapstr.h"
#include "miext/extinit_priv.h"
#include "os/client_priv.h"
#include "Xext/panoramiX.h"
#include "Xext/panoramiXsrv.h"
#include "xfixes/xfixes.h"

#include "damageextint.h"
#include "damagestr.h"
#include "protocol-versions.h"
#include "dixstruct_priv.h"

typedef struct _DamageClient {
    CARD32 major_version;
    CARD32 minor_version;
    int critical;
} DamageClientRec, *DamageClientPtr;

typedef struct _DamageExt {
    DamagePtr pDamage;
    DrawablePtr pDrawable;
    DamageReportLevel level;
    ClientPtr pClient;
    XID id;
    XID drawable;
} DamageExtRec, *DamageExtPtr;

#define VERIFY_DAMAGEEXT(pDamageExt, rid, client, mode) { \
    int rc = dixLookupResourceByType((void **)&(pDamageExt), rid, \
                                     DamageExtType, client, mode); \
    if (rc != Success) \
        return rc; \
}

#define GetDamageClient(pClient) ((DamageClientPtr)dixLookupPrivate(&(pClient)->devPrivates, DamageClientPrivateKey))

#ifdef XINERAMA

typedef struct {
    DamageExtPtr ext;
    DamagePtr damage[MAXSCREENS];
} PanoramiXDamageRes;

static RESTYPE XRT_DAMAGE;
static int damageUseXinerama = 0;

static int PanoramiXDamageCreate(ClientPtr client, xDamageCreateReq *stuff);

#endif /* XINERAMA */

static unsigned char DamageReqCode;
static int DamageEventBase;
static RESTYPE DamageExtType;

static DevPrivateKeyRec DamageClientPrivateKeyRec;

#define DamageClientPrivateKey (&DamageClientPrivateKeyRec)

Bool noDamageExtension = FALSE;

static void
DamageNoteCritical(ClientPtr pClient)
{
    DamageClientPtr pDamageClient = GetDamageClient(pClient);

    /* Composite extension marks clients with manual Subwindows as critical */
    if (pDamageClient->critical > 0) {
        SetCriticalOutputPending();
        pClient->smart_priority = SMART_MAX_PRIORITY;
    }
}

static void
damageGetGeometry(DrawablePtr draw, int *x, int *y, int *w, int *h)
{
#ifdef XINERAMA
    if (!noPanoramiXExtension && draw->type == DRAWABLE_WINDOW) {
        WindowPtr win = (WindowPtr)draw;

        if (!win->parent) {
            *x = screenInfo.x;
            *y = screenInfo.y;
            *w = screenInfo.width;
            *h = screenInfo.height;
            return;
        }
    }
#endif /* XINERAMA */

    *x = draw->x;
    *y = draw->y;
    *w = draw->width;
    *h = draw->height;
}

static void
DamageExtNotify(DamageExtPtr pDamageExt, BoxPtr pBoxes, int nBoxes)
{
    ClientPtr pClient = pDamageExt->pClient;
    DrawablePtr pDrawable = pDamageExt->pDrawable;
    int i, x, y, w, h;

    damageGetGeometry(pDrawable, &x, &y, &w, &h);

    UpdateCurrentTimeIf();
    xDamageNotifyEvent ev = {
        .type = DamageEventBase + XDamageNotify,
        .level = pDamageExt->level,
        .drawable = pDamageExt->drawable,
        .damage = pDamageExt->id,
        .timestamp = currentTime.milliseconds,
        .geometry.x = x,
        .geometry.y = y,
        .geometry.width = w,
        .geometry.height = h
    };
    if (pBoxes) {
        for (i = 0; i < nBoxes; i++) {
            ev.level = pDamageExt->level;
            if (i < nBoxes - 1)
                ev.level |= DamageNotifyMore;
            ev.area.x = pBoxes[i].x1;
            ev.area.y = pBoxes[i].y1;
            ev.area.width = pBoxes[i].x2 - pBoxes[i].x1;
            ev.area.height = pBoxes[i].y2 - pBoxes[i].y1;
            WriteEventsToClient(pClient, 1, (xEvent *) &ev);
        }
    }
    else {
        ev.area.x = 0;
        ev.area.y = 0;
        ev.area.width = w;
        ev.area.height = h;
        WriteEventsToClient(pClient, 1, (xEvent *) &ev);
    }

    DamageNoteCritical(pClient);
}

static void
DamageExtReport(DamagePtr pDamage, RegionPtr pRegion, void *closure)
{
    DamageExtPtr pDamageExt = closure;

    switch (pDamageExt->level) {
    case DamageReportRawRegion:
    case DamageReportDeltaRegion:
        DamageExtNotify(pDamageExt, RegionRects(pRegion),
                        RegionNumRects(pRegion));
        break;
    case DamageReportBoundingBox:
        DamageExtNotify(pDamageExt, RegionExtents(pRegion), 1);
        break;
    case DamageReportNonEmpty:
        DamageExtNotify(pDamageExt, NullBox, 0);
        break;
    case DamageReportNone:
        break;
    }
}

static void
DamageExtDestroy(DamagePtr pDamage, void *closure)
{
    DamageExtPtr pDamageExt = closure;

    pDamageExt->pDamage = 0;
    if (pDamageExt->id)
        FreeResource(pDamageExt->id, X11_RESTYPE_NONE);
}

void
DamageExtSetCritical(ClientPtr pClient, Bool critical)
{
    DamageClientPtr pDamageClient = GetDamageClient(pClient);

    if (pDamageClient)
        pDamageClient->critical += critical ? 1 : -1;
}

static int
ProcDamageQueryVersion(ClientPtr client)
{
    X_REQUEST_HEAD_STRUCT(xDamageQueryVersionReq);
    X_REQUEST_FIELD_CARD32(majorVersion);
    X_REQUEST_FIELD_CARD32(minorVersion);

    DamageClientPtr pDamageClient = GetDamageClient(client);

    xDamageQueryVersionReply reply = { 0 };
    if (stuff->majorVersion < SERVER_DAMAGE_MAJOR_VERSION) {
        reply.majorVersion = stuff->majorVersion;
        reply.minorVersion = stuff->minorVersion;
    }
    else {
        reply.majorVersion = SERVER_DAMAGE_MAJOR_VERSION;
        if (stuff->majorVersion == SERVER_DAMAGE_MAJOR_VERSION &&
            stuff->minorVersion < SERVER_DAMAGE_MINOR_VERSION)
            reply.minorVersion = stuff->minorVersion;
        else
            reply.minorVersion = SERVER_DAMAGE_MINOR_VERSION;
    }
    pDamageClient->major_version = reply.majorVersion;
    pDamageClient->minor_version = reply.minorVersion;
    if (client->swapped) {
        swapl(&reply.majorVersion);
        swapl(&reply.minorVersion);
    }

    return X_SEND_REPLY_SIMPLE(client, reply);
}

static void
DamageExtRegister(DrawablePtr pDrawable, DamagePtr pDamage, Bool report)
{
    DamageSetReportAfterOp(pDamage, TRUE);
    DamageRegister(pDrawable, pDamage);

    if (report) {
        RegionPtr pRegion = &((WindowPtr) pDrawable)->borderClip;
        RegionTranslate(pRegion, -pDrawable->x, -pDrawable->y);
        DamageReportDamage(pDamage, pRegion);
        RegionTranslate(pRegion, pDrawable->x, pDrawable->y);
    }
}

static DamageExtPtr
DamageExtCreate(DrawablePtr pDrawable, DamageReportLevel level,
                ClientPtr client, XID id, XID drawable)
{
    DamageExtPtr pDamageExt = calloc(1, sizeof(DamageExtRec));
    if (!pDamageExt)
        return NULL;

    pDamageExt->id = id;
    pDamageExt->drawable = drawable;
    pDamageExt->pDrawable = pDrawable;
    pDamageExt->level = level;
    pDamageExt->pClient = client;
    pDamageExt->pDamage = DamageCreate(DamageExtReport, DamageExtDestroy, level,
                                       FALSE, pDrawable->pScreen, pDamageExt);
    if (!pDamageExt->pDamage) {
        free(pDamageExt);
        return NULL;
    }

    if (!AddResource(id, DamageExtType, (void *) pDamageExt))
        return NULL;

    DamageExtRegister(pDrawable, pDamageExt->pDamage,
                      pDrawable->type == DRAWABLE_WINDOW);

    return pDamageExt;
}

static DamageExtPtr
doDamageCreate(ClientPtr client, int *rc, xDamageCreateReq *stuff)
{
    DrawablePtr pDrawable;
    DamageExtPtr pDamageExt;
    DamageReportLevel level;

    *rc = dixLookupDrawable(&pDrawable, stuff->drawable, client, 0,
                            DixGetAttrAccess | DixReadAccess);
    if (*rc != Success)
        return NULL;

    switch (stuff->level) {
    case XDamageReportRawRectangles:
        level = DamageReportRawRegion;
        break;
    case XDamageReportDeltaRectangles:
        level = DamageReportDeltaRegion;
        break;
    case XDamageReportBoundingBox:
        level = DamageReportBoundingBox;
        break;
    case XDamageReportNonEmpty:
        level = DamageReportNonEmpty;
        break;
    default:
        client->errorValue = stuff->level;
        *rc = BadValue;
        return NULL;
    }

    pDamageExt = DamageExtCreate(pDrawable, level, client, stuff->damage,
                                 stuff->drawable);
    if (!pDamageExt)
        *rc = BadAlloc;

    return pDamageExt;
}

static int
ProcDamageCreate(ClientPtr client)
{
    X_REQUEST_HEAD_STRUCT(xDamageCreateReq);
    X_REQUEST_FIELD_CARD32(damage);
    X_REQUEST_FIELD_CARD32(drawable);

    int rc;

#ifdef XINERAMA
    if (damageUseXinerama)
        return PanoramiXDamageCreate(client, stuff);
#endif

    LEGAL_NEW_RESOURCE(stuff->damage, client);
    doDamageCreate(client, &rc, stuff);
    return rc;
}

static int
ProcDamageDestroy(ClientPtr client)
{
    X_REQUEST_HEAD_STRUCT(xDamageDestroyReq);
    X_REQUEST_FIELD_CARD32(damage);

    DamageExtPtr pDamageExt;
    VERIFY_DAMAGEEXT(pDamageExt, stuff->damage, client, DixDestroyAccess);
    FreeResource(stuff->damage, X11_RESTYPE_NONE);
    return Success;
}

#ifdef XINERAMA
static RegionPtr
DamageExtSubtractWindowClip(DamageExtPtr pDamageExt)
{
    WindowPtr win = (WindowPtr)pDamageExt->pDrawable;
    PanoramiXRes *res = NULL;
    RegionPtr ret;

    if (!win->parent)
        return &PanoramiXScreenRegion;

    dixLookupResourceByType((void **)&res, win->drawable.id, XRT_WINDOW,
                            serverClient, DixReadAccess);
    if (!res)
        return NULL;

    ret = RegionCreate(NULL, 0);
    if (!ret)
        return NULL;

    XINERAMA_FOR_EACH_SCREEN_FORWARD({
        ScreenPtr screen;
        if (Success != dixLookupWindow(&win, res->info[walkScreenIdx].id, serverClient,
                                       DixReadAccess))
            goto out;

        screen = win->drawable.pScreen;

        RegionTranslate(ret, -screen->x, -screen->y);
        if (!RegionUnion(ret, ret, &win->borderClip))
            goto out;
        RegionTranslate(ret, screen->x, screen->y);
    });

    return ret;

out:
    RegionDestroy(ret);
    return NULL;
}

static void
DamageExtFreeWindowClip(RegionPtr reg)
{
    if (reg != &PanoramiXScreenRegion)
        RegionDestroy(reg);
}
#endif /* XINERAMA */

/*
 * DamageSubtract intersects with borderClip, so we must reconstruct the
 * protocol's perspective of same...
 */
static Bool
DamageExtSubtract(DamageExtPtr pDamageExt, const RegionPtr pRegion)
{
    DamagePtr pDamage = pDamageExt->pDamage;

#ifdef XINERAMA
    if (!noPanoramiXExtension) {
        RegionPtr damage = DamageRegion(pDamage);
        RegionSubtract(damage, damage, pRegion);

        if (pDamageExt->pDrawable->type == DRAWABLE_WINDOW) {
            DrawablePtr pDraw = pDamageExt->pDrawable;
            RegionPtr clip = DamageExtSubtractWindowClip(pDamageExt);
            if (clip) {
                RegionTranslate(clip, -pDraw->x, -pDraw->y);
                RegionIntersect(damage, damage, clip);
                RegionTranslate(clip, pDraw->x, pDraw->y);
                DamageExtFreeWindowClip(clip);
            }
        }

        return RegionNotEmpty(damage);
    }
#endif /* XINERAMA */

    return DamageSubtract(pDamage, pRegion);
}

static int
ProcDamageSubtract(ClientPtr client)
{
    X_REQUEST_HEAD_STRUCT(xDamageSubtractReq);
    X_REQUEST_FIELD_CARD32(damage);
    X_REQUEST_FIELD_CARD32(repair);
    X_REQUEST_FIELD_CARD32(parts);

    DamageExtPtr pDamageExt;
    RegionPtr pRepair;
    RegionPtr pParts;

    VERIFY_DAMAGEEXT(pDamageExt, stuff->damage, client, DixWriteAccess);
    VERIFY_REGION_OR_NONE(pRepair, stuff->repair, client, DixWriteAccess);
    VERIFY_REGION_OR_NONE(pParts, stuff->parts, client, DixWriteAccess);

    if (pDamageExt->level != DamageReportRawRegion) {
        DamagePtr pDamage = pDamageExt->pDamage;

        if (pRepair) {
            if (pParts)
                RegionIntersect(pParts, DamageRegion(pDamage), pRepair);
            if (DamageExtSubtract(pDamageExt, pRepair))
                DamageExtReport(pDamage, DamageRegion(pDamage),
                                (void *) pDamageExt);
        }
        else {
            if (pParts)
                RegionCopy(pParts, DamageRegion(pDamage));
            DamageEmpty(pDamage);
        }
    }

    return Success;
}

static int
ProcDamageAdd(ClientPtr client)
{
    X_REQUEST_HEAD_STRUCT(xDamageAddReq);
    X_REQUEST_FIELD_CARD32(drawable);
    X_REQUEST_FIELD_CARD32(region);

    DrawablePtr pDrawable;
    RegionPtr pRegion;
    int rc;

    VERIFY_REGION(pRegion, stuff->region, client, DixWriteAccess);
    rc = dixLookupDrawable(&pDrawable, stuff->drawable, client, 0,
                           DixWriteAccess);
    if (rc != Success)
        return rc;

    /* The region is relative to the drawable origin, so translate it out to
     * screen coordinates like damage expects.
     */
    RegionTranslate(pRegion, pDrawable->x, pDrawable->y);
    DamageDamageRegion(pDrawable, pRegion);
    RegionTranslate(pRegion, -pDrawable->x, -pDrawable->y);

    return Success;
}

static int
ProcDamageDispatch(ClientPtr client)
{
    REQUEST(xReq);
    switch (stuff->data) {
        /* version 1 */
        case X_DamageQueryVersion:
            return ProcDamageQueryVersion(client);
        case X_DamageCreate:
            return ProcDamageCreate(client);
        case X_DamageDestroy:
            return ProcDamageDestroy(client);
        case X_DamageSubtract:
            return ProcDamageSubtract(client);
        /* version 1.1 */
        case X_DamageAdd:
            return ProcDamageAdd(client);
        default:
            return BadRequest;
    }
}

static int
FreeDamageExt(void *value, XID did)
{
    DamageExtPtr pDamageExt = (DamageExtPtr) value;

    /*
     * Get rid of the resource table entry hanging from the window id
     */
    pDamageExt->id = 0;
    if (pDamageExt->pDamage) {
        DamageDestroy(pDamageExt->pDamage);
    }
    free(pDamageExt);
    return Success;
}

static void _X_COLD
SDamageNotifyEvent(xDamageNotifyEvent * from, xDamageNotifyEvent * to)
{
    to->type = from->type;
    cpswaps(from->sequenceNumber, to->sequenceNumber);
    cpswapl(from->drawable, to->drawable);
    cpswapl(from->damage, to->damage);
    cpswaps(from->area.x, to->area.x);
    cpswaps(from->area.y, to->area.y);
    cpswaps(from->area.width, to->area.width);
    cpswaps(from->area.height, to->area.height);
    cpswaps(from->geometry.x, to->geometry.x);
    cpswaps(from->geometry.y, to->geometry.y);
    cpswaps(from->geometry.width, to->geometry.width);
    cpswaps(from->geometry.height, to->geometry.height);
}

#ifdef XINERAMA

static void
PanoramiXDamageReport(DamagePtr pDamage, RegionPtr pRegion, void *closure)
{
    PanoramiXDamageRes *res = closure;
    DamageExtPtr pDamageExt = res->ext;
    WindowPtr pWin = (WindowPtr)pDamage->pDrawable;
    ScreenPtr pScreen = pDamage->pScreen;

    /* happens on unmap? sigh xinerama */
    if (RegionNil(pRegion))
        return;

    /* translate root windows if necessary */
    if (!pWin->parent)
        RegionTranslate(pRegion, pScreen->x, pScreen->y);

    /* add our damage to the protocol view */
    DamageReportDamage(pDamageExt->pDamage, pRegion);

    /* empty our view */
    DamageEmpty(pDamage);
}

static void
PanoramiXDamageExtDestroy(DamagePtr pDamage, void *closure)
{
    PanoramiXDamageRes *damage = closure;
    damage->damage[pDamage->pScreen->myNum] = NULL;
}

static int
PanoramiXDamageCreate(ClientPtr client, xDamageCreateReq *stuff)
{
    PanoramiXDamageRes *damage;
    PanoramiXRes *draw;
    int rc;

    LEGAL_NEW_RESOURCE(stuff->damage, client);
    rc = dixLookupResourceByClass((void **)&draw, stuff->drawable, XRC_DRAWABLE,
                                  client, DixGetAttrAccess | DixReadAccess);
    if (rc != Success)
        return rc;

    if (!(damage = calloc(1, sizeof(PanoramiXDamageRes))))
        return BadAlloc;

    if (!AddResource(stuff->damage, XRT_DAMAGE, damage))
        return BadAlloc;

    damage->ext = doDamageCreate(client, &rc, stuff);
    if (rc == Success && draw->type == XRT_WINDOW) {
        XINERAMA_FOR_EACH_SCREEN_FORWARD({
            DrawablePtr pDrawable;
            DamagePtr pDamage = DamageCreate(PanoramiXDamageReport,
                                             PanoramiXDamageExtDestroy,
                                             DamageReportRawRegion,
                                             FALSE,
                                             walkScreen,
                                             damage);
            if (!pDamage) {
                rc = BadAlloc;
            } else {
                damage->damage[walkScreenIdx] = pDamage;
                rc = dixLookupDrawable(&pDrawable, draw->info[walkScreenIdx].id, client,
                                       M_WINDOW,
                                       DixGetAttrAccess | DixReadAccess);
            }
            if (rc != Success)
                break;

            DamageExtRegister(pDrawable, pDamage, walkScreenIdx != 0);
        });
    }

    if (rc != Success)
        FreeResource(stuff->damage, X11_RESTYPE_NONE);

    return rc;
}

static int
PanoramiXDamageDelete(void *res, XID id)
{
    PanoramiXDamageRes *damage = res;

    XINERAMA_FOR_EACH_SCREEN_BACKWARD({
        if (damage->damage[walkScreenIdx]) {
            DamageDestroy(damage->damage[walkScreenIdx]);
            damage->damage[walkScreenIdx] = NULL;
        }
    });

    free(damage);
    return 1;
}

void
PanoramiXDamageInit(void)
{
    XRT_DAMAGE = CreateNewResourceType(PanoramiXDamageDelete, "XineramaDamage");
    if (!XRT_DAMAGE)
        FatalError("Couldn't Xineramify Damage extension\n");

    damageUseXinerama = 1;
}

void
PanoramiXDamageReset(void)
{
    damageUseXinerama = 0;
}

#endif /* XINERAMA */

void
DamageExtensionInit(void)
{
    ExtensionEntry *extEntry;

    DIX_FOR_EACH_SCREEN({
        DamageSetup(walkScreen);
    });

    DamageExtType = CreateNewResourceType(FreeDamageExt, "DamageExt");
    if (!DamageExtType)
        return;

    if (!dixRegisterPrivateKey
        (&DamageClientPrivateKeyRec, PRIVATE_CLIENT, sizeof(DamageClientRec)))
        return;

    if ((extEntry = AddExtension(DAMAGE_NAME, XDamageNumberEvents,
                                 XDamageNumberErrors,
                                 ProcDamageDispatch, ProcDamageDispatch,
                                 NULL, StandardMinorOpcode)) != 0) {
        DamageReqCode = (unsigned char) extEntry->base;
        DamageEventBase = extEntry->eventBase;
        EventSwapVector[DamageEventBase + XDamageNotify] =
            (EventSwapPtr) SDamageNotifyEvent;
        SetResourceTypeErrorValue(DamageExtType,
                                  extEntry->errorBase + BadDamage);
#ifdef XINERAMA
        if (XRT_DAMAGE)
            SetResourceTypeErrorValue(XRT_DAMAGE,
                                      extEntry->errorBase + BadDamage);
#endif /* XINERAMA */
    }
}

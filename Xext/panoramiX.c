/*****************************************************************
Copyright (c) 1991, 1997 Digital Equipment Corporation, Maynard, Massachusetts.
Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software.

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
DIGITAL EQUIPMENT CORPORATION BE LIABLE FOR ANY CLAIM, DAMAGES, INCLUDING,
BUT NOT LIMITED TO CONSEQUENTIAL OR INCIDENTAL DAMAGES, OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR
IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of Digital Equipment Corporation
shall not be used in advertising or otherwise to promote the sale, use or other
dealings in this Software without prior written authorization from Digital
Equipment Corporation.
******************************************************************/

#include <dix-config.h>

#include <stdio.h>
#include <X11/X.h>
#include <X11/Xproto.h>
#include <X11/Xarch.h>
#include <X11/extensions/panoramiXproto.h>

#include "dix/dix_priv.h"
#include "dix/request_priv.h"
#include "dix/resource_priv.h"
#include "dix/rpcbuf_priv.h"
#include "dix/screen_hooks_priv.h"
#include "dix/screenint_priv.h"
#include "dix/server_priv.h"
#include "miext/extinit_priv.h"
#include "os/osdep.h"
#include "Xext/panoramiX.h"
#include "Xext/panoramiXsrv.h"

#include "misc.h"
#include "cursor.h"
#include "cursorstr.h"
#include "extnsionst.h"
#include "dixstruct.h"
#include "gc.h"
#include "gcstruct.h"
#include "scrnintstr.h"
#include "window.h"
#include "windowstr.h"
#include "pixmapstr.h"
#include "globals.h"
#include "servermd.h"
#include "resource.h"
#include "picturestr_priv.h"
#include "xfixesint.h"
#include "Xext/damage/damageextint.h"
#include "compint.h"
#include "protocol-versions.h"

/* Xinerama is disabled by default unless enabled via +xinerama */
Bool noPanoramiXExtension = TRUE;

/*
 *	PanoramiX data declarations
 */

int PanoramiXPixWidth = 0;
int PanoramiXPixHeight = 0;
int PanoramiXNumScreens = 0;

RegionRec PanoramiXScreenRegion = { {0, 0, 0, 0}, NULL };

static int PanoramiXNumDepths;
static DepthPtr PanoramiXDepths;
static int PanoramiXNumVisuals;
static VisualPtr PanoramiXVisuals;

RESTYPE XRC_DRAWABLE;
RESTYPE XRT_WINDOW;
RESTYPE XRT_PIXMAP;
RESTYPE XRT_GC;
RESTYPE XRT_COLORMAP;

static Bool VisualsEqual(VisualPtr, ScreenPtr, VisualPtr);
static XineramaVisualsEqualProcPtr XineramaVisualsEqualPtr = &VisualsEqual;

/*
 *	Function prototypes
 */

static int ProcPanoramiXDispatch(ClientPtr client);

static void PanoramiXResetProc(ExtensionEntry *);

/*
 *	External references for functions and data variables
 */

#include "panoramiXh.h"

int (*SavedProcVector[256]) (ClientPtr client) = {
NULL,};

static DevPrivateKeyRec PanoramiXGCKeyRec;
static DevPrivateKeyRec PanoramiXScreenKeyRec;

typedef struct {
    DDXPointRec clipOrg;
    DDXPointRec patOrg;
    const GCFuncs *wrapFuncs;
} PanoramiXGCRec, *PanoramiXGCPtr;

typedef struct {
    CreateGCProcPtr CreateGC;
} PanoramiXScreenRec, *PanoramiXScreenPtr;

static void XineramaValidateGC(GCPtr, unsigned long, DrawablePtr);
static void XineramaChangeGC(GCPtr, unsigned long);
static void XineramaCopyGC(GCPtr, unsigned long, GCPtr);
static void XineramaDestroyGC(GCPtr);
static void XineramaChangeClip(GCPtr, int, void *, int);
static void XineramaDestroyClip(GCPtr);
static void XineramaCopyClip(GCPtr, GCPtr);

static const GCFuncs XineramaGCFuncs = {
    XineramaValidateGC, XineramaChangeGC, XineramaCopyGC, XineramaDestroyGC,
    XineramaChangeClip, XineramaDestroyClip, XineramaCopyClip
};

#define Xinerama_GC_FUNC_PROLOGUE(pGC)\
    PanoramiXGCPtr  pGCPriv = (PanoramiXGCPtr) \
	dixLookupPrivate(&(pGC)->devPrivates, &PanoramiXGCKeyRec); \
    (pGC)->funcs = pGCPriv->wrapFuncs;

#define Xinerama_GC_FUNC_EPILOGUE(pGC)\
    pGCPriv->wrapFuncs = (pGC)->funcs;\
    (pGC)->funcs = &XineramaGCFuncs;

static void XineramaCloseScreen(CallbackListPtr *pcbl, ScreenPtr pScreen, void *unsused)
{
    dixScreenUnhookClose(pScreen, XineramaCloseScreen);

    PanoramiXScreenPtr pScreenPriv = (PanoramiXScreenPtr)
        dixLookupPrivate(&pScreen->devPrivates, &PanoramiXScreenKeyRec);

    if (!pScreenPriv)
        return;

    pScreen->CreateGC = pScreenPriv->CreateGC;

    if (pScreen->myNum == 0)
        RegionUninit(&PanoramiXScreenRegion);

    free(pScreenPriv);
    dixSetPrivate(&pScreen->devPrivates, &PanoramiXScreenKeyRec, NULL);
}

static Bool
XineramaCreateGC(GCPtr pGC)
{
    ScreenPtr pScreen = pGC->pScreen;
    PanoramiXScreenPtr pScreenPriv = (PanoramiXScreenPtr)
        dixLookupPrivate(&pScreen->devPrivates, &PanoramiXScreenKeyRec);
    Bool ret;

    pScreen->CreateGC = pScreenPriv->CreateGC;
    if ((ret = (*pScreen->CreateGC) (pGC))) {
        PanoramiXGCPtr pGCPriv = (PanoramiXGCPtr)
            dixLookupPrivate(&pGC->devPrivates, &PanoramiXGCKeyRec);

        pGCPriv->wrapFuncs = pGC->funcs;
        pGC->funcs = &XineramaGCFuncs;

        pGCPriv->clipOrg.x = pGC->clipOrg.x;
        pGCPriv->clipOrg.y = pGC->clipOrg.y;
        pGCPriv->patOrg.x = pGC->patOrg.x;
        pGCPriv->patOrg.y = pGC->patOrg.y;
    }
    pScreen->CreateGC = XineramaCreateGC;

    return ret;
}

static void
XineramaValidateGC(GCPtr pGC, unsigned long changes, DrawablePtr pDraw)
{
    Xinerama_GC_FUNC_PROLOGUE(pGC);

    if ((pDraw->type == DRAWABLE_WINDOW) && !(((WindowPtr) pDraw)->parent)) {
        /* the root window */
        int x_off = pGC->pScreen->x;
        int y_off = pGC->pScreen->y;
        int new_val;

        new_val = pGCPriv->clipOrg.x - x_off;
        if (pGC->clipOrg.x != new_val) {
            pGC->clipOrg.x = new_val;
            changes |= GCClipXOrigin;
        }
        new_val = pGCPriv->clipOrg.y - y_off;
        if (pGC->clipOrg.y != new_val) {
            pGC->clipOrg.y = new_val;
            changes |= GCClipYOrigin;
        }
        new_val = pGCPriv->patOrg.x - x_off;
        if (pGC->patOrg.x != new_val) {
            pGC->patOrg.x = new_val;
            changes |= GCTileStipXOrigin;
        }
        new_val = pGCPriv->patOrg.y - y_off;
        if (pGC->patOrg.y != new_val) {
            pGC->patOrg.y = new_val;
            changes |= GCTileStipYOrigin;
        }
    }
    else {
        if (pGC->clipOrg.x != pGCPriv->clipOrg.x) {
            pGC->clipOrg.x = pGCPriv->clipOrg.x;
            changes |= GCClipXOrigin;
        }
        if (pGC->clipOrg.y != pGCPriv->clipOrg.y) {
            pGC->clipOrg.y = pGCPriv->clipOrg.y;
            changes |= GCClipYOrigin;
        }
        if (pGC->patOrg.x != pGCPriv->patOrg.x) {
            pGC->patOrg.x = pGCPriv->patOrg.x;
            changes |= GCTileStipXOrigin;
        }
        if (pGC->patOrg.y != pGCPriv->patOrg.y) {
            pGC->patOrg.y = pGCPriv->patOrg.y;
            changes |= GCTileStipYOrigin;
        }
    }

    (*pGC->funcs->ValidateGC) (pGC, changes, pDraw);
    Xinerama_GC_FUNC_EPILOGUE(pGC);
}

static void
XineramaDestroyGC(GCPtr pGC)
{
    Xinerama_GC_FUNC_PROLOGUE(pGC);
    (*pGC->funcs->DestroyGC) (pGC);
    Xinerama_GC_FUNC_EPILOGUE(pGC);
}

static void
XineramaChangeGC(GCPtr pGC, unsigned long mask)
{
    Xinerama_GC_FUNC_PROLOGUE(pGC);

    if (mask & GCTileStipXOrigin)
        pGCPriv->patOrg.x = pGC->patOrg.x;
    if (mask & GCTileStipYOrigin)
        pGCPriv->patOrg.y = pGC->patOrg.y;
    if (mask & GCClipXOrigin)
        pGCPriv->clipOrg.x = pGC->clipOrg.x;
    if (mask & GCClipYOrigin)
        pGCPriv->clipOrg.y = pGC->clipOrg.y;

    (*pGC->funcs->ChangeGC) (pGC, mask);
    Xinerama_GC_FUNC_EPILOGUE(pGC);
}

static void
XineramaCopyGC(GCPtr pGCSrc, unsigned long mask, GCPtr pGCDst)
{
    PanoramiXGCPtr pSrcPriv = (PanoramiXGCPtr)
        dixLookupPrivate(&pGCSrc->devPrivates, &PanoramiXGCKeyRec);

    Xinerama_GC_FUNC_PROLOGUE(pGCDst);

    if (mask & GCTileStipXOrigin)
        pGCPriv->patOrg.x = pSrcPriv->patOrg.x;
    if (mask & GCTileStipYOrigin)
        pGCPriv->patOrg.y = pSrcPriv->patOrg.y;
    if (mask & GCClipXOrigin)
        pGCPriv->clipOrg.x = pSrcPriv->clipOrg.x;
    if (mask & GCClipYOrigin)
        pGCPriv->clipOrg.y = pSrcPriv->clipOrg.y;

    (*pGCDst->funcs->CopyGC) (pGCSrc, mask, pGCDst);
    Xinerama_GC_FUNC_EPILOGUE(pGCDst);
}

static void
XineramaChangeClip(GCPtr pGC, int type, void *pvalue, int nrects)
{
    Xinerama_GC_FUNC_PROLOGUE(pGC);
    (*pGC->funcs->ChangeClip) (pGC, type, pvalue, nrects);
    Xinerama_GC_FUNC_EPILOGUE(pGC);
}

static void
XineramaCopyClip(GCPtr pgcDst, GCPtr pgcSrc)
{
    Xinerama_GC_FUNC_PROLOGUE(pgcDst);
    (*pgcDst->funcs->CopyClip) (pgcDst, pgcSrc);
    Xinerama_GC_FUNC_EPILOGUE(pgcDst);
}

static void
XineramaDestroyClip(GCPtr pGC)
{
    Xinerama_GC_FUNC_PROLOGUE(pGC);
    (*pGC->funcs->DestroyClip) (pGC);
    Xinerama_GC_FUNC_EPILOGUE(pGC);
}

int
XineramaDeleteResource(void *data, XID id)
{
    free(data);
    return 1;
}

typedef struct {
    int screen;
    int id;
} PanoramiXSearchData;

static Bool
XineramaFindIDByScrnum(void *resource, XID id, void *privdata)
{
    PanoramiXRes *res = (PanoramiXRes *) resource;
    PanoramiXSearchData *data = (PanoramiXSearchData *) privdata;

    return res->info[data->screen].id == data->id;
}

PanoramiXRes *
PanoramiXFindIDByScrnum(RESTYPE type, XID id, int screen)
{
    PanoramiXSearchData data;
    void *val;

    if (!screen) {
        dixLookupResourceByType(&val, id, type, serverClient, DixReadAccess);
        return val;
    }

    data.screen = screen;
    data.id = id;

    return LookupClientResourceComplex(dixClientForXID(id), type,
                                       XineramaFindIDByScrnum, &data);
}

typedef struct _connect_callback_list {
    void (*func) (void);
    struct _connect_callback_list *next;
} XineramaConnectionCallbackList;

static XineramaConnectionCallbackList *ConnectionCallbackList = NULL;

Bool
XineramaRegisterConnectionBlockCallback(void (*func) (void))
{
    XineramaConnectionCallbackList *newlist;

    if (!(newlist = calloc(1, sizeof(XineramaConnectionCallbackList))))
        return FALSE;

    newlist->next = ConnectionCallbackList;
    newlist->func = func;
    ConnectionCallbackList = newlist;

    return TRUE;
}

static void
XineramaInitData(void)
{
    RegionNull(&PanoramiXScreenRegion);

    XINERAMA_FOR_EACH_SCREEN_BACKWARD({
        BoxRec TheBox;
        RegionRec ScreenRegion;

        TheBox.x1 = walkScreen->x;
        TheBox.x2 = TheBox.x1 + walkScreen->width;
        TheBox.y1 = walkScreen->y;
        TheBox.y2 = TheBox.y1 + walkScreen->height;

        RegionInit(&ScreenRegion, &TheBox, 1);
        RegionUnion(&PanoramiXScreenRegion, &PanoramiXScreenRegion,
                    &ScreenRegion);
        RegionUninit(&ScreenRegion);
    });

    ScreenPtr masterScreen = dixGetMasterScreen();

    PanoramiXPixWidth = masterScreen->x + masterScreen->width;
    PanoramiXPixHeight = masterScreen->y + masterScreen->height;

    XINERAMA_FOR_EACH_SCREEN_FORWARD_SKIP0({
        int w = walkScreen->x + walkScreen->width;
        int h = walkScreen->y + walkScreen->height;

        if (PanoramiXPixWidth < w)
            PanoramiXPixWidth = w;
        if (PanoramiXPixHeight < h)
            PanoramiXPixHeight = h;
    });
}

/*
 *	PanoramiXExtensionInit():
 *		Called from InitExtensions in main().
 *		Register PanoramiXeen Extension
 *		Initialize global variables.
 */

void
PanoramiXExtensionInit(void)
{
    int i;
    Bool success = FALSE;
    ScreenPtr masterScreen = dixGetMasterScreen();

    if (noPanoramiXExtension)
        return;

    if (!dixRegisterPrivateKey(&PanoramiXScreenKeyRec, PRIVATE_SCREEN, 0)) {
        noPanoramiXExtension = TRUE;
        return;
    }

    if (!dixRegisterPrivateKey
        (&PanoramiXGCKeyRec, PRIVATE_GC, sizeof(PanoramiXGCRec))) {
        noPanoramiXExtension = TRUE;
        return;
    }

    PanoramiXNumScreens = screenInfo.numScreens;
    if (PanoramiXNumScreens == 1) {     /* Only 1 screen        */
        noPanoramiXExtension = TRUE;
        return;
    }

    ExtensionEntry *extEntry = AddExtension(
        PANORAMIX_PROTOCOL_NAME, 0, 0,
        ProcPanoramiXDispatch,
        ProcPanoramiXDispatch,
        PanoramiXResetProc,
        StandardMinorOpcode);

    if (!extEntry)
        return;

    /*
     *      First make sure all the basic allocations succeed.  If not,
     *      run in non-PanoramiXeen mode.
     */
    XINERAMA_FOR_EACH_SCREEN_BACKWARD({
        PanoramiXScreenPtr pScreenPriv = calloc(1, sizeof(PanoramiXScreenRec));
        dixSetPrivate(&walkScreen->devPrivates, &PanoramiXScreenKeyRec,
                      pScreenPriv);
        if (!pScreenPriv) {
            noPanoramiXExtension = TRUE;
            return;
        }

        dixScreenHookClose(walkScreen, XineramaCloseScreen);
        pScreenPriv->CreateGC = masterScreen->CreateGC;
        walkScreen->CreateGC = XineramaCreateGC;
    });

    XRC_DRAWABLE = CreateNewResourceClass();
    XRT_WINDOW = CreateNewResourceType(XineramaDeleteResource,
                                           "XineramaWindow");
    if (XRT_WINDOW)
        XRT_WINDOW |= XRC_DRAWABLE;

    XRT_PIXMAP = CreateNewResourceType(XineramaDeleteResource,
                                           "XineramaPixmap");
    if (XRT_PIXMAP)
        XRT_PIXMAP |= XRC_DRAWABLE;

    XRT_GC = CreateNewResourceType(XineramaDeleteResource, "XineramaGC");
    XRT_COLORMAP = CreateNewResourceType(XineramaDeleteResource,
                                             "XineramaColormap");

    if (XRT_WINDOW && XRT_PIXMAP && XRT_GC && XRT_COLORMAP)
        success = TRUE;

    SetResourceTypeErrorValue(XRT_WINDOW, BadWindow);
    SetResourceTypeErrorValue(XRT_PIXMAP, BadPixmap);
    SetResourceTypeErrorValue(XRT_GC, BadGC);
    SetResourceTypeErrorValue(XRT_COLORMAP, BadColor);

    if (!success) {
        noPanoramiXExtension = TRUE;
        ErrorF(PANORAMIX_PROTOCOL_NAME " extension failed to initialize\n");
        return;
    }

    XineramaInitData();

    /*
     *  Put our processes into the ProcVector
     */

    for (i = 256; i--;)
        SavedProcVector[i] = ProcVector[i];

    ProcVector[X_CreateWindow] = PanoramiXCreateWindow;
    ProcVector[X_ChangeWindowAttributes] = PanoramiXChangeWindowAttributes;
    ProcVector[X_DestroyWindow] = PanoramiXDestroyWindow;
    ProcVector[X_DestroySubwindows] = PanoramiXDestroySubwindows;
    ProcVector[X_ChangeSaveSet] = PanoramiXChangeSaveSet;
    ProcVector[X_ReparentWindow] = PanoramiXReparentWindow;
    ProcVector[X_MapWindow] = PanoramiXMapWindow;
    ProcVector[X_MapSubwindows] = PanoramiXMapSubwindows;
    ProcVector[X_UnmapWindow] = PanoramiXUnmapWindow;
    ProcVector[X_UnmapSubwindows] = PanoramiXUnmapSubwindows;
    ProcVector[X_ConfigureWindow] = PanoramiXConfigureWindow;
    ProcVector[X_CirculateWindow] = PanoramiXCirculateWindow;
    ProcVector[X_GetGeometry] = PanoramiXGetGeometry;
    ProcVector[X_TranslateCoords] = PanoramiXTranslateCoords;
    ProcVector[X_CreatePixmap] = PanoramiXCreatePixmap;
    ProcVector[X_FreePixmap] = PanoramiXFreePixmap;
    ProcVector[X_CreateGC] = PanoramiXCreateGC;
    ProcVector[X_ChangeGC] = PanoramiXChangeGC;
    ProcVector[X_CopyGC] = PanoramiXCopyGC;
    ProcVector[X_SetDashes] = PanoramiXSetDashes;
    ProcVector[X_SetClipRectangles] = PanoramiXSetClipRectangles;
    ProcVector[X_FreeGC] = PanoramiXFreeGC;
    ProcVector[X_ClearArea] = PanoramiXClearToBackground;
    ProcVector[X_CopyArea] = PanoramiXCopyArea;
    ProcVector[X_CopyPlane] = PanoramiXCopyPlane;
    ProcVector[X_PolyPoint] = PanoramiXPolyPoint;
    ProcVector[X_PolyLine] = PanoramiXPolyLine;
    ProcVector[X_PolySegment] = PanoramiXPolySegment;
    ProcVector[X_PolyRectangle] = PanoramiXPolyRectangle;
    ProcVector[X_PolyArc] = PanoramiXPolyArc;
    ProcVector[X_FillPoly] = PanoramiXFillPoly;
    ProcVector[X_PolyFillRectangle] = PanoramiXPolyFillRectangle;
    ProcVector[X_PolyFillArc] = PanoramiXPolyFillArc;
    ProcVector[X_PutImage] = PanoramiXPutImage;
    ProcVector[X_GetImage] = PanoramiXGetImage;
    ProcVector[X_PolyText8] = PanoramiXPolyText8;
    ProcVector[X_PolyText16] = PanoramiXPolyText16;
    ProcVector[X_ImageText8] = PanoramiXImageText8;
    ProcVector[X_ImageText16] = PanoramiXImageText16;
    ProcVector[X_CreateColormap] = PanoramiXCreateColormap;
    ProcVector[X_FreeColormap] = PanoramiXFreeColormap;
    ProcVector[X_CopyColormapAndFree] = PanoramiXCopyColormapAndFree;
    ProcVector[X_InstallColormap] = PanoramiXInstallColormap;
    ProcVector[X_UninstallColormap] = PanoramiXUninstallColormap;
    ProcVector[X_AllocColor] = PanoramiXAllocColor;
    ProcVector[X_AllocNamedColor] = PanoramiXAllocNamedColor;
    ProcVector[X_AllocColorCells] = PanoramiXAllocColorCells;
    ProcVector[X_AllocColorPlanes] = PanoramiXAllocColorPlanes;
    ProcVector[X_FreeColors] = PanoramiXFreeColors;
    ProcVector[X_StoreColors] = PanoramiXStoreColors;
    ProcVector[X_StoreNamedColor] = PanoramiXStoreNamedColor;

    PanoramiXRenderInit();
    PanoramiXFixesInit();
    PanoramiXDamageInit();
    PanoramiXCompositeInit();
}

Bool
PanoramiXCreateConnectionBlock(void)
{
    int i, j, length;
    Bool disable_backing_store = FALSE;
    int old_width, old_height;
    float width_mult, height_mult;
    xWindowRoot *root;
    xVisualType *visual;
    xDepth *depth;
    VisualPtr pVisual;

    /*
     *  Do normal CreateConnectionBlock but faking it for only one screen
     */

    if (!PanoramiXNumDepths) {
        ErrorF("Xinerama error: No common visuals\n");
        return FALSE;
    }

    ScreenPtr masterScreen = dixGetMasterScreen();
    DIX_FOR_EACH_SCREEN({
        if (!walkScreenIdx)
            continue;  /* skip the first one */

        if (walkScreen->rootDepth != masterScreen->rootDepth) {
            ErrorF("Xinerama error: Root window depths differ\n");
            return FALSE;
        }
        if (walkScreen->backingStoreSupport !=
            masterScreen->backingStoreSupport)
            disable_backing_store = TRUE;
    });

    if (disable_backing_store) {
        DIX_FOR_EACH_SCREEN({
            walkScreen->backingStoreSupport = NotUseful;
        });
    }

    i = screenInfo.numScreens;
    screenInfo.numScreens = 1;
    if (!CreateConnectionBlock()) {
        screenInfo.numScreens = i;
        return FALSE;
    }

    screenInfo.numScreens = i;

    root = (xWindowRoot *) (ConnectionInfo + connBlockScreenStart);
    length = connBlockScreenStart + sizeof(xWindowRoot);

    /* overwrite the connection block */
    root->nDepths = PanoramiXNumDepths;

    for (unsigned int walkScreenIdx = 0; walkScreenIdx < PanoramiXNumDepths; walkScreenIdx++) {
        depth = (xDepth *) (ConnectionInfo + length);
        depth->depth = PanoramiXDepths[walkScreenIdx].depth;
        depth->nVisuals = PanoramiXDepths[walkScreenIdx].numVids;
        length += sizeof(xDepth);
        visual = (xVisualType *) (ConnectionInfo + length);

        for (j = 0; j < depth->nVisuals; j++, visual++) {
            visual->visualID = PanoramiXDepths[walkScreenIdx].vids[j];

            for (pVisual = PanoramiXVisuals;
                 pVisual->vid != visual->visualID; pVisual++);

            visual->class = pVisual->class;
            visual->bitsPerRGB = pVisual->bitsPerRGBValue;
            visual->colormapEntries = pVisual->ColormapEntries;
            visual->redMask = pVisual->redMask;
            visual->greenMask = pVisual->greenMask;
            visual->blueMask = pVisual->blueMask;
        }

        length += (depth->nVisuals * sizeof(xVisualType));
    }

    connSetupPrefix.length = bytes_to_int32(length);

    for (unsigned int walkScreenIdx = 0; walkScreenIdx < PanoramiXNumDepths; walkScreenIdx++)
        free(PanoramiXDepths[walkScreenIdx].vids);
    free(PanoramiXDepths);
    PanoramiXDepths = NULL;

    /*
     *  OK, change some dimensions so it looks as if it were one big screen
     */

    old_width = root->pixWidth;
    old_height = root->pixHeight;

    root->pixWidth = PanoramiXPixWidth;
    root->pixHeight = PanoramiXPixHeight;
    width_mult = (1.0 * root->pixWidth) / old_width;
    height_mult = (1.0 * root->pixHeight) / old_height;
    root->mmWidth *= width_mult;
    root->mmHeight *= height_mult;

    while (ConnectionCallbackList) {
        void *tmp;

        tmp = (void *) ConnectionCallbackList;
        (*ConnectionCallbackList->func) ();
        ConnectionCallbackList = ConnectionCallbackList->next;
        free(tmp);
    }

    return TRUE;
}

/*
 * This isn't just memcmp(), bitsPerRGBValue is skipped.  markv made that
 * change way back before xf86 4.0, but the comment for _why_ is a bit
 * opaque, so I'm not going to question it for now.
 *
 * This is probably better done as a screen hook so DBE/EVI/GLX can add
 * their own tests, and adding privates to VisualRec so they don't have to
 * do their own back-mapping.
 */
static Bool
VisualsEqual(VisualPtr a, ScreenPtr pScreenB, VisualPtr b)
{
    return ((a->class == b->class) &&
            (a->ColormapEntries == b->ColormapEntries) &&
            (a->nplanes == b->nplanes) &&
            (a->redMask == b->redMask) &&
            (a->greenMask == b->greenMask) &&
            (a->blueMask == b->blueMask) &&
            (a->offsetRed == b->offsetRed) &&
            (a->offsetGreen == b->offsetGreen) &&
            (a->offsetBlue == b->offsetBlue));
}

static void
PanoramiXMaybeAddDepth(DepthPtr pDepth)
{
    int k;
    Bool found = FALSE;

    XINERAMA_FOR_EACH_SCREEN_FORWARD_SKIP0({
        for (k = 0; k < walkScreen->numDepths; k++) {
            if (walkScreen->allowedDepths[k].depth == pDepth->depth) {
                found = TRUE;
                break;
            }
        }
    });

    if (!found)
        return;

    int j = PanoramiXNumDepths;
    PanoramiXNumDepths++;
    PanoramiXDepths = XNFreallocarray(PanoramiXDepths,
                                      PanoramiXNumDepths, sizeof(DepthRec));
    PanoramiXDepths[j].depth = pDepth->depth;
    PanoramiXDepths[j].numVids = 0;
    PanoramiXDepths[j].vids = NULL;
}

static void
PanoramiXMaybeAddVisual(VisualPtr pVisual)
{
    int k;
    Bool found = FALSE;

    XINERAMA_FOR_EACH_SCREEN_FORWARD_SKIP0({
        found = FALSE;

        for (k = 0; k < walkScreen->numVisuals; k++) {
            VisualPtr candidate = &walkScreen->visuals[k];

            if ((*XineramaVisualsEqualPtr) (pVisual, walkScreen, candidate)) {
                found = TRUE;
                break;
            }
        }

        if (!found)
            return;
    });

    /* found a matching visual on all screens, add it to the subset list */
    int j = PanoramiXNumVisuals;
    PanoramiXNumVisuals++;
    PanoramiXVisuals = reallocarray(PanoramiXVisuals,
                                    PanoramiXNumVisuals, sizeof(VisualRec));

    memcpy(&PanoramiXVisuals[j], pVisual, sizeof(VisualRec));

    for (k = 0; k < PanoramiXNumDepths; k++) {
        if (PanoramiXDepths[k].depth == pVisual->nplanes) {
            PanoramiXDepths[k].vids = reallocarray(PanoramiXDepths[k].vids,
                                                   PanoramiXDepths[k].numVids + 1,
                                                   sizeof(VisualID));
            PanoramiXDepths[k].vids[PanoramiXDepths[k].numVids] = pVisual->vid;
            PanoramiXDepths[k].numVids++;
            break;
        }
    }
}

extern void
PanoramiXConsolidate(void)
{
    ScreenPtr masterScreen = dixGetMasterScreen();
    DepthPtr pDepth = masterScreen->allowedDepths;
    VisualPtr pVisual = masterScreen->visuals;

    PanoramiXNumDepths = 0;
    PanoramiXNumVisuals = 0;

    for (int i = 0; i < masterScreen->numDepths; i++)
        PanoramiXMaybeAddDepth(pDepth++);

    for (int i = 0; i < masterScreen->numVisuals; i++)
        PanoramiXMaybeAddVisual(pVisual++);

    PanoramiXRes *root = calloc(1, sizeof(PanoramiXRes));
    if (!root)
        return;

    root->type = XRT_WINDOW;
    PanoramiXRes *defmap = calloc(1, sizeof(PanoramiXRes));
    if (!defmap) {
        free(root);
        return;
    }
    defmap->type = XRT_COLORMAP;
    PanoramiXRes *saver = calloc(1, sizeof(PanoramiXRes));
    if (!saver) {
        free(root);
        free(defmap);
        return;
    }
    saver->type = XRT_WINDOW;

    XINERAMA_FOR_EACH_SCREEN_BACKWARD({
        root->info[walkScreenIdx].id = walkScreen->root->drawable.id;
        root->u.win.class = InputOutput;
        root->u.win.root = TRUE;
        saver->info[walkScreenIdx].id = walkScreen->screensaver.wid;
        saver->u.win.class = InputOutput;
        saver->u.win.root = TRUE;
        defmap->info[walkScreenIdx].id = walkScreen->defColormap;
    });

    AddResource(root->info[0].id, XRT_WINDOW, root);
    AddResource(saver->info[0].id, XRT_WINDOW, saver);
    AddResource(defmap->info[0].id, XRT_COLORMAP, defmap);
}

VisualID
PanoramiXTranslateVisualID(int screen, VisualID orig)
{
    ScreenPtr pOtherScreen = dixGetScreenPtr(screen);
    VisualPtr pVisual = NULL;
    int i;

    for (i = 0; i < PanoramiXNumVisuals; i++) {
        if (orig == PanoramiXVisuals[i].vid) {
            pVisual = &PanoramiXVisuals[i];
            break;
        }
    }

    if (!pVisual)
        return 0;

    /* if screen is 0, orig is already the correct visual ID */
    if (screen == 0)
        return orig;

    /* found the original, now translate it relative to the backend screen */
    for (i = 0; i < pOtherScreen->numVisuals; i++) {
        VisualPtr pOtherVisual = &pOtherScreen->visuals[i];

        if ((*XineramaVisualsEqualPtr) (pVisual, pOtherScreen, pOtherVisual))
            return pOtherVisual->vid;
    }

    return 0;
}

/*
 *	PanoramiXResetProc()
 *		Exit, deallocating as needed.
 */

static void
PanoramiXResetProc(ExtensionEntry * extEntry)
{
    int i;

    PanoramiXRenderReset();
    PanoramiXFixesReset();
    PanoramiXDamageReset();
    PanoramiXCompositeReset ();
    screenInfo.numScreens = PanoramiXNumScreens;
    for (i = 256; i--;)
        ProcVector[i] = SavedProcVector[i];
}

int
ProcPanoramiXQueryVersion(ClientPtr client)
{
    X_REQUEST_HEAD_STRUCT(xPanoramiXQueryVersionReq);

    xPanoramiXQueryVersionReply reply = {
        .majorVersion = SERVER_PANORAMIX_MAJOR_VERSION,
        .minorVersion = SERVER_PANORAMIX_MINOR_VERSION
    };

    if (client->swapped) {
        swaps(&reply.majorVersion);
        swaps(&reply.minorVersion);
    }

    return X_SEND_REPLY_SIMPLE(client, reply);
}

int
ProcPanoramiXGetState(ClientPtr client)
{
    X_REQUEST_HEAD_STRUCT(xPanoramiXGetStateReq);
    X_REQUEST_FIELD_CARD32(window);

    WindowPtr pWin;
    int rc;

    rc = dixLookupWindow(&pWin, stuff->window, client, DixGetAttrAccess);
    if (rc != Success)
        return rc;

    xPanoramiXGetStateReply reply = {
        .state = !noPanoramiXExtension,
        .window = stuff->window
    };

    if (client->swapped) {
        swapl(&reply.window);
    }

    return X_SEND_REPLY_SIMPLE(client, reply);
}

int
ProcPanoramiXGetScreenCount(ClientPtr client)
{
    X_REQUEST_HEAD_STRUCT(xPanoramiXGetScreenCountReq);
    X_REQUEST_FIELD_CARD32(window);

    WindowPtr pWin;
    int rc;

    rc = dixLookupWindow(&pWin, stuff->window, client, DixGetAttrAccess);
    if (rc != Success)
        return rc;

    xPanoramiXGetScreenCountReply reply = {
        .ScreenCount = PanoramiXNumScreens,
        .window = stuff->window
    };

    if (client->swapped) {
        swapl(&reply.window);
    }

    return X_SEND_REPLY_SIMPLE(client, reply);
}

int
ProcPanoramiXGetScreenSize(ClientPtr client)
{
    X_REQUEST_HEAD_STRUCT(xPanoramiXGetScreenSizeReq);
    X_REQUEST_FIELD_CARD32(window);
    X_REQUEST_FIELD_CARD32(screen);

    WindowPtr pWin;
    int rc;

    if (stuff->screen >= PanoramiXNumScreens)
        return BadMatch;

    rc = dixLookupWindow(&pWin, stuff->window, client, DixGetAttrAccess);
    if (rc != Success)
        return rc;

    ScreenPtr pScreen = dixGetScreenPtr(stuff->screen);

    xPanoramiXGetScreenSizeReply reply = {
        /* screen dimensions */
        .width = pScreen->width,
        .height = pScreen->height,
        .window = stuff->window,
        .screen = stuff->screen
    };

    if (client->swapped) {
        swapl(&reply.width);
        swapl(&reply.height);
        swapl(&reply.window);
        swapl(&reply.screen);
    }

    return X_SEND_REPLY_SIMPLE(client, reply);
}

int
ProcXineramaIsActive(ClientPtr client)
{
    X_REQUEST_HEAD_STRUCT(xXineramaIsActiveReq);

    xXineramaIsActiveReply reply = {
#if 1
        /* The following hack fools clients into thinking that Xinerama
         * is disabled even though it is not. */
        .state = !noPanoramiXExtension && !PanoramiXExtensionDisabledHack
#else
        .state = !noPanoramiXExtension;
#endif
    };

    if (client->swapped) {
        swapl(&reply.state);
    }

    return X_SEND_REPLY_SIMPLE(client, reply);
}

int
ProcXineramaQueryScreens(ClientPtr client)
{
    X_REQUEST_HEAD_STRUCT(xXineramaQueryScreensReq);

    CARD32 number = (noPanoramiXExtension) ? 0 : PanoramiXNumScreens;
    xXineramaQueryScreensReply reply = {
        .number = number
    };

    if (client->swapped) {
        swapl(&reply.number);
    }

    x_rpcbuf_t rpcbuf = { .swapped = client->swapped, .err_clear = TRUE };

    if (!noPanoramiXExtension) {
        XINERAMA_FOR_EACH_SCREEN_BACKWARD({
            /* xXineramaScreenInfo is the same as xRectangle */
            x_rpcbuf_write_rect(&rpcbuf,
                                walkScreen->x,
                                walkScreen->y,
                                walkScreen->width,
                                walkScreen->height);
        });
    }

    return X_SEND_REPLY_WITH_RPCBUF(client, reply, rpcbuf);
}

static int
ProcPanoramiXDispatch(ClientPtr client)
{
    REQUEST(xReq);
    switch (stuff->data) {
    case X_PanoramiXQueryVersion:
        return ProcPanoramiXQueryVersion(client);
    case X_PanoramiXGetState:
        return ProcPanoramiXGetState(client);
    case X_PanoramiXGetScreenCount:
        return ProcPanoramiXGetScreenCount(client);
    case X_PanoramiXGetScreenSize:
        return ProcPanoramiXGetScreenSize(client);
    case X_XineramaIsActive:
        return ProcXineramaIsActive(client);
    case X_XineramaQueryScreens:
        return ProcXineramaQueryScreens(client);
    }
    return BadRequest;
}

#if X_BYTE_ORDER == X_LITTLE_ENDIAN
#define SHIFT_L(v,s) (v) << (s)
#define SHIFT_R(v,s) (v) >> (s)
#else
#define SHIFT_L(v,s) (v) >> (s)
#define SHIFT_R(v,s) (v) << (s)
#endif

static void
CopyBits(char *dst, int shiftL, char *src, int bytes)
{
    /* Just get it to work.  Worry about speed later */
    int shiftR = 8 - shiftL;

    while (bytes--) {
        *dst |= SHIFT_L(*src, shiftL);
        *(dst + 1) |= SHIFT_R(*src, shiftR);
        dst++;
        src++;
    }
}

/* Caution.  This doesn't support 2 and 4 bpp formats.  We expect
   1 bpp and planar data to be already cleared when presented
   to this function */

static Bool XineramaGetImageDataScr(BoxRec SrcBox,
                                    RegionPtr GrabRegion,
                                    RegionPtr SrcRegion,
                                    const int width,
                                    const int height,
                                    const unsigned int format,
                                    const unsigned long planemask,
                                    char *data,
                                    const int depth,
                                    const int pitch,
                                    ScreenPtr walkScreen,
                                    DrawablePtr pWalkDraw)
{
        BoxRec TheBox;

        ScreenPtr pScreen = pWalkDraw->pScreen;

        TheBox.x1 = pScreen->x;
        TheBox.x2 = TheBox.x1 + pScreen->width;
        TheBox.y1 = pScreen->y;
        TheBox.y2 = TheBox.y1 + pScreen->height;

        RegionRec ScreenRegion;
        RegionInit(&ScreenRegion, &TheBox, 1);
        int inOut = RegionContainsRect(&ScreenRegion, &SrcBox);
        if (inOut == rgnPART)
            RegionIntersect(GrabRegion, SrcRegion, &ScreenRegion);
        RegionUninit(&ScreenRegion);

        if (inOut == rgnIN) {
            pScreen->GetImage(pWalkDraw,
                                  SrcBox.x1 - pWalkDraw->x -
                                  walkScreen->x,
                                  SrcBox.y1 - pWalkDraw->y -
                                  walkScreen->y, width, height,
                                  format, planemask, data);
            return FALSE;
        }
        else if (inOut == rgnOUT)
            return TRUE;

        int nbox = RegionNumRects(GrabRegion);
        if (!nbox)
            return TRUE;

        BoxRec *pbox = RegionRects(GrabRegion);

        int size = 0;
        char *ScratchMem = NULL;

        while (nbox--) {
            int w = pbox->x2 - pbox->x1;
            int h = pbox->y2 - pbox->y1;
            int ScratchPitch = PixmapBytePad(w, depth);
            int sizeNeeded = ScratchPitch * h;

            if (sizeNeeded > size) {
                char *tmpdata = ScratchMem;

                ScratchMem = realloc(ScratchMem, sizeNeeded);
                if (ScratchMem)
                    size = sizeNeeded;
                else {
                    ScratchMem = tmpdata;
                    break;
                }
            }

            int x = pbox->x1 - pWalkDraw->x - walkScreen->x;
            int y = pbox->y1 - pWalkDraw->y - walkScreen->y;

            (*pScreen->GetImage) (pWalkDraw, x, y, w, h,
                                  format, planemask, ScratchMem);

            /* copy the memory over */

            if (depth == 1) {
                int shift, leftover;

                x = pbox->x1 - SrcBox.x1;
                y = pbox->y1 - SrcBox.y1;
                shift = x & 7;
                x >>= 3;
                leftover = w & 7;
                w >>= 3;

                /* clean up the edge */
                if (leftover) {
                    int mask = (1 << leftover) - 1;

                    for (int j = h, k = w; j--; k += ScratchPitch)
                        ScratchMem[k] &= mask;
                }

                for (int j = 0, index = (pitch * y) + x, index2 = 0; j < h;
                         j++, index += pitch, index2 += ScratchPitch) {
                    if (w) {
                        if (!shift) {
                            assert(ScratchMem);
                            memcpy(data + index, ScratchMem + index2, w);
                        }
                        else {
                            assert(ScratchMem);
                            CopyBits(data + index, shift,
                                     ScratchMem + index2, w);
                        }
                    }

                    if (leftover) {
                        data[index + w] |=
                            SHIFT_L(ScratchMem[index2 + w], shift);
                        if ((shift + leftover) > 8)
                            data[index + w + 1] |=
                                SHIFT_R(ScratchMem[index2 + w],
                                        (8 - shift));
                    }
                }
            }
            else {
                int bpp = BitsPerPixel(depth) >> 3;
                x = (pbox->x1 - SrcBox.x1) * bpp;
                y = pbox->y1 - SrcBox.y1;
                w *= bpp;

                for (int j = 0; j < h; j++) {
                    assert(ScratchMem);
                    memcpy(data + (pitch * (y + j)) + x,
                           ScratchMem + (ScratchPitch * j), w);
                }
            }
            pbox++;
        }

        free(ScratchMem);
        RegionSubtract(SrcRegion, SrcRegion, GrabRegion);
        if (!RegionNotEmpty(SrcRegion))
            return FALSE;

    return TRUE;
}

void
XineramaGetImageData(DrawablePtr *pDrawables,
                     int left,
                     int top,
                     int width,
                     int height,
                     unsigned int format,
                     unsigned long planemask,
                     char *data, int pitch, Bool isRoot)
{
    RegionRec SrcRegion, GrabRegion;
    BoxRec SrcBox;
    DrawablePtr pDraw = pDrawables[0];

    /* find box in logical screen space */
    SrcBox.x1 = left;
    SrcBox.y1 = top;
    if (!isRoot) {
        ScreenPtr masterScreen = dixGetMasterScreen();
        SrcBox.x1 += pDraw->x + masterScreen->x;
        SrcBox.y1 += pDraw->y + masterScreen->y;
    }
    SrcBox.x2 = SrcBox.x1 + width;
    SrcBox.y2 = SrcBox.y1 + height;

    RegionInit(&SrcRegion, &SrcBox, 1);
    RegionNull(&GrabRegion);

    int depth = (format == XYPixmap) ? 1 : pDraw->depth;

    XINERAMA_FOR_EACH_SCREEN_BACKWARD({
        if (!XineramaGetImageDataScr(
                SrcBox,
                &GrabRegion,
                &SrcRegion,
                width,
                height,
                format,
                planemask,
                data,
                depth,
                pitch,
                walkScreen,
                pDrawables[walkScreenIdx]))
            break;
    });

    RegionUninit(&SrcRegion);
    RegionUninit(&GrabRegion);
}

// work around broken X11 proto headers
#define sz_xXineramaQueryScreensReply sz_XineramaQueryScreensReply
#define sz_xXineramaIsActiveReply sz_XineramaIsActiveReply
#define sz_xPanoramiXGetScreenSizeReply sz_panoramiXGetScreenSizeReply
#define sz_xPanoramiXGetScreenCountReply sz_panoramiXGetScreenCountReply
#define sz_xPanoramiXGetStateReply sz_panoramiXGetStateReply

XTYPE_SIZE_ASSERT(xPanoramiXQueryVersionReply);
XTYPE_SIZE_ASSERT(xPanoramiXGetStateReply);
XTYPE_SIZE_ASSERT(xPanoramiXGetScreenCountReply);
XTYPE_SIZE_ASSERT(xPanoramiXGetScreenSizeReply);
XTYPE_SIZE_ASSERT(xXineramaIsActiveReply);
XTYPE_SIZE_ASSERT(xTranslateCoordsReply);
XTYPE_SIZE_ASSERT(xXineramaQueryScreensReply);
XTYPE_SIZE_ASSERT(xGetGeometryReply);
XTYPE_SIZE_ASSERT(xGetImageReply);

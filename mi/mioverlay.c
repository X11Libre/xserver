
#include <dix-config.h>

#include <X11/X.h>
#include <X11/Xmd.h>
#include <X11/extensions/shapeproto.h>

#include "dix/cursor_priv.h"
#include "dix/dix_priv.h"
#include "dix/screen_hooks_priv.h"
#include "dix/screensaver_priv.h"
#include "dix/window_priv.h"
#include "mi/mi_priv.h"

#include "scrnintstr.h"
#include "validate.h"
#include "windowstr.h"
#include "gcstruct.h"
#include "regionstr.h"
#include "privates.h"
#include "mivalidate.h"
#include "mioverlay.h"
#include "migc.h"

#include "globals.h"

typedef struct {
    RegionRec exposed;
    RegionRec borderExposed;
    RegionPtr borderVisible;
    DDXPointRec oldAbsCorner;
} miOverlayValDataRec, *miOverlayValDataPtr;

typedef struct _TreeRec {
    WindowPtr pWin;
    struct _TreeRec *parent;
    struct _TreeRec *firstChild;
    struct _TreeRec *lastChild;
    struct _TreeRec *prevSib;
    struct _TreeRec *nextSib;
    RegionRec borderClip;
    RegionRec clipList;
    unsigned visibility;
    miOverlayValDataPtr valdata;
} miOverlayTreeRec, *miOverlayTreePtr;

typedef struct {
    miOverlayTreePtr tree;
} miOverlayWindowRec, *miOverlayWindowPtr;

typedef struct {
    CreateWindowProcPtr CreateWindow;
    DestroyWindowProcPtr DestroyWindow;
    UnrealizeWindowProcPtr UnrealizeWindow;
    RealizeWindowProcPtr RealizeWindow;
    miOverlayTransFunc MakeTransparent;
    miOverlayInOverlayFunc InOverlay;
    Bool underlayMarked;
    Bool copyUnderlay;
} miOverlayScreenRec, *miOverlayScreenPtr;

#define miOverlayWindowKey (&miOverlayWindowKeyRec)

#define miOverlayScreenKey (&miOverlayScreenKeyRec)

#define MIOVERLAY_GET_SCREEN_PRIVATE(pScreen) ((miOverlayScreenPtr) \
	dixLookupPrivate(&(pScreen)->devPrivates, miOverlayScreenKey))
#define MIOVERLAY_GET_WINDOW_PRIVATE(pWin) ((miOverlayWindowPtr) \
	dixLookupPrivate(&(pWin)->devPrivates, miOverlayWindowKey))
#define MIOVERLAY_GET_WINDOW_TREE(pWin) \
	(MIOVERLAY_GET_WINDOW_PRIVATE(pWin)->tree)

#define IN_UNDERLAY(w) MIOVERLAY_GET_WINDOW_TREE(w)
#define IN_OVERLAY(w) !MIOVERLAY_GET_WINDOW_TREE(w)

#define MARK_OVERLAY(w) miMarkWindow(w)
#define MARK_UNDERLAY(w) MarkUnderlayWindow(w)

#define HasParentRelativeBorder(w) (!(w)->borderIsPixel && \
                                    HasBorder(w) && \
                                    (w)->backgroundState == ParentRelative)

#ifndef RECTLIMIT
#define RECTLIMIT 25
#endif

typedef struct {
    RegionPtr over;
    RegionPtr under;
} miOverlayTwoRegions;

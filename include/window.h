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

#ifndef WINDOW_H
#define WINDOW_H

#include <X11/Xproto.h>

#include "misc.h"
#include "regionstr.h"
#include "screenint.h"

#define TOTALLY_OBSCURED 0
#define UNOBSCURED 1
#define OBSCURED 2

#define VisibilityNotViewable	3

/* return values for tree-walking callback procedures */
#define WT_STOPWALKING		0
#define WT_WALKCHILDREN		1
#define WT_DONTWALKCHILDREN	2
#define WT_NOMATCH 3
#define NullWindow ((WindowPtr) 0)

/* Forward declaration, we can't include input.h here */
struct _DeviceIntRec;
struct _Cursor;

typedef struct _BackingStore *BackingStorePtr;
typedef struct _Window *WindowPtr;
typedef struct _Property *PropertyPtr;

enum RootClipMode {
    ROOT_CLIP_NONE = 0, /**< resize the root window to 0x0 */
    ROOT_CLIP_FULL = 1, /**< resize the root window to fit screen */
    ROOT_CLIP_INPUT_ONLY = 2, /**< as above, but no rendering to screen */
};

typedef int (*VisitWindowProcPtr) (WindowPtr pWin,
                                   void *data);

extern _X_EXPORT int TraverseTree(WindowPtr pWin,
                                  VisitWindowProcPtr func,
                                  void *data);

extern _X_EXPORT int WalkTree(ScreenPtr pScreen,
                              VisitWindowProcPtr func,
                              void *data);

Bool CreateRootWindow(ScreenPtr /*pScreen */ );

void InitRootWindow(WindowPtr /*pWin */ );

typedef WindowPtr (*RealChildHeadProc) (WindowPtr pWin);

void RegisterRealChildHeadProc(RealChildHeadProc proc);

WindowPtr RealChildHead(WindowPtr /*pWin */ );

int DeleteWindow(void *pWin,
                                  XID wid);

int DestroySubwindows(WindowPtr /*pWin */ ,
                                       ClientPtr /*client */ );

/* Quartz support on Mac OS X uses the HIToolbox
   framework whose ChangeWindowAttributes function conflicts here. */
#ifdef __APPLE__
#define ChangeWindowAttributes Darwin_X_ChangeWindowAttributes
#endif
extern _X_EXPORT int ChangeWindowAttributes(WindowPtr /*pWin */ ,
                                            Mask /*vmask */ ,
                                            XID * /*vlist */ ,
                                            ClientPtr /*client */ );

int ChangeWindowDeviceCursor(WindowPtr /*pWin */ ,
                                              struct _DeviceIntRec * /*pDev */ ,
                                              struct _Cursor * /*pCursor */ );

struct _Cursor *WindowGetDeviceCursor(WindowPtr /*pWin */ ,
                                                       struct _DeviceIntRec *
                                                       /*pDev */ );

/* Quartz support on Mac OS X uses the HIToolbox
   framework whose GetWindowAttributes function conflicts here. */
#ifdef __APPLE__
#define GetWindowAttributes(w,c,x) Darwin_X_GetWindowAttributes(w,c,x)
extern void Darwin_X_GetWindowAttributes(
#else
extern _X_EXPORT void GetWindowAttributes(
#endif
                                             WindowPtr /*pWin */ ,
                                             ClientPtr /*client */ ,
                                             xGetWindowAttributesReply *
                                             /* wa */ );

void GravityTranslate(int /*x */ ,
                                       int /*y */ ,
                                       int /*oldx */ ,
                                       int /*oldy */ ,
                                       int /*dw */ ,
                                       int /*dh */ ,
                                       unsigned /*gravity */ ,
                                       int * /*destx */ ,
                                       int * /*desty */ );

int ConfigureWindow(WindowPtr /*pWin */ ,
                                     Mask /*mask */ ,
                                     XID * /*vlist */ ,
                                     ClientPtr /*client */ );

int CirculateWindow(WindowPtr /*pParent */ ,
                                     int /*direction */ ,
                                     ClientPtr /*client */ );

// FIXME
extern _X_EXPORT int ReparentWindow(WindowPtr /*pWin */ ,
                                    WindowPtr /*pParent */ ,
                                    int /*x */ ,
                                    int /*y */ ,
                                    ClientPtr /*client */ );

int MapWindow(WindowPtr /*pWin */ ,
                               ClientPtr /*client */ );

void MapSubwindows(WindowPtr /*pParent */ ,
                                    ClientPtr /*client */ );

int UnmapWindow(WindowPtr /*pWin */ ,
                                 Bool /*fromConfigure */ );

void UnmapSubwindows(WindowPtr /*pWin */ );

void HandleSaveSet(ClientPtr /*client */ );

Bool PointInWindowIsVisible(WindowPtr /*pWin */ ,
                                             int /*x */ ,
                                             int /*y */ );

// FIXME:intel
extern _X_EXPORT RegionPtr NotClippedByChildren(WindowPtr /*pWin */ );

void SendVisibilityNotify(WindowPtr /*pWin */ );

int dixSaveScreens(ClientPtr client, int on, int mode);

// fixme: intel
extern _X_EXPORT int SaveScreens(int on, int mode);

extern _X_EXPORT WindowPtr FindWindowWithOptional(WindowPtr /*w */ );

void CheckWindowOptionalNeed(WindowPtr /*w */ );

WindowPtr MoveWindowInStack(WindowPtr /*pWin */ ,
                                             WindowPtr /*pNextSib */ );

void SetWinSize(WindowPtr /*pWin */ );

void SetBorderSize(WindowPtr /*pWin */ );

void ResizeChildrenWinSize(WindowPtr /*pWin */ ,
                                            int /*dx */ ,
                                            int /*dy */ ,
                                            int /*dw */ ,
                                            int /*dh */ );

void SendShapeNotify(WindowPtr /* pWin */ ,
                                      int /* which */);

RegionPtr CreateBoundingShape(WindowPtr /* pWin */ );

RegionPtr CreateClipShape(WindowPtr /* pWin */ );

extern _X_EXPORT void SetRootClip(ScreenPtr pScreen, int enable);

VisualPtr WindowGetVisual(WindowPtr /*pWin*/);
#endif                          /* WINDOW_H */

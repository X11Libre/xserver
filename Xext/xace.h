/************************************************************

Author: Eamon Walsh <ewalsh@tycho.nsa.gov>

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
this permission notice appear in supporting documentation.  This permission
notice shall be included in all copies or substantial portions of the
Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHOR BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

********************************************************/

#ifndef _XACE_H
#define _XACE_H

#ifdef XACE

#define XACE_MAJOR_VERSION		2
#define XACE_MINOR_VERSION		0

#include "extnsionst.h"
#include "pixmap.h"
#include "region.h"
#include "window.h"
#include "property.h"
#include "selection.h"

/* Default window background */
#define XaceBackgroundNoneState(w) ((w)->forcedBG ? BackgroundPixel : None)

/* security hooks */
/* Constants used to identify the available security hooks
 */
#define XACE_EXT_DISPATCH		1
#define XACE_RESOURCE_ACCESS		2
#define XACE_DEVICE_ACCESS		3
#define XACE_PROPERTY_ACCESS		4
#define XACE_SEND_ACCESS		5
#define XACE_RECEIVE_ACCESS		6
#define XACE_CLIENT_ACCESS		7
#define XACE_EXT_ACCESS			8
#define XACE_SERVER_ACCESS		9
#define XACE_SELECTION_ACCESS		10
#define XACE_SCREEN_ACCESS		11
#define XACE_SCREENSAVER_ACCESS		12
#define XACE_AUTH_AVAIL			13
#define XACE_KEY_AVAIL			14
#define XACE_NUM_HOOKS			15

extern CallbackListPtr XaceHooks[XACE_NUM_HOOKS];

/* determine whether any callbacks are present for the XACE hook */
int XaceHookIsSet(int hook);

/* Special-cased hook functions
 */
int XaceHookDispatch0(ClientPtr ptr, int major);
#define XaceHookDispatch(c, m) \
    ((XaceHooks[XACE_EXT_DISPATCH] && (m) >= EXTENSION_BASE) ? \
    XaceHookDispatch0((c), (m)) : \
    Success)

int XaceHookPropertyAccess(ClientPtr ptr, WindowPtr pWin, PropertyPtr *ppProp,
                           Mask access_mode);
int XaceHookSelectionAccess(ClientPtr ptr, Selection ** ppSel, Mask access_mode);

/* needs to be exported for in-tree modsetting, but not part of public API */
_X_EXPORT int XaceHookResourceAccess(ClientPtr client, XID id, RESTYPE rtype, void *res,
                           RESTYPE ptype, void *parent, Mask access_mode);

int XaceHookDeviceAccess(ClientPtr client, DeviceIntPtr dev, Mask access_mode);

int XaceHookSendAccess(ClientPtr client, DeviceIntPtr dev, WindowPtr win,
                       xEventPtr ev, int count);
int XaceHookReceiveAccess(ClientPtr client, WindowPtr win, xEventPtr ev, int count);
int XaceHookClientAccess(ClientPtr client, ClientPtr target, Mask access_mode);
int XaceHookExtAccess(ClientPtr client, ExtensionEntry *ext);
int XaceHookServerAccess(ClientPtr client, Mask access_mode);
int XaceHookScreenAccess(ClientPtr client, ScreenPtr screen, Mask access_mode);
int XaceHookScreensaverAccess(ClientPtr client, ScreenPtr screen, Mask access_mode);
int XaceHookAuthAvail(ClientPtr client, XID authId);
int XaceHookKeyAvail(xEventPtr ev, DeviceIntPtr dev, int count);


/* Register a callback for a given hook.
 */
#define XaceRegisterCallback(hook,callback,data) \
    AddCallback(XaceHooks+(hook), callback, data)

/* Unregister an existing callback for a given hook.
 */
#define XaceDeleteCallback(hook,callback,data) \
    DeleteCallback(XaceHooks+(hook), callback, data)

/* XTrans wrappers for use by security modules
 */
int XaceGetConnectionNumber(ClientPtr ptr);
int XaceIsLocal(ClientPtr ptr);

/* From the original Security extension...
 */

void XaceCensorImage(ClientPtr client,
                     RegionPtr pVisibleRegion,
                     long widthBytesLine,
                     DrawablePtr pDraw,
                     int x, int y, int w, int h,
                     unsigned int format, char *pBuf);

#else                           /* XACE */

/* Default window background */
#define XaceBackgroundNoneState(w)		None

/* Define calls away when XACE is not being built. */

#ifdef __GNUC__
#define XaceHookIsSet(args...) 0
#define XaceHookDispatch(args...) Success
#define XaceHookPropertyAccess(args...) Success
#define XaceHookSelectionAccess(args...) Success
#define XaceCensorImage(args...) { ; }
#else
#define XaceHookIsSet(...) 0
#define XaceHookDispatch(...) Success
#define XaceHookPropertyAccess(...) Success
#define XaceHookSelectionAccess(...) Success
#define XaceCensorImage(...) { ; }
#endif

#endif                          /* XACE */

#endif                          /* _XACE_H */

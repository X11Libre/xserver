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

#define XACE_MAJOR_VERSION		2
#define XACE_MINOR_VERSION		0

#include "dix/selection_priv.h"
#include "include/callback.h"
#include "include/regionstr.h"

#include "extnsionst.h"
#include "pixmap.h"
#include "window.h"
#include "property.h"

/* Default window background */
#define XaceBackgroundNoneState(w) ((w)->forcedBG ? BackgroundPixel : None)

/* security hooks */
/* Constants used to identify the available security hooks
 */
#define XACE_RESOURCE_ACCESS		2
#define XACE_PROPERTY_ACCESS		4
#define XACE_SEND_ACCESS		5
#define XACE_RECEIVE_ACCESS		6
#define XACE_SELECTION_ACCESS		10
#define XACE_NUM_HOOKS			13

extern CallbackListPtr XaceHooks[XACE_NUM_HOOKS];

/* Entry point for hook functions.  Called by Xserver.
 * Required by several modules
 */
_X_EXPORT Bool XaceRegisterCallback(int hook, CallbackProcPtr callback, void *data);
_X_EXPORT Bool XaceDeleteCallback(int hook, CallbackProcPtr callback, void *data);

/* determine whether any callbacks are present for the XACE hook */
int XaceHookIsSet(int hook);

/* Special-cased hook functions
 */
int XaceHookPropertyAccess(ClientPtr ptr, WindowPtr pWin, PropertyPtr *ppProp,
                           Mask access_mode);
int XaceHookSelectionAccess(ClientPtr ptr, Selection ** ppSel, Mask access_mode);

/* needs to be exported for in-tree modesetting, but not part of public API */
_X_EXPORT int XaceHookResourceAccess(ClientPtr client, XID id, RESTYPE rtype, void *res,
                           RESTYPE ptype, void *parent, Mask access_mode);

int XaceHookSendAccess(ClientPtr client, DeviceIntPtr dev, WindowPtr win,
                       xEventPtr ev, int count);
int XaceHookReceiveAccess(ClientPtr client, WindowPtr win, xEventPtr ev, int count);

/* Register / unregister a callback for a given hook. */

/* From the original Security extension...
 */

void XaceCensorImage(ClientPtr client,
                     RegionPtr pVisibleRegion,
                     long widthBytesLine,
                     DrawablePtr pDraw,
                     int x, int y, int w, int h,
                     unsigned int format, char *pBuf);

#endif                          /* _XACE_H */

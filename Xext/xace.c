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

#include <dix-config.h>

#include <stdarg.h>

#include "os/client_priv.h"

#include "scrnintstr.h"
#include "extnsionst.h"
#include "pixmapstr.h"
#include "regionstr.h"
#include "gcstruct.h"
#include "xacestr.h"

CallbackListPtr XaceHooks[XACE_NUM_HOOKS] = { 0 };

/* Special-cased hook functions.  Called by Xserver.
 */
int
XaceHookDispatch0(ClientPtr client, int major)
{
    /* Call the extension dispatch hook */
    ExtensionEntry *ext = GetExtensionEntry(major);
    XaceExtAccessRec erec = { client, ext, DixUseAccess, Success };
    if (ext)
        CallCallbacks(&XaceHooks[XACE_EXT_DISPATCH], &erec);
    /* On error, pretend extension doesn't exist */
    return (erec.status == Success) ? Success : BadRequest;
}

int
XaceHookPropertyAccess(ClientPtr client, WindowPtr pWin,
                       PropertyPtr *ppProp, Mask access_mode)
{
    XacePropertyAccessRec rec = { client, pWin, ppProp, access_mode, Success };
    CallCallbacks(&XaceHooks[XACE_PROPERTY_ACCESS], &rec);
    return rec.status;
}

int
XaceHookSelectionAccess(ClientPtr client, Selection ** ppSel, Mask access_mode)
{
    XaceSelectionAccessRec rec = { client, ppSel, access_mode, Success };
    CallCallbacks(&XaceHooks[XACE_SELECTION_ACCESS], &rec);
    return rec.status;
}

int XaceHookResourceAccess(ClientPtr client, XID id, RESTYPE rtype, void *res,
                           RESTYPE ptype, void *parent, Mask access_mode)
{
    XaceResourceAccessRec rec = { client, id, rtype, res, ptype, parent,
                                  access_mode, Success };
    CallCallbacks(&XaceHooks[XACE_RESOURCE_ACCESS], &rec);
    return rec.status;
}

int XaceHookDeviceAccess(ClientPtr client, DeviceIntPtr dev, Mask access_mode)
{
    XaceDeviceAccessRec rec = { client, dev, access_mode, Success };
    CallCallbacks(&XaceHooks[XACE_DEVICE_ACCESS], &rec);
    return rec.status;
}

int XaceHookSendAccess(ClientPtr client, DeviceIntPtr dev, WindowPtr win,
                       xEventPtr ev, int count)
{
    XaceSendAccessRec rec = { client, dev, win, ev, count, Success };
    CallCallbacks(&XaceHooks[XACE_SEND_ACCESS], &rec);
    return rec.status;
}

int XaceHookReceiveAccess(ClientPtr client, WindowPtr win,
                          xEventPtr ev, int count)
{
    XaceReceiveAccessRec rec = { client, win, ev, count, Success };
    CallCallbacks(&XaceHooks[XACE_RECEIVE_ACCESS], &rec);
    return rec.status;
}

int XaceHookClientAccess(ClientPtr client, ClientPtr target, Mask access_mode)
{
    XaceClientAccessRec rec = { client, target, access_mode, Success };
    CallCallbacks(&XaceHooks[XACE_CLIENT_ACCESS], &rec);
    return rec.status;
}

int XaceHookExtAccess(ClientPtr client, ExtensionEntry *ext)
{
    XaceExtAccessRec rec = { client, ext, DixGetAttrAccess, Success };
    CallCallbacks(&XaceHooks[XACE_EXT_ACCESS], &rec);
    return rec.status;
}

int XaceHookServerAccess(ClientPtr client, Mask access_mode)
{
    XaceServerAccessRec rec = { client, access_mode, Success };
    CallCallbacks(&XaceHooks[XACE_SERVER_ACCESS], &rec);
    return rec.status;
}

int XaceHookScreenAccess(ClientPtr client, ScreenPtr screen, Mask access_mode)
{
    XaceScreenAccessRec rec = { client, screen, access_mode, Success };
    CallCallbacks(&XaceHooks[XACE_SCREEN_ACCESS], &rec);
    return rec.status;
}

int XaceHookScreensaverAccess(ClientPtr client, ScreenPtr screen, Mask access_mode)
{
    XaceScreenAccessRec rec = { client, screen, access_mode, Success };
    CallCallbacks(&XaceHooks[XACE_SCREENSAVER_ACCESS], &rec);
    return rec.status;
}

int XaceHookAuthAvail(ClientPtr client, XID authId)
{
    XaceAuthAvailRec rec = { client, authId };
    CallCallbacks(&XaceHooks[XACE_AUTH_AVAIL], &rec);
    return Success;
}

int XaceHookKeyAvail(xEventPtr ev, DeviceIntPtr dev, int count)
{
    XaceKeyAvailRec rec = { ev, dev, count };
    CallCallbacks(&XaceHooks[XACE_KEY_AVAIL], &rec);
    return Success;
}

/* XaceHookIsSet
 *
 * Utility function to determine whether there are any callbacks listening on a
 * particular XACE hook.
 *
 * Returns non-zero if there is a callback, zero otherwise.
 */
int
XaceHookIsSet(int hook)
{
    if (hook < 0 || hook >= XACE_NUM_HOOKS)
        return 0;
    return XaceHooks[hook] != NULL;
}

/* XaceCensorImage
 *
 * Called after pScreen->GetImage to prevent pieces or trusted windows from
 * being returned in image data from an untrusted window.
 *
 * Arguments:
 *	client is the client doing the GetImage.
 *      pVisibleRegion is the visible region of the window.
 *	widthBytesLine is the width in bytes of one horizontal line in pBuf.
 *	pDraw is the source window.
 *	x, y, w, h is the rectangle of image data from pDraw in pBuf.
 *	format is the format of the image data in pBuf: ZPixmap or XYPixmap.
 *	pBuf is the image data.
 *
 * Returns: nothing.
 *
 * Side Effects:
 *	Any part of the rectangle (x, y, w, h) that is outside the visible
 *	region of the window will be destroyed (overwritten) in pBuf.
 */
void
XaceCensorImage(ClientPtr client,
                RegionPtr pVisibleRegion,
                long widthBytesLine,
                DrawablePtr pDraw,
                int x, int y, int w, int h, unsigned int format, char *pBuf)
{
    RegionRec imageRegion;      /* region representing x,y,w,h */
    RegionRec censorRegion;     /* region to obliterate */
    BoxRec imageBox;
    int nRects;

    imageBox.x1 = pDraw->x + x;
    imageBox.y1 = pDraw->y + y;
    imageBox.x2 = pDraw->x + x + w;
    imageBox.y2 = pDraw->y + y + h;
    RegionInit(&imageRegion, &imageBox, 1);
    RegionNull(&censorRegion);

    /* censorRegion = imageRegion - visibleRegion */
    RegionSubtract(&censorRegion, &imageRegion, pVisibleRegion);
    nRects = RegionNumRects(&censorRegion);
    if (nRects > 0) {           /* we have something to censor */
        GCPtr pScratchGC = NULL;
        PixmapPtr pPix = NULL;
        Bool failed = FALSE;
        int depth = 1;
        int bitsPerPixel = 1;
        int i;
        BoxPtr pBox;

        /* convert region to list-of-rectangles for PolyFillRect */

        xRectangle *pRects = calloc(1, nRects * sizeof(xRectangle));
        if (!pRects) {
            failed = TRUE;
            goto failSafe;
        }
        for (pBox = RegionRects(&censorRegion), i = 0; i < nRects; i++, pBox++) {
            pRects[i].x = pBox->x1 - imageBox.x1;
            pRects[i].y = pBox->y1 - imageBox.y1;
            pRects[i].width = pBox->x2 - pBox->x1;
            pRects[i].height = pBox->y2 - pBox->y1;
        }

        /* use pBuf as a fake pixmap */

        if (format == ZPixmap) {
            depth = pDraw->depth;
            bitsPerPixel = pDraw->bitsPerPixel;
        }

        pPix = GetScratchPixmapHeader(pDraw->pScreen, w, h,
                                      depth, bitsPerPixel,
                                      widthBytesLine, (void *) pBuf);
        if (!pPix) {
            failed = TRUE;
            goto failSafe;
        }

        pScratchGC = GetScratchGC(depth, pPix->drawable.pScreen);
        if (!pScratchGC) {
            failed = TRUE;
            goto failSafe;
        }

        ValidateGC(&pPix->drawable, pScratchGC);
        (*pScratchGC->ops->PolyFillRect) (&pPix->drawable,
                                          pScratchGC, nRects, pRects);

 failSafe:
        if (failed) {
            /* Censoring was not completed above.  To be safe, wipe out
             * all the image data so that nothing trusted gets out.
             */
            memset(pBuf, 0, (int) (widthBytesLine * h));
        }
        free(pRects);
        if (pScratchGC)
            FreeScratchGC(pScratchGC);
        if (pPix)
            FreeScratchPixmapHeader(pPix);
    }
    RegionUninit(&imageRegion);
    RegionUninit(&censorRegion);
}                               /* XaceCensorImage */

/*
 * Xtrans wrappers for use by modules
 */
int
XaceGetConnectionNumber(ClientPtr client)
{
    return GetClientFd(client);
}

Bool
XaceRegisterCallback(int hook, CallbackProcPtr callback, void *data)
{
    return AddCallback(XaceHooks+(hook), callback, data);
}

Bool
XaceDeleteCallback(int hook, CallbackProcPtr callback, void *data)
{
    return DeleteCallback(XaceHooks+(hook), callback, data);
}

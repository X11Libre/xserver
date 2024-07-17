/******************************************************************************
 *
 * Copyright (c) 1994, 1995  Hewlett-Packard Company
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL HEWLETT-PACKARD COMPANY BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR
 * THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the name of the Hewlett-Packard
 * Company shall not be used in advertising or otherwise to promote the
 * sale, use or other dealings in this Software without prior written
 * authorization from the Hewlett-Packard Company.
 *
 *     DIX DBE code
 *
 *****************************************************************************/

#include <dix-config.h>

#include <string.h>
#include <stdint.h>
#include <X11/X.h>
#include <X11/Xproto.h>

#include "dix/dix_priv.h"
#include "dix/request_priv.h"
#include "dix/rpcbuf_priv.h"
#include "dix/screen_hooks_priv.h"
#include "dix/screenint_priv.h"
#include "dix/window_priv.h"
#include "miext/extinit_priv.h"

#include "scrnintstr.h"
#include "extnsionst.h"
#include "gcstruct.h"
#include "dixstruct.h"
#include "dbestruct.h"
#include "midbe.h"
#include "xace.h"

/* GLOBALS */
Bool noDbeExtension = FALSE;

/* These are globals for use by DDX */
DevPrivateKeyRec dbeScreenPrivKeyRec;
DevPrivateKeyRec dbeWindowPrivKeyRec;

/* These are globals for use by DDX */
RESTYPE dbeDrawableResType;
RESTYPE dbeWindowPrivResType;

/* Used to generate DBE's BadBuffer error. */
static int dbeErrorBase;

/******************************************************************************
 *
 * DBE DIX Procedure: DbeStubScreen
 *
 * Description:
 *
 *     This is function stubs the function pointers in the given DBE screen
 *     private and increments the number of stubbed screens.
 *
 *****************************************************************************/

static void
DbeStubScreen(DbeScreenPrivPtr pDbeScreenPriv, int *nStubbedScreens)
{
    /* Stub DIX. */
    pDbeScreenPriv->SetupBackgroundPainter = NULL;

    /* Stub DDX. */
    pDbeScreenPriv->GetVisualInfo = NULL;
    pDbeScreenPriv->AllocBackBufferName = NULL;
    pDbeScreenPriv->SwapBuffers = NULL;
    pDbeScreenPriv->WinPrivDelete = NULL;

    (*nStubbedScreens)++;
}

/******************************************************************************
 *
 * DBE DIX Procedure: ProcDbeGetVersion
 *
 * Description:
 *
 *     This function is for processing a DbeGetVersion request.
 *     This request returns the major and minor version numbers of this
 *     extension.
 *
 * Return Values:
 *
 *     Success
 *
 *****************************************************************************/

static int
ProcDbeGetVersion(ClientPtr client)
{
    X_REQUEST_HEAD_STRUCT(xDbeGetVersionReq);

    xDbeGetVersionReply reply = {
        .majorVersion = DBE_MAJOR_VERSION,
        .minorVersion = DBE_MINOR_VERSION
    };

    return X_SEND_REPLY_SIMPLE(client, reply);
}

/******************************************************************************
 *
 * DBE DIX Procedure: ProcDbeAllocateBackBufferName
 *
 * Description:
 *
 *     This function is for processing a DbeAllocateBackBufferName request.
 *     This request allocates a drawable ID used to refer to the back buffer
 *     of a window.
 *
 * Return Values:
 *
 *     BadAlloc    - server can not allocate resources
 *     BadIDChoice - id is out of range for client; id is already in use
 *     BadMatch    - window is not an InputOutput window;
 *                   visual of window is not on list returned by
 *                   DBEGetVisualInfo;
 *     BadValue    - invalid swap action is specified
 *     BadWindow   - window is not a valid window
 *     Success
 *
 *****************************************************************************/

static int
ProcDbeAllocateBackBufferName(ClientPtr client)
{
    X_REQUEST_HEAD_STRUCT(xDbeAllocateBackBufferNameReq);
    X_REQUEST_FIELD_CARD32(window);
    X_REQUEST_FIELD_CARD32(buffer);

    /* The window must be valid. */
    WindowPtr pWin;
    int status = dixLookupWindow(&pWin, stuff->window, client, DixManageAccess);
    if (status != Success)
        return status;

    /* The window must be InputOutput. */
    if (pWin->drawable.class != InputOutput) {
        return BadMatch;
    }

    /* The swap action must be valid. */
    xDbeSwapAction swapAction = stuff->swapAction; /* use local var for performance. */
    if ((swapAction != XdbeUndefined) &&
        (swapAction != XdbeBackground) &&
        (swapAction != XdbeUntouched) && (swapAction != XdbeCopied)) {
        return BadValue;
    }

    /* The id must be in range and not already in use. */
    LEGAL_NEW_RESOURCE(stuff->buffer, client);

    /* The visual of the window must be in the list returned by
     * GetVisualInfo.
     */
    DbeScreenPrivPtr pDbeScreenPriv = DBE_SCREEN_PRIV_FROM_WINDOW(pWin);
    if (!pDbeScreenPriv->GetVisualInfo)
        return BadMatch;        /* screen doesn't support double buffering */

    XdbeScreenVisualInfo scrVisInfo;
    if (!(*pDbeScreenPriv->GetVisualInfo) (pWin->drawable.pScreen, &scrVisInfo)) {
        /* GetVisualInfo() failed to allocate visual info data. */
        return BadAlloc;
    }

    /* See if the window's visual is on the list. */
    VisualID visual = wVisual(pWin);
    Bool visualMatched = FALSE;
    for (int i = 0; (i < scrVisInfo.count) && !visualMatched; i++) {
        if (scrVisInfo.visinfo[i].visual == visual) {
            visualMatched = TRUE;
        }
    }

    /* Free what was allocated by the GetVisualInfo() call above. */
    free(scrVisInfo.visinfo);

    if (!visualMatched) {
        return BadMatch;
    }

    int add_index = 0;

    DbeWindowPrivPtr pDbeWindowPriv = DBE_WINDOW_PRIV(pWin);
    if (!pDbeWindowPriv) {
        /* There is no buffer associated with the window.
         * Allocate a window priv.
         */

        pDbeWindowPriv = calloc(1, sizeof(DbeWindowPrivRec));
        if (!pDbeWindowPriv)
            return BadAlloc;

        /* Fill out window priv information. */
        pDbeWindowPriv->pWindow = pWin;
        pDbeWindowPriv->width = pWin->drawable.width;
        pDbeWindowPriv->height = pWin->drawable.height;
        pDbeWindowPriv->x = pWin->drawable.x;
        pDbeWindowPriv->y = pWin->drawable.y;
        pDbeWindowPriv->nBufferIDs = 0;

        /* Set the buffer ID array pointer to the initial (static) array). */
        pDbeWindowPriv->IDs = pDbeWindowPriv->initIDs;

        /* Initialize the buffer ID list. */
        pDbeWindowPriv->maxAvailableIDs = DBE_INIT_MAX_IDS;
        pDbeWindowPriv->IDs[0] = stuff->buffer;

        for (int i = 0; i < DBE_INIT_MAX_IDS; i++)
            pDbeWindowPriv->IDs[i] = DBE_FREE_ID_ELEMENT;

        /* Actually connect the window priv to the window. */
        dixSetPrivate(&pWin->devPrivates, &dbeWindowPrivKeyRec, pDbeWindowPriv);
    } else {
        /* A buffer is already associated with the window.
         * Add the new buffer ID to the array, reallocating the array memory
         * if necessary.
         */

        /* Determine if there is a free element in the ID array. */
        int i=0;
        for (i = 0; i < pDbeWindowPriv->maxAvailableIDs; i++) {
            if (pDbeWindowPriv->IDs[i] == DBE_FREE_ID_ELEMENT) {
                /* There is still room in the ID array. */
                break;
            }
        }

        if (i == pDbeWindowPriv->maxAvailableIDs) {
            /* No more room in the ID array -- reallocate another array. */
            XID *pIDs;

            /* Setup an array pointer for the realloc operation below. */
            if (pDbeWindowPriv->maxAvailableIDs == DBE_INIT_MAX_IDS) {
                /* We will malloc a new array. */
                pIDs = NULL;
            }
            else {
                /* We will realloc a new array. */
                pIDs = pDbeWindowPriv->IDs;
            }

            /* malloc/realloc a new array and initialize all elements to 0. */
            pDbeWindowPriv->IDs =
                reallocarray(pIDs,
                             pDbeWindowPriv->maxAvailableIDs + DBE_INCR_MAX_IDS,
                             sizeof(XID));
            if (!pDbeWindowPriv->IDs) {
                return BadAlloc;
            }
            memset(&pDbeWindowPriv->IDs[pDbeWindowPriv->nBufferIDs], 0,
                   (pDbeWindowPriv->maxAvailableIDs + DBE_INCR_MAX_IDS -
                    pDbeWindowPriv->nBufferIDs) * sizeof(XID));

            if (pDbeWindowPriv->maxAvailableIDs == DBE_INIT_MAX_IDS) {
                /* We just went from using the initial (static) array to a
                 * newly allocated array.  Copy the IDs from the initial array
                 * to the new array.
                 */
                memcpy(pDbeWindowPriv->IDs, pDbeWindowPriv->initIDs,
                       DBE_INIT_MAX_IDS * sizeof(XID));
            }

            pDbeWindowPriv->maxAvailableIDs += DBE_INCR_MAX_IDS;
        }

        add_index = i;
    }

    /* Call the DDX routine to allocate the back buffer. */
    status = (*pDbeScreenPriv->AllocBackBufferName) (pWin, stuff->buffer,
                                                     stuff->swapAction);

    if (status == Success) {
        pDbeWindowPriv->IDs[add_index] = stuff->buffer;
        if (!AddResource(stuff->buffer, dbeWindowPrivResType,
                         (void *) pDbeWindowPriv)) {
            pDbeWindowPriv->IDs[add_index] = DBE_FREE_ID_ELEMENT;

            if (pDbeWindowPriv->nBufferIDs == 0) {
                status = BadAlloc;
                goto out_free;
            }
        }
    }
    else {
        /* The DDX buffer allocation routine failed for the first buffer of
         * this window.
         */
        if (pDbeWindowPriv->nBufferIDs == 0) {
            goto out_free;
        }
    }

    /* Increment the number of buffers (XIDs) associated with this window. */
    pDbeWindowPriv->nBufferIDs++;

    /* Set swap action on all calls. */
    pDbeWindowPriv->swapAction = stuff->swapAction;

    return status;

 out_free:
    dixSetPrivate(&pWin->devPrivates, &dbeWindowPrivKeyRec, NULL);
    free(pDbeWindowPriv);
    return status;

}

/******************************************************************************
 *
 * DBE DIX Procedure: ProcDbeDeallocateBackBufferName
 *
 * Description:
 *
 *     This function is for processing a DbeDeallocateBackBufferName request.
 *     This request frees a drawable ID that was obtained by a
 *     DbeAllocateBackBufferName request.
 *
 * Return Values:
 *
 *     BadBuffer - buffer to deallocate is not associated with a window
 *     Success
 *
 *****************************************************************************/

static int
ProcDbeDeallocateBackBufferName(ClientPtr client)
{
    X_REQUEST_HEAD_STRUCT(xDbeDeallocateBackBufferNameReq);
    X_REQUEST_FIELD_CARD32(buffer);

    DbeWindowPrivPtr pDbeWindowPriv;

    /* Buffer name must be valid */
    int rc = dixLookupResourceByType((void **) &pDbeWindowPriv, stuff->buffer,
                                 dbeWindowPrivResType, client,
                                 DixDestroyAccess);
    if (rc != Success)
        return rc;

    void *val;
    rc = dixLookupResourceByType(&val, stuff->buffer, dbeDrawableResType,
                                 client, DixDestroyAccess);
    if (rc != Success)
        return rc;

    /* Make sure that the id is valid for the window.
     * This is paranoid code since we already looked up the ID by type
     * above.
     */

    int i = 0;
    for (i = 0; i < pDbeWindowPriv->nBufferIDs; i++) {
        /* Loop through the ID list to find the ID. */
        if (pDbeWindowPriv->IDs[i] == stuff->buffer) {
            break;
        }
    }

    if (i == pDbeWindowPriv->nBufferIDs) {
        /* We did not find the ID in the ID list. */
        client->errorValue = stuff->buffer;
        return dbeErrorBase + DbeBadBuffer;
    }

    FreeResource(stuff->buffer, X11_RESTYPE_NONE);

    return Success;
}

/******************************************************************************
 *
 * DBE DIX Procedure: ProcDbeSwapBuffers
 *
 * Description:
 *
 *     This function is for processing a DbeSwapBuffers request.
 *     This request swaps the buffers for all windows listed, applying the
 *     appropriate swap action for each window.
 *
 * Return Values:
 *
 *     BadAlloc  - local allocation failed; this return value is not defined
 *                 by the protocol
 *     BadMatch  - a window in request is not double-buffered; a window in
 *                 request is listed more than once
 *     BadValue  - invalid swap action is specified; no swap action is
 *                 specified
 *     BadWindow - a window in request is not valid
 *     Success
 *
 *****************************************************************************/

static int
ProcDbeSwapBuffers(ClientPtr client)
{
    X_REQUEST_HEAD_AT_LEAST(xDbeSwapBuffersReq);
    X_REQUEST_FIELD_CARD32(n);

    if (stuff->n == 0)
        return Success;

    if (stuff->n > UINT32_MAX / sizeof(DbeSwapInfoRec))
        return BadLength;
    REQUEST_FIXED_SIZE(xDbeSwapBuffersReq, stuff->n * sizeof(xDbeSwapInfo));

    if (client->swapped) {
        xDbeSwapInfo *pSwapInfo;
        if (stuff->n != 0) {
            pSwapInfo = (xDbeSwapInfo *) stuff + 1;

            /* The swap info following the fix part of this request is a window(32)
             * followed by a 1 byte swap action and then 3 pad bytes.  We only need
             * to swap the window information.
             */
            for (int i = 0; i < stuff->n; i++, pSwapInfo++)
                swapl(&pSwapInfo->window);
        }
    }

    int error = Success;

    unsigned int nStuff = stuff->n; /* use local variable for performance. */

    /* Get to the swap info appended to the end of the request. */
    xDbeSwapInfo* dbeSwapInfo = (xDbeSwapInfo *) &stuff[1];

    /* Allocate array to record swap information. */
    DbeSwapInfoPtr swapInfo = calloc(nStuff, sizeof(DbeSwapInfoRec));
    if (swapInfo == NULL) {
        return BadAlloc;
    }

    for (int i = 0; i < nStuff; i++) {
        WindowPtr pWin;
        /* Check all windows to swap. */

        /* Each window must be a valid window - BadWindow. */
        error = dixLookupWindow(&pWin, dbeSwapInfo[i].window, client,
                                DixWriteAccess);
        if (error != Success) {
            free(swapInfo);
            return error;
        }

        /* Each window must be double-buffered - BadMatch. */
        if (DBE_WINDOW_PRIV(pWin) == NULL) {
            free(swapInfo);
            return BadMatch;
        }

        /* Each window must only be specified once - BadMatch. */
        for (int j = i + 1; j < nStuff; j++) {
            if (dbeSwapInfo[i].window == dbeSwapInfo[j].window) {
                free(swapInfo);
                return BadMatch;
            }
        }

        /* Each swap action must be valid - BadValue. */
        if ((dbeSwapInfo[i].swapAction != XdbeUndefined) &&
            (dbeSwapInfo[i].swapAction != XdbeBackground) &&
            (dbeSwapInfo[i].swapAction != XdbeUntouched) &&
            (dbeSwapInfo[i].swapAction != XdbeCopied)) {
            free(swapInfo);
            return BadValue;
        }

        /* Everything checks out OK.  Fill in the swap info array. */
        swapInfo[i].pWindow = pWin;
        swapInfo[i].swapAction = dbeSwapInfo[i].swapAction;

    }                           /* for (i = 0; i < nStuff; i++) */

    /* Call the DDX routine to perform the swap(s).  The DDX routine should
     * scan the swap list (swap info), swap any buffers that it knows how to
     * handle, delete them from the list, and update nStuff to indicate how
     * many windows it did not handle.
     *
     * This scheme allows a range of sophistication in the DDX SwapBuffers()
     * implementation.  Naive implementations could just swap the first buffer
     * in the list, move the last buffer to the front, decrement nStuff, and
     * return.  The next level of sophistication could be to scan the whole
     * list for windows on the same screen.  Up another level, the DDX routine
     * could deal with cross-screen synchronization.
     */

    int nStuff_i = nStuff;
    while (nStuff_i > 0) {
        DbeScreenPrivPtr pDbeScreenPriv = DBE_SCREEN_PRIV_FROM_WINDOW(swapInfo[0].pWindow);
        error = (*pDbeScreenPriv->SwapBuffers) (client, &nStuff_i, swapInfo);
        if (error != Success) {
            free(swapInfo);
            return error;
        }
    }

    free(swapInfo);
    return Success;
}

/******************************************************************************
 *
 * DBE DIX Procedure: ProcDbeGetVisualInfo
 *
 * Description:
 *
 *     This function is for processing a ProcDbeGetVisualInfo request.
 *     This request returns information about which visuals support
 *     double buffering.
 *
 * Return Values:
 *
 *     BadDrawable - value in screen specifiers is not a valid drawable
 *     Success
 *
 *****************************************************************************/

static int
ProcDbeGetVisualInfo(ClientPtr client)
{
    X_REQUEST_HEAD_AT_LEAST(xDbeGetVisualInfoReq);
    X_REQUEST_FIELD_CARD32(n);
    X_REQUEST_REST_CARD32();

    DbeScreenPrivPtr pDbeScreenPriv;
    Drawable *drawables;
    DrawablePtr *pDrawables = NULL;
    register int rc;
    register int count;         /* number of visual infos in reply */

    if (stuff->n > UINT32_MAX / sizeof(CARD32))
        return BadLength;
    REQUEST_FIXED_SIZE(xDbeGetVisualInfoReq, stuff->n * sizeof(CARD32));

    if (stuff->n > UINT32_MAX / sizeof(DrawablePtr))
        return BadAlloc;
    /* Make sure any specified drawables are valid. */
    if (stuff->n != 0) {
        if (!(pDrawables = calloc(stuff->n, sizeof(DrawablePtr)))) {
            return BadAlloc;
        }

        drawables = (Drawable *) &stuff[1];

        for (int i = 0; i < stuff->n; i++) {
            rc = dixLookupDrawable(pDrawables + i, drawables[i], client, 0,
                                   DixGetAttrAccess);
            if (rc != Success) {
                free(pDrawables);
                return rc;
            }
        }
    }

    count = (stuff->n == 0) ? screenInfo.numScreens : stuff->n;

    x_rpcbuf_t rpcbuf = { .swapped = client->swapped, .err_clear = TRUE };

    for (int i = 0; i < count; i++) {
        ScreenPtr pScreen = (stuff->n == 0) ? dixGetScreenPtr(i) : pDrawables[i]->pScreen;
        pDbeScreenPriv = DBE_SCREEN_PRIV(pScreen);

        rc = dixCallScreenAccessCallback(client, pScreen, DixGetAttrAccess);
        if (rc != Success)
            goto clearRpcBuf;

        XdbeScreenVisualInfo visualInfo = { 0 };
        if (!(pDbeScreenPriv->GetVisualInfo(pScreen, &visualInfo))) {
            /* We failed to alloc visualInfo.visinfo. */
            rc = BadAlloc;

            /* Free visinfos that we allocated for previous screen infos. */
            goto clearRpcBuf;
        }

        /* ensure enough buffer space here, so we don't need to check for
           errors on individual operations */
        if (!x_rpcbuf_makeroom(&rpcbuf, (visualInfo.count+1)*8)) {
            rc = BadAlloc;
            free(visualInfo.visinfo);
            goto clearRpcBuf;
        }

        /* For each screen in the reply, send off the visual info */

        x_rpcbuf_write_CARD32(&rpcbuf, visualInfo.count);
        for (int j = 0; j < visualInfo.count; j++) {
            /* Write visualID(32), depth(8), perfLevel(8), and pad(16). */
            x_rpcbuf_write_CARD32(&rpcbuf, visualInfo.visinfo[j].visual);
            x_rpcbuf_write_CARD8(&rpcbuf, visualInfo.visinfo[j].depth);
            x_rpcbuf_write_CARD8(&rpcbuf, visualInfo.visinfo[j].perflevel);
            x_rpcbuf_write_CARD16(&rpcbuf, 0);
        }

        free(visualInfo.visinfo);
    }

    xDbeGetVisualInfoReply reply = {
        .m = count
    };

    REPLY_FIELD_CARD32(m);

    rc = X_SEND_REPLY_WITH_RPCBUF(client, reply, rpcbuf);

clearRpcBuf:
    x_rpcbuf_clear(&rpcbuf);
    free(pDrawables);

    return rc;
}

/******************************************************************************
 *
 * DBE DIX Procedure: ProcDbeGetbackBufferAttributes
 *
 * Description:
 *
 *     This function is for processing a ProcDbeGetbackBufferAttributes
 *     request.  This request returns information about a back buffer.
 *
 * Return Values:
 *
 *     Success
 *
 *****************************************************************************/

static int
ProcDbeGetBackBufferAttributes(ClientPtr client)
{
    X_REQUEST_HEAD_STRUCT(xDbeGetBackBufferAttributesReq);
    X_REQUEST_FIELD_CARD32(buffer);

    DbeWindowPrivPtr pDbeWindowPriv;
    int rc;

    rc = dixLookupResourceByType((void **) &pDbeWindowPriv, stuff->buffer,
                                 dbeWindowPrivResType, client,
                                 DixGetAttrAccess);

    xDbeGetBackBufferAttributesReply reply = { 0 };

    if (rc == Success) {
        reply.attributes = pDbeWindowPriv->pWindow->drawable.id;
    }
    else {
        reply.attributes = None;
    }

    REPLY_FIELD_CARD32(attributes);

    return X_SEND_REPLY_SIMPLE(client, reply);
}

/******************************************************************************
 *
 * DBE DIX Procedure: ProcDbeDispatch
 *
 * Description:
 *
 *     This function dispatches DBE requests.
 *
 *****************************************************************************/

static int
ProcDbeDispatch(ClientPtr client)
{
    REQUEST(xReq);

    switch (stuff->data) {
    case X_DbeGetVersion:
        return (ProcDbeGetVersion(client));

    case X_DbeAllocateBackBufferName:
        return (ProcDbeAllocateBackBufferName(client));

    case X_DbeDeallocateBackBufferName:
        return (ProcDbeDeallocateBackBufferName(client));

    case X_DbeSwapBuffers:
        return (ProcDbeSwapBuffers(client));

    case X_DbeBeginIdiom:
        return Success;

    case X_DbeEndIdiom:
        return Success;

    case X_DbeGetVisualInfo:
        return (ProcDbeGetVisualInfo(client));

    case X_DbeGetBackBufferAttributes:
        return (ProcDbeGetBackBufferAttributes(client));

    default:
        return BadRequest;
    }
}

/******************************************************************************
 *
 * DBE DIX Procedure: DbeSetupBackgroundPainter
 *
 * Description:
 *
 *     This function sets up pGC to clear pixmaps.
 *
 * Return Values:
 *
 *     TRUE  - setup was successful
 *     FALSE - the window's background state is NONE
 *
 *****************************************************************************/

static Bool
DbeSetupBackgroundPainter(WindowPtr pWin, GCPtr pGC)
{
    ChangeGCVal gcvalues[4];
    int ts_x_origin, ts_y_origin;
    PixUnion background;
    int backgroundState;
    Mask gcmask;

    /* First take care of any ParentRelative stuff by altering the
     * tile/stipple origin to match the coordinates of the upper-left
     * corner of the first ancestor without a ParentRelative background.
     * This coordinate is, of course, negative.
     */
    ts_x_origin = ts_y_origin = 0;
    while (pWin->backgroundState == ParentRelative) {
        ts_x_origin -= pWin->origin.x;
        ts_y_origin -= pWin->origin.y;

        pWin = pWin->parent;
    }
    backgroundState = pWin->backgroundState;
    background = pWin->background;

    switch (backgroundState) {
    case BackgroundPixel:
        gcvalues[0].val = background.pixel;
        gcvalues[1].val = FillSolid;
        gcmask = GCForeground | GCFillStyle;
        break;

    case BackgroundPixmap:
        gcvalues[0].val = FillTiled;
        gcvalues[1].ptr = background.pixmap;
        gcvalues[2].val = ts_x_origin;
        gcvalues[3].val = ts_y_origin;
        gcmask = GCFillStyle | GCTile | GCTileStipXOrigin | GCTileStipYOrigin;
        break;

    default:
        /* pWin->backgroundState == None */
        return FALSE;
    }

    return ChangeGC(NULL, pGC, gcmask, gcvalues) == 0;
}

/******************************************************************************
 *
 * DBE DIX Procedure: DbeDrawableDelete
 *
 * Description:
 *
 *     This is the resource delete function for dbeDrawableResType.
 *     It is registered when the drawable resource type is created in
 *     DbeExtensionInit().
 *
 *     To make resource deletion simple, we do not do anything in this function
 *     and leave all resource deletion to DbeWindowPrivDelete(), which will
 *     eventually be called or already has been called.  Deletion functions are
 *     not guaranteed to be called in any particular order.
 *
 *****************************************************************************/
static int
DbeDrawableDelete(void *pDrawable, XID id)
{
    return Success;

}

/******************************************************************************
 *
 * DBE DIX Procedure: DbeWindowPrivDelete
 *
 * Description:
 *
 *     This is the resource delete function for dbeWindowPrivResType.
 *     It is registered when the drawable resource type is created in
 *     DbeExtensionInit().
 *
 *****************************************************************************/
static int
DbeWindowPrivDelete(void *pDbeWinPriv, XID id)
{
    DbeScreenPrivPtr pDbeScreenPriv;
    DbeWindowPrivPtr pDbeWindowPriv = (DbeWindowPrivPtr) pDbeWinPriv;
    int i;

    /*
     **************************************************************************
     ** Remove the buffer ID from the ID array.
     **************************************************************************
     */

    /* Find the ID in the ID array. */
    i = 0;
    while ((i < pDbeWindowPriv->nBufferIDs) && (pDbeWindowPriv->IDs[i] != id)) {
        i++;
    }

    if (i == pDbeWindowPriv->nBufferIDs) {
        /* We did not find the ID in the array.  We should never get here. */
        return BadValue;
    }

    /* Remove the ID from the array. */

    if (i < (pDbeWindowPriv->nBufferIDs - 1)) {
        /* Compress the buffer ID array, overwriting the ID in the process. */
        memmove(&pDbeWindowPriv->IDs[i], &pDbeWindowPriv->IDs[i + 1],
                (pDbeWindowPriv->nBufferIDs - i - 1) * sizeof(XID));
    }
    else {
        /* We are removing the last ID in the array, in which case, the
         * assignment below is all that we need to do.
         */
    }
    pDbeWindowPriv->IDs[pDbeWindowPriv->nBufferIDs - 1] = DBE_FREE_ID_ELEMENT;

    pDbeWindowPriv->nBufferIDs--;

    /* If an extended array was allocated, then check to see if the remaining
     * buffer IDs will fit in the static array.
     */

    if ((pDbeWindowPriv->maxAvailableIDs > DBE_INIT_MAX_IDS) &&
        (pDbeWindowPriv->nBufferIDs == DBE_INIT_MAX_IDS)) {
        /* Copy the IDs back into the static array. */
        memcpy(pDbeWindowPriv->initIDs, pDbeWindowPriv->IDs,
               DBE_INIT_MAX_IDS * sizeof(XID));

        /* Free the extended array; use the static array. */
        free(pDbeWindowPriv->IDs);
        pDbeWindowPriv->IDs = pDbeWindowPriv->initIDs;
        pDbeWindowPriv->maxAvailableIDs = DBE_INIT_MAX_IDS;
    }

    /*
     **************************************************************************
     ** Perform DDX level tasks.
     **************************************************************************
     */

    pDbeScreenPriv = DBE_SCREEN_PRIV_FROM_WINDOW_PRIV((DbeWindowPrivPtr)
                                                      pDbeWindowPriv);
    (*pDbeScreenPriv->WinPrivDelete) ((DbeWindowPrivPtr) pDbeWindowPriv, id);

    /*
     **************************************************************************
     ** Perform miscellaneous tasks if this is the last buffer associated
     ** with the window.
     **************************************************************************
     */

    if (pDbeWindowPriv->nBufferIDs == 0) {
        /* Reset the DBE window priv pointer. */
        dixSetPrivate(&pDbeWindowPriv->pWindow->devPrivates, &dbeWindowPrivKeyRec,
                      NULL);

        /* We are done with the window priv. */
        free(pDbeWindowPriv);
    }

    return Success;
}

static void miDbeWindowDestroy(CallbackListPtr *pcbl, ScreenPtr pScreen, WindowPtr pWin);

/******************************************************************************
 *
 * DBE DIX Procedure: DbeResetProc
 *
 * Description:
 *
 *     This routine is called at the end of every server generation.
 *     It deallocates any memory reserved for the extension and performs any
 *     other tasks related to shutting down the extension.
 *
 *****************************************************************************/
static void
DbeResetProc(ExtensionEntry * extEntry)
{
    DIX_FOR_EACH_SCREEN({
        DbeScreenPrivPtr pDbeScreenPriv = DBE_SCREEN_PRIV(walkScreen);
        if (pDbeScreenPriv) {
            dixScreenUnhookWindowDestroy(walkScreen, miDbeWindowDestroy);
            dixScreenUnhookWindowPosition(walkScreen, miDbeWindowPosition);
            free(pDbeScreenPriv);
        }
    });
}

/**
 * @brief window destroy callback
 *
 * Called by DIX when window is being destroyed.
 *
 */
static void miDbeWindowDestroy(CallbackListPtr *pcbl, ScreenPtr pScreen, WindowPtr pWin)
{
    /*
     **************************************************************************
     **    Call the window priv delete function for all buffer IDs associated
     **    with this window.
     **************************************************************************
     */

    DbeWindowPrivPtr pDbeWindowPriv;
    while ((pDbeWindowPriv = DBE_WINDOW_PRIV(pWin))) {
            /* *DbeWinPrivDelete() will free the window private and set it to
             * NULL if there are no more buffer IDs associated with this
             * window.
             */
            FreeResource(pDbeWindowPriv->IDs[0], X11_RESTYPE_NONE);
            pDbeWindowPriv = DBE_WINDOW_PRIV(pWin);
    }
}

/******************************************************************************
 *
 * DBE DIX Procedure: DbeExtensionInit
 *
 * Description:
 *
 *     Called from InitExtensions in main()
 *
 *****************************************************************************/

void
DbeExtensionInit(void)
{
    ExtensionEntry *extEntry;
    DbeScreenPrivPtr pDbeScreenPriv;
    int nStubbedScreens = 0;
    Bool ddxInitSuccess;

#ifdef XINERAMA
    if (!noPanoramiXExtension)
        return;
#endif /* XINERAMA */

    /* Create the resource types. */
    dbeDrawableResType =
        CreateNewResourceType(DbeDrawableDelete, "dbeDrawable");
    if (!dbeDrawableResType)
        return;
    dbeDrawableResType |= RC_DRAWABLE;

    dbeWindowPrivResType =
        CreateNewResourceType(DbeWindowPrivDelete, "dbeWindow");
    if (!dbeWindowPrivResType)
        return;

    if (!dixRegisterPrivateKey(&dbeScreenPrivKeyRec, PRIVATE_SCREEN, 0))
        return;

    if (!dixRegisterPrivateKey(&dbeWindowPrivKeyRec, PRIVATE_WINDOW, 0))
        return;

    DIX_FOR_EACH_SCREEN({
        /* For each screen, set up DBE screen privates and init DIX
         * interface (DDX isn't supported anymore).
         */
        if (!(pDbeScreenPriv = calloc(1, sizeof(DbeScreenPrivRec)))) {
            /* If we can not alloc a window or screen private,
             * then free any privates that we already alloc'ed and return
             */

            for (int j = 0; j < walkScreenIdx; j++) {
                ScreenPtr pScreen = dixGetScreenPtr(j);
                free(dixLookupPrivate(&pScreen->devPrivates, &dbeScreenPrivKeyRec));
                dixSetPrivate(&pScreen->devPrivates, &dbeScreenPrivKeyRec, NULL);
            }
            return;
        }

        dixSetPrivate(&walkScreen->devPrivates, &dbeScreenPrivKeyRec, pDbeScreenPriv);

        {
            /* Setup DIX. */
            pDbeScreenPriv->SetupBackgroundPainter = DbeSetupBackgroundPainter;

            /* Setup DDX. */
            ddxInitSuccess = miDbeInit(walkScreen, pDbeScreenPriv);

            /* DDX DBE initialization may have the side affect of
             * reallocating pDbeScreenPriv, so we need to update it.
             */
            pDbeScreenPriv = DBE_SCREEN_PRIV(walkScreen);

            if (ddxInitSuccess) {
                /* Hook in our window destructor. The DDX initialization function
                 * already added WindowPosition hook for us.
                 */
                dixScreenHookWindowDestroy(walkScreen, miDbeWindowDestroy);
            }
            else {
                /* DDX initialization failed.  Stub the screen. */
                DbeStubScreen(pDbeScreenPriv, &nStubbedScreens);
            }
        }
    });

    if (nStubbedScreens == screenInfo.numScreens) {
        /* All screens stubbed.  Clean up and return. */
        DIX_FOR_EACH_SCREEN({
            free(dixLookupPrivate(&walkScreen->devPrivates, &dbeScreenPrivKeyRec));
            dixSetPrivate(&walkScreen->devPrivates, &dbeScreenPrivKeyRec, NULL);
        });
        return;
    }

    /* Now add the extension. */
    extEntry = AddExtension(DBE_PROTOCOL_NAME, DbeNumberEvents,
                            DbeNumberErrors, ProcDbeDispatch, ProcDbeDispatch,
                            DbeResetProc, StandardMinorOpcode);

    dbeErrorBase = extEntry->errorBase;
    SetResourceTypeErrorValue(dbeWindowPrivResType,
                              dbeErrorBase + DbeBadBuffer);
    SetResourceTypeErrorValue(dbeDrawableResType, dbeErrorBase + DbeBadBuffer);
}

/*
 * SGI FREE SOFTWARE LICENSE B (Version 2.0, Sept. 18, 2008)
 * Copyright (C) 1991-2000 Silicon Graphics, Inc. All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice including the dates of first publication and
 * either this permission notice or a reference to
 * http://oss.sgi.com/projects/FreeB/
 * shall be included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * SILICON GRAPHICS, INC. BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Except as contained in this notice, the name of Silicon Graphics, Inc.
 * shall not be used in advertising or otherwise to promote the sale, use or
 * other dealings in this Software without prior written authorization from
 * Silicon Graphics, Inc.
 */

#include <dix-config.h>

#include <string.h>

#include "dix/dix_priv.h"

#include "glxserver.h"
#include "glxutil.h"
#include <GL/glxtokens.h>
#include <unpack.h>
#include <pixmapstr.h>
#include <windowstr.h>
#include "glxext.h"
#include "indirect_dispatch.h"
#include "indirect_table.h"
#include "indirect_util.h"

/************************************************************************/

/*
** Byteswapping versions of GLX commands.  In most cases they just swap
** the incoming arguments and then call the unswapped routine.  For commands
** that have replies, a separate swapping routine for the reply is provided;
** it is called at the end of the unswapped routine.
*/

int
__glXDispSwap_CreateContext(__GLXclientState * cl, GLbyte * pc)
{
    xGLXCreateContextReq *req = (xGLXCreateContextReq *) pc;

    swaps(&req->length);
    swapl(&req->context);
    swapl(&req->visual);
    swapl(&req->screen);
    swapl(&req->shareList);

    return __glXDisp_CreateContext(cl, pc);
}

int
__glXDispSwap_CreateNewContext(__GLXclientState * cl, GLbyte * pc)
{
    xGLXCreateNewContextReq *req = (xGLXCreateNewContextReq *) pc;

    swaps(&req->length);
    swapl(&req->context);
    swapl(&req->fbconfig);
    swapl(&req->screen);
    swapl(&req->renderType);
    swapl(&req->shareList);

    return __glXDisp_CreateNewContext(cl, pc);
}

int
__glXDispSwap_CreateContextWithConfigSGIX(__GLXclientState * cl, GLbyte * pc)
{
    ClientPtr client = cl->client;
    xGLXCreateContextWithConfigSGIXReq *req =
        (xGLXCreateContextWithConfigSGIXReq *) pc;

    REQUEST_SIZE_MATCH(xGLXCreateContextWithConfigSGIXReq);

    swaps(&req->length);
    swapl(&req->context);
    swapl(&req->fbconfig);
    swapl(&req->screen);
    swapl(&req->renderType);
    swapl(&req->shareList);

    return __glXDisp_CreateContextWithConfigSGIX(cl, pc);
}

int
__glXDispSwap_DestroyContext(__GLXclientState * cl, GLbyte * pc)
{
    xGLXDestroyContextReq *req = (xGLXDestroyContextReq *) pc;

    swaps(&req->length);
    swapl(&req->context);

    return __glXDisp_DestroyContext(cl, pc);
}

int
__glXDispSwap_MakeCurrent(__GLXclientState * cl, GLbyte * pc)
{
    return BadImplementation;
}

int
__glXDispSwap_MakeContextCurrent(__GLXclientState * cl, GLbyte * pc)
{
    return BadImplementation;
}

int
__glXDispSwap_MakeCurrentReadSGI(__GLXclientState * cl, GLbyte * pc)
{
    return BadImplementation;
}

int
__glXDispSwap_IsDirect(__GLXclientState * cl, GLbyte * pc)
{
    xGLXIsDirectReq *req = (xGLXIsDirectReq *) pc;

    swaps(&req->length);
    swapl(&req->context);

    return __glXDisp_IsDirect(cl, pc);
}

int
__glXDispSwap_QueryVersion(__GLXclientState * cl, GLbyte * pc)
{
    xGLXQueryVersionReq *req = (xGLXQueryVersionReq *) pc;

    swaps(&req->length);
    swapl(&req->majorVersion);
    swapl(&req->minorVersion);

    return __glXDisp_QueryVersion(cl, pc);
}

int
__glXDispSwap_WaitGL(__GLXclientState * cl, GLbyte * pc)
{
    xGLXWaitGLReq *req = (xGLXWaitGLReq *) pc;

    swaps(&req->length);
    swapl(&req->contextTag);

    return __glXDisp_WaitGL(cl, pc);
}

int
__glXDispSwap_WaitX(__GLXclientState * cl, GLbyte * pc)
{
    xGLXWaitXReq *req = (xGLXWaitXReq *) pc;

    swaps(&req->length);
    swapl(&req->contextTag);

    return __glXDisp_WaitX(cl, pc);
}

int
__glXDispSwap_CopyContext(__GLXclientState * cl, GLbyte * pc)
{
    xGLXCopyContextReq *req = (xGLXCopyContextReq *) pc;

    swaps(&req->length);
    swapl(&req->source);
    swapl(&req->dest);
    swapl(&req->mask);

    return __glXDisp_CopyContext(cl, pc);
}

int
__glXDispSwap_GetVisualConfigs(__GLXclientState * cl, GLbyte * pc)
{
    xGLXGetVisualConfigsReq *req = (xGLXGetVisualConfigsReq *) pc;

    swapl(&req->screen);
    return __glXDisp_GetVisualConfigs(cl, pc);
}

int
__glXDispSwap_GetFBConfigs(__GLXclientState * cl, GLbyte * pc)
{
    xGLXGetFBConfigsReq *req = (xGLXGetFBConfigsReq *) pc;

    swapl(&req->screen);
    return __glXDisp_GetFBConfigs(cl, pc);
}

int
__glXDispSwap_GetFBConfigsSGIX(__GLXclientState * cl, GLbyte * pc)
{
    ClientPtr client = cl->client;
    xGLXGetFBConfigsSGIXReq *req = (xGLXGetFBConfigsSGIXReq *) pc;

    REQUEST_AT_LEAST_SIZE(xGLXGetFBConfigsSGIXReq);

    swapl(&req->screen);
    return __glXDisp_GetFBConfigsSGIX(cl, pc);
}

int
__glXDispSwap_CreateGLXPixmap(__GLXclientState * cl, GLbyte * pc)
{
    xGLXCreateGLXPixmapReq *req = (xGLXCreateGLXPixmapReq *) pc;

    swaps(&req->length);
    swapl(&req->screen);
    swapl(&req->visual);
    swapl(&req->pixmap);
    swapl(&req->glxpixmap);

    return __glXDisp_CreateGLXPixmap(cl, pc);
}

int
__glXDispSwap_CreatePixmap(__GLXclientState * cl, GLbyte * pc)
{
    ClientPtr client = cl->client;
    xGLXCreatePixmapReq *req = (xGLXCreatePixmapReq *) pc;
    CARD32 *attribs;

    REQUEST_AT_LEAST_SIZE(xGLXCreatePixmapReq);

    swaps(&req->length);
    swapl(&req->screen);
    swapl(&req->fbconfig);
    swapl(&req->pixmap);
    swapl(&req->glxpixmap);
    swapl(&req->numAttribs);

    if (req->numAttribs > (UINT32_MAX >> 3)) {
        client->errorValue = req->numAttribs;
        return BadValue;
    }
    REQUEST_FIXED_SIZE(xGLXCreatePixmapReq, req->numAttribs << 3);
    attribs = (CARD32 *) (req + 1);
    SwapLongs(attribs, req->numAttribs << 1);

    return __glXDisp_CreatePixmap(cl, pc);
}

int
__glXDispSwap_CreateGLXPixmapWithConfigSGIX(__GLXclientState * cl, GLbyte * pc)
{
    ClientPtr client = cl->client;
    xGLXCreateGLXPixmapWithConfigSGIXReq *req =
        (xGLXCreateGLXPixmapWithConfigSGIXReq *) pc;

    REQUEST_SIZE_MATCH(xGLXCreateGLXPixmapWithConfigSGIXReq);

    swaps(&req->length);
    swapl(&req->screen);
    swapl(&req->fbconfig);
    swapl(&req->pixmap);
    swapl(&req->glxpixmap);

    return __glXDisp_CreateGLXPixmapWithConfigSGIX(cl, pc);
}

int
__glXDispSwap_DestroyGLXPixmap(__GLXclientState * cl, GLbyte * pc)
{
    xGLXDestroyGLXPixmapReq *req = (xGLXDestroyGLXPixmapReq *) pc;

    swaps(&req->length);
    swapl(&req->glxpixmap);

    return __glXDisp_DestroyGLXPixmap(cl, pc);
}

int
__glXDispSwap_DestroyPixmap(__GLXclientState * cl, GLbyte * pc)
{
    ClientPtr client = cl->client;
    xGLXDestroyGLXPixmapReq *req = (xGLXDestroyGLXPixmapReq *) pc;

    REQUEST_AT_LEAST_SIZE(xGLXDestroyGLXPixmapReq);

    swaps(&req->length);
    swapl(&req->glxpixmap);

    return __glXDisp_DestroyGLXPixmap(cl, pc);
}

int
__glXDispSwap_QueryContext(__GLXclientState * cl, GLbyte * pc)
{
    xGLXQueryContextReq *req = (xGLXQueryContextReq *) pc;

    swapl(&req->context);

    return __glXDisp_QueryContext(cl, pc);
}

int
__glXDispSwap_CreatePbuffer(__GLXclientState * cl, GLbyte * pc)
{
    ClientPtr client = cl->client;
    xGLXCreatePbufferReq *req = (xGLXCreatePbufferReq *) pc;

    CARD32 *attribs;

    REQUEST_AT_LEAST_SIZE(xGLXCreatePbufferReq);

    swapl(&req->screen);
    swapl(&req->fbconfig);
    swapl(&req->pbuffer);
    swapl(&req->numAttribs);

    if (req->numAttribs > (UINT32_MAX >> 3)) {
        client->errorValue = req->numAttribs;
        return BadValue;
    }
    REQUEST_FIXED_SIZE(xGLXCreatePbufferReq, req->numAttribs << 3);
    attribs = (CARD32 *) (req + 1);
    SwapLongs(attribs, req->numAttribs << 1);

    return __glXDisp_CreatePbuffer(cl, pc);
}

int
__glXDispSwap_CreateGLXPbufferSGIX(__GLXclientState * cl, GLbyte * pc)
{
    ClientPtr client = cl->client;
    xGLXCreateGLXPbufferSGIXReq *req = (xGLXCreateGLXPbufferSGIXReq *) pc;

    REQUEST_AT_LEAST_SIZE(xGLXCreateGLXPbufferSGIXReq);

    swapl(&req->screen);
    swapl(&req->fbconfig);
    swapl(&req->pbuffer);
    swapl(&req->width);
    swapl(&req->height);

    return __glXDisp_CreateGLXPbufferSGIX(cl, pc);
}

int
__glXDispSwap_DestroyPbuffer(__GLXclientState * cl, GLbyte * pc)
{
    xGLXDestroyPbufferReq *req = (xGLXDestroyPbufferReq *) pc;

    swapl(&req->pbuffer);

    return __glXDisp_DestroyPbuffer(cl, pc);
}

int
__glXDispSwap_DestroyGLXPbufferSGIX(__GLXclientState * cl, GLbyte * pc)
{
    ClientPtr client = cl->client;
    xGLXDestroyGLXPbufferSGIXReq *req = (xGLXDestroyGLXPbufferSGIXReq *) pc;

    REQUEST_SIZE_MATCH(xGLXDestroyGLXPbufferSGIXReq);

    swapl(&req->pbuffer);

    return __glXDisp_DestroyGLXPbufferSGIX(cl, pc);
}

int
__glXDispSwap_ChangeDrawableAttributes(__GLXclientState * cl, GLbyte * pc)
{
    ClientPtr client = cl->client;
    xGLXChangeDrawableAttributesReq *req =
        (xGLXChangeDrawableAttributesReq *) pc;
    CARD32 *attribs;

    REQUEST_AT_LEAST_SIZE(xGLXChangeDrawableAttributesReq);

    swapl(&req->drawable);
    swapl(&req->numAttribs);

    if (req->numAttribs > (UINT32_MAX >> 3)) {
        client->errorValue = req->numAttribs;
        return BadValue;
    }
    if (((sizeof(xGLXChangeDrawableAttributesReq) +
          (req->numAttribs << 3)) >> 2) < client->req_len)
        return BadLength;

    attribs = (CARD32 *) (req + 1);
    SwapLongs(attribs, req->numAttribs << 1);

    return __glXDisp_ChangeDrawableAttributes(cl, pc);
}

int
__glXDispSwap_ChangeDrawableAttributesSGIX(__GLXclientState * cl, GLbyte * pc)
{
    ClientPtr client = cl->client;
    xGLXChangeDrawableAttributesSGIXReq *req =
        (xGLXChangeDrawableAttributesSGIXReq *) pc;
    CARD32 *attribs;

    REQUEST_AT_LEAST_SIZE(xGLXChangeDrawableAttributesSGIXReq);

    swapl(&req->drawable);
    swapl(&req->numAttribs);

    if (req->numAttribs > (UINT32_MAX >> 3)) {
        client->errorValue = req->numAttribs;
        return BadValue;
    }
    REQUEST_FIXED_SIZE(xGLXChangeDrawableAttributesSGIXReq,
                       req->numAttribs << 3);
    attribs = (CARD32 *) (req + 1);
    SwapLongs(attribs, req->numAttribs << 1);

    return __glXDisp_ChangeDrawableAttributesSGIX(cl, pc);
}

int
__glXDispSwap_CreateWindow(__GLXclientState * cl, GLbyte * pc)
{
    ClientPtr client = cl->client;
    xGLXCreateWindowReq *req = (xGLXCreateWindowReq *) pc;

    CARD32 *attribs;

    REQUEST_AT_LEAST_SIZE(xGLXCreateWindowReq);

    swapl(&req->screen);
    swapl(&req->fbconfig);
    swapl(&req->window);
    swapl(&req->glxwindow);
    swapl(&req->numAttribs);

    if (req->numAttribs > (UINT32_MAX >> 3)) {
        client->errorValue = req->numAttribs;
        return BadValue;
    }
    REQUEST_FIXED_SIZE(xGLXCreateWindowReq, req->numAttribs << 3);
    attribs = (CARD32 *) (req + 1);
    SwapLongs(attribs, req->numAttribs << 1);

    return __glXDisp_CreateWindow(cl, pc);
}

int
__glXDispSwap_DestroyWindow(__GLXclientState * cl, GLbyte * pc)
{
    ClientPtr client = cl->client;
    xGLXDestroyWindowReq *req = (xGLXDestroyWindowReq *) pc;

    REQUEST_AT_LEAST_SIZE(xGLXDestroyWindowReq);

    swapl(&req->glxwindow);

    return __glXDisp_DestroyWindow(cl, pc);
}

int
__glXDispSwap_SwapBuffers(__GLXclientState * cl, GLbyte * pc)
{
    xGLXSwapBuffersReq *req = (xGLXSwapBuffersReq *) pc;

    swaps(&req->length);
    swapl(&req->contextTag);
    swapl(&req->drawable);

    return __glXDisp_SwapBuffers(cl, pc);
}

int
__glXDispSwap_UseXFont(__GLXclientState * cl, GLbyte * pc)
{
    xGLXUseXFontReq *req = (xGLXUseXFontReq *) pc;

    swaps(&req->length);
    swapl(&req->contextTag);
    swapl(&req->font);
    swapl(&req->first);
    swapl(&req->count);
    swapl(&req->listBase);

    return __glXDisp_UseXFont(cl, pc);
}

int
__glXDispSwap_QueryExtensionsString(__GLXclientState * cl, GLbyte * pc)
{
    xGLXQueryExtensionsStringReq *req = (xGLXQueryExtensionsStringReq *) pc;

    swaps(&req->length);
    swapl(&req->screen);

    return __glXDisp_QueryExtensionsString(cl, pc);
}

int
__glXDispSwap_QueryServerString(__GLXclientState * cl, GLbyte * pc)
{
    xGLXQueryServerStringReq *req = (xGLXQueryServerStringReq *) pc;

    swaps(&req->length);
    swapl(&req->screen);
    swapl(&req->name);

    return __glXDisp_QueryServerString(cl, pc);
}

int
__glXDispSwap_ClientInfo(__GLXclientState * cl, GLbyte * pc)
{
    ClientPtr client = cl->client;
    xGLXClientInfoReq *req = (xGLXClientInfoReq *) pc;

    REQUEST_AT_LEAST_SIZE(xGLXClientInfoReq);

    swaps(&req->length);
    swapl(&req->major);
    swapl(&req->minor);
    swapl(&req->numbytes);

    return __glXDisp_ClientInfo(cl, pc);
}

int
__glXDispSwap_QueryContextInfoEXT(__GLXclientState * cl, GLbyte * pc)
{
    ClientPtr client = cl->client;
    xGLXQueryContextInfoEXTReq *req = (xGLXQueryContextInfoEXTReq *) pc;

    REQUEST_SIZE_MATCH(xGLXQueryContextInfoEXTReq);

    swaps(&req->length);
    swapl(&req->context);

    return __glXDisp_QueryContextInfoEXT(cl, pc);
}

int
__glXDispSwap_BindTexImageEXT(__GLXclientState * cl, GLbyte * pc)
{
    ClientPtr client = cl->client;
    xGLXVendorPrivateReq *req = (xGLXVendorPrivateReq *) pc;
    GLXDrawable *drawId;
    int *buffer;
    CARD32 *num_attribs;

    if ((sizeof(xGLXVendorPrivateReq) + 12) >> 2 > client->req_len)
        return BadLength;

    pc += __GLX_VENDPRIV_HDR_SIZE;

    drawId = ((GLXDrawable *) (pc));
    buffer = ((int *) (pc + 4));
    num_attribs = ((CARD32 *) (pc + 8));

    swaps(&req->length);
    swapl(&req->contextTag);
    swapl(drawId);
    swapl(buffer);
    swapl(num_attribs);

    return __glXDisp_BindTexImageEXT(cl, (GLbyte *) pc);
}

int
__glXDispSwap_ReleaseTexImageEXT(__GLXclientState * cl, GLbyte * pc)
{
    ClientPtr client = cl->client;
    xGLXVendorPrivateReq *req = (xGLXVendorPrivateReq *) pc;
    GLXDrawable *drawId;
    int *buffer;

    REQUEST_FIXED_SIZE(xGLXVendorPrivateReq, 8);

    pc += __GLX_VENDPRIV_HDR_SIZE;

    drawId = ((GLXDrawable *) (pc));
    buffer = ((int *) (pc + 4));

    swaps(&req->length);
    swapl(&req->contextTag);
    swapl(drawId);
    swapl(buffer);

    return __glXDisp_ReleaseTexImageEXT(cl, (GLbyte *) pc);
}

int
__glXDispSwap_CopySubBufferMESA(__GLXclientState * cl, GLbyte * pc)
{
    ClientPtr client = cl->client;
    xGLXVendorPrivateReq *req = (xGLXVendorPrivateReq *) pc;
    GLXDrawable *drawId;
    int *buffer;

    REQUEST_FIXED_SIZE(xGLXVendorPrivateReq, 20);

    (void) drawId;
    (void) buffer;

    pc += __GLX_VENDPRIV_HDR_SIZE;

    swaps(&req->length);
    swapl(&req->contextTag);
    swapl((CARD32*)(pc));
    swapl((CARD32*)(pc + 4));
    swapl((CARD32*)(pc + 8));
    swapl((CARD32*)(pc + 12));
    swapl((CARD32*)(pc + 16));

    return __glXDisp_CopySubBufferMESA(cl, pc);

}

int
__glXDispSwap_GetDrawableAttributesSGIX(__GLXclientState * cl, GLbyte * pc)
{
    ClientPtr client = cl->client;
    xGLXVendorPrivateWithReplyReq *req = (xGLXVendorPrivateWithReplyReq *) pc;
    CARD32 *data;

    REQUEST_SIZE_MATCH(xGLXGetDrawableAttributesSGIXReq);

    data = (CARD32 *) (req + 1);
    swaps(&req->length);
    swapl(&req->contextTag);
    swapl(data);

    return __glXDisp_GetDrawableAttributesSGIX(cl, pc);
}

int
__glXDispSwap_GetDrawableAttributes(__GLXclientState * cl, GLbyte * pc)
{
    ClientPtr client = cl->client;
    xGLXGetDrawableAttributesReq *req = (xGLXGetDrawableAttributesReq *) pc;

    REQUEST_AT_LEAST_SIZE(xGLXGetDrawableAttributesReq);

    swaps(&req->length);
    swapl(&req->drawable);

    return __glXDisp_GetDrawableAttributes(cl, pc);
}

/************************************************************************/

/*
** Render and Renderlarge are not in the GLX API.  They are used by the GLX
** client library to send batches of GL rendering commands.
*/

int
__glXDispSwap_Render(__GLXclientState * cl, GLbyte * pc)
{
    return __glXDisp_Render(cl, pc);
}

/*
** Execute a large rendering request (one that spans multiple X requests).
*/
int
__glXDispSwap_RenderLarge(__GLXclientState * cl, GLbyte * pc)
{
    return __glXDisp_RenderLarge(cl, pc);
}

/************************************************************************/

/*
** No support is provided for the vendor-private requests other than
** allocating these entry points in the dispatch table.
*/

int
__glXDispSwap_VendorPrivate(__GLXclientState * cl, GLbyte * pc)
{
    ClientPtr client = cl->client;
    xGLXVendorPrivateReq *req;
    GLint vendorcode;
    __GLXdispatchVendorPrivProcPtr proc;

    REQUEST_AT_LEAST_SIZE(xGLXVendorPrivateReq);

    req = (xGLXVendorPrivateReq *) pc;
    swaps(&req->length);
    swapl(&req->vendorCode);

    vendorcode = req->vendorCode;

    proc = (__GLXdispatchVendorPrivProcPtr)
        __glXGetProtocolDecodeFunction(&VendorPriv_dispatch_info,
                                       vendorcode, 1);
    if (proc != NULL) {
        return (*proc) (cl, (GLbyte *) req);
    }

    cl->client->errorValue = req->vendorCode;
    return __glXError(GLXUnsupportedPrivateRequest);
}

int
__glXDispSwap_VendorPrivateWithReply(__GLXclientState * cl, GLbyte * pc)
{
    ClientPtr client = cl->client;
    xGLXVendorPrivateWithReplyReq *req;
    GLint vendorcode;
    __GLXdispatchVendorPrivProcPtr proc;

    REQUEST_AT_LEAST_SIZE(xGLXVendorPrivateWithReplyReq);

    req = (xGLXVendorPrivateWithReplyReq *) pc;
    swaps(&req->length);
    swapl(&req->vendorCode);

    vendorcode = req->vendorCode;

    proc = (__GLXdispatchVendorPrivProcPtr)
        __glXGetProtocolDecodeFunction(&VendorPriv_dispatch_info,
                                       vendorcode, 1);
    if (proc != NULL) {
        return (*proc) (cl, (GLbyte *) req);
    }

    cl->client->errorValue = req->vendorCode;
    return __glXError(GLXUnsupportedPrivateRequest);
}

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

#include "dix/dix_priv.h"
#include "dix/request_priv.h"

#include "glxserver.h"
#include "glxutil.h"
#include "glxext.h"
#include "indirect_dispatch.h"
#include "unpack.h"

int
__glXDispSwap_FeedbackBuffer(__GLXclientState * cl, GLbyte * pc)
{
    ClientPtr client = cl->client;
    GLsizei size;
    GLenum type;
    __GLXcontext *cx;
    int error;

    REQUEST_FIXED_SIZE(xGLXSingleReq, 8);

    swapl(&((xGLXSingleReq *) pc)->contextTag);
    cx = __glXForceCurrent(cl, __GLX_GET_SINGLE_CONTEXT_TAG(pc), &error);
    if (!cx) {
        return error;
    }

    pc += __GLX_SINGLE_HDR_SIZE;
    swapl((CARD32*)(pc + 0));
    swapl((CARD32*)(pc + 4));
    size = *(GLsizei *) (pc + 0);
    type = *(GLenum *) (pc + 4);
    if (size < 0) {
        cl->client->errorValue = size;
        return BadValue;
    }
    if (cx->feedbackBufSize < size) {
        cx->feedbackBuf = reallocarray(cx->feedbackBuf,
                                       (size_t) size, __GLX_SIZE_FLOAT32);
        if (!cx->feedbackBuf) {
            cl->client->errorValue = size;
            return BadAlloc;
        }
        cx->feedbackBufSize = size;
    }
    glFeedbackBuffer(size, type, cx->feedbackBuf);
    return Success;
}

int
__glXDispSwap_SelectBuffer(__GLXclientState * cl, GLbyte * pc)
{
    ClientPtr client = cl->client;
    __GLXcontext *cx;
    GLsizei size;
    int error;

    REQUEST_FIXED_SIZE(xGLXSingleReq, 4);

    swapl(&((xGLXSingleReq *) pc)->contextTag);
    cx = __glXForceCurrent(cl, __GLX_GET_SINGLE_CONTEXT_TAG(pc), &error);
    if (!cx) {
        return error;
    }

    pc += __GLX_SINGLE_HDR_SIZE;
    swapl((CARD32*)(pc + 0));
    size = *(GLsizei *) (pc + 0);
    if (size < 0) {
        cl->client->errorValue = size;
        return BadValue;
    }
    if (cx->selectBufSize < size) {
        cx->selectBuf = reallocarray(cx->selectBuf,
                                     (size_t) size, __GLX_SIZE_CARD32);
        if (!cx->selectBuf) {
            cl->client->errorValue = size;
            return BadAlloc;
        }
        cx->selectBufSize = size;
    }
    glSelectBuffer(size, cx->selectBuf);
    return Success;
}

int
__glXDispSwap_RenderMode(__GLXclientState * cl, GLbyte * pc)
{
    ClientPtr client = cl->client;
    __GLXcontext *cx;
    GLint nitems = 0, retval, newModeCheck;
    GLenum newMode;
    int error;

    REQUEST_FIXED_SIZE(xGLXSingleReq, 4);

    swapl(&((xGLXSingleReq *) pc)->contextTag);
    cx = __glXForceCurrent(cl, __GLX_GET_SINGLE_CONTEXT_TAG(pc), &error);
    if (!cx) {
        return error;
    }

    pc += __GLX_SINGLE_HDR_SIZE;
    swapl((CARD32*)pc);
    newMode = *(GLenum *) pc;
    retval = glRenderMode(newMode);

    /* feedback/select payload is CARD32-sized; rpcbuf byte-swaps it when
       client->swapped */
    x_rpcbuf_t rpcbuf = { .swapped = client->swapped, .err_clear = TRUE };

    /* Check that render mode worked */
    glGetIntegerv(GL_RENDER_MODE, &newModeCheck);
    if (newModeCheck != newMode) {
        /* Render mode change failed.  Bail */
        newMode = newModeCheck;
        goto noChangeAllowed;
    }

    /*
     ** Render mode might have still failed if we get here.  But in this
     ** case we can't really tell, nor does it matter.  If it did fail, it
     ** will return 0, and thus we won't send any data across the wire.
     */

    switch (cx->renderMode) {
    case GL_RENDER:
        cx->renderMode = newMode;
        break;
    case GL_FEEDBACK:
        if (retval < 0) {
            /* Overflow happened. Copy the entire buffer */
            nitems = cx->feedbackBufSize;
        }
        else {
            nitems = retval;
        }
        x_rpcbuf_write_CARD32s(&rpcbuf, (CARD32*)cx->feedbackBuf, nitems);
        cx->renderMode = newMode;
        break;
    case GL_SELECT:
        if (retval < 0) {
            /* Overflow happened.  Copy the entire buffer */
            nitems = cx->selectBufSize;
        }
        else {
            GLuint *bp = cx->selectBuf;
            GLint i;

            /*
             ** Figure out how many bytes of data need to be sent.  Parse
             ** the selection buffer to determine this fact as the
             ** return value is the number of hits, not the number of
             ** items in the buffer.
             */
            nitems = 0;
            i = retval;
            while (--i >= 0) {
                GLuint n;

                /* Parse select data for this hit */
                n = *bp;
                bp += 3 + n;
            }
            nitems = bp - cx->selectBuf;
        }
        x_rpcbuf_write_CARD32s(&rpcbuf, (CARD32*)cx->selectBuf, nitems);
        cx->renderMode = newMode;
        break;
    }

    /*
     ** First reply is the number of elements returned in the feedback or
     ** selection array, as per the API for glRenderMode itself.
     */
 noChangeAllowed:;
    xGLXRenderModeReply reply = {
        .retval = retval,
        .size = nitems,
        .newMode = newMode
    };
    X_REPLY_FIELD_CARD32(retval);
    X_REPLY_FIELD_CARD32(size);
    X_REPLY_FIELD_CARD32(newMode);

    return X_SEND_REPLY_WITH_RPCBUF(client, reply, rpcbuf);
}

int
__glXDispSwap_Flush(__GLXclientState * cl, GLbyte * pc)
{
    ClientPtr client = cl->client;
    __GLXcontext *cx;
    int error;

    REQUEST_SIZE_MATCH(xGLXSingleReq);

    swapl(&((xGLXSingleReq *) pc)->contextTag);
    cx = __glXForceCurrent(cl, __GLX_GET_SINGLE_CONTEXT_TAG(pc), &error);
    if (!cx) {
        return error;
    }

    glFlush();
    return Success;
}

int
__glXDispSwap_Finish(__GLXclientState * cl, GLbyte * pc)
{
    ClientPtr client = cl->client;
    __GLXcontext *cx;
    int error;

    REQUEST_SIZE_MATCH(xGLXSingleReq);

    swapl(&((xGLXSingleReq *) pc)->contextTag);
    cx = __glXForceCurrent(cl, __GLX_GET_SINGLE_CONTEXT_TAG(pc), &error);
    if (!cx) {
        return error;
    }

    /* Do a local glFinish */
    glFinish();

    /* Send empty reply packet to indicate finish is finished */
    xGLXSingleReply reply = { 0 };
    return X_SEND_REPLY_SIMPLE(client, reply);
}

int
__glXDispSwap_GetString(__GLXclientState * cl, GLbyte * pc)
{
    return DoGetString(cl, pc, GL_TRUE);
}

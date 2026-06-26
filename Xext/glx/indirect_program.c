/*
 * (C) Copyright IBM Corporation 2005, 2006
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sub license,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDERS, THE AUTHORS, AND/OR THEIR SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
 * OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/**
 * \file indirect_program.c
 * Hand-coded routines needed to support programmable pipeline extensions.
 *
 * \author Ian Romanick <idr@us.ibm.com>
 */
#include <dix-config.h>

#include "dix/request_priv.h"
#include "include/misc.h"

#include "glxserver.h"
#include "glxext.h"
#include "singlesize.h"
#include "unpack.h"
#include "indirect_size_get.h"
#include "indirect_dispatch.h"

/**
 * Handle both types of glGetProgramString calls.
 */
static int
DoGetProgramString(struct __GLXclientStateRec *cl, GLbyte * pc,
                   PFNGLGETPROGRAMIVARBPROC get_programiv,
                   PFNGLGETPROGRAMSTRINGARBPROC get_program_string,
                   Bool do_swap)
{
    xGLXVendorPrivateWithReplyReq *const req =
        (xGLXVendorPrivateWithReplyReq *) pc;
    int error;
    __GLXcontext *const cx =
        __glXForceCurrent(cl,
                          do_swap ? bswap_32(req->contextTag) : req->contextTag,
                          &error);
    ClientPtr client = cl->client;

    REQUEST_FIXED_SIZE(xGLXVendorPrivateWithReplyReq, 8);

    pc += __GLX_VENDPRIV_HDR_SIZE;
    if (cx != NULL) {
        GLenum target;
        GLenum pname;
        GLint compsize = 0;
        char *answer = NULL, answerBuffer[200];
        xGLXGetTexImageReply reply = { 0 };

        if (do_swap) {
            target = (GLenum) bswap_32(*(int *) (pc + 0));
            pname = (GLenum) bswap_32(*(int *) (pc + 4));
        }
        else {
            target = *(GLenum *) (pc + 0);
            pname = *(GLuint *) (pc + 4);
        }

        /* The value of the GL_PROGRAM_LENGTH_ARB and GL_PROGRAM_LENGTH_NV
         * enumerants is the same.
         */
        get_programiv(target, GL_PROGRAM_LENGTH_ARB, &compsize);

        if (compsize != 0) {
            __GLX_GET_ANSWER_BUFFER(answer, cl, compsize, 1);
            __glXClearErrorOccured();

            get_program_string(target, pname, (GLubyte *) answer);
        }

        x_rpcbuf_t rpcbuf = { .swapped = client->swapped, .err_clear = TRUE };
        if (!__glXErrorOccured()) {
            reply.width = compsize;
            X_REPLY_FIELD_CARD32(width);
            x_rpcbuf_write_binary_pad(&rpcbuf, answer, compsize);
        }

        error = X_SEND_REPLY_WITH_RPCBUF(client, reply, rpcbuf);
    }

    return error;
}

int
__glXDisp_GetProgramStringARB(struct __GLXclientStateRec *cl, GLbyte * pc)
{
    PFNGLGETPROGRAMIVARBPROC get_program =
        __glGetProcAddress("glGetProgramivARB");
    PFNGLGETPROGRAMSTRINGARBPROC get_program_string =
        __glGetProcAddress("glGetProgramStringARB");

    return DoGetProgramString(cl, pc, get_program, get_program_string, FALSE);
}

int
__glXDispSwap_GetProgramStringARB(struct __GLXclientStateRec *cl, GLbyte * pc)
{
    PFNGLGETPROGRAMIVARBPROC get_program =
        __glGetProcAddress("glGetProgramivARB");
    PFNGLGETPROGRAMSTRINGARBPROC get_program_string =
        __glGetProcAddress("glGetProgramStringARB");

    return DoGetProgramString(cl, pc, get_program, get_program_string, TRUE);
}

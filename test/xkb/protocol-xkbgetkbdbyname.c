/**
 * Copyright © 2026 Mikhail Dmitrichenko
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a
 *  copy of this software and associated documentation files (the "Software"),
 *  to deal in the Software without restriction, including without limitation
 *  the rights to use, copy, modify, merge, publish, distribute, sublicense,
 *  and/or sell copies of the Software, and to permit persons to whom the
 *  Software is furnished to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice (including the next
 *  paragraph) shall be included in all copies or substantial portions of the
 *  Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 *  THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 *  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 *  DEALINGS IN THE SOFTWARE.
 */

/* Test relies on assert() */
#undef NDEBUG

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

/*
 * Protocol testing for the XkbGetKbdByName request.
 *
 * A load request compiles a keymap with xkbcomp and reads the resulting XKM
 * file back in. Reading a section of that file can fail for reasons that
 * have nothing to do with the file being malformed: every section reader
 * bails out with -1 when it cannot allocate the component it is about to
 * fill in. XkmReadFile() leaves such a section in the mask of missing
 * components it returns, and the description it hands back is non-NULL but
 * incomplete.
 *
 * The tests below drive the request with the client map allocations failing,
 * which is what an out-of-memory condition looks like to the XKM reader. No
 * malformed xkbcomp output is needed to get there.
 *
 * Tests include:
 * A keymap that reads completely is installed and reported as loaded.
 * A keymap whose types and symbols cannot be read is not installed: the
 * reply reports neither section as found, "loaded" stays FALSE, and the
 * device keeps the keymap it already had.
 */

#include <stdint.h>
#include <string.h>
#include <X11/X.h>
#include <X11/Xproto.h>
#include <X11/extensions/XKB.h>
#include <X11/extensions/XKM.h>
#include <X11/extensions/XKBproto.h>
#include "inputstr.h"
#include <xkbsrv.h>

#include "protocol-common.h"
#include "../../xkb/xkb-procs.h"

DECLARE_WRAP_FUNCTION(WriteToClient, void, ClientPtr client, int len, void *data);

/* xkbsrv.h renames the server's copy of XkbAllocClientMap(). */
WRAP_FUNCTION(SrvXkbAllocClientMap, Status,
              XkbDescPtr xkb, unsigned int which, unsigned int nTotalTypes)
{
    IMPLEMENT_WRAP_FUNCTION_WITH_RETURN(SrvXkbAllocClientMap, xkb, which,
                                        nTotalTypes);
}

static ClientRec client_request;

/* The request is followed by six component specs: keymap, keycodes, types,
 * compat, symbols and geometry. All of them are left empty, so the server
 * substitutes "%" and reuses the names of the keymap already loaded. */
static struct {
    xkbGetKbdByNameReq req;
    CARD8 components[6];
    CARD8 pad[2];
} request;

static xkbGetKbdByNameReply reply;
static int nreplies;

static void
reply_XkbGetKbdByName(ClientPtr client, int len, void *data)
{
    /* The map, compat map, names, ... replies for every component the server
     * reports follow this one, we only care about the first. */
    if (nreplies++ > 0)
        return;

    assert(len >= (int) sz_xkbGetKbdByNameReply);
    memcpy(&reply, data, sz_xkbGetKbdByNameReply);
}

static Bool key_types_alloc_failed;

/* Fail every client map allocation, the way an out-of-memory condition
 * would. ReadXkmKeyTypes() asks for the key types on their own,
 * ReadXkmSymbols() for the whole client map, so both sections fail. */
static Status
fail_client_map_alloc(XkbDescPtr xkb, unsigned int which,
                      unsigned int nTotalTypes)
{
    if (which == XkbKeyTypesMask)
        key_types_alloc_failed = TRUE;

    return BadAlloc;
}

static void
request_XkbGetKbdByName(void)
{
    memset(&request, 0, sizeof(request));
    request.req.reqType = 0;    /* the XKB major opcode, unused here */
    request.req.xkbReqType = X_kbGetKbdByName;
    request.req.length = sizeof(request) >> 2;
    request.req.deviceSpec = devices.vck->id;
    request.req.need = 0;
    /* What xkbcomp itself asks for when it installs a keymap. */
    request.req.want = XkbGBN_AllComponentsMask & ~XkbGBN_GeometryMask;
    request.req.load = TRUE;

    client_request = init_client(request.req.length, &request);
    client_request.xkbClientFlags = _XkbClientInitialized;

    nreplies = 0;
    memset(&reply, 0, sizeof(reply));
    wrapped_WriteToClient = reply_XkbGetKbdByName;
    assert(ProcXkbGetKbdByName(&client_request) == Success);
    wrapped_WriteToClient = NULL;
    assert(nreplies > 0);
}

/* A keymap that could be read completely replaces the one on the device. */
static void
test_XkbGetKbdByName_load(void)
{
    XkbDescPtr old;

    init_simple();

    old = devices.vck->key->xkbInfo->desc;
    request_XkbGetKbdByName();

    /* rep.found carries the XKM section bits of the keymap that was read. */
    assert(reply.found & XkmTypesMask);
    assert(reply.found & XkmSymbolsMask);
    assert(reply.loaded == TRUE);
    assert(devices.vck->key->xkbInfo->desc != old);
    assert(devices.vck->key->xkbInfo->desc->map != NULL);
    assert(devices.vck->key->xkbInfo->desc->ctrls != NULL);
    assert(devices.vck->key->xkbInfo->desc->compat != NULL);
}

/* A keymap with unreadable sections must not be installed. Before the fix
 * XkmReadFile() reported both of them as read anyway: it stored the symbols
 * reader's -1 in an unsigned variable, and the key types reader returned the
 * number of bytes it had read before the allocation failed. The request then
 * marked the keymap as loaded no matter what was missing from it, and
 * installing it crashed the server in XkbCopyControls(), which dereferences
 * the controls the incomplete description never got. */
static void
test_XkbGetKbdByName_incomplete(void)
{
    XkbDescPtr old;

    init_simple();

    old = devices.vck->key->xkbInfo->desc;
    wrapped_SrvXkbAllocClientMap = fail_client_map_alloc;
    request_XkbGetKbdByName();
    wrapped_SrvXkbAllocClientMap = NULL;

    assert(key_types_alloc_failed);
    assert((reply.found & XkmTypesMask) == 0);
    assert((reply.found & XkmSymbolsMask) == 0);
    assert(reply.loaded == FALSE);
    assert(devices.vck->key->xkbInfo->desc == old);
    assert(old->map != NULL);
    assert(old->ctrls != NULL);
    assert(old->compat != NULL);
}

const testfunc_t*
protocol_xkbgetkbdbyname_test(void)
{
    static const testfunc_t testfuncs[] = {
        test_XkbGetKbdByName_load,
        test_XkbGetKbdByName_incomplete,
        NULL,
    };

    return testfuncs;
}

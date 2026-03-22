/*
 * Copyright 2007-2008 Peter Hutterer
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Author: Peter Hutterer, University of South Australia, NICTA
 */

#include <dix-config.h>

#include <X11/extensions/ge.h>
#include <X11/extensions/geproto.h>

#include "dix/dix_priv.h"
#include "dix/request_priv.h"
#include "miext/extinit_priv.h"
#include "Xext/geext_priv.h"

#include "windowstr.h"
#include "protocol-versions.h"

#define MAXEXTENSIONS   128

DevPrivateKeyRec GEClientPrivateKeyRec;

/** Struct to keep information about registered extensions */
typedef struct _GEExtension {
    /** Event swapping routine */
    void (*evswap) (xGenericEvent *from, xGenericEvent *to);
} GEExtension, *GEExtensionPtr;

static GEExtension GEExtensions[MAXEXTENSIONS];

typedef struct _GEClientInfo {
    CARD32 major_version;
    CARD32 minor_version;
} GEClientInfoRec, *GEClientInfoPtr;

#define GEGetClient(pClient)    ((GEClientInfoPtr)(dixLookupPrivate(&((pClient)->devPrivates), &GEClientPrivateKeyRec)))

/* Forward declarations */
static void SGEGenericEvent(xEvent *from, xEvent *to);

#define EXT_MASK(ext) ((ext) & 0x7F)

/************************************************************/
/*                request handlers                          */
/************************************************************/

static int
ProcGEQueryVersion(ClientPtr client)
{
    X_REQUEST_HEAD_STRUCT(xGEQueryVersionReq);
    X_REQUEST_FIELD_CARD16(majorVersion);
    X_REQUEST_FIELD_CARD16(minorVersion);

    GEClientInfoPtr pGEClient = GEGetClient(client);

    xGEQueryVersionReply reply = {
        .RepType = X_GEQueryVersion,
        /* return the supported version by the server */
        .majorVersion = SERVER_GE_MAJOR_VERSION,
        .minorVersion = SERVER_GE_MINOR_VERSION
    };

    /* Remember version the client requested */
    pGEClient->major_version = stuff->majorVersion;
    pGEClient->minor_version = stuff->minorVersion;

    if (client->swapped) {
        swaps(&reply.majorVersion);
        swaps(&reply.minorVersion);
    }

    return X_SEND_REPLY_SIMPLE(client, reply);
}

/************************************************************/
/*                callbacks                                 */
/************************************************************/

/* dispatch requests */
static int
ProcGEDispatch(ClientPtr client)
{
    REQUEST(xReq);

    switch (stuff->data) {
    case X_GEQueryVersion:
        return ProcGEQueryVersion(client);
    default:
        return BadRequest;
    }
}

/* Reset extension. Called on server shutdown. */
static void
GEResetProc(ExtensionEntry * extEntry)
{
    EventSwapVector[GenericEvent] = NotImplemented;
}

/*  Calls the registered event swap function for the extension.
 *
 *  Each extension can register a swap function to handle GenericEvents being
 *  swapped properly. The server calls SGEGenericEvent() before the event is
 *  written on the wire, this one calls the registered swap function to do the
 *  work.
 */
static void _X_COLD
SGEGenericEvent(xEvent *from, xEvent *to)
{
    xGenericEvent *gefrom = (xGenericEvent *) from;
    xGenericEvent *geto = (xGenericEvent *) to;

    if ((gefrom->extension & 0x7f) > MAXEXTENSIONS) {
        ErrorF("GE: Invalid extension offset for event.\n");
        return;
    }

    if (GEExtensions[EXT_MASK(gefrom->extension)].evswap)
        GEExtensions[EXT_MASK(gefrom->extension)].evswap(gefrom, geto);
}

/* Init extension, register at server.
 * Since other extensions may rely on XGE (XInput does already), it is a good
 * idea to init XGE first, before any other extension.
 */
void
GEExtensionInit(void)
{
    if (!dixRegisterPrivateKey
        (&GEClientPrivateKeyRec, PRIVATE_CLIENT, sizeof(GEClientInfoRec)))
        FatalError("GEExtensionInit: GE private request failed.\n");

    if (!AddExtension(GE_NAME, 0, GENumberErrors, ProcGEDispatch, ProcGEDispatch,
                      GEResetProc, StandardMinorOpcode))
        FatalError("GEInit: AddExtensions failed.\n");

    memset(GEExtensions, 0, sizeof(GEExtensions));
    EventSwapVector[GenericEvent] = (EventSwapPtr) SGEGenericEvent;
}

/************************************************************/
/*                interface for extensions                  */
/************************************************************/

/* Register an extension with GE. The given swap function will be called each
 * time an event is sent to a client with different byte order.
 * @param extension The extensions major opcode
 * @param ev_swap The event swap function.
 * @param ev_fill Called for an event before delivery. The extension now has
 * the chance to fill in necessary fields for the event.
 */
void
GERegisterExtension(int extension,
                    void (*ev_swap) (xGenericEvent *from, xGenericEvent *to))
{
    if (EXT_MASK(extension) >= MAXEXTENSIONS)
        FatalError("GE: extension > MAXEXTENSIONS. This should not happen.\n");

    /* extension opcodes are > 128, might as well save some space here */
    GEExtensions[EXT_MASK(extension)].evswap = ev_swap;
}

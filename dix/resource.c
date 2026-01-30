/************************************************************

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

********************************************************/
/* The panoramix components contained the following notice */
/*****************************************************************

Copyright (c) 1991, 1997 Digital Equipment Corporation, Maynard, Massachusetts.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software.

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
DIGITAL EQUIPMENT CORPORATION BE LIABLE FOR ANY CLAIM, DAMAGES, INCLUDING,
BUT NOT LIMITED TO CONSEQUENTIAL OR INCIDENTAL DAMAGES, OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR
IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of Digital Equipment Corporation
shall not be used in advertising or otherwise to promote the sale, use or other
dealings in this Software without prior written authorization from Digital
Equipment Corporation.

******************************************************************/
/* XSERVER_DTRACE additions:
 * Copyright (c) 2005-2006, Oracle and/or its affiliates.
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
 */

/*	Routines to manage various kinds of resources:
 *
 *	CreateNewResourceType, CreateNewResourceClass, InitClientResources,
 *	FakeClientID, AddResource, FreeResource, FreeClientResources,
 *	FreeAllResources, LookupIDByType, LookupIDByClass, GetXIDRange
 */

/*
 *      A resource ID is a 32 bit quantity, the upper 2 bits of which are
 *	off-limits for client-visible resources.  The next 8 bits are
 *      used as client ID, and the low 22 bits come from the client.
 *	A resource ID is "hashed" by extracting and xoring subfields
 *      (varying with the size of the hash table).
 *
 *      It is sometimes necessary for the server to create an ID that looks
 *      like it belongs to a client.  This ID, however,  must not be one
 *      the client actually can create, or we have the potential for conflict.
 *      The 31st bit of the ID is reserved for the server's use for this
 *      purpose.  By setting dixClientIdForXID(id) to the client, the SERVER_BIT to
 *      1, and an otherwise arbitrary ID in the low 22 bits, we can create a
 *      resource "owned" by the client.
 */

#include <dix-config.h>

#include <X11/X.h>

#include "dix/colormap_priv.h"
#include "dix/dix_priv.h"
#include "dix/dixgrabs_priv.h"
#include "dix/gc_priv.h"
#include "dix/registry_priv.h"
#include "dix/resource_priv.h"
#include "include/extinit.h"
#include "os/osdep.h"
#include "os/probes_priv.h"
#include "Xext/panoramiX.h"
#include "Xext/panoramiXsrv.h"

#include "misc.h"
#include "os.h"
#include "resource.h"
#include "dixstruct.h"
#include "opaque.h"
#include "windowstr.h"
#include "dixfont.h"
#include "colormap.h"
#include "inputstr.h"
#include "cursor.h"
#include "xace.h"
#include <assert.h>
#include "gcstruct.h"
#include <libxht/xht.h>

#ifdef XSERVER_DTRACE

#define TypeNameString(t) LookupResourceName(t)
#endif

#define SERVER_MINID 32

#define INITBUCKETS 64

typedef struct _Resource {
    RESTYPE type;
    void *value;
} ResourceRec, *ResourcePtr;

typedef struct _ClientResource {
    xht_t *resources;
    XID fakeID;
    XID endFakeID;
} ClientResourceRec;

RESTYPE lastResourceType;
static RESTYPE lastResourceClass;
RESTYPE TypeMask;

struct ResourceType {
    DeleteType deleteFunc;
    SizeType sizeFunc;
    FindTypeSubResources findSubResFunc;
    int errorValue;
};

/**
 * Used by all resources that don't specify a function to calculate
 * resource size. Currently this is used for all resources with
 * insignificant memory usage.
 *
 * @see GetResourceTypeSizeFunc, SetResourceTypeSizeFunc
 *
 * @param[in] value Pointer to resource object.
 *
 * @param[in] id Resource ID for the object.
 *
 * @param[out] size Fill all fields to zero to indicate that size of
 *                  resource can't be determined.
 */
static void
GetDefaultBytes(void *value, XID id, ResourceSizePtr size)
{
    size->resourceSize = 0;
    size->pixmapRefSize = 0;
    size->refCnt = 1;
}

/**
 * Used by all resources that don't specify a function to iterate
 * through subresources. Currently this is used for all resources with
 * insignificant memory usage.
 *
 * @see FindSubResources, SetResourceTypeFindSubResFunc
 *
 * @param[in] value Pointer to resource object.
 *
 * @param[in] func Function to call for each subresource.

 * @param[out] cdata Pointer to opaque data.
 */
static void
DefaultFindSubRes(void *value, FindAllRes func, void *cdata)
{
    /* do nothing */
}

/**
 * Calculate drawable size in bytes. Reference counting is not taken
 * into account.
 *
 * @param[in] drawable Pointer to a drawable.
 *
 * @return Estimate of total memory usage for the drawable.
 */
static unsigned long
GetDrawableBytes(DrawablePtr drawable)
{
    int bytes = 0;

    if (drawable)
    {
        int bytesPerPixel = drawable->bitsPerPixel >> 3;
        int numberOfPixels = drawable->width * drawable->height;
        bytes = numberOfPixels * bytesPerPixel;
    }

    return bytes;
}

/**
 * Calculate pixmap size in bytes. Reference counting is taken into
 * account. Any extra data attached by extensions and drivers is not
 * taken into account. The purpose of this function is to estimate
 * memory usage that can be attributed to single reference of the
 * pixmap.
 *
 * @param[in] value Pointer to a pixmap.
 *
 * @param[in] id Resource ID of pixmap. If the pixmap hasn't been
 *               added as resource, just pass value->drawable.id.
 *
 * @param[out] size Estimate of memory usage attributed to a single
 *                  pixmap reference.
 */
static void
GetPixmapBytes(void *value, XID id, ResourceSizePtr size)
{
    PixmapPtr pixmap = value;

    size->resourceSize = 0;
    size->pixmapRefSize = 0;
    size->refCnt = pixmap->refcnt;

    if (pixmap && pixmap->refcnt)
    {
        DrawablePtr drawable = &pixmap->drawable;
        size->resourceSize = GetDrawableBytes(drawable);
        size->pixmapRefSize = size->resourceSize / pixmap->refcnt;
    }
}

/**
 * Calculate window size in bytes. The purpose of this function is to
 * estimate memory usage that can be attributed to all pixmap
 * references of the window.
 *
 * @param[in] value Pointer to a window.
 *
 * @param[in] id Resource ID of window.
 *
 * @param[out] size Estimate of memory usage attributed to a all
 *                  pixmap references of a window.
 */
static void
GetWindowBytes(void *value, XID id, ResourceSizePtr size)
{
    SizeType pixmapSizeFunc = GetResourceTypeSizeFunc(X11_RESTYPE_PIXMAP);
    ResourceSizeRec pixmapSize = { 0, 0, 0 };
    WindowPtr window = value;

    /* Currently only pixmap bytes are reported to clients. */
    size->resourceSize = 0;

    /* Calculate pixmap reference sizes. */
    size->pixmapRefSize = 0;

    size->refCnt = 1;

    if (window->backgroundState == BackgroundPixmap)
    {
        PixmapPtr pixmap = window->background.pixmap;
        pixmapSizeFunc(pixmap, pixmap->drawable.id, &pixmapSize);
        size->pixmapRefSize += pixmapSize.pixmapRefSize;
    }
    if (window->border.pixmap && !window->borderIsPixel)
    {
        PixmapPtr pixmap = window->border.pixmap;
        pixmapSizeFunc(pixmap, pixmap->drawable.id, &pixmapSize);
        size->pixmapRefSize += pixmapSize.pixmapRefSize;
    }
}

/**
 * Iterate through subresources of a window. The purpose of this
 * function is to gather accurate information on what resources
 * a resource uses.
 *
 * @note Currently only sub-pixmaps are iterated
 *
 * @param[in] value  Pointer to a window
 *
 * @param[in] func   Function to call with each subresource
 *
 * @param[out] cdata Pointer to opaque data
 */
static void
FindWindowSubRes(void *value, FindAllRes func, void *cdata)
{
    WindowPtr window = value;

    /* Currently only pixmap subresources are reported to clients. */

    if (window->backgroundState == BackgroundPixmap)
    {
        PixmapPtr pixmap = window->background.pixmap;
        func(window->background.pixmap, pixmap->drawable.id, X11_RESTYPE_PIXMAP, cdata);
    }
    if (window->border.pixmap && !window->borderIsPixel)
    {
        PixmapPtr pixmap = window->border.pixmap;
        func(window->background.pixmap, pixmap->drawable.id, X11_RESTYPE_PIXMAP, cdata);
    }
}

/**
 * Calculate graphics context size in bytes. The purpose of this
 * function is to estimate memory usage that can be attributed to all
 * pixmap references of the graphics context.
 *
 * @param[in] value Pointer to a graphics context.
 *
 * @param[in] id    Resource ID of graphics context.
 *
 * @param[out] size Estimate of memory usage attributed to a all
 *                  pixmap references of a graphics context.
 */
static void
GetGcBytes(void *value, XID id, ResourceSizePtr size)
{
    SizeType pixmapSizeFunc = GetResourceTypeSizeFunc(X11_RESTYPE_PIXMAP);
    ResourceSizeRec pixmapSize = { 0, 0, 0 };
    GCPtr gc = value;

    /* Currently only pixmap bytes are reported to clients. */
    size->resourceSize = 0;

    /* Calculate pixmap reference sizes. */
    size->pixmapRefSize = 0;

    size->refCnt = 1;
    if (gc->stipple)
    {
        PixmapPtr pixmap = gc->stipple;
        pixmapSizeFunc(pixmap, pixmap->drawable.id, &pixmapSize);
        size->pixmapRefSize += pixmapSize.pixmapRefSize;
    }
    if (gc->tile.pixmap && !gc->tileIsPixel)
    {
        PixmapPtr pixmap = gc->tile.pixmap;
        pixmapSizeFunc(pixmap, pixmap->drawable.id, &pixmapSize);
        size->pixmapRefSize += pixmapSize.pixmapRefSize;
    }
}

/**
 * Iterate through subresources of a graphics context. The purpose of
 * this function is to gather accurate information on what resources a
 * resource uses.
 *
 * @note Currently only sub-pixmaps are iterated
 *
 * @param[in] value  Pointer to a window
 *
 * @param[in] func   Function to call with each subresource
 *
 * @param[out] cdata Pointer to opaque data
 */
static void
FindGCSubRes(void *value, FindAllRes func, void *cdata)
{
    GCPtr gc = value;

    /* Currently only pixmap subresources are reported to clients. */

    if (gc->stipple)
    {
        PixmapPtr pixmap = gc->stipple;
        func(pixmap, pixmap->drawable.id, X11_RESTYPE_PIXMAP, cdata);
    }
    if (gc->tile.pixmap && !gc->tileIsPixel)
    {
        PixmapPtr pixmap = gc->tile.pixmap;
        func(pixmap, pixmap->drawable.id, X11_RESTYPE_PIXMAP, cdata);
    }
}

static struct ResourceType *resourceTypes;

static const struct ResourceType predefTypes[] = {
    [X11_RESTYPE_NONE & (RC_LASTPREDEF - 1)] = {
                                       .deleteFunc = (DeleteType) NoopDDA,
                                       .sizeFunc = GetDefaultBytes,
                                       .findSubResFunc = DefaultFindSubRes,
                                       .errorValue = BadValue,
                                       },
    [X11_RESTYPE_WINDOW & (RC_LASTPREDEF - 1)] = {
                                         .deleteFunc = DeleteWindow,
                                         .sizeFunc = GetWindowBytes,
                                         .findSubResFunc = FindWindowSubRes,
                                         .errorValue = BadWindow,
                                         },
    [X11_RESTYPE_PIXMAP & (RC_LASTPREDEF - 1)] = {
                                         .deleteFunc = dixDestroyPixmap,
                                         .sizeFunc = GetPixmapBytes,
                                         .findSubResFunc = DefaultFindSubRes,
                                         .errorValue = BadPixmap,
                                         },
    [X11_RESTYPE_GC & (RC_LASTPREDEF - 1)] = {
                                     .deleteFunc = FreeGC,
                                     .sizeFunc = GetGcBytes,
                                     .findSubResFunc = FindGCSubRes,
                                     .errorValue = BadGC,
                                     },
    [X11_RESTYPE_FONT & (RC_LASTPREDEF - 1)] = {
                                       .deleteFunc = CloseFont,
                                       .sizeFunc = GetDefaultBytes,
                                       .findSubResFunc = DefaultFindSubRes,
                                       .errorValue = BadFont,
                                       },
    [X11_RESTYPE_CURSOR & (RC_LASTPREDEF - 1)] = {
                                         .deleteFunc = FreeCursor,
                                         .sizeFunc = GetDefaultBytes,
                                         .findSubResFunc = DefaultFindSubRes,
                                         .errorValue = BadCursor,
                                         },
    [X11_RESTYPE_COLORMAP & (RC_LASTPREDEF - 1)] = {
                                           .deleteFunc = FreeColormap,
                                           .sizeFunc = GetDefaultBytes,
                                           .findSubResFunc = DefaultFindSubRes,
                                           .errorValue = BadColor,
                                           },
    [X11_RESTYPE_CMAPENTRY & (RC_LASTPREDEF - 1)] = {
                                            .deleteFunc = FreeClientPixels,
                                            .sizeFunc = GetDefaultBytes,
                                            .findSubResFunc = DefaultFindSubRes,
                                            .errorValue = BadColor,
                                            },
    [X11_RESTYPE_OTHERCLIENT & (RC_LASTPREDEF - 1)] = {
                                              .deleteFunc = OtherClientGone,
                                              .sizeFunc = GetDefaultBytes,
                                              .findSubResFunc = DefaultFindSubRes,
                                              .errorValue = BadValue,
                                              },
    [X11_RESTYPE_PASSIVEGRAB & (RC_LASTPREDEF - 1)] = {
                                              .deleteFunc = DeletePassiveGrab,
                                              .sizeFunc = GetDefaultBytes,
                                              .findSubResFunc = DefaultFindSubRes,
                                              .errorValue = BadValue,
                                              },
};

CallbackListPtr ResourceStateCallback;

static inline void
CallResourceStateCallback(ResourceState state, XID id, RESTYPE type, void *value)
{
    if (ResourceStateCallback) {
        ResourceStateInfoRec rsi = { state, id, type, value };
        CallCallbacks(&ResourceStateCallback, &rsi);
    }
}

RESTYPE
CreateNewResourceType(DeleteType deleteFunc, const char *name)
{
    RESTYPE next = lastResourceType + 1;
    struct ResourceType *types;

    if (next & lastResourceClass)
        return 0;
    types = reallocarray(resourceTypes, next + 1, sizeof(*resourceTypes));
    if (!types)
        return 0;

    lastResourceType = next;
    resourceTypes = types;
    resourceTypes[next].deleteFunc = deleteFunc;
    resourceTypes[next].sizeFunc = GetDefaultBytes;
    resourceTypes[next].findSubResFunc = DefaultFindSubRes;
    resourceTypes[next].errorValue = BadValue;

#if X_REGISTRY_RESOURCE
    /* Called even if name is NULL, to remove any previous entry */
    RegisterResourceName(next, name);
#endif

    return next;
}

/**
 * Get the function used to calculate resource size. Extensions and
 * drivers need to be able to determine the current size calculation
 * function if they want to wrap or override it.
 *
 * @param[in] type     Resource type used in size calculations.
 *
 * @return Function to calculate the size of a single
 *                     resource.
 */
SizeType
GetResourceTypeSizeFunc(RESTYPE type)
{
    return resourceTypes[type & TypeMask].sizeFunc;
}

/**
 * Override the default function that calculates resource size. For
 * example, video driver knows better how to calculate pixmap memory
 * usage and can therefore wrap or override size calculation for
 * X11_RESTYPE_PIXMAP.
 *
 * @param[in] type     Resource type used in size calculations.
 *
 * @param[in] sizeFunc Function to calculate the size of a single
 *                     resource.
 */
void
SetResourceTypeSizeFunc(RESTYPE type, SizeType sizeFunc)
{
    resourceTypes[type & TypeMask].sizeFunc = sizeFunc;
}

/**
 * Provide a function for iterating the subresources of a resource.
 * This allows for example more accurate accounting of the (memory)
 * resources consumed by a resource.
 *
 * @see FindSubResources
 *
 * @param[in] type     Resource type used in size calculations.
 *
 * @param[in] sizeFunc Function to calculate the size of a single
 *                     resource.
 */
void
SetResourceTypeFindSubResFunc(RESTYPE type, FindTypeSubResources findFunc)
{
    resourceTypes[type & TypeMask].findSubResFunc = findFunc;
}

void
SetResourceTypeErrorValue(RESTYPE type, int errorValue)
{
    resourceTypes[type & TypeMask].errorValue = errorValue;
}

RESTYPE
CreateNewResourceClass(void)
{
    RESTYPE next = lastResourceClass >> 1;

    if (next & lastResourceType)
        return 0;
    lastResourceClass = next;
    TypeMask = next - 1;
    return next;
}

static ClientResourceRec clientTable[MAXCLIENTS];

static unsigned int
ilog2(int val)
{
    int bits;

    if (val <= 0)
	return 0;
    for (bits = 0; val != 0; bits++)
	val >>= 1;
    return bits - 1;
}

/*****************
 * ResourceClientBits
 *    Returns the client bit offset in the client + resources ID field
 *****************/

unsigned int
ResourceClientBits(void)
{
    static unsigned int cache_ilog2 = 0;
    static unsigned int cache_limit = 0;

    if (LimitClients != cache_limit) {
        cache_limit = LimitClients;
        cache_ilog2 = ilog2(LimitClients);
    }

    return cache_ilog2;
}

/*****************
 * InitClientResources
 *    When a new client is created, call this to allocate space
 *    in resource table
 *****************/

Bool
InitClientResources(ClientPtr client)
{
    int i;

    if (client == serverClient) {
        lastResourceType = X11_RESTYPE_LASTPREDEF;
        lastResourceClass = RC_LASTPREDEF;
        TypeMask = RC_LASTPREDEF - 1;
        free(resourceTypes);
        resourceTypes = calloc(1, sizeof(predefTypes));
        if (!resourceTypes)
            return FALSE;
        memcpy(resourceTypes, predefTypes, sizeof(predefTypes));
    }
    i = client->index;
    clientTable[i].resources = xht_create_int_table(INITBUCKETS);
    if (!clientTable[i].resources)
        return FALSE;
    /* Many IDs allocated from the server client are visible to clients,
     * so we don't use the SERVER_BIT for them, but we have to start
     * past the magic value constants used in the protocol.  For normal
     * clients, we can start from zero, with SERVER_BIT set.
     */
    clientTable[i].fakeID = client->clientAsMask |
        (client->index ? SERVER_BIT : SERVER_MINID);
    clientTable[i].endFakeID = (clientTable[i].fakeID | RESOURCE_ID_MASK) + 1;
    return TRUE;
}

static void find_unused_ids_iter(uint64_t key, void *data, void *cdata)
{
    XID *ids = cdata;
    XID id = (XID)key;
    if (id >= ids[0] && id <= ids[1]) {
        if (id == ids[0])
            ids[0]++;
        if (id == ids[1])
            ids[1]--;
    }
}

void
GetXIDRange(int client, Bool server, XID *minp, XID *maxp)
{
    XID ids[2];

    ids[0] = (Mask) client << CLIENTOFFSET;
    if (server)
        ids[0] |= client ? SERVER_BIT : SERVER_MINID;
    ids[1] = ids[0] | RESOURCE_ID_MASK;

    xht_iter_int(clientTable[client].resources, find_unused_ids_iter, ids);

    if (ids[0] > ids[1])
        ids[0] = ids[1] = 0;
    *minp = ids[0];
    *maxp = ids[1];
}

/**
 *  GetXIDList is called by the XC-MISC extension's MiscGetXIDList function.
 *  This function tries to find count unused XIDs for the given client.  It
 *  puts the IDs in the array pids and returns the number found, which should
 *  almost always be the number requested.
 *
 *  The circumstances that lead to a call to this function are very rare.
 *  Xlib must run out of IDs while trying to generate a request that wants
 *  multiple ID's, like the Multi-buffering CreateImageBuffers request.
 *
 *  No rocket science in the implementation; just iterate over all
 *  possible IDs for the given client and pick the first count IDs
 *  that aren't in use.  A more efficient algorithm could probably be
 *  invented, but this will be used so rarely that this should suffice.
 */

unsigned int
GetXIDList(ClientPtr pClient, unsigned count, XID *pids)
{
    unsigned int found = 0;
    XID id = pClient->clientAsMask;
    XID maxid;

    maxid = id | RESOURCE_ID_MASK;
    while ((found < count) && (id <= maxid)) {
        if (!xht_get_int(clientTable[pClient->index].resources, id)) {
            pids[found++] = id;
        }
        id++;
    }
    return found;
}

/*
 * Return the next usable fake client ID.
 *
 * Normally this is just the next one in line, but if we've used the last
 * in the range, we need to find a new range of safe IDs to avoid
 * over-running another client.
 */

XID
FakeClientID(int client)
{
    XID id, maxid;

    id = clientTable[client].fakeID++;

    // extra paranoid protection, because many places expect 0 as
    // sign for resource not existing
    if (!id)
        return FakeClientID(client);

    if (id != clientTable[client].endFakeID)
        return id;
    GetXIDRange(client, TRUE, &id, &maxid);
    if (!id) {
        if (!client)
            FatalError("FakeClientID: server internal ids exhausted\n");
        dixMarkClientException(clients[client]);
        id = ((Mask) client << CLIENTOFFSET) | (SERVER_BIT * 3);
        maxid = id | RESOURCE_ID_MASK;
    }
    clientTable[client].fakeID = id + 1;
    clientTable[client].endFakeID = maxid + 1;

    if (!id)
        return FakeClientID(client);

    return id;
}

Bool
AddResource(XID id, RESTYPE type, void *value)
{
    int client;
    ClientResourceRec *rrec;
    ResourceRec res;

#ifdef XSERVER_DTRACE
    XSERVER_RESOURCE_ALLOC(id, type, value, TypeNameString(type));
#endif
    client = dixClientIdForXID(id);
    rrec = &clientTable[client];
    if (!rrec->resources) {
        ErrorF("[dix] AddResource(%lx, %x, %lx), client=%d \n",
               (unsigned long) id, type, (unsigned long) value, client);
        FatalError("client not in use\n");
    }

    res.type = type;
    res.value = value;
    if (!xht_set_int(rrec->resources, id, &res)) {
        (*resourceTypes[type & TypeMask].deleteFunc) (value, id);
        return FALSE;
    }
    CallResourceStateCallback(ResourceStateAdding, id, type, value);
    return TRUE;
}

static void
doFreeResource(XID id, ResourcePtr res, Bool skip)
{
    CallResourceStateCallback(ResourceStateFreeing, id, res->type, res->value);

    if (!skip)
        resourceTypes[res->type & TypeMask].deleteFunc(res->value, id);

    free(res);
}

void
FreeResource(XID id, RESTYPE skipDeleteFuncType)
{
    int cid;
    ResourcePtr res;

    if (((cid = dixClientIdForXID(id)) < LimitClients) && clientTable[cid].resources) {
        res = xht_get_int(clientTable[cid].resources, id);
        if (res) {
#ifdef XSERVER_DTRACE
            XSERVER_RESOURCE_FREE(id, res->type, res->value, TypeNameString(res->type));
#endif
            xht_delete_int(clientTable[cid].resources, id);
            doFreeResource(id, res, res->type == skipDeleteFuncType);
        }
    }
}

void
FreeResourceByType(XID id, RESTYPE type, Bool skipFree)
{
    int cid;
    ResourcePtr res;

    if (((cid = dixClientIdForXID(id)) < LimitClients) && clientTable[cid].resources) {
        res = xht_get_int(clientTable[cid].resources, id);
        if (res && res->type == type) {
#ifdef XSERVER_DTRACE
            XSERVER_RESOURCE_FREE(id, res->type, res->value, TypeNameString(res->type));
#endif
            xht_delete_int(clientTable[cid].resources, id);
            doFreeResource(id, res, skipFree);
        }
    }
}

/*
 * Change the value associated with a resource id.  Caller
 * is responsible for "doing the right thing" with the old
 * data
 */

Bool
ChangeResourceValue(XID id, RESTYPE rtype, void *value)
{
    int cid;
    ResourcePtr res;

    if (((cid = dixClientIdForXID(id)) < LimitClients) && clientTable[cid].resources) {
        res = xht_get_int(clientTable[cid].resources, id);
        if (res && res->type == rtype) {
            res->value = value;
            return TRUE;
        }
    }
    return FALSE;
}

struct find_cdata {
    FindResType func;
    void *cdata;
    RESTYPE type;
};

static void find_client_res_iter(uint64_t key, void *data, void *user_data)
{
    ResourcePtr res = data;
    struct find_cdata *fd = user_data;
    if (!fd->type || res->type == fd->type) {
        fd->func(res->value, (XID)key, fd->cdata);
    }
}

void
FindClientResourcesByType(ClientPtr client,
                          RESTYPE type, FindResType func, void *cdata)
{
    if (!client)
        client = serverClient;

    struct find_cdata fd = { func, cdata, type };
    xht_iter_int(clientTable[client->index].resources, find_client_res_iter, &fd);
}

void FindSubResources(void *resource,
                      RESTYPE    type,
                      FindAllRes func,
                      void *cdata)
{
    struct ResourceType rtype = resourceTypes[type & TypeMask];
    rtype.findSubResFunc(resource, func, cdata);
}

struct find_all_cdata {
    FindAllRes func;
    void *cdata;
};

static void find_all_client_res_iter(uint64_t key, void *data, void *user_data)
{
    ResourcePtr res = data;
    struct find_all_cdata *fd = user_data;
    fd->func(res->value, (XID)key, res->type, fd->cdata);
}

void
FindAllClientResources(ClientPtr client, FindAllRes func, void *cdata)
{
    if (!client)
        client = serverClient;

    struct find_all_cdata fd = { func, cdata };
    xht_iter_int(clientTable[client->index].resources, find_all_client_res_iter, &fd);
}

struct find_complex_cdata {
    FindComplexResType func;
    void *cdata;
    RESTYPE type;
    void *value;
};

static void find_complex_res_iter(uint64_t key, void *data, void *user_data)
{
    ResourcePtr res = data;
    struct find_complex_cdata *fd = user_data;
    if (!fd->value && (!fd->type || res->type == fd->type)) {
        if (fd->func(res->value, (XID)key, fd->cdata))
            fd->value = res->value;
    }
}

void *
LookupClientResourceComplex(ClientPtr client,
                            RESTYPE type,
                            FindComplexResType func, void *cdata)
{
    if (!client)
        client = serverClient;

    struct find_complex_cdata fd = { func, cdata, type, NULL };
    xht_iter_int(clientTable[client->index].resources, find_complex_res_iter, &fd);
    return fd.value;
}

static void free_never_retain_iter(uint64_t key, void *data, void *user_data)
{
    ResourcePtr res = data;
    xht_t *table = user_data;
    if (res->type & RC_NEVERRETAIN) {
        XID id = (XID)key;
#ifdef XSERVER_DTRACE
        XSERVER_RESOURCE_FREE(id, res->type, res->value, TypeNameString(res->type));
#endif
        xht_delete_int(table, id);
        doFreeResource(id, res, FALSE);
    }
}

void
FreeClientNeverRetainResources(ClientPtr client)
{
    if (!client)
        return;

    xht_iter_int(clientTable[client->index].resources, free_never_retain_iter, clientTable[client->index].resources);
}

static void free_client_res_iter(uint64_t key, void *data, void *user_data)
{
    ResourcePtr res = data;
    XID id = (XID)key;
#ifdef XSERVER_DTRACE
    XSERVER_RESOURCE_FREE(id, res->type, res->value, TypeNameString(res->type));
#endif
    doFreeResource(id, res, FALSE);
}

void
FreeClientResources(ClientPtr client)
{
    /* This routine shouldn't be called with a null client, but just in
       case ... */

    if (!client)
        return;

    HandleSaveSet(client);

    xht_iter_int(clientTable[client->index].resources, free_client_res_iter, NULL);
    xht_destroy_int_table(clientTable[client->index].resources);
    clientTable[client->index].resources = NULL;
}

void
FreeAllResources(void)
{
    for (int i = currentMaxClients; --i >= 0;) {
        if (clientTable[i].resources)
            FreeClientResources(clients[i]);
    }
}

Bool
LegalNewID(XID id, ClientPtr client)
{
#ifdef XINERAMA
    XID minid, maxid;

    if (!noPanoramiXExtension) {
        minid = client->clientAsMask | (client->index ?
                                        SERVER_BIT : SERVER_MINID);
        maxid = (clientTable[client->index].fakeID | RESOURCE_ID_MASK) + 1;
        if ((id >= minid) && (id <= maxid))
            return TRUE;
    }
#endif /* XINERAMA */
    if (client->clientAsMask == (id & ~RESOURCE_ID_MASK)) {
        return !xht_get_int(clientTable[client->index].resources, id);
    }
    return FALSE;
}

int
dixLookupResourceByType(void **result, XID id, RESTYPE rtype,
                        ClientPtr client, Mask mode)
{
    int cid = dixClientIdForXID(id);
    ResourcePtr res = NULL;

    *result = NULL;
    if ((rtype & TypeMask) > lastResourceType)
        return BadImplementation;

    if ((cid < LimitClients) && clientTable[cid].resources) {
        res = xht_get_int(clientTable[cid].resources, id);
        if (res && res->type != rtype)
            res = NULL;
    }
    if (client) {
        client->errorValue = id;
    }
    if (!res)
        return resourceTypes[rtype & TypeMask].errorValue;

    if (client) {
        cid = XaceHookResourceAccess(client, id, res->type,
                       res->value, X11_RESTYPE_NONE, NULL, mode);
        if (cid == BadValue)
            return resourceTypes[rtype & TypeMask].errorValue;
        if (cid != Success)
            return cid;
    }

    *result = res->value;
    return Success;
}

int
dixLookupResourceByClass(void **result, XID id, RESTYPE rclass,
                         ClientPtr client, Mask mode)
{
    int cid = dixClientIdForXID(id);
    ResourcePtr res = NULL;

    *result = NULL;

    if ((cid < LimitClients) && clientTable[cid].resources) {
        res = xht_get_int(clientTable[cid].resources, id);
        if (res && !(res->type & rclass))
            res = NULL;
    }
    if (client) {
        client->errorValue = id;
    }
    if (!res)
        return BadValue;

    if (client) {
        cid = XaceHookResourceAccess(client, id, res->type,
                       res->value, X11_RESTYPE_NONE, NULL, mode);
        if (cid != Success)
            return cid;
    }

    *result = res->value;
    return Success;
}

/* new API - try not to call FakeClientID() directly anymore */
XID dixAllocServerXID(void)
{
    return FakeClientID(0);
}

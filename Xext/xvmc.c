
#include <dix-config.h>

#include <string.h>
#include <X11/X.h>
#include <X11/Xfuncproto.h>
#include <X11/Xproto.h>
#include <X11/extensions/XvMC.h>
#include <X11/extensions/Xvproto.h>
#include <X11/extensions/XvMCproto.h>

#include "dix/dix_priv.h"
#include "dix/request_priv.h"
#include "dix/screen_hooks_priv.h"
#include "miext/extinit_priv.h"
#include "Xext/xvdix_priv.h"

#include "misc.h"
#include "os.h"
#include "dixstruct.h"
#include "resource.h"
#include "scrnintstr.h"
#include "extnsionst.h"
#include "servermd.h"
#include "xvmcext.h"

#define SERVER_XVMC_MAJOR_VERSION               1
#define SERVER_XVMC_MINOR_VERSION               1

#define DR_CLIENT_DRIVER_NAME_SIZE 48
#define DR_BUSID_SIZE 48

static DevPrivateKeyRec XvMCScreenKeyRec;
static Bool XvMCInUse;

int XvMCReqCode;
int XvMCEventBase;

static RESTYPE XvMCRTContext;
static RESTYPE XvMCRTSurface;
static RESTYPE XvMCRTSubpicture;

typedef struct {
    int num_adaptors;
    XvMCAdaptorPtr adaptors;
    char clientDriverName[DR_CLIENT_DRIVER_NAME_SIZE];
    char busID[DR_BUSID_SIZE];
    int major;
    int minor;
    int patchLevel;
} XvMCScreenRec, *XvMCScreenPtr;

#define XVMC_GET_PRIVATE(pScreen) \
    (XvMCScreenPtr)(dixLookupPrivate(&(pScreen)->devPrivates, &XvMCScreenKeyRec))

static int
XvMCDestroyContextRes(void *data, XID id)
{
    XvMCContextPtr pContext = (XvMCContextPtr) data;

    pContext->refcnt--;

    if (!pContext->refcnt) {
        XvMCScreenPtr pScreenPriv = XVMC_GET_PRIVATE(pContext->pScreen);

        (*pScreenPriv->adaptors[pContext->adapt_num].DestroyContext) (pContext);
        free(pContext);
    }

    return Success;
}

static int
XvMCDestroySurfaceRes(void *data, XID id)
{
    XvMCSurfacePtr pSurface = (XvMCSurfacePtr) data;
    XvMCContextPtr pContext = pSurface->context;
    XvMCScreenPtr pScreenPriv = XVMC_GET_PRIVATE(pContext->pScreen);

    (*pScreenPriv->adaptors[pContext->adapt_num].DestroySurface) (pSurface);
    free(pSurface);

    XvMCDestroyContextRes((void *) pContext, pContext->context_id);

    return Success;
}

static int
XvMCDestroySubpictureRes(void *data, XID id)
{
    XvMCSubpicturePtr pSubpict = (XvMCSubpicturePtr) data;
    XvMCContextPtr pContext = pSubpict->context;
    XvMCScreenPtr pScreenPriv = XVMC_GET_PRIVATE(pContext->pScreen);

    (*pScreenPriv->adaptors[pContext->adapt_num].DestroySubpicture) (pSubpict);
    free(pSubpict);

    XvMCDestroyContextRes((void *) pContext, pContext->context_id);

    return Success;
}

static int
ProcXvMCQueryVersion(ClientPtr client)
{
    X_REQUEST_HEAD_STRUCT(xvmcQueryVersionReq);

    xvmcQueryVersionReply reply = {
        .major = SERVER_XVMC_MAJOR_VERSION,
        .minor = SERVER_XVMC_MINOR_VERSION
    };

    REPLY_FIELD_CARD32(major);
    REPLY_FIELD_CARD32(minor);

    return X_SEND_REPLY_SIMPLE(client, reply);
}

static int
ProcXvMCListSurfaceTypes(ClientPtr client)
{
    XvPortPtr pPort;
    XvMCScreenPtr pScreenPriv;
    XvMCAdaptorPtr adaptor = NULL;

    X_REQUEST_HEAD_STRUCT(xvmcListSurfaceTypesReq);
    X_REQUEST_FIELD_CARD32(port);

    VALIDATE_XV_PORT(stuff->port, pPort, DixReadAccess);

    if (XvMCInUse) {            /* any adaptors at all */
        ScreenPtr pScreen = pPort->pAdaptor->pScreen;

        if ((pScreenPriv = XVMC_GET_PRIVATE(pScreen))) {        /* any this screen */
            for (int i = 0; i < pScreenPriv->num_adaptors; i++) {
                if (pPort->pAdaptor == pScreenPriv->adaptors[i].xv_adaptor) {
                    adaptor = &(pScreenPriv->adaptors[i]);
                    break;
                }
            }
        }
    }

    x_rpcbuf_t rpcbuf = { .swapped = client->swapped, .err_clear = TRUE };

    int num_surfaces = (adaptor) ? adaptor->num_surfaces : 0;
    for (int i = 0; i < num_surfaces; i++) {
        XvMCSurfaceInfoPtr surface = adaptor->surfaces[i];

        /* write xvmcSurfaceInfo */
        x_rpcbuf_write_CARD32(&rpcbuf, surface->surface_type_id);
        x_rpcbuf_write_CARD16(&rpcbuf, surface->chroma_format);
        x_rpcbuf_write_CARD16(&rpcbuf, 0);
        x_rpcbuf_write_CARD16(&rpcbuf, surface->max_width);
        x_rpcbuf_write_CARD16(&rpcbuf, surface->max_height);
        x_rpcbuf_write_CARD16(&rpcbuf, surface->subpicture_max_width);
        x_rpcbuf_write_CARD16(&rpcbuf, surface->subpicture_max_height);
        x_rpcbuf_write_CARD32(&rpcbuf, surface->mc_type);
        x_rpcbuf_write_CARD32(&rpcbuf, surface->flags);
    }

    xvmcListSurfaceTypesReply reply = {
        .num = num_surfaces,
    };

    if (client->swapped) {
        swapl(&reply.num);
    }

    return X_SEND_REPLY_WITH_RPCBUF(client, reply, rpcbuf);
}

static int
ProcXvMCCreateContext(ClientPtr client)
{
    XvPortPtr pPort;
    CARD32 *data = NULL;
    int dwords = 0;
    int result, adapt_num = -1;
    ScreenPtr pScreen;
    XvMCContextPtr pContext;
    XvMCScreenPtr pScreenPriv;
    XvMCAdaptorPtr adaptor = NULL;
    XvMCSurfaceInfoPtr surface = NULL;

    X_REQUEST_HEAD_STRUCT(xvmcCreateContextReq);
    X_REQUEST_FIELD_CARD32(context_id);
    X_REQUEST_FIELD_CARD16(width);
    X_REQUEST_FIELD_CARD16(height);
    X_REQUEST_FIELD_CARD32(flags);

    VALIDATE_XV_PORT(stuff->port, pPort, DixReadAccess);

    pScreen = pPort->pAdaptor->pScreen;

    if (!XvMCInUse)             /* no XvMC adaptors */
        return BadMatch;

    if (!(pScreenPriv = XVMC_GET_PRIVATE(pScreen)))     /* none this screen */
        return BadMatch;

    for (int i = 0; i < pScreenPriv->num_adaptors; i++) {
        if (pPort->pAdaptor == pScreenPriv->adaptors[i].xv_adaptor) {
            adaptor = &(pScreenPriv->adaptors[i]);
            adapt_num = i;
            break;
        }
    }

    if (adapt_num < 0)          /* none this port */
        return BadMatch;

    for (int i = 0; i < adaptor->num_surfaces; i++) {
        if (adaptor->surfaces[i]->surface_type_id == stuff->surface_type_id) {
            surface = adaptor->surfaces[i];
            break;
        }
    }

    /* adaptor doesn't support this suface_type_id */
    if (!surface)
        return BadMatch;

    if ((stuff->width > surface->max_width) ||
        (stuff->height > surface->max_height))
        return BadValue;

    if (!(pContext = calloc(1, sizeof(XvMCContextRec)))) {
        return BadAlloc;
    }

    pContext->pScreen = pScreen;
    pContext->adapt_num = adapt_num;
    pContext->context_id = stuff->context_id;
    pContext->surface_type_id = stuff->surface_type_id;
    pContext->width = stuff->width;
    pContext->height = stuff->height;
    pContext->flags = stuff->flags;
    pContext->refcnt = 1;

    result = (*adaptor->CreateContext) (pPort, pContext, &dwords, &data);

    if (result != Success) {
        free(pContext);
        return result;
    }
    if (!AddResource(pContext->context_id, XvMCRTContext, pContext)) {
        free(data);
        return BadAlloc;
    }

    x_rpcbuf_t rpcbuf = { .swapped = client->swapped, .err_clear = TRUE };
    x_rpcbuf_write_CARD32s(&rpcbuf, data, dwords);
    free(data);

    xvmcCreateContextReply reply = {
        .width_actual = pContext->width,
        .height_actual = pContext->height,
        .flags_return = pContext->flags
    };

    if (client->swapped) {
        swaps(&reply.width_actual);
        swaps(&reply.height_actual);
        swapl(&reply.flags_return);
    }

    return X_SEND_REPLY_WITH_RPCBUF(client, reply, rpcbuf);
}

static int
ProcXvMCDestroyContext(ClientPtr client)
{
    X_REQUEST_HEAD_STRUCT(xvmcDestroyContextReq);
    X_REQUEST_FIELD_CARD32(context_id);

    void *val;
    int rc;

    rc = dixLookupResourceByType(&val, stuff->context_id, XvMCRTContext,
                                 client, DixDestroyAccess);
    if (rc != Success)
        return rc;

    FreeResource(stuff->context_id, X11_RESTYPE_NONE);

    return Success;
}

static int
ProcXvMCCreateSurface(ClientPtr client)
{
    X_REQUEST_HEAD_STRUCT(xvmcCreateSurfaceReq);
    X_REQUEST_FIELD_CARD32(surface_id);
    X_REQUEST_FIELD_CARD32(context_id);

    CARD32 *data = NULL;
    int dwords = 0;
    int result;
    XvMCContextPtr pContext;
    XvMCSurfacePtr pSurface;
    XvMCScreenPtr pScreenPriv;

    result = dixLookupResourceByType((void **) &pContext, stuff->context_id,
                                     XvMCRTContext, client, DixUseAccess);
    if (result != Success)
        return result;

    pScreenPriv = XVMC_GET_PRIVATE(pContext->pScreen);

    if (!(pSurface = calloc(1, sizeof(XvMCSurfaceRec))))
        return BadAlloc;

    pSurface->surface_id = stuff->surface_id;
    pSurface->surface_type_id = pContext->surface_type_id;
    pSurface->context = pContext;

    result =
        (*pScreenPriv->adaptors[pContext->adapt_num].CreateSurface) (pSurface,
                                                                     &dwords,
                                                                     &data);

    if (result != Success) {
        free(pSurface);
        return result;
    }
    if (!AddResource(pSurface->surface_id, XvMCRTSurface, pSurface)) {
        free(data);
        return BadAlloc;
    }

    x_rpcbuf_t rpcbuf = { .swapped = client->swapped, .err_clear = TRUE };
    x_rpcbuf_write_CARD32s(&rpcbuf, data, dwords);
    free(data);

    xvmcCreateSurfaceReply reply = { 0 };

    pContext->refcnt++;

    return X_SEND_REPLY_WITH_RPCBUF(client, reply, rpcbuf);
}

static int
ProcXvMCDestroySurface(ClientPtr client)
{
    X_REQUEST_HEAD_STRUCT(xvmcDestroySurfaceReq);
    X_REQUEST_FIELD_CARD32(surface_id);

    void *val;
    int rc;

    rc = dixLookupResourceByType(&val, stuff->surface_id, XvMCRTSurface,
                                 client, DixDestroyAccess);
    if (rc != Success)
        return rc;

    FreeResource(stuff->surface_id, X11_RESTYPE_NONE);

    return Success;
}

static int
ProcXvMCCreateSubpicture(ClientPtr client)
{
    X_REQUEST_HEAD_STRUCT(xvmcCreateSubpictureReq);
    X_REQUEST_FIELD_CARD32(subpicture_id);
    X_REQUEST_FIELD_CARD32(context_id);
    X_REQUEST_FIELD_CARD32(xvimage_id);
    X_REQUEST_FIELD_CARD16(width);
    X_REQUEST_FIELD_CARD16(height);

    Bool image_supported = FALSE;
    CARD32 *data = NULL;
    int result, dwords = 0;
    XvMCContextPtr pContext;
    XvMCSubpicturePtr pSubpicture;
    XvMCScreenPtr pScreenPriv;
    XvMCAdaptorPtr adaptor;
    XvMCSurfaceInfoPtr surface = NULL;

    result = dixLookupResourceByType((void **) &pContext, stuff->context_id,
                                     XvMCRTContext, client, DixUseAccess);
    if (result != Success)
        return result;

    pScreenPriv = XVMC_GET_PRIVATE(pContext->pScreen);

    adaptor = &(pScreenPriv->adaptors[pContext->adapt_num]);

    /* find which surface this context supports */
    for (int i = 0; i < adaptor->num_surfaces; i++) {
        if (adaptor->surfaces[i]->surface_type_id == pContext->surface_type_id) {
            surface = adaptor->surfaces[i];
            break;
        }
    }

    if (!surface)
        return BadMatch;

    /* make sure this surface supports that xvimage format */
    if (!surface->compatible_subpictures)
        return BadMatch;

    for (int i = 0; i < surface->compatible_subpictures->num_xvimages; i++) {
        if (surface->compatible_subpictures->xvimage_ids[i] ==
            stuff->xvimage_id) {
            image_supported = TRUE;
            break;
        }
    }

    if (!image_supported)
        return BadMatch;

    /* make sure the size is OK */
    if ((stuff->width > surface->subpicture_max_width) ||
        (stuff->height > surface->subpicture_max_height))
        return BadValue;

    if (!(pSubpicture = calloc(1, sizeof(XvMCSubpictureRec))))
        return BadAlloc;

    pSubpicture->subpicture_id = stuff->subpicture_id;
    pSubpicture->xvimage_id = stuff->xvimage_id;
    pSubpicture->width = stuff->width;
    pSubpicture->height = stuff->height;
    pSubpicture->num_palette_entries = 0;       /* overwritten by DDX */
    pSubpicture->entry_bytes = 0;       /* overwritten by DDX */
    pSubpicture->component_order[0] = 0;        /* overwritten by DDX */
    pSubpicture->component_order[1] = 0;
    pSubpicture->component_order[2] = 0;
    pSubpicture->component_order[3] = 0;
    pSubpicture->context = pContext;

    result =
        (*pScreenPriv->adaptors[pContext->adapt_num].
         CreateSubpicture) (pSubpicture, &dwords, &data);

    if (result != Success) {
        free(pSubpicture);
        return result;
    }
    if (!AddResource(pSubpicture->subpicture_id, XvMCRTSubpicture, pSubpicture)) {
        free(data);
        return BadAlloc;
    }

    x_rpcbuf_t rpcbuf = { .swapped = client->swapped, .err_clear = TRUE };
    x_rpcbuf_write_CARD32s(&rpcbuf, data, dwords);
    free(data);

    xvmcCreateSubpictureReply reply = {
        .width_actual = pSubpicture->width,
        .height_actual = pSubpicture->height,
        .num_palette_entries = pSubpicture->num_palette_entries,
        .entry_bytes = pSubpicture->entry_bytes,
        .component_order[0] = pSubpicture->component_order[0],
        .component_order[1] = pSubpicture->component_order[1],
        .component_order[2] = pSubpicture->component_order[2],
        .component_order[3] = pSubpicture->component_order[3]
    };

    if (client->swapped) {
        swaps(&reply.width_actual);
        swaps(&reply.height_actual);
        swaps(&reply.num_palette_entries);
        swaps(&reply.entry_bytes);
    }

    pContext->refcnt++;

    return X_SEND_REPLY_WITH_RPCBUF(client, reply, rpcbuf);
}

static int
ProcXvMCDestroySubpicture(ClientPtr client)
{
    X_REQUEST_HEAD_STRUCT(xvmcDestroySubpictureReq);
    X_REQUEST_FIELD_CARD32(subpicture_id);

    void *val;
    int rc;

    rc = dixLookupResourceByType(&val, stuff->subpicture_id, XvMCRTSubpicture,
                                 client, DixDestroyAccess);
    if (rc != Success)
        return rc;

    FreeResource(stuff->subpicture_id, X11_RESTYPE_NONE);

    return Success;
}

static int
ProcXvMCListSubpictureTypes(ClientPtr client)
{
    X_REQUEST_HEAD_STRUCT(xvmcListSubpictureTypesReq);
    X_REQUEST_FIELD_CARD32(port);
    X_REQUEST_FIELD_CARD32(surface_type_id);

    XvPortPtr pPort;
    XvMCScreenPtr pScreenPriv;
    ScreenPtr pScreen;
    XvMCAdaptorPtr adaptor = NULL;
    XvMCSurfaceInfoPtr surface = NULL;
    XvImagePtr pImage;

    VALIDATE_XV_PORT(stuff->port, pPort, DixReadAccess);

    pScreen = pPort->pAdaptor->pScreen;

    if (!dixPrivateKeyRegistered(&XvMCScreenKeyRec))
        return BadMatch;        /* No XvMC adaptors */

    if (!(pScreenPriv = XVMC_GET_PRIVATE(pScreen)))
        return BadMatch;        /* None this screen */

    for (int i = 0; i < pScreenPriv->num_adaptors; i++) {
        if (pPort->pAdaptor == pScreenPriv->adaptors[i].xv_adaptor) {
            adaptor = &(pScreenPriv->adaptors[i]);
            break;
        }
    }

    if (!adaptor)
        return BadMatch;

    for (int i = 0; i < adaptor->num_surfaces; i++) {
        if (adaptor->surfaces[i]->surface_type_id == stuff->surface_type_id) {
            surface = adaptor->surfaces[i];
            break;
        }
    }

    if (!surface)
        return BadMatch;

    int num = (surface->compatible_subpictures ?
               surface->compatible_subpictures->num_xvimages : 0);

    x_rpcbuf_t rpcbuf = { .swapped = client->swapped, .err_clear = TRUE };

    if (num) {
        for (int i = 0; i < num; i++) {
            pImage = NULL;
            for (int j = 0; j < adaptor->num_subpictures; j++) {
                if (surface->compatible_subpictures->xvimage_ids[i] ==
                    adaptor->subpictures[j]->id) {
                    pImage = adaptor->subpictures[j];
                    break;
                }
            }
            if (!pImage) {
                return BadImplementation;
            }

            /* xvImageFormatInfo */
            x_rpcbuf_write_CARD32(&rpcbuf, pImage->id);
            x_rpcbuf_write_CARD8(&rpcbuf, pImage->type);
            x_rpcbuf_write_CARD8(&rpcbuf, pImage->byte_order);
            x_rpcbuf_write_CARD16(&rpcbuf, 0); /* pad1 */
            x_rpcbuf_write_CARD8s(&rpcbuf, (CARD8*)pImage->guid, 16);
            x_rpcbuf_write_CARD8(&rpcbuf, pImage->bits_per_pixel);
            x_rpcbuf_write_CARD8(&rpcbuf, pImage->num_planes);
            x_rpcbuf_write_CARD16(&rpcbuf, 0); /* pad2 */
            x_rpcbuf_write_CARD8(&rpcbuf, pImage->depth);
            x_rpcbuf_write_CARD8(&rpcbuf, 0); /* pad3 */
            x_rpcbuf_write_CARD16(&rpcbuf, 0); /* pad4 */
            x_rpcbuf_write_CARD32(&rpcbuf, pImage->red_mask);
            x_rpcbuf_write_CARD32(&rpcbuf, pImage->green_mask);
            x_rpcbuf_write_CARD32(&rpcbuf, pImage->blue_mask);
            x_rpcbuf_write_CARD8(&rpcbuf, pImage->format);
            x_rpcbuf_write_CARD8(&rpcbuf, 0); /* pad5 */
            x_rpcbuf_write_CARD16(&rpcbuf, 0); /* pad6 */
            x_rpcbuf_write_CARD32(&rpcbuf, pImage->y_sample_bits);
            x_rpcbuf_write_CARD32(&rpcbuf, pImage->u_sample_bits);
            x_rpcbuf_write_CARD32(&rpcbuf, pImage->v_sample_bits);
            x_rpcbuf_write_CARD32(&rpcbuf, pImage->horz_y_period);
            x_rpcbuf_write_CARD32(&rpcbuf, pImage->horz_u_period);
            x_rpcbuf_write_CARD32(&rpcbuf, pImage->horz_v_period);
            x_rpcbuf_write_CARD32(&rpcbuf, pImage->vert_y_period);
            x_rpcbuf_write_CARD32(&rpcbuf, pImage->vert_u_period);
            x_rpcbuf_write_CARD32(&rpcbuf, pImage->vert_v_period);
            x_rpcbuf_write_CARD8s(&rpcbuf, (CARD8*)pImage->component_order, 32);
            x_rpcbuf_write_CARD8(&rpcbuf,  pImage->scanline_order);
            x_rpcbuf_write_CARD8(&rpcbuf, 0); /* pad7 */
            x_rpcbuf_write_CARD16(&rpcbuf, 0); /* pad8 */
            x_rpcbuf_write_CARD32(&rpcbuf, 0); /* pad9 */
            x_rpcbuf_write_CARD32(&rpcbuf, 0); /* pad10 */
        }
    }

    xvmcListSubpictureTypesReply reply = {
        .num = num,
    };

    if (client->swapped) {
        swapl(&reply.num);
    }

    return X_SEND_REPLY_WITH_RPCBUF(client, reply, rpcbuf);
}

static int
ProcXvMCGetDRInfo(ClientPtr client)
{
    X_REQUEST_HEAD_STRUCT(xvmcGetDRInfoReq);
    X_REQUEST_FIELD_CARD32(port);
    X_REQUEST_FIELD_CARD32(shmKey);
    X_REQUEST_FIELD_CARD32(magic);

    XvPortPtr pPort;
    ScreenPtr pScreen;
    XvMCScreenPtr pScreenPriv;

    VALIDATE_XV_PORT(stuff->port, pPort, DixReadAccess);

    pScreen = pPort->pAdaptor->pScreen;
    pScreenPriv = XVMC_GET_PRIVATE(pScreen);

    int nameLen = strlen(pScreenPriv->clientDriverName) + 1;
    int busIDLen = strlen(pScreenPriv->busID) + 1;

    x_rpcbuf_t rpcbuf = { .swapped = client->swapped, .err_clear = TRUE };
    x_rpcbuf_write_CARD8s(&rpcbuf, (CARD8*)pScreenPriv->clientDriverName, nameLen);
    x_rpcbuf_write_CARD8s(&rpcbuf, (CARD8*)pScreenPriv->busID, busIDLen);

    xvmcGetDRInfoReply reply = {
        .major = pScreenPriv->major,
        .minor = pScreenPriv->minor,
        .patchLevel = pScreenPriv->patchLevel,
        .nameLen = nameLen,
        .busIDLen = busIDLen,
        .isLocal = 1
    };

    /*
     * Read back to the client what she has put in the shared memory
     * segment she prepared for us.
     */
    if (client->swapped) {
        swapl(&reply.major);
        swapl(&reply.minor);
        swapl(&reply.patchLevel);
        swapl(&reply.nameLen);
        swapl(&reply.busIDLen);
        swapl(&reply.isLocal);
    }

    return X_SEND_REPLY_WITH_RPCBUF(client, reply, rpcbuf);
}

static int
ProcXvMCDispatch(ClientPtr client)
{
    if (!(client->local))
        return BadImplementation;

    REQUEST(xReq);
    switch (stuff->data)
    {
        case xvmc_QueryVersion:
            return ProcXvMCQueryVersion(client);
        case xvmc_ListSurfaceTypes:
            return ProcXvMCListSurfaceTypes(client);
        case xvmc_CreateContext:
            return ProcXvMCCreateContext(client);
        case xvmc_DestroyContext:
            return ProcXvMCDestroyContext(client);
        case xvmc_CreateSurface:
            return ProcXvMCCreateSurface(client);
        case xvmc_DestroySurface:
            return ProcXvMCDestroySurface(client);
        case xvmc_CreateSubpicture:
            return ProcXvMCCreateSubpicture(client);
        case xvmc_DestroySubpicture:
            return ProcXvMCDestroySubpicture(client);
        case xvmc_ListSubpictureTypes:
            return ProcXvMCListSubpictureTypes(client);
        case xvmc_GetDRInfo:
            return ProcXvMCGetDRInfo(client);
        default:
            return BadRequest;
    }
}

void
XvMCExtensionInit(void)
{
    ExtensionEntry *extEntry;

    if (!dixPrivateKeyRegistered(&XvMCScreenKeyRec))
        return;

    if (!(XvMCRTContext = CreateNewResourceType(XvMCDestroyContextRes,
                                                "XvMCRTContext")))
        return;

    if (!(XvMCRTSurface = CreateNewResourceType(XvMCDestroySurfaceRes,
                                                "XvMCRTSurface")))
        return;

    if (!(XvMCRTSubpicture = CreateNewResourceType(XvMCDestroySubpictureRes,
                                                   "XvMCRTSubpicture")))
        return;

    extEntry = AddExtension(XvMCName, XvMCNumEvents, XvMCNumErrors,
                            ProcXvMCDispatch, ProcXvMCDispatch,
                            NULL, StandardMinorOpcode);

    if (!extEntry)
        return;

    XvMCReqCode = extEntry->base;
    XvMCEventBase = extEntry->eventBase;
    SetResourceTypeErrorValue(XvMCRTContext,
                              extEntry->errorBase + XvMCBadContext);
    SetResourceTypeErrorValue(XvMCRTSurface,
                              extEntry->errorBase + XvMCBadSurface);
    SetResourceTypeErrorValue(XvMCRTSubpicture,
                              extEntry->errorBase + XvMCBadSubpicture);
}

static void XvMCScreenClose(CallbackListPtr *pcbl, ScreenPtr pScreen, void *unused)
{
    XvMCScreenPtr pScreenPriv = XVMC_GET_PRIVATE(pScreen);
    free(pScreenPriv);
    dixSetPrivate(&pScreen->devPrivates, &XvMCScreenKeyRec, NULL);
    dixScreenUnhookClose(pScreen, XvMCScreenClose);
}

int
XvMCScreenInit(ScreenPtr pScreen, int num, XvMCAdaptorPtr pAdapt)
{
    XvMCScreenPtr pScreenPriv;

    if (!dixRegisterPrivateKey(&XvMCScreenKeyRec, PRIVATE_SCREEN, 0))
        return BadAlloc;

    if (!(pScreenPriv = calloc(1, sizeof(XvMCScreenRec))))
        return BadAlloc;

    dixSetPrivate(&pScreen->devPrivates, &XvMCScreenKeyRec, pScreenPriv);

    dixScreenHookClose(pScreen, XvMCScreenClose);

    pScreenPriv->num_adaptors = num;
    pScreenPriv->adaptors = pAdapt;
    pScreenPriv->clientDriverName[0] = 0;
    pScreenPriv->busID[0] = 0;
    pScreenPriv->major = 0;
    pScreenPriv->minor = 0;
    pScreenPriv->patchLevel = 0;

    XvMCInUse = TRUE;

    return Success;
}

XvImagePtr
XvMCFindXvImage(XvPortPtr pPort, CARD32 id)
{
    XvImagePtr pImage = NULL;
    ScreenPtr pScreen = pPort->pAdaptor->pScreen;
    XvMCScreenPtr pScreenPriv;
    XvMCAdaptorPtr adaptor = NULL;

    if (!dixPrivateKeyRegistered(&XvMCScreenKeyRec))
        return NULL;

    if (!(pScreenPriv = XVMC_GET_PRIVATE(pScreen)))
        return NULL;

    for (int i = 0; i < pScreenPriv->num_adaptors; i++) {
        if (pPort->pAdaptor == pScreenPriv->adaptors[i].xv_adaptor) {
            adaptor = &(pScreenPriv->adaptors[i]);
            break;
        }
    }

    if (!adaptor)
        return NULL;

    for (int i = 0; i < adaptor->num_subpictures; i++) {
        if (adaptor->subpictures[i]->id == id) {
            pImage = adaptor->subpictures[i];
            break;
        }
    }

    return pImage;
}

int
xf86XvMCRegisterDRInfo(ScreenPtr pScreen, const char *name,
                       const char *busID, int major, int minor, int patchLevel)
{
    XvMCScreenPtr pScreenPriv = XVMC_GET_PRIVATE(pScreen);

    strlcpy(pScreenPriv->clientDriverName, name, DR_CLIENT_DRIVER_NAME_SIZE);
    strlcpy(pScreenPriv->busID, busID, DR_BUSID_SIZE);
    pScreenPriv->major = major;
    pScreenPriv->minor = minor;
    pScreenPriv->patchLevel = patchLevel;
    return Success;
}

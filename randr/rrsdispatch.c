/*
 * Copyright © 2006 Keith Packard
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that copyright
 * notice and this permission notice appear in supporting documentation, and
 * that the name of the copyright holders not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  The copyright holders make no representations
 * about the suitability of this software for any purpose.  It is provided "as
 * is" without express or implied warranty.
 *
 * THE COPYRIGHT HOLDERS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
 * OF THIS SOFTWARE.
 */

#include "randrstr_priv.h"

static int _X_COLD
SProcRRQueryVersion(ClientPtr client)
{
    REQUEST(xRRQueryVersionReq);

    REQUEST_SIZE_MATCH(xRRQueryVersionReq);
    swapl(&stuff->majorVersion);
    swapl(&stuff->minorVersion);
    return ProcRRQueryVersion(client);
}

static int _X_COLD
SProcRRGetScreenInfo(ClientPtr client)
{
    REQUEST(xRRGetScreenInfoReq);

    REQUEST_SIZE_MATCH(xRRGetScreenInfoReq);
    swapl(&stuff->window);
    return ProcRRGetScreenInfo(client);
}

static int _X_COLD
SProcRRSetScreenConfig(ClientPtr client)
{
    REQUEST(xRRSetScreenConfigReq);

    if (RRClientKnowsRates(client)) {
        REQUEST_SIZE_MATCH(xRRSetScreenConfigReq);
        swaps(&stuff->rate);
    }
    else {
        REQUEST_SIZE_MATCH(xRR1_0SetScreenConfigReq);
    }

    swapl(&stuff->drawable);
    swapl(&stuff->timestamp);
    swaps(&stuff->sizeID);
    swaps(&stuff->rotation);
    return ProcRRSetScreenConfig(client);
}

static int _X_COLD
SProcRRSelectInput(ClientPtr client)
{
    REQUEST(xRRSelectInputReq);

    REQUEST_SIZE_MATCH(xRRSelectInputReq);
    swapl(&stuff->window);
    swaps(&stuff->enable);
    return ProcRRSelectInput(client);
}

static int _X_COLD
SProcRRGetScreenSizeRange(ClientPtr client)
{
    REQUEST(xRRGetScreenSizeRangeReq);

    REQUEST_SIZE_MATCH(xRRGetScreenSizeRangeReq);
    swapl(&stuff->window);
    return ProcRRGetScreenSizeRange(client);
}

static int _X_COLD
SProcRRSetScreenSize(ClientPtr client)
{
    REQUEST(xRRSetScreenSizeReq);

    REQUEST_SIZE_MATCH(xRRSetScreenSizeReq);
    swapl(&stuff->window);
    swaps(&stuff->width);
    swaps(&stuff->height);
    swapl(&stuff->widthInMillimeters);
    swapl(&stuff->heightInMillimeters);
    return ProcRRSetScreenSize(client);
}

static int _X_COLD
SProcRRGetScreenResources(ClientPtr client)
{
    REQUEST(xRRGetScreenResourcesReq);

    REQUEST_SIZE_MATCH(xRRGetScreenResourcesReq);
    swapl(&stuff->window);
    return ProcRRGetScreenResources(client);
}

static int _X_COLD
SProcRRGetScreenResourcesCurrent(ClientPtr client)
{
    REQUEST(xRRGetScreenResourcesCurrentReq);

    REQUEST_SIZE_MATCH(xRRGetScreenResourcesCurrentReq);
    swaps(&stuff->length);
    swapl(&stuff->window);
    return ProcRRGetScreenResourcesCurrent(client);
}

static int _X_COLD
SProcRRGetOutputInfo(ClientPtr client)
{
    REQUEST(xRRGetOutputInfoReq);

    REQUEST_SIZE_MATCH(xRRGetOutputInfoReq);
    swapl(&stuff->output);
    swapl(&stuff->configTimestamp);
    return ProcRRGetScreenResources(client);
}

static int _X_COLD
SProcRRListOutputProperties(ClientPtr client)
{
    REQUEST(xRRListOutputPropertiesReq);

    REQUEST_SIZE_MATCH(xRRListOutputPropertiesReq);
    swapl(&stuff->output);
    return ProcRRListOutputProperties(client);
}

static int _X_COLD
SProcRRQueryOutputProperty(ClientPtr client)
{
    REQUEST(xRRQueryOutputPropertyReq);

    REQUEST_SIZE_MATCH(xRRQueryOutputPropertyReq);
    swapl(&stuff->output);
    swapl(&stuff->property);
    return ProcRRQueryOutputProperty(client);
}

static int _X_COLD
SProcRRConfigureOutputProperty(ClientPtr client)
{
    REQUEST(xRRConfigureOutputPropertyReq);

    REQUEST_AT_LEAST_SIZE(xRRConfigureOutputPropertyReq);
    swapl(&stuff->output);
    swapl(&stuff->property);
    SwapRestL(stuff);
    return ProcRRConfigureOutputProperty(client);
}

static int _X_COLD
SProcRRChangeOutputProperty(ClientPtr client)
{
    REQUEST(xRRChangeOutputPropertyReq);

    REQUEST_AT_LEAST_SIZE(xRRChangeOutputPropertyReq);
    swapl(&stuff->output);
    swapl(&stuff->property);
    swapl(&stuff->type);
    swapl(&stuff->nUnits);
    switch (stuff->format) {
    case 8:
        break;
    case 16:
        SwapRestS(stuff);
        break;
    case 32:
        SwapRestL(stuff);
        break;
    default:
        client->errorValue = stuff->format;
        return BadValue;
    }
    return ProcRRChangeOutputProperty(client);
}

static int _X_COLD
SProcRRDeleteOutputProperty(ClientPtr client)
{
    REQUEST(xRRDeleteOutputPropertyReq);

    REQUEST_SIZE_MATCH(xRRDeleteOutputPropertyReq);
    swapl(&stuff->output);
    swapl(&stuff->property);
    return ProcRRDeleteOutputProperty(client);
}

static int _X_COLD
SProcRRGetOutputProperty(ClientPtr client)
{
    REQUEST(xRRGetOutputPropertyReq);

    REQUEST_SIZE_MATCH(xRRGetOutputPropertyReq);
    swapl(&stuff->output);
    swapl(&stuff->property);
    swapl(&stuff->type);
    swapl(&stuff->longOffset);
    swapl(&stuff->longLength);
    return ProcRRGetOutputProperty(client);
}

static int _X_COLD
SProcRRCreateMode(ClientPtr client)
{
    xRRModeInfo *modeinfo;

    REQUEST(xRRCreateModeReq);

    REQUEST_AT_LEAST_SIZE(xRRCreateModeReq);
    swapl(&stuff->window);

    modeinfo = &stuff->modeInfo;
    swapl(&modeinfo->id);
    swaps(&modeinfo->width);
    swaps(&modeinfo->height);
    swapl(&modeinfo->dotClock);
    swaps(&modeinfo->hSyncStart);
    swaps(&modeinfo->hSyncEnd);
    swaps(&modeinfo->hTotal);
    swaps(&modeinfo->vSyncStart);
    swaps(&modeinfo->vSyncEnd);
    swaps(&modeinfo->vTotal);
    swaps(&modeinfo->nameLength);
    swapl(&modeinfo->modeFlags);
    return ProcRRCreateMode(client);
}

static int _X_COLD
SProcRRDestroyMode(ClientPtr client)
{
    REQUEST(xRRDestroyModeReq);

    REQUEST_SIZE_MATCH(xRRDestroyModeReq);
    swapl(&stuff->mode);
    return ProcRRDestroyMode(client);
}

static int _X_COLD
SProcRRAddOutputMode(ClientPtr client)
{
    REQUEST(xRRAddOutputModeReq);

    REQUEST_SIZE_MATCH(xRRAddOutputModeReq);
    swapl(&stuff->output);
    swapl(&stuff->mode);
    return ProcRRAddOutputMode(client);
}

static int _X_COLD
SProcRRDeleteOutputMode(ClientPtr client)
{
    REQUEST(xRRDeleteOutputModeReq);

    REQUEST_SIZE_MATCH(xRRDeleteOutputModeReq);
    swapl(&stuff->output);
    swapl(&stuff->mode);
    return ProcRRDeleteOutputMode(client);
}

static int _X_COLD
SProcRRGetCrtcInfo(ClientPtr client)
{
    REQUEST(xRRGetCrtcInfoReq);

    REQUEST_SIZE_MATCH(xRRGetCrtcInfoReq);
    swapl(&stuff->crtc);
    swapl(&stuff->configTimestamp);
    return ProcRRGetCrtcInfo(client);
}

static int _X_COLD
SProcRRSetCrtcConfig(ClientPtr client)
{
    REQUEST(xRRSetCrtcConfigReq);

    REQUEST_AT_LEAST_SIZE(xRRSetCrtcConfigReq);
    swapl(&stuff->crtc);
    swapl(&stuff->timestamp);
    swapl(&stuff->configTimestamp);
    swaps(&stuff->x);
    swaps(&stuff->y);
    swapl(&stuff->mode);
    swaps(&stuff->rotation);
    SwapRestL(stuff);
    return ProcRRSetCrtcConfig(client);
}

static int _X_COLD
SProcRRGetCrtcGammaSize(ClientPtr client)
{
    REQUEST(xRRGetCrtcGammaSizeReq);

    REQUEST_SIZE_MATCH(xRRGetCrtcGammaSizeReq);
    swapl(&stuff->crtc);
    return ProcRRGetCrtcGammaSize(client);
}

static int _X_COLD
SProcRRGetCrtcGamma(ClientPtr client)
{
    REQUEST(xRRGetCrtcGammaReq);

    REQUEST_SIZE_MATCH(xRRGetCrtcGammaReq);
    swapl(&stuff->crtc);
    return ProcRRGetCrtcGamma(client);
}

static int _X_COLD
SProcRRSetCrtcGamma(ClientPtr client)
{
    REQUEST(xRRSetCrtcGammaReq);

    REQUEST_AT_LEAST_SIZE(xRRSetCrtcGammaReq);
    swapl(&stuff->crtc);
    swaps(&stuff->size);
    SwapRestS(stuff);
    return ProcRRSetCrtcGamma(client);
}

static int _X_COLD
SProcRRSetCrtcTransform(ClientPtr client)
{
    int nparams;
    char *filter;
    CARD32 *params;

    REQUEST(xRRSetCrtcTransformReq);

    REQUEST_AT_LEAST_SIZE(xRRSetCrtcTransformReq);
    swapl(&stuff->crtc);
    SwapLongs((CARD32 *) &stuff->transform,
              bytes_to_int32(sizeof(xRenderTransform)));
    swaps(&stuff->nbytesFilter);
    filter = (char *) (stuff + 1);
    params = (CARD32 *) (filter + pad_to_int32(stuff->nbytesFilter));
    nparams = ((CARD32 *) stuff + client->req_len) - params;
    if (nparams < 0)
        return BadLength;

    SwapLongs(params, nparams);
    return ProcRRSetCrtcTransform(client);
}

static int _X_COLD
SProcRRGetCrtcTransform(ClientPtr client)
{
    REQUEST(xRRGetCrtcTransformReq);

    REQUEST_SIZE_MATCH(xRRGetCrtcTransformReq);
    swapl(&stuff->crtc);
    return ProcRRGetCrtcTransform(client);
}

static int _X_COLD
SProcRRGetPanning(ClientPtr client)
{
    REQUEST(xRRGetPanningReq);

    REQUEST_SIZE_MATCH(xRRGetPanningReq);
    swapl(&stuff->crtc);
    return ProcRRGetPanning(client);
}

static int _X_COLD
SProcRRSetPanning(ClientPtr client)
{
    REQUEST(xRRSetPanningReq);

    REQUEST_SIZE_MATCH(xRRSetPanningReq);
    swapl(&stuff->crtc);
    swapl(&stuff->timestamp);
    swaps(&stuff->left);
    swaps(&stuff->top);
    swaps(&stuff->width);
    swaps(&stuff->height);
    swaps(&stuff->track_left);
    swaps(&stuff->track_top);
    swaps(&stuff->track_width);
    swaps(&stuff->track_height);
    swaps(&stuff->border_left);
    swaps(&stuff->border_top);
    swaps(&stuff->border_right);
    swaps(&stuff->border_bottom);
    return ProcRRSetPanning(client);
}

static int _X_COLD
SProcRRSetOutputPrimary(ClientPtr client)
{
    REQUEST(xRRSetOutputPrimaryReq);

    REQUEST_SIZE_MATCH(xRRSetOutputPrimaryReq);
    swapl(&stuff->window);
    swapl(&stuff->output);
    return ProcRRSetOutputPrimary(client);
}

static int _X_COLD
SProcRRGetOutputPrimary(ClientPtr client)
{
    REQUEST(xRRGetOutputPrimaryReq);

    REQUEST_SIZE_MATCH(xRRGetOutputPrimaryReq);
    swapl(&stuff->window);
    return ProcRRGetOutputPrimary(client);
}

static int _X_COLD
SProcRRGetProviders(ClientPtr client)
{
    REQUEST(xRRGetProvidersReq);

    REQUEST_SIZE_MATCH(xRRGetProvidersReq);
    swapl(&stuff->window);
    return ProcRRGetProviders(client);
}

static int _X_COLD
SProcRRGetProviderInfo(ClientPtr client)
{
    REQUEST(xRRGetProviderInfoReq);

    REQUEST_SIZE_MATCH(xRRGetProviderInfoReq);
    swapl(&stuff->provider);
    swapl(&stuff->configTimestamp);
    return ProcRRGetProviderInfo(client);
}

static int _X_COLD
SProcRRSetProviderOffloadSink(ClientPtr client)
{
    REQUEST(xRRSetProviderOffloadSinkReq);

    REQUEST_SIZE_MATCH(xRRSetProviderOffloadSinkReq);
    swapl(&stuff->provider);
    swapl(&stuff->sink_provider);
    swapl(&stuff->configTimestamp);
    return ProcRRSetProviderOffloadSink(client);
}

static int _X_COLD
SProcRRSetProviderOutputSource(ClientPtr client)
{
    REQUEST(xRRSetProviderOutputSourceReq);

    REQUEST_SIZE_MATCH(xRRSetProviderOutputSourceReq);
    swapl(&stuff->provider);
    swapl(&stuff->source_provider);
    swapl(&stuff->configTimestamp);
    return ProcRRSetProviderOutputSource(client);
}

static int _X_COLD
SProcRRListProviderProperties(ClientPtr client)
{
    REQUEST(xRRListProviderPropertiesReq);

    REQUEST_SIZE_MATCH(xRRListProviderPropertiesReq);
    swapl(&stuff->provider);
    return ProcRRListProviderProperties(client);
}

static int _X_COLD
SProcRRQueryProviderProperty(ClientPtr client)
{
    REQUEST(xRRQueryProviderPropertyReq);

    REQUEST_SIZE_MATCH(xRRQueryProviderPropertyReq);
    swapl(&stuff->provider);
    swapl(&stuff->property);
    return ProcRRQueryProviderProperty(client);
}

static int _X_COLD
SProcRRConfigureProviderProperty(ClientPtr client)
{
    REQUEST(xRRConfigureProviderPropertyReq);

    REQUEST_AT_LEAST_SIZE(xRRConfigureProviderPropertyReq);
    swapl(&stuff->provider);
    swapl(&stuff->property);
    /* TODO: no way to specify format? */
    SwapRestL(stuff);
    return ProcRRConfigureProviderProperty(client);
}

static int _X_COLD
SProcRRChangeProviderProperty(ClientPtr client)
{
    REQUEST(xRRChangeProviderPropertyReq);

    REQUEST_AT_LEAST_SIZE(xRRChangeProviderPropertyReq);
    swapl(&stuff->provider);
    swapl(&stuff->property);
    swapl(&stuff->type);
    swapl(&stuff->nUnits);
    switch (stuff->format) {
    case 8:
        break;
    case 16:
        SwapRestS(stuff);
        break;
    case 32:
        SwapRestL(stuff);
        break;
    }
    return ProcRRChangeProviderProperty(client);
}

static int _X_COLD
SProcRRDeleteProviderProperty(ClientPtr client)
{
    REQUEST(xRRDeleteProviderPropertyReq);

    REQUEST_SIZE_MATCH(xRRDeleteProviderPropertyReq);
    swapl(&stuff->provider);
    swapl(&stuff->property);
    return ProcRRDeleteProviderProperty(client);
}

static int _X_COLD
SProcRRGetProviderProperty(ClientPtr client)
{
    REQUEST(xRRGetProviderPropertyReq);

    REQUEST_SIZE_MATCH(xRRGetProviderPropertyReq);
    swapl(&stuff->provider);
    swapl(&stuff->property);
    swapl(&stuff->type);
    swapl(&stuff->longOffset);
    swapl(&stuff->longLength);
    return ProcRRGetProviderProperty(client);
}

static int _X_COLD
SProcRRGetMonitors(ClientPtr client) {
    REQUEST(xRRGetMonitorsReq);

    REQUEST_SIZE_MATCH(xRRGetMonitorsReq);
    swapl(&stuff->window);
    return ProcRRGetMonitors(client);
}

static int _X_COLD
SProcRRSetMonitor(ClientPtr client) {
    REQUEST(xRRSetMonitorReq);

    REQUEST_AT_LEAST_SIZE(xRRGetMonitorsReq);
    swapl(&stuff->window);
    swapl(&stuff->monitor.name);
    swaps(&stuff->monitor.noutput);
    swaps(&stuff->monitor.x);
    swaps(&stuff->monitor.y);
    swaps(&stuff->monitor.width);
    swaps(&stuff->monitor.height);
    SwapRestL(stuff);
    return ProcRRSetMonitor(client);
}

static int _X_COLD
SProcRRDeleteMonitor(ClientPtr client) {
    REQUEST(xRRDeleteMonitorReq);

    REQUEST_SIZE_MATCH(xRRDeleteMonitorReq);
    swapl(&stuff->window);
    swapl(&stuff->name);
    return ProcRRDeleteMonitor(client);
}

static int _X_COLD
SProcRRCreateLease(ClientPtr client) {
    REQUEST(xRRCreateLeaseReq);

    REQUEST_AT_LEAST_SIZE(xRRCreateLeaseReq);
    swapl(&stuff->window);
    swaps(&stuff->nCrtcs);
    swaps(&stuff->nOutputs);
    SwapRestL(stuff);
    return ProcRRCreateLease(client);
}

static int _X_COLD
SProcRRFreeLease(ClientPtr client) {
    REQUEST(xRRFreeLeaseReq);

    REQUEST_SIZE_MATCH(xRRFreeLeaseReq);
    swapl(&stuff->lid);
    return ProcRRFreeLease(client);
}

int (*SProcRandrVector[RRNumberRequests]) (ClientPtr) = {
    SProcRRQueryVersion,        /* 0 */
/* we skip 1 to make old clients fail pretty immediately */
        NULL,                   /* 1 SProcRandrOldGetScreenInfo */
/* V1.0 apps share the same set screen config request id */
        SProcRRSetScreenConfig, /* 2 */
        NULL,                   /* 3 SProcRandrOldScreenChangeSelectInput */
/* 3 used to be ScreenChangeSelectInput; deprecated */
        SProcRRSelectInput,     /* 4 */
        SProcRRGetScreenInfo,   /* 5 */
/* V1.2 additions */
        SProcRRGetScreenSizeRange,      /* 6 */
        SProcRRSetScreenSize,   /* 7 */
        SProcRRGetScreenResources,      /* 8 */
        SProcRRGetOutputInfo,   /* 9 */
        SProcRRListOutputProperties,    /* 10 */
        SProcRRQueryOutputProperty,     /* 11 */
        SProcRRConfigureOutputProperty, /* 12 */
        SProcRRChangeOutputProperty,    /* 13 */
        SProcRRDeleteOutputProperty,    /* 14 */
        SProcRRGetOutputProperty,       /* 15 */
        SProcRRCreateMode,      /* 16 */
        SProcRRDestroyMode,     /* 17 */
        SProcRRAddOutputMode,   /* 18 */
        SProcRRDeleteOutputMode,        /* 19 */
        SProcRRGetCrtcInfo,     /* 20 */
        SProcRRSetCrtcConfig,   /* 21 */
        SProcRRGetCrtcGammaSize,        /* 22 */
        SProcRRGetCrtcGamma,    /* 23 */
        SProcRRSetCrtcGamma,    /* 24 */
/* V1.3 additions */
        SProcRRGetScreenResourcesCurrent,      /* 25 */
        SProcRRSetCrtcTransform,        /* 26 */
        SProcRRGetCrtcTransform,        /* 27 */
        SProcRRGetPanning,      /* 28 */
        SProcRRSetPanning,      /* 29 */
        SProcRRSetOutputPrimary,        /* 30 */
        SProcRRGetOutputPrimary,        /* 31 */
/* V1.4 additions */
        SProcRRGetProviders,            /* 32 */
        SProcRRGetProviderInfo,         /* 33 */
        SProcRRSetProviderOffloadSink,  /* 34 */
        SProcRRSetProviderOutputSource, /* 35 */
        SProcRRListProviderProperties,  /* 36 */
        SProcRRQueryProviderProperty,   /* 37 */
        SProcRRConfigureProviderProperty, /* 38 */
        SProcRRChangeProviderProperty,  /* 39 */
        SProcRRDeleteProviderProperty,  /* 40 */
        SProcRRGetProviderProperty,     /* 41 */
/* V1.5 additions */
        SProcRRGetMonitors,            /* 42 */
        SProcRRSetMonitor,             /* 43 */
        SProcRRDeleteMonitor,          /* 44 */
/* V1.6 additions */
        SProcRRCreateLease,            /* 45 */
        SProcRRFreeLease,              /* 46 */
};

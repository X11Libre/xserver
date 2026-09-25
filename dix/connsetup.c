/* SPDX-License-Identifier: X11 OR MIT OR AGPL-3.0-or-later
 *
 * Copyright © 2026 Enrico Weigelt, metux IT consult <info@metux.net>
 */
#include <dix-config.h>

#include <stdbool.h>
#include <stddef.h>
#include <version-config.h>

#include <X11/Xproto.h>

#include "include/misc.h"
#include "include/windowstr.h"
#include "dix/client_priv.h"
#include "dix/rpcbuf_priv.h"
#include "dix/screenint_priv.h"
#include "dix/server_priv.h"
#include "dix/dix_priv.h"
#include "Xext/panoramiX/panoramiXsrv.h"
#include "Xext/panoramiX/panoramiX_priv.h"

size_t ConnectionInfoSize = 0;

void dixSendConnAbort(ClientPtr pClient, const char *reason)
{
    xConnSetupPrefix csp = {
        .success = xFalse,
        .lengthReason = strlen(reason),
        .length = bytes_to_int32(csp.lengthReason),
        .majorVersion = X_PROTOCOL,
        .minorVersion = X_PROTOCOL_REVISION,
    };

    if (pClient->swapped) {
        swaps(&csp.majorVersion);
        swaps(&csp.minorVersion);
        swaps(&csp.length);
    }

    dixWriteToClient(pClient, sizeof(csp), &csp);
    dixWriteToClient(pClient, (int) csp.lengthReason, reason);
    pClient->noClientException = -1;
}

void dixInitConnectionBlock(void)
{
    free(ConnectionInfo);
    ConnectionInfo = NULL;
    ConnectionInfoSize = 0;

#ifdef XINERAMA
    if (PanoramiXIsEnabled()) {
        if (!PanoramiXCreateConnectionBlock()) {
            FatalError("could not create panoramix connection block info");
        }
    }
    else
#endif /* XINERAMA */
    {
        if (!CreateConnectionBlock(0)) {
            FatalError("could not create connection block info");
        }
    }
}

void x_rpcbuf_write_xWindowRoot(x_rpcbuf_t *rpcbuf, ScreenPtr pScreen)
{
    xWindowRoot root = {
        .windowId = pScreen->root->drawable.id,
        .defaultColormap = pScreen->defColormap,
        .whitePixel = pScreen->whitePixel,
        .blackPixel = pScreen->blackPixel,
        .currentInputMask = 0,      /* filled in when sent */
        .pixWidth = pScreen->width,
        .pixHeight = pScreen->height,
        .mmWidth = pScreen->mmWidth,
        .mmHeight = pScreen->mmHeight,
        .minInstalledMaps = pScreen->minInstalledCmaps,
        .maxInstalledMaps = pScreen->maxInstalledCmaps,
        .rootVisualID = pScreen->rootVisual,
        .backingStore = pScreen->backingStoreSupport,
        .saveUnders = FALSE,
        .rootDepth = pScreen->rootDepth,
        .nDepths = pScreen->numDepths,
    };
    x_rpcbuf_write_CARD8s(rpcbuf, (CARD8*)&root, sizeof(root));
};

void x_rpcbuf_write_xDepth(x_rpcbuf_t *rpcbuf, DepthPtr pDepth)
{
    /* write the xDepth header */
    xDepth depth = {
        .depth = pDepth->depth,
        .nVisuals = pDepth->numVids,
    };
    x_rpcbuf_write_CARD8s(rpcbuf, (CARD8*)&depth, sizeof(depth));
}

void x_rpcbuf_write_xVisualInfo(x_rpcbuf_t *rpcbuf, VisualPtr pVisual)
{
    xVisualType visual = {
        .visualID = pVisual->vid,
        .class = pVisual->class,
        .bitsPerRGB = pVisual->bitsPerRGBValue,
        .colormapEntries = pVisual->ColormapEntries,
        .redMask = pVisual->redMask,
        .greenMask = pVisual->greenMask,
        .blueMask = pVisual->blueMask,
    };
    x_rpcbuf_write_CARD8s(rpcbuf, (CARD8*)&visual, sizeof(visual));
}

void x_rpcbuf_write_xPixmapFormat(x_rpcbuf_t *rpcbuf, PixmapFormatPtr pPixmapFormat)
{
    xPixmapFormat format = {
        .depth = pPixmapFormat->depth,
        .bitsPerPixel = pPixmapFormat->bitsPerPixel,
        .scanLinePad = pPixmapFormat->scanlinePad,
    };
    x_rpcbuf_write_CARD8s(rpcbuf, (CARD8*)&format, sizeof(format));
}

x_rpcbuf_t dixBuildConnectionBlock(int maxscreens, size_t *screenDataOffset)
{
    const char VendorString[] = "XLibre";

    if (!maxscreens) {
        maxscreens = screenInfo.numScreens;
    }

    x_rpcbuf_t rpcbuf = { 0 };

    xConnSetup setup = {
        /* Leave off the ridBase and ridMask, these must be sent with
           connection */
        .release = VENDOR_RELEASE,
        /*
         * per-server image and bitmap parameters are defined in Xmd.h
         */
        .imageByteOrder = screenInfo.imageByteOrder,

        .bitmapScanlineUnit = screenInfo.bitmapScanlineUnit,
        .bitmapScanlinePad = screenInfo.bitmapScanlinePad,

        .bitmapBitOrder = screenInfo.bitmapBitOrder,
        .motionBufferSize = NumMotionEvents(),
        .numRoots = maxscreens,
        .nbytesVendor = strlen(VendorString),
        .numFormats = screenInfo.numPixmapFormats,
        .maxRequestSize = MAX_REQUEST_SIZE,
    };
    QueryMinMaxKeyCodes(&setup.minKeyCode, &setup.maxKeyCode);
    x_rpcbuf_write_CARD8s(&rpcbuf, (CARD8*)&setup, sizeof(setup));

    x_rpcbuf_write_string_pad(&rpcbuf, VendorString);

    for (int i = 0; i < screenInfo.numPixmapFormats; i++) {
        x_rpcbuf_write_xPixmapFormat(&rpcbuf, &(screenInfo.formats[i]));
    }

    /* record this for other parts which later going to manipulate the data */
    *screenDataOffset = rpcbuf.wpos;

    DIX_FOR_N_SCREENS(0, maxscreens, {
        x_rpcbuf_write_xWindowRoot(&rpcbuf, walkScreen);

        DepthPtr pDepth = walkScreen->allowedDepths;
        for (int j = 0; j < walkScreen->numDepths; j++, pDepth++) {
            x_rpcbuf_write_xDepth(&rpcbuf, pDepth);
            for (int k = 0; k < pDepth->numVids; k++) {
                unsigned long vid = pDepth->vids[k];
                VisualPtr pVisual;
                for (pVisual = walkScreen->visuals;
                     pVisual->vid != vid; pVisual++);
                x_rpcbuf_write_xVisualInfo(&rpcbuf, pVisual);
            }
        }
    });

    return rpcbuf;
}

bool CreateConnectionBlock(int maxscreens)
{
    if (!maxscreens) {
        maxscreens = screenInfo.numScreens;
    }

    size_t screenDataOffset = 0;
    x_rpcbuf_t rpcbuf = dixBuildConnectionBlock(maxscreens, &screenDataOffset);

    /* record this for other parts which later going to manipulate the data */
    connBlockScreenStart = screenDataOffset;

    if (rpcbuf.error)
        return false;

    /* must not free the rpcbuf here, because we store the data elsewhere */
    ConnectionInfo = rpcbuf.buffer;
    ConnectionInfoSize = rpcbuf.wpos;
    return true;
}

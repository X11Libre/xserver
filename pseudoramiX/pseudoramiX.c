/*
 * Minimal implementation of PanoramiX/Xinerama
 *
 * This is used in rootless mode where the underlying window server
 * already provides an abstracted view of multiple screens as one
 * large screen area.
 *
 * This code is largely based on panoramiX.c, which contains the
 * following copyright notice:
 */
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

#include <dix-config.h>

#include <X11/Xfuncproto.h>

#include "dix/dix_priv.h"
#include "dix/request_priv.h"
#include "miext/extinit_priv.h"

#include "pseudoramiX.h"
#include "extnsionst.h"
#include "dixstruct.h"
#include "window.h"
#include <X11/extensions/panoramiXproto.h>
#include "globals.h"

#define TRACE LogMessageVerb(X_NONE, 10, "TRACE " __FILE__ ":%s", __func__)
#define DEBUG_LOG(...) LogMessageVerb(X_NONE, 3, __VA_ARGS__);

Bool noPseudoramiXExtension = FALSE;
extern Bool noRRXineramaExtension;

extern int
ProcPanoramiXQueryVersion(ClientPtr client);

static void
PseudoramiXResetProc(ExtensionEntry *extEntry);

static int
ProcPseudoramiXGetState(ClientPtr client);
static int
ProcPseudoramiXGetScreenCount(ClientPtr client);
static int
ProcPseudoramiXGetScreenSize(ClientPtr client);
static int
ProcPseudoramiXIsActive(ClientPtr client);
static int
ProcPseudoramiXQueryScreens(ClientPtr client);
static int
ProcPseudoramiXDispatch(ClientPtr client);

typedef struct {
    int x;
    int y;
    int w;
    int h;
} PseudoramiXScreenRec;

static PseudoramiXScreenRec *pseudoramiXScreens = NULL;
static int pseudoramiXScreensAllocated = 0;
static int pseudoramiXNumScreens = 0;
static x_server_generation_t pseudoramiXGeneration = 0;

// Add a PseudoramiX screen.
// The rest of the X server will know nothing about this screen.
// Can be called before or after extension init.
// Screens must be re-added once per generation.
void
PseudoramiXAddScreen(int x, int y, int w, int h)
{
    PseudoramiXScreenRec *s;

    if (noPseudoramiXExtension) return;

    if (pseudoramiXNumScreens == pseudoramiXScreensAllocated) {
        pseudoramiXScreensAllocated += pseudoramiXScreensAllocated + 1;
        pseudoramiXScreens = reallocarray(pseudoramiXScreens,
                                          pseudoramiXScreensAllocated,
                                          sizeof(PseudoramiXScreenRec));
    }

    DEBUG_LOG("x: %d, y: %d, w: %d, h: %d\n", x, y, w, h);

    s = &pseudoramiXScreens[pseudoramiXNumScreens++];
    s->x = x;
    s->y = y;
    s->w = w;
    s->h = h;
}

// Initialize PseudoramiX.
// Copied from PanoramiXExtensionInit
void
PseudoramiXExtensionInit(void)
{
    Bool success = FALSE;
    ExtensionEntry      *extEntry;

    if (noPseudoramiXExtension) return;

    TRACE;

    /* Even with only one screen we need to enable PseudoramiX to allow
       dynamic screen configuration changes. */
#if 0
    if (pseudoramiXNumScreens == 1) {
        // Only one screen - disable Xinerama extension.
        noPseudoramiXExtension = TRUE;
        return;
    }
#endif

    if (pseudoramiXGeneration != serverGeneration) {
        extEntry = AddExtension(PANORAMIX_PROTOCOL_NAME, 0, 0,
                                ProcPseudoramiXDispatch,
                                ProcPseudoramiXDispatch,
                                PseudoramiXResetProc,
                                StandardMinorOpcode);
        if (!extEntry) {
            ErrorF("PseudoramiXExtensionInit(): AddExtension failed\n");
        }
        else {
            pseudoramiXGeneration = serverGeneration;
            success = TRUE;
        }
    }

    /* Do not allow RRXinerama to initialize if we did */
    noRRXineramaExtension = success;

    if (!success) {
        ErrorF("%s Extension (PseudoramiX) failed to initialize\n",
               PANORAMIX_PROTOCOL_NAME);
        return;
    }
}

void
PseudoramiXResetScreens(void)
{
    TRACE;

    pseudoramiXNumScreens = 0;
}

static void
PseudoramiXResetProc(ExtensionEntry *extEntry)
{
    TRACE;

    PseudoramiXResetScreens();
}

// was PanoramiX
static int
ProcPseudoramiXGetState(ClientPtr client)
{
    REQUEST(xPanoramiXGetStateReq);
    REQUEST_SIZE_MATCH(xPanoramiXGetStateReq);

    if (client->swapped)
        swapl(&stuff->window);

    WindowPtr pWin;
    register int rc;

    TRACE;

    rc = dixLookupWindow(&pWin, stuff->window, client, DixGetAttrAccess);
    if (rc != Success)
        return rc;

    xPanoramiXGetStateReply reply = {
        .state = !noPseudoramiXExtension,
        .window = stuff->window
    };

    if (client->swapped) {
        swapl(&reply.window);
    }

    return X_SEND_REPLY_SIMPLE(client, reply);
}

// was PanoramiX
static int
ProcPseudoramiXGetScreenCount(ClientPtr client)
{
    REQUEST(xPanoramiXGetScreenCountReq);
    REQUEST_SIZE_MATCH(xPanoramiXGetScreenCountReq);

    if (client->swapped)
        swapl(&stuff->window);

    WindowPtr pWin;
    register int rc;

    TRACE;

    rc = dixLookupWindow(&pWin, stuff->window, client, DixGetAttrAccess);
    if (rc != Success)
        return rc;

    xPanoramiXGetScreenCountReply reply = {
        .ScreenCount = pseudoramiXNumScreens,
        .window = stuff->window
    };

    if (client->swapped) {
        swapl(&reply.window);
    }

    return X_SEND_REPLY_SIMPLE(client, reply);
}

// was PanoramiX
static int
ProcPseudoramiXGetScreenSize(ClientPtr client)
{
    REQUEST(xPanoramiXGetScreenSizeReq);
    REQUEST_SIZE_MATCH(xPanoramiXGetScreenSizeReq);

    if (client->swapped) {
        swapl(&stuff->window);
        swapl(&stuff->screen);
    }

    WindowPtr pWin;
    register int rc;

    TRACE;

    if (stuff->screen >= pseudoramiXNumScreens)
      return BadMatch;

    rc = dixLookupWindow(&pWin, stuff->window, client, DixGetAttrAccess);
    if (rc != Success)
        return rc;

    xPanoramiXGetScreenSizeReply reply = {
        .width = pseudoramiXScreens[stuff->screen].w,
        .height = pseudoramiXScreens[stuff->screen].h,
        .window = stuff->window,
        .screen = stuff->screen
    };

    if (client->swapped) {
        swapl(&reply.width);
        swapl(&reply.height);
        swapl(&reply.window);
        swapl(&reply.screen);
    }

    return X_SEND_REPLY_SIMPLE(client, reply);
}

// was Xinerama
static int
ProcPseudoramiXIsActive(ClientPtr client)
{
    /* REQUEST(xXineramaIsActiveReq); */
    TRACE;
    REQUEST_SIZE_MATCH(xXineramaIsActiveReq);

    xXineramaIsActiveReply reply = {
        .state = !noPseudoramiXExtension
    };

    if (client->swapped) {
        swapl(&reply.state);
    }

    return X_SEND_REPLY_SIMPLE(client, reply);
}

// was Xinerama
static int
ProcPseudoramiXQueryScreens(ClientPtr client)
{
    /* REQUEST(xXineramaQueryScreensReq); */

    DEBUG_LOG("noPseudoramiXExtension=%d, pseudoramiXNumScreens=%d\n",
              noPseudoramiXExtension,
              pseudoramiXNumScreens);

    REQUEST_SIZE_MATCH(xXineramaQueryScreensReq);

    x_rpcbuf_t rpcbuf = { .swapped = client->swapped, .err_clear = TRUE };

    if (!noPseudoramiXExtension) {
        for (int i = 0; i < pseudoramiXNumScreens; i++) {
            /* xXineramaScreenInfo is the same as xRectangle */
            x_rpcbuf_write_rect(&rpcbuf,
                                pseudoramiXScreens[i].x,
                                pseudoramiXScreens[i].y,
                                pseudoramiXScreens[i].w,
                                pseudoramiXScreens[i].h);
        }
    }

    xXineramaQueryScreensReply reply = {
        .number = noPseudoramiXExtension ? 0 : pseudoramiXNumScreens
    };

    if (client->swapped)
        swapl(&reply.number);

    return X_SEND_REPLY_WITH_RPCBUF(client, reply, rpcbuf);
}

// was PanoramiX
static int
ProcPseudoramiXDispatch(ClientPtr client)
{
    REQUEST(xReq);
    TRACE;
    switch (stuff->data) {
    case X_PanoramiXQueryVersion:
        return ProcPanoramiXQueryVersion(client);

    case X_PanoramiXGetState:
        return ProcPseudoramiXGetState(client);

    case X_PanoramiXGetScreenCount:
        return ProcPseudoramiXGetScreenCount(client);

    case X_PanoramiXGetScreenSize:
        return ProcPseudoramiXGetScreenSize(client);

    case X_XineramaIsActive:
        return ProcPseudoramiXIsActive(client);

    case X_XineramaQueryScreens:
        return ProcPseudoramiXQueryScreens(client);
    }
    return BadRequest;
}

#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include "dix-config.h"
#include "nexus.h"
#include "dix.h"
#include "screenint.h"
#include "window.h"
#include "colormap.h"
#include "cursor.h"
#include "pixmap.h"
#include "gc.h"
#include "dix/screenint_priv.h"
#include "dix/screen_hooks_priv.h"
#include "miext/extinit_priv.h"
#include "resource.h"
#include "globals.h"
#include "dixstruct.h"
#include "scrnintstr.h"
#include "servermd.h"
#include "mi.h"
#include "dix_priv.h"

/* Nexus extension can be disabled via -extension NEXUS */
Bool noNexusExtension = FALSE;

static DevPrivateKeyRec nexus_screen_private_key;
static ScreenPtr nexus_screen = NULL;
static bool nexus_privates_registered = false;

static int
nexus_dummy_screen_init(ScreenPtr pScreen, int argc, char **argv)
{
    (void)argc;
    (void)argv;

    /* Set up a basic 24-bit TrueColor visual for the dummy screen */
    VisualPtr visuals = calloc(1, sizeof(VisualRec));
    DepthPtr depths = calloc(1, sizeof(DepthRec));
    int numVisuals = 0;
    int numDepths = 0;

    if (!visuals || !depths) {
        free(visuals);
        free(depths);
        return FALSE;
    }

    /* 24-bit TrueColor visual */
    visuals[numVisuals] = (VisualRec) {
        .class = TrueColor,
        .bitsPerRGBValue = 8,
        .ColormapEntries = 256,
        .nplanes = 24,
        .redMask = 0xFF0000,
        .greenMask = 0x00FF00,
        .blueMask = 0x0000FF,
        .offsetRed = 16,
        .offsetGreen = 8,
        .offsetBlue = 0,
        .vid = FakeClientID(0),
    };
    numVisuals++;

    depths[numDepths] = (DepthRec) {
        .depth = 24,
        .numVids = 1,
        .vids = calloc(1, sizeof(VisualID)),
    };
    depths[numDepths].vids[0] = visuals[0].vid;
    numDepths++;

    pScreen->mmWidth = 1024;
    pScreen->mmHeight = 768;
    pScreen->width = 1024;
    pScreen->height = 768;
    pScreen->rootDepth = 24;
    pScreen->rootVisual = visuals[0].vid;
    pScreen->minInstalledCmaps = 1;
    pScreen->maxInstalledCmaps = 1;
    pScreen->backingStoreSupport = NotUseful;
    pScreen->saveUnderSupport = false;
    pScreen->whitePixel = 0xFFFFFF;
    pScreen->blackPixel = 0x000000;
    pScreen->numDepths = numDepths;
    pScreen->allowedDepths = depths;
    pScreen->visuals = visuals;

    if (!miScreenInit(pScreen, NULL, pScreen->width, pScreen->height,
                      96, 96, pScreen->width,
                      pScreen->rootDepth, numDepths, depths, pScreen->rootVisual,
                      numVisuals, visuals))
        return FALSE;

    return TRUE;
}

static bool
nexus_dummy_validate_physical(ScreenPtr nexus, ScreenPtr physical)
{
    (void)nexus;
    (void)physical;
    return true;
}

static bool
nexus_register_privates(void)
{
    if (nexus_privates_registered)
        return true;

    if (!dixRegisterPrivateKey(&nexus_screen_private_key, PRIVATE_SCREEN, 0))
        return false;

    nexus_privates_registered = true;
    return true;
}

static ScreenPtr nexus_create_screen(void)
{
    if (!nexus_register_privates())
        return NULL;

    int idx = AddScreen(nexus_dummy_screen_init, 0, NULL);
    if (idx < 0)
        return NULL;

    ScreenPtr pScreen = screenInfo.screens[idx];
    dixSetPrivate(&pScreen->devPrivates, &nexus_screen_private_key, NULL);
    nexus_screen = pScreen;

    return pScreen;
}

bool NexusPreInit(void)
{
    if (noNexusExtension)
        return TRUE;

    if (!nexus_register_privates())
        return FALSE;

    if (!nexus_create_screen())
        return FALSE;

    return TRUE;
}

bool NexusPostInit(void)
{
    if (!nexus_screen)
        return TRUE;

    // FIXME: we're going to have initialize lots of things here,
    // now that the actual screens are present.
    return nexus_validate_all_physical();
}

bool nexus_validate_all_physical(void)
{
    if (!nexus_screen)
        return TRUE;

    for (int i = 1; i < screenInfo.numScreens; i++) {
        ScreenPtr physical = screenInfo.screens[i];
        if (physical && !nexus_dummy_validate_physical(nexus_screen, physical))
            return FALSE;
    }
    return TRUE;
}

/* Extension init function for runtime enable/disable via +extension/-extension */
void
NexusExtensionInit(void)
{
    if (noNexusExtension)
        return;

    /* Check that we have at least one physical screen registered */
    if (screenInfo.numScreens < 2) {
        FatalError("NEXUS extension requires at least one physical screen");
    }
}

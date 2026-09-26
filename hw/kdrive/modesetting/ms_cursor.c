/* SPDX-License-Identifier: MIT OR X11
 *
 * Copyright © 2026 stefan11111 <stefan11111@shitposting.expert>
 */

#include <kdrive-config.h>

#include "modesetting.h"

#if 0
extern miPointerSpriteFuncRec miSpritePointerFuncs;

static miPointerSpriteFuncRec msPointerSpriteFuncs;
#endif

Bool
msCursorInit(ScreenPtr pScreen)
{
#if 0
    msPointerSpriteFuncs = miSpritePointerFuncs;
    msPointerSpriteFuncs.DeviceCursorInitialize = NULL;
    msPointerSpriteFuncs.DeviceCursorCleanup = NULL;
#endif

#if 0
    pScreen->QueryBestSize = msQueryBestSize;
#endif

#if 0
    if (!miPointerInitialize(pScreen, &msPointerSpriteFuncs,
         &kdPointerScreenFuncs, FALSE /* waitForUpdate */)) {
        return FALSE;
    }
#endif

    miDCInitialize(pScreen, &kdPointerScreenFuncs);

    return TRUE;
}

void
msCursorEnable(ScreenPtr pScreen)
{
}

void
msCursorDisable(ScreenPtr pScreen)
{
}

void
msRecolorCursor(ScreenPtr pScreen, int ndef, xColorItem *pdef)
{
}

void
msCursorFini(ScreenPtr pScreen)
{
}

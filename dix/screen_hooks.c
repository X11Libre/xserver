/* SPDX-License-Identifier: MIT OR X11
 *
 * Copyright © 2024 Enrico Weigelt, metux IT consult <info@metux.net>
 */

#include <dix-config.h>

#include "dix/dix_priv.h"
#include "include/dix.h"
#include "include/os.h"
#include "include/scrnintstr.h"
#include "include/windowstr.h"

#define ARRAY_LENGTH(x) (sizeof(x) / sizeof((x)[0]))

#define ARRAY_FOR_EACH(_ARRAY, _WALK) \
    for (struct { int idx; typeof(_ARRAY[0])*ptr; } _WALK = { 0, _ARRAY }; _WALK.idx < ARRAY_LENGTH(_ARRAY); _WALK.idx++, _WALK.ptr++)

#define DECLARE_HOOK_LIST(NAME, FIELD) \
    void dixScreenHook##NAME(ScreenPtr pScreen, typeof(((ScreenRec){0}).FIELD[0].func) func, void *arg) \
    { \
        for (int i=0; i<ARRAY_LENGTH(pScreen->FIELD); i++) { \
            if (!(pScreen->FIELD[i].func)) { \
                pScreen->FIELD[i].func = func; \
                pScreen->FIELD[i].arg = arg; \
                return; \
            } \
        } \
        FatalError("%s: out of slots", __FUNCTION__); \
    } \
    \
    void dixScreenUnhook##NAME(ScreenPtr pScreen, typeof(((ScreenRec){0}).FIELD[0].func) func, void *arg) \
    { \
        for (int i=0; i<ARRAY_LENGTH(pScreen->FIELD); i++) { \
            if ((pScreen->FIELD[i].func == func) && (pScreen->FIELD[i].arg == arg)) { \
                pScreen->FIELD[i].func = NULL; \
                pScreen->FIELD[i].arg = NULL; \
                return; \
            } \
        } \
    }

DECLARE_HOOK_LIST(WindowDestroy, _notify_window_destroy)
DECLARE_HOOK_LIST(WindowPosition, _notify_window_position)

int dixScreenRaiseWindowDestroy(WindowPtr pWin)
{
    if (!pWin)
        return Success;

    ScreenPtr pScreen = pWin->drawable.pScreen;

    ARRAY_FOR_EACH(pScreen->_notify_window_destroy, walk) {
        if (walk.ptr->func)
            walk.ptr->func(pScreen, pWin, walk.ptr->arg);
    }
    return (pScreen->DestroyWindow ? (*pScreen->DestroyWindow) (pWin) : Success);
}

void dixScreenRaiseWindowPosition(WindowPtr pWin, uint32_t x, uint32_t y)
{
    if (!pWin)
        return;

    ScreenPtr pScreen = pWin->drawable.pScreen;

    ARRAY_FOR_EACH(pScreen->_notify_window_position, walk) {
        if (walk.ptr->func)
            walk.ptr->func(pScreen, pWin, walk.ptr->arg, x, y);
    }

    if (pScreen->PositionWindow)
        (*pScreen->PositionWindow) (pWin, x, y);
}

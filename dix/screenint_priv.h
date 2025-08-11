/* SPDX-License-Identifier: MIT OR X11
 *
 * Copyright © 2024 Enrico Weigelt, metux IT consult <info@metux.net>
 * Copyright © 1987, 1998 The Open Group
 */
#ifndef _XSERVER_DIX_SCREENINT_PRIV_H
#define _XSERVER_DIX_SCREENINT_PRIV_H

#include <X11/Xdefs.h>

#include "include/screenint.h"
#include "include/scrnintstr.h" /* for screenInfo */

typedef Bool (*ScreenInitProcPtr)(ScreenPtr pScreen, int argc, char **argv);

int AddScreen(ScreenInitProcPtr pfnInit, int argc, char **argv);
int AddGPUScreen(ScreenInitProcPtr pfnInit, int argc, char **argv);

void RemoveGPUScreen(ScreenPtr pScreen);

void AttachUnboundGPU(ScreenPtr pScreen, ScreenPtr newScreen);
void DetachUnboundGPU(ScreenPtr unbound);

void AttachOffloadGPU(ScreenPtr pScreen, ScreenPtr newScreen);
void DetachOffloadGPU(ScreenPtr slave);

/*
 * macro for looping over all screens (up to `screenInfo.numScreens`).
 * Makes a new scopes and declares `walkScreenIdx` as the current screen's
 * index number as well as `walkScreen` as poiner to current ScreenRec
 *
 * @param __LAMBDA__ the code to be executed in each iteration step.
 */
#define DIX_FOR_EACH_SCREEN(__LAMBDA__) \
    do { \
        for (unsigned walkScreenIdx = 0; walkScreenIdx < screenInfo.numScreens; walkScreenIdx++) { \
            ScreenPtr walkScreen = screenInfo.screens[walkScreenIdx]; \
            (void)walkScreen; \
            __LAMBDA__; \
        } \
    } while (0);

/*
 * macro for looping over all screens (up to `screenInfo.numScreens`),
 * but if XINERAMA enabled only hit the first screen.
 *
 * @param __LAMBDA__ the code to be executed in each iteration step.
 */
#ifdef XINERAMA
#define DIX_FOR_EACH_SCREEN_XINERAMA(__LAMBDA__) \
    do { \
        unsigned int __num_screens = screenInfo.numScreens; \
        if (!noPanoramiXExtension) \
            __num_screens = 1; \
        for (unsigned walkScreenIdx = 0; walkScreenIdx < __num_screens; walkScreenIdx++) { \
            ScreenPtr walkScreen = screenInfo.screens[walkScreenIdx]; \
            (void)walkScreen; \
            __LAMBDA__; \
        } \
    } while (0);
#else
#define DIX_FOR_EACH_SCREEN_XINERAMA DIX_FOR_EACH_SCREEN
#endif

/*
 * macro for looping over all GPU screens (up to `screenInfo.numScreens`).
 * Makes a new scopes and declares `walkScreenIdx` as the current screen's
 * index number as well as `walkScreen` as poiner to current ScreenRec
 *
 * @param __LAMBDA__ the code to be executed in each iteration step.
 */
#define DIX_FOR_EACH_GPU_SCREEN(__LAMBDA__) \
    do { \
        for (unsigned walkScreenIdx = 0; walkScreenIdx < screenInfo.numGPUScreens; walkScreenIdx++) { \
            ScreenPtr walkScreen = screenInfo.gpuscreens[walkScreenIdx]; \
            (void)walkScreen; \
            __LAMBDA__; \
        } \
    } while (0);

/*
 * retrieve pointer to screen by it's index. If index is above the total
 * number of screens, returns NULL
 *
 * @param idx screen index
 * @return pointer to idx'th screen or NULL
 */
static inline ScreenPtr dixGetScreenPtr(unsigned int idx) {
    if (idx < screenInfo.numScreens)
        return screenInfo.screens[idx];
    return NULL;
}

static inline ScreenPtr dixGetFirstScreenPtr(void) {
    return dixGetScreenPtr(0);
}

#endif /* _XSERVER_DIX_SCREENINT_PRIV_H */

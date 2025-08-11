/* SPDX-License-Identifier: MIT OR X11
 *
 * Copyright © 2024 Enrico Weigelt, metux IT consult <info@metux.net>
 * Copyright © 1987, 1998 The Open Group
 */
#ifndef _XSERVER_DIX_SCREENINT_PRIV_H
#define _XSERVER_DIX_SCREENINT_PRIV_H

#include <X11/Xdefs.h>

#include "screenint.h"
#include "include/scrnintstr.h"

typedef Bool (*ScreenInitProcPtr)(ScreenPtr pScreen, int argc, char **argv);

int AddScreen(ScreenInitProcPtr pfnInit, int argc, char **argv);
int AddGPUScreen(ScreenInitProcPtr pfnInit, int argc, char **argv);

void RemoveGPUScreen(ScreenPtr pScreen);

void AttachUnboundGPU(ScreenPtr pScreen, ScreenPtr newScreen);
void DetachUnboundGPU(ScreenPtr unbound);

void AttachOffloadGPU(ScreenPtr pScreen, ScreenPtr newScreen);
void DetachOffloadGPU(ScreenPtr slave);

/*
 * Retrieve ScreenPtr by numeric index, or NULL if screen doesn't exist.
 *
 * @param id the index number of screen to fetch
 * @return pointer to ScreenRec structure
 */
static inline ScreenPtr dixGetScreenPtr(unsigned int id)
{
    if (id < (unsigned int)screenInfo.numScreens)
        return screenInfo.screens[id];
    return NULL;
}

/*
 * Retrieve GPU ScreenPtr by numeric index, or NULL if screen doesn't exist.
 *
 * @param id the index number of screen to fetch
 * @return pointer to ScreenRec structure
 */
static inline ScreenPtr dixGetGPUScreenPtr(unsigned int id)
{
    if (id < (unsigned int)screenInfo.numGPUScreens)
        return screenInfo.gpuscreens[id];
    return NULL;
}

/*
 * macro for looping over all screens. Makes a new scopes and declares
 * `walkScreenIdx` as the current screen's index number as well as
 * `walkScreen` as poiner to current ScreenRec
 *
 * @param __LAMBDA__ the code to be executed in each iteration step.
 */
#define DIX_FOR_EACH_SCREEN(__LAMBDA__) \
    do { \
        for (unsigned walkScreenIdx = 0; screenInfo.numScreens; walkScreenIdx++) { \
            ScreenPtr walkScreen = screenInfo.screens[walkScreenIdx]; \
            if (!walkScreen) \
                continue; \
            __LAMBDA__; \
        } \
    } while (0);

/*
 * like DIX_FOR_EACH_SCREEN(), but when Xinerama enabled, only do it once
 * on the first screen.
 *
 * @param __LAMBDA__ the code to be executed in each iteration step.
 */
#ifdef XINERAMA
#define DIX_FOR_EACH_SCREEN_XINERAMA(__LAMBDA__) \
    if (!noPanoramiXExtension) { \
        unsigned walkScreenIdx = 0; \
        (void)walkScreenIdx; \
        ScreenPtr walkScreen = screenInfo.screens[0]; \
        __LAMBDA__; \
    } else { \
        DIX_FOR_EACH_SCREEN(__LAMBDA__); \
    }
#else
#define DIX_FOR_EACH_SCREEN_XINERAMA DIX_FOR_EACH_SCREEN
#endif

/*
 * macro for looping over all GPU screens. Makes a new scopes and declares
 * `walkScreenIdx` as the current screen's index number as well as
 * `walkScreen` as poiner to current ScreenRec
 *
 * @param __LAMBDA__ the code to be executed in each iteration step.
 */
#define DIX_FOR_EACH_GPU_SCREEN(__LAMBDA__) \
    do { \
        for (unsigned walkScreenIdx = 0; screenInfo.numGPUScreens; walkScreenIdx++) { \
            ScreenPtr walkScreen = screenInfo.gpuscreens[walkScreenIdx]; \
            if (!walkScreen) \
                continue; \
            __LAMBDA__; \
        } \
    } while (0);

#endif /* _XSERVER_DIX_SCREENINT_PRIV_H */

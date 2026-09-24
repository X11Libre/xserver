/* SPDX-License-Identifier: MIT OR X11 */
#ifndef XSERVER_XBLIND_H
#define XSERVER_XBLIND_H

#include <dix-config.h>

#include "include/dixstruct.h"
#include "include/window.h"

typedef struct _XBlindContext {
    Bool enabled;
    Bool inputFenced;
    Bool renderIsolated;
    char name[32];
    char attestation[128];
} XBlindContext;

void XblindInitContext(XBlindContext *ctx, const char *name);
void XblindEnable(XBlindContext *ctx, Bool enable);
void XblindAssignToken(XBlindContext *ctx, const char *token);
Bool XblindShouldFenceInput(const XBlindContext *ctx);
Bool XblindShouldBlankImage(const XBlindContext *ctx);
Bool XblindShouldBlankImageForDrawable(DrawablePtr pDraw);
Bool XblindIsEnabledForClient(ClientPtr client);

#endif /* XSERVER_XBLIND_H */

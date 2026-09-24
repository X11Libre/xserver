/* SPDX-License-Identifier: MIT OR X11 */

#include <dix-config.h>

#include <stdio.h>
#include <string.h>

#include "dix/dix_priv.h"
#include "Xext/namespace/namespace.h"
#include "Xext/namespace/xblind.h"

void
XblindInitContext(XBlindContext *ctx, const char *name)
{
    if (!ctx)
        return;

    memset(ctx, 0, sizeof(*ctx));
    if (name && *name) {
        snprintf(ctx->name, sizeof(ctx->name), "%.*s",
                 (int) (sizeof(ctx->name) - 1), name);
    }
    if (!ctx->name[0])
        snprintf(ctx->name, sizeof(ctx->name), "xblind");
}

void
XblindEnable(XBlindContext *ctx, Bool enable)
{
    if (!ctx)
        return;

    ctx->enabled = enable ? TRUE : FALSE;
    if (enable) {
        ctx->inputFenced = TRUE;
        ctx->renderIsolated = TRUE;
    }
    else {
        ctx->inputFenced = FALSE;
        ctx->renderIsolated = FALSE;
    }
}

void
XblindAssignToken(XBlindContext *ctx, const char *token)
{
    if (!ctx)
        return;

    if (!token || !*token)
        token = "alpha";

    snprintf(ctx->attestation, sizeof(ctx->attestation), "%s:%s",
             ctx->name[0] ? ctx->name : "xblind", token);

    if (ctx->enabled) {
        ctx->inputFenced = TRUE;
        ctx->renderIsolated = TRUE;
    }
}

Bool
XblindShouldFenceInput(const XBlindContext *ctx)
{
    return ctx && ctx->enabled && ctx->inputFenced;
}

Bool
XblindShouldBlankImage(const XBlindContext *ctx)
{
    return ctx && ctx->enabled && ctx->renderIsolated;
}

Bool
XblindShouldBlankImageForDrawable(DrawablePtr pDraw)
{
    if (!pDraw || pDraw->type != DRAWABLE_WINDOW)
        return FALSE;

    ClientPtr client = dixClientForWindow((WindowPtr) pDraw);
    if (!client)
        return FALSE;

    struct XnamespaceClientPriv *priv = XnsClientPriv(client);
    if (!priv)
        return FALSE;

    if (XblindShouldBlankImage(&priv->blind))
        return TRUE;

    return priv->ns && XblindShouldBlankImage(&priv->ns->blind);
}

Bool
XblindIsEnabledForClient(ClientPtr client)
{
    if (!client)
        return FALSE;

    struct XnamespaceClientPriv *priv = XnsClientPriv(client);
    if (!priv)
        return FALSE;

    if (XblindShouldFenceInput(&priv->blind))
        return TRUE;

    return priv->ns && XblindShouldFenceInput(&priv->ns->blind);
}

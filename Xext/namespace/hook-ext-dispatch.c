/* SPDX-License-Identifier: X11 OR MIT OR AGPL-3.0-or-later */
/* Copyright (C) 2026 Enrico Weigelt, metux IT consult <info@metux.net> */
#define HOOK_NAME "ext-dispatch"

#include <dix-config.h>

#include <stdio.h>
#include <X11/Xdefs.h> // syncproto.h is broken
#include <X11/Xmd.h>
#include <X11/extensions/syncproto.h>
#include <X11/extensions/shapeproto.h>
#include <X11/extensions/XIproto.h>
#include <X11/extensions/XI2proto.h>
#include <X11/extensions/XKB.h>
#include <X11/extensions/xfixeswire.h>

#include "dix/dix_priv.h"
#include "dix/extension_priv.h"
#include "dix/registry_priv.h"
#include "Xext/xacestr.h"

#include "namespace.h"
#include "namespaceproto.h"
#include "hooks.h"

void hookExtDispatch(CallbackListPtr *pcbl, void *unused, void *calldata)
{
    XNS_HOOK_HEAD(ExtensionAccessCallbackParam);

    /* only the builtin root namespace may dispatch the X-NAMESPACE management
       extension; granting ADMIN (superPower) to a sandbox disables its X11
       isolation without letting it manage namespaces */
    if (streq(param->ext->name, XNS_EXTENSION_NAME)) {
        if (subj->ns == &ns_root)
            goto pass;
        goto reject;
    }

    /* root NS or ADMIN bypass has super powers */
    if (subj->ns->superPower)
        goto pass;

    switch (client->majorOp) {
        /* Blacklisted extensions that allow global input spoofing or snooping */
        case EXTENSION_MAJOR_COMPOSITE:
        case EXTENSION_MAJOR_MIT_SCREEN_SAVER:
        case EXTENSION_MAJOR_RECORD:
        case EXTENSION_MAJOR_SECURITY:
        case EXTENSION_MAJOR_XTEST:
            goto reject;

        /* Allow non-destructive shape queries always; shape modification requires allowShape */
        case EXTENSION_MAJOR_SHAPE:
            if (subj->ns->allowShape ||
                client->minorOp == X_ShapeQueryVersion ||
                client->minorOp == X_ShapeQueryExtents ||
                client->minorOp == X_ShapeSelectInput ||
                client->minorOp == X_ShapeInputSelected ||
                client->minorOp == X_ShapeGetRectangles)
                goto pass;
            goto reject;

        /* XKEYBOARD, XINPUT, XFIXES, SYNC, RANDR, RENDER, PRESENT, DRI2/3, SHM,
           DAMAGE, etc. are enforced per-resource and per-access-mode in
           hookResourceAccess, hookDevice, hookSelectionFilter, and hookServerAccess. */
        default:
            goto pass;
    }

    goto pass;

reject:
    XNS_HOOK_LOG("rejecting extension call: %s\n", param->ext->name);
    param->status = BadAccess;
    return;

pass:
    param->status = Success;
    return;
}

/* SPDX-License-Identifier: X11 OR MIT OR AGPL-3.0-or-later */
/* Copyright (C) 2026 Enrico Weigelt, metux IT consult <info@metux.net> */
#define HOOK_NAME "ext-access"

#include <dix-config.h>

#include "dix/dix_priv.h"
#include "dix/extension_priv.h"
#include "dix/registry_priv.h"
#include "Xext/xacestr.h"

#include "namespace.h"
#include "namespaceproto.h"
#include "hooks.h"

/* called on X_QueryExtension */
void hookExtAccess(CallbackListPtr *pcbl, void *unused, void *calldata)
{
    XNS_HOOK_HEAD(ExtensionAccessCallbackParam);

    /* only the builtin root namespace may access the X-NAMESPACE management
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

    switch (param->ext->index + EXTENSION_BASE) {
        /* unrestricted access */
        case EXTENSION_MAJOR_BIG_REQUESTS:
        case EXTENSION_MAJOR_DAMAGE:
        case EXTENSION_MAJOR_DOUBLE_BUFFER:
        case EXTENSION_MAJOR_GENERIC_EVENT:
        case EXTENSION_MAJOR_PRESENT:
        case EXTENSION_MAJOR_SYNC:
        case EXTENSION_MAJOR_XC_MISC:
        case EXTENSION_MAJOR_XFIXES:
        case EXTENSION_MAJOR_XKEYBOARD:
        case EXTENSION_MAJOR_XRESOURCE:
            goto pass;

        /* really blacklisted */
        case EXTENSION_MAJOR_COMPOSITE:
        case EXTENSION_MAJOR_MIT_SCREEN_SAVER:
        case EXTENSION_MAJOR_RECORD:
        case EXTENSION_MAJOR_SECURITY:
        case EXTENSION_MAJOR_XTEST:
            goto reject;

        /* allow QueryExtension for SHAPE and XINPUT so client libraries do not
           permanently cache extension absence; actual requests are gated live
           in hookExtDispatch */
        case EXTENSION_MAJOR_SHAPE:
        case EXTENSION_MAJOR_XINPUT:
            goto pass;
    }

    goto pass;

reject:
    param->status = BadAccess;
    return;

pass:
    param->status = Success;
    return;
}

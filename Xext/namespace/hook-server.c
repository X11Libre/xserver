/* SPDX-License-Identifier: X11 OR MIT OR AGPL-3.0-or-later */
/* Copyright (C) 2026 Enrico Weigelt, metux IT consult <info@metux.net> */
#define HOOK_NAME "server"

#include <dix-config.h>

#include "dix/dix_priv.h"
#include "dix/registry_priv.h"
#include "dix/server_priv.h"
#include "Xext/xacestr.h"

#include "namespace.h"
#include "hooks.h"

void hookServerAccess(CallbackListPtr *pcbl, void *unused, void *calldata)
{
    XNS_HOOK_HEAD(ServerAccessCallbackParam);

    if (subj->ns->superPower)
        goto pass;

    /* Allow read-only server queries (ListFonts, ListFontsWithInfo, GetFontPath, ListHosts);
       block server management, debug, and global server grab (GrabServer/SetFontPath/ChangeHosts). */
    if ((param->access_mode & ~(DixReadAccess | DixGetAttrAccess)) == 0)
        goto pass;

    XNS_HOOK_LOG("BLOCKED access to server configuration request %s\n",
        LookupRequestName(client->majorOp, client->minorOp));

    param->status = BadAccess;
    return;

pass:
    param->status = Success;
}

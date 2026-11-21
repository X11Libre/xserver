/* SPDX-License-Identifier: X11 OR MIT OR AGPL-3.0-or-later */
/* Copyright (C) 2026 Enrico Weigelt, metux IT consult <info@metux.net> */
#define HOOK_NAME "clienstate"

#include <dix-config.h>

#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "dix/registry_priv.h"
#include "os/client_priv.h"
#include "os/auth.h"

#include "namespace.h"
#include "namespaceproto.h"
#include "hooks.h"

/*
 * Inspect /proc/<pid>/cgroup for a configured sandbox cgroup prefix:
 *   0::<namespaceCgroupPrefix>[<uid>/]<boxname>[/...]
 * If matched, look up or auto-create a transient Xnamespace "<boxname>" using
 * the capabilities configured in the "cgroup <prefix>" block (ns_cgroup_tmpl).
 *
 * Non-local connections (pid <= 0, e.g. TCP) or unreadable /proc/<pid>/cgroup
 * always fail closed to ns_anon. Verified local processes outside the sandbox
 * cgroup prefix use ns_root if "default root" was configured, or ns_anon.
 */
static struct Xnamespace *resolveClientNamespace(ClientPtr client)
{
    if (!namespaceDefaultRoot && !namespaceCgroupPrefix)
        return &ns_anon;

    pid_t pid = GetClientPid(client);
    if (pid <= 0)
        return &ns_anon;

    char path[64];
    char buf[1024];
    const char *box = NULL;
    size_t boxlen = 0;

    snprintf(path, sizeof(path), "/proc/%d/cgroup", (int)pid);
    FILE *fp = fopen(path, "r");
    if (!fp)
        return &ns_anon;

    size_t pfxlen = namespaceCgroupPrefix ? strlen(namespaceCgroupPrefix) : 0;

    if (pfxlen > 0) {
        while (fgets(buf, sizeof(buf), fp)) {
            const char *colon = strrchr(buf, ':');
            if (!colon)
                continue;
            const char *cg = colon + 1;
            if (strncmp(cg, namespaceCgroupPrefix, pfxlen) != 0)
                continue;
            const char *p = cg + pfxlen;

            /* if next segment is numeric <uid>/, skip it */
            const char *slash = strchr(p, '/');
            if (slash && slash > p) {
                Bool all_digits = TRUE;
                for (const char *d = p; d < slash; d++) {
                    if (!isdigit((unsigned char)*d)) {
                        all_digits = FALSE;
                        break;
                    }
                }
                if (all_digits)
                    p = slash + 1;
            }

            if (*p && *p != '\n' && *p != '\r') {
                const char *end = p;
                while (*end && *end != '/' && *end != '\n' && *end != '\r')
                    end++;
                if (end > p) {
                    box = p;
                    boxlen = (size_t)(end - p);
                    break;
                }
            }
        }
    }
    fclose(fp);

    if (!box || boxlen == 0)
        return namespaceDefaultRoot ? &ns_root : &ns_anon;

    struct Xnamespace *ns = XnsLookup(box, boxlen);
    if (ns) {
        if (ns->builtin || !ns->autoRemove)
            return &ns_anon;
        return ns;
    }

    int err = Success;
    CARD32 default_caps = XnsCaps(&ns_cgroup_tmpl);
    ns = XnsCreate(box, boxlen, default_caps, XNS_ATTR_TRANSIENT, &err);
    if (ns) {
        XNS_LOG("auto-created transient namespace \"%s\" for pid %d\n",
                ns->name, (int)pid);
        return ns;
    }
    return &ns_anon;
}

void hookClientState(CallbackListPtr *pcbl, void *unused, void *calldata)
{
    XNS_HOOK_HEAD(NewClientInfoRec);

    switch (client->clientState) {
    case ClientStateInitial:
        // better assign *someting* than null -- clients can't do anything yet anyways
        XnamespaceAssignClient(subj, &ns_anon);
        break;

    case ClientStateRunning: {
        subj->authId = AuthorizationIDOfClient(client);

        struct Xnamespace *target = NULL;
        short unsigned int name_len = 0, data_len = 0;
        const char *name = NULL;
        char *data = NULL;
        if (AuthorizationFromID(subj->authId, &name_len, &name, &data_len, &data)) {
            struct Xnamespace *byAuth = XnsFindByAuth(name_len, name, data_len, data);
            if (byAuth && byAuth != &ns_anon)
                target = byAuth;
        }

        if (!target)
            target = resolveClientNamespace(client);

        XnamespaceAssignClient(subj, target);
        XNS_HOOK_LOG("assigned client #%d (pid %d) -> namespace \"%s\"\n",
                     client->index, (int)GetClientPid(client), target->name);
        break;
    }

    case ClientStateRetained:
        break;
    case ClientStateGone:
        break;
    default:
        XNS_HOOK_LOG("unknown state =%d\n", client->clientState);
        break;
    }
}

void hookClientDestroy(CallbackListPtr *pcbl, void *unused, void *calldata)
{
    ClientPtr client = calldata;
    struct XnamespaceClientPriv *subj = XnsClientPriv(client);

    if (!subj)
        return; /* no XNS devprivate assigned ? */

    XnamespaceAssignClient(subj, NULL);
    /* the devprivate is embedded, so no free() necessary */
}

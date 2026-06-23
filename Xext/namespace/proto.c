/* SPDX-License-Identifier: MIT OR X11
 *
 * Xnamespace management extension - protocol dispatch (DRAFT SKELETON)
 *
 * See doc/Xnamespace-protocol.md. Request parsing uses the X_REQUEST_*
 * macros (including the X_REQUEST_VAR_* helpers); the namespace-model
 * mutations go through the shared setter layer in config.c, which is also
 * used by the config loader so both share one code path.
 *
 * Access control is NOT done here: the extension is made unreachable to
 * non-superPower clients in hook-ext-access.c / hook-ext-dispatch.c, so
 * every handler below may assume its caller is superPower.
 */
#include <dix-config.h>

#include <X11/Xmd.h>
#include <X11/Xproto.h>

#include "dix/dix_priv.h"
#include "dix/request_priv.h"
#include "dix/rpcbuf_priv.h"
#include "include/dixstruct.h"
#include "include/os.h"
#include "miext/extinit_priv.h"

#include "namespace.h"
#include "namespaceproto.h"

static unsigned char XnsReqCode = 0;

/* The namespace-model setters (XnsCreate/XnsDelete/XnsSetCaps/XnsAddToken/
   XnsRemoveToken/XnsLookup/XnsCaps/XnsAttrs/XnsCountTokens) live in config.c
   and are shared with the config loader - declared in namespace.h. */

/* ------------------------------------------------------------------ *
 *  Requests
 * ------------------------------------------------------------------ */

/** @brief Negotiate the extension (major, minor) version. */
static int
ProcXnsQueryVersion(ClientPtr client)
{
    X_REQUEST_HEAD_STRUCT(xXnsQueryVersionReq);
    X_REQUEST_FIELD_CARD16(clientMajorVersion);
    X_REQUEST_FIELD_CARD16(clientMinorVersion);

    xXnsQueryVersionReply reply = {
        .majorVersion = XNS_MAJOR_VERSION,
        .minorVersion = (stuff->clientMajorVersion < XNS_MAJOR_VERSION)
                            ? stuff->clientMinorVersion : XNS_MINOR_VERSION,
    };
    X_REPLY_FIELD_CARD16(majorVersion);
    X_REPLY_FIELD_CARD16(minorVersion);
    return X_SEND_REPLY_SIMPLE(client, reply);
}

/** @brief Create a namespace with the requested capabilities/attributes. */
static int
ProcXnsCreateNamespace(ClientPtr client)
{
    X_REQUEST_HEAD_AT_LEAST(xXnsCreateNamespaceReq);
    X_REQUEST_FIELD_CARD32(capabilities);
    X_REQUEST_FIELD_CARD32(attributes);
    X_REQUEST_FIELD_CARD16(nameLen);

    const char *name;
    X_REQUEST_VAR_BEGIN();
    X_REQUEST_VAR_FIELD(name, stuff->nameLen);
    X_REQUEST_VAR_END();

    if (stuff->capabilities & ~XNS_CAPABILITY_ALL)
        return BadValue;
    if (stuff->attributes & (~XNS_ATTR_ALL | XNS_ATTR_IMMUTABLE))
        return BadValue;       /* IMMUTABLE is server-set only */

    int err = Success;
    if (!XnsCreate(name, stuff->nameLen, stuff->capabilities,
                   stuff->attributes, &err))
        return err;            /* BadName / BadAlloc */

    xXnsAckReply reply = { 0 };
    return X_SEND_REPLY_SIMPLE(client, reply);
}

/** @brief Delete a namespace, optionally killing its clients first. */
static int
ProcXnsDeleteNamespace(ClientPtr client)
{
    X_REQUEST_HEAD_AT_LEAST(xXnsDeleteNamespaceReq);
    X_REQUEST_FIELD_CARD16(nameLen);

    const char *name;
    X_REQUEST_VAR_BEGIN();
    X_REQUEST_VAR_FIELD(name, stuff->nameLen);
    X_REQUEST_VAR_END();

    if (stuff->onClients != XNS_DELETE_FAIL_IF_BUSY &&
        stuff->onClients != XNS_DELETE_KILL_CLIENTS)
        return BadValue;

    struct Xnamespace *ns = XnsLookup(name, stuff->nameLen);
    if (!ns)
        return BadName;
    if (XnsAttrs(ns) & XNS_ATTR_IMMUTABLE)
        return BadAccess;

    /* refuse to delete the namespace the caller itself belongs to: killing
       our own client mid-request would free `client` under us (UAF on the
       reply path). A manager lives in a separate superPower namespace. */
    struct XnamespaceClientPriv *self = XnsClientPriv(client);
    if (self && self->ns == ns)
        return BadAccess;

    int rc = XnsDelete(ns, stuff->onClients);
    if (rc != Success)
        return rc;             /* BadAccess if busy && FAIL_IF_BUSY */

    xXnsAckReply reply = { 0 };
    return X_SEND_REPLY_SIMPLE(client, reply);
}

/** @brief Report a namespace's capabilities, attributes, refcount and token count. */
static int
ProcXnsQueryNamespace(ClientPtr client)
{
    X_REQUEST_HEAD_AT_LEAST(xXnsQueryNamespaceReq);
    X_REQUEST_FIELD_CARD16(nameLen);

    const char *name;
    X_REQUEST_VAR_BEGIN();
    X_REQUEST_VAR_FIELD(name, stuff->nameLen);
    X_REQUEST_VAR_END();

    struct Xnamespace *ns = XnsLookup(name, stuff->nameLen);
    if (!ns)
        return BadName;

    xXnsQueryNamespaceReply reply = {
        .capabilities = XnsCaps(ns),
        .attributes   = XnsAttrs(ns),
        .refcnt       = (CARD32) ns->refcnt,
        .numTokens    = XnsCountTokens(ns),
    };
    X_REPLY_FIELD_CARD32(capabilities);
    X_REPLY_FIELD_CARD32(attributes);
    X_REPLY_FIELD_CARD32(refcnt);
    X_REPLY_FIELD_CARD32(numTokens);
    return X_SEND_REPLY_SIMPLE(client, reply);
}

/** @brief Apply a masked capability update to a namespace. */
static int
ProcXnsSetNamespaceFlags(ClientPtr client)
{
    X_REQUEST_HEAD_AT_LEAST(xXnsSetNamespaceFlagsReq);
    X_REQUEST_FIELD_CARD32(valueMask);
    X_REQUEST_FIELD_CARD32(values);
    X_REQUEST_FIELD_CARD16(nameLen);

    const char *name;
    X_REQUEST_VAR_BEGIN();
    X_REQUEST_VAR_FIELD(name, stuff->nameLen);
    X_REQUEST_VAR_END();

    if (stuff->valueMask & ~XNS_CAPABILITY_ALL)
        return BadValue;

    struct Xnamespace *ns = XnsLookup(name, stuff->nameLen);
    if (!ns)
        return BadName;
    if (XnsAttrs(ns) & XNS_ATTR_IMMUTABLE)
        return BadAccess;

    int rc = XnsSetCaps(ns, stuff->valueMask, stuff->values);
    if (rc != Success)
        return rc;

    xXnsSetNamespaceFlagsReply reply = { .capabilities = XnsCaps(ns) };
    X_REPLY_FIELD_CARD32(capabilities);
    return X_SEND_REPLY_SIMPLE(client, reply);
}

/** @brief Register an auth token in a namespace; reply with its handle. */
static int
ProcXnsAddAuthToken(ClientPtr client)
{
    X_REQUEST_HEAD_AT_LEAST(xXnsAddAuthTokenReq);
    X_REQUEST_FIELD_CARD16(nameLen);
    X_REQUEST_FIELD_CARD16(protoLen);
    X_REQUEST_FIELD_CARD16(dataLen);

    /* three variable-length fields, peeled off in declared order */
    const char *name, *proto, *data;
    X_REQUEST_VAR_BEGIN();
    X_REQUEST_VAR_FIELD(name,  stuff->nameLen);
    X_REQUEST_VAR_FIELD(proto, stuff->protoLen);
    X_REQUEST_VAR_FIELD(data,  stuff->dataLen);
    X_REQUEST_VAR_END();

    if (stuff->protoLen == 0)
        return BadValue;

    struct Xnamespace *ns = XnsLookup(name, stuff->nameLen);
    if (!ns)
        return BadName;
    if (XnsAttrs(ns) & XNS_ATTR_IMMUTABLE)
        return BadAccess;

    CARD32 handle = 0;
    int rc = XnsAddToken(ns, proto, stuff->protoLen, data, stuff->dataLen,
                         &handle);
    if (rc != Success)
        return rc;             /* BadAlloc */

    xXnsAddAuthTokenReply reply = { .tokenHandle = handle };
    X_REPLY_FIELD_CARD32(tokenHandle);
    return X_SEND_REPLY_SIMPLE(client, reply);
}

/** @brief Remove an auth token from a namespace by handle. */
static int
ProcXnsRemoveAuthToken(ClientPtr client)
{
    X_REQUEST_HEAD_AT_LEAST(xXnsRemoveAuthTokenReq);
    X_REQUEST_FIELD_CARD32(tokenHandle);
    X_REQUEST_FIELD_CARD16(nameLen);

    const char *name;
    X_REQUEST_VAR_BEGIN();
    X_REQUEST_VAR_FIELD(name, stuff->nameLen);
    X_REQUEST_VAR_END();

    struct Xnamespace *ns = XnsLookup(name, stuff->nameLen);
    if (!ns)
        return BadName;
    if (XnsAttrs(ns) & XNS_ATTR_IMMUTABLE)
        return BadAccess;

    int rc = XnsRemoveToken(ns, stuff->tokenHandle);
    if (rc != Success)
        return rc;             /* BadMatch */

    xXnsAckReply reply = { 0 };
    return X_SEND_REPLY_SIMPLE(client, reply);
}

/** @brief List every namespace with its capabilities, attributes and counts. */
static int
ProcXnsListNamespaces(ClientPtr client)
{
    X_REQUEST_HEAD_STRUCT(xXnsListNamespacesReq);

    x_rpcbuf_t buf = { .swapped = client->swapped, .err_clear = TRUE };
    CARD32 count = 0;

    struct Xnamespace *ns;
    xorg_list_for_each_entry(ns, &ns_list, entry) {
        x_rpcbuf_write_CARD32(&buf, XnsCaps(ns));
        x_rpcbuf_write_CARD32(&buf, XnsAttrs(ns));
        x_rpcbuf_write_CARD32(&buf, (CARD32) ns->refcnt);
        x_rpcbuf_write_CARD32(&buf, XnsCountTokens(ns));
        x_rpcbuf_write_CARD16(&buf, (CARD16) strlen(ns->name));
        x_rpcbuf_write_CARD16(&buf, 0 /* pad */);
        x_rpcbuf_write_string_pad(&buf, ns->name);
        count++;
    }

    xXnsListNamespacesReply reply = { .count = count };
    X_REPLY_FIELD_CARD32(count);
    return X_SEND_REPLY_WITH_RPCBUF(client, reply, buf);
}

/** @brief List a namespace's auth tokens (handle + protocol name; no key data). */
static int
ProcXnsListAuthTokens(ClientPtr client)
{
    X_REQUEST_HEAD_AT_LEAST(xXnsListAuthTokensReq);
    X_REQUEST_FIELD_CARD16(nameLen);

    const char *name;
    X_REQUEST_VAR_BEGIN();
    X_REQUEST_VAR_FIELD(name, stuff->nameLen);
    X_REQUEST_VAR_END();

    struct Xnamespace *ns = XnsLookup(name, stuff->nameLen);
    if (!ns)
        return BadName;

    x_rpcbuf_t buf = { .swapped = client->swapped, .err_clear = TRUE };
    CARD32 count = 0;
    struct auth_token *at;
    xorg_list_for_each_entry(at, &ns->auth_tokens, entry) {
        x_rpcbuf_write_CARD32(&buf, at->handle);
        x_rpcbuf_write_CARD16(&buf, (CARD16)(at->authProto ? strlen(at->authProto) : 0));
        x_rpcbuf_write_CARD16(&buf, 0 /* pad */);
        x_rpcbuf_write_string_pad(&buf, at->authProto);
        /* deliberately NOT writing authTokenData - no key exfiltration */
        count++;
    }

    xXnsListAuthTokensReply reply = { .count = count };
    X_REPLY_FIELD_CARD32(count);
    return X_SEND_REPLY_WITH_RPCBUF(client, reply, buf);
}

/** @brief Report which namespace a given client (0 = caller) belongs to. */
static int
ProcXnsGetClientNamespace(ClientPtr client)
{
    X_REQUEST_HEAD_STRUCT(xXnsGetClientNamespaceReq);
    X_REQUEST_FIELD_CARD32(clientResource);

    ClientPtr target = client;
    if (stuff->clientResource != 0) {
        int rc = dixLookupResourceOwner(&target, stuff->clientResource, client,
                                        DixGetAttrAccess);
        if (rc != Success)
            return rc;
    }

    struct XnamespaceClientPriv *priv = XnsClientPriv(target);
    const char *nsname = (priv && priv->ns) ? priv->ns->name : "";

    x_rpcbuf_t buf = { .swapped = client->swapped, .err_clear = TRUE };
    x_rpcbuf_write_string_pad(&buf, nsname);

    xXnsGetClientNamespaceReply reply = {
        .isServer = (priv && priv->isServer) ? TRUE : FALSE,
        .nameLen  = (CARD16) strlen(nsname),
    };
    X_REPLY_FIELD_CARD16(nameLen);
    return X_SEND_REPLY_WITH_RPCBUF(client, reply, buf);
}

/* ------------------------------------------------------------------ *
 *  Dispatch (registered for both proc and sproc - all field swapping
 *  happens inside the handlers via the X_REQUEST_* macros, like DPMS)
 * ------------------------------------------------------------------ */

/**
 * @brief Extension request entry point (used for both swapped and unswapped
 *        clients; each handler does its own field swapping via the
 *        X_REQUEST_* macros).
 * @param client the requesting client
 * @return an X11 status code
 */
static int
ProcXnsDispatch(ClientPtr client)
{
    REQUEST(xReq);

    switch (stuff->data) {
    case X_XnsQueryVersion:        return ProcXnsQueryVersion(client);
    case X_XnsListNamespaces:      return ProcXnsListNamespaces(client);
    case X_XnsCreateNamespace:     return ProcXnsCreateNamespace(client);
    case X_XnsDeleteNamespace:     return ProcXnsDeleteNamespace(client);
    case X_XnsQueryNamespace:      return ProcXnsQueryNamespace(client);
    case X_XnsSetNamespaceFlags:   return ProcXnsSetNamespaceFlags(client);
    case X_XnsAddAuthToken:        return ProcXnsAddAuthToken(client);
    case X_XnsRemoveAuthToken:     return ProcXnsRemoveAuthToken(client);
    case X_XnsListAuthTokens:      return ProcXnsListAuthTokens(client);
    case X_XnsGetClientNamespace:  return ProcXnsGetClientNamespace(client);
    default:                       return BadRequest;
    }
}

/**
 * @brief Register the namespace management extension.
 *
 * Called from NamespaceExtensionInit() once a namespace config has loaded.
 * Access is gated to superPower clients by the Xace extension hooks, so the
 * extension is invisible and unreachable to namespaced clients.
 */
void
XnsProtoExtensionInit(void)
{
    ExtensionEntry *ext = AddExtension(XNS_EXTENSION_NAME, 0, 0,
                                       ProcXnsDispatch, ProcXnsDispatch,
                                       NULL, StandardMinorOpcode);
    if (ext)
        XnsReqCode = (unsigned char) ext->base;
}

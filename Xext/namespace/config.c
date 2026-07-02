#include <dix-config.h>

#include <stdlib.h>
#include <string.h>
#include <X11/Xdefs.h>

#include "os/auth.h"
#include "include/os.h"
#include "include/dix.h"
#include "dix/dix_priv.h"

#include "namespace.h"
#include "namespaceproto.h"

struct Xnamespace ns_root = {
    .allowMouseMotion = TRUE,
    .allowShape = TRUE,
    .allowTransparency = TRUE,
    .allowXInput = TRUE,
    .allowXKeyboard = TRUE,
    .builtin = TRUE,
    .name = NS_NAME_ROOT,
    .refcnt = 1,
    .superPower = TRUE,
};

struct Xnamespace ns_anon = {
    .builtin = TRUE,
    .name = NS_NAME_ANONYMOUS,
    .refcnt = 1,
};

struct xorg_list ns_list = { 0 };

char *namespaceConfigFile = NULL;

static struct Xnamespace* select_ns(const char* name)
{
    struct Xnamespace *ns = XnsLookup(name, strlen(name));
    if (ns)
        return ns;

    struct Xnamespace *newns = calloc(1, sizeof(struct Xnamespace));
    newns->name = strdup(name);
    xorg_list_init(&newns->auth_tokens);
    xorg_list_append(&newns->entry, &ns_list);
    return newns;
}

#define atox(c) ('0' <= (c) && (c) <= '9' ? (c) - '0' : \
                 'a' <= (c) && (c) <= 'f' ? (c) - 'a' + 10 : \
                 'A' <= (c) && (c) <= 'F' ? (c) - 'A' + 10 : -1)

// warning: no error checking, no buffer clearing
static int hex2bin(const char *in, char *out)
{
    while (in[0] && in[1]) {
        int top = atox(in[0]);
        if (top == -1)
            return 0;
        int bottom = atox(in[1]);
        if (bottom == -1)
            return 0;
        *out++ = (top << 4) | bottom;
        in += 2;
    }
    return 1;
}

/**
 * @brief Parse a single line from the configuration file,
 * ignoring comments and newlines. Prints a warning if it finds an unknown token.
*/
static void parseLine(char *line, struct Xnamespace **walk_ns)
{
    // trim newline and comments
    char *c1 = strchr(line, '\n');
    if (c1 != NULL)
        *c1 = 0;
    c1 = strchr(line, '#');
    if (c1 != NULL)
        *c1 = 0;

    /* get the first token */
    char *token = strtok(line, " \t");

    if (token == NULL)
        return;

    /* if no "namespace" statement hasn't been issued yet, use root NS */
    struct Xnamespace * curr = (*walk_ns ? *walk_ns : &ns_root);

    if ((strcmp(token, "namespace") == 0) ||
        (strcmp(token, "container") == 0)) /* "container" is deprecated ! */
    {
        if ((token = strtok(NULL, " ")) == NULL)
        {
            XNS_LOG("namespace missing id\n");
            return;
        }

        curr = *walk_ns = select_ns(token);
        return;
    }

    if (strcmp(token, "auth") == 0)
    {
        char *proto = strtok(NULL, " \t");
        if (proto == NULL)
            return;

        char *hex = strtok(NULL, " ");
        if (hex == NULL)
            return;

        size_t binlen = strlen(hex) / 2;
        char *bin = calloc(1, binlen ? binlen : 1);
        if (bin == NULL)
            FatalError("Xnamespace: failed allocating token\n");

        if (!hex2bin(hex, bin)) {
            XNS_LOG("invalid hex auth data, ignoring\n");
            free(bin);
            return;
        }

        /* the config file stores the key hex-encoded; the model stores it
           binary. Registration itself is shared with the protocol path. */
        if (XnsAddToken(curr, proto, strlen(proto), bin, binlen, NULL) != Success)
            XNS_LOG("failed to add auth token for namespace \"%s\"\n", curr->name);
        free(bin);
        return;
    }

    if (strcmp(token, "allow") == 0)
    {
        while ((token = strtok(NULL, " \t")) != NULL)
        {
            if (strcmp(token, "mouse-motion") == 0)
                curr->allowMouseMotion = TRUE;
            else if (strcmp(token, "shape") == 0)
                curr->allowShape = TRUE;
            else if (strcmp(token, "transparency") == 0)
                curr->allowTransparency = TRUE;
            else if (strcmp(token, "xinput") == 0)
                curr->allowXInput = TRUE;
            else if (strcmp(token, "xkeyboard") == 0)
                curr->allowXKeyboard = TRUE;
            else
                XNS_LOG("unknown allow: %s\n", token);
        }
        return;
    }

    if (strcmp(token, "superpower") == 0)
    {
        curr->superPower = TRUE;
        return;
    }

    XNS_LOG("unknown token \"%s\"\n", token);
}

Bool XnsLoadConfig(void)
{
    xorg_list_append_ndup(&ns_root.entry, &ns_list);
    xorg_list_append_ndup(&ns_anon.entry, &ns_list);
    xorg_list_init(&ns_root.auth_tokens);
    xorg_list_init(&ns_anon.auth_tokens);

    if (!namespaceConfigFile) {
        XNS_LOG("no namespace config given - Xnamespace disabled\n");
        return FALSE;
    }

    FILE *fp = fopen(namespaceConfigFile, "r");
    if (fp == NULL) {
        FatalError("failed loading namespace config: %s\n", namespaceConfigFile);
        return FALSE;
    }

    struct Xnamespace *walk_ns = NULL;
    char linebuf[1024];
    while (fgets(linebuf, sizeof(linebuf), fp) != NULL)
        parseLine(linebuf, &walk_ns);

    fclose(fp);

    XNS_LOG("loaded namespace config file: %s\n", namespaceConfigFile);

    struct Xnamespace *ns;
    xorg_list_for_each_entry(ns, &ns_list, entry) {
        XNS_LOG("namespace: \"%s\" \n", ns->name);
        struct auth_token *at;
        xorg_list_for_each_entry(at, &ns->auth_tokens, entry) {
            XNS_LOG("      auth: \"%s\" \"", at->authProto);
            for (int i=0; i<at->authTokenLen; i++)
                printf("%02X", (unsigned char)at->authTokenData[i]);
            printf("\"\n");
        }
    }

    return TRUE;
}

/**
 * @brief Look up a namespace by name, comparing exactly @p namelen bytes.
 *
 * Length-aware so it can be called with names that are not NUL-terminated
 * (e.g. straight out of a request buffer).
 *
 * @param name    pointer to the name bytes (need not be NUL-terminated)
 * @param namelen number of bytes to compare
 * @return the matching namespace, or NULL if none matches
 */
struct Xnamespace *XnsLookup(const char *name, size_t namelen)
{
    struct Xnamespace *walk;
    xorg_list_for_each_entry(walk, &ns_list, entry) {
        if (strlen(walk->name) == namelen &&
            memcmp(walk->name, name, namelen) == 0)
            return walk;
    }
    return NULL;
}

struct Xnamespace *XnsFindByName(const char* name) {
    /* the (NUL-terminated) name path is just the length-aware lookup */
    return XnsLookup(name, strlen(name));
}

/**
 * @brief Register an authentication token and map it into a namespace.
 *
 * Copies the protocol name and key, registers the authorization with the OS
 * layer and assigns a per-namespace handle for later removal. The single
 * code path for adding tokens, shared by the config loader and (later) the
 * management protocol.
 *
 * @param ns       the namespace to add the token to
 * @param proto    auth protocol name bytes (e.g. "MIT-MAGIC-COOKIE-1")
 * @param protolen length of @p proto
 * @param data     raw key bytes (may be NULL when @p datalen is 0)
 * @param datalen  length of @p data
 * @param[out] handleOut if non-NULL, set to the new token's handle
 * @return Success, or BadAlloc on allocation failure
 */
int XnsAddToken(struct Xnamespace *ns, const char *proto, size_t protolen,
                const char *data, size_t datalen, CARD32 *handleOut)
{
    struct auth_token *t = calloc(1, sizeof(*t));
    if (!t)
        return BadAlloc;

    t->authProto = strndup(proto, protolen);
    if (!t->authProto) {
        free(t);
        return BadAlloc;
    }

    t->authTokenLen = datalen;
    if (datalen) {
        t->authTokenData = malloc(datalen);
        if (!t->authTokenData) {
            free(t->authProto);
            free(t);
            return BadAlloc;
        }
        memcpy(t->authTokenData, data, datalen);
    }

    t->authId = AddAuthorization((unsigned int) protolen, t->authProto,
                                 (unsigned int) datalen, t->authTokenData);
    t->handle = ++ns->tokenHandleSeq;   /* 1-based; 0 is never a valid handle */
    xorg_list_append(&t->entry, &ns->auth_tokens);

    if (handleOut)
        *handleOut = t->handle;
    return Success;
}

/**
 * @brief Validate a namespace name coming off the wire.
 *
 * Config-file input is trusted and does not go through this. Rejects empty
 * or over-long names and any non-printable, whitespace, '#' (comment) or '/'
 * (reserved for future nesting) characters, so a protocol-created name stays
 * expressible in the config file.
 *
 * @param name pointer to the name bytes
 * @param len  number of bytes
 * @return Success if acceptable, else BadName
 */
static int validateName(const char *name, size_t len)
{
    if (len == 0 || len > XNS_NAME_MAX)
        return BadName;
    for (size_t i = 0; i < len; i++) {
        unsigned char c = (unsigned char) name[i];
        /* printable, no whitespace/control, no '#' (comment) or '/' (reserved
           for future nesting) - keeps protocol names config-file expressible */
        if (c <= ' ' || c == 0x7f || c == '#' || c == '/')
            return BadName;
    }
    return Success;
}

/**
 * @brief Apply a capability bitmask to a namespace's individual flag fields.
 * @param ns   the namespace to update
 * @param caps capability bitmask (XNS_CAPABILITY_*)
 */
static void capsToFields(struct Xnamespace *ns, CARD32 caps)
{
    ns->allowMouseMotion  = !!(caps & XNS_CAPABILITY_MOUSE_MOTION);
    ns->allowShape        = !!(caps & XNS_CAPABILITY_SHAPE);
    ns->allowTransparency = !!(caps & XNS_CAPABILITY_TRANSPARENCY);
    ns->allowXInput       = !!(caps & XNS_CAPABILITY_INPUT);
    ns->allowXKeyboard    = !!(caps & XNS_CAPABILITY_KEYBOARD);
    ns->superPower        = !!(caps & XNS_CAPABILITY_ADMIN);
}

/**
 * @brief Build the capability bitmask describing a namespace.
 * @param ns the namespace to inspect
 * @return the XNS_CAPABILITY_* bitmask of its enabled capabilities
 */
CARD32 XnsCaps(const struct Xnamespace *ns)
{
    return (ns->allowMouseMotion  ? XNS_CAPABILITY_MOUSE_MOTION : 0)
         | (ns->allowShape        ? XNS_CAPABILITY_SHAPE        : 0)
         | (ns->allowTransparency ? XNS_CAPABILITY_TRANSPARENCY : 0)
         | (ns->allowXInput       ? XNS_CAPABILITY_INPUT        : 0)
         | (ns->allowXKeyboard    ? XNS_CAPABILITY_KEYBOARD     : 0)
         | (ns->superPower        ? XNS_CAPABILITY_ADMIN        : 0);
}

/**
 * @brief Build the attribute bitmask describing a namespace.
 * @param ns the namespace to inspect
 * @return the XNS_ATTR_* bitmask (e.g. immutable, transient)
 */
CARD32 XnsAttrs(const struct Xnamespace *ns)
{
    return (ns->builtin    ? XNS_ATTR_IMMUTABLE : 0)
         | (ns->autoRemove ? XNS_ATTR_TRANSIENT : 0);
}

/**
 * @brief Atomically update a subset of a namespace's capability bits.
 *
 * Computes @c (old & ~mask) | (values & mask), so several managers can change
 * disjoint bits without read-modify-write races.
 *
 * @param ns     the namespace to update
 * @param mask   which capability bits to apply
 * @param values new values for the masked bits
 * @return Success, or BadValue if @p mask contains reserved bits
 */
int XnsSetCaps(struct Xnamespace *ns, CARD32 mask, CARD32 values)
{
    if (mask & ~XNS_CAPABILITY_ALL)
        return BadValue;
    CARD32 newcaps = (XnsCaps(ns) & ~mask) | (values & mask);
    capsToFields(ns, newcaps);
    return Success;
}

/**
 * @brief Count the auth tokens currently registered in a namespace.
 * @param ns the namespace to inspect
 * @return the number of auth tokens
 */
CARD32 XnsCountTokens(struct Xnamespace *ns)
{
    CARD32 n = 0;
    struct auth_token *at;
    xorg_list_for_each_entry(at, &ns->auth_tokens, entry)
        n++;
    return n;
}

/**
 * @brief Create a new namespace (the runtime equivalent of a config
 *        @c namespace stanza).
 *
 * Validates the name, rejects duplicates, and initialises capabilities and
 * attributes. The name is copied; the caller's buffer need not persist.
 *
 * @param name    pointer to the name bytes (need not be NUL-terminated)
 * @param namelen number of name bytes
 * @param caps    initial capability bitmask (XNS_CAPABILITY_*)
 * @param attrs   initial attribute bitmask (XNS_ATTR_*; only TRANSIENT honored)
 * @param[out] err set to Success, or BadName / BadAlloc on failure
 * @return the new namespace, or NULL on failure (see @p err)
 */
struct Xnamespace *XnsCreate(const char *name, size_t namelen,
                             CARD32 caps, CARD32 attrs, int *err)
{
    int rc = validateName(name, namelen);
    if (rc != Success) {
        *err = rc;
        return NULL;
    }
    if (XnsLookup(name, namelen)) {     /* duplicate */
        *err = BadName;
        return NULL;
    }

    struct Xnamespace *ns = calloc(1, sizeof(*ns));
    if (!ns) {
        *err = BadAlloc;
        return NULL;
    }
    ns->name = strndup(name, namelen);
    if (!ns->name) {
        free(ns);
        *err = BadAlloc;
        return NULL;
    }
    xorg_list_init(&ns->auth_tokens);
    capsToFields(ns, caps);
    ns->autoRemove = !!(attrs & XNS_ATTR_TRANSIENT);
    xorg_list_append(&ns->entry, &ns_list);

    *err = Success;
    return ns;
}

/**
 * @brief Unregister and free a single auth token.
 *
 * Revokes the underlying authorization, unlinks the token from its namespace
 * and frees it.
 *
 * @param at the token to remove (must be linked into a namespace)
 */
static void freeToken(struct auth_token *at)
{
    if (at->authId)
        RemoveAuthorization(at->authProto ? (unsigned short) strlen(at->authProto) : 0,
                            at->authProto,
                            (unsigned short) at->authTokenLen, at->authTokenData);
    xorg_list_del(&at->entry);
    free(at->authProto);
    free(at->authTokenData);
    free(at);
}

/**
 * @brief Tear down a namespace: free its tokens and the namespace itself.
 *
 * Built-in namespaces are never destroyed. Any client still pointing at @p ns
 * is detached first so that its later teardown cannot dereference freed
 * memory. Does not touch refcnt (it is discarded along with the namespace).
 *
 * @param ns the namespace to destroy (NULL and built-ins are ignored)
 */
void XnsDestroyNamespace(struct Xnamespace *ns)
{
    if (!ns || ns->builtin)
        return;

    /* detach any clients still pointing here so their later teardown does not
       dereference freed memory (refcnt is being discarded with the namespace) */
    for (int i = 1; i < currentMaxClients; i++) {
        if (!clients[i])
            continue;
        struct XnamespaceClientPriv *p = XnsClientPriv(clients[i]);
        if (p && p->ns == ns)
            p->ns = NULL;
    }

    struct auth_token *at, *tmp;
    xorg_list_for_each_entry_safe(at, tmp, &ns->auth_tokens, entry)
        freeToken(at);

    xorg_list_del(&ns->entry);
    free((void *) ns->name);
    free(ns);
}

/**
 * @brief Delete a namespace, optionally terminating its clients first.
 *
 * Built-in namespaces cannot be deleted. A non-empty namespace is only
 * removed when @p onClients is XNS_DELETE_KILL_CLIENTS, in which case every
 * client in it is forcibly closed before the namespace is destroyed.
 *
 * @param ns        the namespace to delete
 * @param onClients XNS_DELETE_FAIL_IF_BUSY or XNS_DELETE_KILL_CLIENTS
 * @return Success, or BadAccess (built-in, or busy without the kill flag)
 */
int XnsDelete(struct Xnamespace *ns, CARD8 onClients)
{
    if (ns->builtin)
        return BadAccess;

    if (ns->refcnt > 0) {
        if (onClients != XNS_DELETE_KILL_CLIENTS)
            return BadAccess;       /* busy */

        /* keep the last client's exit from auto-destroying ns under us */
        ns->autoRemove = FALSE;
        for (int i = 1; i < currentMaxClients; i++) {
            if (!clients[i])
                continue;
            struct XnamespaceClientPriv *p = XnsClientPriv(clients[i]);
            if (p && p->ns == ns) {
                /* force full teardown even for RetainPermanent clients, so
                   ClientDestroyCallback fires (refcnt--, priv->ns cleared)
                   and no client is left pointing at the freed namespace.
                   Mirrors KillAllClients() in dix/dispatch.c. */
                clients[i]->closeDownMode = DestroyAll;
                CloseDownClient(clients[i]);
            }
        }
    }

    XnsDestroyNamespace(ns);
    return Success;
}

/**
 * @brief Remove an auth token from a namespace by its handle.
 * @param ns     the namespace to remove from
 * @param handle the token handle returned by XnsAddToken()
 * @return Success, or BadMatch if no token with that handle exists
 */
int XnsRemoveToken(struct Xnamespace *ns, CARD32 handle)
{
    struct auth_token *at, *tmp;
    xorg_list_for_each_entry_safe(at, tmp, &ns->auth_tokens, entry) {
        if (at->handle == handle) {
            freeToken(at);
            return Success;
        }
    }
    return BadMatch;
}

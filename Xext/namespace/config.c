#include <dix-config.h>

#include <stdlib.h>
#include <string.h>
#include <X11/Xdefs.h>

#include "os/auth.h"
#include "include/os.h"

#include "namespace.h"

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

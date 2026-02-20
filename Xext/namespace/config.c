#include <dix-config.h>

#include <string.h>
#include <X11/Xdefs.h>

#include "os/auth.h"

#include "namespace.h"

struct Xnamespace ns_root = {
    .perms = {
        .allowComposite = TRUE,
        .allowGlobalKeyboard = TRUE,
        .allowMouseMotion = TRUE,
        .allowRandr = TRUE,
        .allowRender = TRUE,
        .allowShape = TRUE,
        .allowScreen = TRUE,
        .allowTransparency = TRUE,
        .allowXInput = TRUE,
        .allowXKeyboard = TRUE,
    },
    .builtin = TRUE,
    .isRoot = TRUE,
    .name = NS_NAME_ROOT,
    .refcnt = 1,
    .superPower = TRUE,
};

struct Xnamespace ns_anon = {
    .builtin = TRUE,
    .name = NS_NAME_ANONYMOUS,
    .refcnt = 1,
};

struct Xnamespace *ns_default;

struct xorg_list ns_list = { 0 };

char *namespaceConfigFile = NULL;
static const char *default_namespace;

static struct Xnamespace* select_ns(const char* name)
{
    struct Xnamespace *walk;
    xorg_list_for_each_entry(walk, &ns_list, entry) {
        if (strcmp(walk->name, name)==0)
            return walk;
    }

    struct Xnamespace *newns = calloc(1, sizeof(struct Xnamespace));
    newns->name = strdup(name);
    xorg_list_append(&newns->entry, &ns_list);
    return newns;
}

#define atox(c) ('0' <= c && c <= '9' ? c - '0' : \
                 'a' <= c && c <= 'f' ? c - 'a' + 10 : \
                 'A' <= c && c <= 'F' ? c - 'A' + 10 : -1)

/* warning: no error checking, no buffer clearing */
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
    /* trim newline and comments */
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

    if (strcmp(token, "default") == 0)
    {
        if ((token = strtok(NULL, " \t")) == NULL)
        {
            XNS_LOG("none given, default DENY\n");
            return;
        }
        default_namespace = strdup(token);
        return;
    }

    /* if no "namespace" statement hasn't been issued yet, use root NS */
    struct Xnamespace * curr = (*walk_ns ? *walk_ns : &ns_root);

    if ((strcmp(token, "namespace") == 0) ||
        (strcmp(token, "container") == 0)) /* "container" is deprecated ! */
    {
        if ((token = strtok(NULL, " \t")) == NULL)
        {
            XNS_LOG("namespace missing id\n");
            return;
        }

        curr = *walk_ns = select_ns(token);
        /* anything made from the config should be retained, this flag will serve dual purpose */
        curr->builtin = TRUE;
        return;
    }

    if (strcmp(token, "auth") == 0)
    {
        token = strtok(NULL, " \t");
        if (token == NULL)
            return;

        if (strcmp(token, "generate") == 0) {
            GenerateAuthForXnamespace(curr);
            return;
        }

        struct auth_token *new_token = calloc(1, sizeof(struct auth_token));
        if (new_token == NULL)
            FatalError("Xnamespace: failed allocating token\n");

        new_token->authProto = strdup(token);
        token = strtok(NULL, " \t");

        new_token->authTokenLen = strlen(token)/2;
        new_token->authTokenData = calloc(1, new_token->authTokenLen);
        if (!new_token->authTokenData) {
            free(new_token->authProto);
            free(new_token);
            return;
        }
        hex2bin(token, new_token->authTokenData);

        new_token->authId = AddAuthorization(strlen(new_token->authProto),
                                             new_token->authProto,
                                             new_token->authTokenLen,
                                             new_token->authTokenData);

        xorg_list_append(&new_token->entry, &curr->auth_tokens);
        return;
    }


    if (strcmp(token, "allow") == 0)
    {
        while ((token = strtok(NULL, " \t")) != NULL)
        {
            if (strcmp(token, "mouse-motion") == 0)
                curr->perms.allowMouseMotion = TRUE;
            else if (strcmp(token, "shape") == 0)
                curr->perms.allowShape = TRUE;
            else if (strcmp(token, "transparency") == 0)
                curr->perms.allowTransparency = TRUE;
            else if (strcmp(token, "xinput") == 0)
                curr->perms.allowXInput = TRUE;
            else if (strcmp(token, "xkeyboard") == 0)
                curr->perms.allowXKeyboard = TRUE;
            else if (strcmp(token, "globalxkeyboard") == 0)
                curr->perms.allowGlobalKeyboard = TRUE;
            else if (strcmp(token, "render") == 0)
                curr->perms.allowRender = TRUE;
            else if (strcmp(token, "randr") == 0)
                curr->perms.allowRandr = TRUE;
            else if (strcmp(token, "screen") == 0)
                curr->perms.allowScreen = TRUE;
            else if (strcmp(token, "composite") == 0)
                curr->perms.allowComposite = TRUE;
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
    if (strcmp(token, "client") == 0)
    {
        while ((token = strtok(NULL, " \t")) != NULL) {
            struct client_token *new_token = calloc(1, sizeof(struct client_token));
            new_token->clientName = strdup(token);
            xorg_list_append_ndup(&new_token->entry, &curr->client_list);
            XNS_LOG("client %s added for %s\n",new_token->clientName,curr->name);
        }
        return;
    }
    if (strcmp(token, "client_by_path") == 0)
    {
        while ((token = strtok(NULL, " \t")) != NULL) {
            struct client_token *new_token = calloc(1, sizeof(struct client_token));
            new_token->clientName = strdup(token);
            xorg_list_append_ndup(&new_token->entry, &curr->client_path_list);
            XNS_LOG("client %s added for %s\n",new_token->clientName,curr->name);
        }
        return;
    }
    XNS_LOG("unknown token \"%s\"\n", token);
}

Bool XnsLoadConfig(void)
{
    /* default to anon namespace */
    default_namespace = "anon";
    ns_default = &ns_anon;

    xorg_list_append_ndup(&ns_root.entry, &ns_list);
    xorg_list_append_ndup(&ns_anon.entry, &ns_list);

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

    if (strcmp(default_namespace, "new_ns") == 0) {
        XNS_LOG("Defaulting to NEW Namespace per client\n");
        ns_default->builtin = FALSE;
    } else if (strcmp(default_namespace, "deny") == 0) {
        XNS_LOG("Defaulting to DENY connection\n");
        ns_default->deny = TRUE;
    } else if (strcmp(default_namespace, "anon") == 0) {
        XNS_LOG("Defaulting to anon connection\n");
    } else if (XnsFindByName(default_namespace)) {
        XNS_LOG("found default ns %s\n",default_namespace);
        ns_default = XnsFindByName(default_namespace);
    } else {
        XNS_LOG("Incorrect Default, Defaulting to anon\n");
    }

    return TRUE;
}

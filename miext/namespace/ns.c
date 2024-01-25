
#include <dix-config.h>

#include <stdio.h> // printf // FIXME: use Xorg debug functions
#include <stdbool.h>

#include "namespace.h"

struct Xnamespace namespaces[MAX_NAMESPACE] = { 0 };
int namespace_cnt = 0;

struct Xnamespace *nsFindByName(const char* nsname) {
    for (int x=0; x<namespace_cnt; x++) {
        if (strcmp(namespaces[x].id, nsname) == 0) {
            return &namespaces[x];
        }
    }
    return NULL;
}

struct Xnamespace* findNSByAuth(const char *proto, const char *token)
{
    // start at first user-defined NS --> idx = 2
    for (int x=2; x<namespace_cnt; x++)
    {
        if ((strcmp(namespaces[x].authProto, proto) == 0) &&
            (strcmp(namespaces[x].authToken, token) == 0))
            return &namespaces[x];
    }
    // default to rootns if credentials aren't assigned to specific NS
    return &(namespaces[NS_ROOT]);
}

struct Xnamespace* findNSByAuth2(int proto_n, const char* auth_proto, int string_n, const char* auth_string)
{
    /* store auth credentials for later use */
    char * authProto = (char*)alloca(proto_n+1);
    memcpy(authProto, auth_proto, proto_n);
    authProto[proto_n] = 0;

    char *authToken = (char*)alloca(string_n*2+1);
    char *ptr = authToken;
    for (int x=0; x<string_n; x++)
    {
        snprintf(ptr, 3, "%02hhx", auth_string[x]);
        ptr++;
        ptr++;
    }
    *ptr = 0;

    return findNSByAuth(authProto, authToken);
}

static const char *container_conf_file = "containers.conf";

/*
 * loadConfig
 *
 * Load the container config
*/
static void parseLine(char *line)
{
    // trim newline and comments
    char *c1 = strchr(line, '\n');
    if (c1 != NULL)
        *c1 = 0;
    c1 = strchr(line, '#');
    if (c1 != NULL)
        *c1 = 0;

    /* get the first token */
    char *token = strtok(line, " ");

    if (token == NULL)
        return;

    struct Xnamespace * curr = &namespaces[namespace_cnt-1];

    if (strcmp(token, "container") == 0)
    {
        namespace_cnt++;
        curr = &namespaces[namespace_cnt-1];

        if ((token = strtok(NULL, " ")) == NULL)
        {
            printf("container missing id\n");
            return;
        }
        curr->id = strdup(token);

        if ((token = strtok(NULL, " ")) == NULL) {
            printf("container missing parent id\n");
            return;
        }
        curr->parentId = strdup(token);
        return;
    }

    if (strcmp(token, "auth") == 0)
    {
        token = strtok(NULL, " ");
        if (token == NULL)
            return;
        curr->authProto = strdup(token);

        token = strtok(NULL, " ");
        curr->authToken = strdup(token);
        return;
    }

    if (strcmp(token, "isolate") == 0)
    {
        while ((token = strtok(NULL, " ")) != NULL)
        {
            if (strcmp(token, "objects") == 0)
                curr->isolateObjects = TRUE;
            else if (strcmp(token, "pointer") == 0)
                curr->isolatePointer = TRUE;
            else
                printf("unknown isolate: %s\n", token);
        }
        return;
    }

    if (strcmp(token, "allow") == 0)
    {
        while ((token = strtok(NULL, " ")) != NULL)
        {
            if (strcmp(token, "mouse-motion") == 0)
                curr->allow_mouse_motion = TRUE;
            else
                printf("unknown allow: %s\n", token);
        }
        return;
    }

    if (strcmp(token, "superpower") == 0)
    {
        curr->superPower = TRUE;
        return;
    }
}

void loadConfig(void)
{
    namespaces[NS_ROOT] = (struct Xnamespace) {
        .id = strdup("root"),
        .builtin = TRUE,
        .superPower = TRUE,
        .authProto = strdup("<NONE>"),
        .authToken = strdup(""),
    };
    namespaces[NS_ANONYMOUS] = (struct Xnamespace) {
        .id = strdup("anon"),
        .builtin = TRUE,
        .authProto = strdup("<NONE>"),
        .authToken = strdup(""),
        .isolateObjects = TRUE,
        .isolatePointer = TRUE,
    };
    namespace_cnt = 2;

    char linebuf[1024];
    FILE *fp = fopen(container_conf_file, "r");

    if (fp == NULL)
    {
        printf("failed loading container config: %s\n", container_conf_file);
        return;
    }

    while (fgets(linebuf, sizeof(linebuf), fp) != NULL)
        parseLine(linebuf);

    fclose(fp);

    for (int x=0; x<namespace_cnt; x++) {
        printf("namespace: \"%s\" \"%s\" \"%s\" \"%s\"\n",
            namespaces[x].id,
            namespaces[x].parentId,
            namespaces[x].authProto,
            namespaces[x].authToken);
    }
}

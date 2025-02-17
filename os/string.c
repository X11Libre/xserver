/* SPDX-License-Identifier: MIT OR X11
 *
 * Copyright © 1987, 1998  The Open Group
 * Copyright © 2024 Enrico Weigelt, metux IT consult <info@metux.net>
 */
#include <dix-config.h>

#include <stdlib.h>
#include <string.h>

#include "os/fmt.h"

#include "os.h"

char *
Xstrdup(const char *s)
{
    if (s == NULL)
        return NULL;
    return strdup(s);
}

char *
XNFstrdup(const char *s)
{
    char *ret;

    if (s == NULL)
        return NULL;

    ret = strdup(s);
    if (!ret)
        FatalError("XNFstrdup: Out of memory");
    return ret;
}

/*
 * Tokenize a string into a NULL terminated array of strings. Always returns
 * an allocated array unless an error occurs.
 */
char **
xstrtokenize(const char *str, const char *separators)
{
    char **list, **nlist;
    char *tok, *tmp;
    unsigned num = 0, n;

    if (!str)
        return NULL;
    list = calloc(1, sizeof(*list));
    if (!list)
        return NULL;
    tmp = strdup(str);
    if (!tmp)
        goto error;
    for (tok = strtok(tmp, separators); tok; tok = strtok(NULL, separators)) {
        nlist = reallocarray(list, num + 2, sizeof(*list));
        if (!nlist)
            goto error;
        list = nlist;
        list[num] = strdup(tok);
        if (!list[num])
            goto error;
        list[++num] = NULL;
    }
    free(tmp);
    return list;

 error:
    free(tmp);
    for (n = 0; n < num; n++)
        free(list[n]);
    free(list);
    return NULL;
}

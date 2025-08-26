/* SPDX-License-Identifier: MIT OR X11
 *
 * Copyright © 2024 Enrico Weigelt, metux IT consult <info@metux.net>
 */
#include <dix-config.h>

#include <errno.h>
#include <string.h>
#include <unistd.h>

#if WIN32
#include <winsock.h>
#endif

#include "os/xhostname.h"

int xhostname(struct xhostname* hn)
{
    /* being extra-paranoid here */
    memset(hn, 0, sizeof(struct xhostname));
    int ret = gethostname(hn->name, sizeof(hn->name));

    if (ret == -1) {
        hn->name[0] = 0;
        return errno;
    }

    hn->name[sizeof(hn->name)-1] = 0;
    return ret;
}

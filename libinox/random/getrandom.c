/* SPDX-License-Identifier: X11 or OR AGPLv3
   Copyright (C) 2026 Enrico Weigelt, metux IT consult <info@metux.net> 
*/

#include <stdint.h>
#include <errno.h>
#include <sys/random.h>

#include "inox_random.h"

int inox_random(void *buffer, size_t len)
{
    /* using getrandom() */
    ssize_t ret = getrandom(buffer, len, GRND_RANDOM);

    if (ret == -1) {
        if (errno == EINTR) {
            return inox_random(buffer, len);
        }
        return -errno;
    }

    if (ret < len) {
        return inox_random((unsigned char*)buffer + ret, len - ret);
    }

    return 0;
}

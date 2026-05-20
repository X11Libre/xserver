/* SPDX-License-Identifier: X11 or OR AGPLv3
   Copyright (C) 2026 Enrico Weigelt, metux IT consult <info@metux.net> 
*/

#include <windows.h>
#include <bcrypt.h>
#include <stddef.h>

#include "inox_random.h"

int inox_random(void *buf, size_t len)
{
    if (buf == NULL || len == 0)
        return -1;

    NTSTATUS status = BCryptGenRandom(
        NULL,
        (PUCHAR)buf,
        (ULONG)len,
        BCRYPT_USE_SYSTEM_PREFERRED_RNG
    );

    return BCRYPT_SUCCESS(status) ? 0 : -1;
}

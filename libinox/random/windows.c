/* SPDX-License-Identifier: X11 or OR AGPLv3
   Copyright (C) 2026 Enrico Weigelt, metux IT consult <info@metux.net> 
*/

#include <windows.h>
#include <bcrypt.h>

#include "inox_random.h"

int secure_random(void *buf, size_t len)
{
    NTSTATUS status = BCryptGenRandom(NULL, buf, len, BCRYPT_USE_SYSTEM_PREFERRED_RNG);
    return (status == STATUS_SUCCESS) ? 0 : -1;
}

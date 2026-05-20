/* SPDX-License-Identifier: X11 or OR AGPLv3
   Copyright (C) 2026 Enrico Weigelt, metux IT consult <info@metux.net> 
*/

#include <stdint.h>
#include <stdlib.h>

#include "inox_random.h"

int inox_random(void *buffer, size_t len)
{
    arc4random_buf(buffer, len);
    return 0;
}

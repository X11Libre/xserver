/* SPDX-License-Identifier: MIT OR X11 OR AGPLv3
 *
 * Copyright © 2026 Enrico Weigelt, metux IT consult <info@metux.net>
 */
#include <dix-config.h>

#include <X11/Xproto.h>
#include <X11/extensions/shmproto.h>

#include "include/misc.h"

#include "shm_intern.h"

void ShmSwapCompletionEvent(xEvent *_from, xEvent *_to)
{
    xShmCompletionEvent *from = (xShmCompletionEvent*)_from;
    xShmCompletionEvent *to = (xShmCompletionEvent*)_to;

    *to = *from;
    swaps(&to->sequenceNumber);
    swapl(&to->drawable);
    swaps(&to->minorEvent);
    swapl(&to->shmseg);
    swapl(&to->offset);
}

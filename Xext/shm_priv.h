/* SPDX-License-Identifier: MIT OR X11
 *
 * Copyright © 2024 Enrico Weigelt, metux IT consult <info@metux.net>
 */

#ifndef _XSERVER_XEXT_SHM_PRIV_H
#define _XSERVER_XEXT_SHM_PRIV_H

#include "include/resource.h"
#include "Xext/shmint.h"

typedef struct _ShmDesc {
    struct _ShmDesc *next;
    int shmid;
    int refcnt;
    char *addr;
    Bool writable;
    unsigned long size;
#ifdef SHM_FD_PASSING
    Bool is_fd;
    struct busfault *busfault;
    XID resource;
#endif
} ShmDescRec, *ShmDescPtr;

extern RESTYPE ShmSegType;

#endif /* _XSERVER_XEXT_SHM_PRIV_H */

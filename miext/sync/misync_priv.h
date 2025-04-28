/* SPDX-License-Identifier: MIT OR X11
 *
 * Copyright © 2024 Enrico Weigelt, metux IT consult <info@metux.net>
 * Copyright © 2010 NVIDIA Corporation
 */
#ifndef _XSERVER_MISYNC_PRIV_H
#define _XSERVER_MISYNC_PRIV_H

#include "misync.h"

extern DevPrivateKeyRec miSyncScreenPrivateKey;

typedef struct _syncScreenPriv {
    /* Wrappable sync-specific screen functions */
    SyncScreenFuncsRec funcs;
} SyncScreenPrivRec, *SyncScreenPrivPtr;

#define SYNC_SCREEN_PRIV(pScreen)                               \
    (SyncScreenPrivPtr) dixLookupPrivate(&pScreen->devPrivates, \
                                         &miSyncScreenPrivateKey)

Bool miSyncFenceCheckTriggered(SyncFence * pFence);
void miSyncFenceSetTriggered(SyncFence * pFence);
void miSyncFenceReset(SyncFence * pFence);
void miSyncFenceAddTrigger(SyncTrigger * pTrigger);
void miSyncFenceDeleteTrigger(SyncTrigger * pTrigger);
int miSyncInitFenceFromFD(DrawablePtr pDraw, SyncFence *pFence, int fd, BOOL initially_triggered);
int miSyncFDFromFence(DrawablePtr pDraw, SyncFence *pFence);

#endif /* _XSERVER_MISYNC_PRIV_H */

/*
 * Copyright © 2013 Keith Packard
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that copyright
 * notice and this permission notice appear in supporting documentation, and
 * that the name of the copyright holders not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  The copyright holders make no representations
 * about the suitability of this software for any purpose.  It is provided "as
 * is" without express or implied warranty.
 *
 * THE COPYRIGHT HOLDERS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
 * OF THIS SOFTWARE.
 */

#include <dix-config.h>

#include "dix/input_priv.h"

#include "scrnintstr.h"
#include "misync_priv.h"
#include "misyncstr.h"
#include "misyncfd.h"
#include "pixmapstr.h"

static DevPrivateKeyRec syncFdScreenPrivateKey;

typedef struct _SyncFdScreenPrivate {
    SyncFdScreenFuncsRec        funcs;
} SyncFdScreenPrivateRec, *SyncFdScreenPrivatePtr;

static inline SyncFdScreenPrivatePtr sync_fd_screen_priv(ScreenPtr pScreen)
{
    SyncFdScreenPrivatePtr      priv;

    if (!dixPrivateKeyRegistered(&syncFdScreenPrivateKey))
        return NULL;

    /* The private is preallocated on every screen as soon as the key is
     * registered by any single screen, so its mere presence says nothing.
     * Only screens that went through miSyncFdScreenInit() have funcs set.
     */
    priv = dixLookupPrivate(&pScreen->devPrivates, &syncFdScreenPrivateKey);
    if (priv->funcs.version <= 0)
        return NULL;

    return priv;
}

int
miSyncInitFenceFromFD(DrawablePtr pDraw, SyncFence *pFence, int fd, BOOL initially_triggered)

{
    SyncFdScreenPrivatePtr      priv = sync_fd_screen_priv(pDraw->pScreen);

    if (!priv)
        return BadMatch;

    return (*priv->funcs.CreateFenceFromFd)(pDraw->pScreen, pFence, fd, initially_triggered);
}

int
miSyncFDFromFence(DrawablePtr pDraw, SyncFence *pFence)
{
    SyncFdScreenPrivatePtr      priv = sync_fd_screen_priv(pDraw->pScreen);

    if (!priv)
        return -1;

    return (*priv->funcs.GetFenceFd)(pDraw->pScreen, pFence);
}

Bool miSyncFdScreenInit(ScreenPtr pScreen,
                                  const SyncFdScreenFuncsRec *funcs)
{
    SyncFdScreenPrivatePtr     priv;

    /* Check to see if we've already been initialized */
    priv = dixLookupPrivate(&pScreen->devPrivates, &syncFdScreenPrivateKey);
    if (priv && priv->funcs.version > 0)
        return FALSE;

    if (!miSyncSetup(pScreen))
        return FALSE;

    if (!dixPrivateKeyRegistered(&syncFdScreenPrivateKey)) {
        if (!dixRegisterPrivateKey(&syncFdScreenPrivateKey, PRIVATE_SCREEN, 0))
            return FALSE;
    }

    priv = dixLookupPrivate(&pScreen->devPrivates, &syncFdScreenPrivateKey);
    memset(priv, 0, sizeof(*priv));

    /* Will require version checks when there are multiple versions
     * of the funcs structure
     */

    priv->funcs = *funcs;

    dixSetPrivate(&pScreen->devPrivates, &syncFdScreenPrivateKey, priv);

    return TRUE;
}

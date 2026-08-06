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

#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <X11/xshmfence.h>

#include "os/busfault.h"
#include "os/osdep.h"

#include "os.h"
#include "scrnintstr.h"
#include "misync_priv.h"
#include "misyncstr.h"
#include "misyncshm.h"
#include "misyncfd.h"
#include "pixmapstr.h"

static DevPrivateKeyRec syncShmFencePrivateKey;

typedef struct _SyncShmFencePrivate {
    struct xshmfence    *fence;
    int                 fd;
#ifdef BUSFAULT
    struct busfault     *busfault;
#endif
} SyncShmFencePrivateRec, *SyncShmFencePrivatePtr;

#define SYNC_FENCE_PRIV(pFence) \
    (SyncShmFencePrivatePtr) dixLookupPrivate(&(pFence)->devPrivates, &syncShmFencePrivateKey)

#ifdef BUSFAULT

#include <setjmp.h>

#define SYNC_SHM_FENCE_ACCESS(pFence, pPriv, call)                      \
    do {                                                                \
        if ((pPriv)->fence && (pPriv)->busfault) {                      \
            sigjmp_buf jmp;                                             \
            if (sigsetjmp(jmp, 1) == 0) {                               \
                busfault_set_jmp((pPriv)->busfault, &jmp);              \
                call;                                                   \
                busfault_set_jmp((pPriv)->busfault, NULL);              \
            } else {                                                    \
                ErrorF("shared memory fence 0x%x truncated by client\n",\
                       (unsigned int) (pFence)->sync.id);               \
                miSyncShmFenceDiscard(pFence);                          \
            }                                                           \
        } else if ((pPriv)->fence) {                                    \
            call;                                                       \
        }                                                               \
    } while (0)


/* Cover the full mmap VMA; xshmfence maps a small object which the
 * kernel rounds up to a page. */
static unsigned int pagesize;

static void
initPageSize(void)
{
    if (!pagesize) {
#ifdef _SC_PAGESIZE
        pagesize = sysconf(_SC_PAGESIZE);
#else
        pagesize = getpagesize();
#endif
    }
}

static void
miSyncShmFenceDiscard(SyncFence *pFence)
{
    SyncShmFencePrivatePtr pPriv = SYNC_FENCE_PRIV(pFence);

    if (pPriv->busfault) {
        busfault_unregister(pPriv->busfault);
        pPriv->busfault = NULL;
    }
    if (pPriv->fence) {
        xshmfence_unmap_shm(pPriv->fence);
        pPriv->fence = NULL;
        close(pPriv->fd);
    }
}

static void
miSyncShmFenceBusfaultNotify(void *context)
{
    SyncFence *pFence = context;

    ErrorF("shared memory fence 0x%x truncated by client\n",
           (unsigned int) pFence->sync.id);
    /* Drop the mapping; the fence XID remains and further ops use
     * the in-server SyncFence state only. */
    miSyncShmFenceDiscard(pFence);
}

static void
miSyncShmFenceRegisterBusfault(SyncFence *pFence)
{
    SyncShmFencePrivatePtr pPriv = SYNC_FENCE_PRIV(pFence);

    pPriv->busfault = busfault_register_mmap(pPriv->fence, pagesize,
                                             miSyncShmFenceBusfaultNotify,
                                             pFence);
    if (!pPriv->busfault)
        ErrorF("shared memory fence 0x%x could not register for truncation detection\n",
               (unsigned int) pFence->sync.id);
}
#endif /* BUSFAULT */

static void
miSyncShmFenceSetTriggered(SyncFence * pFence)
{
    SyncShmFencePrivatePtr      pPriv = SYNC_FENCE_PRIV(pFence);

#ifdef BUSFAULT
    SYNC_SHM_FENCE_ACCESS(pFence, pPriv, xshmfence_trigger(pPriv->fence));
#else
    if (pPriv->fence)
        xshmfence_trigger(pPriv->fence);
#endif
    miSyncFenceSetTriggered(pFence);
}

static void
miSyncShmFenceReset(SyncFence * pFence)
{
    SyncShmFencePrivatePtr      pPriv = SYNC_FENCE_PRIV(pFence);

#ifdef BUSFAULT
    SYNC_SHM_FENCE_ACCESS(pFence, pPriv, xshmfence_reset(pPriv->fence));
#else
    if (pPriv->fence)
        xshmfence_reset(pPriv->fence);
#endif
    miSyncFenceReset(pFence);
}

static Bool
miSyncShmFenceCheckTriggered(SyncFence * pFence)
{
    SyncShmFencePrivatePtr      pPriv = SYNC_FENCE_PRIV(pFence);

#ifdef BUSFAULT
    if (pPriv->fence && pPriv->busfault) {
        sigjmp_buf jmp;
        if (sigsetjmp(jmp, 1) == 0) {
            Bool result;

            busfault_set_jmp(pPriv->busfault, &jmp);
            result = xshmfence_query(pPriv->fence);
            busfault_set_jmp(pPriv->busfault, NULL);
            return result;
        } else {
            ErrorF("shared memory fence 0x%x truncated by client\n",
                   (unsigned int) pFence->sync.id);
            miSyncShmFenceDiscard(pFence);
            return miSyncFenceCheckTriggered(pFence);
        }
    }
#endif
    if (pPriv->fence)
        return xshmfence_query(pPriv->fence);
    else
        return miSyncFenceCheckTriggered(pFence);
}

static void
miSyncShmFenceAddTrigger(SyncTrigger * pTrigger)
{
    miSyncFenceAddTrigger(pTrigger);
}

static void
miSyncShmFenceDeleteTrigger(SyncTrigger * pTrigger)
{
    miSyncFenceDeleteTrigger(pTrigger);
}

static const SyncFenceFuncsRec miSyncShmFenceFuncs = {
    &miSyncShmFenceSetTriggered,
    &miSyncShmFenceReset,
    &miSyncShmFenceCheckTriggered,
    &miSyncShmFenceAddTrigger,
    &miSyncShmFenceDeleteTrigger
};

static void
miSyncShmScreenCreateFence(ScreenPtr pScreen, SyncFence * pFence,
                        Bool initially_triggered)
{
    SyncShmFencePrivatePtr      pPriv = SYNC_FENCE_PRIV(pFence);

    pPriv->fence = NULL;
    miSyncScreenCreateFence(pScreen, pFence, initially_triggered);
    pFence->funcs = miSyncShmFenceFuncs;
}

static void
miSyncShmScreenDestroyFence(ScreenPtr pScreen, SyncFence * pFence)
{
    SyncShmFencePrivatePtr      pPriv = SYNC_FENCE_PRIV(pFence);

    if (pPriv->fence) {
#ifdef BUSFAULT
        /* Wake waiters if possible; SIGBUS discards via longjmp. */
        SYNC_SHM_FENCE_ACCESS(pFence, pPriv, xshmfence_trigger(pPriv->fence));
        miSyncShmFenceDiscard(pFence);
#else
        xshmfence_trigger(pPriv->fence);
        xshmfence_unmap_shm(pPriv->fence);
        close(pPriv->fd);
#endif
    }
    miSyncScreenDestroyFence(pScreen, pFence);
}

static int
miSyncShmCreateFenceFromFd(ScreenPtr pScreen, SyncFence *pFence, int fd, Bool initially_triggered)
{
    SyncShmFencePrivatePtr      pPriv = SYNC_FENCE_PRIV(pFence);

    miSyncInitFence(pScreen, pFence, initially_triggered);

    fd = os_move_fd(fd);
    pPriv->fence = xshmfence_map_shm(fd);
    if (pPriv->fence) {
#ifdef BUSFAULT
        miSyncShmFenceRegisterBusfault(pFence);
#endif
        pPriv->fd = fd;
        return Success;
    }
    else
        close(fd);
    return BadValue;
}

static int
miSyncShmGetFenceFd(ScreenPtr pScreen, SyncFence *pFence)
{
    SyncShmFencePrivatePtr      pPriv = SYNC_FENCE_PRIV(pFence);

    if (!pPriv->fence) {
        pPriv->fd = xshmfence_alloc_shm();
        if (pPriv->fd < 0)
            return -1;
        pPriv->fd = os_move_fd(pPriv->fd);
        pPriv->fence = xshmfence_map_shm(pPriv->fd);
        if (!pPriv->fence) {
            close (pPriv->fd);
            return -1;
        }
#ifdef BUSFAULT
        miSyncShmFenceRegisterBusfault(pFence);
#endif
    }
    return pPriv->fd;
}

static const SyncFdScreenFuncsRec miSyncShmScreenFuncs = {
    .version = SYNC_FD_SCREEN_FUNCS_VERSION,
    .CreateFenceFromFd = miSyncShmCreateFenceFromFd,
    .GetFenceFd = miSyncShmGetFenceFd
};

Bool miSyncShmScreenInit(ScreenPtr pScreen)
{
    SyncScreenFuncsPtr  funcs;

    if (!miSyncFdScreenInit(pScreen, &miSyncShmScreenFuncs))
        return FALSE;

    if (!dixPrivateKeyRegistered(&syncShmFencePrivateKey)) {
        if (!dixRegisterPrivateKey(&syncShmFencePrivateKey, PRIVATE_SYNC_FENCE,
                                   sizeof(SyncShmFencePrivateRec)))
            return FALSE;
    }

#ifdef BUSFAULT
    initPageSize();
#endif

    funcs = miSyncGetScreenFuncs(pScreen);

    funcs->CreateFence = miSyncShmScreenCreateFence;
    funcs->DestroyFence = miSyncShmScreenDestroyFence;

    return TRUE;
}


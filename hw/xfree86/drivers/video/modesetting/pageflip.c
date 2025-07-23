/*
 * Copyright © 2014 Intel Corporation
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

#include "dix-config.h"

#include <errno.h>

#include "os/xserver_poll.h"

#include <xf86drm.h>

#include "driver.h"

/*
 * Flush the DRM event queue when full; makes space for new events.
 *
 * Returns a negative value on error, 0 if there was nothing to process,
 * or 1 if we handled any events.
 */
static int
ms_flush_drm_events_timeout(ScreenPtr screen, int timeout)
{
    ScrnInfoPtr scrn = xf86ScreenToScrn(screen);
    modesettingPtr ms = modesettingPTR(scrn);

    struct pollfd p = { .fd = ms->fd, .events = POLLIN };
    int r;

    do {
            r = xserver_poll(&p, 1, timeout);
    } while (r == -1 && (errno == EINTR || errno == EAGAIN));

    /* If there was an error, r will be < 0.  Return that.  If there was
     * nothing to process, r == 0.  Return that.
     */
    if (r <= 0)
        return r;

    /* Try to handle the event.  If there was an error, return it. */
    r = drmHandleEvent(ms->fd, &ms->event_context);
    if (r < 0)
        return r;

    /* Otherwise return 1 to indicate that we handled an event. */
    return 1;
}

int
ms_flush_drm_events(ScreenPtr screen)
{
    return ms_flush_drm_events_timeout(screen, 0);
}

void
ms_drain_drm_events(ScreenPtr screen)
{
    while (!ms_drm_queue_is_empty())
        ms_flush_drm_events_timeout(screen, -1);
}

#ifdef GLAMOR_HAS_GBM

/*
 * Event data for an in progress flip.
 * This contains a pointer to the vblank event,
 * and information about the flip in progress.
 * a reference to this is stored in the per-crtc
 * flips.
 */
struct ms_flipdata {
    ScreenPtr screen;
    void *event;
    ms_pageflip_handler_proc event_handler;
    ms_pageflip_abort_proc abort_handler;
    /* number of CRTC events referencing this */
    int flip_count;
    uint64_t fe_msc;
    uint64_t fe_usec;
    uint32_t old_fb_id;
};

/*
 * Per crtc pageflipping information,
 * These are submitted to the queuing code
 * one of them per crtc per flip.
 */
struct ms_crtc_pageflip {
    Bool on_reference_crtc;
    /* reference to the ms_flipdata */
    struct ms_flipdata *flipdata;
    struct xorg_list node;
    uint32_t tearfree_seq;
};

/**
 * Free an ms_crtc_pageflip.
 *
 * Drops the reference count on the flipdata.
 */
static void
ms_pageflip_free(struct ms_crtc_pageflip *flip)
{
    struct ms_flipdata *flipdata = flip->flipdata;

    free(flip);
    if (--flipdata->flip_count > 0)
        return;
    free(flipdata);
}

/**
 * Callback for the DRM event queue when a single flip has completed
 *
 * Once the flip has been completed on all pipes, notify the
 * extension code telling it when that happened
 */
static void
ms_pageflip_handler(uint64_t msc, uint64_t ust, void *data)
{
    struct ms_crtc_pageflip *flip = data;
    struct ms_flipdata *flipdata = flip->flipdata;
    ScreenPtr screen = flipdata->screen;
    ScrnInfoPtr scrn = xf86ScreenToScrn(screen);
    modesettingPtr ms = modesettingPTR(scrn);

    if (flip->on_reference_crtc) {
        flipdata->fe_msc = msc;
        flipdata->fe_usec = ust;
    }

    if (flipdata->flip_count == 1) {
        flipdata->event_handler(ms, flipdata->fe_msc,
                                flipdata->fe_usec,
                                flipdata->event);

        if (flipdata->old_fb_id)
            drmModeRmFB(ms->fd, flipdata->old_fb_id);
    }
    ms_pageflip_free(flip);
}

/*
 * Callback for the DRM queue abort code.  A flip has been aborted.
 */
static void
ms_pageflip_abort(void *data)
{
    struct ms_crtc_pageflip *flip = data;
    struct ms_flipdata *flipdata = flip->flipdata;
    ScreenPtr screen = flipdata->screen;
    ScrnInfoPtr scrn = xf86ScreenToScrn(screen);
    modesettingPtr ms = modesettingPTR(scrn);

    if (flipdata->flip_count == 1)
        flipdata->abort_handler(ms, flipdata->event);

    ms_pageflip_free(flip);
}

static Bool
do_queue_flip_on_crtc(ScreenPtr screen, xf86CrtcPtr crtc, uint32_t flags,
                      uint32_t seq, uint32_t fb_id, int x, int y)
{
    drmmode_crtc_private_ptr drmmode_crtc = crtc->driver_private;
    drmmode_tearfree_ptr trf = &drmmode_crtc->tearfree;

    while (drmmode_crtc_flip(crtc, fb_id, x, y, flags, (void *)(long)seq)) {
        /* We may have failed because the event queue was full.  Flush it
         * and retry.  If there was nothing to flush, then we failed for
         * some other reason and should just return an error.
         */
        if (ms_flush_drm_events(screen) <= 0) {
            /* The failure could be caused by a pending TearFree flip, in which
             * case we should wait until there's a new event and try again.
             */
            if (!trf->flip_seq || ms_flush_drm_events_timeout(screen, -1) < 0) {
                ms_drm_abort_seq(crtc->scrn, seq);
                return TRUE;
            }
        }

        /* We flushed some events, so try again. */
        xf86DrvMsg(crtc->scrn->scrnIndex, X_WARNING, "flip queue retry\n");
    }

    return FALSE;
}

enum queue_flip_status {
    QUEUE_FLIP_SUCCESS,
    QUEUE_FLIP_ALLOC_FAILED,
    QUEUE_FLIP_QUEUE_ALLOC_FAILED,
    QUEUE_FLIP_DRM_FLUSH_FAILED,
};

static int
queue_flip_on_crtc(ScreenPtr screen, xf86CrtcPtr crtc,
                   struct ms_flipdata *flipdata,
                   xf86CrtcPtr ref_crtc, uint32_t flags)
{
    ScrnInfoPtr scrn = xf86ScreenToScrn(screen);
    modesettingPtr ms = modesettingPTR(scrn);
    struct ms_crtc_pageflip *flip;
    uint32_t seq;

    flip = calloc(1, sizeof(struct ms_crtc_pageflip));
    if (flip == NULL) {
        return QUEUE_FLIP_ALLOC_FAILED;
    }

    /* Only the reference crtc will finally deliver its page flip
     * completion event. All other crtc's events will be discarded.
     */
    flip->on_reference_crtc = crtc == ref_crtc;
    flip->flipdata = flipdata;

    seq = ms_drm_queue_alloc(crtc, flip, ms_pageflip_handler, ms_pageflip_abort);
    if (!seq) {
        free(flip);
        return QUEUE_FLIP_QUEUE_ALLOC_FAILED;
    }

    /* take a reference on flipdata for use in flip */
    flipdata->flip_count++;

    if (do_queue_flip_on_crtc(screen, crtc, flags, seq, ms->drmmode.fb_id,
                              crtc->x, crtc->y))
        return QUEUE_FLIP_DRM_FLUSH_FAILED;

    /* The page flip succeeded. */
    return QUEUE_FLIP_SUCCESS;
}


#define MS_ASYNC_FLIP_LOG_ENABLE_LOGS_INTERVAL_MS 10000
#define MS_ASYNC_FLIP_LOG_FREQUENT_LOGS_INTERVAL_MS 1000
#define MS_ASYNC_FLIP_FREQUENT_LOG_COUNT 10

static void
ms_print_pageflip_error(int screen_index, const char *log_prefix,
                        int crtc_index, int flags, int err)
{
    /* In certain circumstances we will have a lot of flip errors without a
     * reasonable way to prevent them. In such case we reduce the number of
     * logged messages to at least not fill the error logs.
     *
     * The details are as follows:
     *
     * At least on i915 hardware support for async page flip support depends
     * on the used modifiers which themselves can change dynamically for a
     * screen. This results in the following problems:
     *
     *  - We can't know about whether a particular CRTC will be able to do an
     *    async flip without hardcoding the same logic as the kernel as there's
     *    no interface to query this information.
     *
     *  - There is no way to give this information to an application, because
     *    the protocol of the present extension does not specify anything about
     *    changing of the capabilities on runtime or the need to re-query them.
     *
     * Even if the above was solved, the only benefit would be avoiding a
     * roundtrip to the kernel and reduced amount of error logs. The former
     * does not seem to be a good enough benefit compared to the amount of work
     * that would need to be done. The latter is solved below. */

    static CARD32 error_last_time_ms;
    static int frequent_logs;
    static Bool logs_disabled;

    if (flags & DRM_MODE_PAGE_FLIP_ASYNC) {
        CARD32 curr_time_ms = GetTimeInMillis();
        int clocks_since_last_log = curr_time_ms - error_last_time_ms;

        if (clocks_since_last_log >
                MS_ASYNC_FLIP_LOG_ENABLE_LOGS_INTERVAL_MS) {
            frequent_logs = 0;
            logs_disabled = FALSE;
        }
        if (!logs_disabled) {
            if (clocks_since_last_log <
                    MS_ASYNC_FLIP_LOG_FREQUENT_LOGS_INTERVAL_MS) {
                frequent_logs++;
            }

            if (frequent_logs > MS_ASYNC_FLIP_FREQUENT_LOG_COUNT) {
                xf86DrvMsg(screen_index, X_WARNING,
                           "%s: detected too frequent flip errors, disabling "
                           "logs until frequency is reduced\n", log_prefix);
                logs_disabled = TRUE;
            } else {
                xf86DrvMsg(screen_index, X_WARNING,
                           "%s: queue async flip during flip on CRTC %d failed: %s\n",
                           log_prefix, crtc_index, strerror(err));
            }
        }
        error_last_time_ms = curr_time_ms;
    } else {
        xf86DrvMsg(screen_index, X_WARNING,
                   "%s: queue flip during flip on CRTC %d failed: %s\n",
                   log_prefix, crtc_index, strerror(err));
    }
}

static Bool
ms_tearfree_dri_flip(modesettingPtr ms, xf86CrtcPtr crtc, void *event,
                     ms_pageflip_handler_proc pageflip_handler,
                     ms_pageflip_abort_proc pageflip_abort)
{
    drmmode_crtc_private_ptr drmmode_crtc = crtc->driver_private;
    drmmode_tearfree_ptr trf = &drmmode_crtc->tearfree;
    struct ms_crtc_pageflip *flip;
    struct ms_flipdata *flipdata;
    RegionRec region;
    RegionPtr dirty;

    if (!ms_tearfree_is_active_on_crtc(crtc))
        return FALSE;

    /* Check for damage on the primary scanout to know if TearFree will flip */
    dirty = DamageRegion(ms->damage);
    if (RegionNil(dirty))
        return FALSE;

    /* Compute how much of the current damage intersects with this CRTC */
    RegionInit(&region, &crtc->bounds, 0);
    RegionIntersect(&region, &region, dirty);

    /* No damage on this CRTC means no TearFree flip. This means the DRI client
     * didn't change this CRTC's contents at all with its presentation, possibly
     * because its window is fully occluded by another window on this CRTC.
     */
    if (RegionNil(&region))
        return FALSE;

    flip = calloc(1, sizeof(*flip));
    if (!flip)
        return FALSE;

    flipdata = calloc(1, sizeof(*flipdata));
    if (!flipdata) {
        free(flip);
        return FALSE;
    }

    /* Only track the DRI client's fake flip on the reference CRTC, which aligns
     * with the behavior of Present when a client copies its pixmap rather than
     * directly flipping it onto the display.
     */
    flip->on_reference_crtc = TRUE;
    flip->flipdata = flipdata;
    flip->tearfree_seq = trf->flip_seq;
    flipdata->screen = xf86ScrnToScreen(crtc->scrn);
    flipdata->event = event;
    flipdata->flip_count = 1;
    flipdata->event_handler = pageflip_handler;
    flipdata->abort_handler = pageflip_abort;

    /* Keep the list in FIFO order so that clients are notified in order */
    xorg_list_append(&flip->node, &trf->dri_flip_list);
    return TRUE;
}

Bool
ms_do_pageflip(ScreenPtr screen,
               PixmapPtr new_front,
               void *event,
               xf86CrtcPtr ref_crtc,
               Bool async,
               ms_pageflip_handler_proc pageflip_handler,
               ms_pageflip_abort_proc pageflip_abort,
               const char *log_prefix)
{
    ScrnInfoPtr scrn = xf86ScreenToScrn(screen);
    modesettingPtr ms = modesettingPTR(scrn);
    xf86CrtcConfigPtr config = XF86_CRTC_CONFIG_PTR(scrn);
    drmmode_bo new_front_bo;
    uint32_t flags;
    int i;
    struct ms_flipdata *flipdata;

    /* A NULL pixmap indicates this DRI client's pixmap is to be flipped through
     * TearFree instead. The pixmap is already copied to the primary scanout at
     * this point, so all that's left is to wire up this fake flip to TearFree
     * so that TearFree can send a notification to the DRI client when the
     * pixmap actually appears on the display. This is the only way to let DRI
     * clients accurately know when their pixmaps appear on the display when
     * TearFree is enabled.
     */
    if (!new_front) {
        if (!ms_tearfree_dri_flip(ms, ref_crtc, event, pageflip_handler,
                                  pageflip_abort))
            goto error_free_event;
        return TRUE;
    }

    ms->glamor.block_handler(screen);

    new_front_bo.gbm = ms->glamor.gbm_bo_from_pixmap(screen, new_front);
    new_front_bo.dumb = NULL;

    if (!new_front_bo.gbm) {
        xf86DrvMsg(scrn->scrnIndex, X_ERROR,
                   "%s: Failed to get GBM BO for flip to new front.\n",
                   log_prefix);
        goto error_free_event;
    }

    flipdata = calloc(1, sizeof(struct ms_flipdata));
    if (!flipdata) {
        drmmode_bo_destroy(&ms->drmmode, &new_front_bo);
        xf86DrvMsg(scrn->scrnIndex, X_ERROR,
                   "%s: Failed to allocate flipdata.\n", log_prefix);
        goto error_free_event;
    }

    flipdata->event = event;
    flipdata->screen = screen;
    flipdata->event_handler = pageflip_handler;
    flipdata->abort_handler = pageflip_abort;

    /*
     * Take a local reference on flipdata.
     * if the first flip fails, the sequence abort
     * code will free the crtc flip data, and drop
     * its reference which would cause this to be
     * freed when we still required it.
     */
    flipdata->flip_count++;

    /* Create a new handle for the back buffer */
    flipdata->old_fb_id = ms->drmmode.fb_id;

    new_front_bo.width = new_front->drawable.width;
    new_front_bo.height = new_front->drawable.height;
    if (drmmode_bo_import(&ms->drmmode, &new_front_bo,
                          &ms->drmmode.fb_id)) {
        if (!ms->drmmode.flip_bo_import_failed) {
            xf86DrvMsg(scrn->scrnIndex, X_WARNING, "%s: Import BO failed: %s\n",
                       log_prefix, strerror(errno));
            ms->drmmode.flip_bo_import_failed = TRUE;
        }
        goto error_out;
    } else {
        if (ms->drmmode.flip_bo_import_failed &&
            new_front != screen->GetScreenPixmap(screen))
            ms->drmmode.flip_bo_import_failed = FALSE;
    }

    /* Queue flips on all enabled CRTCs.
     *
     * Note that if/when we get per-CRTC buffers, we'll have to update this.
     * Right now it assumes a single shared fb across all CRTCs, with the
     * kernel fixing up the offset of each CRTC as necessary.
     *
     * Also, flips queued on disabled or incorrectly configured displays
     * may never complete; this is a configuration error.
     */
    for (i = 0; i < config->num_crtc; i++) {
        enum queue_flip_status flip_status;
        xf86CrtcPtr crtc = config->crtc[i];

        if (!xf86_crtc_on(crtc))
            continue;

        flags = DRM_MODE_PAGE_FLIP_EVENT;
        if (ms->drmmode.can_async_flip && async)
            flags |= DRM_MODE_PAGE_FLIP_ASYNC;

        /*
         * If this is not the reference crtc used for flip timing and flip event
         * delivery and timestamping, ie. not the one whose presentation timing
         * we do really care about, and async flips are possible, and requested
         * by an xorg.conf option, then we flip this "secondary" crtc without
         * sync to vblank. This may cause tearing on such "secondary" outputs,
         * but it will prevent throttling of multi-display flips to the refresh
         * cycle of any of the secondary crtcs, avoiding periodic slowdowns and
         * judder caused by unsynchronized outputs. This is especially useful for
         * outputs in a "clone-mode" or "mirror-mode" configuration.
         */
        if (ms->drmmode.can_async_flip && ms->drmmode.async_flip_secondaries &&
            ref_crtc && crtc != ref_crtc)
            flags |= DRM_MODE_PAGE_FLIP_ASYNC;

        flip_status = queue_flip_on_crtc(screen, crtc, flipdata,
                                         ref_crtc, flags);

        switch (flip_status) {
            case QUEUE_FLIP_ALLOC_FAILED:
                xf86DrvMsg(scrn->scrnIndex, X_WARNING,
                           "%s: carrier alloc for queue flip on CRTC %d failed.\n",
                           log_prefix, i);
                goto error_undo;
            case QUEUE_FLIP_QUEUE_ALLOC_FAILED:
                xf86DrvMsg(scrn->scrnIndex, X_WARNING,
                           "%s: entry alloc for queue flip on CRTC %d failed.\n",
                           log_prefix, i);
                goto error_undo;
            case QUEUE_FLIP_DRM_FLUSH_FAILED:
                ms_print_pageflip_error(scrn->scrnIndex, log_prefix, i, flags, errno);
                goto error_undo;
            case QUEUE_FLIP_SUCCESS:
                break;
        }
    }

    drmmode_bo_destroy(&ms->drmmode, &new_front_bo);

    /*
     * Do we have more than our local reference,
     * if so and no errors, then drop our local
     * reference and return now.
     */
    if (flipdata->flip_count > 1) {
        flipdata->flip_count--;
        return TRUE;
    }

error_undo:

    /*
     * Have we just got the local reference?
     * free the framebuffer if so since nobody successfully
     * submitted anything
     */
    if (flipdata->flip_count == 1) {
        drmModeRmFB(ms->fd, ms->drmmode.fb_id);
        ms->drmmode.fb_id = flipdata->old_fb_id;
    }

error_out:
    drmmode_bo_destroy(&ms->drmmode, &new_front_bo);
    /* if only the local reference - free the structure,
     * else drop the local reference and return */
    if (flipdata->flip_count == 1) {
        free(flipdata);
    } else {
        flipdata->flip_count--;
        return FALSE;
    }

error_free_event:
    /* Free the event since the caller has no way to know it's safe to free */
    free(event);
    return FALSE;
}

Bool
ms_tearfree_dri_abort(xf86CrtcPtr crtc,
                      Bool (*match)(void *data, void *match_data),
                      void *match_data)
{
    drmmode_crtc_private_ptr drmmode_crtc = crtc->driver_private;
    drmmode_tearfree_ptr trf = &drmmode_crtc->tearfree;
    struct ms_crtc_pageflip *flip;

    /* The window is getting destroyed; abort without notifying the client */
    xorg_list_for_each_entry(flip, &trf->dri_flip_list, node) {
        if (match(flip->flipdata->event, match_data)) {
            xorg_list_del(&flip->node);
            ms_pageflip_abort(flip);
            return TRUE;
        }
    }

    return FALSE;
}

void
ms_tearfree_dri_abort_all(xf86CrtcPtr crtc)
{
    drmmode_crtc_private_ptr drmmode_crtc = crtc->driver_private;
    drmmode_tearfree_ptr trf = &drmmode_crtc->tearfree;
    struct ms_crtc_pageflip *flip, *tmp;
    uint64_t usec = 0, msc = 0;

    /* Nothing to abort if there aren't any DRI clients waiting for a flip */
    if (xorg_list_is_empty(&trf->dri_flip_list))
        return;

    /* Even though we're aborting, these clients' pixmaps were actually blitted,
     * so technically the presentation isn't aborted. That's why the normal
     * handler is called instead of the abort handler, along with the current
     * time and MSC for this CRTC.
     */
    ms_get_crtc_ust_msc(crtc, &usec, &msc);
    xorg_list_for_each_entry_safe(flip, tmp, &trf->dri_flip_list, node)
        ms_pageflip_handler(msc, usec, flip);
    xorg_list_init(&trf->dri_flip_list);
}

static void
ms_tearfree_dri_notify(drmmode_tearfree_ptr trf, uint64_t msc, uint64_t usec)
{
    struct ms_crtc_pageflip *flip, *tmp;

    xorg_list_for_each_entry_safe(flip, tmp, &trf->dri_flip_list, node) {
        /* If a TearFree flip was already pending at the time this DRI client's
         * pixmap was copied, then the pixmap isn't contained in this TearFree
         * flip, but will be part of the next TearFree flip instead.
         */
        if (flip->tearfree_seq) {
            flip->tearfree_seq = 0;
        } else {
            xorg_list_del(&flip->node);
            ms_pageflip_handler(msc, usec, flip);
        }
    }
}

static void
ms_tearfree_flip_abort(void *data)
{
    xf86CrtcPtr crtc = data;
    drmmode_crtc_private_ptr drmmode_crtc = crtc->driver_private;
    drmmode_tearfree_ptr trf = &drmmode_crtc->tearfree;

    trf->flip_seq = 0;
    ms_tearfree_dri_abort_all(crtc);
}

static void
ms_tearfree_flip_handler(uint64_t msc, uint64_t usec, void *data)
{
    xf86CrtcPtr crtc = data;
    drmmode_crtc_private_ptr drmmode_crtc = crtc->driver_private;
    drmmode_tearfree_ptr trf = &drmmode_crtc->tearfree;

    /* Swap the buffers and complete the flip */
    trf->back_idx ^= 1;
    trf->flip_seq = 0;

    /* Notify DRI clients that their pixmaps are now visible on the display */
    ms_tearfree_dri_notify(trf, msc, usec);
}

Bool
ms_do_tearfree_flip(ScreenPtr screen, xf86CrtcPtr crtc)
{
    drmmode_crtc_private_ptr drmmode_crtc = crtc->driver_private;
    drmmode_tearfree_ptr trf = &drmmode_crtc->tearfree;
    uint32_t idx = trf->back_idx, seq;

    seq = ms_drm_queue_alloc(crtc, crtc, ms_tearfree_flip_handler,
                             ms_tearfree_flip_abort);
    if (!seq) {
        /* Need to notify the DRI clients if a sequence wasn't allocated. Once a
         * sequence is allocated, explicitly performing this cleanup isn't
         * necessary since it's already done as part of aborting the sequence.
         */
        ms_tearfree_dri_abort_all(crtc);
        goto no_flip;
    }

    /* Copy the damage to the back buffer and then flip it at the vblank */
    drmmode_copy_damage(crtc, trf->buf[idx].px, &trf->buf[idx].dmg, TRUE);
    if (do_queue_flip_on_crtc(screen, crtc, DRM_MODE_PAGE_FLIP_EVENT,
                              seq, trf->buf[idx].fb_id, 0, 0))
        goto no_flip;

    trf->flip_seq = seq;
    return FALSE;

no_flip:
    xf86DrvMsg(crtc->scrn->scrnIndex, X_WARNING,
               "TearFree flip failed, rendering frame without TearFree\n");
    drmmode_copy_damage(crtc, trf->buf[idx ^ 1].px,
                        &trf->buf[idx ^ 1].dmg, FALSE);
    return TRUE;
}
#endif

Bool
ms_tearfree_is_active_on_crtc(xf86CrtcPtr crtc)
{
    drmmode_crtc_private_ptr drmmode_crtc = crtc->driver_private;
    drmmode_tearfree_ptr trf = &drmmode_crtc->tearfree;

    /* If TearFree is enabled, XServer owns the VT, and the CRTC is active */
    return trf->buf[0].px && crtc->scrn->vtSema && xf86_crtc_on(crtc);
}

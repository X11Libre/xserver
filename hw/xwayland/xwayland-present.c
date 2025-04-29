/*
 * Copyright © 2018 Roman Gilg
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without
 * fee, provided that the above copyright notice appear in all copies
 * and that both that copyright notice and this permission notice
 * appear in supporting documentation, and that the name of the
 * copyright holders not be used in advertising or publicity
 * pertaining to distribution of the software without specific,
 * written prior permission.  The copyright holders make no
 * representations about the suitability of this software for any
 * purpose.  It is provided "as is" without express or implied
 * warranty.
 *
 * THE COPYRIGHT HOLDERS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS
 * SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS, IN NO EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
 * AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING
 * OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 */

#include <xwayland-config.h>

#include "os/bug_priv.h"

#include <compint.h>
#ifdef XWL_HAS_GLAMOR
#include <glamor.h>
#endif
#include <windowstr.h>
#include <present.h>
#ifdef DRI3
#include <sys/eventfd.h>
#endif /* DRI3 */

#include "xwayland-present.h"
#include "xwayland-screen.h"
#include "xwayland-shm.h"
#include "xwayland-window.h"
#include "xwayland-window-buffers.h"
#include "xwayland-pixmap.h"

#include "tearing-control-v1-client-protocol.h"
#include "linux-drm-syncobj-v1-client-protocol.h"

#define XWL_PRESENT_CAPS PresentCapabilityAsync | PresentCapabilityAsyncMayTear


/*
 * When not flipping let Present copy with 60fps.
 * When flipping wait on frame_callback, otherwise
 * the surface is not visible, in this case update
 * with long interval.
 */
#define TIMER_LEN_COPY      17  // ~60fps
#define TIMER_LEN_FLIP    1000  // 1fps

static DevPrivateKeyRec xwl_present_window_private_key;

static struct xwl_present_window *
xwl_present_window_priv(WindowPtr window)
{
    return dixGetPrivate(&window->devPrivates,
                         &xwl_present_window_private_key);
}

static struct xwl_present_window *
xwl_present_window_get_priv(WindowPtr window)
{
    struct xwl_present_window *xwl_present_window = xwl_present_window_priv(window);

    if (xwl_present_window == NULL) {
        xwl_present_window = calloc (1, sizeof (struct xwl_present_window));
        if (!xwl_present_window)
            return NULL;

        xwl_present_window->window = window;
        xwl_present_window->msc = 1;
        xwl_present_window->ust = GetTimeInMicros();

        xorg_list_init(&xwl_present_window->frame_callback_list);
        xorg_list_init(&xwl_present_window->wait_list);
        xorg_list_init(&xwl_present_window->flip_queue);
        xorg_list_init(&xwl_present_window->idle_queue);
        xorg_list_init(&xwl_present_window->blocked_queue);

        dixSetPrivate(&window->devPrivates,
                      &xwl_present_window_private_key,
                      xwl_present_window);
    }

    return xwl_present_window;
}

static struct xwl_present_event *
xwl_present_event_from_id(WindowPtr present_window, uint64_t event_id)
{
    present_window_priv_ptr window_priv = present_get_window_priv(present_window, TRUE);
    struct xwl_present_event *event;

    xorg_list_for_each_entry(event, &window_priv->vblank, vblank.window_list) {
        if (event->vblank.event_id == event_id)
            return event;
    }
    return NULL;
}

static struct xwl_present_event *
xwl_present_event_from_vblank(present_vblank_ptr vblank)
{
    return container_of(vblank, struct xwl_present_event, vblank);
}

static Bool entered_for_each_frame_callback;

Bool
xwl_present_entered_for_each_frame_callback(void)
{
    return entered_for_each_frame_callback;
}

void
xwl_present_for_each_frame_callback(struct xwl_window *xwl_window,
                                    void iter_func(struct xwl_present_window *))
{
    struct xwl_present_window *xwl_present_window, *tmp;

    if (entered_for_each_frame_callback)
        FatalError("Nested xwl_present_for_each_frame_callback call");

    entered_for_each_frame_callback = TRUE;

    xorg_list_for_each_entry_safe(xwl_present_window, tmp,
                                  &xwl_window->frame_callback_list,
                                  frame_callback_list)
        iter_func(xwl_present_window);

    entered_for_each_frame_callback = FALSE;
}

static void
xwl_present_free_timer(struct xwl_present_window *xwl_present_window)
{
    TimerFree(xwl_present_window->frame_timer);
    xwl_present_window->frame_timer = NULL;
    xwl_present_window->timer_armed = 0;
}

static CARD32
xwl_present_timer_callback(OsTimerPtr timer,
                           CARD32 time,
                           void *arg);

static present_vblank_ptr
xwl_present_get_pending_flip(struct xwl_present_window *xwl_present_window)
{
    present_vblank_ptr flip_pending;

    if (xorg_list_is_empty(&xwl_present_window->flip_queue))
        return NULL;

    flip_pending = xorg_list_first_entry(&xwl_present_window->flip_queue, present_vblank_rec,
                                         event_queue);

    if (flip_pending->queued)
        return NULL;

    return flip_pending;
}

static inline Bool
xwl_present_has_pending_events(struct xwl_present_window *xwl_present_window)
{
    present_vblank_ptr flip_pending = xwl_present_get_pending_flip(xwl_present_window);

    return (flip_pending && flip_pending->sync_flip) ||
           !xorg_list_is_empty(&xwl_present_window->wait_list) ||
           !xorg_list_is_empty(&xwl_present_window->blocked_queue);
}

void
xwl_present_reset_timer(struct xwl_present_window *xwl_present_window)
{
    if (xwl_present_has_pending_events(xwl_present_window)) {
        struct xwl_window *xwl_window = xwl_window_from_window(xwl_present_window->window);
        CARD32 now = GetTimeInMillis();
        CARD32 timeout;

        if (xwl_window && xwl_window->frame_callback &&
            !xorg_list_is_empty(&xwl_present_window->frame_callback_list))
            timeout = TIMER_LEN_FLIP;
        else
            timeout = TIMER_LEN_COPY;

        /* Make sure the timer callback runs if at least a second has passed
         * since we first armed the timer. This can happen e.g. if the Wayland
         * compositor doesn't send a pending frame event, e.g. because the
         * Wayland surface isn't visible anywhere.
         */
        if (xwl_present_window->timer_armed) {
            if ((int)(now - xwl_present_window->timer_armed) > 1000) {
                xwl_present_timer_callback(xwl_present_window->frame_timer, now,
                                           xwl_present_window);
                return;
            }
        } else {
            xwl_present_window->timer_armed = now;
        }

        xwl_present_window->frame_timer = TimerSet(xwl_present_window->frame_timer,
                                                   0, timeout,
                                                   &xwl_present_timer_callback,
                                                   xwl_present_window);
    } else {
        xwl_present_free_timer(xwl_present_window);
    }
}


static void
xwl_present_execute(present_vblank_ptr vblank, uint64_t ust, uint64_t crtc_msc);

static int
xwl_present_queue_vblank(ScreenPtr screen,
                         WindowPtr present_window,
                         RRCrtcPtr crtc,
                         uint64_t event_id,
                         uint64_t msc);

static uint32_t
xwl_present_query_capabilities(present_screen_priv_ptr screen_priv)
{
    struct xwl_screen *xwl_screen = xwl_screen_get(screen_priv->pScreen);
    return xwl_screen->present_capabilities;
}

static int
xwl_present_get_ust_msc(ScreenPtr screen,
                        WindowPtr present_window,
                        uint64_t *ust,
                        uint64_t *msc)
{
    struct xwl_present_window *xwl_present_window = xwl_present_window_get_priv(present_window);
    if (!xwl_present_window)
        return BadAlloc;

    *ust = xwl_present_window->ust;
    *msc = xwl_present_window->msc;

    return Success;
}

static uint64_t
xwl_present_get_exec_msc(uint32_t options, uint64_t target_msc)
{
    /* Synchronous Xwayland presentations always complete (at least) one frame after they
     * are executed
     */
    if (options & PresentOptionAsyncMayTear)
        return target_msc;

    return target_msc - 1;
}

/*
 * When the wait fence or previous flip is completed, it's time
 * to re-try the request
 */
static void
xwl_present_re_execute(present_vblank_ptr vblank)
{
    struct xwl_present_event *event = xwl_present_event_from_vblank(vblank);
    uint64_t ust = 0, crtc_msc = 0;

    (void) xwl_present_get_ust_msc(vblank->screen, vblank->window, &ust, &crtc_msc);
    /* re-compute target / exec msc */
    vblank->target_msc = present_get_target_msc(0, crtc_msc,
                                                event->divisor,
                                                event->remainder,
                                                event->options);
    vblank->exec_msc = xwl_present_get_exec_msc(event->options,
                                                vblank->target_msc);

    vblank->queued = TRUE;
    if (msc_is_after(vblank->exec_msc, crtc_msc) &&
        xwl_present_queue_vblank(vblank->screen, vblank->window,
                                 vblank->crtc,
                                 vblank->event_id,
                                 vblank->exec_msc) == Success) {
        return;
    }

    xwl_present_execute(vblank, ust, crtc_msc);
}

static void
xwl_present_flip_try_ready(struct xwl_present_window *xwl_present_window)
{
    present_vblank_ptr vblank;

    xorg_list_for_each_entry(vblank, &xwl_present_window->flip_queue, event_queue) {
        if (vblank->queued) {
            xwl_present_re_execute(vblank);
            return;
        }
    }
}

static void
xwl_present_release_pixmap(struct xwl_present_event *event)
{
    if (!event->pixmap)
        return;

    xwl_pixmap_del_buffer_release_cb(event->pixmap);
    dixDestroyPixmap(event->pixmap, event->pixmap->drawable.id);
    event->pixmap = NULL;
}

static void
xwl_present_free_event(struct xwl_present_event *event)
{
    xwl_present_release_pixmap(event);
    xorg_list_del(&event->vblank.event_queue);
    present_vblank_destroy(&event->vblank);
}

static void
xwl_present_free_idle_vblank(present_vblank_ptr vblank)
{
#if defined(XWL_HAS_GLAMOR) && defined(DRI3)
    if (vblank->release_syncobj) {
        /* transfer implicit fence to release syncobj */
        int fence_fd = xwl_glamor_dmabuf_export_sync_file(vblank->pixmap);
        vblank->release_syncobj->import_fence(vblank->release_syncobj,
                                              vblank->release_point,
                                              fence_fd);
    } else
#endif /* defined(XWL_HAS_GLAMOR) && defined(DRI3) */
        present_pixmap_idle(vblank->pixmap, vblank->window,
                            vblank->serial, vblank->idle_fence);
    xwl_present_free_event(xwl_present_event_from_vblank(vblank));
}

static WindowPtr
xwl_present_toplvl_pixmap_window(WindowPtr window)
{
    ScreenPtr       screen = window->drawable.pScreen;
    PixmapPtr       pixmap = (*screen->GetWindowPixmap)(window);
    WindowPtr       w = window;
    WindowPtr       next_w;

    while(w->parent) {
        next_w = w->parent;
        if ( (*screen->GetWindowPixmap)(next_w) != pixmap) {
            break;
        }
        w = next_w;
    }
    return w;
}

static void
xwl_present_flips_stop(WindowPtr window)
{
    struct xwl_present_window *xwl_present_window = xwl_present_window_priv(window);
    present_vblank_ptr vblank, tmp;

    /* Change back to the fast refresh rate */
    xwl_present_reset_timer(xwl_present_window);

    /* Free any left over idle vblanks */
    xorg_list_for_each_entry_safe(vblank, tmp, &xwl_present_window->idle_queue, event_queue)
        xwl_present_free_idle_vblank(vblank);

    if (xwl_present_window->flip_active) {
        struct xwl_present_event *event;

        vblank = xwl_present_window->flip_active;
        event = xwl_present_event_from_vblank(vblank);
        if (event->pixmap)
            xwl_present_free_idle_vblank(vblank);
        else
            xwl_present_free_event(event);

        xwl_present_window->flip_active = NULL;
    }

    xwl_present_flip_try_ready(xwl_present_window);
}

static void
xwl_present_flip_notify_vblank(present_vblank_ptr vblank, uint64_t ust, uint64_t crtc_msc)
{
    WindowPtr                   window = vblank->window;
#ifdef DRI3
    struct xwl_screen *xwl_screen = xwl_screen_get(window->drawable.pScreen);
#endif /* DRI3 */
    struct xwl_present_window *xwl_present_window = xwl_present_window_priv(window);
    uint8_t mode = PresentCompleteModeFlip;

    DebugPresent(("\tn %" PRIu64 " %p %" PRIu64 " %" PRIu64 ": %08" PRIx32 " -> %08" PRIx32 "\n",
                  vblank->event_id, vblank, vblank->exec_msc, vblank->target_msc,
                  vblank->pixmap ? vblank->pixmap->drawable.id : 0,
                  vblank->window ? vblank->window->drawable.id : 0));

    assert (&vblank->event_queue == xwl_present_window->flip_queue.next);

    xorg_list_del(&vblank->event_queue);

    if (xwl_present_window->flip_active) {
        struct xwl_present_event *event =
            xwl_present_event_from_vblank(xwl_present_window->flip_active);

        if (!event->pixmap
#ifdef DRI3
            /* If this flip used explicit sync, we won't get a release event */
            || (xwl_screen->explicit_sync && vblank->release_syncobj)
#endif /* DRI3 */
            ) {
            xwl_present_free_event(event);
        } else
            /* Put the previous flip in the idle_queue and wait for further notice from
             * the Wayland compositor
             */
            xorg_list_append(&xwl_present_window->flip_active->event_queue, &xwl_present_window->idle_queue);
    }

    xwl_present_window->flip_active = vblank;

    if (vblank->reason == PRESENT_FLIP_REASON_BUFFER_FORMAT)
        mode = PresentCompleteModeSuboptimalCopy;

    present_vblank_notify(vblank, PresentCompleteKindPixmap, mode, ust, crtc_msc);

    if (vblank->abort_flip)
        xwl_present_flips_stop(window);

    xwl_present_flip_try_ready(xwl_present_window);
}

static void
xwl_present_update_window_crtc(present_window_priv_ptr window_priv, RRCrtcPtr crtc, uint64_t new_msc)
{
    /* Crtc unchanged, no offset. */
    if (crtc == window_priv->crtc)
        return;

    /* No crtc earlier to offset against, just set the crtc. */
    if (window_priv->crtc == PresentCrtcNeverSet) {
        window_priv->msc_offset = 0;
        window_priv->crtc = crtc;
        return;
    }

    /* In window-mode the last correct msc-offset is always kept
     * in window-priv struct because msc is saved per window and
     * not per crtc as in screen-mode.
     */
    window_priv->msc_offset += new_msc - window_priv->msc;
    window_priv->crtc = crtc;
}


void
xwl_present_cleanup(WindowPtr window)
{
    struct xwl_present_window *xwl_present_window = xwl_present_window_priv(window);
    present_window_priv_ptr window_priv = present_window_priv(window);
    struct xwl_present_event *event, *tmp;

    if (!xwl_present_window)
        return;

    xorg_list_del(&xwl_present_window->frame_callback_list);

    if (xwl_present_window->sync_callback) {
        wl_callback_destroy(xwl_present_window->sync_callback);
        xwl_present_window->sync_callback = NULL;
    }

    if (window_priv) {
        /* Clear remaining events */
        xorg_list_for_each_entry_safe(event, tmp, &window_priv->vblank, vblank.window_list)
            xwl_present_free_event(event);
    }

    /* Clear timer */
    xwl_present_free_timer(xwl_present_window);
    TimerFree(xwl_present_window->unredirect_timer);

    /* Remove from privates so we don't try to access it later */
    dixSetPrivate(&window->devPrivates,
                  &xwl_present_window_private_key,
                  NULL);

    free(xwl_present_window);
}

static void
xwl_present_buffer_release(void *data)
{
    struct xwl_present_window *xwl_present_window;
    struct xwl_present_event *event = data;
    present_vblank_ptr vblank;

    if (!event)
        return;

    vblank = &event->vblank;

#if defined(XWL_HAS_GLAMOR) && defined(DRI3)
    if (vblank->release_syncobj) {
        /* transfer implicit fence to release syncobj */
        int fence_fd = xwl_glamor_dmabuf_export_sync_file(vblank->pixmap);
        vblank->release_syncobj->import_fence(vblank->release_syncobj,
                                              vblank->release_point,
                                              fence_fd);
    } else
#endif /* defined(XWL_HAS_GLAMOR) && defined(DRI3) */
        present_pixmap_idle(vblank->pixmap, vblank->window, vblank->serial, vblank->idle_fence);

    xwl_present_window = xwl_present_window_priv(vblank->window);
    if (xwl_present_window->flip_active == vblank ||
        xwl_present_get_pending_flip(xwl_present_window) == vblank)
        xwl_present_release_pixmap(event);
    else
        xwl_present_free_event(event);
}

static void
xwl_present_msc_bump(struct xwl_present_window *xwl_present_window)
{
    present_vblank_ptr flip_pending = xwl_present_get_pending_flip(xwl_present_window);
    uint64_t msc = ++xwl_present_window->msc;
    present_vblank_ptr vblank, tmp;

    xwl_present_window->ust = GetTimeInMicros();

    xwl_present_window->timer_armed = 0;

    if (flip_pending && flip_pending->sync_flip)
        xwl_present_flip_notify_vblank(flip_pending, xwl_present_window->ust, msc);

    xorg_list_for_each_entry_safe(vblank, tmp, &xwl_present_window->wait_list, event_queue) {
        if (vblank->exec_msc <= msc) {
            DebugPresent(("\te %" PRIu64 " ust %" PRIu64 " msc %" PRIu64 "\n",
                          vblank->event_id, xwl_present_window->ust, msc));

            xwl_present_execute(vblank, xwl_present_window->ust, msc);
        }
    }
}

static CARD32
xwl_present_timer_callback(OsTimerPtr timer,
                           CARD32 time,
                           void *arg)
{
    struct xwl_present_window *xwl_present_window = arg;

    /* If we were expecting a frame callback for this window, it didn't arrive
     * in a second. Stop listening to it to avoid double-bumping the MSC
     */
    xorg_list_del(&xwl_present_window->frame_callback_list);

    xwl_present_msc_bump(xwl_present_window);
    xwl_present_reset_timer(xwl_present_window);

    return 0;
}

void
xwl_present_frame_callback(struct xwl_present_window *xwl_present_window)
{
    xorg_list_del(&xwl_present_window->frame_callback_list);

    xwl_present_msc_bump(xwl_present_window);

    /* we do not need the timer anymore for this frame,
     * reset it for potentially the next one
     */
    xwl_present_reset_timer(xwl_present_window);
}

static void
xwl_present_sync_callback(void *data,
               struct wl_callback *callback,
               uint32_t time)
{
    present_vblank_ptr vblank = data;
    struct xwl_present_window *xwl_present_window = xwl_present_window_get_priv(vblank->window);

    wl_callback_destroy(xwl_present_window->sync_callback);
    xwl_present_window->sync_callback = NULL;

    xwl_present_flip_notify_vblank(vblank, xwl_present_window->ust, xwl_present_window->msc);
}

static const struct wl_callback_listener xwl_present_sync_listener = {
    xwl_present_sync_callback
};

static RRCrtcPtr
xwl_present_get_crtc(present_screen_priv_ptr screen_priv,
                     WindowPtr present_window)
{
    struct xwl_present_window *xwl_present_window = xwl_present_window_get_priv(present_window);
    rrScrPrivPtr rr_private;

    if (xwl_present_window == NULL)
        return NULL;

    rr_private = rrGetScrPriv(present_window->drawable.pScreen);

    if (rr_private->numCrtcs == 0)
        return NULL;

    return rr_private->crtcs[0];
}

/*
 * Queue an event to report back to the Present extension when the specified
 * MSC has passed
 */
static int
xwl_present_queue_vblank(ScreenPtr screen,
                         WindowPtr present_window,
                         RRCrtcPtr crtc,
                         uint64_t event_id,
                         uint64_t msc)
{
    struct xwl_present_window *xwl_present_window = xwl_present_window_get_priv(present_window);
    struct xwl_window *xwl_window = xwl_window_from_window(present_window);
    struct xwl_present_event *event = xwl_present_event_from_id(present_window, event_id);

    if (!event) {
        ErrorF("present: Error getting event\n");
        return BadImplementation;
    }

    BUG_RETURN_VAL(!xwl_present_window, BadImplementation);

    event->vblank.exec_msc = msc;

    xorg_list_del(&event->vblank.event_queue);
    xorg_list_append(&event->vblank.event_queue, &xwl_present_window->wait_list);

    /* Hook up to frame callback */
    if (xwl_window &&
        xorg_list_is_empty(&xwl_present_window->frame_callback_list)) {
        xorg_list_add(&xwl_present_window->frame_callback_list,
                      &xwl_window->frame_callback_list);
    }

    if ((xwl_window && xwl_window->frame_callback) ||
        !xwl_present_window->frame_timer)
        xwl_present_reset_timer(xwl_present_window);

    return Success;
}

/*
 * Remove a pending vblank event so that it is not reported
 * to the extension
 */
static void
xwl_present_abort_vblank(ScreenPtr screen,
                         WindowPtr present_window,
                         RRCrtcPtr crtc,
                         uint64_t event_id,
                         uint64_t msc)
{
    static Bool called;

    if (called)
        return;

    /* xwl_present_cleanup should have cleaned up everything,
     * present_free_window_vblank shouldn't need to call this.
     */
    ErrorF("Unexpected call to %s:\n", __func__);
    xorg_backtrace();
}

static void
xwl_present_flush(WindowPtr window)
{
#ifdef XWL_HAS_GLAMOR
    ScreenPtr screen = window->drawable.pScreen;
    struct xwl_screen *xwl_screen = xwl_screen_get(screen);

    if (xwl_screen->glamor)
        glamor_block_handler(screen);
#endif
}

static void
xwl_present_maybe_set_reason(struct xwl_window *xwl_window, PresentFlipReason *reason)
{
    struct xwl_screen *xwl_screen = xwl_window->xwl_screen;

    if (!reason || xwl_screen->dmabuf_protocol_version < 4)
        return;

    if (xwl_window->feedback.unprocessed_feedback_pending) {
        xwl_window->feedback.unprocessed_feedback_pending = 0;

        *reason = PRESENT_FLIP_REASON_BUFFER_FORMAT;
    }

    if (xwl_screen->default_feedback.unprocessed_feedback_pending) {
        xwl_screen->default_feedback.unprocessed_feedback_pending = 0;

        *reason = PRESENT_FLIP_REASON_BUFFER_FORMAT;
    }
}

static int
xwl_present_flush_fenced(WindowPtr window)
{
    int fence = -1;
#ifdef XWL_HAS_GLAMOR
    struct xwl_screen *xwl_screen = xwl_screen_get(window->drawable.pScreen);
    fence = xwl_glamor_get_fence(xwl_screen);
#endif /* XWL_HAS_GLAMOR */
    xwl_present_flush(window);
    return fence;
}

static Bool
xwl_present_check_flip(RRCrtcPtr crtc,
                       WindowPtr present_window,
                       PixmapPtr pixmap,
                       Bool sync_flip,
                       RegionPtr valid,
                       int16_t x_off,
                       int16_t y_off,
                       PresentFlipReason *reason)
{
    WindowPtr toplvl_window = xwl_present_toplvl_pixmap_window(present_window);
    struct xwl_window *xwl_window = xwl_window_from_window(present_window);
    ScreenPtr screen = pixmap->drawable.pScreen;

    if (reason)
        *reason = PRESENT_FLIP_REASON_UNKNOWN;

    if (!xwl_window)
        return FALSE;

    xwl_present_maybe_set_reason(xwl_window, reason);

    if (!crtc)
        return FALSE;

    /* Source pixmap must align with window exactly */
    if (x_off || y_off)
        return FALSE;

    /* Valid area must contain window (for simplicity for now just never flip when one is set). */
    if (valid)
        return FALSE;

    /* Flip pixmap must have same dimensions as window */
    if (present_window->drawable.width != pixmap->drawable.width ||
            present_window->drawable.height != pixmap->drawable.height)
        return FALSE;

    if (!xwl_pixmap_get_wl_buffer(pixmap))
        return FALSE;

    /* Window must be same region as toplevel window */
    if ( !RegionEqual(&present_window->winSize, &toplvl_window->winSize) )
        return FALSE;

    /* Can't flip if window clipped by children */
    if (!RegionEqual(&present_window->clipList, &present_window->winSize))
        return FALSE;

    /* Can't flip if the window pixmap doesn't match the xwl_window parent
     * window's, e.g. because a client redirected this window or one of its
     * parents.
     */
    if (screen->GetWindowPixmap(xwl_window->surface_window) != screen->GetWindowPixmap(present_window))
        return FALSE;

    /*
     * We currently only allow flips of windows, that have the same
     * dimensions as their xwl_window parent window. For the case of
     * different sizes subsurfaces are presumably the way forward.
     */
    if (!RegionEqual(&xwl_window->toplevel->winSize, &present_window->winSize))
        return FALSE;

#ifdef XWL_HAS_GLAMOR
    if (!xwl_glamor_supports_implicit_sync(xwl_window->xwl_screen) &&
        !xwl_window->xwl_screen->explicit_sync)
        return FALSE;

    if (xwl_window->xwl_screen->glamor &&
        !xwl_glamor_check_flip(present_window, pixmap))
        return FALSE;
#endif /* XWL_HAS_GLAMOR */

    return TRUE;
}

/*
 * 'window' is being reconfigured. Check to see if it is involved
 * in flipping and clean up as necessary.
 */
static void
xwl_present_check_flip_window (WindowPtr window)
{
    struct xwl_present_window *xwl_present_window = xwl_present_window_priv(window);
    present_window_priv_ptr window_priv = present_window_priv(window);
    present_vblank_ptr      flip_pending;
    present_vblank_ptr      flip_active;
    present_vblank_ptr      vblank;
    PresentFlipReason       reason;

    /* If this window hasn't ever been used with Present, it can't be
     * flipping
     */
    if (!xwl_present_window || !window_priv)
        return;

    flip_pending = xwl_present_get_pending_flip(xwl_present_window);
    flip_active = xwl_present_window->flip_active;

    if (flip_pending) {
        if (!xwl_present_check_flip(flip_pending->crtc, flip_pending->window, flip_pending->pixmap,
                                    flip_pending->sync_flip, flip_pending->valid, 0, 0, NULL))
            flip_pending->abort_flip = TRUE;
    } else if (flip_active) {
        if (!xwl_present_check_flip(flip_active->crtc, flip_active->window, flip_active->pixmap,
                                    flip_active->sync_flip, flip_active->valid, 0, 0, NULL))
            xwl_present_flips_stop(window);
    }

    /* Now check any queued vblanks */
    xorg_list_for_each_entry(vblank, &window_priv->vblank, window_list) {
        if (vblank->queued && vblank->flip &&
                !xwl_present_check_flip(vblank->crtc, window, vblank->pixmap,
                                        vblank->sync_flip, vblank->valid, 0, 0, &reason)) {
            vblank->flip = FALSE;
            vblank->reason = reason;
        }
    }
}

/*
 * Clean up any pending or current flips for this window
 */
static void
xwl_present_clear_window_flip(WindowPtr window)
{
    /* xwl_present_cleanup cleaned up everything */
}

static Bool
xwl_present_flip(present_vblank_ptr vblank, RegionPtr damage)
{
    WindowPtr present_window = vblank->window;
    PixmapPtr pixmap = vblank->pixmap;
    struct xwl_window           *xwl_window = xwl_window_from_window(present_window);
    struct xwl_present_window   *xwl_present_window = xwl_present_window_priv(present_window);
    BoxPtr                      damage_box;
    struct wl_buffer            *buffer;
    struct xwl_present_event    *event = xwl_present_event_from_vblank(vblank);
    Bool                        implicit_sync = TRUE;

    if (!xwl_window || !xwl_window->allow_commits)
        return FALSE;

    buffer = xwl_pixmap_get_wl_buffer(pixmap);
    if (!buffer) {
        ErrorF("present: Error getting buffer\n");
        return FALSE;
    }

    damage_box = RegionExtents(damage);

    pixmap->refcnt++;

    event->pixmap = pixmap;

#if defined(XWL_HAS_GLAMOR) && defined(DRI3)
    if (vblank->acquire_syncobj && vblank->release_syncobj) {
        if (xwl_window->xwl_screen->explicit_sync) {
            xwl_glamor_dri3_syncobj_passthrough(xwl_window,
                                                vblank->acquire_syncobj,
                                                vblank->release_syncobj,
                                                vblank->acquire_point,
                                                vblank->release_point);
            implicit_sync = FALSE;
        } else {
            /* transfer from acquire syncobj to implicit fence */
            int fence_fd =
                vblank->acquire_syncobj->export_fence(vblank->acquire_syncobj,
                                                      vblank->acquire_point);
            xwl_glamor_dmabuf_import_sync_file(vblank->pixmap, fence_fd);
        }
    }
#endif /* defined(XWL_HAS_GLAMOR) && defined(DRI3) */

    if (implicit_sync) {
        xwl_pixmap_set_buffer_release_cb(pixmap, xwl_present_buffer_release, event);

        if (xwl_window->surface_sync) {
            wp_linux_drm_syncobj_surface_v1_destroy(xwl_window->surface_sync);
            xwl_window->surface_sync = NULL;
        }
    }

    /* We can flip directly to the main surface (full screen window without clips) */
    wl_surface_attach(xwl_window->surface, buffer, 0, 0);

    if (xorg_list_is_empty(&xwl_present_window->frame_callback_list)) {
        xorg_list_add(&xwl_present_window->frame_callback_list,
                      &xwl_window->frame_callback_list);
    }

    if (!xwl_window->frame_callback)
        xwl_window_create_frame_callback(xwl_window);

    xwl_surface_damage(xwl_window->xwl_screen, xwl_window->surface,
                       damage_box->x1 - present_window->drawable.x,
                       damage_box->y1 - present_window->drawable.y,
                       damage_box->x2 - damage_box->x1,
                       damage_box->y2 - damage_box->y1);

    if (xwl_window->tearing_control) {
        uint32_t hint;
        if (event->options & PresentOptionAsyncMayTear)
            hint = WP_TEARING_CONTROL_V1_PRESENTATION_HINT_ASYNC;
        else
            hint = WP_TEARING_CONTROL_V1_PRESENTATION_HINT_VSYNC;

        wp_tearing_control_v1_set_presentation_hint(xwl_window->tearing_control, hint);
    }

    wl_surface_commit(xwl_window->surface);

    if (!vblank->sync_flip) {
        xwl_present_window->sync_callback =
            wl_display_sync(xwl_window->xwl_screen->display);
        wl_callback_add_listener(xwl_present_window->sync_callback,
                                 &xwl_present_sync_listener,
                                 &event->vblank);
    }

    wl_display_flush(xwl_window->xwl_screen->display);
    return TRUE;
}

#if defined(XWL_HAS_GLAMOR) && defined(DRI3)
static void
xwl_present_acquire_fence_avail(int fd, int xevents, void *data)
{
    present_vblank_ptr vblank = data;

    SetNotifyFd(fd, NULL, 0, NULL);
    close(fd);
    vblank->efd = -1;

    xwl_present_re_execute(vblank);
}
#endif /* defined(XWL_HAS_GLAMOR) && defined(DRI3) */

static Bool
xwl_present_wait_acquire_fence_avail(struct xwl_screen *xwl_screen,
                                     present_vblank_ptr vblank)
{
#if defined(XWL_HAS_GLAMOR) && defined(DRI3)
    /* If the compositor does not support explicit sync we need to wait for the
     * acquire fence to be submitted before flipping. */
    if (vblank->flip && !xwl_screen->explicit_sync &&
        vblank->pixmap && vblank->acquire_syncobj &&
        !vblank->acquire_syncobj->has_fence(vblank->acquire_syncobj,
                                            vblank->acquire_point)) {
        vblank->efd = eventfd(0, EFD_CLOEXEC);
        SetNotifyFd(vblank->efd, xwl_present_acquire_fence_avail, X_NOTIFY_READ, vblank);
        vblank->acquire_syncobj->submitted_eventfd(vblank->acquire_syncobj,
                                                   vblank->acquire_point,
                                                   vblank->efd);
        return TRUE;
    }
#endif /* defined(XWL_HAS_GLAMOR) && defined(DRI3) */
    return FALSE;
}

static void
xwl_present_flush_blocked(struct xwl_present_window *xwl_present_window,
                          uint64_t crtc_msc)
{
    struct xwl_screen *xwl_screen =
        xwl_screen_get(xwl_present_window->window->drawable.pScreen);
    struct xwl_present_event *blocked_event, *tmp;

    if (!xwl_present_window->blocking_event)
        return;

    xwl_present_window->blocking_event = 0;

    xorg_list_for_each_entry_safe(blocked_event, tmp,
                                  &xwl_present_window->blocked_queue,
                                  blocked) {
        present_vblank_ptr blocked_vblank = &blocked_event->vblank;
        xorg_list_del(&blocked_event->blocked);
        if (present_execute_wait(blocked_vblank, crtc_msc) ||
            xwl_present_wait_acquire_fence_avail(xwl_screen, blocked_vblank)) {
            xwl_present_window->blocking_event = blocked_vblank->event_id;
            return;
        }

        xwl_present_re_execute(blocked_vblank);
    }
}

/*
 * Once the required MSC has been reached, execute the pending request.
 *
 * For requests to actually present something, either blt contents to
 * the window pixmap or queue a window buffer swap on the backend.
 *
 * For requests to just get the current MSC/UST combo, skip that part and
 * go straight to event delivery.
 */
static void
xwl_present_execute(present_vblank_ptr vblank, uint64_t ust, uint64_t crtc_msc)
{
    WindowPtr               window = vblank->window;
    struct xwl_present_window *xwl_present_window = xwl_present_window_get_priv(window);
    present_vblank_ptr flip_pending = xwl_present_get_pending_flip(xwl_present_window);
    struct xwl_present_event *event = xwl_present_event_from_vblank(vblank);
    struct xwl_screen *xwl_screen = xwl_screen_get(window->drawable.pScreen);
    Bool notify_only = !vblank->window || !vblank->pixmap;

    xorg_list_del(&vblank->event_queue);

    if (!notify_only && !event->copy_executed &&
        xwl_present_window->blocking_event &&
        xwl_present_window->blocking_event != event->vblank.event_id) {
        /* an earlier request is blocking execution */
        xorg_list_append(&event->blocked, &xwl_present_window->blocked_queue);
        return;
    }

retry:
    if (present_execute_wait(vblank, crtc_msc) ||
        xwl_present_wait_acquire_fence_avail(xwl_screen, vblank)) {
        if (!notify_only)
            /* block execution of subsequent requests until this request is ready */
            xwl_present_window->blocking_event = event->vblank.event_id;
        return;
    }

    if (flip_pending && vblank->flip && !notify_only) {
        present_vblank_ptr flip_queued_last;

        flip_queued_last = xorg_list_last_entry(&xwl_present_window->flip_queue,
                                                present_vblank_rec, event_queue);

        /* Do mailbox handling for queued flips, to prevent the flip queue from
         * growing unbounded.
         */
        if (flip_queued_last != flip_pending &&
            (flip_queued_last->sync_flip
#ifdef DRI3
             || vblank->acquire_syncobj
#endif
             )) {
            xorg_list_del(&flip_queued_last->event_queue);
            present_vblank_scrap(flip_queued_last);
            xwl_present_re_execute(flip_queued_last);
        }

        DebugPresent(("\tr %" PRIu64 " %p (pending %p)\n",
                      vblank->event_id, vblank, flip_pending));
        xorg_list_append(&vblank->event_queue, &xwl_present_window->flip_queue);
        return;
    }

    vblank->queued = FALSE;

    if (!notify_only && !event->copy_executed) {
        ScreenPtr screen = window->drawable.pScreen;
        int ret;

        if (vblank->flip) {
            RegionPtr damage;

            DebugPresent(("\tf %" PRIu64 " %p %" PRIu64 ": %08" PRIx32 " -> %08" PRIx32 "\n",
                          vblank->event_id, vblank, crtc_msc,
                          vblank->pixmap->drawable.id, vblank->window->drawable.id));

            /* Set update region as damaged */
            if (vblank->update) {
                damage = RegionDuplicate(vblank->update);
                /* Translate update region to screen space */
                assert(vblank->x_off == 0 && vblank->y_off == 0);
                RegionTranslate(damage, window->drawable.x, window->drawable.y);
                RegionIntersect(damage, damage, &window->clipList);
            } else
                damage = RegionDuplicate(&window->clipList);

            if (xwl_present_flip(vblank, damage)) {
                WindowPtr toplvl_window = xwl_present_toplvl_pixmap_window(vblank->window);
                struct xwl_window *xwl_window = xwl_window_from_window(window);
                PixmapPtr old_pixmap = screen->GetWindowPixmap(window);

                /* Replace window pixmap with flip pixmap */
                vblank->pixmap->screen_x = old_pixmap->screen_x;
                vblank->pixmap->screen_y = old_pixmap->screen_y;
                present_set_tree_pixmap(toplvl_window, old_pixmap, vblank->pixmap);

                if (toplvl_window == screen->root &&
                    screen->GetScreenPixmap(screen) == old_pixmap)
                    screen->SetScreenPixmap(vblank->pixmap);

                vblank->pixmap->refcnt++;
                dixDestroyPixmap(old_pixmap, old_pixmap->drawable.id);

                /* Report damage, let damage_report ignore it though */
                xwl_screen->ignore_damage = TRUE;
                DamageDamageRegion(&vblank->window->drawable, damage);
                xwl_screen->ignore_damage = FALSE;
                RegionDestroy(damage);

                /* Clear damage region, to ensure damage_report is called before
                 * any drawing to the window
                 */
                xwl_window_buffer_add_damage_region(xwl_window);
                RegionEmpty(xwl_window_get_damage_region(xwl_window));
                xorg_list_del(&xwl_window->link_damage);

                /* Put pending flip at the flip queue head */
                xorg_list_add(&vblank->event_queue, &xwl_present_window->flip_queue);

                /* Realign timer */
                xwl_present_reset_timer(xwl_present_window);

                xwl_present_flush_blocked(xwl_present_window, crtc_msc);
                return;
            }

            vblank->flip = FALSE;
            /* re-execute, falling through to copy */
            goto retry;
        }
        DebugPresent(("\tc %p %" PRIu64 ": %08" PRIx32 " -> %08" PRIx32 "\n",
                      vblank, crtc_msc, vblank->pixmap->drawable.id, vblank->window->drawable.id));

        if (flip_pending)
            flip_pending->abort_flip = TRUE;
        else if (xwl_present_window->flip_active)
            xwl_present_flips_stop(window);

        present_execute_copy(vblank, crtc_msc);
        assert(!vblank->queued);

        /* Set the copy_executed field, so this will fall through to present_execute_post next time */
        event->copy_executed = TRUE;

        ret = xwl_present_queue_vblank(screen, window, vblank->crtc,
                                       vblank->event_id, crtc_msc + 1);

        xwl_present_flush_blocked(xwl_present_window, crtc_msc);

        if (ret == Success)
            return;
    }

    present_execute_post(vblank, ust, crtc_msc);
}

static int
xwl_present_pixmap(WindowPtr window,
                   PixmapPtr pixmap,
                   CARD32 serial,
                   RegionPtr valid,
                   RegionPtr update,
                   int16_t x_off,
                   int16_t y_off,
                   RRCrtcPtr target_crtc,
                   SyncFence *wait_fence,
                   SyncFence *idle_fence,
#ifdef DRI3
                   struct dri3_syncobj *acquire_syncobj,
                   struct dri3_syncobj *release_syncobj,
                   uint64_t acquire_point,
                   uint64_t release_point,
#endif /* DRI3 */
                   uint32_t options,
                   uint64_t target_window_msc,
                   uint64_t divisor,
                   uint64_t remainder,
                   present_notify_ptr notifies,
                   int num_notifies)
{
    static uint64_t xwl_present_event_id;
    uint64_t                    ust = 0;
    uint64_t                    target_msc;
    uint64_t                    crtc_msc = 0;
    int                         ret;
    present_vblank_ptr          vblank;
    ScreenPtr                   screen = window->drawable.pScreen;
    present_window_priv_ptr     window_priv = present_get_window_priv(window, TRUE);
    present_screen_priv_ptr     screen_priv = present_screen_priv(screen);
    struct xwl_screen          *xwl_screen = xwl_screen_get(screen_priv->pScreen);
    uint32_t                    caps = xwl_screen->present_capabilities;
    struct xwl_present_event *event;

    if (!window_priv)
        return BadAlloc;

#ifdef DRI3
    if (!(caps & PresentCapabilitySyncobj) &&
        (acquire_syncobj || release_syncobj))
        return BadValue;
#endif /* DRI3 */

    target_crtc = xwl_present_get_crtc(screen_priv, window);

    ret = xwl_present_get_ust_msc(screen, window, &ust, &crtc_msc);

    xwl_present_update_window_crtc(window_priv, target_crtc, crtc_msc);

    if (ret == Success) {
        /* Stash the current MSC away in case we need it later
         */
        window_priv->msc = crtc_msc;
    }

    target_msc = present_get_target_msc(target_window_msc + window_priv->msc_offset,
                                        crtc_msc,
                                        divisor,
                                        remainder,
                                        options);

    event = calloc(1, sizeof(*event));
    if (!event)
        return BadAlloc;

    vblank = &event->vblank;
    if (!present_vblank_init(vblank, window, pixmap, serial, valid, update, x_off, y_off,
                             target_crtc, wait_fence, idle_fence,
#ifdef DRI3
                             acquire_syncobj, release_syncobj, acquire_point, release_point,
#endif /* DRI3 */
                             options, caps, notifies, num_notifies, target_msc, crtc_msc)) {
        present_vblank_destroy(vblank);
        return BadAlloc;
    }

    vblank->event_id = ++xwl_present_event_id;
    event->options = options;
    event->divisor = divisor;
    event->remainder = remainder;
    vblank->exec_msc = xwl_present_get_exec_msc(options, vblank->target_msc);

    vblank->queued = TRUE;
    if (crtc_msc < vblank->exec_msc) {
        if (xwl_present_queue_vblank(screen, window, target_crtc, vblank->event_id, vblank->exec_msc) == Success)
            return Success;

        DebugPresent(("present_queue_vblank failed\n"));
    }

    xwl_present_execute(vblank, ust, crtc_msc);
    return Success;
}

void
xwl_present_unrealize_window(struct xwl_present_window *xwl_present_window)
{
    /* The pending frame callback may never be called, so drop it and shorten
     * the frame timer interval.
     */
    xorg_list_del(&xwl_present_window->frame_callback_list);

    /* Make sure the timer callback doesn't get called */
    xwl_present_window->timer_armed = 0;
    xwl_present_reset_timer(xwl_present_window);
}

Bool
xwl_present_maybe_redirect_window(WindowPtr window)
{
    struct xwl_present_window *xwl_present_window = xwl_present_window_get_priv(window);
    struct xwl_window *xwl_window = xwl_window_from_window(window);

    if (xwl_present_window->redirect_failed)
        return FALSE;

    if (compRedirectWindow(serverClient, window, CompositeRedirectManual) != Success) {
        xwl_present_window->redirect_failed = TRUE;
        return FALSE;
    }

    xwl_present_window->redirected = TRUE;

    xwl_window_update_surface_window(xwl_window);
    if (xwl_window->surface_window != window) {
        compUnredirectWindow(serverClient, window, CompositeRedirectManual);
        xwl_present_window->redirected = FALSE;
        xwl_present_window->redirect_failed = TRUE;
        xwl_window_update_surface_window(xwl_window);
        return FALSE;
    }

    if (!xwl_window->surface_window_damage)
        xwl_window->surface_window_damage = RegionCreate(NullBox, 1);

    return TRUE;
}

static CARD32
unredirect_window(OsTimerPtr timer, CARD32 time, void *arg)
{
    WindowPtr window = arg;
    struct xwl_present_window *xwl_present_window = xwl_present_window_get_priv(window);

    compUnredirectWindow(serverClient, window, CompositeRedirectManual);
    xwl_present_window->redirected = FALSE;

    xwl_present_window->unredirect_timer = NULL;
    return 0;
}

Bool
xwl_present_maybe_unredirect_window(WindowPtr window)
{
    struct xwl_present_window *xwl_present_window = xwl_present_window_get_priv(window);

    if (!xwl_present_window || !xwl_present_window->redirected)
        return FALSE;

    /* This function may get called from composite layer code, in which case
     * calling compUnredirectWindow would blow up. To avoid this, set up a timer
     * which will call it "as soon as possible".
     */
    if (!xwl_present_window->unredirect_timer) {
        xwl_present_window->unredirect_timer =
            TimerSet(NULL, 0, 1, unredirect_window, window);
    }

    return TRUE;
}

Bool
xwl_present_window_redirected(WindowPtr window)
{
    struct xwl_present_window *xwl_present_window = xwl_present_window_get_priv(window);

    return xwl_present_window->redirected;
}

Bool
xwl_present_init(ScreenPtr screen)
{
    struct xwl_screen *xwl_screen = xwl_screen_get(screen);
    present_screen_priv_ptr screen_priv;

    if (!present_screen_register_priv_keys())
        return FALSE;

    if (present_screen_priv(screen))
        return TRUE;

    screen_priv = present_screen_priv_init(screen);
    if (!screen_priv)
        return FALSE;

    if (!dixRegisterPrivateKey(&xwl_present_window_private_key, PRIVATE_WINDOW, 0))
        return FALSE;

    xwl_screen->present_capabilities = XWL_PRESENT_CAPS;
#ifdef XWL_HAS_GLAMOR
    if (xwl_glamor_supports_syncobjs(xwl_screen))
        xwl_screen->present_capabilities |=
            PresentCapabilitySyncobj;
#endif /* XWL_HAS_GLAMOR */

    screen_priv->query_capabilities = xwl_present_query_capabilities;
    screen_priv->get_crtc = xwl_present_get_crtc;

    screen_priv->check_flip = xwl_present_check_flip;
    screen_priv->check_flip_window = xwl_present_check_flip_window;
    screen_priv->clear_window_flip = xwl_present_clear_window_flip;

    screen_priv->present_pixmap = xwl_present_pixmap;
    screen_priv->queue_vblank = xwl_present_queue_vblank;
    screen_priv->flush = xwl_present_flush;
    screen_priv->flush_fenced = xwl_present_flush_fenced;
    screen_priv->re_execute = xwl_present_re_execute;

    screen_priv->abort_vblank = xwl_present_abort_vblank;

    return TRUE;
}

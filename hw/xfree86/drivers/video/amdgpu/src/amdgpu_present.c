/*
 * Copyright © 2014 Intel Corporation
 * Copyright © 2015 Advanced Micro Devices, Inc.
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
#include "config.h"
#include <xorg-server.h>

#include "amdgpu_drv.h"

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <sys/time.h>
#include <time.h>
#include <errno.h>

#include "amdgpu_glamor.h"
#include "amdgpu_pixmap.h"
#include "amdgpu_video.h"

#include "present.h"

static present_screen_info_rec amdgpu_present_screen_info;

struct amdgpu_present_vblank_event {
	uint64_t event_id;
	Bool unflip;
};

static RRCrtcPtr
amdgpu_present_get_crtc(WindowPtr window)
{
	return amdgpu_randr_crtc_covering_drawable(&window->drawable);
}

static int
amdgpu_present_get_ust_msc(RRCrtcPtr crtc, CARD64 *ust, CARD64 *msc)
{
	xf86CrtcPtr xf86_crtc = crtc->devPrivate;
	drmmode_crtc_private_ptr drmmode_crtc = xf86_crtc->driver_private;

	if (drmmode_crtc->dpms_mode != DPMSModeOn)
		return BadAlloc;

	return drmmode_crtc_get_ust_msc(xf86_crtc, ust, msc);
}

/*
 * Changes the variable refresh state for every CRTC on the screen.
 */
void
amdgpu_present_set_screen_vrr(ScrnInfoPtr scrn, Bool vrr_enabled)
{
	xf86CrtcConfigPtr config = XF86_CRTC_CONFIG_PTR(scrn);
	xf86CrtcPtr crtc;
	int i;

	for (i = 0; i < config->num_crtc; i++) {
		crtc = config->crtc[i];
		drmmode_crtc_set_vrr(crtc, vrr_enabled);
	}
}

/*
 * Flush the DRM event queue when full; this
 * makes space for new requests
 */
static Bool
amdgpu_present_flush_drm_events(ScreenPtr screen)
{
	ScrnInfoPtr scrn = xf86ScreenToScrn(screen);
	AMDGPUEntPtr pAMDGPUEnt = AMDGPUEntPriv(scrn);
	xf86CrtcConfigPtr xf86_config = XF86_CRTC_CONFIG_PTR(scrn);
	drmmode_crtc_private_ptr drmmode_crtc = xf86_config->crtc[0]->driver_private;
	drmmode_ptr drmmode = drmmode_crtc->drmmode;
	struct pollfd p = { .fd = pAMDGPUEnt->fd, .events = POLLIN };
	int r;

	do {
		r = poll(&p, 1, 0);
	} while (r == -1 && (errno == EINTR || errno == EAGAIN));

	if (r <= 0)
		return 0;

	return amdgpu_drm_handle_event(pAMDGPUEnt->fd, &drmmode->event_context) >= 0;
}

/*
 * Called when the queued vblank event has occurred
 */
static void
amdgpu_present_vblank_handler(xf86CrtcPtr crtc, unsigned int msc,
			      uint64_t usec, void *data)
{
	struct amdgpu_present_vblank_event *event = data;

	present_event_notify(event->event_id, usec, msc);
	free(event);
}

/*
 * Called when the queued vblank is aborted
 */
static void
amdgpu_present_vblank_abort(xf86CrtcPtr crtc, void *data)
{
	struct amdgpu_present_vblank_event *event = data;

	free(event);
}

/*
 * Queue an event to report back to the Present extension when the specified
 * MSC has past
 */
static int
amdgpu_present_queue_vblank(RRCrtcPtr crtc, uint64_t event_id, uint64_t msc)
{
	xf86CrtcPtr xf86_crtc = crtc->devPrivate;
	ScreenPtr screen = crtc->pScreen;
	struct amdgpu_present_vblank_event *event;
	uintptr_t drm_queue_seq;

	event = calloc(sizeof(struct amdgpu_present_vblank_event), 1);
	if (!event)
		return BadAlloc;
	event->event_id = event_id;

	drm_queue_seq = amdgpu_drm_queue_alloc(xf86_crtc,
					       AMDGPU_DRM_QUEUE_CLIENT_DEFAULT,
					       event_id, event,
					       amdgpu_present_vblank_handler,
					       amdgpu_present_vblank_abort,
					       FALSE);
	if (drm_queue_seq == AMDGPU_DRM_QUEUE_ERROR) {
		free(event);
		return BadAlloc;
	}

	for (;;) {
		if (drmmode_wait_vblank(xf86_crtc,
					DRM_VBLANK_ABSOLUTE | DRM_VBLANK_EVENT, msc,
					drm_queue_seq, NULL, NULL))
			break;
		if (errno != EBUSY || !amdgpu_present_flush_drm_events(screen)) {
			amdgpu_drm_abort_entry(drm_queue_seq);
			return BadAlloc;
		}
	}

	return Success;
}

/*
 * Remove a pending vblank event from the DRM queue so that it is not reported
 * to the extension
 */
static void
amdgpu_present_abort_vblank(RRCrtcPtr crtc, uint64_t event_id, uint64_t msc)
{
	amdgpu_drm_abort_id(event_id);
}

/*
 * Flush our batch buffer when requested by the Present extension.
 */
static void
amdgpu_present_flush(WindowPtr window)
{
	amdgpu_glamor_flush(xf86ScreenToScrn(window->drawable.pScreen));
}

/*
 * Test to see if unflipping is possible
 *
 * These tests have to pass for flips as well
 */
static Bool
amdgpu_present_check_unflip(ScrnInfoPtr scrn)
{
	xf86CrtcConfigPtr config = XF86_CRTC_CONFIG_PTR(scrn);
	int num_crtcs_on;
	int i;

	if (!scrn->vtSema)
		return FALSE;

	for (i = 0, num_crtcs_on = 0; i < config->num_crtc; i++) {
		xf86CrtcPtr crtc = config->crtc[i];

		if (drmmode_crtc_can_flip(crtc)) {
			drmmode_crtc_private_ptr drmmode_crtc = crtc->driver_private;

			if (drmmode_crtc->flip_pending)
				return FALSE;

			if (!drmmode_crtc->tear_free)
				num_crtcs_on++;
		}
	}

	return num_crtcs_on > 0;
}

/*
 * Test to see if page flipping is possible on the target crtc
 */
static Bool
amdgpu_present_check_flip(RRCrtcPtr crtc, WindowPtr window, PixmapPtr pixmap,
			  Bool sync_flip)
{
	xf86CrtcPtr xf86_crtc = crtc->devPrivate;
	ScreenPtr screen = window->drawable.pScreen;
	ScrnInfoPtr scrn = xf86_crtc->scrn;
	struct amdgpu_pixmap *priv = amdgpu_get_pixmap_private(pixmap);
	PixmapPtr screen_pixmap = screen->GetScreenPixmap(screen);
	xf86CrtcConfigPtr config = XF86_CRTC_CONFIG_PTR(scrn);
	AMDGPUInfoPtr info = AMDGPUPTR(scrn);
	int num_crtcs_on;
	Bool dc_enabled;
	int i;

	if (!scrn->vtSema)
		return FALSE;

	if (!info->allowPageFlip)
		return FALSE;

	if (info->sprites_visible > 0)
		return FALSE;

	if (info->drmmode.dri2_flipping)
		return FALSE;

	if (priv && priv->fb_failed)
		return FALSE;

	if (!amdgpu_pixmap_get_fb(pixmap)) {
		if (!priv)
			priv = amdgpu_get_pixmap_private(pixmap);

		if (priv && !priv->fb_failed) {
			xf86DrvMsg(scrn->scrnIndex, X_WARNING,
				   "Cannot get FB for Present flip (may be "
				   "normal if using PRIME render offloading)\n");
			priv->fb_failed = TRUE;
		}

		return FALSE;
	}

	/* Only DC supports advanced color management features, so we can use
	 * drmmode_cm_prop_supported with gamma_lut to check if the hw
	 * is capable of color management.
	 */
	dc_enabled = drmmode_cm_prop_supported(&info->drmmode, CM_GAMMA_LUT);

	if (info->dri2.pKernelDRMVersion->version_minor < (dc_enabled ? 31 : 34)) {
		/* The kernel driver doesn't handle flipping between BOs with
		 * different pitch correctly
		 */
		if (pixmap->devKind != screen_pixmap->devKind)
			return FALSE;
	}

	if (!dc_enabled || info->dri2.pKernelDRMVersion->version_minor < 31) {
		/* The kernel driver doesn't handle flipping between BOs with
		 * different tiling parameters correctly
		 */
		if (amdgpu_pixmap_get_tiling_info(pixmap) !=
		    amdgpu_pixmap_get_tiling_info(screen_pixmap))
			return FALSE;
	}

	for (i = 0, num_crtcs_on = 0; i < config->num_crtc; i++) {
		if (drmmode_crtc_can_flip(config->crtc[i]))
			num_crtcs_on++;
		else if (config->crtc[i] == crtc->devPrivate)
			return FALSE;
	}

	if (num_crtcs_on == 0)
		return FALSE;

	info->flip_window = window;

	return TRUE;
}

/*
 * Once the flip has been completed on all CRTCs, notify the
 * extension code telling it when that happened
 */
static void
amdgpu_present_flip_event(xf86CrtcPtr crtc, uint32_t msc, uint64_t ust, void *pageflip_data)
{
	AMDGPUInfoPtr info = AMDGPUPTR(crtc->scrn);
	struct amdgpu_present_vblank_event *event = pageflip_data;

	if (event->unflip)
		info->drmmode.present_flipping = FALSE;

	present_event_notify(event->event_id, ust, msc);
	free(event);
}

/*
 * The flip has been aborted, free the structure
 */
static void
amdgpu_present_flip_abort(xf86CrtcPtr crtc, void *pageflip_data)
{
	struct amdgpu_present_vblank_event *event = pageflip_data;

	free(event);
}

/*
 * Queue a flip on 'crtc' to 'pixmap' at 'target_msc'. If 'sync_flip' is true,
 * then wait for vblank. Otherwise, flip immediately
 */
static Bool
amdgpu_present_flip(RRCrtcPtr crtc, uint64_t event_id, uint64_t target_msc,
                   PixmapPtr pixmap, Bool sync_flip)
{
	xf86CrtcPtr xf86_crtc = crtc->devPrivate;
	ScrnInfoPtr scrn = xf86_crtc->scrn;
	AMDGPUInfoPtr info = AMDGPUPTR(scrn);
	struct amdgpu_present_vblank_event *event;
	Bool ret = FALSE;

	if (!amdgpu_present_check_flip(crtc, info->flip_window, pixmap, sync_flip))
		return ret;

	event = calloc(1, sizeof(struct amdgpu_present_vblank_event));
	if (!event)
		return ret;

	event->event_id = event_id;

	/* A window can only flip if it covers the entire X screen.
	 * Only one window can flip at a time.
	 *
	 * If the window also has the variable refresh property then
	 * variable refresh supported can be enabled on every CRTC.
	 */
	if (info->vrr_support &&
	    amdgpu_window_has_variable_refresh(info->flip_window))
		amdgpu_present_set_screen_vrr(scrn, TRUE);

	amdgpu_glamor_flush(scrn);

	ret = amdgpu_do_pageflip(scrn, AMDGPU_DRM_QUEUE_CLIENT_DEFAULT,
				 pixmap, event_id, event, crtc->devPrivate,
				 amdgpu_present_flip_event,
				 amdgpu_present_flip_abort,
				 sync_flip ? FLIP_VSYNC : FLIP_ASYNC,
				 target_msc);
	if (!ret)
		xf86DrvMsg(scrn->scrnIndex, X_ERROR, "present flip failed\n");
	else
		info->drmmode.present_flipping = TRUE;

	return ret;
}

/*
 * Queue a flip back to the normal frame buffer
 */
static void
amdgpu_present_unflip(ScreenPtr screen, uint64_t event_id)
{
	ScrnInfoPtr scrn = xf86ScreenToScrn(screen);
	AMDGPUInfoPtr info = AMDGPUPTR(scrn);
	xf86CrtcConfigPtr config = XF86_CRTC_CONFIG_PTR(scrn);
	struct amdgpu_present_vblank_event *event;
	PixmapPtr pixmap = screen->GetScreenPixmap(screen);
	enum drmmode_flip_sync flip_sync =
		(amdgpu_present_screen_info.capabilities & PresentCapabilityAsync) ?
		FLIP_ASYNC : FLIP_VSYNC;
	int i;

	amdgpu_present_set_screen_vrr(scrn, FALSE);

	if (!amdgpu_present_check_unflip(scrn))
		goto modeset;

	event = calloc(1, sizeof(struct amdgpu_present_vblank_event));
	if (!event) {
		ErrorF("%s: calloc failed, display might freeze\n", __func__);
		goto modeset;
	}

	event->event_id = event_id;
	event->unflip = TRUE;

	amdgpu_glamor_flush(scrn);
	if (amdgpu_do_pageflip(scrn, AMDGPU_DRM_QUEUE_CLIENT_DEFAULT, pixmap,
			       event_id, event, NULL, amdgpu_present_flip_event,
			       amdgpu_present_flip_abort, flip_sync, 0))
		return;

modeset:
	amdgpu_glamor_finish(scrn);
	for (i = 0; i < config->num_crtc; i++) {
		xf86CrtcPtr crtc = config->crtc[i];
		drmmode_crtc_private_ptr drmmode_crtc = crtc->driver_private;

		if (!crtc->enabled || drmmode_crtc->tear_free)
			continue;

		if (drmmode_crtc->dpms_mode == DPMSModeOn)
			crtc->funcs->set_mode_major(crtc, &crtc->mode, crtc->rotation,
						    crtc->x, crtc->y);
		else
			drmmode_crtc->need_modeset = TRUE;
	}

	present_event_notify(event_id, 0, 0);
	info->drmmode.present_flipping = FALSE;
}

static present_screen_info_rec amdgpu_present_screen_info = {
	.version = 0,

	.get_crtc = amdgpu_present_get_crtc,
	.get_ust_msc = amdgpu_present_get_ust_msc,
	.queue_vblank = amdgpu_present_queue_vblank,
	.abort_vblank = amdgpu_present_abort_vblank,
	.flush = amdgpu_present_flush,

	.capabilities = PresentCapabilityNone,
	.check_flip = amdgpu_present_check_flip,
	.flip = amdgpu_present_flip,
	.unflip = amdgpu_present_unflip,
};

static Bool
amdgpu_present_has_async_flip(ScreenPtr screen)
{
#ifdef DRM_CAP_ASYNC_PAGE_FLIP
	ScrnInfoPtr scrn = xf86ScreenToScrn(screen);
	AMDGPUEntPtr pAMDGPUEnt = AMDGPUEntPriv(scrn);
	int ret;
	uint64_t value;

	ret = drmGetCap(pAMDGPUEnt->fd, DRM_CAP_ASYNC_PAGE_FLIP, &value);
	if (ret == 0)
		return value == 1;
#endif
	return FALSE;
}

Bool
amdgpu_present_screen_init(ScreenPtr screen)
{
	ScrnInfoPtr scrn = xf86ScreenToScrn(screen);
	AMDGPUInfoPtr info = AMDGPUPTR(scrn);

	if (amdgpu_present_has_async_flip(screen)) {
		amdgpu_present_screen_info.capabilities |= PresentCapabilityAsync;
		info->can_async_flip = TRUE;
	}

	if (!present_screen_init(screen, &amdgpu_present_screen_info)) {
		xf86DrvMsg(xf86ScreenToScrn(screen)->scrnIndex, X_WARNING,
			   "Present extension disabled because present_screen_init failed\n");
		return FALSE;
	}

	xf86DrvMsg(xf86ScreenToScrn(screen)->scrnIndex, X_INFO,
		   "Present extension enabled\n");

	return TRUE;
}

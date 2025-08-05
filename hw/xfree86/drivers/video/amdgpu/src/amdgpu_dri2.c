/*
 * Copyright 2008 Kristian Høgsberg
 * Copyright 2008 Jérôme Glisse
 *
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation on the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial
 * portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NON-INFRINGEMENT.  IN NO EVENT SHALL ATI, VA LINUX SYSTEMS AND/OR
 * THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */
#include "config.h"
#include <xorg-server.h>

#include "amdgpu_drv.h"
#include "amdgpu_dri2.h"
#include "amdgpu_glamor.h"
#include "amdgpu_video.h"
#include "amdgpu_pixmap.h"

#ifdef DRI2

#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#include <gbm.h>

#include "amdgpu_bo_helper.h"
#include "amdgpu_version.h"

#include <list.h>
#include <xf86Priv.h>
#include <X11/extensions/dpmsconst.h>

#define FALLBACK_SWAP_DELAY 16

typedef DRI2BufferPtr BufferPtr;

struct dri2_buffer_priv {
	PixmapPtr pixmap;
	unsigned int attachment;
	unsigned int refcnt;
};

struct dri2_window_priv {
	xf86CrtcPtr crtc;
	int vblank_delta;
};

static DevPrivateKeyRec dri2_window_private_key_rec;
#define dri2_window_private_key (&dri2_window_private_key_rec)

#define get_dri2_window_priv(window) \
	((struct dri2_window_priv*) \
	 dixLookupPrivate(&(window)->devPrivates, dri2_window_private_key))

/* Get GEM flink name for a pixmap */
static Bool
amdgpu_get_flink_name(AMDGPUEntPtr pAMDGPUEnt, PixmapPtr pixmap, uint32_t *name)
{
	struct amdgpu_buffer *bo = amdgpu_get_pixmap_bo(pixmap);
	struct drm_gem_flink flink;

	if (bo && !(bo->flags & AMDGPU_BO_FLAGS_GBM) &&
	    amdgpu_bo_export(bo->bo.amdgpu,
			     amdgpu_bo_handle_type_gem_flink_name,
			     name) == 0)
		return TRUE;

	if (!amdgpu_pixmap_get_handle(pixmap, &flink.handle) ||
	    ioctl(pAMDGPUEnt->fd, DRM_IOCTL_GEM_FLINK, &flink) < 0)
		return FALSE;
	*name = flink.name;
	return TRUE;
}

static BufferPtr
amdgpu_dri2_create_buffer2(ScreenPtr pScreen,
			   DrawablePtr drawable,
			   unsigned int attachment, unsigned int format)
{
	ScrnInfoPtr pScrn = xf86ScreenToScrn(pScreen);
	AMDGPUEntPtr pAMDGPUEnt = AMDGPUEntPriv(pScrn);
	AMDGPUInfoPtr info = AMDGPUPTR(pScrn);
	BufferPtr buffers;
	struct dri2_buffer_priv *privates;
	PixmapPtr pixmap;
	unsigned front_width;
	unsigned aligned_width = drawable->width;
	unsigned height = drawable->height;
	Bool is_glamor_pixmap = FALSE;
	int depth;
	int cpp;

	if (format) {
		depth = format;

		switch (depth) {
		case 15:
			cpp = 2;
			break;
		case 24:
		case 30:
			cpp = 4;
			break;
		default:
			cpp = depth / 8;
		}
	} else {
		depth = drawable->depth;
		cpp = drawable->bitsPerPixel / 8;
	}

	front_width = pScreen->GetScreenPixmap(pScreen)->drawable.width;

	pixmap = NULL;

	if (attachment == DRI2BufferFrontLeft) {
		uint32_t handle;

		pixmap = get_drawable_pixmap(drawable);
		if (pScreen != pixmap->drawable.pScreen)
			pixmap = NULL;
		else if (info->use_glamor && !amdgpu_pixmap_get_handle(pixmap, &handle)) {
			is_glamor_pixmap = TRUE;
			aligned_width = pixmap->drawable.width;
			height = pixmap->drawable.height;
			pixmap = NULL;
		} else
			pixmap->refcnt++;
	}

	if (!pixmap && (is_glamor_pixmap || attachment != DRI2BufferFrontLeft)) {
		if (aligned_width == front_width)
			aligned_width = pScrn->virtualX;

		pixmap = (*pScreen->CreatePixmap) (pScreen,
						   aligned_width,
						   height,
						   depth,
						   AMDGPU_CREATE_PIXMAP_DRI2);
	}

	if (!pixmap)
		return NULL;

	buffers = calloc(1, sizeof *buffers);
	if (!buffers)
		goto error;

	if (is_glamor_pixmap) {
		pixmap = amdgpu_glamor_set_pixmap_bo(drawable, pixmap);
		pixmap->refcnt++;

		/* The copy operation from amdgpu_glamor_set_pixmap_bo needs to
		 * be flushed to the kernel driver before the client starts
		 * using the pixmap storage for direct rendering.
		 */
		amdgpu_glamor_flush(pScrn);
	}

	if (!amdgpu_get_flink_name(pAMDGPUEnt, pixmap, &buffers->name))
		goto error;

	privates = calloc(1, sizeof(struct dri2_buffer_priv));
	if (!privates)
		goto error;

	buffers->attachment = attachment;
	buffers->pitch = pixmap->devKind;
	buffers->cpp = cpp;
	buffers->driverPrivate = privates;
	buffers->format = format;
	buffers->flags = 0;	/* not tiled */
	privates->pixmap = pixmap;
	privates->attachment = attachment;
	privates->refcnt = 1;

	return buffers;

error:
	free(buffers);
	dixDestroyPixmap(pixmap, 0);
	return NULL;
}

static void
amdgpu_dri2_destroy_buffer2(ScreenPtr pScreen,
			    DrawablePtr drawable, BufferPtr buffers)
{
	if (buffers) {
		struct dri2_buffer_priv *private = buffers->driverPrivate;

		/* Trying to free an already freed buffer is unlikely to end well */
		if (private->refcnt == 0) {
			ScrnInfoPtr scrn = xf86ScreenToScrn(pScreen);

			xf86DrvMsg(scrn->scrnIndex, X_WARNING,
				   "Attempted to destroy previously destroyed buffer.\
 This is a programming error\n");
			return;
		}

		private->refcnt--;
		if (private->refcnt == 0) {
			if (private->pixmap)
				dixDestroyPixmap(private->pixmap, 0);

			free(buffers->driverPrivate);
			free(buffers);
		}
	}
}

static inline PixmapPtr GetDrawablePixmap(DrawablePtr drawable)
{
	if (drawable->type == DRAWABLE_PIXMAP)
		return (PixmapPtr) drawable;
	else {
		struct _Window *pWin = (struct _Window *)drawable;
		return drawable->pScreen->GetWindowPixmap(pWin);
	}
}

static void
amdgpu_dri2_copy_region2(ScreenPtr pScreen,
			 DrawablePtr drawable,
			 RegionPtr region,
			 BufferPtr dest_buffer, BufferPtr src_buffer)
{
	struct dri2_buffer_priv *src_private = src_buffer->driverPrivate;
	struct dri2_buffer_priv *dst_private = dest_buffer->driverPrivate;
	DrawablePtr src_drawable;
	DrawablePtr dst_drawable;
	RegionPtr copy_clip;
	GCPtr gc;
	Bool translate = FALSE;
	int off_x = 0, off_y = 0;

	src_drawable = &src_private->pixmap->drawable;
	dst_drawable = &dst_private->pixmap->drawable;

	if (src_private->attachment == DRI2BufferFrontLeft) {
		if (drawable->pScreen != pScreen) {
			src_drawable = DRI2UpdatePrime(drawable, src_buffer);
			if (!src_drawable)
				return;
		} else
			src_drawable = drawable;
	}
	if (dst_private->attachment == DRI2BufferFrontLeft) {
		if (drawable->pScreen != pScreen) {
			dst_drawable = DRI2UpdatePrime(drawable, dest_buffer);
			if (!dst_drawable)
				return;
			if (dst_drawable != drawable)
				translate = TRUE;
		} else
			dst_drawable = drawable;
	}

	if (translate && drawable->type == DRAWABLE_WINDOW) {
		PixmapPtr pPix = GetDrawablePixmap(drawable);

		off_x = drawable->x - pPix->screen_x;
		off_y = drawable->y - pPix->screen_y;
	}
	gc = GetScratchGC(dst_drawable->depth, pScreen);
	copy_clip = REGION_CREATE(pScreen, NULL, 0);
	REGION_COPY(pScreen, copy_clip, region);

	if (translate) {
		REGION_TRANSLATE(pScreen, copy_clip, off_x, off_y);
	}

	(*gc->funcs->ChangeClip) (gc, CT_REGION, copy_clip, 0);
	ValidateGC(dst_drawable, gc);

	(*gc->ops->CopyArea) (src_drawable, dst_drawable, gc,
			      0, 0, drawable->width, drawable->height, off_x,
			      off_y);

	FreeScratchGC(gc);
}

enum DRI2FrameEventType {
	DRI2_SWAP,
	DRI2_FLIP,
	DRI2_WAITMSC,
};

typedef struct _DRI2FrameEvent {
	XID drawable_id;
	ClientPtr client;
	enum DRI2FrameEventType type;
	unsigned frame;
	xf86CrtcPtr crtc;
	OsTimerPtr timer;
	uintptr_t drm_queue_seq;

	/* for swaps & flips only */
	DRI2SwapEventPtr event_complete;
	void *event_data;
	DRI2BufferPtr front;
	DRI2BufferPtr back;
} DRI2FrameEventRec, *DRI2FrameEventPtr;

static int DRI2InfoCnt;

static void amdgpu_dri2_ref_buffer(BufferPtr buffer)
{
	struct dri2_buffer_priv *private = buffer->driverPrivate;
	private->refcnt++;
}

static void amdgpu_dri2_unref_buffer(BufferPtr buffer)
{
	if (buffer) {
		struct dri2_buffer_priv *private = buffer->driverPrivate;
		DrawablePtr draw = &private->pixmap->drawable;

		amdgpu_dri2_destroy_buffer2(draw->pScreen, draw, buffer);
	}
}

static void
amdgpu_dri2_client_state_changed(CallbackListPtr * cbl, void* data, void* calldata)
{
	NewClientInfoRec *clientinfo = calldata;
	ClientPtr pClient = clientinfo->client;

	switch (pClient->clientState) {
	case ClientStateRetained:
	case ClientStateGone:
		amdgpu_drm_abort_client(pClient);
		break;
	default:
		break;
	}
}

/*
 * Get current frame count delta for the specified drawable and CRTC
 */
static uint32_t amdgpu_get_msc_delta(DrawablePtr pDraw, xf86CrtcPtr crtc)
{
	drmmode_crtc_private_ptr drmmode_crtc = crtc->driver_private;

	if (pDraw && pDraw->type == DRAWABLE_WINDOW)
		return drmmode_crtc->interpolated_vblanks +
			get_dri2_window_priv((WindowPtr)pDraw)->vblank_delta;

	return drmmode_crtc->interpolated_vblanks;
}

/*
 * Get current frame count and timestamp of the specified CRTC
 */
static Bool amdgpu_dri2_get_crtc_msc(xf86CrtcPtr crtc, CARD64 *ust, CARD64 *msc)
{
	drmmode_crtc_private_ptr drmmode_crtc = crtc->driver_private;

	if (!amdgpu_crtc_is_enabled(crtc) ||
	    drmmode_crtc_get_ust_msc(crtc, ust, msc) != Success) {
		/* CRTC is not running, extrapolate MSC and timestamp */
		ScrnInfoPtr scrn = crtc->scrn;
		AMDGPUEntPtr pAMDGPUEnt = AMDGPUEntPriv(scrn);
		CARD64 now, delta_t, delta_seq;

		if (!drmmode_crtc->dpms_last_ust)
			return FALSE;

		if (drmmode_get_current_ust(pAMDGPUEnt->fd, &now) != 0) {
			xf86DrvMsg(scrn->scrnIndex, X_ERROR,
				   "%s cannot get current time\n", __func__);
			return FALSE;
		}

		delta_t = now - drmmode_crtc->dpms_last_ust;
		delta_seq = delta_t * drmmode_crtc->dpms_last_fps;
		delta_seq /= 1000000;
		*ust = drmmode_crtc->dpms_last_ust;
		delta_t = delta_seq * 1000000;
		delta_t /= drmmode_crtc->dpms_last_fps;
		*ust += delta_t;
		*msc = drmmode_crtc->dpms_last_seq;
		*msc += delta_seq;
	}

	*msc += drmmode_crtc->interpolated_vblanks;

	return TRUE;
}

static
xf86CrtcPtr amdgpu_dri2_drawable_crtc(DrawablePtr pDraw)
{
	ScreenPtr pScreen = pDraw->pScreen;
	xf86CrtcPtr crtc = amdgpu_pick_best_crtc(pScreen,
						 pDraw->x, pDraw->x + pDraw->width,
						 pDraw->y, pDraw->y + pDraw->height);

	if (pDraw->type == DRAWABLE_WINDOW) {
		struct dri2_window_priv *priv = get_dri2_window_priv((WindowPtr)pDraw);

		if (!crtc) {
			crtc = priv->crtc;
		} else if (priv->crtc && priv->crtc != crtc) {
			CARD64 ust, mscold, mscnew;

			if (amdgpu_dri2_get_crtc_msc(priv->crtc, &ust, &mscold) &&
			    amdgpu_dri2_get_crtc_msc(crtc, &ust, &mscnew))
				priv->vblank_delta += mscold - mscnew;
		}

		priv->crtc = crtc;
	}

	return crtc;
}

static void
amdgpu_dri2_flip_event_abort(xf86CrtcPtr crtc, void *event_data)
{
	if (crtc)
		AMDGPUPTR(crtc->scrn)->drmmode.dri2_flipping = FALSE;

	free(event_data);
}

static void
amdgpu_dri2_flip_event_handler(xf86CrtcPtr crtc, uint32_t frame, uint64_t usec,
			       void *event_data)
{
	DRI2FrameEventPtr flip = event_data;
	ScrnInfoPtr scrn = crtc->scrn;
	unsigned tv_sec, tv_usec;
	DrawablePtr drawable;
	ScreenPtr screen;
	int status;
	PixmapPtr pixmap;

	status = dixLookupDrawable(&drawable, flip->drawable_id, serverClient,
				   M_ANY, DixWriteAccess);
	if (status != Success)
		goto abort;

	frame += amdgpu_get_msc_delta(drawable, crtc);

	screen = scrn->pScreen;
	pixmap = screen->GetScreenPixmap(screen);
	xf86DrvMsgVerb(scrn->scrnIndex, X_INFO, AMDGPU_LOGLEVEL_DEBUG,
		       "%s:%d fevent[%p] width %d pitch %d (/4 %d)\n",
		       __func__, __LINE__, flip, pixmap->drawable.width,
		       pixmap->devKind, pixmap->devKind / 4);

	tv_sec = usec / 1000000;
	tv_usec = usec % 1000000;

	/* We assume our flips arrive in order, so we don't check the frame */
	switch (flip->type) {
	case DRI2_SWAP:
		/* Check for too small vblank count of pageflip completion, taking wraparound
		 * into account. This usually means some defective kms pageflip completion,
		 * causing wrong (msc, ust) return values and possible visual corruption.
		 */
		if ((frame < flip->frame) && (flip->frame - frame < 5)) {
			xf86DrvMsg(scrn->scrnIndex, X_WARNING,
				   "%s: Pageflip completion event has impossible msc %u < target_msc %u\n",
				   __func__, frame, flip->frame);
			/* All-Zero values signal failure of (msc, ust) timestamping to client. */
			frame = tv_sec = tv_usec = 0;
		}

		DRI2SwapComplete(flip->client, drawable, frame, tv_sec, tv_usec,
				 DRI2_FLIP_COMPLETE, flip->event_complete,
				 flip->event_data);
		break;
	default:
		xf86DrvMsg(scrn->scrnIndex, X_WARNING,
			   "%s: unknown vblank event received\n", __func__);
		/* Unknown type */
		break;
	}

abort:
	amdgpu_dri2_flip_event_abort(crtc, event_data);
}

static Bool
amdgpu_dri2_schedule_flip(xf86CrtcPtr crtc, ClientPtr client,
			  DrawablePtr draw, DRI2BufferPtr front,
			  DRI2BufferPtr back, DRI2SwapEventPtr func,
			  void *data, unsigned int target_msc)
{
	ScrnInfoPtr scrn = crtc->scrn;
	AMDGPUInfoPtr info = AMDGPUPTR(scrn);
	struct dri2_buffer_priv *back_priv;
	DRI2FrameEventPtr flip_info;

	flip_info = calloc(1, sizeof(DRI2FrameEventRec));
	if (!flip_info)
		return FALSE;

	flip_info->drawable_id = draw->id;
	flip_info->client = client;
	flip_info->type = DRI2_SWAP;
	flip_info->event_complete = func;
	flip_info->event_data = data;
	flip_info->frame = target_msc;
	flip_info->crtc = crtc;

	xf86DrvMsgVerb(scrn->scrnIndex, X_INFO, AMDGPU_LOGLEVEL_DEBUG,
		       "%s:%d fevent[%p]\n", __func__, __LINE__, flip_info);

	/* Page flip the full screen buffer */
	back_priv = back->driverPrivate;
	if (amdgpu_do_pageflip(scrn, client, back_priv->pixmap,
			       AMDGPU_DRM_QUEUE_ID_DEFAULT, flip_info, crtc,
			       amdgpu_dri2_flip_event_handler,
			       amdgpu_dri2_flip_event_abort, FLIP_VSYNC,
			       target_msc - amdgpu_get_msc_delta(draw, crtc))) {
		info->drmmode.dri2_flipping = TRUE;
		return TRUE;
	}
	return FALSE;
}

static Bool update_front(DrawablePtr draw, DRI2BufferPtr front)
{
	ScreenPtr screen = draw->pScreen;
	ScrnInfoPtr scrn = xf86ScreenToScrn(screen);
	AMDGPUEntPtr pAMDGPUEnt = AMDGPUEntPriv(scrn);
	PixmapPtr pixmap = get_drawable_pixmap(draw);
	struct dri2_buffer_priv *priv = front->driverPrivate;

	if (!amdgpu_get_flink_name(pAMDGPUEnt, pixmap, &front->name))
		return FALSE;

	dixDestroyPixmap(priv->pixmap, 0);
	front->pitch = pixmap->devKind;
	front->cpp = pixmap->drawable.bitsPerPixel / 8;
	priv->pixmap = pixmap;
	pixmap->refcnt++;

	return TRUE;
}

static Bool
can_exchange(ScrnInfoPtr pScrn, DrawablePtr draw,
	     DRI2BufferPtr front, DRI2BufferPtr back)
{
	struct dri2_buffer_priv *front_priv = front->driverPrivate;
	struct dri2_buffer_priv *back_priv = back->driverPrivate;
	PixmapPtr front_pixmap;
	PixmapPtr back_pixmap = back_priv->pixmap;

	if (!update_front(draw, front))
		return FALSE;

	front_pixmap = front_priv->pixmap;

	if (front_pixmap->drawable.width != back_pixmap->drawable.width)
		return FALSE;

	if (front_pixmap->drawable.height != back_pixmap->drawable.height)
		return FALSE;

	if (front_pixmap->drawable.bitsPerPixel !=
	    back_pixmap->drawable.bitsPerPixel)
		return FALSE;

	if (front_pixmap->devKind != back_pixmap->devKind)
		return FALSE;

	return TRUE;
}

static Bool
can_flip(xf86CrtcPtr crtc, DrawablePtr draw,
	 DRI2BufferPtr front, DRI2BufferPtr back)
{
	ScrnInfoPtr pScrn = crtc->scrn;
	AMDGPUInfoPtr info = AMDGPUPTR(pScrn);
	xf86CrtcConfigPtr config = XF86_CRTC_CONFIG_PTR(pScrn);
	int num_crtcs_on;
	int i;

	if (draw->type != DRAWABLE_WINDOW ||
	    !info->allowPageFlip ||
	    info->sprites_visible > 0 ||
	    info->drmmode.present_flipping ||
	    !pScrn->vtSema ||
	    !DRI2CanFlip(draw))
		return FALSE;

	for (i = 0, num_crtcs_on = 0; i < config->num_crtc; i++) {
		if (drmmode_crtc_can_flip(config->crtc[i]))
			num_crtcs_on++;
	}

	return num_crtcs_on > 0 && can_exchange(pScrn, draw, front, back);
}

static void
amdgpu_dri2_exchange_buffers(DrawablePtr draw, DRI2BufferPtr front,
			     DRI2BufferPtr back)
{
	struct dri2_buffer_priv *front_priv = front->driverPrivate;
	struct dri2_buffer_priv *back_priv = back->driverPrivate;
	struct amdgpu_pixmap *front_pix;
	struct amdgpu_pixmap *back_pix;
	ScreenPtr screen;
	AMDGPUInfoPtr info;
	RegionRec region;
	int tmp;

	region.extents.x1 = region.extents.y1 = 0;
	region.extents.x2 = front_priv->pixmap->drawable.width;
	region.extents.y2 = front_priv->pixmap->drawable.height;
	region.data = NULL;
	DamageRegionAppend(&front_priv->pixmap->drawable, &region);

	/* Swap BO names so DRI works */
	tmp = front->name;
	front->name = back->name;
	back->name = tmp;

	/* Swap pixmap privates */
	front_pix = amdgpu_get_pixmap_private(front_priv->pixmap);
	back_pix = amdgpu_get_pixmap_private(back_priv->pixmap);
	amdgpu_set_pixmap_private(front_priv->pixmap, back_pix);
	amdgpu_set_pixmap_private(back_priv->pixmap, front_pix);

	/* Do we need to update the Screen? */
	screen = draw->pScreen;
	info = AMDGPUPTR(xf86ScreenToScrn(screen));
	if (front_pix->bo == info->front_buffer) {
		struct amdgpu_pixmap *screen_priv =
			amdgpu_get_pixmap_private(screen->GetScreenPixmap(screen));

		amdgpu_bo_ref(back_pix->bo);
		amdgpu_bo_unref(&info->front_buffer);
		info->front_buffer = back_pix->bo;
		*screen_priv = *back_pix;
	}

	amdgpu_glamor_exchange_buffers(front_priv->pixmap, back_priv->pixmap);

	DamageRegionProcessPending(&front_priv->pixmap->drawable);
}

static void amdgpu_dri2_frame_event_abort(xf86CrtcPtr crtc, void *event_data)
{
	DRI2FrameEventPtr event = event_data;

	TimerCancel(event->timer);
	TimerFree(event->timer);
	amdgpu_dri2_unref_buffer(event->front);
	amdgpu_dri2_unref_buffer(event->back);
	free(event);
}

static void amdgpu_dri2_frame_event_handler(xf86CrtcPtr crtc, uint32_t seq,
					    uint64_t usec, void *event_data)
{
	DRI2FrameEventPtr event = event_data;
	ScrnInfoPtr scrn = crtc->scrn;
	DrawablePtr drawable;
	int status;
	int swap_type;
	BoxRec box;
	RegionRec region;

	status = dixLookupDrawable(&drawable, event->drawable_id, serverClient,
				   M_ANY, DixWriteAccess);
	if (status != Success)
		goto cleanup;

	seq += amdgpu_get_msc_delta(drawable, crtc);

	switch (event->type) {
	case DRI2_FLIP:
		if (can_flip(crtc, drawable, event->front, event->back) &&
		    amdgpu_dri2_schedule_flip(crtc,
					      event->client,
					      drawable,
					      event->front,
					      event->back,
					      event->event_complete,
					      event->event_data,
					      event->frame)) {
			amdgpu_dri2_exchange_buffers(drawable, event->front,
						     event->back);
			break;
		}
		/* else fall through to exchange/blit */
	case DRI2_SWAP:
		if (DRI2CanExchange(drawable) &&
		    can_exchange(scrn, drawable, event->front, event->back)) {
			amdgpu_dri2_exchange_buffers(drawable, event->front,
						     event->back);
			swap_type = DRI2_EXCHANGE_COMPLETE;
		} else {
			box.x1 = 0;
			box.y1 = 0;
			box.x2 = drawable->width;
			box.y2 = drawable->height;
			REGION_INIT(pScreen, &region, &box, 0);
			amdgpu_dri2_copy_region2(drawable->pScreen, drawable, &region,
						 event->front, event->back);
			swap_type = DRI2_BLIT_COMPLETE;
		}

		DRI2SwapComplete(event->client, drawable, seq, usec / 1000000,
				 usec % 1000000, swap_type, event->event_complete,
				 event->event_data);

		break;
	case DRI2_WAITMSC:
		DRI2WaitMSCComplete(event->client, drawable, seq, usec / 1000000,
				    usec % 1000000);
		break;
	default:
		/* Unknown type */
		xf86DrvMsg(scrn->scrnIndex, X_WARNING,
			   "%s: unknown vblank event received\n", __func__);
		break;
	}

cleanup:
	amdgpu_dri2_frame_event_abort(crtc, event_data);
}

/*
 * This function should be called on a disabled CRTC only (i.e., CRTC
 * in DPMS-off state). It will calculate the delay necessary to reach
 * target_msc from present time if the CRTC were running.
 */
static
CARD32 amdgpu_dri2_extrapolate_msc_delay(xf86CrtcPtr crtc, CARD64 * target_msc,
					 CARD64 divisor, CARD64 remainder)
{
	drmmode_crtc_private_ptr drmmode_crtc = crtc->driver_private;
	ScrnInfoPtr pScrn = crtc->scrn;
	AMDGPUEntPtr pAMDGPUEnt = AMDGPUEntPriv(pScrn);
	int nominal_frame_rate = drmmode_crtc->dpms_last_fps;
	CARD64 last_vblank_ust = drmmode_crtc->dpms_last_ust;
	uint32_t last_vblank_seq = drmmode_crtc->dpms_last_seq;
	CARD64 now, target_time, delta_t;
	int64_t d, delta_seq;
	int ret;
	CARD32 d_ms;

	if (!last_vblank_ust) {
		*target_msc = 0;
		return FALLBACK_SWAP_DELAY;
	}
	ret = drmmode_get_current_ust(pAMDGPUEnt->fd, &now);
	if (ret) {
		xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
			   "%s cannot get current time\n", __func__);
		*target_msc = 0;
		return FALLBACK_SWAP_DELAY;
	}
	delta_seq = *target_msc - last_vblank_seq;
	delta_seq *= 1000000;
	target_time = last_vblank_ust;
	target_time += delta_seq / nominal_frame_rate;
	d = target_time - now;
	if (d < 0) {
		/* we missed the event, adjust target_msc, do the divisor magic */
		CARD64 current_msc = last_vblank_seq;

		delta_t = now - last_vblank_ust;
		delta_seq = delta_t * nominal_frame_rate;
		current_msc += delta_seq / 1000000;
		current_msc &= 0xffffffff;
		if (divisor == 0) {
			*target_msc = current_msc;
			d = 0;
		} else {
			*target_msc =
			    current_msc - (current_msc % divisor) + remainder;
			if ((current_msc % divisor) >= remainder)
				*target_msc += divisor;
			*target_msc &= 0xffffffff;
			delta_seq = *target_msc - last_vblank_seq;
			delta_seq *= 1000000;
			target_time = last_vblank_ust;
			target_time += delta_seq / nominal_frame_rate;
			d = target_time - now;
		}
	}
	/*
	 * convert delay to milliseconds and add margin to prevent the client
	 * from coming back early (due to timer granularity and rounding
	 * errors) and getting the same MSC it just got
	 */
	d_ms = (CARD32) d / 1000;
	if ((CARD32) d - d_ms * 1000 > 0)
		d_ms += 2;
	else
		d_ms++;
	return d_ms;
}

/*
 * Get current interpolated frame count and frame count timestamp, based on
 * drawable's crtc.
 */
static int amdgpu_dri2_get_msc(DrawablePtr draw, CARD64 * ust, CARD64 * msc)
{
	xf86CrtcPtr crtc = amdgpu_dri2_drawable_crtc(draw);

	/* Drawable not displayed, make up a value */
	if (!crtc) {
		*ust = 0;
		*msc = 0;
		return TRUE;
	}

	if (!amdgpu_dri2_get_crtc_msc(crtc, ust, msc))
		return FALSE;

	if (draw && draw->type == DRAWABLE_WINDOW)
		*msc += get_dri2_window_priv((WindowPtr)draw)->vblank_delta;
	*msc &= 0xffffffff;
	return TRUE;
}

static
CARD32 amdgpu_dri2_deferred_event(OsTimerPtr timer, CARD32 now, void* data)
{
	DRI2FrameEventPtr event_info = (DRI2FrameEventPtr) data;
	xf86CrtcPtr crtc = event_info->crtc;
	ScrnInfoPtr scrn;
	AMDGPUEntPtr pAMDGPUEnt;
	CARD64 drm_now;
	int ret;
	CARD64 delta_t, delta_seq, frame;
	drmmode_crtc_private_ptr drmmode_crtc;

	/*
	 * This is emulated event, so its time is current time, which we
	 * have to get in DRM-compatible form (which is a bit messy given
	 * the information that we have at this point). Can't use now argument
	 * because DRM event time may come from monotonic clock, while
	 * DIX timer facility uses real-time clock.
	 */
	if (!event_info->crtc) {
		ErrorF("%s no crtc\n", __func__);
		if (event_info->drm_queue_seq)
			amdgpu_drm_abort_entry(event_info->drm_queue_seq);
		else
			amdgpu_dri2_frame_event_abort(NULL, data);
		return 0;
	}

	scrn = crtc->scrn;
	pAMDGPUEnt = AMDGPUEntPriv(scrn);
	drmmode_crtc = event_info->crtc->driver_private;
	ret = drmmode_get_current_ust(pAMDGPUEnt->fd, &drm_now);
	if (ret) {
		xf86DrvMsg(scrn->scrnIndex, X_ERROR,
			   "%s cannot get current time\n", __func__);

		if (event_info->drm_queue_seq) {
			drmmode_crtc->drmmode->event_context.
				vblank_handler(pAMDGPUEnt->fd, 0, 0, 0,
					       (void*)event_info->drm_queue_seq);
			drmmode_crtc->wait_flip_nesting_level++;
			amdgpu_drm_queue_handle_deferred(crtc);

		} else {
			amdgpu_dri2_frame_event_handler(crtc, 0, 0, data);
		}

		return 0;
	}
	/*
	 * calculate the frame number from current time
	 * that would come from CRTC if it were running
	 */
	delta_t = drm_now - (CARD64) drmmode_crtc->dpms_last_ust;
	delta_seq = delta_t * drmmode_crtc->dpms_last_fps;
	delta_seq /= 1000000;
	frame = (CARD64) drmmode_crtc->dpms_last_seq + delta_seq;

	if (event_info->drm_queue_seq) {
		drmmode_crtc->drmmode->event_context.
			vblank_handler(pAMDGPUEnt->fd, frame, drm_now / 1000000,
				       drm_now % 1000000,
				       (void*)event_info->drm_queue_seq);
		drmmode_crtc->wait_flip_nesting_level++;
		amdgpu_drm_queue_handle_deferred(crtc);
	} else {
		amdgpu_dri2_frame_event_handler(crtc, frame, drm_now, data);
	}

	return 0;
}

static
void amdgpu_dri2_schedule_event(CARD32 delay, DRI2FrameEventPtr event_info)
{
	event_info->timer = TimerSet(NULL, 0, delay, amdgpu_dri2_deferred_event,
				     event_info);
	if (delay == 0) {
		CARD32 now = GetTimeInMillis();
		amdgpu_dri2_deferred_event(event_info->timer, now, event_info);
	}
}

/*
 * Request a DRM event when the requested conditions will be satisfied.
 *
 * We need to handle the event and ask the server to wake up the client when
 * we receive it.
 */
static int amdgpu_dri2_schedule_wait_msc(ClientPtr client, DrawablePtr draw,
					 CARD64 target_msc, CARD64 divisor,
					 CARD64 remainder)
{
	ScreenPtr screen = draw->pScreen;
	ScrnInfoPtr scrn = xf86ScreenToScrn(screen);
	DRI2FrameEventPtr wait_info = NULL;
	uintptr_t drm_queue_seq = 0;
	xf86CrtcPtr crtc = amdgpu_dri2_drawable_crtc(draw);
	uint32_t msc_delta;
	uint32_t seq;
	CARD64 current_msc;

	/* Truncate to match kernel interfaces; means occasional overflow
	 * misses, but that's generally not a big deal */
	target_msc &= 0xffffffff;
	divisor &= 0xffffffff;
	remainder &= 0xffffffff;

	/* Drawable not visible, return immediately */
	if (!crtc)
		goto out_complete;

	msc_delta = amdgpu_get_msc_delta(draw, crtc);

	wait_info = calloc(1, sizeof(DRI2FrameEventRec));
	if (!wait_info)
		goto out_complete;

	wait_info->drawable_id = draw->id;
	wait_info->client = client;
	wait_info->type = DRI2_WAITMSC;
	wait_info->crtc = crtc;

	/*
	 * CRTC is in DPMS off state, calculate wait time from current time,
	 * target_msc and last vblank time/sequence when CRTC was turned off
	 */
	if (!amdgpu_crtc_is_enabled(crtc)) {
		CARD32 delay;
		target_msc -= msc_delta;
		delay = amdgpu_dri2_extrapolate_msc_delay(crtc, &target_msc,
							  divisor, remainder);
		amdgpu_dri2_schedule_event(delay, wait_info);
		DRI2BlockClient(client, draw);
		return TRUE;
	}

	/* Get current count */
	if (!drmmode_wait_vblank(crtc, DRM_VBLANK_RELATIVE, 0, 0, NULL, &seq)) {
		xf86DrvMsg(scrn->scrnIndex, X_WARNING,
			   "get vblank counter failed: %s\n", strerror(errno));
		goto out_complete;
	}

	current_msc = seq + msc_delta;
	current_msc &= 0xffffffff;

	drm_queue_seq = amdgpu_drm_queue_alloc(crtc, client, AMDGPU_DRM_QUEUE_ID_DEFAULT,
					       wait_info, amdgpu_dri2_frame_event_handler,
					       amdgpu_dri2_frame_event_abort, FALSE);
	if (drm_queue_seq == AMDGPU_DRM_QUEUE_ERROR) {
		xf86DrvMsg(scrn->scrnIndex, X_WARNING,
			   "Allocating DRM queue event entry failed.\n");
		goto out_complete;
	}
	wait_info->drm_queue_seq = drm_queue_seq;

	/*
	 * If divisor is zero, or current_msc is smaller than target_msc,
	 * we just need to make sure target_msc passes  before waking up the
	 * client.
	 */
	if (divisor == 0 || current_msc < target_msc) {
		/* If target_msc already reached or passed, set it to
		 * current_msc to ensure we return a reasonable value back
		 * to the caller. This keeps the client from continually
		 * sending us MSC targets from the past by forcibly updating
		 * their count on this call.
		 */
		if (current_msc >= target_msc)
			target_msc = current_msc;
		if (!drmmode_wait_vblank(crtc, DRM_VBLANK_ABSOLUTE | DRM_VBLANK_EVENT,
					 target_msc - msc_delta, drm_queue_seq, NULL,
					 NULL)) {
			xf86DrvMsg(scrn->scrnIndex, X_WARNING,
				   "get vblank counter failed: %s\n",
				   strerror(errno));
			goto out_complete;
		}

		DRI2BlockClient(client, draw);
		return TRUE;
	}

	/*
	 * If we get here, target_msc has already passed or we don't have one,
	 * so we queue an event that will satisfy the divisor/remainder equation.
	 */
	target_msc = current_msc - (current_msc % divisor) + remainder - msc_delta;

	/*
	 * If calculated remainder is larger than requested remainder,
	 * it means we've passed the last point where
	 * seq % divisor == remainder, so we need to wait for the next time
	 * that will happen.
	 */
	if ((current_msc % divisor) >= remainder)
		target_msc += divisor;

	if (!drmmode_wait_vblank(crtc, DRM_VBLANK_ABSOLUTE | DRM_VBLANK_EVENT,
				 target_msc, drm_queue_seq, NULL, NULL)) {
		xf86DrvMsg(scrn->scrnIndex, X_WARNING,
			   "get vblank counter failed: %s\n", strerror(errno));
		goto out_complete;
	}

	DRI2BlockClient(client, draw);

	return TRUE;

out_complete:
	if (wait_info)
		amdgpu_dri2_deferred_event(NULL, 0, wait_info);
	else
		DRI2WaitMSCComplete(client, draw, 0, 0, 0);

	return TRUE;
}

/*
 * ScheduleSwap is responsible for requesting a DRM vblank event for the
 * appropriate frame.
 *
 * In the case of a blit (e.g. for a windowed swap) or buffer exchange,
 * the vblank requested can simply be the last queued swap frame + the swap
 * interval for the drawable.
 *
 * In the case of a page flip, we request an event for the last queued swap
 * frame + swap interval - 1, since we'll need to queue the flip for the frame
 * immediately following the received event.
 *
 * The client will be blocked if it tries to perform further GL commands
 * after queueing a swap, though in the Intel case after queueing a flip, the
 * client is free to queue more commands; they'll block in the kernel if
 * they access buffers busy with the flip.
 *
 * When the swap is complete, the driver should call into the server so it
 * can send any swap complete events that have been requested.
 */
static int amdgpu_dri2_schedule_swap(ClientPtr client, DrawablePtr draw,
				     DRI2BufferPtr front, DRI2BufferPtr back,
				     CARD64 * target_msc, CARD64 divisor,
				     CARD64 remainder, DRI2SwapEventPtr func,
				     void *data)
{
	ScreenPtr screen = draw->pScreen;
	ScrnInfoPtr scrn = xf86ScreenToScrn(screen);
	xf86CrtcPtr crtc = amdgpu_dri2_drawable_crtc(draw);
	uint32_t msc_delta;
	drmVBlankSeqType type;
	uint32_t seq;
	int flip = 0;
	DRI2FrameEventPtr swap_info = NULL;
	uintptr_t drm_queue_seq;
	CARD64 current_msc, event_msc;
	BoxRec box;
	RegionRec region;

	/* Truncate to match kernel interfaces; means occasional overflow
	 * misses, but that's generally not a big deal */
	*target_msc &= 0xffffffff;
	divisor &= 0xffffffff;
	remainder &= 0xffffffff;

	/* amdgpu_dri2_frame_event_handler will get called some unknown time in the
	 * future with these buffers.  Take a reference to ensure that they won't
	 * get destroyed before then.
	 */
	amdgpu_dri2_ref_buffer(front);
	amdgpu_dri2_ref_buffer(back);

	/* either off-screen or CRTC not usable... just complete the swap */
	if (!crtc)
		goto blit_fallback;

	msc_delta = amdgpu_get_msc_delta(draw, crtc);

	swap_info = calloc(1, sizeof(DRI2FrameEventRec));
	if (!swap_info)
		goto blit_fallback;

	swap_info->type = DRI2_SWAP;
	swap_info->drawable_id = draw->id;
	swap_info->client = client;
	swap_info->event_complete = func;
	swap_info->event_data = data;
	swap_info->front = front;
	swap_info->back = back;
	swap_info->crtc = crtc;

	drm_queue_seq = amdgpu_drm_queue_alloc(crtc, client, AMDGPU_DRM_QUEUE_ID_DEFAULT,
					       swap_info, amdgpu_dri2_frame_event_handler,
					       amdgpu_dri2_frame_event_abort, FALSE);
	if (drm_queue_seq == AMDGPU_DRM_QUEUE_ERROR) {
		xf86DrvMsg(scrn->scrnIndex, X_WARNING,
			   "Allocating DRM queue entry failed.\n");
		goto blit_fallback;
	}
	swap_info->drm_queue_seq = drm_queue_seq;

	/*
	 * CRTC is in DPMS off state, fallback to blit, but calculate
	 * wait time from current time, target_msc and last vblank
	 * time/sequence when CRTC was turned off
	 */
	if (!amdgpu_crtc_is_enabled(crtc)) {
		CARD32 delay;
		*target_msc -= msc_delta;
		delay = amdgpu_dri2_extrapolate_msc_delay(crtc, target_msc,
							  divisor, remainder);
		*target_msc += msc_delta;
		*target_msc &= 0xffffffff;
		amdgpu_dri2_schedule_event(delay, swap_info);
		return TRUE;
	}

	/* Get current count */
	if (!drmmode_wait_vblank(crtc, DRM_VBLANK_RELATIVE, 0, 0, NULL, &seq)) {
		xf86DrvMsg(scrn->scrnIndex, X_WARNING,
			   "first get vblank counter failed: %s\n",
			   strerror(errno));
		goto blit_fallback;
	}

	current_msc = seq + msc_delta;
	current_msc &= 0xffffffff;

	/* Flips need to be submitted one frame before */
	if (can_flip(crtc, draw, front, back)) {
		swap_info->type = DRI2_FLIP;
		flip = 1;
	}

	/* Correct target_msc by 'flip' if swap_info->type == DRI2_FLIP.
	 * Do it early, so handling of different timing constraints
	 * for divisor, remainder and msc vs. target_msc works.
	 */
	if (*target_msc > 0)
		*target_msc -= flip;

	/*
	 * If divisor is zero, or current_msc is smaller than target_msc
	 * we just need to make sure target_msc passes before initiating
	 * the swap.
	 */
	if (divisor == 0 || current_msc < *target_msc) {
		type = DRM_VBLANK_ABSOLUTE | DRM_VBLANK_EVENT;
		/* If non-pageflipping, but blitting/exchanging, we need to use
		 * DRM_VBLANK_NEXTONMISS to avoid unreliable timestamping later
		 * on.
		 */
		if (flip == 0)
			type |= DRM_VBLANK_NEXTONMISS;

		/* If target_msc already reached or passed, set it to
		 * current_msc to ensure we return a reasonable value back
		 * to the caller. This makes swap_interval logic more robust.
		 */
		if (current_msc >= *target_msc)
			*target_msc = current_msc;

		if (!drmmode_wait_vblank(crtc, type, *target_msc - msc_delta,
					 drm_queue_seq, NULL, &seq)) {
			xf86DrvMsg(scrn->scrnIndex, X_WARNING,
				   "divisor 0 get vblank counter failed: %s\n",
				   strerror(errno));
			goto blit_fallback;
		}

		*target_msc = seq + flip + msc_delta;
		*target_msc &= 0xffffffff;
		swap_info->frame = *target_msc;

		return TRUE;
	}

	/*
	 * If we get here, target_msc has already passed or we don't have one,
	 * and we need to queue an event that will satisfy the divisor/remainder
	 * equation.
	 */
	type = DRM_VBLANK_ABSOLUTE | DRM_VBLANK_EVENT;
	if (flip == 0)
		type |= DRM_VBLANK_NEXTONMISS;

	event_msc = current_msc - (current_msc % divisor) + remainder - msc_delta;

	/*
	 * If the calculated deadline vbl.request.sequence is smaller than
	 * or equal to current_msc, it means we've passed the last point
	 * when effective onset frame seq could satisfy
	 * seq % divisor == remainder, so we need to wait for the next time
	 * this will happen.

	 * This comparison takes the 1 frame swap delay in pageflipping mode
	 * into account, as well as a potential DRM_VBLANK_NEXTONMISS delay
	 * if we are blitting/exchanging instead of flipping.
	 */
	if (event_msc <= current_msc)
		event_msc += divisor;

	/* Account for 1 frame extra pageflip delay if flip > 0 */
	event_msc -= flip;

	if (!drmmode_wait_vblank(crtc, type, event_msc, drm_queue_seq, NULL, &seq)) {
		xf86DrvMsg(scrn->scrnIndex, X_WARNING,
			   "final get vblank counter failed: %s\n",
			   strerror(errno));
		goto blit_fallback;
	}

	/* Adjust returned value for 1 fame pageflip offset of flip > 0 */
	*target_msc = seq + flip + msc_delta;
	*target_msc &= 0xffffffff;
	swap_info->frame = *target_msc;

	return TRUE;

blit_fallback:
	if (swap_info) {
		swap_info->type = DRI2_SWAP;
		amdgpu_dri2_schedule_event(FALLBACK_SWAP_DELAY, swap_info);
	} else {
		box.x1 = 0;
		box.y1 = 0;
		box.x2 = draw->width;
		box.y2 = draw->height;
		REGION_INIT(pScreen, &region, &box, 0);

		amdgpu_dri2_copy_region2(draw->pScreen, draw, &region, front, back);

		DRI2SwapComplete(client, draw, 0, 0, 0, DRI2_BLIT_COMPLETE, func, data);

		amdgpu_dri2_unref_buffer(front);
		amdgpu_dri2_unref_buffer(back);
	}

	*target_msc = 0;	/* offscreen, so zero out target vblank count */
	return TRUE;
}

Bool amdgpu_dri2_screen_init(ScreenPtr pScreen)
{
	ScrnInfoPtr pScrn = xf86ScreenToScrn(pScreen);
	AMDGPUInfoPtr info = AMDGPUPTR(pScrn);
	AMDGPUEntPtr pAMDGPUEnt = AMDGPUEntPriv(pScrn);
	DRI2InfoRec dri2_info = { 0 };
	const char *driverNames[2];
	Bool scheduling_works = TRUE;

	if (!info->dri2.available)
		return FALSE;

	info->dri2.device_name = drmGetDeviceNameFromFd(pAMDGPUEnt->fd);

	dri2_info.driverName = SI_DRIVER_NAME;
	dri2_info.fd = pAMDGPUEnt->fd;
	dri2_info.deviceName = info->dri2.device_name;

	if (info->drmmode.count_crtcs > 2) {
		uint64_t cap_value;

		if (drmGetCap
		    (pAMDGPUEnt->fd, DRM_CAP_VBLANK_HIGH_CRTC, &cap_value)) {
			xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
				   "You need a newer kernel "
				   "for VBLANKs on CRTC > 1\n");
			scheduling_works = FALSE;
		} else if (!cap_value) {
			xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
				   "Your kernel does not "
				   "handle VBLANKs on CRTC > 1\n");
			scheduling_works = FALSE;
		}
	}

	if (scheduling_works) {
		dri2_info.ScheduleSwap = amdgpu_dri2_schedule_swap;
		dri2_info.GetMSC = amdgpu_dri2_get_msc;
		dri2_info.ScheduleWaitMSC = amdgpu_dri2_schedule_wait_msc;
		dri2_info.numDrivers = ARRAY_SIZE(driverNames);
		dri2_info.driverNames = driverNames;
		driverNames[0] = driverNames[1] = dri2_info.driverName;

		if (DRI2InfoCnt == 0) {
			if (!dixRegisterPrivateKey(dri2_window_private_key,
						   PRIVATE_WINDOW,
						   sizeof(struct dri2_window_priv))) {
				xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
					   "Failed to get DRI2 window private\n");
				return FALSE;
			}

			AddCallback(&ClientStateCallback,
				    amdgpu_dri2_client_state_changed, 0);
		}

		DRI2InfoCnt++;
	}

	dri2_info.version = 9;
	dri2_info.CreateBuffer2 = amdgpu_dri2_create_buffer2;
	dri2_info.DestroyBuffer2 = amdgpu_dri2_destroy_buffer2;
	dri2_info.CopyRegion2 = amdgpu_dri2_copy_region2;

	info->dri2.enabled = DRI2ScreenInit(pScreen, &dri2_info);
	return info->dri2.enabled;
}

void amdgpu_dri2_close_screen(ScreenPtr pScreen)
{
	ScrnInfoPtr pScrn = xf86ScreenToScrn(pScreen);
	AMDGPUInfoPtr info = AMDGPUPTR(pScrn);

	if (--DRI2InfoCnt == 0)
		DeleteCallback(&ClientStateCallback,
			       amdgpu_dri2_client_state_changed, 0);

	DRI2CloseScreen(pScreen);
	drmFree(info->dri2.device_name);
}

#endif /* DRI2 */

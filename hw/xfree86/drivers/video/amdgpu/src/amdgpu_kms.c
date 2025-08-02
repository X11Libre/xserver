/*
 * Copyright © 2009 Red Hat, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Authors:
 *    Dave Airlie <airlied@redhat.com>
 *
 */
#include "config.h"
#include <xorg-server.h>

#include <errno.h>
#include <sys/ioctl.h>

#include "fb.h"

/* Driver data structures */
#include "amdgpu_drv.h"
#include "amdgpu_bo_helper.h"
#include "amdgpu_drm_queue.h"
#include "amdgpu_glamor.h"
#include "amdgpu_probe.h"
#include "micmap.h"
#include "mipointrst.h"

#include "amdgpu_version.h"
#include "shadow.h"
#include <xf86Priv.h>

#include <present.h>

#include <X11/extensions/dpmsconst.h>
#include <X11/extensions/damageproto.h>

#include "amdgpu_bo_helper.h"
#include "amdgpu_pixmap.h"

#include <gbm.h>

static DevPrivateKeyRec amdgpu_window_private_key;
static DevScreenPrivateKeyRec amdgpu_client_private_key;
DevScreenPrivateKeyRec amdgpu_device_private_key;

static Atom amdgpu_vrr_atom;
static Bool amdgpu_property_vectors_wrapped;
static Bool restore_property_vector;
static int (*saved_change_property) (ClientPtr client);
static int (*saved_delete_property) (ClientPtr client);

static Bool amdgpu_setup_kernel_mem(ScreenPtr pScreen);

const OptionInfoRec AMDGPUOptions_KMS[] = {
	{OPTION_ACCEL, "Accel", OPTV_BOOLEAN, .value = {0}, FALSE},
	{OPTION_SW_CURSOR, "SWcursor", OPTV_BOOLEAN, .value = {0}, FALSE},
	{OPTION_PAGE_FLIP, "EnablePageFlip", OPTV_BOOLEAN, .value = {0}, FALSE},
	{OPTION_SUBPIXEL_ORDER, "SubPixelOrder", OPTV_ANYSTR, .value = {0}, FALSE},
	{OPTION_ZAPHOD_HEADS, "ZaphodHeads", OPTV_STRING, .value = {0}, FALSE},
	{OPTION_ACCEL_METHOD, "AccelMethod", OPTV_STRING, .value = {0}, FALSE},
	{OPTION_DRI3, "DRI3", OPTV_BOOLEAN, .value = {0}, FALSE},
	{OPTION_DRI, "DRI", OPTV_INTEGER, .value = {0}, FALSE},
	{OPTION_SHADOW_PRIMARY, "ShadowPrimary", OPTV_BOOLEAN, .value = {0}, FALSE},
	{OPTION_TEAR_FREE, "TearFree", OPTV_BOOLEAN, .value = {0}, FALSE},
	{OPTION_DELETE_DP12, "DeleteUnusedDP12Displays", OPTV_BOOLEAN, .value = {0}, FALSE},
	{OPTION_VARIABLE_REFRESH, "VariableRefresh", OPTV_BOOLEAN, .value = {0}, FALSE },
	{OPTION_ASYNC_FLIP_SECONDARIES, "AsyncFlipSecondaries", OPTV_BOOLEAN, .value = {0}, FALSE},
	{-1, NULL, OPTV_NONE, .value = {0}, FALSE}
};

const OptionInfoRec *AMDGPUOptionsWeak(void)
{
	return AMDGPUOptions_KMS;
}

static inline struct amdgpu_window_priv *get_window_priv(WindowPtr win) {
	return dixLookupPrivate(&win->devPrivates, &amdgpu_window_private_key);
}

static void
amdgpu_vrr_property_update(WindowPtr window, Bool variable_refresh)
{
	ScrnInfoPtr scrn = xf86ScreenToScrn(window->drawable.pScreen);
	AMDGPUInfoPtr info = AMDGPUPTR(scrn);

	get_window_priv(window)->variable_refresh = variable_refresh;

	if (info->flip_window == window &&
	    info->drmmode.present_flipping)
		amdgpu_present_set_screen_vrr(scrn, variable_refresh);
}

/* Wrapper for xserver/dix/property.c:ProcChangeProperty */
static int
amdgpu_change_property(ClientPtr client)
{
	WindowPtr window;
	int ret;

	REQUEST(xChangePropertyReq);

	client->requestVector[X_ChangeProperty] = saved_change_property;
	ret = saved_change_property(client);

	if (restore_property_vector)
		return ret;

	client->requestVector[X_ChangeProperty] = amdgpu_change_property;

	if (ret != Success)
		return ret;

	ret = dixLookupWindow(&window, stuff->window, client, DixSetPropAccess);
	if (ret != Success)
		return ret;

	if (stuff->property == amdgpu_vrr_atom &&
	    xf86ScreenToScrn(window->drawable.pScreen)->PreInit ==
	    AMDGPUPreInit_KMS && stuff->format == 32 && stuff->nUnits == 1) {
		uint32_t *value = (uint32_t*)(stuff + 1);

		amdgpu_vrr_property_update(window, *value != 0);
	}

	return ret;
}

/* Wrapper for xserver/dix/property.c:ProcDeleteProperty */
static int
amdgpu_delete_property(ClientPtr client)
{
	WindowPtr window;
	int ret;

	REQUEST(xDeletePropertyReq);

	client->requestVector[X_DeleteProperty] = saved_delete_property;
	ret = saved_delete_property(client);

	if (restore_property_vector)
		return ret;

	client->requestVector[X_DeleteProperty] = amdgpu_delete_property;

	if (ret != Success)
		return ret;

	ret = dixLookupWindow(&window, stuff->window, client, DixSetPropAccess);
	if (ret != Success)
		return ret;

	if (stuff->property == amdgpu_vrr_atom &&
	    xf86ScreenToScrn(window->drawable.pScreen)->PreInit ==
	    AMDGPUPreInit_KMS)
		amdgpu_vrr_property_update(window, FALSE);

	return ret;
}

static void
amdgpu_unwrap_property_requests(ScrnInfoPtr scrn)
{
	int i;

	if (!amdgpu_property_vectors_wrapped)
		return;

	if (ProcVector[X_ChangeProperty] == amdgpu_change_property)
		ProcVector[X_ChangeProperty] = saved_change_property;
	else
		restore_property_vector = TRUE;

	if (ProcVector[X_DeleteProperty] == amdgpu_delete_property)
		ProcVector[X_DeleteProperty] = saved_delete_property;
	else
		restore_property_vector = TRUE;

	for (i = 0; i < currentMaxClients; i++) {
		if (clients[i]->requestVector[X_ChangeProperty] ==
		    amdgpu_change_property) {
			clients[i]->requestVector[X_ChangeProperty] =
				saved_change_property;
		} else {
			restore_property_vector = TRUE;
		}

		if (clients[i]->requestVector[X_DeleteProperty] ==
		    amdgpu_delete_property) {
			clients[i]->requestVector[X_DeleteProperty] =
				saved_delete_property;
		} else {
			restore_property_vector = TRUE;
		}
	}

	if (restore_property_vector) {
		xf86DrvMsg(scrn->scrnIndex, X_WARNING,
			   "Couldn't unwrap some window property request vectors\n");
	}

	amdgpu_property_vectors_wrapped = FALSE;
}

extern _X_EXPORT int gAMDGPUEntityIndex;

static int getAMDGPUEntityIndex(void)
{
	return gAMDGPUEntityIndex;
}

AMDGPUEntPtr AMDGPUEntPriv(ScrnInfoPtr pScrn)
{
	DevUnion *pPriv;
	AMDGPUInfoPtr info = AMDGPUPTR(pScrn);
	pPriv = xf86GetEntityPrivate(info->pEnt->index, getAMDGPUEntityIndex());
	return pPriv->ptr;
}

/* Allocate our private AMDGPUInfoRec */
static Bool AMDGPUGetRec(ScrnInfoPtr pScrn)
{
	if (pScrn->driverPrivate)
		return TRUE;

	pScrn->driverPrivate = XNFcallocarray(sizeof(AMDGPUInfoRec), 1);
	return TRUE;
}

/* Free our private AMDGPUInfoRec */
static void AMDGPUFreeRec(ScrnInfoPtr pScrn)
{
	DevUnion *pPriv;
	AMDGPUEntPtr pAMDGPUEnt;
	AMDGPUInfoPtr info;
	EntityInfoPtr pEnt;

	if (!pScrn)
		return;

	pEnt = xf86GetEntityInfo(pScrn->entityList[pScrn->numEntities - 1]);
	pPriv = xf86GetEntityPrivate(pEnt->index, gAMDGPUEntityIndex);
	pAMDGPUEnt = pPriv->ptr;

	info = AMDGPUPTR(pScrn);
	if (info) {
		pAMDGPUEnt->scrn[info->instance_id] = NULL;
		pAMDGPUEnt->num_scrns--;
		free(pScrn->driverPrivate);
		pScrn->driverPrivate = NULL;
	}

	if (pAMDGPUEnt->fd > 0) {
		DevUnion *pPriv;
		AMDGPUEntPtr pAMDGPUEnt;
		pPriv = xf86GetEntityPrivate(pScrn->entityList[0],
					     getAMDGPUEntityIndex());

		pAMDGPUEnt = pPriv->ptr;
		pAMDGPUEnt->fd_ref--;
		if (!pAMDGPUEnt->fd_ref) {
			amdgpu_unwrap_property_requests(pScrn);
			amdgpu_device_deinitialize(pAMDGPUEnt->pDev);
			amdgpu_kernel_close_fd(pAMDGPUEnt);
			free(pAMDGPUEnt->busid);
			free(pPriv->ptr);
			pPriv->ptr = NULL;
		}
	}

	free(pEnt);
}

Bool amdgpu_window_has_variable_refresh(WindowPtr win) {
	struct amdgpu_window_priv *priv = get_window_priv(win);

	return priv->variable_refresh;
}

static void *amdgpuShadowWindow(ScreenPtr screen, CARD32 row, CARD32 offset,
				int mode, CARD32 * size, void *closure)
{
	ScrnInfoPtr pScrn = xf86ScreenToScrn(screen);
	AMDGPUInfoPtr info = AMDGPUPTR(pScrn);
	int stride;

	stride = (pScrn->displayWidth * pScrn->bitsPerPixel) / 8;
	*size = stride;

	return ((uint8_t *) info->front_buffer->cpu_ptr + row * stride + offset);
}

static void
amdgpuUpdatePacked(ScreenPtr pScreen, shadowBufPtr pBuf)
{
	shadowUpdatePacked(pScreen, pBuf);
}

static Bool
callback_needs_flush(AMDGPUInfoPtr info, struct amdgpu_client_priv *client_priv)
{
	return (int)(client_priv->needs_flush - info->gpu_flushed) > 0;
}

static void
amdgpu_event_callback(CallbackListPtr *list,
		      pointer user_data, pointer call_data)
{
	EventInfoRec *eventinfo = call_data;
	ScrnInfoPtr pScrn = user_data;
	ScreenPtr pScreen = pScrn->pScreen;
	struct amdgpu_client_priv *client_priv =
		dixLookupScreenPrivate(&eventinfo->client->devPrivates,
				       &amdgpu_client_private_key, pScreen);
	struct amdgpu_client_priv *server_priv =
		dixLookupScreenPrivate(&serverClient->devPrivates,
				       &amdgpu_client_private_key, pScreen);
	AMDGPUInfoPtr info = AMDGPUPTR(pScrn);
	int i;

	if (callback_needs_flush(info, client_priv) ||
	    callback_needs_flush(info, server_priv))
		return;

	/* Don't let gpu_flushed get too far ahead of needs_flush, in order
	 * to prevent false positives in callback_needs_flush()
	 */
	client_priv->needs_flush = info->gpu_flushed;
	server_priv->needs_flush = info->gpu_flushed;

	for (i = 0; i < eventinfo->count; i++) {
		if (eventinfo->events[i].u.u.type == info->callback_event_type) {
			client_priv->needs_flush++;
			server_priv->needs_flush++;
			return;
		}
	}
}

static void
amdgpu_flush_callback(CallbackListPtr *list,
		      pointer user_data, pointer call_data)
{
	ScrnInfoPtr pScrn = user_data;
	ScreenPtr pScreen = pScrn->pScreen;
	ClientPtr client = call_data ? call_data : serverClient;
	struct amdgpu_client_priv *client_priv =
		dixLookupScreenPrivate(&client->devPrivates,
				       &amdgpu_client_private_key, pScreen);
	AMDGPUInfoPtr info = AMDGPUPTR(pScrn);

	if (pScrn->vtSema && callback_needs_flush(info, client_priv))
		amdgpu_glamor_flush(pScrn);
}

static Bool AMDGPUCreateScreenResources_KMS(ScreenPtr pScreen)
{
	ExtensionEntry *damage_ext;
	ScrnInfoPtr pScrn = xf86ScreenToScrn(pScreen);
	AMDGPUInfoPtr info = AMDGPUPTR(pScrn);
	PixmapPtr pixmap;

	pScreen->CreateScreenResources = info->CreateScreenResources;
	if (!(*pScreen->CreateScreenResources) (pScreen))
		return FALSE;
	pScreen->CreateScreenResources = AMDGPUCreateScreenResources_KMS;

	/* Set the RandR primary output if Xorg hasn't */
	if (dixPrivateKeyRegistered(rrPrivKey)) {
		rrScrPrivPtr rrScrPriv = rrGetScrPriv(pScreen);

		if (!pScreen->isGPU && !rrScrPriv->primaryOutput) {
			xf86CrtcConfigPtr xf86_config = XF86_CRTC_CONFIG_PTR(pScrn);

			rrScrPriv->primaryOutput = xf86_config->output[0]->randr_output;
			RROutputChanged(rrScrPriv->primaryOutput, FALSE);
			rrScrPriv->layoutChanged = TRUE;
		}

		drmmode_uevent_init(pScrn, &info->drmmode);
	}

	if (!drmmode_set_desired_modes(pScrn, &info->drmmode, pScreen->isGPU))
		return FALSE;

	if (info->shadow_fb) {
		pixmap = pScreen->GetScreenPixmap(pScreen);

		if (!shadowAdd(pScreen, pixmap, amdgpuUpdatePacked,
			       amdgpuShadowWindow, 0, NULL))
			return FALSE;
	}

	if (info->dri2.enabled || info->use_glamor) {
		if (info->front_buffer) {
			PixmapPtr pPix = pScreen->GetScreenPixmap(pScreen);

			if (!amdgpu_set_pixmap_bo(pPix, info->front_buffer))
				return FALSE;
		}
	}

	if (info->use_glamor)
		amdgpu_glamor_create_screen_resources(pScreen);

	info->callback_event_type = -1;
	if (!pScreen->isGPU && (damage_ext = CheckExtension("DAMAGE"))) {
		info->callback_event_type = damage_ext->eventBase + XDamageNotify;

		if (!AddCallback(&FlushCallback, amdgpu_flush_callback, pScrn))
			return FALSE;

		if (!AddCallback(&EventCallback, amdgpu_event_callback, pScrn)) {
			DeleteCallback(&FlushCallback, amdgpu_flush_callback, pScrn);
			return FALSE;
		}

		if (!dixRegisterScreenPrivateKey(&amdgpu_client_private_key, pScreen,
						 PRIVATE_CLIENT, sizeof(struct amdgpu_client_priv))) {
			DeleteCallback(&FlushCallback, amdgpu_flush_callback, pScrn);
			DeleteCallback(&EventCallback, amdgpu_event_callback, pScrn);
			return FALSE;
		}
	}

	if (info->vrr_support &&
	    !dixRegisterPrivateKey(&amdgpu_window_private_key,
				   PRIVATE_WINDOW,
				   sizeof(struct amdgpu_window_priv)))
		return FALSE;

	return TRUE;
}

static Bool
amdgpu_scanout_extents_intersect(xf86CrtcPtr xf86_crtc, BoxPtr extents)
{
	if (xf86_crtc->scrn->is_gpu) {
		extents->x1 -= xf86_crtc->x;
		extents->y1 -= xf86_crtc->y;
		extents->x2 -= xf86_crtc->x;
		extents->y2 -= xf86_crtc->y;
	} else {
		extents->x1 -= xf86_crtc->filter_width >> 1;
		extents->x2 += xf86_crtc->filter_width >> 1;
		extents->y1 -= xf86_crtc->filter_height >> 1;
		extents->y2 += xf86_crtc->filter_height >> 1;
		pixman_f_transform_bounds(&xf86_crtc->f_framebuffer_to_crtc, extents);
	}

	extents->x1 = max(extents->x1, 0);
	extents->y1 = max(extents->y1, 0);
	extents->x2 = min(extents->x2, xf86_crtc->mode.HDisplay);
	extents->y2 = min(extents->y2, xf86_crtc->mode.VDisplay);

	return (extents->x1 < extents->x2 && extents->y1 < extents->y2);
}

static RegionPtr
transform_region(RegionPtr region, struct pixman_f_transform *transform,
		 int w, int h)
{
	BoxPtr boxes = RegionRects(region);
	int nboxes = RegionNumRects(region);
	xRectanglePtr rects = malloc(nboxes * sizeof(*rects));
	RegionPtr transformed;
	int nrects = 0;
	BoxRec box;
	int i;

	for (i = 0; i < nboxes; i++) {
		box.x1 = boxes[i].x1;
		box.x2 = boxes[i].x2;
		box.y1 = boxes[i].y1;
		box.y2 = boxes[i].y2;
		pixman_f_transform_bounds(transform, &box);

		box.x1 = max(box.x1, 0);
		box.y1 = max(box.y1, 0);
		box.x2 = min(box.x2, w);
		box.y2 = min(box.y2, h);
		if (box.x1 >= box.x2 || box.y1 >= box.y2)
			continue;

		rects[nrects].x = box.x1;
		rects[nrects].y = box.y1;
		rects[nrects].width = box.x2 - box.x1;
		rects[nrects].height = box.y2 - box.y1;
		nrects++;
	}

	transformed = RegionFromRects(nrects, rects, CT_UNSORTED);
	free(rects);
	return transformed;
}

static void
amdgpu_sync_scanout_pixmaps(xf86CrtcPtr xf86_crtc, RegionPtr new_region,
							int scanout_id)
{
	drmmode_crtc_private_ptr drmmode_crtc = xf86_crtc->driver_private;
	DrawablePtr dst = &drmmode_crtc->scanout[scanout_id]->drawable;
	DrawablePtr src = &drmmode_crtc->scanout[scanout_id ^ 1]->drawable;
	RegionPtr last_region = &drmmode_crtc->scanout_last_region;
	ScrnInfoPtr scrn = xf86_crtc->scrn;
	ScreenPtr pScreen = scrn->pScreen;
	RegionRec remaining;
	RegionPtr sync_region = NULL;
	BoxRec extents;
	GCPtr gc;

	if (RegionNil(last_region))
		return;

	RegionNull(&remaining);
	RegionSubtract(&remaining, last_region, new_region);
	if (RegionNil(&remaining))
		goto uninit;

	extents = *RegionExtents(&remaining);
	if (!amdgpu_scanout_extents_intersect(xf86_crtc, &extents))
		goto uninit;

	if (xf86_crtc->driverIsPerformingTransform) {
		sync_region = transform_region(&remaining,
					       &xf86_crtc->f_framebuffer_to_crtc,
					       dst->width, dst->height);
	} else {
		sync_region = RegionDuplicate(&remaining);
		RegionTranslate(sync_region, -xf86_crtc->x, -xf86_crtc->y);
	}

	gc = GetScratchGC(dst->depth, pScreen);
	if (gc) {
		gc->funcs->ChangeClip(gc, CT_REGION, sync_region, 0);
		ValidateGC(dst, gc);
		sync_region = NULL;
		gc->ops->CopyArea(src, dst, gc, 0, 0, dst->width, dst->height, 0, 0);
		FreeScratchGC(gc);
	}

 uninit:
	if (sync_region)
		RegionDestroy(sync_region);
	RegionUninit(&remaining);
}

static void
amdgpu_scanout_flip_abort(xf86CrtcPtr crtc, void *event_data)
{
	AMDGPUEntPtr pAMDGPUEnt = AMDGPUEntPriv(crtc->scrn);
	drmmode_crtc_private_ptr drmmode_crtc = crtc->driver_private;
	struct drmmode_fb *fb = event_data;

	drmmode_crtc->scanout_update_pending = 0;

	if (drmmode_crtc->flip_pending == fb) {
		drmmode_fb_reference(pAMDGPUEnt->fd, &drmmode_crtc->flip_pending,
				     NULL);
	}
}

static void
amdgpu_scanout_flip_handler(xf86CrtcPtr crtc, uint32_t msc, uint64_t usec,
			    void *event_data)
{
	AMDGPUEntPtr pAMDGPUEnt = AMDGPUEntPriv(crtc->scrn);
	drmmode_crtc_private_ptr drmmode_crtc = crtc->driver_private;
	struct drmmode_fb *fb = event_data;

	drmmode_fb_reference(pAMDGPUEnt->fd, &drmmode_crtc->fb, fb);
	amdgpu_scanout_flip_abort(crtc, event_data);
}


static RegionPtr
dirty_region(PixmapDirtyUpdatePtr dirty)
{
	RegionPtr damageregion = DamageRegion(dirty->damage);
	RegionPtr dstregion;

	if (dirty->rotation != RR_Rotate_0) {
		dstregion = transform_region(damageregion,
					     &dirty->f_inverse,
					     dirty->secondary_dst->drawable.width,
					     dirty->secondary_dst->drawable.height);
	} else
	{
		RegionRec pixregion;

		dstregion = RegionDuplicate(damageregion);
		RegionTranslate(dstregion, -dirty->x, -dirty->y);
		PixmapRegionInit(&pixregion, dirty->secondary_dst);
		RegionIntersect(dstregion, dstregion, &pixregion);
		RegionUninit(&pixregion);
	}

	return dstregion;
}

static void
redisplay_dirty(PixmapDirtyUpdatePtr dirty, RegionPtr region)
{
	ScrnInfoPtr src_scrn =
		xf86ScreenToScrn(amdgpu_dirty_src_drawable(dirty)->pScreen);

	if (RegionNil(region))
		goto out;

	if (dirty->secondary_dst->primary_pixmap)
		DamageRegionAppend(&dirty->secondary_dst->drawable, region);

	PixmapSyncDirtyHelper(dirty);

	amdgpu_glamor_flush(src_scrn);
	if (dirty->secondary_dst->primary_pixmap)
		DamageRegionProcessPending(&dirty->secondary_dst->drawable);

out:
	DamageEmpty(dirty->damage);
}

static void
amdgpu_prime_scanout_update_abort(xf86CrtcPtr crtc, void *event_data)
{
	drmmode_crtc_private_ptr drmmode_crtc = crtc->driver_private;

	drmmode_crtc->scanout_update_pending = 0;
}

void
amdgpu_sync_shared_pixmap(PixmapDirtyUpdatePtr dirty)
{
	ScreenPtr primary_screen = amdgpu_dirty_primary(dirty);
	PixmapDirtyUpdatePtr ent;
	RegionPtr region;

	xorg_list_for_each_entry(ent, &primary_screen->pixmap_dirty_list, ent) {
		if (!amdgpu_dirty_src_equals(dirty, ent->secondary_dst))
			continue;

		region = dirty_region(ent);
		redisplay_dirty(ent, region);
		RegionDestroy(region);
	}
}

static Bool
primary_has_sync_shared_pixmap(ScrnInfoPtr scrn, PixmapDirtyUpdatePtr dirty)
{
	ScreenPtr primary_screen = amdgpu_dirty_primary(dirty);

	return primary_screen->SyncSharedPixmap != NULL;
}

static Bool
secondary_has_sync_shared_pixmap(ScrnInfoPtr scrn, PixmapDirtyUpdatePtr dirty)
{
	ScreenPtr secondary_screen = dirty->secondary_dst->drawable.pScreen;

	return secondary_screen->SyncSharedPixmap != NULL;
}

static void
call_sync_shared_pixmap(PixmapDirtyUpdatePtr dirty)
{
	ScreenPtr primary_screen = amdgpu_dirty_primary(dirty);

	primary_screen->SyncSharedPixmap(dirty);
}

static xf86CrtcPtr
amdgpu_prime_dirty_to_crtc(PixmapDirtyUpdatePtr dirty)
{
	ScreenPtr screen = dirty->secondary_dst->drawable.pScreen;
	ScrnInfoPtr scrn = xf86ScreenToScrn(screen);
	xf86CrtcConfigPtr xf86_config = XF86_CRTC_CONFIG_PTR(scrn);
	int c;

	/* Find the CRTC which is scanning out from this secondary pixmap */
	for (c = 0; c < xf86_config->num_crtc; c++) {
		xf86CrtcPtr xf86_crtc = xf86_config->crtc[c];
		drmmode_crtc_private_ptr drmmode_crtc = xf86_crtc->driver_private;

		if (amdgpu_dirty_src_equals(dirty, drmmode_crtc->prime_scanout_pixmap))
			return xf86_crtc;
	}

	return NULL;
}

static Bool
amdgpu_prime_scanout_do_update(xf86CrtcPtr crtc, unsigned scanout_id)
{
	ScrnInfoPtr scrn = crtc->scrn;
	ScreenPtr screen = scrn->pScreen;
	drmmode_crtc_private_ptr drmmode_crtc = crtc->driver_private;
	PixmapDirtyUpdatePtr dirty;
	Bool ret = FALSE;

	xorg_list_for_each_entry(dirty, &screen->pixmap_dirty_list, ent) {
		if (amdgpu_dirty_src_equals(dirty, drmmode_crtc->prime_scanout_pixmap)) {
			RegionPtr region;

			if (primary_has_sync_shared_pixmap(scrn, dirty))
				call_sync_shared_pixmap(dirty);

			region = dirty_region(dirty);
			if (RegionNil(region))
				goto destroy;

			if (drmmode_crtc->tear_free) {
				RegionTranslate(region, crtc->x, crtc->y);
				amdgpu_sync_scanout_pixmaps(crtc, region, scanout_id);
				amdgpu_glamor_flush(scrn);
				RegionCopy(&drmmode_crtc->scanout_last_region, region);
				RegionTranslate(region, -crtc->x, -crtc->y);
				dirty->secondary_dst = drmmode_crtc->scanout[scanout_id];
			}

			redisplay_dirty(dirty, region);
			ret = TRUE;
		destroy:
			RegionDestroy(region);
			break;
		}
	}

	return ret;
}

static void
amdgpu_prime_scanout_update_handler(xf86CrtcPtr crtc, uint32_t frame, uint64_t usec,
				     void *event_data)
{
	drmmode_crtc_private_ptr drmmode_crtc = crtc->driver_private;

	amdgpu_prime_scanout_do_update(crtc, 0);
	drmmode_crtc->scanout_update_pending = 0;
}

static void
amdgpu_prime_scanout_update(PixmapDirtyUpdatePtr dirty)
{
	ScreenPtr screen = dirty->secondary_dst->drawable.pScreen;
	ScrnInfoPtr scrn = xf86ScreenToScrn(screen);
	AMDGPUEntPtr pAMDGPUEnt = AMDGPUEntPriv(scrn);
	xf86CrtcPtr xf86_crtc = amdgpu_prime_dirty_to_crtc(dirty);
	drmmode_crtc_private_ptr drmmode_crtc;
	uintptr_t drm_queue_seq;

	if (!xf86_crtc || !xf86_crtc->enabled)
		return;

	drmmode_crtc = xf86_crtc->driver_private;
	if (drmmode_crtc->scanout_update_pending ||
	    !drmmode_crtc->scanout[drmmode_crtc->scanout_id] ||
	    drmmode_crtc->dpms_mode != DPMSModeOn)
		return;

	drm_queue_seq = amdgpu_drm_queue_alloc(xf86_crtc,
					       AMDGPU_DRM_QUEUE_CLIENT_DEFAULT,
					       AMDGPU_DRM_QUEUE_ID_DEFAULT, NULL,
					       amdgpu_prime_scanout_update_handler,
					       amdgpu_prime_scanout_update_abort,
					       FALSE);
	if (drm_queue_seq == AMDGPU_DRM_QUEUE_ERROR) {
		xf86DrvMsg(scrn->scrnIndex, X_WARNING,
			   "amdgpu_drm_queue_alloc failed for PRIME update\n");
		amdgpu_prime_scanout_update_handler(xf86_crtc, 0, 0, NULL);
		return;
	}

	drmmode_crtc->scanout_update_pending = drm_queue_seq;

	if (!drmmode_wait_vblank(xf86_crtc, DRM_VBLANK_RELATIVE | DRM_VBLANK_EVENT,
				 1, drm_queue_seq, NULL, NULL)) {
		if (!(drmmode_crtc->scanout_status & DRMMODE_SCANOUT_VBLANK_FAILED)) {
			xf86DrvMsg(scrn->scrnIndex, X_WARNING,
				   "drmmode_wait_vblank failed for PRIME update: %s\n",
				   strerror(errno));
			drmmode_crtc->scanout_status |= DRMMODE_SCANOUT_VBLANK_FAILED;
		}

		drmmode_crtc->drmmode->event_context.vblank_handler(pAMDGPUEnt->fd,
								    0, 0, 0,
								    (void*)drm_queue_seq);
		drmmode_crtc->wait_flip_nesting_level++;
		amdgpu_drm_queue_handle_deferred(xf86_crtc);
		return;
	}

	if (drmmode_crtc->scanout_status ==
	    (DRMMODE_SCANOUT_FLIP_FAILED | DRMMODE_SCANOUT_VBLANK_FAILED)) {
		/* The page flip and vblank ioctls failed before, but the vblank
		 * ioctl is working again, so we can try re-enabling TearFree
		 */
		xf86_crtc->funcs->set_mode_major(xf86_crtc, &xf86_crtc->mode,
						 xf86_crtc->rotation,
						 xf86_crtc->x, xf86_crtc->y);
	}

	drmmode_crtc->scanout_status &= ~DRMMODE_SCANOUT_VBLANK_FAILED;
}

static void
amdgpu_prime_scanout_flip(PixmapDirtyUpdatePtr ent)
{
	ScreenPtr screen = ent->secondary_dst->drawable.pScreen;
	ScrnInfoPtr scrn = xf86ScreenToScrn(screen);
	AMDGPUEntPtr pAMDGPUEnt = AMDGPUEntPriv(scrn);
	xf86CrtcPtr crtc = amdgpu_prime_dirty_to_crtc(ent);
	drmmode_crtc_private_ptr drmmode_crtc;
	uintptr_t drm_queue_seq;
	unsigned scanout_id;
	struct drmmode_fb *fb;

	if (!crtc || !crtc->enabled)
		return;

	drmmode_crtc = crtc->driver_private;
	scanout_id = drmmode_crtc->scanout_id ^ 1;
	if (drmmode_crtc->scanout_update_pending ||
	    !drmmode_crtc->scanout[scanout_id] ||
	    drmmode_crtc->dpms_mode != DPMSModeOn)
		return;

	if (!amdgpu_prime_scanout_do_update(crtc, scanout_id))
		return;

	fb = amdgpu_pixmap_get_fb(drmmode_crtc->scanout[scanout_id]);
	if (!fb) {
		xf86DrvMsg(scrn->scrnIndex, X_WARNING,
			   "Failed to get FB for PRIME flip.\n");
		return;
	}

	drm_queue_seq = amdgpu_drm_queue_alloc(crtc,
					       AMDGPU_DRM_QUEUE_CLIENT_DEFAULT,
					       AMDGPU_DRM_QUEUE_ID_DEFAULT, fb,
					       amdgpu_scanout_flip_handler,
					       amdgpu_scanout_flip_abort, TRUE);
	if (drm_queue_seq == AMDGPU_DRM_QUEUE_ERROR) {
		xf86DrvMsg(scrn->scrnIndex, X_WARNING,
			   "Allocating DRM event queue entry failed for PRIME flip.\n");
		return;
	}

	if (drmmode_page_flip_target_relative(pAMDGPUEnt, drmmode_crtc,
					      fb->handle, 0, drm_queue_seq, 1)
	    != 0) {
		if (!(drmmode_crtc->scanout_status & DRMMODE_SCANOUT_FLIP_FAILED)) {
			xf86DrvMsg(scrn->scrnIndex, X_WARNING,
				   "flip queue failed in %s: %s, TearFree inactive\n",
				   __func__, strerror(errno));
			drmmode_crtc->scanout_status |= DRMMODE_SCANOUT_FLIP_FAILED;
		}

		amdgpu_drm_abort_entry(drm_queue_seq);
		return;
	}

	if (drmmode_crtc->scanout_status & DRMMODE_SCANOUT_FLIP_FAILED) {
		xf86DrvMsg(scrn->scrnIndex, X_INFO, "TearFree active again\n");
		drmmode_crtc->scanout_status &= ~DRMMODE_SCANOUT_FLIP_FAILED;
	}

	drmmode_crtc->scanout_id = scanout_id;
	drmmode_crtc->scanout_update_pending = drm_queue_seq;
	drmmode_fb_reference(pAMDGPUEnt->fd, &drmmode_crtc->flip_pending, fb);
}

static void
amdgpu_dirty_update(ScrnInfoPtr scrn)
{
	ScreenPtr screen = scrn->pScreen;
	PixmapDirtyUpdatePtr ent;
	RegionPtr region;

	xorg_list_for_each_entry(ent, &screen->pixmap_dirty_list, ent) {
		if (screen->isGPU) {
			PixmapDirtyUpdatePtr region_ent = ent;

			if (primary_has_sync_shared_pixmap(scrn, ent)) {
				ScreenPtr primary_screen = amdgpu_dirty_primary(ent);

				xorg_list_for_each_entry(region_ent, &primary_screen->pixmap_dirty_list, ent) {
					if (amdgpu_dirty_src_equals(ent, region_ent->secondary_dst))
						break;
				}
			}

			region = dirty_region(region_ent);

			if (RegionNotEmpty(region)) {
				xf86CrtcPtr crtc = amdgpu_prime_dirty_to_crtc(ent);
				drmmode_crtc_private_ptr drmmode_crtc = NULL;

				if (crtc)
					drmmode_crtc = crtc->driver_private;

				if (drmmode_crtc && drmmode_crtc->tear_free)
					amdgpu_prime_scanout_flip(ent);
				else
					amdgpu_prime_scanout_update(ent);
			} else {
				DamageEmpty(region_ent->damage);
			}

			RegionDestroy(region);
		} else {
			if (secondary_has_sync_shared_pixmap(scrn, ent))
				continue;

			region = dirty_region(ent);
			redisplay_dirty(ent, region);
			RegionDestroy(region);
		}
	}
}

static void
amdgpuSourceValidate(DrawablePtr draw, int x, int y, int w, int h,
		     unsigned int subWindowMode)
{
}

Bool
amdgpu_scanout_do_update(xf86CrtcPtr xf86_crtc, int scanout_id,
			 PixmapPtr src_pix, BoxRec extents)
{
	drmmode_crtc_private_ptr drmmode_crtc = xf86_crtc->driver_private;
	RegionRec region = { .extents = extents, .data = NULL };
	ScrnInfoPtr scrn = xf86_crtc->scrn;
	ScreenPtr pScreen = scrn->pScreen;
	DrawablePtr pDraw;

	if (!xf86_crtc->enabled ||
	    !drmmode_crtc->scanout[scanout_id] ||
	    extents.x1 >= extents.x2 || extents.y1 >= extents.y2)
		return FALSE;

	pDraw = &drmmode_crtc->scanout[scanout_id]->drawable;
	if (!amdgpu_scanout_extents_intersect(xf86_crtc, &extents))
		return FALSE;

	if (drmmode_crtc->tear_free) {
		amdgpu_sync_scanout_pixmaps(xf86_crtc, &region, scanout_id);
		RegionCopy(&drmmode_crtc->scanout_last_region, &region);
	}

	if (xf86_crtc->driverIsPerformingTransform) {
		SourceValidateProcPtr SourceValidate = pScreen->SourceValidate;
		PictFormatPtr format = PictureWindowFormat(pScreen->root);
		int error;
		PicturePtr src, dst;

		src = CreatePicture(None, &src_pix->drawable, format, 0L, NULL,
				    serverClient, &error);
		if (!src) {
			ErrorF("Failed to create source picture for transformed scanout "
			       "update\n");
			goto out;
		}

		dst = CreatePicture(None, pDraw, format, 0L, NULL, serverClient, &error);
		if (!dst) {
			ErrorF("Failed to create destination picture for transformed scanout "
			       "update\n");
			goto free_src;
		}
		error = SetPictureTransform(src, &xf86_crtc->crtc_to_framebuffer);
		if (error) {
			ErrorF("SetPictureTransform failed for transformed scanout "
			       "update\n");
			goto free_dst;
		}

		if (xf86_crtc->filter)
			SetPicturePictFilter(src, xf86_crtc->filter, xf86_crtc->params,
					     xf86_crtc->nparams);

		pScreen->SourceValidate = amdgpuSourceValidate;
		CompositePicture(PictOpSrc,
				 src, NULL, dst,
				 extents.x1, extents.y1, 0, 0, extents.x1,
				 extents.y1, extents.x2 - extents.x1,
				 extents.y2 - extents.y1);
		pScreen->SourceValidate = SourceValidate;

 free_dst:
		FreePicture(dst, None);
 free_src:
		FreePicture(src, None);
	} else
 out:
	{
		GCPtr gc = GetScratchGC(pDraw->depth, pScreen);

		ValidateGC(pDraw, gc);
		(*gc->ops->CopyArea)(&src_pix->drawable, pDraw, gc,
				     xf86_crtc->x + extents.x1, xf86_crtc->y + extents.y1,
				     extents.x2 - extents.x1, extents.y2 - extents.y1,
				     extents.x1, extents.y1);
		FreeScratchGC(gc);
	}

	return TRUE;
}

static void
amdgpu_scanout_update_abort(xf86CrtcPtr crtc, void *event_data)
{
	drmmode_crtc_private_ptr drmmode_crtc = event_data;

	drmmode_crtc->scanout_update_pending = 0;
}

static void
amdgpu_scanout_update_handler(xf86CrtcPtr crtc, uint32_t frame, uint64_t usec,
							  void *event_data)
{
	drmmode_crtc_private_ptr drmmode_crtc = event_data;
	ScreenPtr screen = crtc->scrn->pScreen;
	RegionPtr region = DamageRegion(drmmode_crtc->scanout_damage);

	if (crtc->enabled &&
	    !drmmode_crtc->flip_pending &&
	    drmmode_crtc->dpms_mode == DPMSModeOn) {
		if (amdgpu_scanout_do_update(crtc, drmmode_crtc->scanout_id,
					     screen->GetWindowPixmap(screen->root),
					     region->extents)) {
			amdgpu_glamor_flush(crtc->scrn);
			RegionEmpty(region);
		}
	}

	amdgpu_scanout_update_abort(crtc, event_data);
}

static void
amdgpu_scanout_update(xf86CrtcPtr xf86_crtc)
{
	drmmode_crtc_private_ptr drmmode_crtc = xf86_crtc->driver_private;
	ScrnInfoPtr scrn = xf86_crtc->scrn;
	AMDGPUEntPtr pAMDGPUEnt = AMDGPUEntPriv(scrn);
	uintptr_t drm_queue_seq;
	DamagePtr pDamage;
	RegionPtr pRegion;
	BoxRec extents;

	if (!xf86_crtc->enabled ||
	    drmmode_crtc->scanout_update_pending ||
	    drmmode_crtc->flip_pending ||
	    drmmode_crtc->dpms_mode != DPMSModeOn)
		return;

	pDamage = drmmode_crtc->scanout_damage;
	if (!pDamage)
		return;

	pRegion = DamageRegion(pDamage);
	if (!RegionNotEmpty(pRegion))
		return;

	extents = *RegionExtents(pRegion);
	if (!amdgpu_scanout_extents_intersect(xf86_crtc, &extents)) {
		RegionEmpty(pRegion);
		return;
	}

	drm_queue_seq = amdgpu_drm_queue_alloc(xf86_crtc,
					       AMDGPU_DRM_QUEUE_CLIENT_DEFAULT,
					       AMDGPU_DRM_QUEUE_ID_DEFAULT,
					       drmmode_crtc,
					       amdgpu_scanout_update_handler,
					       amdgpu_scanout_update_abort,
					       FALSE);
	if (drm_queue_seq == AMDGPU_DRM_QUEUE_ERROR) {
		xf86DrvMsg(scrn->scrnIndex, X_WARNING,
			   "amdgpu_drm_queue_alloc failed for scanout update\n");
		amdgpu_scanout_update_handler(xf86_crtc, 0, 0, drmmode_crtc);
		return;
	}

	drmmode_crtc->scanout_update_pending = drm_queue_seq;

	if (!drmmode_wait_vblank(xf86_crtc, DRM_VBLANK_RELATIVE | DRM_VBLANK_EVENT,
				 1, drm_queue_seq, NULL, NULL)) {
		if (!(drmmode_crtc->scanout_status & DRMMODE_SCANOUT_VBLANK_FAILED)) {
			xf86DrvMsg(scrn->scrnIndex, X_WARNING,
				   "drmmode_wait_vblank failed for scanout update: %s\n",
				   strerror(errno));
			drmmode_crtc->scanout_status |= DRMMODE_SCANOUT_VBLANK_FAILED;
		}

		drmmode_crtc->drmmode->event_context.vblank_handler(pAMDGPUEnt->fd,
								    0, 0, 0,
								    (void*)drm_queue_seq);
		drmmode_crtc->wait_flip_nesting_level++;
		amdgpu_drm_queue_handle_deferred(xf86_crtc);
		return;
	}

	if (drmmode_crtc->scanout_status ==
	    (DRMMODE_SCANOUT_FLIP_FAILED | DRMMODE_SCANOUT_VBLANK_FAILED)) {
		/* The page flip and vblank ioctls failed before, but the vblank
		 * ioctl is working again, so we can try re-enabling TearFree
		 */
		xf86_crtc->funcs->set_mode_major(xf86_crtc, &xf86_crtc->mode,
						 xf86_crtc->rotation,
						 xf86_crtc->x, xf86_crtc->y);
	}

	drmmode_crtc->scanout_status &= ~DRMMODE_SCANOUT_VBLANK_FAILED;
}

static void
amdgpu_scanout_flip(ScreenPtr pScreen, AMDGPUInfoPtr info,
					xf86CrtcPtr xf86_crtc)
{
	drmmode_crtc_private_ptr drmmode_crtc = xf86_crtc->driver_private;
	RegionPtr region = DamageRegion(drmmode_crtc->scanout_damage);
	ScrnInfoPtr scrn = xf86_crtc->scrn;
	AMDGPUEntPtr pAMDGPUEnt = AMDGPUEntPriv(scrn);
	uintptr_t drm_queue_seq;
	unsigned scanout_id;
	struct drmmode_fb *fb;

	if (drmmode_crtc->scanout_update_pending ||
	    drmmode_crtc->flip_pending ||
	    drmmode_crtc->dpms_mode != DPMSModeOn)
		return;

	scanout_id = drmmode_crtc->scanout_id ^ 1;
	if (!amdgpu_scanout_do_update(xf86_crtc, scanout_id,
				      pScreen->GetWindowPixmap(pScreen->root),
				      region->extents))
		return;

	amdgpu_glamor_flush(scrn);
	RegionEmpty(region);

	fb = amdgpu_pixmap_get_fb(drmmode_crtc->scanout[scanout_id]);
	if (!fb) {
		xf86DrvMsg(scrn->scrnIndex, X_WARNING,
			   "Failed to get FB for scanout flip.\n");
		return;
	}

	drm_queue_seq = amdgpu_drm_queue_alloc(xf86_crtc,
					       AMDGPU_DRM_QUEUE_CLIENT_DEFAULT,
					       AMDGPU_DRM_QUEUE_ID_DEFAULT, fb,
					       amdgpu_scanout_flip_handler,
					       amdgpu_scanout_flip_abort, TRUE);
	if (drm_queue_seq == AMDGPU_DRM_QUEUE_ERROR) {
		xf86DrvMsg(scrn->scrnIndex, X_WARNING,
			   "Allocating DRM event queue entry failed.\n");
		return;
	}

	if (drmmode_page_flip_target_relative(pAMDGPUEnt, drmmode_crtc,
					      fb->handle, 0, drm_queue_seq, 1)
	    != 0) {
		if (!(drmmode_crtc->scanout_status & DRMMODE_SCANOUT_FLIP_FAILED)) {
			xf86DrvMsg(scrn->scrnIndex, X_WARNING,
				   "flip queue failed in %s: %s, TearFree inactive\n",
				   __func__, strerror(errno));
			drmmode_crtc->scanout_status |= DRMMODE_SCANOUT_FLIP_FAILED;
		}

		amdgpu_drm_abort_entry(drm_queue_seq);
		RegionCopy(DamageRegion(drmmode_crtc->scanout_damage),
			   &drmmode_crtc->scanout_last_region);
		RegionEmpty(&drmmode_crtc->scanout_last_region);
		amdgpu_scanout_update(xf86_crtc);
		drmmode_crtc_scanout_destroy(&drmmode_crtc->scanout[scanout_id]);
		drmmode_crtc->tear_free = FALSE;
		return;
	}

	if (drmmode_crtc->scanout_status & DRMMODE_SCANOUT_FLIP_FAILED) {
		xf86DrvMsg(scrn->scrnIndex, X_INFO, "TearFree active again\n");
		drmmode_crtc->scanout_status &= ~DRMMODE_SCANOUT_FLIP_FAILED;
	}

	drmmode_crtc->scanout_id = scanout_id;
	drmmode_crtc->scanout_update_pending = drm_queue_seq;
	drmmode_fb_reference(pAMDGPUEnt->fd, &drmmode_crtc->flip_pending, fb);
}

static void AMDGPUBlockHandler_KMS(BLOCKHANDLER_ARGS_DECL)
{
	ScrnInfoPtr pScrn = xf86ScreenToScrn(pScreen);
	AMDGPUInfoPtr info = AMDGPUPTR(pScrn);
	xf86CrtcConfigPtr xf86_config = XF86_CRTC_CONFIG_PTR(pScrn);
	int c;

	pScreen->BlockHandler = info->BlockHandler;
	(*pScreen->BlockHandler) (BLOCKHANDLER_ARGS);
	pScreen->BlockHandler = AMDGPUBlockHandler_KMS;

	if (!xf86ScreenToScrn(amdgpu_primary_screen(pScreen))->vtSema)
		return;

	if (!pScreen->isGPU)
	{
		for (c = 0; c < xf86_config->num_crtc; c++) {
			xf86CrtcPtr crtc = xf86_config->crtc[c];
			drmmode_crtc_private_ptr drmmode_crtc = crtc->driver_private;

			if (drmmode_crtc->rotate)
				continue;

			if (drmmode_crtc->tear_free)
				amdgpu_scanout_flip(pScreen, info, crtc);
			else if (drmmode_crtc->scanout[drmmode_crtc->scanout_id])
				amdgpu_scanout_update(crtc);
		}
	}

	amdgpu_dirty_update(pScrn);
}

/* This is called by AMDGPUPreInit to set up the default visual */
static Bool AMDGPUPreInitVisual(ScrnInfoPtr pScrn)
{
	AMDGPUInfoPtr info = AMDGPUPTR(pScrn);

	if (!xf86SetDepthBpp(pScrn, 0, 0, 0, Support32bppFb))
		return FALSE;

	switch (pScrn->depth) {
	case 8:
	case 15:
	case 16:
	case 24:
	case 30:
		break;

	default:
		xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
			   "Given depth (%d) is not supported by %s driver\n",
			   pScrn->depth, AMDGPU_DRIVER_NAME);
		return FALSE;
	}

	xf86PrintDepthBpp(pScrn);

	info->pix24bpp = xf86GetBppFromDepth(pScrn, pScrn->depth);
	info->pixel_bytes = pScrn->bitsPerPixel / 8;

	if (info->pix24bpp == 24) {
		xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
			   "Amdgpu does NOT support 24bpp\n");
		return FALSE;
	}

	xf86DrvMsg(pScrn->scrnIndex, X_INFO,
		   "Pixel depth = %d bits stored in %d byte%s (%d bpp pixmaps)\n",
		   pScrn->depth,
		   info->pixel_bytes,
		   info->pixel_bytes > 1 ? "s" : "", info->pix24bpp);

	if (!xf86SetDefaultVisual(pScrn, -1))
		return FALSE;

	if (pScrn->depth > 8 && pScrn->defaultVisual != TrueColor) {
		xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
			   "Default visual (%s) is not supported at depth %d\n",
			   xf86GetVisualName(pScrn->defaultVisual),
			   pScrn->depth);
		return FALSE;
	}
	return TRUE;
}

/* This is called by AMDGPUPreInit to handle all color weight issues */
static Bool AMDGPUPreInitWeight(ScrnInfoPtr pScrn)
{
	AMDGPUInfoPtr info = AMDGPUPTR(pScrn);

	/* Save flag for 6 bit DAC to use for
	   setting CRTC registers.  Otherwise use
	   an 8 bit DAC, even if xf86SetWeight sets
	   pScrn->rgbBits to some value other than
	   8. */
	info->dac6bits = FALSE;

	if (pScrn->depth > 8) {
		rgb defaultWeight = { 0, 0, 0 };

		if (!xf86SetWeight(pScrn, defaultWeight, defaultWeight))
			return FALSE;
	} else {
		pScrn->rgbBits = 8;
	}

	xf86DrvMsg(pScrn->scrnIndex, X_INFO,
		   "Using %d bits per RGB (%d bit DAC)\n",
		   pScrn->rgbBits, info->dac6bits ? 6 : 8);

	return TRUE;
}

static Bool AMDGPUPreInitAccel_KMS(ScrnInfoPtr pScrn)
{
	AMDGPUInfoPtr info = AMDGPUPTR(pScrn);

	if (xf86ReturnOptValBool(info->Options, OPTION_ACCEL, TRUE)) {
		AMDGPUEntPtr pAMDGPUEnt = AMDGPUEntPriv(pScrn);
		Bool use_glamor = TRUE;
		const char *accel_method;

		accel_method = xf86GetOptValString(info->Options, OPTION_ACCEL_METHOD);
		if ((accel_method && !strcmp(accel_method, "none")))
			use_glamor = FALSE;

#ifdef DRI2
		info->dri2.available = ! !xf86LoadSubModule(pScrn, "dri2");
#endif

		if (info->dri2.available)
			info->gbm = gbm_create_device(pAMDGPUEnt->fd);

		if (info->gbm) {
			if (use_glamor) {
				if (amdgpu_glamor_pre_init(pScrn))
					return TRUE;

				xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
					   "amdgpu_glamor_pre_init returned "
					   "FALSE, using ShadowFB\n");
			}
		} else {
			xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
				   "gbm_create_device returned NULL, using "
				   "ShadowFB\n");
		}
	} else {
		xf86DrvMsg(pScrn->scrnIndex, X_CONFIG,
			   "GPU acceleration disabled, using ShadowFB\n");
	}

	if (!xf86LoadSubModule(pScrn, "shadow"))
		return FALSE;

	info->dri2.available = FALSE;
	info->shadow_fb = TRUE;
	return TRUE;
}

static Bool AMDGPUPreInitChipType_KMS(ScrnInfoPtr pScrn,
				      struct amdgpu_gpu_info *gpu_info)
{
	AMDGPUInfoPtr info = AMDGPUPTR(pScrn);
	AMDGPUEntPtr pAMDGPUEnt = AMDGPUEntPriv(pScrn);

	pScrn->chipset = (char*)amdgpu_get_marketing_name(pAMDGPUEnt->pDev);
	if (!pScrn->chipset)
		pScrn->chipset = "Unknown AMD Radeon GPU";

	xf86DrvMsg(pScrn->scrnIndex, X_PROBED,
		   "Chipset: \"%s\" (ChipID = 0x%04x)\n",
		   pScrn->chipset, gpu_info->asic_id);

	info->family = gpu_info->family_id;

	return TRUE;
}

static Bool amdgpu_get_tile_config(AMDGPUInfoPtr info,
				   struct amdgpu_gpu_info *gpu_info)
{
	switch ((gpu_info->gb_addr_cfg & 0x70) >> 4) {
	case 0:
		info->group_bytes = 256;
		break;
	case 1:
		info->group_bytes = 512;
		break;
	default:
		return FALSE;
	}

	info->have_tiling_info = TRUE;
	return TRUE;
}

static void AMDGPUSetupCapabilities(ScrnInfoPtr pScrn)
{
	AMDGPUInfoPtr info = AMDGPUPTR(pScrn);
	AMDGPUEntPtr pAMDGPUEnt = AMDGPUEntPriv(pScrn);
	uint64_t value;
	int ret;

	pScrn->capabilities = 0;

	/* PRIME offloading requires acceleration */
	if (!info->use_glamor)
		return;

	ret = drmGetCap(pAMDGPUEnt->fd, DRM_CAP_PRIME, &value);
	if (ret == 0) {
		if (value & DRM_PRIME_CAP_EXPORT)
			pScrn->capabilities |= RR_Capability_SourceOutput | RR_Capability_SourceOffload;
		if (value & DRM_PRIME_CAP_IMPORT) {
			pScrn->capabilities |= RR_Capability_SinkOffload;
			if (info->drmmode.count_crtcs)
				pScrn->capabilities |= RR_Capability_SinkOutput;
		}
	}
}

/* When the root window is created, initialize the screen contents from
 * console if -background none was specified on the command line
 */
static Bool AMDGPUCreateWindow_oneshot(WindowPtr pWin)
{
	ScreenPtr pScreen = pWin->drawable.pScreen;
	ScrnInfoPtr pScrn;
	AMDGPUInfoPtr info;
	Bool ret;

	if (pWin != pScreen->root)
		ErrorF("%s called for non-root window %p\n", __func__, pWin);

	pScrn = xf86ScreenToScrn(pScreen);
	info = AMDGPUPTR(pScrn);
	pScreen->CreateWindow = info->CreateWindow;
	ret = pScreen->CreateWindow(pWin);

	if (ret)
		drmmode_copy_fb(pScrn, &info->drmmode);

	return ret;
}

static void amdgpu_determine_cursor_size(int fd, AMDGPUInfoPtr info)
{
	uint64_t value;

	if (drmGetCap(fd, DRM_CAP_CURSOR_WIDTH, &value) == 0)
		info->cursor_w = value;
	else if (info->family < AMDGPU_FAMILY_CI)
		info->cursor_w = CURSOR_WIDTH;
	else
		info->cursor_w = CURSOR_WIDTH_CIK;

	if (drmGetCap(fd, DRM_CAP_CURSOR_HEIGHT, &value) == 0)
		info->cursor_h = value;
	else if (info->family < AMDGPU_FAMILY_CI)
		info->cursor_h = CURSOR_HEIGHT;
	else
		info->cursor_h = CURSOR_HEIGHT_CIK;
}

/* When the root window is mapped, set the initial modes */
void AMDGPUWindowExposures_oneshot(WindowPtr pWin, RegionPtr pRegion)
{
	ScreenPtr pScreen = pWin->drawable.pScreen;
	ScrnInfoPtr pScrn = xf86ScreenToScrn(pScreen);
	AMDGPUInfoPtr info = AMDGPUPTR(pScrn);

	if (pWin != pScreen->root)
		ErrorF("%s called for non-root window %p\n", __func__, pWin);

	pScreen->WindowExposures = info->WindowExposures;
	pScreen->WindowExposures(pWin, pRegion);

	amdgpu_glamor_finish(pScrn);
	drmmode_set_desired_modes(pScrn, &info->drmmode, TRUE);
}

Bool AMDGPUPreInit_KMS(ScrnInfoPtr pScrn, int flags)
{
	AMDGPUInfoPtr info;
	AMDGPUEntPtr pAMDGPUEnt;
	struct amdgpu_gpu_info gpu_info;
	MessageType from;
	Gamma zeros = { 0.0, 0.0, 0.0 };
	int cpp;
	uint64_t heap_size = 0;
	uint64_t max_allocation = 0;

	if (flags & PROBE_DETECT)
		return TRUE;

	xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, AMDGPU_LOGLEVEL_DEBUG,
		       "AMDGPUPreInit_KMS\n");
	if (pScrn->numEntities != 1)
		return FALSE;

	pAMDGPUEnt = xf86GetEntityPrivate(pScrn->entityList[0],
					  getAMDGPUEntityIndex())->ptr;

	if (!AMDGPUGetRec(pScrn))
		return FALSE;

	info = AMDGPUPTR(pScrn);
	info->instance_id = pAMDGPUEnt->num_scrns++;
	pAMDGPUEnt->scrn[info->instance_id] = pScrn;

	info->pEnt =
	    xf86GetEntityInfo(pScrn->entityList[pScrn->numEntities - 1]);
	if (info->pEnt->location.type != BUS_PCI
#ifdef XSERVER_PLATFORM_BUS
	    && info->pEnt->location.type != BUS_PLATFORM
#endif
	    )
		return FALSE;

	if (xf86IsEntityShared(pScrn->entityList[0]) &&
	    info->instance_id == 0) {
		xf86SetPrimInitDone(pScrn->entityList[0]);
	}

	pScrn->monitor = pScrn->confScreen->monitor;

	if (!AMDGPUPreInitVisual(pScrn))
		return FALSE;

	xf86CollectOptions(pScrn, NULL);
	if (!(info->Options = malloc(sizeof(AMDGPUOptions_KMS))))
		return FALSE;

	memcpy(info->Options, AMDGPUOptions_KMS, sizeof(AMDGPUOptions_KMS));
	xf86ProcessOptions(pScrn->scrnIndex, pScrn->options, info->Options);

	if (!AMDGPUPreInitWeight(pScrn))
		return FALSE;

	memset(&gpu_info, 0, sizeof(gpu_info));
	amdgpu_query_gpu_info(pAMDGPUEnt->pDev, &gpu_info);

	if (!AMDGPUPreInitChipType_KMS(pScrn, &gpu_info))
		return FALSE;

	info->dri2.available = FALSE;
	info->dri2.enabled = FALSE;
	info->dri2.pKernelDRMVersion = drmGetVersion(pAMDGPUEnt->fd);
	if (info->dri2.pKernelDRMVersion == NULL) {
		xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
			   "AMDGPUDRIGetVersion failed to get the DRM version\n");
		return FALSE;
	}

	/* Get ScreenInit function */
	if (!xf86LoadSubModule(pScrn, "fb"))
		return FALSE;

	if (!AMDGPUPreInitAccel_KMS(pScrn))
		return FALSE;

	amdgpu_drm_queue_init(pScrn);

	/* don't enable tiling if accel is not enabled */
	if (info->use_glamor) {
		/* set default group bytes, overridden by kernel info below */
		info->group_bytes = 256;
		info->have_tiling_info = FALSE;
		amdgpu_get_tile_config(info, &gpu_info);
	}

	if (info->use_glamor) {
		from = X_DEFAULT;

		info->tear_free = 2;
		if (xf86GetOptValBool(info->Options, OPTION_TEAR_FREE,
				      &info->tear_free))
			from = X_CONFIG;
		xf86DrvMsg(pScrn->scrnIndex, from, "TearFree property default: %s\n",
			   info->tear_free == 2 ? "auto" : (info->tear_free ? "on" : "off"));

		info->shadow_primary =
			xf86ReturnOptValBool(info->Options, OPTION_SHADOW_PRIMARY, FALSE);

		if (info->shadow_primary)
			xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "ShadowPrimary enabled\n");

		if (!pScrn->is_gpu) {
			from = xf86GetOptValBool(info->Options, OPTION_VARIABLE_REFRESH,
						 &info->vrr_support) ? X_CONFIG : X_DEFAULT;

			if (info->vrr_support && !info->tear_free)
				xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
					   "Enabling VariableRefresh while TearFree is disabled can cause instability!\n");

			xf86DrvMsg(pScrn->scrnIndex, from, "VariableRefresh: %sabled\n",
				   info->vrr_support ? "en" : "dis");

			info->async_flip_secondaries = FALSE;
			from = xf86GetOptValBool(info->Options, OPTION_ASYNC_FLIP_SECONDARIES,
						 &info->async_flip_secondaries) ? X_CONFIG : X_DEFAULT;

			xf86DrvMsg(pScrn->scrnIndex, from, "AsyncFlipSecondaries: %sabled\n",
				   info->async_flip_secondaries ? "en" : "dis");
		}
	}

	if (!pScrn->is_gpu) {
		info->allowPageFlip = xf86ReturnOptValBool(info->Options,
							   OPTION_PAGE_FLIP,
							   TRUE);
		if (info->shadow_primary) {
			xf86DrvMsg(pScrn->scrnIndex,
				   info->allowPageFlip ? X_WARNING : X_DEFAULT,
				   "KMS Pageflipping: disabled%s\n",
				   info->allowPageFlip ?
				   " because of ShadowPrimary" : "");
			info->allowPageFlip = FALSE;
		} else {
			xf86DrvMsg(pScrn->scrnIndex, X_INFO,
				   "KMS Pageflipping: %sabled\n",
				   info->allowPageFlip ? "en" : "dis");
		}
	}

	if (xf86ReturnOptValBool(info->Options, OPTION_DELETE_DP12, FALSE)) {
		info->drmmode.delete_dp_12_displays = TRUE;
	}

	if (drmmode_pre_init(pScrn, &info->drmmode, pScrn->bitsPerPixel / 8) ==
	    FALSE) {
		xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
			   "Kernel modesetting setup failed\n");
		return FALSE;
	}

	AMDGPUSetupCapabilities(pScrn);

	if (info->drmmode.count_crtcs == 1)
		pAMDGPUEnt->HasCRTC2 = FALSE;
	else
		pAMDGPUEnt->HasCRTC2 = TRUE;

	amdgpu_determine_cursor_size(pAMDGPUEnt->fd, info);

	amdgpu_query_heap_size(pAMDGPUEnt->pDev, AMDGPU_GEM_DOMAIN_GTT,
				&heap_size, &max_allocation);
	info->gart_size = heap_size;
	amdgpu_query_heap_size(pAMDGPUEnt->pDev, AMDGPU_GEM_DOMAIN_VRAM,
				&heap_size, &max_allocation);
	info->vram_size = max_allocation;

	xf86DrvMsg(pScrn->scrnIndex, X_INFO,
		   "mem size init: gart size :%llx vram size: s:%llx visible:%llx\n",
		   (unsigned long long)info->gart_size,
		   (unsigned long long)heap_size,
		   (unsigned long long)max_allocation);

	cpp = pScrn->bitsPerPixel / 8;
	pScrn->displayWidth =
	    AMDGPU_ALIGN(pScrn->virtualX, drmmode_get_pitch_align(pScrn, cpp));

	/* Set display resolution */
	xf86SetDpi(pScrn, 0, 0);

	if (!xf86SetGamma(pScrn, zeros))
		return FALSE;

	if (!xf86ReturnOptValBool(info->Options, OPTION_SW_CURSOR, FALSE)) {
		if (!xf86LoadSubModule(pScrn, "ramdac"))
			return FALSE;
	}

	if (!pScrn->modes
#ifdef XSERVER_PLATFORM_BUS
	    && !pScrn->is_gpu
#endif
	    ) {
		xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "No modes.\n");
		return FALSE;
	}

	return TRUE;
}

static Bool AMDGPUCursorInit_KMS(ScreenPtr pScreen)
{
	ScrnInfoPtr pScrn = xf86ScreenToScrn(pScreen);
	AMDGPUInfoPtr info = AMDGPUPTR(pScrn);

	xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, AMDGPU_LOGLEVEL_DEBUG,
		       "Initializing Cursor\n");

	/* Set Silken Mouse */
	xf86SetSilkenMouse(pScreen);

	/* Cursor setup */
	miDCInitialize(pScreen, xf86GetPointerScreenFuncs());

	if (info->allowPageFlip) {
		miPointerScreenPtr PointPriv =
			dixLookupPrivate(&pScreen->devPrivates, miPointerScreenKey);

		if (!dixRegisterScreenPrivateKey(&amdgpu_device_private_key, pScreen,
						 PRIVATE_DEVICE,
						 sizeof(struct amdgpu_device_priv))) {
			xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "dixRegisterScreenPrivateKey failed\n");
			return FALSE;
		}

		info->SpriteFuncs = PointPriv->spriteFuncs;
		PointPriv->spriteFuncs = &drmmode_sprite_funcs;
	}

	if (xf86ReturnOptValBool(info->Options, OPTION_SW_CURSOR, FALSE))
		return TRUE;

	if (!xf86_cursors_init(pScreen, info->cursor_w, info->cursor_h,
			       HARDWARE_CURSOR_TRUECOLOR_AT_8BPP |
			       HARDWARE_CURSOR_AND_SOURCE_WITH_MASK |
			       HARDWARE_CURSOR_SOURCE_MASK_INTERLEAVE_1 |
			       HARDWARE_CURSOR_UPDATE_UNHIDDEN |
			       HARDWARE_CURSOR_ARGB)) {
		xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "xf86_cursors_init failed\n");
		return FALSE;
	}

	return TRUE;
}

void AMDGPUBlank(ScrnInfoPtr pScrn)
{
	xf86CrtcConfigPtr xf86_config = XF86_CRTC_CONFIG_PTR(pScrn);
	xf86OutputPtr output;
	xf86CrtcPtr crtc;
	int o, c;

	for (c = 0; c < xf86_config->num_crtc; c++) {
		crtc = xf86_config->crtc[c];
		for (o = 0; o < xf86_config->num_output; o++) {
			output = xf86_config->output[o];
			if (output->crtc != crtc)
				continue;

			output->funcs->dpms(output, DPMSModeOff);
		}
		crtc->funcs->dpms(crtc, DPMSModeOff);
	}
}

void AMDGPUUnblank(ScrnInfoPtr pScrn)
{
	xf86CrtcConfigPtr xf86_config = XF86_CRTC_CONFIG_PTR(pScrn);
	xf86OutputPtr output;
	xf86CrtcPtr crtc;
	int o, c;
	for (c = 0; c < xf86_config->num_crtc; c++) {
		crtc = xf86_config->crtc[c];
		if (!crtc->enabled)
			continue;
		crtc->funcs->dpms(crtc, DPMSModeOn);
		for (o = 0; o < xf86_config->num_output; o++) {
			output = xf86_config->output[o];
			if (output->crtc != crtc)
				continue;
			output->funcs->dpms(output, DPMSModeOn);
		}
	}
}

static Bool amdgpu_set_drm_master(ScrnInfoPtr pScrn)
{
	AMDGPUEntPtr pAMDGPUEnt = AMDGPUEntPriv(pScrn);
	int err;

	if (pAMDGPUEnt->platform_dev &&
	    (pAMDGPUEnt->platform_dev->flags & XF86_PDEV_SERVER_FD))
		return TRUE;

	err = drmSetMaster(pAMDGPUEnt->fd);
	if (err)
		ErrorF("Unable to retrieve master\n");

	return err == 0;
}

static void amdgpu_drop_drm_master(ScrnInfoPtr pScrn)
{
	AMDGPUEntPtr pAMDGPUEnt = AMDGPUEntPriv(pScrn);

	if (pAMDGPUEnt->platform_dev &&
	    (pAMDGPUEnt->platform_dev->flags & XF86_PDEV_SERVER_FD))
		return;

	drmDropMaster(pAMDGPUEnt->fd);
}


static
CARD32 cleanup_black_fb(OsTimerPtr timer, CARD32 now, pointer data)
{
	ScreenPtr screen = data;
	ScrnInfoPtr scrn = xf86ScreenToScrn(screen);
	AMDGPUEntPtr pAMDGPUEnt = AMDGPUEntPriv(scrn);
	xf86CrtcConfigPtr xf86_config = XF86_CRTC_CONFIG_PTR(scrn);
	int c;

	if (xf86ScreenToScrn(amdgpu_primary_screen(screen))->vtSema)
		return 0;

	/* Unreference the all-black FB created by AMDGPULeaveVT_KMS. After
	 * this, there should be no FB left created by this driver.
	 */
	for (c = 0; c < xf86_config->num_crtc; c++) {
		drmmode_crtc_private_ptr drmmode_crtc =
			xf86_config->crtc[c]->driver_private;

		drmmode_fb_reference(pAMDGPUEnt->fd, &drmmode_crtc->fb, NULL);
	}

	TimerFree(timer);
	return 0;
}

static Bool AMDGPUSaveScreen_KMS(ScreenPtr pScreen, int mode)
{
	ScrnInfoPtr pScrn = xf86ScreenToScrn(pScreen);
	Bool unblank;

	xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, AMDGPU_LOGLEVEL_DEBUG,
		       "AMDGPUSaveScreen(%d)\n", mode);

	unblank = xf86IsUnblank(mode);
	if (unblank)
		SetTimeSinceLastInputEvent();

	if ((pScrn != NULL) && pScrn->vtSema) {
		if (unblank)
			AMDGPUUnblank(pScrn);
		else
			AMDGPUBlank(pScrn);
	}
	return TRUE;
}

/* Called at the end of each server generation.  Restore the original
 * text mode, unmap video memory, and unwrap and call the saved
 * CloseScreen function.
 */
static Bool AMDGPUCloseScreen_KMS(ScreenPtr pScreen)
{
	ScrnInfoPtr pScrn = xf86ScreenToScrn(pScreen);
	AMDGPUInfoPtr info = AMDGPUPTR(pScrn);
	AMDGPUEntPtr pAMDGPUEnt = AMDGPUEntPriv(pScrn);

	xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, AMDGPU_LOGLEVEL_DEBUG,
		       "AMDGPUCloseScreen\n");

	/* Clear mask of assigned crtc's in this generation */
	pAMDGPUEnt->assigned_crtcs = 0;

	drmmode_uevent_fini(pScrn, &info->drmmode);
	amdgpu_drm_queue_close(pScrn);

	if (info->callback_event_type != -1) {
		DeleteCallback(&EventCallback, amdgpu_event_callback, pScrn);
		DeleteCallback(&FlushCallback, amdgpu_flush_callback, pScrn);
	}

	amdgpu_sync_close(pScreen);
	amdgpu_drop_drm_master(pScrn);

	drmmode_fini(pScrn, &info->drmmode);
	if (info->dri2.enabled) {
		amdgpu_dri2_close_screen(pScreen);
	}
	amdgpu_glamor_fini(pScreen);
	pScrn->vtSema = FALSE;
	xf86ClearPrimInitDone(info->pEnt->index);

	if (info->allowPageFlip) {
		miPointerScreenPtr PointPriv =
			dixLookupPrivate(&pScreen->devPrivates, miPointerScreenKey);

		if (PointPriv->spriteFuncs == &drmmode_sprite_funcs)
			PointPriv->spriteFuncs = info->SpriteFuncs;
	}

	pScreen->BlockHandler = info->BlockHandler;
	pScreen->CloseScreen = info->CloseScreen;
	return pScreen->CloseScreen(pScreen);
}

void AMDGPUFreeScreen_KMS(ScrnInfoPtr pScrn)
{
	xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, AMDGPU_LOGLEVEL_DEBUG,
		       "AMDGPUFreeScreen\n");

	AMDGPUFreeRec(pScrn);
}

Bool AMDGPUScreenInit_KMS(ScreenPtr pScreen, int argc, char **argv)
{
	ScrnInfoPtr pScrn = xf86ScreenToScrn(pScreen);
	AMDGPUInfoPtr info = AMDGPUPTR(pScrn);
	int subPixelOrder = SubPixelUnknown;
	MessageType from;
	Bool value;
	int driLevel;
	const char *s;
	void *front_ptr;

	pScrn->fbOffset = 0;

	miClearVisualTypes();
	if (!miSetVisualTypes(pScrn->depth,
			      miGetDefaultVisualMask(pScrn->depth),
			      pScrn->rgbBits, pScrn->defaultVisual))
		return FALSE;
	miSetPixmapDepths();

	if (!amdgpu_set_drm_master(pScrn))
		return FALSE;

	info->directRenderingEnabled = FALSE;
	if (info->shadow_fb == FALSE)
		info->directRenderingEnabled = amdgpu_dri2_screen_init(pScreen);

	if (!amdgpu_setup_kernel_mem(pScreen)) {
		xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
			   "amdgpu_setup_kernel_mem failed\n");
		return FALSE;
	}
	front_ptr = info->front_buffer->cpu_ptr;

	if (info->shadow_fb) {
		info->fb_shadow = calloc(1,
					 pScrn->displayWidth * pScrn->virtualY *
					 ((pScrn->bitsPerPixel + 7) >> 3));
		if (!info->fb_shadow) {
			xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
				   "Failed to allocate shadow framebuffer\n");
			return FALSE;
		} else {
			if (!fbScreenInit(pScreen, info->fb_shadow,
					  pScrn->virtualX, pScrn->virtualY,
					  pScrn->xDpi, pScrn->yDpi,
					  pScrn->displayWidth,
					  pScrn->bitsPerPixel))
				return FALSE;
		}
	}

	if (info->shadow_fb == FALSE) {
		/* Init fb layer */
		if (!fbScreenInit(pScreen, front_ptr,
				  pScrn->virtualX, pScrn->virtualY,
				  pScrn->xDpi, pScrn->yDpi, pScrn->displayWidth,
				  pScrn->bitsPerPixel))
			return FALSE;
	}

	xf86SetBlackWhitePixels(pScreen);

	if (pScrn->bitsPerPixel > 8) {
		VisualPtr visual;

		visual = pScreen->visuals + pScreen->numVisuals;
		while (--visual >= pScreen->visuals) {
			if ((visual->class | DynamicClass) == DirectColor) {
				visual->offsetRed = pScrn->offset.red;
				visual->offsetGreen = pScrn->offset.green;
				visual->offsetBlue = pScrn->offset.blue;
				visual->redMask = pScrn->mask.red;
				visual->greenMask = pScrn->mask.green;
				visual->blueMask = pScrn->mask.blue;
			}
		}
	}

	/* Must be after RGB order fixed */
	fbPictureInit(pScreen, 0, 0);

#ifdef RENDER
	if ((s = xf86GetOptValString(info->Options, OPTION_SUBPIXEL_ORDER))) {
		if (strcmp(s, "RGB") == 0)
			subPixelOrder = SubPixelHorizontalRGB;
		else if (strcmp(s, "BGR") == 0)
			subPixelOrder = SubPixelHorizontalBGR;
		else if (strcmp(s, "NONE") == 0)
			subPixelOrder = SubPixelNone;
		PictureSetSubpixelOrder(pScreen, subPixelOrder);
	}
#endif

	value = info->use_glamor;
	from = X_DEFAULT;

	if (info->use_glamor) {
		if (xf86GetOptValBool(info->Options, OPTION_DRI3, &value))
			from = X_CONFIG;

		if (xf86GetOptValInteger(info->Options, OPTION_DRI, &driLevel) &&
				(driLevel == 2 || driLevel == 3)) {
			from = X_CONFIG;
			value = driLevel == 3;
		}
	}

	if (value) {
		value = amdgpu_sync_init(pScreen) &&
			amdgpu_present_screen_init(pScreen) &&
			amdgpu_dri3_screen_init(pScreen);

		if (!value)
			from = X_WARNING;
	}

	xf86DrvMsg(pScrn->scrnIndex, from, "DRI3 %sabled\n", value ? "en" : "dis");

	pScrn->vtSema = TRUE;
	xf86SetBackingStore(pScreen);

	if (info->directRenderingEnabled) {
		xf86DrvMsg(pScrn->scrnIndex, X_INFO,
			   "Direct rendering enabled\n");
	} else {
		xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
			   "Direct rendering disabled\n");
	}

	if (info->use_glamor && info->directRenderingEnabled) {
		xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, AMDGPU_LOGLEVEL_DEBUG,
			       "Initializing Acceleration\n");
		if (amdgpu_glamor_init(pScreen)) {
			xf86DrvMsg(pScrn->scrnIndex, X_INFO,
				   "Acceleration enabled\n");
		} else {
			xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
				   "Acceleration initialization failed\n");
			xf86DrvMsg(pScrn->scrnIndex, X_INFO,
				   "2D and 3D acceleration disabled\n");
			info->use_glamor = FALSE;
		}
	} else if (info->directRenderingEnabled) {
		if (!amdgpu_pixmap_init(pScreen))
			xf86DrvMsg(pScrn->scrnIndex, X_INFO, "3D acceleration disabled\n");
		xf86DrvMsg(pScrn->scrnIndex, X_INFO, "2D acceleration disabled\n");
	} else {
		xf86DrvMsg(pScrn->scrnIndex, X_INFO, "2D and 3D acceleration disabled\n");
	}

	/* Init DPMS */
	xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, AMDGPU_LOGLEVEL_DEBUG,
		       "Initializing DPMS\n");
	xf86DPMSInit(pScreen, xf86DPMSSet, 0);

	if (!AMDGPUCursorInit_KMS(pScreen))
		return FALSE;

	/* DGA setup */
#ifdef XFreeXDGA
	/* DGA is dangerous on kms as the base and framebuffer location may change:
	 * http://lists.freedesktop.org/archives/xorg-devel/2009-September/002113.html
	 */
	/* xf86DiDGAInit(pScreen, info->LinearAddr + pScrn->fbOffset); */
#endif
	if (info->shadow_fb == FALSE && !pScreen->isGPU) {
		/* Init Xv */
		xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, AMDGPU_LOGLEVEL_DEBUG,
			       "Initializing Xv\n");
		AMDGPUInitVideo(pScreen);
	}

	if (info->shadow_fb == TRUE) {
		if (!shadowSetup(pScreen)) {
			xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
				   "Shadowfb initialization failed\n");
			return FALSE;
		}
	}
	pScrn->pScreen = pScreen;

	if (!pScreen->isGPU) {
		if (serverGeneration == 1 && bgNoneRoot && info->use_glamor) {
			info->CreateWindow = pScreen->CreateWindow;
			pScreen->CreateWindow = AMDGPUCreateWindow_oneshot;
		}
		info->WindowExposures = pScreen->WindowExposures;
		pScreen->WindowExposures = AMDGPUWindowExposures_oneshot;
	}

	/* Provide SaveScreen & wrap BlockHandler and CloseScreen */
	/* Wrap CloseScreen */
	info->CloseScreen = pScreen->CloseScreen;
	pScreen->CloseScreen = AMDGPUCloseScreen_KMS;
	pScreen->SaveScreen = AMDGPUSaveScreen_KMS;
	info->BlockHandler = pScreen->BlockHandler;
	pScreen->BlockHandler = AMDGPUBlockHandler_KMS;

	info->CreateScreenResources = pScreen->CreateScreenResources;
	pScreen->CreateScreenResources = AMDGPUCreateScreenResources_KMS;

	pScreen->StartPixmapTracking = PixmapStartDirtyTracking;
	pScreen->StopPixmapTracking = PixmapStopDirtyTracking;
	pScreen->SyncSharedPixmap = amdgpu_sync_shared_pixmap;

	if (!xf86CrtcScreenInit(pScreen))
		return FALSE;

	/* Wrap pointer motion to flip touch screen around */
//    info->PointerMoved = pScrn->PointerMoved;
//    pScrn->PointerMoved = AMDGPUPointerMoved;

	if (!drmmode_setup_colormap(pScreen, pScrn))
		return FALSE;

	/* Note unused options */
	if (serverGeneration == 1)
		xf86ShowUnusedOptions(pScrn->scrnIndex, pScrn->options);

	if (info->vrr_support) {
		if (!amdgpu_property_vectors_wrapped) {
			saved_change_property = ProcVector[X_ChangeProperty];
			ProcVector[X_ChangeProperty] = amdgpu_change_property;
			saved_delete_property = ProcVector[X_DeleteProperty];
			ProcVector[X_DeleteProperty] = amdgpu_delete_property;
			amdgpu_property_vectors_wrapped = TRUE;
		}

		amdgpu_vrr_atom = MakeAtom("_VARIABLE_REFRESH",
					   strlen("_VARIABLE_REFRESH"), TRUE);
	}

	drmmode_init(pScrn, &info->drmmode);

	xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, AMDGPU_LOGLEVEL_DEBUG,
		       "AMDGPUScreenInit finished\n");

	return TRUE;
}

Bool AMDGPUEnterVT_KMS(ScrnInfoPtr pScrn)
{
	AMDGPUInfoPtr info = AMDGPUPTR(pScrn);

	xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, AMDGPU_LOGLEVEL_DEBUG,
		       "AMDGPUEnterVT_KMS\n");

	amdgpu_set_drm_master(pScrn);

	if (info->shadow_fb) {
		int pitch;
		struct amdgpu_buffer *front_buffer =
			amdgpu_alloc_pixmap_bo(pScrn, pScrn->virtualX,
					       pScrn->virtualY, pScrn->depth,
					       AMDGPU_CREATE_PIXMAP_SCANOUT |
					       AMDGPU_CREATE_PIXMAP_LINEAR,
					       pScrn->bitsPerPixel,
					       &pitch);

		if (front_buffer) {
			if (amdgpu_bo_map(pScrn, front_buffer) == 0) {
				memset(front_buffer->cpu_ptr, 0, pitch * pScrn->virtualY);
				amdgpu_bo_unref(&info->front_buffer);
				info->front_buffer = front_buffer;
			} else {
				amdgpu_bo_unref(&front_buffer);
				front_buffer = NULL;
			}
		}

		if (!front_buffer) {
			xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
				   "Failed to allocate new scanout BO after VT switch, "
				   "other DRM masters may see screen contents\n");
		}
	}

	pScrn->vtSema = TRUE;

	if (!drmmode_set_desired_modes(pScrn, &info->drmmode, TRUE))
		return FALSE;

	return TRUE;
}

static void
pixmap_unref_fb(PixmapPtr pixmap)
{
	ScrnInfoPtr scrn = xf86ScreenToScrn(pixmap->drawable.pScreen);
	struct drmmode_fb **fb_ptr = amdgpu_pixmap_get_fb_ptr(pixmap);
	AMDGPUEntPtr pAMDGPUEnt = AMDGPUEntPriv(scrn);

	if (fb_ptr)
		drmmode_fb_reference(pAMDGPUEnt->fd, fb_ptr, NULL);
}

static void
client_pixmap_unref_fb(void *value, XID id, void *pScreen)
{
	PixmapPtr pixmap = value;

	if (pixmap->drawable.pScreen == pScreen)
		pixmap_unref_fb(pixmap);
}

void AMDGPULeaveVT_KMS(ScrnInfoPtr pScrn)
{
	AMDGPUInfoPtr info = AMDGPUPTR(pScrn);
	ScreenPtr pScreen = pScrn->pScreen;

	xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, AMDGPU_LOGLEVEL_DEBUG,
		       "AMDGPULeaveVT_KMS\n");

	if (!info->shadow_fb) {
		AMDGPUEntPtr pAMDGPUEnt = AMDGPUEntPriv(pScrn);
		xf86CrtcConfigPtr xf86_config = XF86_CRTC_CONFIG_PTR(pScrn);
		xf86CrtcPtr crtc;
		drmmode_crtc_private_ptr drmmode_crtc;
		unsigned w = 0, h = 0;
		int i;

		/* If we're called from CloseScreen, trying to clear the black
		 * scanout BO will likely crash and burn
		 */
		if (!pScreen->GCperDepth[0])
			goto hide_cursors;

		/* Compute maximum scanout dimensions of active CRTCs */
		for (i = 0; i < xf86_config->num_crtc; i++) {
			crtc = xf86_config->crtc[i];
			drmmode_crtc = crtc->driver_private;

			if (!drmmode_crtc->fb)
				continue;

			w = max(w, crtc->mode.HDisplay);
			h = max(h, crtc->mode.VDisplay);
		}

		/* Make all active CRTCs scan out from an all-black framebuffer */
		if (w > 0 && h > 0) {
			PixmapPtr black_scanout =
				pScreen->CreatePixmap(pScreen, w, h, pScrn->depth,
						      AMDGPU_CREATE_PIXMAP_SCANOUT);

			if (black_scanout) {
				struct drmmode_fb *black_fb =
					amdgpu_pixmap_get_fb(black_scanout);

				amdgpu_pixmap_clear(black_scanout);
				amdgpu_glamor_finish(pScrn);

				for (i = 0; i < xf86_config->num_crtc; i++) {
					crtc = xf86_config->crtc[i];
					drmmode_crtc = crtc->driver_private;

					if (drmmode_crtc->fb) {
						if (black_fb) {
							drmmode_set_mode(crtc, black_fb, &crtc->mode, 0, 0);
						} else {
							drmModeSetCrtc(pAMDGPUEnt->fd,
								       drmmode_crtc->mode_crtc->crtc_id, 0,
								       0, 0, NULL, 0, NULL);
							drmmode_fb_reference(pAMDGPUEnt->fd,
									     &drmmode_crtc->fb, NULL);
						}

						if (pScrn->is_gpu) {
							if (drmmode_crtc->scanout[0])
								pixmap_unref_fb(drmmode_crtc->scanout[0]);
							if (drmmode_crtc->scanout[1])
								pixmap_unref_fb(drmmode_crtc->scanout[1]);
						} else {
							drmmode_crtc_scanout_free(crtc);
						}
					}
				}
				dixDestroyPixmap(black_scanout, 0);
			}
		}

		xf86RotateFreeShadow(pScrn);

		/* Unreference FBs of all pixmaps. After this, the only FB remaining
		 * should be the all-black one being scanned out by active CRTCs
		 */
		for (i = 0; i < currentMaxClients; i++) {
			if (i > 0 &&
			    (!clients[i] || clients[i]->clientState != ClientStateRunning))
				continue;

			FindClientResourcesByType(clients[i], RT_PIXMAP,
						  client_pixmap_unref_fb, pScreen);
		}

		pixmap_unref_fb(pScreen->GetScreenPixmap(pScreen));
	} else {
		memset(info->front_buffer->cpu_ptr, 0, pScrn->virtualX *
		       info->pixel_bytes * pScrn->virtualY);
	}

	if (pScreen->GCperDepth[0])
		TimerSet(NULL, 0, 1000, cleanup_black_fb, pScreen);

 hide_cursors:
	xf86_hide_cursors(pScrn);

	amdgpu_drop_drm_master(pScrn);

	xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, AMDGPU_LOGLEVEL_DEBUG,
		       "Ok, leaving now...\n");
}

Bool AMDGPUSwitchMode_KMS(ScrnInfoPtr pScrn, DisplayModePtr mode)
{
	Bool ret;
	ret = xf86SetSingleMode(pScrn, mode, RR_Rotate_0);
	return ret;

}

void AMDGPUAdjustFrame_KMS(ScrnInfoPtr pScrn, int x, int y)
{
	AMDGPUInfoPtr info = AMDGPUPTR(pScrn);
	drmmode_adjust_frame(pScrn, &info->drmmode, x, y);
	return;
}

static Bool amdgpu_setup_kernel_mem(ScreenPtr pScreen)
{
	ScrnInfoPtr pScrn = xf86ScreenToScrn(pScreen);
	AMDGPUEntPtr pAMDGPUEnt = AMDGPUEntPriv(pScrn);
	AMDGPUInfoPtr info = AMDGPUPTR(pScrn);
	xf86CrtcConfigPtr xf86_config = XF86_CRTC_CONFIG_PTR(pScrn);
	int cpp = info->pixel_bytes;
	int cursor_size;
	int c, i;

	cursor_size = info->cursor_w * info->cursor_h * 4;
	cursor_size = AMDGPU_ALIGN(cursor_size, AMDGPU_GPU_PAGE_SIZE);
	for (c = 0; c < xf86_config->num_crtc; c++) {
		drmmode_crtc_private_ptr drmmode_crtc = xf86_config->crtc[c]->driver_private;

		for (i = 0; i < 2; i++) {
			if (!drmmode_crtc->cursor_buffer[i]) {
				drmmode_crtc->cursor_buffer[i] =
					amdgpu_bo_open(pAMDGPUEnt->pDev,
						       cursor_size, 0,
						       AMDGPU_GEM_DOMAIN_VRAM);

				if (!(drmmode_crtc->cursor_buffer[i])) {
					ErrorF("Failed to allocate cursor buffer memory\n");
					return FALSE;
				}

				if (amdgpu_bo_cpu_map(drmmode_crtc->cursor_buffer[i]->bo.amdgpu,
						      &drmmode_crtc->cursor_buffer[i]->cpu_ptr))
					ErrorF("Failed to map cursor buffer memory\n");
			}
		}
	}

	if (!info->front_buffer) {
		int pitch;
		int hint = AMDGPU_CREATE_PIXMAP_SCANOUT | AMDGPU_CREATE_PIXMAP_FRONT;

		if (info->shadow_primary)
			hint |= AMDGPU_CREATE_PIXMAP_LINEAR | AMDGPU_CREATE_PIXMAP_GTT;
		else if (!info->use_glamor)
			hint |= AMDGPU_CREATE_PIXMAP_LINEAR;

		info->front_buffer =
			amdgpu_alloc_pixmap_bo(pScrn, pScrn->virtualX,
					       pScrn->virtualY, pScrn->depth,
					       hint, pScrn->bitsPerPixel,
					       &pitch);
		if (!(info->front_buffer)) {
			ErrorF("Failed to allocate front buffer memory\n");
			return FALSE;
		}

		if (!info->use_glamor &&
		    amdgpu_bo_map(pScrn, info->front_buffer) != 0) {
			ErrorF("Failed to map front buffer memory\n");
			return FALSE;
		}

		pScrn->displayWidth = pitch / cpp;
	}

	xf86DrvMsg(pScrn->scrnIndex, X_INFO, "Front buffer pitch: %d bytes\n",
		   pScrn->displayWidth * cpp);
	return TRUE;
}

/* Used to disallow modes that are not supported by the hardware */
ModeStatus AMDGPUValidMode(ScrnInfoPtr pScrn, DisplayModePtr mode,
			   Bool verbose, int flag)
{
	/* There are problems with double scan mode at high clocks
	 * They're likely related PLL and display buffer settings.
	 * Disable these modes for now.
	 */
	if (mode->Flags & V_DBLSCAN) {
		if ((mode->CrtcHDisplay >= 1024) || (mode->CrtcVDisplay >= 768))
			return MODE_CLOCK_RANGE;
	}
	return MODE_OK;
}

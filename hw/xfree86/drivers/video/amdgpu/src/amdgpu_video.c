#include "config.h"
#include <xorg-server.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

#include "amdgpu_drv.h"
#include "amdgpu_glamor.h"
#include "amdgpu_probe.h"
#include "amdgpu_video.h"
#include "amdgpu_pixmap.h"

#include "xf86.h"
#include "dixstruct.h"

#include <X11/extensions/dpmsconst.h>
#include <X11/extensions/Xv.h>
#include "fourcc.h"

#define OFF_DELAY       250	/* milliseconds */
#define FREE_DELAY      15000

#define OFF_TIMER       0x01
#define FREE_TIMER      0x02
#define CLIENT_VIDEO_ON 0x04

static void amdgpu_box_intersect(BoxPtr dest, BoxPtr a, BoxPtr b)
{
	dest->x1 = a->x1 > b->x1 ? a->x1 : b->x1;
	dest->x2 = a->x2 < b->x2 ? a->x2 : b->x2;
	if (dest->x1 >= dest->x2) {
		dest->x1 = dest->x2 = dest->y1 = dest->y2 = 0;
		return;
	}

	dest->y1 = a->y1 > b->y1 ? a->y1 : b->y1;
	dest->y2 = a->y2 < b->y2 ? a->y2 : b->y2;
	if (dest->y1 >= dest->y2)
		dest->x1 = dest->x2 = dest->y1 = dest->y2 = 0;
}

static int amdgpu_box_area(BoxPtr box)
{
	return (int)(box->x2 - box->x1) * (int)(box->y2 - box->y1);
}

Bool
amdgpu_crtc_is_enabled(xf86CrtcPtr crtc)
{
	drmmode_crtc_private_ptr drmmode_crtc = crtc->driver_private;

	return crtc->enabled && drmmode_crtc->dpms_mode == DPMSModeOn;
}

static void amdgpu_crtc_box(RRCrtcPtr crtc, BoxPtr crtc_box)
{
	if (crtc->mode) {
		crtc_box->x1 = crtc->x;
		crtc_box->y1 = crtc->y;
		switch (crtc->rotation) {
		case RR_Rotate_0:
		case RR_Rotate_180:
		default:
			crtc_box->x2 = crtc->x + crtc->mode->mode.width;
			crtc_box->y2 = crtc->y + crtc->mode->mode.height;
			break;
		case RR_Rotate_90:
		case RR_Rotate_270:
			crtc_box->x2 = crtc->x + crtc->mode->mode.height;
			crtc_box->y2 = crtc->y + crtc->mode->mode.width;
			break;
		}
	} else
		crtc_box->x1 = crtc_box->x2 = crtc_box->y1 = crtc_box->y2 = 0;
}

static Bool amdgpu_crtc_on(RRCrtcPtr crtc, Bool crtc_is_xf86_hint)
{
	if (!crtc) {
		return FALSE;
	}
	if (crtc_is_xf86_hint && crtc->devPrivate) {
		return amdgpu_crtc_is_enabled(crtc->devPrivate);
	} else {
		return !!crtc->mode;
	}
}

/*
 * Return the crtc covering 'box'. If two crtcs cover a portion of
 * 'box', then prefer the crtc with greater coverage.
 */
static RRCrtcPtr
amdgpu_crtc_covering_box(ScreenPtr pScreen, BoxPtr box, Bool screen_is_xf86_hint)
{
	rrScrPrivPtr pScrPriv;
	RRCrtcPtr crtc, best_crtc, primary_crtc;
	int coverage, best_coverage;
	int c;
	BoxRec crtc_box, cover_box;
	RROutputPtr primary_output;

	best_crtc = NULL;
	best_coverage = 0;
	primary_crtc = NULL;
	primary_output = NULL;

	if (!dixPrivateKeyRegistered(rrPrivKey))
		return NULL;

	pScrPriv = rrGetScrPriv(pScreen);

	if (!pScrPriv)
		return NULL;

	primary_output = RRFirstOutput(pScreen);
	if (primary_output && primary_output->crtc)
		primary_crtc = primary_output->crtc;

	for (c = 0; c < pScrPriv->numCrtcs; c++) {
		crtc = pScrPriv->crtcs[c];

		/* If the CRTC is off, treat it as not covering */
		if (!amdgpu_crtc_on(crtc, screen_is_xf86_hint))
			continue;

		amdgpu_crtc_box(crtc, &crtc_box);
		amdgpu_box_intersect(&cover_box, &crtc_box, box);
		coverage = amdgpu_box_area(&cover_box);
		if (coverage > best_coverage ||
		   (crtc == primary_crtc && coverage == best_coverage)) {
			best_crtc = crtc;
			best_coverage = coverage;
		}
	}

	return best_crtc;
}

static RRCrtcPtr
amdgpu_crtc_covering_box_on_secondary(ScreenPtr pScreen, BoxPtr box)
{
	if (!pScreen->isGPU) {
		ScreenPtr secondary;
		RRCrtcPtr crtc = NULL;

		xorg_list_for_each_entry(secondary, &pScreen->secondary_list, secondary_head) {
			if (!secondary->is_output_secondary)
				continue;

			crtc = amdgpu_crtc_covering_box(secondary, box, FALSE);
			if (crtc)
				return crtc;
		}
	}

	return NULL;
}

RRCrtcPtr
amdgpu_randr_crtc_covering_drawable(DrawablePtr pDraw)
{
	ScreenPtr pScreen = pDraw->pScreen;
	RRCrtcPtr crtc = NULL;
	BoxRec box;

	box.x1 = pDraw->x;
	box.y1 = pDraw->y;
	box.x2 = box.x1 + pDraw->width;
	box.y2 = box.y1 + pDraw->height;

	crtc = amdgpu_crtc_covering_box(pScreen, &box, TRUE);
	if (!crtc) {
		crtc = amdgpu_crtc_covering_box_on_secondary(pScreen, &box);
	}
	return crtc;
}

xf86CrtcPtr
amdgpu_pick_best_crtc(ScreenPtr pScreen,
		      int x1, int x2, int y1, int y2)
{
	ScrnInfoPtr pScrn = xf86ScreenToScrn(pScreen);

	if (!pScrn->vtSema)
		return NULL;

	RRCrtcPtr crtc = NULL;
	BoxRec box;

	box.x1 = x1;
	box.x2 = x2;
	box.y1 = y1;
	box.y2 = y2;

	crtc = amdgpu_crtc_covering_box(pScreen, &box, TRUE);
	if (crtc) {
		return crtc->devPrivate;
	}
	return NULL;
}

void AMDGPUInitVideo(ScreenPtr pScreen)
{
	ScrnInfoPtr pScrn = xf86ScreenToScrn(pScreen);
	AMDGPUInfoPtr info = AMDGPUPTR(pScrn);
	XF86VideoAdaptorPtr *adaptors, *newAdaptors = NULL;
	XF86VideoAdaptorPtr texturedAdaptor = NULL;
	int num_adaptors;

	num_adaptors = xf86XVListGenericAdaptors(pScrn, &adaptors);
	newAdaptors =
	    malloc((num_adaptors + 2) * sizeof(*newAdaptors));
	if (!newAdaptors)
		return;

	memcpy(newAdaptors, adaptors,
	       num_adaptors * sizeof(XF86VideoAdaptorPtr));
	adaptors = newAdaptors;

	if (info->use_glamor) {
		texturedAdaptor = amdgpu_glamor_xv_init(pScreen, 16);
		if (texturedAdaptor != NULL) {
			adaptors[num_adaptors++] = texturedAdaptor;
			xf86DrvMsg(pScrn->scrnIndex, X_INFO,
				   "Set up textured video (glamor)\n");
		} else
			xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
				   "Failed to set up textured video (glamor)\n");
	}

	if (num_adaptors)
		xf86XVScreenInit(pScreen, adaptors, num_adaptors);

	if (newAdaptors)
		free(newAdaptors);

}

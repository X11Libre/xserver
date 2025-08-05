/*
 * Copyright © 2007 Red Hat, Inc.
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
 *     Dave Airlie <airlied@redhat.com>
 *
 */
#ifndef DRMMODE_DISPLAY_H
#define DRMMODE_DISPLAY_H

#include "xf86drmMode.h"
#ifdef CONFIG_UDEV
#include "libudev.h"
#endif

#include "amdgpu_drm_queue.h"
#include "amdgpu_probe.h"
#include "amdgpu.h"

/*
 * Enum of non-legacy color management properties, according to DRM. Note that
 * the values should be incremental (with the exception of the INVALID member),
 * as defined by C99. The ordering also matters. Some logics (such as iterators
 * and bitmasks) depend on these facts.
 */
enum drmmode_cm_prop {
	CM_DEGAMMA_LUT,
	CM_CTM,
	CM_GAMMA_LUT,
	CM_DEGAMMA_LUT_SIZE,
	CM_GAMMA_LUT_SIZE,
	CM_NUM_PROPS,
	CM_INVALID_PROP = -1,
};

typedef struct {
	ScrnInfoPtr scrn;
#ifdef CONFIG_UDEV
	struct udev_monitor *uevent_monitor;
	InputHandlerProc uevent_handler;
#endif
	drmEventContext event_context;
	int count_crtcs;

	Bool delete_dp_12_displays;

	Bool dri2_flipping;
	Bool present_flipping;
	uint32_t vrr_prop_id;

	/* Cache for DRM property type IDs for CRTC color management */
	uint32_t cm_prop_ids[CM_NUM_PROPS];
	/* Lookup table sizes */
	uint32_t degamma_lut_size;
	uint32_t gamma_lut_size;
} drmmode_rec, *drmmode_ptr;

typedef struct {
	void *event_data;
	int flip_count;
	unsigned int fe_frame;
	uint64_t fe_usec;
	xf86CrtcPtr fe_crtc;
	amdgpu_drm_handler_proc handler;
	amdgpu_drm_abort_proc abort;
	struct drmmode_fb *fb[0];
} drmmode_flipdata_rec, *drmmode_flipdata_ptr;

struct drmmode_fb {
	int refcnt;
	uint32_t handle;
};

enum drmmode_scanout_status {
	DRMMODE_SCANOUT_OK,
	DRMMODE_SCANOUT_FLIP_FAILED = 1u << 0,
	DRMMODE_SCANOUT_VBLANK_FAILED = 1u << 1,
};

typedef struct {
	drmmode_ptr drmmode;
	drmModeCrtcPtr mode_crtc;
	int hw_id;

	CursorPtr cursor;
	int cursor_x;
	int cursor_y;
	int cursor_xhot;
	int cursor_yhot;
	unsigned cursor_id;
	struct amdgpu_buffer *cursor_buffer[2];

	PixmapPtr rotate;
	PixmapPtr scanout[2];
	DamagePtr scanout_damage;
	Bool ignore_damage;
	RegionRec scanout_last_region;
	unsigned scanout_id;
	uintptr_t scanout_update_pending;
	Bool tear_free;
	enum drmmode_scanout_status scanout_status;
	Bool vrr_enabled;

	PixmapPtr prime_scanout_pixmap;

	int dpms_mode;
	CARD64 dpms_last_ust;
	uint32_t dpms_last_seq;
	int dpms_last_fps;
	uint32_t interpolated_vblanks;

	/* Modeset needed for DPMS on */
	Bool need_modeset;
	/* For keeping track of nested calls to drm_wait_pending_flip /
	 * drm_queue_handle_deferred
	 */
	int wait_flip_nesting_level;
	/* A flip to this FB is pending for this CRTC */
	struct drmmode_fb *flip_pending;
	/* The FB currently being scanned out by this CRTC, if any */
	struct drmmode_fb *fb;

	struct drm_color_lut *degamma_lut;
	struct drm_color_ctm *ctm;
	struct drm_color_lut *gamma_lut;
} drmmode_crtc_private_rec, *drmmode_crtc_private_ptr;

typedef struct {
	drmModePropertyPtr mode_prop;
	uint64_t value;
	int num_atoms;		/* if range prop, num_atoms == 1; if enum prop, num_atoms == num_enums + 1 */
	Atom *atoms;
} drmmode_prop_rec, *drmmode_prop_ptr;

typedef struct {
	drmmode_ptr drmmode;
	int output_id;
	drmModeConnectorPtr mode_output;
	drmModeEncoderPtr *mode_encoders;
	drmModePropertyBlobPtr edid_blob;
	drmModePropertyBlobPtr tile_blob;
	int dpms_enum_id;
	int num_props;
	drmmode_prop_ptr props;
	int enc_mask;
	int enc_clone_mask;
	int tear_free;
} drmmode_output_private_rec, *drmmode_output_private_ptr;

typedef struct {
	uint32_t lessee_id;
} drmmode_lease_private_rec, *drmmode_lease_private_ptr;


enum drmmode_flip_sync {
    FLIP_VSYNC,
    FLIP_ASYNC,
};


/**
 * Return TRUE if kernel supports non-legacy color management.
 */
static inline Bool
drmmode_cm_prop_supported(drmmode_ptr drmmode, enum drmmode_cm_prop cm_prop_index)
{
	if (drmmode->cm_prop_ids[cm_prop_index] == 0)
		return FALSE;
	return TRUE;
}

/* Can the page flip ioctl be used for this CRTC? */
static inline Bool
drmmode_crtc_can_flip(xf86CrtcPtr crtc)
{
	drmmode_crtc_private_ptr drmmode_crtc = crtc->driver_private;

	return crtc->enabled &&
		drmmode_crtc->dpms_mode == DPMSModeOn &&
		!drmmode_crtc->rotate &&
		(drmmode_crtc->tear_free ||
		 !drmmode_crtc->scanout[drmmode_crtc->scanout_id]);
}


static inline void
drmmode_fb_reference_loc(int drm_fd, struct drmmode_fb **old, struct drmmode_fb *new,
			 const char *caller, unsigned line)
{
	if (new) {
		if (new->refcnt <= 0) {
			FatalError("New FB's refcnt was %d at %s:%u",
				   new->refcnt, caller, line);
		}

		new->refcnt++;
	}

	if (*old) {
		if ((*old)->refcnt <= 0) {
			FatalError("Old FB's refcnt was %d at %s:%u",
				   (*old)->refcnt, caller, line);
		}

		if (--(*old)->refcnt == 0) {
			drmModeRmFB(drm_fd, (*old)->handle);
			free(*old);
		}
	}

	*old = new;
}

#define drmmode_fb_reference(fd, old, new) \
	drmmode_fb_reference_loc(fd, old, new, __func__, __LINE__)


static inline void
drmmode_crtc_scanout_destroy(PixmapPtr *scanout)
{
	if (!*scanout)
		return;

	dixDestroyPixmap(*scanout, 0);
	(*scanout) = NULL;
}


extern int drmmode_page_flip_target_absolute(AMDGPUEntPtr pAMDGPUEnt,
					     drmmode_crtc_private_ptr drmmode_crtc,
					     int fb_id, uint32_t flags,
					     uintptr_t drm_queue_seq,
					     uint32_t target_msc);
extern int drmmode_page_flip_target_relative(AMDGPUEntPtr pAMDGPUEnt,
					     drmmode_crtc_private_ptr drmmode_crtc,
					     int fb_id, uint32_t flags,
					     uintptr_t drm_queue_seq,
					     uint32_t target_msc);
extern Bool drmmode_pre_init(ScrnInfoPtr pScrn, drmmode_ptr drmmode, int cpp);
extern void drmmode_init(ScrnInfoPtr pScrn, drmmode_ptr drmmode);
extern void drmmode_fini(ScrnInfoPtr pScrn, drmmode_ptr drmmode);
void drmmode_adjust_frame(ScrnInfoPtr pScrn, drmmode_ptr drmmode, int x, int y);
extern Bool drmmode_set_desired_modes(ScrnInfoPtr pScrn, drmmode_ptr drmmode,
				      Bool set_hw);
extern void drmmode_copy_fb(ScrnInfoPtr pScrn, drmmode_ptr drmmode);
extern Bool drmmode_setup_colormap(ScreenPtr pScreen, ScrnInfoPtr pScrn);

void drmmode_crtc_scanout_free(xf86CrtcPtr crtc);

extern void drmmode_uevent_init(ScrnInfoPtr scrn, drmmode_ptr drmmode);
extern void drmmode_uevent_fini(ScrnInfoPtr scrn, drmmode_ptr drmmode);

Bool drmmode_set_mode(xf86CrtcPtr crtc, struct drmmode_fb *fb,
		      DisplayModePtr mode, int x, int y);

extern int drmmode_get_crtc_id(xf86CrtcPtr crtc);
extern int drmmode_get_pitch_align(ScrnInfoPtr scrn, int bpe);
Bool amdgpu_do_pageflip(ScrnInfoPtr scrn, ClientPtr client,
			PixmapPtr new_front, uint64_t id, void *data,
			xf86CrtcPtr ref_crtc, amdgpu_drm_handler_proc handler,
			amdgpu_drm_abort_proc abort,
			enum drmmode_flip_sync flip_sync,
			uint32_t target_msc);
int drmmode_crtc_get_ust_msc(xf86CrtcPtr crtc, CARD64 *ust, CARD64 *msc);
int drmmode_get_current_ust(int drm_fd, CARD64 * ust);
void drmmode_crtc_set_vrr(xf86CrtcPtr crtc, Bool enabled);

Bool drmmode_wait_vblank(xf86CrtcPtr crtc, drmVBlankSeqType type,
			 uint32_t target_seq, unsigned long signal,
			 uint64_t *ust, uint32_t *result_seq);


extern miPointerSpriteFuncRec drmmode_sprite_funcs;


#endif

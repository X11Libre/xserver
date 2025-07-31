/*
 * Copyright © 2007 Red Hat, Inc.
 * Copyright © 2015 Advanced Micro Devices, Inc.
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
#include <X11/Xdefs.h>
#include <list.h>

#include "amdgpu_drv.h"
#include "amdgpu_drm_queue.h"


struct amdgpu_drm_queue_entry {
	struct xorg_list list;
	uint64_t usec;
	uint64_t id;
	uintptr_t seq;
	void *data;
	ClientPtr client;
	xf86CrtcPtr crtc;
	amdgpu_drm_handler_proc handler;
	amdgpu_drm_abort_proc abort;
	Bool is_flip;
	unsigned int frame;
};

static int amdgpu_drm_queue_refcnt;
static struct xorg_list amdgpu_drm_queue;
static struct xorg_list amdgpu_drm_flip_signalled;
static struct xorg_list amdgpu_drm_vblank_signalled;
static struct xorg_list amdgpu_drm_vblank_deferred;
static uintptr_t amdgpu_drm_queue_seq;


/*
 * Process a DRM event
 */
static void
amdgpu_drm_queue_handle_one(struct amdgpu_drm_queue_entry *e)
{
	xorg_list_del(&e->list);
	if (e->handler) {
		e->handler(e->crtc, e->frame, e->usec, e->data);
	} else
		e->abort(e->crtc, e->data);
	free(e);
}

/*
 * Abort one queued DRM entry, removing it
 * from the list, calling the abort function and
 * freeing the memory
 */
static void
amdgpu_drm_abort_one(struct amdgpu_drm_queue_entry *e)
{
	xorg_list_del(&e->list);
	e->abort(e->crtc, e->data);
	free(e);
}

static void
amdgpu_drm_queue_handler(int fd, unsigned int frame, unsigned int sec,
			 unsigned int usec, void *user_ptr)
{
	uintptr_t seq = (uintptr_t)user_ptr;
	struct amdgpu_drm_queue_entry *e, *tmp;

	xorg_list_for_each_entry_safe(e, tmp, &amdgpu_drm_queue, list) {
		if (e->seq == seq) {
			if (!e->handler) {
				amdgpu_drm_abort_one(e);
				break;
			}

			xorg_list_del(&e->list);
			e->usec = (uint64_t)sec * 1000000 + usec;
			e->frame = frame;
			xorg_list_append(&e->list, e->is_flip ?
					 &amdgpu_drm_flip_signalled :
					 &amdgpu_drm_vblank_signalled);
			break;
		}
	}
}

/*
 * Handle signalled vblank events. If we're waiting for a flip event,
 * put events for that CRTC in the vblank_deferred list.
 */
static void
amdgpu_drm_handle_vblank_signalled(void)
{
	drmmode_crtc_private_ptr drmmode_crtc;
	struct amdgpu_drm_queue_entry *e;

	while (!xorg_list_is_empty(&amdgpu_drm_vblank_signalled)) {
		e = xorg_list_first_entry(&amdgpu_drm_vblank_signalled,
					  struct amdgpu_drm_queue_entry, list);
		drmmode_crtc = e->crtc->driver_private;

		if (drmmode_crtc->wait_flip_nesting_level == 0) {
			amdgpu_drm_queue_handle_one(e);
			continue;
		}

		xorg_list_del(&e->list);
		xorg_list_append(&e->list, &amdgpu_drm_vblank_deferred);
	}
}

/*
 * Handle deferred DRM vblank events
 *
 * This function must be called after amdgpu_drm_wait_pending_flip, once
 * it's safe to attempt queueing a flip again
 */
void
amdgpu_drm_queue_handle_deferred(xf86CrtcPtr crtc)
{
	drmmode_crtc_private_ptr drmmode_crtc = crtc->driver_private;
	struct amdgpu_drm_queue_entry *e, *tmp;

	if (drmmode_crtc->wait_flip_nesting_level == 0 ||
	    --drmmode_crtc->wait_flip_nesting_level > 0)
		return;

	/* Put previously deferred vblank events for this CRTC back in the
	 * signalled queue
	 */
	xorg_list_for_each_entry_safe(e, tmp, &amdgpu_drm_vblank_deferred, list) {
		if (e->crtc != crtc)
			continue;

		xorg_list_del(&e->list);
		xorg_list_append(&e->list, &amdgpu_drm_vblank_signalled);
	}

	amdgpu_drm_handle_vblank_signalled();
}

/*
 * Enqueue a potential drm response; when the associated response
 * appears, we've got data to pass to the handler from here
 */
uintptr_t
amdgpu_drm_queue_alloc(xf86CrtcPtr crtc, ClientPtr client,
		       uint64_t id, void *data,
		       amdgpu_drm_handler_proc handler,
		       amdgpu_drm_abort_proc abort,
		       Bool is_flip)
{
	struct amdgpu_drm_queue_entry *e;

	e = calloc(1, sizeof(struct amdgpu_drm_queue_entry));
	if (!e)
		return AMDGPU_DRM_QUEUE_ERROR;

	if (_X_UNLIKELY(amdgpu_drm_queue_seq == AMDGPU_DRM_QUEUE_ERROR))
		amdgpu_drm_queue_seq++;

	e->seq = amdgpu_drm_queue_seq++;
	e->client = client;
	e->crtc = crtc;
	e->id = id;
	e->data = data;
	e->handler = handler;
	e->abort = abort;
	e->is_flip = is_flip;

	xorg_list_append(&e->list, &amdgpu_drm_queue);

	return e->seq;
}

/*
 * Abort drm queue entries for a client
 *
 * NOTE: This keeps the entries in the list until the DRM event arrives,
 * but then it calls the abort functions instead of the handler
 * functions.
 */
void
amdgpu_drm_abort_client(ClientPtr client)
{
	struct amdgpu_drm_queue_entry *e;

	xorg_list_for_each_entry(e, &amdgpu_drm_queue, list) {
		if (e->client == client)
			e->handler = NULL;
	}
}

/*
 * Abort specific drm queue entry
 */
void
amdgpu_drm_abort_entry(uintptr_t seq)
{
	struct amdgpu_drm_queue_entry *e, *tmp;

	if (seq == AMDGPU_DRM_QUEUE_ERROR)
		return;

	xorg_list_for_each_entry_safe(e, tmp, &amdgpu_drm_vblank_signalled, list) {
		if (e->seq == seq) {
			amdgpu_drm_abort_one(e);
			return;
		}
	}

	xorg_list_for_each_entry_safe(e, tmp, &amdgpu_drm_vblank_deferred, list) {
		if (e->seq == seq) {
			amdgpu_drm_abort_one(e);
			return;
		}
	}

	xorg_list_for_each_entry_safe(e, tmp, &amdgpu_drm_queue, list) {
		if (e->seq == seq) {
			amdgpu_drm_abort_one(e);
			break;
		}
	}
}

/*
 * Abort specific drm queue entry by ID
 */
void
amdgpu_drm_abort_id(uint64_t id)
{
	struct amdgpu_drm_queue_entry *e, *tmp;

	xorg_list_for_each_entry_safe(e, tmp, &amdgpu_drm_queue, list) {
		if (e->id == id) {
			amdgpu_drm_abort_one(e);
			break;
		}
	}
}

/*
 * drmHandleEvent wrapper
 */
int
amdgpu_drm_handle_event(int fd, drmEventContext *event_context)
{
	struct amdgpu_drm_queue_entry *e;
	int r;

	/* Retry drmHandleEvent if it was interrupted by a signal in read() */
	do {
		r = drmHandleEvent(fd, event_context);
	} while (r < 0 && (errno == EINTR || errno == EAGAIN));

	if (r < 0) {
		static Bool printed;

		if (!printed) {
			ErrorF("%s: drmHandleEvent returned %d, errno=%d (%s)\n",
			       __func__, r, errno, strerror(errno));
			printed = TRUE;
		}
	}

	while (!xorg_list_is_empty(&amdgpu_drm_flip_signalled)) {
		e = xorg_list_first_entry(&amdgpu_drm_flip_signalled,
					  struct amdgpu_drm_queue_entry, list);
		amdgpu_drm_queue_handle_one(e);
	}

	amdgpu_drm_handle_vblank_signalled();

	return r;
}

/*
 * Wait for pending page flip on given CRTC to complete
 */
void amdgpu_drm_wait_pending_flip(xf86CrtcPtr crtc)
{
	drmmode_crtc_private_ptr drmmode_crtc = crtc->driver_private;
	AMDGPUEntPtr pAMDGPUEnt = AMDGPUEntPriv(crtc->scrn);
	struct amdgpu_drm_queue_entry *e;

	drmmode_crtc->wait_flip_nesting_level++;

	while (drmmode_crtc->flip_pending &&
	       !xorg_list_is_empty(&amdgpu_drm_flip_signalled)) {
		e = xorg_list_first_entry(&amdgpu_drm_flip_signalled,
					  struct amdgpu_drm_queue_entry, list);
		amdgpu_drm_queue_handle_one(e);
	}

	while (drmmode_crtc->flip_pending
	       && amdgpu_drm_handle_event(pAMDGPUEnt->fd,
					  &drmmode_crtc->drmmode->event_context) >= 0);
}

/*
 * Initialize the DRM event queue
 */
void
amdgpu_drm_queue_init(ScrnInfoPtr scrn)
{
	AMDGPUInfoPtr info = AMDGPUPTR(scrn);
	drmmode_ptr drmmode = &info->drmmode;

	drmmode->event_context.version = 2;
	drmmode->event_context.vblank_handler = amdgpu_drm_queue_handler;
	drmmode->event_context.page_flip_handler = amdgpu_drm_queue_handler;

	if (amdgpu_drm_queue_refcnt++)
		return;

	xorg_list_init(&amdgpu_drm_queue);
	xorg_list_init(&amdgpu_drm_flip_signalled);
	xorg_list_init(&amdgpu_drm_vblank_signalled);
	xorg_list_init(&amdgpu_drm_vblank_deferred);
}

/*
 * Deinitialize the DRM event queue
 */
void
amdgpu_drm_queue_close(ScrnInfoPtr scrn)
{
	struct amdgpu_drm_queue_entry *e, *tmp;

	xorg_list_for_each_entry_safe(e, tmp, &amdgpu_drm_queue, list) {
		if (e->crtc->scrn == scrn)
			amdgpu_drm_abort_one(e);
	}

	amdgpu_drm_queue_refcnt--;
}

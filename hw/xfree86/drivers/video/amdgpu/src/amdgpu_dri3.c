/*
 * Copyright © 2013-2014 Intel Corporation
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

#include "amdgpu_glamor.h"
#include "amdgpu_pixmap.h"
#include "dri3.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <gbm.h>
#include <errno.h>
#include <libgen.h>

static int open_card_node(ScreenPtr screen, int *out)
{
	ScrnInfoPtr scrn = xf86ScreenToScrn(screen);
	AMDGPUEntPtr pAMDGPUEnt = AMDGPUEntPriv(scrn);
	AMDGPUInfoPtr info = AMDGPUPTR(scrn);
	drm_magic_t magic;
	int fd;

	fd = open(info->dri2.device_name, O_RDWR | O_CLOEXEC);
	if (fd < 0)
		return BadAlloc;

	/* Before FD passing in the X protocol with DRI3 (and increased
	 * security of rendering with per-process address spaces on the
	 * GPU), the kernel had to come up with a way to have the server
	 * decide which clients got to access the GPU, which was done by
	 * each client getting a unique (magic) number from the kernel,
	 * passing it to the server, and the server then telling the
	 * kernel which clients were authenticated for using the device.
	 *
	 * Now that we have FD passing, the server can just set up the
	 * authentication on its own and hand the prepared FD off to the
	 * client.
	 */
	if (drmGetMagic(fd, &magic) < 0) {
		if (errno == EACCES) {
			/* Assume that we're on a render node, and the fd is
			 * already as authenticated as it should be.
			 */
			*out = fd;
			return Success;
		} else {
			close(fd);
			return BadMatch;
		}
	}

	if (drmAuthMagic(pAMDGPUEnt->fd, magic) < 0) {
		close(fd);
		return BadMatch;
	}

	*out = fd;
	return Success;
}

static int open_render_node(ScreenPtr screen, int *out)
{
	ScrnInfoPtr scrn = xf86ScreenToScrn(screen);
	AMDGPUEntPtr pAMDGPUEnt = AMDGPUEntPriv(scrn);
	int fd;

	fd = open(pAMDGPUEnt->render_node, O_RDWR | O_CLOEXEC);
	if (fd < 0)
		return BadAlloc;

	*out = fd;
	return Success;
}

static int
amdgpu_dri3_open(ScreenPtr screen, RRProviderPtr provider, int *out)
{
	ScrnInfoPtr scrn = xf86ScreenToScrn(screen);
	AMDGPUEntPtr pAMDGPUEnt = AMDGPUEntPriv(scrn);
	int ret = BadAlloc;

	if (pAMDGPUEnt->render_node)
		ret = open_render_node(screen, out);

	if (ret != Success)
		ret = open_card_node(screen, out);

	return ret;
}

static PixmapPtr amdgpu_dri3_pixmap_from_fd(ScreenPtr screen,
					    int fd,
					    CARD16 width,
					    CARD16 height,
					    CARD16 stride,
					    CARD8 depth,
					    CARD8 bpp)
{
	PixmapPtr pixmap;

#ifdef USE_GLAMOR
	/* Avoid generating a GEM flink name if possible */
	if (AMDGPUPTR(xf86ScreenToScrn(screen))->use_glamor) {
		pixmap = glamor_pixmap_from_fd(screen, fd, width, height,
					       stride, depth, bpp);
		if (pixmap) {
			struct amdgpu_pixmap *priv = calloc(1, sizeof(*priv));

			if (priv) {
				amdgpu_set_pixmap_private(pixmap, priv);
				pixmap->usage_hint |= AMDGPU_CREATE_PIXMAP_DRI2;
				return pixmap;
			}

			screen->DestroyPixmap(pixmap);
			return NULL;
		}
	}
#endif

	if (depth < 8)
		return NULL;

	switch (bpp) {
	case 8:
	case 16:
	case 32:
		break;
	default:
		return NULL;
	}

	pixmap = screen->CreatePixmap(screen, 0, 0, depth,
				      AMDGPU_CREATE_PIXMAP_DRI2);
	if (!pixmap)
		return NULL;

	if (!screen->ModifyPixmapHeader(pixmap, width, height, 0, bpp, stride,
					NULL))
		goto free_pixmap;

	if (screen->SetSharedPixmapBacking(pixmap, (void*)(intptr_t)fd))
		return pixmap;

free_pixmap:
	fbDestroyPixmap(pixmap);
	return NULL;
}

static int amdgpu_dri3_fd_from_pixmap(ScreenPtr screen,
				      PixmapPtr pixmap,
				      CARD16 *stride,
				      CARD32 *size)
{
	struct amdgpu_buffer *bo;
	struct amdgpu_bo_info bo_info;
	uint32_t fd;
#ifdef USE_GLAMOR
	ScrnInfoPtr scrn = xf86ScreenToScrn(screen);
	AMDGPUInfoPtr info = AMDGPUPTR(scrn);

	if (info->use_glamor) {
		int ret = glamor_fd_from_pixmap(screen, pixmap, stride, size);

		/* Any pending drawing operations need to be flushed to the
		 * kernel driver before the client starts using the pixmap
		 * storage for direct rendering.
		 */
		if (ret >= 0)
			amdgpu_glamor_flush(scrn);

		return ret;
	}
#endif

	bo = amdgpu_get_pixmap_bo(pixmap);
	if (!bo)
		return -1;

	if (pixmap->devKind > UINT16_MAX)
		return -1;

	if (amdgpu_bo_query_info(bo->bo.amdgpu, &bo_info) != 0)
		return -1;

	if (amdgpu_bo_export(bo->bo.amdgpu, amdgpu_bo_handle_type_dma_buf_fd,
			     &fd) != 0)
		return -1;

	*stride = pixmap->devKind;
	*size = bo_info.alloc_size;
	return fd;
}

static dri3_screen_info_rec amdgpu_dri3_screen_info = {
	.version = 0,
	.open = amdgpu_dri3_open,
	.pixmap_from_fd = amdgpu_dri3_pixmap_from_fd,
	.fd_from_pixmap = amdgpu_dri3_fd_from_pixmap
};

Bool
amdgpu_dri3_screen_init(ScreenPtr screen)
{
	ScrnInfoPtr scrn = xf86ScreenToScrn(screen);
	AMDGPUEntPtr pAMDGPUEnt = AMDGPUEntPriv(scrn);

	pAMDGPUEnt->render_node = drmGetRenderDeviceNameFromFd(pAMDGPUEnt->fd);

	if (!dri3_screen_init(screen, &amdgpu_dri3_screen_info)) {
		xf86DrvMsg(scrn->scrnIndex, X_WARNING,
			   "dri3_screen_init failed\n");
		return FALSE;
	}

	return TRUE;
}

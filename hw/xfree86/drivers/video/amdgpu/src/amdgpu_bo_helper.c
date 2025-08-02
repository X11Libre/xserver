/*
 * Copyright 2012  Advanced Micro Devices, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */
#include "config.h"
#include <xorg-server.h>

#include <sys/mman.h>
#include <gbm.h>

#include "amdgpu_drv.h"
#include "amdgpu_bo_helper.h"
#include "amdgpu_glamor.h"
#include "amdgpu_pixmap.h"

static uint32_t
amdgpu_get_gbm_format(int depth, int bitsPerPixel)
{
	switch (depth) {
#ifdef GBM_FORMAT_R8
	case 8:
		return GBM_FORMAT_R8;
#endif
	case 15:
		return GBM_FORMAT_ARGB1555;
	case 16:
		return GBM_FORMAT_RGB565;
	case 32:
		return GBM_FORMAT_ARGB8888;
	case 30:
		return GBM_FORMAT_XRGB2101010;
	case 24:
		if (bitsPerPixel == 32)
			return GBM_FORMAT_XRGB8888;
		/* fall through */
	default:
		ErrorF("%s: Unsupported depth/bpp %d/%d\n", __func__,
		       depth, bitsPerPixel);
		return ~0U;
	}
}

/* Calculate appropriate pitch for a pixmap and allocate a BO that can hold it.
 */
struct amdgpu_buffer *amdgpu_alloc_pixmap_bo(ScrnInfoPtr pScrn, int width,
					      int height, int depth, int usage_hint,
					      int bitsPerPixel, int *new_pitch)
{
	AMDGPUInfoPtr info = AMDGPUPTR(pScrn);
	struct amdgpu_buffer *pixmap_buffer;

	if (!(usage_hint & AMDGPU_CREATE_PIXMAP_GTT) && info->gbm) {
		uint32_t bo_use = GBM_BO_USE_RENDERING;
		uint32_t gbm_format = amdgpu_get_gbm_format(depth, bitsPerPixel);

		if (gbm_format == ~0U)
			return NULL;

		pixmap_buffer = (struct amdgpu_buffer *)calloc(1, sizeof(struct amdgpu_buffer));
		if (!pixmap_buffer) {
			return NULL;
		}
		pixmap_buffer->ref_count = 1;

		if (usage_hint & AMDGPU_CREATE_PIXMAP_SCANOUT)
			bo_use |= GBM_BO_USE_SCANOUT;

		if (usage_hint & AMDGPU_CREATE_PIXMAP_FRONT)
			bo_use |= GBM_BO_USE_FRONT_RENDERING;

		if (usage_hint == CREATE_PIXMAP_USAGE_SHARED ||
		    (usage_hint & AMDGPU_CREATE_PIXMAP_LINEAR)) {
			bo_use |= GBM_BO_USE_LINEAR;
		}

		pixmap_buffer->bo.gbm = gbm_bo_create(info->gbm, width, height,
						      gbm_format,
						      bo_use);
		if (!pixmap_buffer->bo.gbm) {
			free(pixmap_buffer);
			return NULL;
		}

		pixmap_buffer->flags |= AMDGPU_BO_FLAGS_GBM;

		if (new_pitch)
			*new_pitch = gbm_bo_get_stride(pixmap_buffer->bo.gbm);
	} else {
		AMDGPUEntPtr pAMDGPUEnt = AMDGPUEntPriv(pScrn);
		unsigned cpp = (bitsPerPixel + 7) / 8;
		unsigned pitch = cpp *
			AMDGPU_ALIGN(width, drmmode_get_pitch_align(pScrn, cpp));
		uint32_t domain = (usage_hint & AMDGPU_CREATE_PIXMAP_GTT) ?
			AMDGPU_GEM_DOMAIN_GTT : AMDGPU_GEM_DOMAIN_VRAM;

		pixmap_buffer = amdgpu_bo_open(pAMDGPUEnt->pDev, pitch * height,
					       4096, domain);

		if (new_pitch)
			*new_pitch = pitch;
	}

	return pixmap_buffer;
}

/* Clear the pixmap contents to black */
void
amdgpu_pixmap_clear(PixmapPtr pixmap)
{
	ScreenPtr screen = pixmap->drawable.pScreen;
	AMDGPUInfoPtr info = AMDGPUPTR(xf86ScreenToScrn(screen));
	GCPtr gc = GetScratchGC(pixmap->drawable.depth, screen);
	xRectangle rect;

	ValidateGC(&pixmap->drawable, gc);
	rect.x = 0;
	rect.y = 0;
	rect.width = pixmap->drawable.width;
	rect.height = pixmap->drawable.height;
	info->force_accel = TRUE;
	gc->ops->PolyFillRect(&pixmap->drawable, gc, 1, &rect);
	info->force_accel = FALSE;
	FreeScratchGC(gc);
}

Bool amdgpu_bo_get_handle(struct amdgpu_buffer *bo, uint32_t *handle)
{
	if (bo->flags & AMDGPU_BO_FLAGS_GBM) {
		*handle = gbm_bo_get_handle(bo->bo.gbm).u32;
		return TRUE;
	}

	return amdgpu_bo_export(bo->bo.amdgpu, amdgpu_bo_handle_type_kms,
				handle) == 0;
}

#ifdef USE_GLAMOR

static void amdgpu_pixmap_do_get_tiling_info(PixmapPtr pixmap)
{
	struct amdgpu_pixmap *priv = amdgpu_get_pixmap_private(pixmap);
	ScreenPtr screen = pixmap->drawable.pScreen;
	ScrnInfoPtr scrn = xf86ScreenToScrn(screen);
	AMDGPUEntPtr pAMDGPUEnt = AMDGPUEntPriv(scrn);
	struct drm_amdgpu_gem_metadata gem_metadata;

	gem_metadata.handle = priv->handle;
	gem_metadata.op = AMDGPU_GEM_METADATA_OP_GET_METADATA;

	if (drmCommandWriteRead(pAMDGPUEnt->fd, DRM_AMDGPU_GEM_METADATA,
				&gem_metadata, sizeof(gem_metadata)) == 0)
		priv->tiling_info = gem_metadata.data.tiling_info;
}

#endif

uint64_t amdgpu_pixmap_get_tiling_info(PixmapPtr pixmap)
{
	struct amdgpu_pixmap *priv = amdgpu_get_pixmap_private(pixmap);
	uint32_t handle;

	if (!priv || !priv->handle_valid) {
		amdgpu_pixmap_get_handle(pixmap, &handle);
		priv = amdgpu_get_pixmap_private(pixmap);
	}

	return priv->tiling_info;
}

Bool amdgpu_pixmap_get_handle(PixmapPtr pixmap, uint32_t *handle)
{
	ScreenPtr screen = pixmap->drawable.pScreen;
	ScrnInfoPtr scrn = xf86ScreenToScrn(screen);
	AMDGPUInfoPtr info = AMDGPUPTR(scrn);
	struct amdgpu_pixmap *priv;

	if (info->shadow_fb)
		return FALSE;

	priv = amdgpu_get_pixmap_private(pixmap);
	if (!priv) {
		priv = calloc(1, sizeof(*priv));
		amdgpu_set_pixmap_private(pixmap, priv);
	}

	if (priv->handle_valid)
		goto success;

#ifdef USE_GLAMOR
	if (info->use_glamor) {
		AMDGPUEntPtr pAMDGPUEnt = AMDGPUEntPriv(scrn);
		CARD16 stride;
		CARD32 size;
		int fd, r;

		fd = glamor_fd_from_pixmap(screen, pixmap, &stride, &size);
		if (fd < 0)
			return FALSE;

		r = drmPrimeFDToHandle(pAMDGPUEnt->fd, fd, &priv->handle);
		close(fd);
		if (r)
			return FALSE;

		amdgpu_pixmap_do_get_tiling_info(pixmap);
		goto success;
	}
#endif

	if (!priv->bo || !amdgpu_bo_get_handle(priv->bo, &priv->handle))
		return FALSE;

 success:
	priv->handle_valid = TRUE;
	*handle = priv->handle;
	return TRUE;
}

int amdgpu_bo_map(ScrnInfoPtr pScrn, struct amdgpu_buffer *bo)
{
	int ret = 0;

	if (bo->flags & AMDGPU_BO_FLAGS_GBM) {
		AMDGPUEntPtr pAMDGPUEnt = AMDGPUEntPriv(pScrn);
		uint32_t handle, stride, height;
		union drm_amdgpu_gem_mmap args;
		int fd = pAMDGPUEnt->fd;
		void *ptr;

		handle = gbm_bo_get_handle(bo->bo.gbm).u32;
		stride = gbm_bo_get_stride(bo->bo.gbm);
		height = gbm_bo_get_height(bo->bo.gbm);

		memset(&args, 0, sizeof(union drm_amdgpu_gem_mmap));
		args.in.handle = handle;

		ret = drmCommandWriteRead(fd, DRM_AMDGPU_GEM_MMAP,
					&args, sizeof(args));
		if (ret) {
			ErrorF("Failed to get the mmap offset\n");
			return ret;
		}

		ptr = mmap(NULL, stride * height,
			PROT_READ | PROT_WRITE, MAP_SHARED,
			fd, args.out.addr_ptr);

		if (!ptr) {
			ErrorF("Failed to mmap the bo\n");
			return -1;
		}

		bo->cpu_ptr = ptr;
	} else
		ret = amdgpu_bo_cpu_map(bo->bo.amdgpu, &bo->cpu_ptr);

	return ret;
}

void amdgpu_bo_unmap(struct amdgpu_buffer *bo)
{
	if (!bo->cpu_ptr)
		return;

	if (bo->flags & AMDGPU_BO_FLAGS_GBM) {
		uint32_t stride, height;
		stride = gbm_bo_get_stride(bo->bo.gbm);
		height = gbm_bo_get_height(bo->bo.gbm);
		munmap(bo->cpu_ptr, stride * height);
	} else
		amdgpu_bo_cpu_unmap(bo->bo.amdgpu);
}

struct amdgpu_buffer *amdgpu_bo_open(amdgpu_device_handle pDev,
				       uint32_t alloc_size,
				       uint32_t phys_alignment,
				       uint32_t domains)
{
	struct amdgpu_bo_alloc_request alloc_request;
	struct amdgpu_buffer *bo = NULL;

	memset(&alloc_request, 0, sizeof(struct amdgpu_bo_alloc_request));

	bo = (struct amdgpu_buffer *)calloc(1, sizeof(struct amdgpu_buffer));
	if (!bo)
		return NULL;

	alloc_request.alloc_size = alloc_size;
	alloc_request.phys_alignment = phys_alignment;
	alloc_request.preferred_heap = domains;

	if (amdgpu_bo_alloc(pDev, &alloc_request, &bo->bo.amdgpu)) {
		free(bo);
		return NULL;
	}

	bo->ref_count = 1;

	return bo;
}

void amdgpu_bo_ref(struct amdgpu_buffer *buffer)
{
	buffer->ref_count++;
}

void amdgpu_bo_unref(struct amdgpu_buffer **buffer)
{
	struct amdgpu_buffer *buf = *buffer;

	buf->ref_count--;
	if (buf->ref_count) {
		return;
	}

	amdgpu_bo_unmap(buf);

	if (buf->flags & AMDGPU_BO_FLAGS_GBM) {
		gbm_bo_destroy(buf->bo.gbm);
	} else {
		amdgpu_bo_free(buf->bo.amdgpu);
	}
	free(buf);
	*buffer = NULL;
}

int amdgpu_query_bo_size(amdgpu_bo_handle buf_handle, uint32_t *size)
{
	struct amdgpu_bo_info buffer_info;
	int ret;

	memset(&buffer_info, 0, sizeof(struct amdgpu_bo_info));
	ret = amdgpu_bo_query_info(buf_handle, &buffer_info);
	if (ret)
		*size = 0;
	else
		*size = (uint32_t)(buffer_info.alloc_size);

	return ret;
}

int amdgpu_query_heap_size(amdgpu_device_handle pDev,
			    uint32_t heap,
			    uint64_t *heap_size,
			    uint64_t *max_allocation)
{
	struct amdgpu_heap_info heap_info;
	int ret;

	memset(&heap_info, 0, sizeof(struct amdgpu_heap_info));
	ret = amdgpu_query_heap_info(pDev, heap, 0, &heap_info);
	if (ret) {
		*heap_size = 0;
		*max_allocation = 0;
	} else {
		*heap_size = heap_info.heap_size;
		*max_allocation = heap_info.max_allocation;
	}

	return ret;
}

struct amdgpu_buffer *amdgpu_gem_bo_open_prime(amdgpu_device_handle pDev,
						 int fd_handle,
						 uint32_t size)
{
	struct amdgpu_buffer *bo = NULL;
	struct amdgpu_bo_import_result buffer = {0};

	bo = (struct amdgpu_buffer *)calloc(1, sizeof(struct amdgpu_buffer));
	if (!bo)
		return NULL;

	if (amdgpu_bo_import(pDev, amdgpu_bo_handle_type_dma_buf_fd,
			     (uint32_t)fd_handle, &buffer)) {
		free(bo);
		return FALSE;
	}
	bo->bo.amdgpu = buffer.buf_handle;
	bo->ref_count = 1;

	return bo;
}


Bool amdgpu_set_shared_pixmap_backing(PixmapPtr ppix, void *fd_handle)
{
	ScrnInfoPtr pScrn = xf86ScreenToScrn(ppix->drawable.pScreen);
	AMDGPUInfoPtr info = AMDGPUPTR(pScrn);
	AMDGPUEntPtr pAMDGPUEnt = AMDGPUEntPriv(pScrn);
	struct amdgpu_buffer *pixmap_buffer = NULL;
	int ihandle = (int)(long)fd_handle;
	uint32_t size = ppix->devKind * ppix->drawable.height;
	Bool ret;

	if (ihandle == -1)
		return amdgpu_set_pixmap_bo(ppix, NULL);

	if (info->gbm) {
		struct amdgpu_buffer *bo;
		struct gbm_import_fd_data data;
		uint32_t bo_use = GBM_BO_USE_RENDERING;

		data.format = amdgpu_get_gbm_format(ppix->drawable.depth,
						    ppix->drawable.bitsPerPixel);
		if (data.format == ~0U)
			return FALSE;

		bo = calloc(1, sizeof(struct amdgpu_buffer));
		if (!bo)
			return FALSE;
		bo->ref_count = 1;

		data.fd = ihandle;
		data.width = ppix->drawable.width;
		data.height = ppix->drawable.height;
		data.stride = ppix->devKind;

		if (ppix->drawable.bitsPerPixel == pScrn->bitsPerPixel)
			bo_use |= GBM_BO_USE_SCANOUT;

		bo->bo.gbm = gbm_bo_import(info->gbm, GBM_BO_IMPORT_FD, &data,
					   bo_use);
		if (!bo->bo.gbm) {
			free(bo);
			return FALSE;
		}

		bo->flags |= AMDGPU_BO_FLAGS_GBM;

#ifdef USE_GLAMOR
		if (info->use_glamor &&
		    !amdgpu_glamor_create_textured_pixmap(ppix, bo)) {
			amdgpu_bo_unref(&bo);
			return FALSE;
		}
#endif

		ret = amdgpu_set_pixmap_bo(ppix, bo);
		/* amdgpu_set_pixmap_bo increments ref_count if it succeeds */
		amdgpu_bo_unref(&bo);
		return ret;
	}

	pixmap_buffer = amdgpu_gem_bo_open_prime(pAMDGPUEnt->pDev, ihandle, size);
	if (!pixmap_buffer) {
		return FALSE;
	}

	close(ihandle);

	ret = amdgpu_set_pixmap_bo(ppix, pixmap_buffer);

	/* we have a reference from the alloc and one from set pixmap bo,
	   drop one */
	amdgpu_bo_unref(&pixmap_buffer);

	return ret;
}

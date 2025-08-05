/* Copyright © 2014 Advanced Micro Devices, Inc.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including
 * the next paragraph) shall be included in all copies or substantial
 * portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT.  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */
#include "config.h"
#include <xorg-server.h>

#include <xf86.h>
#include "amdgpu_pixmap.h"
#include "amdgpu_bo_helper.h"

static PixmapPtr
amdgpu_pixmap_create(ScreenPtr screen, int w, int h, int depth,	unsigned usage)
{
	ScrnInfoPtr scrn;
	struct amdgpu_pixmap *priv;
	PixmapPtr pixmap;
	AMDGPUInfoPtr info;

	/* only DRI2 pixmap is supported */
	if (!(usage & AMDGPU_CREATE_PIXMAP_DRI2))
		return fbCreatePixmap(screen, w, h, depth, usage);

	if (w > 32767 || h > 32767)
		return NullPixmap;

	if (depth == 1)
		return fbCreatePixmap(screen, w, h, depth, usage);

	pixmap = fbCreatePixmap(screen, 0, 0, depth, usage);
	if (pixmap == NullPixmap)
		return pixmap;

	if (w && h) {
		int stride;

		priv = calloc(1, sizeof(struct amdgpu_pixmap));
		if (!priv)
			goto fallback_pixmap;

		scrn = xf86ScreenToScrn(screen);
		info = AMDGPUPTR(scrn);
		if (!info->use_glamor)
			usage |= AMDGPU_CREATE_PIXMAP_LINEAR;
		priv->bo = amdgpu_alloc_pixmap_bo(scrn, w, h, depth, usage,
						  pixmap->drawable.bitsPerPixel,
						  &stride);
		if (!priv->bo)
			goto fallback_priv;

		amdgpu_set_pixmap_private(pixmap, priv);

		if (amdgpu_bo_map(scrn, priv->bo)) {
			ErrorF("Failed to mmap the bo\n");
			goto fallback_bo;
		}

		screen->ModifyPixmapHeader(pixmap, w, h, 0, 0, stride,
					   priv->bo->cpu_ptr);
	}

	return pixmap;

fallback_bo:
	amdgpu_bo_unref(&priv->bo);
fallback_priv:
	free(priv);
fallback_pixmap:
	fbDestroyPixmap(pixmap);
	return fbCreatePixmap(screen, w, h, depth, usage);
}

static Bool amdgpu_pixmap_destroy(PixmapPtr pixmap)
{
	if (pixmap->refcnt == 1) {
		amdgpu_set_pixmap_bo(pixmap, NULL);
	}
	fbDestroyPixmap(pixmap);
	return TRUE;
}

/* This should only be called when glamor is disabled */
Bool amdgpu_pixmap_init(ScreenPtr screen)
{
	if (!dixRegisterPrivateKey(&amdgpu_pixmap_index, PRIVATE_PIXMAP, 0))
		return FALSE;

	screen->CreatePixmap = amdgpu_pixmap_create;
	screen->DestroyPixmap = amdgpu_pixmap_destroy;
	return TRUE;
}

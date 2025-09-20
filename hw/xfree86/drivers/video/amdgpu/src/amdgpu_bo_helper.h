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

#ifndef AMDGPU_BO_HELPER_H
#define AMDGPU_BO_HELPER_H 1

#include "amdgpu_drv.h"

extern struct amdgpu_buffer *amdgpu_alloc_pixmap_bo(ScrnInfoPtr pScrn, int width,
						     int height, int depth, int usage_hint,
						     int bitsPerPixel, int *new_pitch);

extern void amdgpu_pixmap_clear(PixmapPtr pixmap);

extern Bool amdgpu_bo_get_handle(struct amdgpu_buffer *bo, uint32_t *handle);

extern uint64_t amdgpu_pixmap_get_tiling_info(PixmapPtr pixmap);

extern Bool amdgpu_pixmap_get_handle(PixmapPtr pixmap, uint32_t *handle);

extern int amdgpu_bo_map(ScrnInfoPtr pScrn, struct amdgpu_buffer *bo);

extern void amdgpu_bo_unmap(struct amdgpu_buffer *bo);

extern Bool
amdgpu_set_shared_pixmap_backing(PixmapPtr ppix, void *fd_handle);

/* helper function to allocate memory to be used for GPU operations
 *
 * \param	pDev		- \c [in] device handle
 * \param	alloc_size	- \c [in] allocation size
 * \param	phys_alignment	- \c [in] requested alignment. 0 means no alignment requirement
 * \param	domains		- \c [in] GEM domains
 *
 * \return	pointer to amdgpu_buffer on success
 *		NULL on failure
*/
extern struct amdgpu_buffer *amdgpu_bo_open(amdgpu_device_handle pDev,
					      uint32_t alloc_size,
					      uint32_t phys_alignment,
					      uint32_t domains);

/* helper function to add the ref_count of a amdgpu_buffer
 * \param	buffer	- \c [in] amdgpu_buffer
*/
extern void amdgpu_bo_ref(struct amdgpu_buffer *buffer);

/* helper function to dec the ref_count of a amdgpu_buffer
 * \param	buffer	- \c [in] amdgpu_buffer
*/
extern void amdgpu_bo_unref(struct amdgpu_buffer **buffer);

/* helper function to query the buffer size
 * \param	buf_handle	- \c [in] amdgpu bo handle
 * \param	size		- \c [out] pointer to buffer size
 *
 * \return	0 on success
		>0 - AMD specific error code \n
		<0 - Negative POSIX error code
*/
int amdgpu_query_bo_size(amdgpu_bo_handle buf_handle, uint32_t *size);

/* helper function to query the heap information
 * \param	pDev		- \c [in] amdgpu device handle
 * \param 	heap		- \c [in] heap type
 * \param	heap_size	- \c [out] theoretical max available memory
 * \param	max_allcoation	- \c [out] theoretical possible max. size of buffer
 *
 * \return 	0 on success
		>0 - AMD specific error code \n
		<0 - Negative POSIX error code
*/
int amdgpu_query_heap_size(amdgpu_device_handle pDev,
                            uint32_t heap,
                            uint64_t *heap_size,
                            uint64_t *max_allocation);

/* helper function to convert a DMA buf handle to a KMS handle
 * \param	pDev		- \c [in] amdgpu device handle
 * \param	fd_handle 	- \c [in] dma-buf fd handle
 * \size	size		- \c [in] buffer size
 *
 * \return	pointer to amdgpu_buffer on success
		NULL on failure
*/
struct amdgpu_buffer *amdgpu_gem_bo_open_prime(amdgpu_device_handle pDev,
                                                 int fd_handle,
                                                 uint32_t size);

/**
 * get_drawable_pixmap() returns the backing pixmap for a given drawable.
 *
 * @param drawable the drawable being requested.
 *
 * This function returns the backing pixmap for a drawable, whether it is a
 * redirected window, unredirected window, or already a pixmap.
 */
static inline PixmapPtr get_drawable_pixmap(DrawablePtr drawable)
{
	if (drawable->type == DRAWABLE_PIXMAP)
		return (PixmapPtr)drawable;
	else
		return drawable->pScreen->GetWindowPixmap((WindowPtr)drawable);
}

#endif /* AMDGPU_BO_HELPER_H */

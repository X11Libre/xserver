/*
 * Copyright 2000 ATI Technologies Inc., Markham, Ontario, and
 *                VA Linux Systems Inc., Fremont, California.
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

/*
 * Authors:
 *   Kevin E. Martin <martin@xfree86.org>
 *
 * Modified by Marc Aurele La France <tsi@xfree86.org> for ATI driver merge.
 */

#ifndef _AMDGPU_PROBE_H_
#define _AMDGPU_PROBE_H_ 1

#include <stdint.h>
#include "xorg-server.h"
#include "xf86str.h"
#include "xf86DDC.h"
#include "randrstr.h"

#include "xf86Crtc.h"

#ifdef XSERVER_PLATFORM_BUS
#include "xf86platformBus.h"
#endif

#include <amdgpu.h>

#include "compat-api.h"

extern DriverRec AMDGPU;

typedef struct {
	Bool HasCRTC2;		/* All cards except original Radeon  */
	Bool has_page_flip_target;

	amdgpu_device_handle pDev;

	int fd;			/* for sharing across zaphod heads   */
	int fd_ref;
	unsigned long fd_wakeup_registered;	/* server generation for which fd has been registered for wakeup handling */
	int fd_wakeup_ref;
	unsigned int assigned_crtcs;
	unsigned int num_scrns;
	ScrnInfoPtr scrn[6];
	struct xf86_platform_device *platform_dev;
	char *render_node;
	char *busid;
} AMDGPUEntRec, *AMDGPUEntPtr;

extern void amdgpu_kernel_close_fd(AMDGPUEntPtr pAMDGPUEnt);

extern const OptionInfoRec *AMDGPUOptionsWeak(void);

extern Bool AMDGPUPreInit_KMS(ScrnInfoPtr, int);
extern Bool AMDGPUScreenInit_KMS(ScreenPtr pScreen, int argc, char **argv);
extern Bool AMDGPUSwitchMode_KMS(ScrnInfoPtr pScrn, DisplayModePtr mode);
extern void AMDGPUAdjustFrame_KMS(ScrnInfoPtr pScrn, int x, int y);
extern Bool AMDGPUEnterVT_KMS(ScrnInfoPtr pScrn);
extern void AMDGPULeaveVT_KMS(ScrnInfoPtr pScrn);
extern void AMDGPUFreeScreen_KMS(ScrnInfoPtr pScrn);

extern ModeStatus AMDGPUValidMode(ScrnInfoPtr pScrn, DisplayModePtr mode,
				  Bool verbose, int flag);
#endif /* _AMDGPU_PROBE_H_ */

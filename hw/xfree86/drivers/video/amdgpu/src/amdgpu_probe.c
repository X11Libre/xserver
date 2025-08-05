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
#include "config.h"
#include <xorg-server.h>

#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>

/*
 * Authors:
 *   Kevin E. Martin <martin@xfree86.org>
 *   Rickard E. Faith <faith@valinux.com>
 * KMS support - Dave Airlie <airlied@redhat.com>
 */

#include "amdgpu_probe.h"
#include "amdgpu_version.h"
#include "amdgpu_drv.h"

#include "xf86.h"

#include "xf86drmMode.h"

#if defined(__FreeBSD__) || defined(__FreeBSD_kernel__)
#include <xf86_OSproc.h>
#endif

#include <xf86platformBus.h>

int gAMDGPUEntityIndex = -1;

/* Return the options for supported chipset 'n'; NULL otherwise */
static const OptionInfoRec *AMDGPUAvailableOptions(int chipid, int busid)
{
	return AMDGPUOptionsWeak();
}

static SymTabRec AMDGPUAny[] = {
	{ 0, "All GPUs supported by the amdgpu kernel driver" },
	{ -1, NULL }
};

/* Return the string name for supported chipset 'n'; NULL otherwise. */
static void AMDGPUIdentify(int flags)
{
	xf86PrintChipsets(AMDGPU_NAME, "Driver for AMD Radeon", AMDGPUAny);
}

static Bool amdgpu_device_matches(const drmDevicePtr device,
				  const struct pci_device *dev)
{
	return (device->bustype == DRM_BUS_PCI &&
		device->businfo.pci->domain == dev->domain &&
		device->businfo.pci->bus == dev->bus &&
		device->businfo.pci->dev == dev->dev &&
		device->businfo.pci->func == dev->func);
}

static Bool amdgpu_kernel_mode_enabled(ScrnInfoPtr pScrn)
{
#if defined(__FreeBSD__) || defined(__FreeBSD_kernel__)
	AMDGPUEntPtr pAMDGPUEnt = AMDGPUEntPriv(pScrn);
	const char *busIdString = pAMDGPUEnt->busid;
	int ret = drmCheckModesettingSupported(busIdString);

	if (ret) {
		if (xf86LoadKernelModule("amdgpukms"))
			ret = drmCheckModesettingSupported(busIdString);
	}
	if (ret) {
		xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, 0,
			       "[KMS] drm report modesetting isn't supported.\n");
		return FALSE;
	}

#endif
	xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, 0,
		       "[KMS] Kernel modesetting enabled.\n");
	return TRUE;
}

static int amdgpu_kernel_open_fd(ScrnInfoPtr pScrn,
				 struct pci_device *pci_dev,
				 struct xf86_platform_device *platform_dev,
				 AMDGPUEntPtr pAMDGPUEnt)
{
#define MAX_DRM_DEVICES 64
	drmDevicePtr devices[MAX_DRM_DEVICES];
	struct pci_device *dev;
	const char *path;
	int fd = -1, i, ret;

	if (platform_dev)
		dev = platform_dev->pdev;
	else
		dev = pci_dev;

	XNFasprintf(&pAMDGPUEnt->busid, "pci:%04x:%02x:%02x.%u",
		    dev->domain, dev->bus, dev->dev, dev->func);

	if (platform_dev) {
		fd = xf86_get_platform_device_int_attrib(platform_dev,
							 ODEV_ATTRIB_FD, -1);
		if (fd != -1)
			return fd;

		path = xf86_get_platform_device_attrib(platform_dev,
						       ODEV_ATTRIB_PATH);

		fd = open(path, O_RDWR | O_CLOEXEC);
		if (fd != -1)
			return fd;
	}

	if (!amdgpu_kernel_mode_enabled(pScrn))
		return -1;

	ret = drmGetDevices2(0, devices, ARRAY_SIZE(devices));
	if (ret == -1) {
		xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
			   "[drm] Failed to retrieve DRM devices information.\n");
		return -1;
	}
	for (i = 0; i < ret; i++) {
		if (amdgpu_device_matches(devices[i], dev) &&
		    devices[i]->available_nodes & (1 << DRM_NODE_PRIMARY)) {
			path = devices[i]->nodes[DRM_NODE_PRIMARY];
			fd = open(path, O_RDWR | O_CLOEXEC);
			break;
		}
	}
	drmFreeDevices(devices, ret);

	if (fd == -1)
		xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
			   "[drm] Failed to open DRM device for %s: %s\n",
			   pAMDGPUEnt->busid, strerror(errno));
	return fd;
#undef MAX_DRM_DEVICES
}

void amdgpu_kernel_close_fd(AMDGPUEntPtr pAMDGPUEnt)
{
#if defined(XSERVER_PLATFORM_BUS)
	if (!(pAMDGPUEnt->platform_dev &&
	      pAMDGPUEnt->platform_dev->flags & XF86_PDEV_SERVER_FD))
#endif
		close(pAMDGPUEnt->fd);
	pAMDGPUEnt->fd = -1;
}

/* Pull a local version of the helper. It's available since 2.4.98 yet
 * it may be too new for some distributions.
 */
static int local_drmIsMaster(int fd)
{
	return drmAuthMagic(fd, 0) != -EACCES;
}

static Bool amdgpu_open_drm_master(ScrnInfoPtr pScrn,
				   struct pci_device *pci_dev,
				   struct xf86_platform_device *platform_dev,
				   AMDGPUEntPtr pAMDGPUEnt)
{
	pAMDGPUEnt->fd = amdgpu_kernel_open_fd(pScrn, pci_dev, platform_dev, pAMDGPUEnt);
	if (pAMDGPUEnt->fd == -1)
		return FALSE;

	/* Check that what we opened is a master or a master-capable FD */
	if (!local_drmIsMaster(pAMDGPUEnt->fd)) {
		xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
			   "[drm] device is not DRM master.\n");
		amdgpu_kernel_close_fd(pAMDGPUEnt);
		return FALSE;
	}

	return TRUE;
}

static Bool amdgpu_device_setup(ScrnInfoPtr pScrn,
				struct pci_device *pci_dev,
				struct xf86_platform_device *platform_dev,
				AMDGPUEntPtr pAMDGPUEnt)
{
	uint32_t major_version;
	uint32_t minor_version;

	pAMDGPUEnt->platform_dev = platform_dev;
	if (!amdgpu_open_drm_master(pScrn, pci_dev, platform_dev,
				    pAMDGPUEnt))
		return FALSE;

	if (amdgpu_device_initialize(pAMDGPUEnt->fd,
				     &major_version,
				     &minor_version,
				     &pAMDGPUEnt->pDev)) {
		xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
			   "amdgpu_device_initialize failed\n");
		goto error_amdgpu;
	}

	return TRUE;

error_amdgpu:
	amdgpu_kernel_close_fd(pAMDGPUEnt);
	return FALSE;
}

static Bool
amdgpu_probe(ScrnInfoPtr pScrn, int entity_num,
	     struct pci_device *pci_dev, struct xf86_platform_device *dev)
{
	EntityInfoPtr pEnt = NULL;
	DevUnion *pPriv;
	AMDGPUEntPtr pAMDGPUEnt;

	if (!pScrn)
		return FALSE;

	pScrn->driverVersion = AMDGPU_VERSION_CURRENT;
	pScrn->driverName = AMDGPU_DRIVER_NAME;
	pScrn->name = AMDGPU_NAME;
	pScrn->Probe = NULL;
	pScrn->PreInit = AMDGPUPreInit_KMS;
	pScrn->ScreenInit = AMDGPUScreenInit_KMS;
	pScrn->SwitchMode = AMDGPUSwitchMode_KMS;
	pScrn->AdjustFrame = AMDGPUAdjustFrame_KMS;
	pScrn->EnterVT = AMDGPUEnterVT_KMS;
	pScrn->LeaveVT = AMDGPULeaveVT_KMS;
	pScrn->FreeScreen = AMDGPUFreeScreen_KMS;
	pScrn->ValidMode = AMDGPUValidMode;

	pEnt = xf86GetEntityInfo(entity_num);

	/* Create a AMDGPUEntity for all chips, even with old single head
	 * Radeon, need to use pAMDGPUEnt for new monitor detection routines.
	 */
	xf86SetEntitySharable(entity_num);

	if (gAMDGPUEntityIndex == -1)
		gAMDGPUEntityIndex = xf86AllocateEntityPrivateIndex();

	pPriv = xf86GetEntityPrivate(pEnt->index, gAMDGPUEntityIndex);

	if (!pPriv->ptr) {
		pPriv->ptr = XNFcallocarray(sizeof(AMDGPUEntRec), 1);
		if (!pPriv->ptr)
			goto error;

		pAMDGPUEnt = pPriv->ptr;
		if (!amdgpu_device_setup(pScrn, pci_dev, dev, pAMDGPUEnt))
			goto error;

		pAMDGPUEnt->fd_ref = 1;

	} else {
		pAMDGPUEnt = pPriv->ptr;

		if (pAMDGPUEnt->fd_ref == ARRAY_SIZE(pAMDGPUEnt->scrn)) {
			xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
				   "Only up to %u Zaphod instances supported\n",
				   (unsigned)ARRAY_SIZE(pAMDGPUEnt->scrn));
			goto error;
		}

		pAMDGPUEnt->fd_ref++;
	}

	xf86SetEntityInstanceForScreen(pScrn, pEnt->index,
				       xf86GetNumEntityInstances(pEnt->
								 index)
				       - 1);
	free(pEnt);

	return TRUE;

error:
	free(pEnt);
	return FALSE;
}

static Bool
amdgpu_pci_probe(DriverPtr pDriver,
		 int entity_num, struct pci_device *device, intptr_t match_data)
{
	ScrnInfoPtr pScrn = xf86ConfigPciEntity(NULL, 0, entity_num, NULL,
						NULL, NULL, NULL, NULL, NULL);

	return amdgpu_probe(pScrn, entity_num, device, NULL);
}

static Bool AMDGPUDriverFunc(ScrnInfoPtr scrn, xorgDriverFuncOp op, void *data)
{
	xorgHWFlags *flag;

	switch (op) {
	case GET_REQUIRED_HW_INTERFACES:
		flag = (CARD32 *) data;
		(*flag) = 0;
		return TRUE;
	case SUPPORTS_SERVER_FDS:
		return TRUE;
       default:
		return FALSE;
	}
}

#ifdef XSERVER_PLATFORM_BUS
static Bool
amdgpu_platform_probe(DriverPtr pDriver,
		      int entity_num, int flags,
		      struct xf86_platform_device *dev, intptr_t match_data)
{
	ScrnInfoPtr pScrn;
	int scr_flags = 0;

	if (!dev->pdev)
		return FALSE;

	if (flags & PLATFORM_PROBE_GPU_SCREEN)
		scr_flags = XF86_ALLOCATE_GPU_SCREEN;

	pScrn = xf86AllocateScreen(pDriver, scr_flags);
	if (xf86IsEntitySharable(entity_num))
		xf86SetEntityShared(entity_num);
	xf86AddEntityToScreen(pScrn, entity_num);

	return amdgpu_probe(pScrn, entity_num, NULL, dev);
}
#endif

static const struct pci_id_match amdgpu_device_match[] = {
    {0x1002, PCI_MATCH_ANY, PCI_MATCH_ANY, PCI_MATCH_ANY, 0, 0, 0},
    {0, 0, 0},
};

DriverRec AMDGPU = {
	AMDGPU_VERSION_CURRENT,
	AMDGPU_DRIVER_NAME,
	AMDGPUIdentify,
	NULL,
	AMDGPUAvailableOptions,
	NULL,
	0,
	AMDGPUDriverFunc,
	amdgpu_device_match,
	amdgpu_pci_probe,
#ifdef XSERVER_PLATFORM_BUS
	amdgpu_platform_probe
#endif
};

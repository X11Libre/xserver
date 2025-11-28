/*
 * Copyright © 2012 Red Hat.
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
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Author: Dave Airlie <airlied@redhat.com>
 */

/*
 * This file contains the interfaces to the bus-specific code
 */

#ifdef HAVE_XORG_CONFIG_H
#include <xorg-config.h>
#endif

#ifdef XSERVER_PLATFORM_BUS
#include <errno.h>

#include <pciaccess.h>
#include <fcntl.h>
#include <unistd.h>

#include "config/hotplug_priv.h"
#include "dix/pixmap_priv.h"
#include "dix/screenint_priv.h"
#include "randr/randrstr_priv.h"
#include "os/osdep.h"

#include "os.h"
#include "../os-support/linux/systemd-logind.h"

#include "loaderProcs.h"
#include "xf86_priv.h"
#include "xf86_os_support.h"
#include "xf86_OSproc.h"
#include "xf86Opt_priv.h"
#include "xf86Priv.h"
#include "xf86str.h"
#include "xf86Bus.h"
#include "Pci.h"
#include "xf86platformBus_priv.h"
#include "xf86Xinput_priv.h"
#include "xf86Config.h"
#include "xf86Crtc.h"

int xf86_num_platform_devices;

struct xf86_platform_device *xf86_platform_devices;

int
xf86_add_platform_device(struct OdevAttributes *attribs, Bool unowned)
{
    xf86_platform_devices = XNFreallocarray(xf86_platform_devices,
                                            xf86_num_platform_devices + 1,
                                            sizeof(struct xf86_platform_device));

    xf86_platform_devices[xf86_num_platform_devices].attribs = attribs;
    xf86_platform_devices[xf86_num_platform_devices].pdev = NULL;
    xf86_platform_devices[xf86_num_platform_devices].flags =
        unowned ? XF86_PDEV_UNOWNED : 0;

    xf86_num_platform_devices++;
    return 0;
}

int
xf86_remove_platform_device(int dev_index)
{
    int j;

    config_odev_free_attributes(xf86_platform_devices[dev_index].attribs);

    for (j = dev_index; j < xf86_num_platform_devices - 1; j++)
        memcpy(&xf86_platform_devices[j], &xf86_platform_devices[j + 1], sizeof(struct xf86_platform_device));
    xf86_num_platform_devices--;
    return 0;
}

Bool
xf86_get_platform_device_unowned(int index)
{
    return (xf86_platform_devices[index].flags & XF86_PDEV_UNOWNED) ?
        TRUE : FALSE;
}

struct xf86_platform_device *
xf86_find_platform_device_by_devnum(unsigned int major, unsigned int minor)
{
    for (unsigned int i = 0; i < xf86_num_platform_devices; i++) {
        unsigned int attr_major = xf86_platform_odev_attributes(i)->major;
        unsigned int attr_minor = xf86_platform_odev_attributes(i)->minor;
        if (attr_major == major && attr_minor == minor)
            return &xf86_platform_devices[i];
    }
    return NULL;
}

/*
 * xf86IsPrimaryPlatform() -- return TRUE if primary device
 * is a platform device and it matches this one.
 */

static Bool
xf86IsPrimaryPlatform(struct xf86_platform_device *plat)
{
    /* Add max. 1 screen for the IgnorePrimary fallback path */
    if (xf86ProbeIgnorePrimary && xf86NumScreens == 0)
        return TRUE;

    if (primaryBus.type == BUS_PLATFORM)
        return plat == primaryBus.id.plat;
#ifdef XSERVER_LIBPCIACCESS
    if (primaryBus.type == BUS_PCI)
        if (plat->pdev)
            if (MATCH_PCI_DEVICES(primaryBus.id.pci, plat->pdev))
                return TRUE;
#endif
    return FALSE;
}

static void
platform_find_pci_info(struct xf86_platform_device *pd, char *busid)
{
    struct pci_slot_match devmatch;
    struct pci_device *info;
    struct pci_device_iterator *iter;
    int ret;

    ret = sscanf(busid, "pci:%04x:%02x:%02x.%u",
                 &devmatch.domain, &devmatch.bus, &devmatch.dev,
                 &devmatch.func);
    if (ret != 4)
        return;

    iter = pci_slot_match_iterator_create(&devmatch);
    info = pci_device_next(iter);
    if (info)
        pd->pdev = info;
    pci_iterator_destroy(iter);
}

static Bool
xf86_check_platform_slot(const struct xf86_platform_device *pd)
{
    int i;

    for (i = 0; i < xf86NumEntities; i++) {
        const EntityPtr u = xf86Entities[i];

        if (pd->pdev && u->bus.type == BUS_PCI &&
            MATCH_PCI_DEVICES(pd->pdev, u->bus.id.pci)) {
            return FALSE;
        }
        if ((u->bus.type == BUS_PLATFORM) && (pd == u->bus.id.plat)) {
            return FALSE;
        }
    }
    return TRUE;
}

static Bool
OutputClassMatches(const XF86ConfOutputClassPtr oclass,
                   struct xf86_platform_device *dev)
{
    char *driver = dev->attribs->driver;
    const char *layout;

    if (!MatchAttrToken(driver, &oclass->match_driver))
        return FALSE;

    /* MatchLayout string
     *
     * If no Layout section is found, xf86ServerLayout.id becomes "(implicit)"
     * It is convenient that "" in patterns means "no explicit layout"
     */
    if (strcmp(xf86ConfigLayout.id,"(implicit)"))
        layout = xf86ConfigLayout.id;
    else
        layout = "";
    if (!MatchAttrToken(layout, &oclass->match_layout))
            return FALSE;

    return TRUE;
}

static void
xf86OutputClassDriverList(int index, XF86MatchedDrivers *md)
{
    XF86ConfOutputClassPtr cl;

    for (cl = xf86configptr->conf_outputclass_lst; cl; cl = cl->list.next) {
        if (OutputClassMatches(cl, &xf86_platform_devices[index])) {
            char *path = xf86_platform_odev_attributes(index)->path;

            LogMessageVerb(X_INFO, 1, "Applying OutputClass \"%s\" to %s\n",
                           cl->identifier, path);
            if (cl->driver != NULL && *(cl->driver)) {
                LogMessageVerb(X_NONE, 1, "\tloading driver: %s\n", cl->driver);
                xf86AddMatchedDriver(md, cl->driver);
            } else
                LogMessageVerb(X_NONE, 1, "\tno driver specified\n");
        }
    }
}

/**
 *  @return The numbers of found devices that match with the current system
 *  drivers.
 */
void
xf86PlatformMatchDriver(XF86MatchedDrivers *md)
{
    int i;
    struct pci_device *info = NULL;
    int pass = 0;

    for (pass = 0; pass < 2; pass++) {
        for (i = 0; i < xf86_num_platform_devices; i++) {

            if (xf86IsPrimaryPlatform(&xf86_platform_devices[i]) && (pass == 1))
                continue;
            else if (!xf86IsPrimaryPlatform(&xf86_platform_devices[i]) && (pass == 0))
                continue;

            xf86OutputClassDriverList(i, md);

            info = xf86_platform_devices[i].pdev;
#ifdef __linux__
            if (info)
                xf86MatchDriverFromFiles(info->vendor_id, info->device_id, md);
#endif

            if (info != NULL) {
                xf86VideoPtrToDriverList(info, md);
            }
        }
    }
}

void xf86PlatformScanPciDev(void)
{
    int i;

    if (!xf86scanpci())
        return;

    LogMessageVerb(X_CONFIG, 1, "Scanning the platform PCI devices\n");
    for (i = 0; i < xf86_num_platform_devices; i++) {
        char *busid = xf86_platform_odev_attributes(i)->busid;

        if (strncmp(busid, "pci:", 4) == 0)
            platform_find_pci_info(&xf86_platform_devices[i], busid);
    }
}

int
xf86platformProbe(void)
{
    int i;
    Bool pci = TRUE;
    XF86ConfOutputClassPtr cl, cl_head = (xf86configptr) ?
            xf86configptr->conf_outputclass_lst : NULL;
    char *driver_path, *path = NULL;
    char *curr, *next, *copy;

    config_odev_probe(xf86PlatformDeviceProbe);

    if (!xf86scanpci()) {
        pci = FALSE;
    }

    for (i = 0; i < xf86_num_platform_devices; i++) {
        char *busid = xf86_platform_odev_attributes(i)->busid;

        if (pci && busid && (strncmp(busid, "pci:", 4) == 0)) {
            platform_find_pci_info(&xf86_platform_devices[i], busid);
        }

        /*
         * Deal with OutputClass ModulePath directives, these must be
         * processed before we do any module loading.
         */
        for (cl = cl_head; cl; cl = cl->list.next) {
            if (!OutputClassMatches(cl, &xf86_platform_devices[i]))
                continue;

            if (xf86ModPathFrom != X_CMDLINE) {
                if (cl->driver) {
                    if (cl->modulepath) {
                        if (*(cl->modulepath)) {
                            XNFasprintf(&driver_path, "%s,%s", cl->modulepath, xf86ModulePath);
                            LogMessageVerb(X_CONFIG, 1, "OutputClass \"%s\" ModulePath for driver %s overridden with \"%s\"\n",
                                    cl->identifier, cl->driver, driver_path);
                        } else {
                            XNFasprintf(&driver_path, "%s", xf86ModulePath);
                            LogMessageVerb(X_CONFIG, 1, "OutputClass \"%s\" ModulePath for driver %s reset to standard \"%s\"\n",
                                    cl->identifier, cl->driver, driver_path);
                        }
                    } else {
                        driver_path = NULL;
                        LogMessageVerb(X_CONFIG, 1, "OutputClass \"%s\" ModulePath for driver %s reset to default\n",
                                cl->identifier, cl->driver);
                    }
                    if (*(cl->driver)) LoaderSetPath(cl->driver, driver_path);
                    if (cl->modules) {
                        LogMessageVerb(X_CONFIG, 1, "    and for modules \"%s\" as well\n",
                                cl->modules);
                        XNFasprintf(&copy, "%s", cl->modules);
                        curr = copy;
                        while ((curr = strtok_r(curr, ",", &next))) {
                            if (*curr) LoaderSetPath(curr, driver_path);
                            curr = NULL;
                        }
                        free(copy);
                    }
                    free(driver_path);
                }
                else if (cl->modules) {
                    if (cl->modulepath) {
                        if (*(cl->modulepath)) {
                            XNFasprintf(&driver_path, "%s,%s", cl->modulepath, xf86ModulePath);
                            LogMessageVerb(X_CONFIG, 1, "OutputClass \"%s\" ModulePath for modules %s overridden with \"%s\"\n",
                                    cl->identifier, cl->modules, driver_path);
                        } else {
                            XNFasprintf(&driver_path, "%s", xf86ModulePath);
                            LogMessageVerb(X_CONFIG, 1, "OutputClass \"%s\" ModulePath for modules %s reset to standard \"%s\"\n",
                                    cl->identifier, cl->modules, driver_path);
                        }
                    } else {
                        driver_path = NULL;
                        LogMessageVerb(X_CONFIG, 1, "OutputClass \"%s\" ModulePath for modules %s reset to default\n",
                                cl->identifier, cl->modules);
                    }
                    XNFasprintf(&copy, "%s", cl->modules);
                    curr = copy;
                    while ((curr = strtok_r(curr, ",", &next))) {
                        if (*curr) LoaderSetPath(curr, driver_path);
                        curr = NULL;
                    }
                    free(copy);
                } else {
                        driver_path = path; /* Reuse for temporary storage */
                        if (*(cl->modulepath)) {
                            XNFasprintf(&path, "%s,%s", cl->modulepath,
                                    path ? path : xf86ModulePath);
                            LogMessageVerb(X_CONFIG, 1, "OutputClass \"%s\" default ModulePath extended to \"%s\"\n",
                                    cl->identifier, path);
                        } else {
                            XNFasprintf(&path, "%s", xf86ModulePath);
                            LogMessageVerb(X_CONFIG, 1, "OutputClass \"%s\" default ModulePath reset to standard \"%s\"\n",
                                    cl->identifier, path);
                        }
                }
                /* Otherwise global module search path is left unchanged */
            }
        }
    }

    if (xf86ModPathFrom != X_CMDLINE) {
        if (path) {
            LoaderSetPath(NULL, path);
            free(path);
        } else
            LoaderSetPath(NULL, xf86ModulePath);
    }

    /* First see if there is an OutputClass match marking a device as primary */
    for (i = 0; i < xf86_num_platform_devices; i++) {
        struct xf86_platform_device *dev = &xf86_platform_devices[i];
        for (cl = cl_head; cl; cl = cl->list.next) {
            if (!OutputClassMatches(cl, dev))
                continue;

            if (xf86CheckBoolOption(cl->option_lst, "PrimaryGPU", FALSE)) {
                LogMessageVerb(X_CONFIG, 1, "OutputClass \"%s\" setting %s as PrimaryGPU\n",
                               cl->identifier, dev->attribs->path);
                primaryBus.type = BUS_PLATFORM;
                primaryBus.id.plat = dev;
                return 0;
            }
        }
    }

    /* Then check for pci_device_is_boot_vga() */
    for (i = 0; i < xf86_num_platform_devices; i++) {
        struct xf86_platform_device *dev = &xf86_platform_devices[i];

        if (!dev->pdev)
            continue;

        pci_device_probe(dev->pdev);
        if (pci_device_is_boot_vga(dev->pdev)) {
            primaryBus.type = BUS_PLATFORM;
            primaryBus.id.plat = dev;
        }
    }

    return 0;
}

void
xf86MergeOutputClassOptions(int entityIndex, void **options)
{
    const EntityPtr entity = xf86Entities[entityIndex];
    struct xf86_platform_device *dev = NULL;
    XF86ConfOutputClassPtr cl;
    XF86OptionPtr classopts;
    int i = 0;

    switch (entity->bus.type) {
    case BUS_PLATFORM:
        dev = entity->bus.id.plat;
        break;
    case BUS_PCI:
        for (i = 0; i < xf86_num_platform_devices; i++) {
            if (xf86_platform_devices[i].pdev) {
                if (MATCH_PCI_DEVICES(xf86_platform_devices[i].pdev,
                                      entity->bus.id.pci)) {
                    dev = &xf86_platform_devices[i];
                    break;
                }
            }
        }
        break;
    default:
        LogMessageVerb(X_DEBUG, 1, "xf86MergeOutputClassOptions unsupported bus type %d\n",
                       entity->bus.type);
    }

    if (!dev)
        return;

    for (cl = xf86configptr->conf_outputclass_lst; cl; cl = cl->list.next) {
        if (!OutputClassMatches(cl, dev) || !cl->option_lst)
            continue;

        LogMessageVerb(X_INFO, 1, "Applying OutputClass \"%s\" options to %s\n",
                       cl->identifier, dev->attribs->path);

        classopts = xf86optionListDup(cl->option_lst);
        *options = xf86optionListMerge(*options, classopts);
    }
}

static int
xf86ClaimPlatformSlot(struct xf86_platform_device * d, DriverPtr drvp,
                  int chipset, GDevPtr dev, Bool active)
{
    EntityPtr p = NULL;
    int num;

    if (xf86_check_platform_slot(d)) {
        num = xf86AllocateEntity();
        p = xf86Entities[num];
        p->driver = drvp;
        p->chipset = chipset;
        p->bus.type = BUS_PLATFORM;
        p->bus.id.plat = d;
        p->active = active;
        p->inUse = FALSE;
        if (dev)
            xf86AddDevToEntity(num, dev);

        return num;
    }
    else
        return -1;
}

static int
xf86UnclaimPlatformSlot(struct xf86_platform_device *d, GDevPtr dev)
{
    int i;

    for (i = 0; i < xf86NumEntities; i++) {
        const EntityPtr p = xf86Entities[i];

        if ((p->bus.type == BUS_PLATFORM) && (p->bus.id.plat == d)) {
            if (dev)
                xf86RemoveDevFromEntity(i, dev);
            p->bus.type = BUS_NONE;
            return 0;
        }
    }
    return 0;
}


#define END_OF_MATCHES(m)                                               \
    (((m).vendor_id == 0) && ((m).device_id == 0) && ((m).subvendor_id == 0))

static Bool doPlatformProbe(struct xf86_platform_device *dev, DriverPtr drvp,
                            GDevPtr gdev, int flags, intptr_t match_data)
{
    Bool foundScreen = FALSE;
    int entity;

    if (gdev && gdev->screen == 0 && !xf86_check_platform_slot(dev))
        return FALSE;

    entity = xf86ClaimPlatformSlot(dev, drvp, 0,
                                   gdev, gdev ? gdev->active : 0);

    if ((entity == -1) && gdev && (gdev->screen > 0)) {
        unsigned nent;

        for (nent = 0; nent < xf86NumEntities; nent++) {
            EntityPtr pEnt = xf86Entities[nent];

            if (pEnt->bus.type != BUS_PLATFORM)
                continue;
            if (pEnt->bus.id.plat == dev) {
                entity = nent;
                xf86AddDevToEntity(nent, gdev);
                break;
            }
        }
    }
    if (entity != -1) {
        if ((dev->flags & XF86_PDEV_SERVER_FD) && (!drvp->driverFunc ||
                !drvp->driverFunc(NULL, SUPPORTS_SERVER_FDS, NULL))) {
            systemd_logind_release_fd(dev->attribs->major, dev->attribs->minor, dev->attribs->fd);
            dev->attribs->fd = -1;
            dev->flags &= ~XF86_PDEV_SERVER_FD;
        }

        if (drvp->platformProbe(drvp, entity, flags, dev, match_data))
            foundScreen = TRUE;
        else
            xf86UnclaimPlatformSlot(dev, gdev);
    }
    return foundScreen;
}

static Bool
probeSingleDevice(struct xf86_platform_device *dev, DriverPtr drvp, GDevPtr gdev, int flags)
{
    int k;
    Bool foundScreen = FALSE;
    struct pci_device *pPci;
    const struct pci_id_match *const devices = drvp->supported_devices;

    if (dev->pdev && devices) {
        int device_id = dev->pdev->device_id;
        pPci = dev->pdev;
        for (k = 0; !END_OF_MATCHES(devices[k]); k++) {
            if (PCI_ID_COMPARE(devices[k].vendor_id, pPci->vendor_id)
                && PCI_ID_COMPARE(devices[k].device_id, device_id)
                && ((devices[k].device_class_mask & pPci->device_class)
                    ==  devices[k].device_class)) {
                foundScreen = doPlatformProbe(dev, drvp, gdev, flags, devices[k].match_data);
                if (foundScreen)
                    break;
            }
        }
    }
    else if (dev->pdev && !devices)
        return FALSE;
    else
        foundScreen = doPlatformProbe(dev, drvp, gdev, flags, 0);
    return foundScreen;
}

static Bool
isGPUDevice(GDevPtr gdev)
{
    int i;

    for (i = 0; i < gdev->myScreenSection->num_gpu_devices; i++) {
        if (gdev == gdev->myScreenSection->gpu_devices[i])
            return TRUE;
    }

    return FALSE;
}

int
xf86platformProbeDev(DriverPtr drvp)
{
    Bool foundScreen = FALSE;
    GDevPtr *devList;
    const unsigned numDevs = xf86MatchDevice(drvp->driverName, &devList);
    int i, j;

    /* find the main device or any device specified in xorg.conf */
    for (i = 0; i < numDevs; i++) {
        const char *devpath;

        /* skip inactive devices */
        if (!devList[i]->active)
            continue;

        /* This is specific to modesetting. */
        devpath = xf86FindOptionValue(devList[i]->options, "kmsdev");

        for (j = 0; j < xf86_num_platform_devices; j++) {
            if (devpath && *devpath) {
                if (strcmp(xf86_platform_devices[j].attribs->path, devpath) == 0)
                    break;
            } else if (devList[i]->busID && *devList[i]->busID) {
                if (xf86PlatformDeviceCheckBusID(&xf86_platform_devices[j], devList[i]->busID))
                    break;
            }
            else {
                /* for non-seat0 servers assume first device is the master */
                if (ServerIsNotSeat0()) {
                    break;
                } else {
                    /* Accept the device if the driver is hyperv_drm */
                    if (strcmp(xf86_platform_devices[j].attribs->driver, "hyperv_drm") == 0)
                        break;
                    /* Accept the device if the driver is ofdrm */
                    if (strcmp(xf86_platform_devices[j].attribs->driver, "ofdrm") == 0)
                        break;
                    /* Accept the device if the driver is simpledrm */
                    if (strcmp(xf86_platform_devices[j].attribs->driver, "simpledrm") == 0)
                        break;
                }

                if (xf86IsPrimaryPlatform(&xf86_platform_devices[j]))
                    break;
            }
        }

        if (j == xf86_num_platform_devices)
             continue;

        foundScreen = probeSingleDevice(&xf86_platform_devices[j], drvp, devList[i],
                                        isGPUDevice(devList[i]) ? PLATFORM_PROBE_GPU_SCREEN : 0);
    }

    free(devList);

    return foundScreen;
}

int
xf86platformAddGPUDevices(DriverPtr drvp)
{
    Bool foundScreen = FALSE;
    GDevPtr *devList;
    int j;

    if (!drvp->platformProbe)
        return FALSE;

    xf86MatchDevice(drvp->driverName, &devList);

    /* if autoaddgpu devices is enabled then go find any unclaimed platform
     * devices and add them as GPU screens */
    if (xf86Info.autoAddGPU) {
        for (j = 0; j < xf86_num_platform_devices; j++) {
            if (probeSingleDevice(&xf86_platform_devices[j], drvp,
                                  devList ?  devList[0] : NULL,
                                  PLATFORM_PROBE_GPU_SCREEN))
                foundScreen = TRUE;
        }
    }

    free(devList);

    return foundScreen;
}

const char *
xf86PlatformFindHotplugDriver(int dev_index)
{
    XF86ConfOutputClassPtr cl;
    const char *hp_driver = NULL;
    struct xf86_platform_device *dev = &xf86_platform_devices[dev_index];

    for (cl = xf86configptr->conf_outputclass_lst; cl; cl = cl->list.next) {
        if (!OutputClassMatches(cl, dev) || !cl->option_lst)
	    continue;

        hp_driver = xf86FindOptionValue(cl->option_lst, "HotplugDriver");
        if (hp_driver)
            xf86MarkOptionUsed(cl->option_lst);
    }

    /* Return the first driver from the match list */
    LogMessageVerb(X_INFO, 1, "matching hotplug-driver is %s\n",
                   hp_driver ? hp_driver : "none");
    return hp_driver;
}

int
xf86platformAddDevice(const char *driver_name, int index)
{
    int i, old_screens, scr_index, scrnum;
    DriverPtr drvp = NULL;
    screenLayoutPtr layout;

    if (!xf86Info.autoAddGPU)
        return -1;

    /* Load modesetting driver if no driver given, or driver open failed */
    if (!driver_name || !xf86LoadOneModule(driver_name, NULL)) {
        driver_name = "modesetting";
        xf86LoadOneModule(driver_name, NULL);
    }

    for (i = 0; i < xf86NumDrivers; i++) {
        if (!xf86DriverList[i])
            continue;

        if (!strcmp(xf86DriverList[i]->driverName, driver_name)) {
            drvp = xf86DriverList[i];
            break;
        }
    }

    if (!drvp) {
        ErrorF("can't find driver %s for hotplugged device\n", driver_name);
        return -1;
    }

    old_screens = xf86NumGPUScreens;
    doPlatformProbe(&xf86_platform_devices[index], drvp, NULL,
                    PLATFORM_PROBE_GPU_SCREEN, 0);
    if (old_screens == xf86NumGPUScreens)
        return -1;
    i = old_screens;

    for (layout = xf86ConfigLayout.screens; layout->screen != NULL;
         layout++) {
        xf86GPUScreens[i]->confScreen = layout->screen;
        break;
    }

    if (xf86GPUScreens[i]->PreInit &&
        xf86GPUScreens[i]->PreInit(xf86GPUScreens[i], 0))
        xf86GPUScreens[i]->configured = TRUE;

    if (!xf86GPUScreens[i]->configured) {
        ErrorF("hotplugged device %d didn't configure\n", i);
        xf86DeleteScreen(xf86GPUScreens[i]);
        return -1;
    }

   scr_index = AddGPUScreen(xf86GPUScreens[i]->ScreenInit, 0, NULL);
   if (scr_index == -1) {
       xf86DeleteScreen(xf86GPUScreens[i]);
       xf86UnclaimPlatformSlot(&xf86_platform_devices[index], NULL);
       xf86NumGPUScreens = old_screens;
       return -1;
   }
   dixSetPrivate(&xf86GPUScreens[i]->pScreen->devPrivates,
                 xf86ScreenKey, xf86GPUScreens[i]);

   PixmapScreenInit(xf86GPUScreens[i]->pScreen);

   if (dixScreenRaiseCreateResources(xf86GPUScreens[i]->pScreen)) {
       RemoveGPUScreen(xf86GPUScreens[i]->pScreen);
       xf86DeleteScreen(xf86GPUScreens[i]);
       xf86UnclaimPlatformSlot(&xf86_platform_devices[index], NULL);
       xf86NumGPUScreens = old_screens;
       return -1;
   }
   /* attach unbound to the configured protocol screen (or 0) */
   scrnum = xf86GPUScreens[i]->confScreen->screennum;
   AttachUnboundGPU(xf86Screens[scrnum]->pScreen, xf86GPUScreens[i]->pScreen);
   if (xf86Info.autoBindGPU)
       RRProviderAutoConfigGpuScreen(xf86ScrnToScreen(xf86GPUScreens[i]),
                                     xf86ScrnToScreen(xf86Screens[scrnum]));

   RRResourcesChanged(xf86Screens[scrnum]->pScreen);
   RRTellChanged(xf86Screens[scrnum]->pScreen);

   return 0;
}

void
xf86platformRemoveDevice(int index)
{
    EntityPtr entity;
    int ent_num, i, j, scrnum;
    Bool found;

    for (ent_num = 0; ent_num < xf86NumEntities; ent_num++) {
        entity = xf86Entities[ent_num];
        if (entity->bus.type == BUS_PLATFORM &&
            entity->bus.id.plat == &xf86_platform_devices[index])
            break;
    }
    if (ent_num == xf86NumEntities)
        goto out;

    found = FALSE;
    for (i = 0; i < xf86NumGPUScreens; i++) {
        for (j = 0; j < xf86GPUScreens[i]->numEntities; j++)
            if (xf86GPUScreens[i]->entityList[j] == ent_num) {
                found = TRUE;
                break;
            }
        if (found)
            break;
    }
    if (!found) {
        ErrorF("failed to find screen to remove\n");
        goto out;
    }

    scrnum = xf86GPUScreens[i]->confScreen->screennum;

    dixScreenRaiseClose(xf86GPUScreens[i]->pScreen);

    RemoveGPUScreen(xf86GPUScreens[i]->pScreen);
    xf86DeleteScreen(xf86GPUScreens[i]);

    xf86UnclaimPlatformSlot(&xf86_platform_devices[index], NULL);

    xf86_remove_platform_device(index);

    RRResourcesChanged(xf86Screens[scrnum]->pScreen);
    RRTellChanged(xf86Screens[scrnum]->pScreen);
 out:
    return;
}

/* called on return from VT switch to find any new devices */
void xf86platformVTProbe(void)
{
    int i;

    for (i = 0; i < xf86_num_platform_devices; i++) {
        if (!(xf86_platform_devices[i].flags & XF86_PDEV_UNOWNED))
            continue;

        xf86_platform_devices[i].flags &= ~XF86_PDEV_UNOWNED;
        xf86PlatformReprobeDevice(i, xf86_platform_devices[i].attribs);
    }
}

void xf86platformPrimary(void)
{
    /* use the first platform device as a fallback */
    if (primaryBus.type == BUS_NONE) {
        LogMessageVerb(X_INFO, 1, "no primary bus or device found\n");

        if (xf86_num_platform_devices > 0) {
            primaryBus.id.plat = &xf86_platform_devices[0];
            primaryBus.type = BUS_PLATFORM;

            LogMessageVerb(X_NONE, 1, "\tfalling back to %s\n", primaryBus.id.plat->attribs->syspath);
        }
    }
}

char *
_xf86_get_platform_device_attrib(struct xf86_platform_device *device, int attrib, int (*fake)[0])
{
    switch (attrib) {
    case ODEV_ATTRIB_PATH:
        return xf86_platform_device_odev_attributes(device)->path;
    case ODEV_ATTRIB_SYSPATH:
        return xf86_platform_device_odev_attributes(device)->syspath;
    case ODEV_ATTRIB_BUSID:
        return xf86_platform_device_odev_attributes(device)->busid;
    case ODEV_ATTRIB_DRIVER:
        return xf86_platform_device_odev_attributes(device)->driver;
    default:
        assert(FALSE);
        return NULL;
    }
}

int
_xf86_get_platform_device_int_attrib(struct xf86_platform_device *device, int attrib, int (*fake)[0])
{
    switch (attrib) {
    case ODEV_ATTRIB_FD:
        return xf86_platform_device_odev_attributes(device)->fd;
    case ODEV_ATTRIB_MAJOR:
        return xf86_platform_device_odev_attributes(device)->major;
    case ODEV_ATTRIB_MINOR:
        return xf86_platform_device_odev_attributes(device)->minor;
    default:
        assert(FALSE);
        return 0;
    }
}

#endif /* XSERVER_PLATFORM_BUS */


/*
 * Copyright 1999 by The XFree86 Project, Inc.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <xorg-server.h>
#include <X11/X.h>
#include "xf86.h"
#include "xf86Xinput.h"
#include "mouse.h"
#include "xf86_OSlib.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

static int
SupportedInterfaces(void)
{
    return MSE_SERIAL | MSE_BUS | MSE_PS2 | MSE_XPS2 | MSE_AUTO;
}

static const char *
DefaultProtocol(void)
{
    return "Auto";
}

#define DEFAULT_MOUSE_DEV               "/dev/input/mice"
#define DEFAULT_PS2_DEV                 "/dev/psaux"
#define DEFAULT_GPM_DATA_DEV            "/dev/gpmdata"
#define DEFAULT_GPM_CTL_DEV             "/dev/gpmdata"

static const char *mouseDevs[] = {
        DEFAULT_MOUSE_DEV,
        DEFAULT_PS2_DEV,
        DEFAULT_GPM_DATA_DEV,
        NULL
};

typedef enum {
        MOUSE_PROTO_UNKNOWN = 0,
        MOUSE_PROTO_SERIAL,
        MOUSE_PROTO_PS2,
        MOUSE_PROTO_MSC,
        MOUSE_PROTO_GPM,
        MOUSE_PROTO_EXPPS2,
} protocolTypes;

static struct {
        protocolTypes proto;
        const char *name;
} devproto[] = {
        { MOUSE_PROTO_UNKNOWN,  NULL },
        { MOUSE_PROTO_PS2,      "PS/2" },
        { MOUSE_PROTO_MSC,      "MouseSystems" },
        { MOUSE_PROTO_GPM,      "GPM" },
        { MOUSE_PROTO_EXPPS2,   "ExplorerPS/2" },
};

static const char *
FindDevice(InputInfoPtr pInfo, const char *protocol, int flags)
{
    int fd = -1;
    const char **pdev;

    for (pdev = mouseDevs; *pdev; pdev++) {
        SYSCALL (fd = open(*pdev, O_RDWR | O_NONBLOCK | O_EXCL));
        if (fd == -1) {
#ifdef DEBUG
            ErrorF("Cannot open %s (%s)\n", *pdev, strerror(errno));
#endif
        } else
            break;
    }

    if (*pdev) {
        close(fd);
        /* Set the Device option. */
        pInfo->options =
            xf86AddNewOption(pInfo->options, "Device", *pdev);
        xf86Msg(X_INFO, "%s: Setting Device option to \"%s\"\n",
                pInfo->name, *pdev);
    }

    return *pdev;
}

static const char *
lnxMouseMagic(InputInfoPtr pInfo)
{
    int fd = -1;
    const char *dev;
    char *realdev;
    struct stat sbuf;
    int i;
    int proto = MOUSE_PROTO_UNKNOWN;

    dev = xf86SetStrOption(pInfo->options, "Device", NULL);
    if (!dev) {
#ifdef DEBUG
        ErrorF("xf86SetStrOption failed to return the device name\n");
#endif
        return NULL;
    }
    /* Look at the device name to guess the protocol. */
    realdev = NULL;
    if (strcmp(dev, DEFAULT_MOUSE_DEV) == 0) {
        if (lstat(dev, &sbuf) != 0) {
#ifdef DEBUG
            ErrorF("lstat failed for %s (%s)\n", dev, strerror(errno));
#endif
            return NULL;
        }
        if (S_ISLNK(sbuf.st_mode)) {
            realdev = xnfalloc(PATH_MAX + 1);
            i = readlink(dev, realdev, PATH_MAX);
            if (i <= 0) {
#ifdef DEBUG
                ErrorF("readlink failed for %s (%s)\n", dev, strerror(errno));
#endif
                free(realdev);
                return NULL;
            }
            realdev[i] = '\0';
        }
    }
    if (!realdev)
        realdev = xnfstrdup(dev);
    else {
        /* If realdev doesn't contain a '/' then prepend "/dev/" */
        if (!strchr(realdev, '/')) {
            char *tmp = xnfalloc(strlen(realdev) + 5 + 1);
            sprintf(tmp, "/dev/%s", realdev);
            free(realdev);
            realdev = tmp;
        }
    }

    if (strcmp(realdev, DEFAULT_MOUSE_DEV) == 0)
        proto = MOUSE_PROTO_EXPPS2;
    else if (strcmp(realdev, DEFAULT_PS2_DEV) == 0)
        proto = MOUSE_PROTO_EXPPS2;
    else if (strcmp(realdev, DEFAULT_GPM_DATA_DEV) == 0)
        proto = MOUSE_PROTO_MSC;
    else if (strcmp(realdev, DEFAULT_GPM_CTL_DEV) == 0)
        proto = MOUSE_PROTO_GPM;
    free(realdev);
    /*
     * If the protocol can't be guessed from the device name,
     * try to characterise it.
     */
    if (proto == MOUSE_PROTO_UNKNOWN) {
        SYSCALL (fd = open(dev, O_RDWR | O_NONBLOCK | O_EXCL));
        if (isatty(fd)) {
            /* Serial PnP has already failed, so give up. */
        } else {
            if (fstat(fd, &sbuf) != 0) {
#ifdef DEBUG
                ErrorF("fstat failed for %s (%s)\n", dev, strerror(errno));
#endif
                close(fd);
                return NULL;
            }
            if (S_ISFIFO(sbuf.st_mode)) {
                /* Assume GPM data in MSC format. */
                proto = MOUSE_PROTO_MSC;
            } else {
                /* Default to PS/2 */
                proto = MOUSE_PROTO_PS2;
            }
        }
        close(fd);
    }
    if (proto == MOUSE_PROTO_UNKNOWN) {
        xf86Msg(X_ERROR, "%s: Cannot find mouse protocol.\n",
                pInfo->name);
        return NULL;
    } else {
        for (i = 0; i < sizeof(devproto)/sizeof(devproto[0]); i++) {
            if (devproto[i].proto == proto) {
                xf86Msg(X_INFO,
                        "%s: Setting mouse protocol to \"%s\"\n",
                        pInfo->name, devproto[i].name);
                return devproto[i].name;
            }
        }
    }
    return NULL;
}

static const char *
GuessProtocol(InputInfoPtr pInfo, int flags)
{
    return lnxMouseMagic(pInfo);
}

static const char *
SetupAuto(InputInfoPtr pInfo, int *protoPara)
{
    return lnxMouseMagic(pInfo);
}

OSMouseInfoPtr
OSMouseInit(int flags)
{
    OSMouseInfoPtr p;

    p = calloc(sizeof(OSMouseInfoRec), 1);
    if (!p)
        return NULL;
    p->SupportedInterfaces = SupportedInterfaces;
    p->DefaultProtocol = DefaultProtocol;
    p->FindDevice = FindDevice;
    p->GuessProtocol = GuessProtocol;
    p->SetupAuto = SetupAuto;
    return p;
}


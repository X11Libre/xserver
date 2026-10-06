/* SPDX-License-Identifier: MIT OR X11 OR AGPL-3.0-or-later
 *
 * Copyright © 2026 Enrico Weigelt, metux IT consult <info@metux.net>
 */

#include <xorg-config.h>

#include <mach.h>
#include <device/device.h>

#include <pthread.h>
#include <unistd.h>
#include <errno.h>

#include <exevents.h>
#include <xserver-properties.h>

#include <xorgVersion.h>
#include <xf86Xinput.h>

#define HURD_KBD_DEVICE  "kd"
#define SCANCODE_BUF_SIZE  64
#define HURD_KEYCODE_OFFSET 8
#define HURD_EXT_KEYCODE_OFFSET 128

typedef struct {
    mach_port_t kd_device;
    int wakeup_pipe[2];
    pthread_t reader_thread;
    volatile Bool running;
} HurdInputPriv;

static void
hurd_kbd_bell(int percent, DeviceIntPtr pDev, void *ctrl, int unused)
{
}

static void
hurd_kbd_keyctrl(DeviceIntPtr pDev, KeybdCtrl *ctrl)
{
}

static void
hurd_kbd_ptrctrl(DeviceIntPtr dev, PtrCtrl *ctrl)
{
}

static void *
hurd_reader_thread(void *arg)
{
    InputInfoPtr pInfo = arg;
    HurdInputPriv *priv = pInfo->private;
    kern_return_t err;
    char buf[SCANCODE_BUF_SIZE];
    int nread;
    int ret;

    while (priv->running) {
        nread = 0;
        err = device_read_inband(priv->kd_device, 0, 0,
                                 sizeof(buf), buf, &nread);
        if (err != KERN_SUCCESS)
            break;

        if (nread <= 0)
            continue;

        ret = write(priv->wakeup_pipe[1], buf, nread);
        if (ret < 0)
            break;
    }
    return NULL;
}

static void
hurd_handle_scancode(InputInfoPtr pInfo, unsigned char byte)
{
    static int extended = 0;
    int keycode;
    Bool is_down;

    if (byte == 0xE0) {
        extended = 1;
        return;
    }

    if (extended) {
        keycode = (byte & 0x7f) + HURD_EXT_KEYCODE_OFFSET;
        extended = 0;
    }
    else {
        keycode = (byte & 0x7f) + HURD_KEYCODE_OFFSET;
    }

    is_down = !(byte & 0x80);
    xf86PostKeyboardEvent(pInfo->dev, keycode, is_down);
}

static void
hurd_read_input(InputInfoPtr pInfo)
{
    HurdInputPriv *priv = pInfo->private;
    unsigned char buf[SCANCODE_BUF_SIZE];
    int nread;
    int i;

    nread = read(priv->wakeup_pipe[0], buf, sizeof(buf));
    if (nread <= 0)
        return;

    for (i = 0; i < nread; i++)
        hurd_handle_scancode(pInfo, buf[i]);
}

static int
hurd_device_init(DeviceIntPtr dev)
{
    Atom axes_labels[2] = {
        XIGetKnownProperty(AXIS_LABEL_PROP_ABS_X),
        XIGetKnownProperty(AXIS_LABEL_PROP_ABS_Y)
    };
    Atom btn_labels[3] = {
        XIGetKnownProperty(BTN_LABEL_PROP_BTN_LEFT),
        XIGetKnownProperty(BTN_LABEL_PROP_BTN_MIDDLE),
        XIGetKnownProperty(BTN_LABEL_PROP_BTN_RIGHT)
    };
    unsigned char map[4] = { 0, 1, 2, 3 };

    dev->public.on = FALSE;

    if (!InitKeyboardDeviceStruct(dev, NULL, hurd_kbd_bell, hurd_kbd_keyctrl))
        return !Success;

    if (!InitButtonClassDeviceStruct(dev, sizeof(map) - 1, btn_labels, map))
        return !Success;

    if (!InitValuatorClassDeviceStruct(dev, 2, axes_labels, 0, Absolute))
        return !Success;

    if (!InitPtrFeedbackClassDeviceStruct(dev, hurd_kbd_ptrctrl))
        return !Success;

    return Success;
}

static int
hurd_device_on(DeviceIntPtr dev)
{
    InputInfoPtr pInfo = dev->public.devicePrivate;
    HurdInputPriv *priv = pInfo->private;
    kern_return_t err;
    mach_port_t master_port;

    if (priv->kd_device != MACH_PORT_NULL)
        return Success;

    err = get_privileged_ports(&master_port, NULL);
    if (err != KERN_SUCCESS) {
        xf86IDrvMsg(pInfo, X_ERROR,
                    "get_privileged_ports failed: %s\n", mach_error_string(err));
        return BadValue;
    }

    err = device_open(master_port, D_READ, HURD_KBD_DEVICE, &priv->kd_device);
    mach_port_deallocate(mach_task_self(), master_port);
    if (err != KERN_SUCCESS) {
        xf86IDrvMsg(pInfo, X_ERROR,
                    "device_open(%s) failed: %s\n", HURD_KBD_DEVICE,
                    mach_error_string(err));
        return BadValue;
    }

    xf86IDrvMsg(pInfo, X_PROBED,
                "Opened mach device '%s' port=%#x\n",
                HURD_KBD_DEVICE, priv->kd_device);

    if (pipe(priv->wakeup_pipe) < 0) {
        xf86IDrvMsg(pInfo, X_ERROR, "pipe() failed: %s\n", strerror(errno));
        device_close(priv->kd_device);
        priv->kd_device = MACH_PORT_NULL;
        return BadValue;
    }

    priv->running = TRUE;
    if (pthread_create(&priv->reader_thread, NULL,
                       hurd_reader_thread, pInfo) != 0) {
        xf86IDrvMsg(pInfo, X_ERROR,
                    "pthread_create failed: %s\n", strerror(errno));
        priv->running = FALSE;
        close(priv->wakeup_pipe[0]);
        close(priv->wakeup_pipe[1]);
        device_close(priv->kd_device);
        priv->kd_device = MACH_PORT_NULL;
        return BadValue;
    }

    pInfo->fd = priv->wakeup_pipe[0];
    xf86AddEnabledDevice(pInfo);
    dev->public.on = TRUE;

    return Success;
}

static void
hurd_device_off(DeviceIntPtr dev)
{
    InputInfoPtr pInfo = dev->public.devicePrivate;
    HurdInputPriv *priv = pInfo->private;

    if (!dev->public.on)
        return;

    xf86RemoveEnabledDevice(pInfo);
    priv->running = FALSE;
    pthread_cancel(priv->reader_thread);
    pthread_join(priv->reader_thread, NULL);

    close(priv->wakeup_pipe[0]);
    close(priv->wakeup_pipe[1]);
    priv->wakeup_pipe[0] = -1;
    priv->wakeup_pipe[1] = -1;
    pInfo->fd = -1;

    dev->public.on = FALSE;
}

static int
hurd_device_control(DeviceIntPtr dev, int what)
{
    switch (what) {
    case DEVICE_INIT:
        return hurd_device_init(dev);
    case DEVICE_ON:
        return hurd_device_on(dev);
    case DEVICE_OFF:
        hurd_device_off(dev);
        return Success;
    case DEVICE_CLOSE:
        hurd_device_off(dev);
        {
            InputInfoPtr pInfo = dev->public.devicePrivate;
            HurdInputPriv *priv = pInfo->private;
            if (priv->kd_device != MACH_PORT_NULL) {
                device_close(priv->kd_device);
                priv->kd_device = MACH_PORT_NULL;
            }
        }
        return Success;
    }
    return BadValue;
}

static int
hurd_preinit(InputDriverPtr drv, InputInfoPtr pInfo, int flags)
{
    HurdInputPriv *priv;

    priv = calloc(1, sizeof(HurdInputPriv));
    if (!priv)
        return BadValue;

    priv->kd_device = MACH_PORT_NULL;
    priv->wakeup_pipe[0] = -1;
    priv->wakeup_pipe[1] = -1;
    priv->running = FALSE;

    pInfo->type_name = "Hurd Keyboard";
    pInfo->device_control = hurd_device_control;
    pInfo->read_input = hurd_read_input;
    pInfo->fd = -1;
    pInfo->private = priv;

    return Success;
}

static void
hurd_uninit(InputDriverPtr drv, InputInfoPtr pInfo, int flags)
{
    HurdInputPriv *priv = pInfo->private;

    if (priv) {
        pInfo->dev->public.on = FALSE;
        if (priv->kd_device != MACH_PORT_NULL)
            device_close(priv->kd_device);
        free(priv);
    }
    pInfo->private = NULL;
}

InputDriverRec HurdInput = {
    .driverVersion = 1,
    .driverName    = "hurd",
    .PreInit       = hurd_preinit,
    .UnInit        = hurd_uninit,
};

static void *
hurd_setup(void *mod, void *opt, int *errmaj, int *errmin)
{
    xf86AddInputDriver(&HurdInput, mod, 0);
    return mod;
}

XF86_MODULE_DATA_INPUT(
    hurdinput,
    hurd_setup,
    NULL,
    "xf86-input-hurd",
    XORG_VERSION_MAJOR,
    XORG_VERSION_MINOR,
    XORG_VERSION_PATCH);

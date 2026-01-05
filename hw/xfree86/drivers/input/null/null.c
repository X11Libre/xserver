#include <xorg-config.h>

#include <exevents.h>
#include <xserver-properties.h>

#include <xorgVersion.h>
#include <xf86Xinput.h>

/* dummy needed, so that clients don't get BadValue error
   when trying to ring the bell. */
static void nullinput_bell(int percent, DeviceIntPtr pDev, void *ctrl, int unused) { }

/* dummy needed, because no NULL protection here yet */
static void nullinput_keyctrl(DeviceIntPtr pDev, KeybdCtrl *ctrl) { }

/* dummy needed, because no NULL protection here yet */
static void nullinput_pointer(DeviceIntPtr dev, PtrCtrl *ctrl) { }

static int nullinput_device_init(DeviceIntPtr dev)
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

    if (!InitButtonClassDeviceStruct(dev, sizeof(map)-1, btn_labels, map))
        return !Success;

    if (InitKeyboardDeviceStruct(dev, NULL, nullinput_bell, nullinput_keyctrl))
        return !Success;

    if (!InitValuatorClassDeviceStruct(dev, 2, axes_labels, 0, Absolute))
        return !Success;

    if (!InitPtrFeedbackClassDeviceStruct(dev, nullinput_pointer))
        return !Success;

    return Success;
}

static int nullinput_device_control(DeviceIntPtr dev, int what)
{
    switch (what)
    {
        case DEVICE_INIT:
            return nullinput_device_init(dev);

        case DEVICE_ON:
            dev->public.on = TRUE;
            return Success;

        case DEVICE_OFF:
        case DEVICE_CLOSE:
            dev->public.on = FALSE;
            return Success;
    }
    return BadValue;
}

static int nullinput_preinit(InputDriverPtr drv, InputInfoPtr pInfo, int flags)
{
    pInfo->type_name = "null";
    pInfo->device_control = nullinput_device_control;
    pInfo->fd = -1;
    return Success;
}

static void nullinput_uninit(InputDriverPtr drv,InputInfoPtr pInfo, int flags)
{
    pInfo->dev->public.on = FALSE;
}

InputDriverRec NullInput = {
    .driverVersion = 1,
    .driverName    = "null",
    .PreInit       = nullinput_preinit,
    .UnInit        = nullinput_uninit,
};

static void* nullinput_setup(void *mod, void *opt, int *errmaj, int *errmin)
{
    xf86AddInputDriver(&NullInput, mod, 0);
    return mod;
}

XF86_MODULE_DATA_INPUT(
    nullinput,
    nullinput_setup,
    NULL,
    "nullinput",
    XORG_VERSION_MAJOR,
    XORG_VERSION_MINOR,
    XORG_VERSION_PATCH);

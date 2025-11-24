/* SPDX-License-Identifier: MIT OR X112
 *
 * Copyright © 2024 Enrico Weigelt, metux IT consult <info@metux.net>
 */
#ifndef _XSERVER_EXTENSION_PRIV_H
#define _XSERVER_EXTENSION_PRIV_H

#include "include/callback.h"
#include "include/extnsionst.h"
#include "include/misc.h"

#define EXTENSION_BASE 128

#define EXTENSION_MAJOR_APPLE_WM            (EXTENSION_BASE + 0)
#define EXTENSION_MAJOR_APPLE_DRI           (EXTENSION_BASE + 1)
#define EXTENSION_MAJOR_BIG_REQUESTS        (EXTENSION_BASE + 2)
#define EXTENSION_MAJOR_COMPOSITE           (EXTENSION_BASE + 3)
#define EXTENSION_MAJOR_DAMAGE              (EXTENSION_BASE + 4)
#define EXTENSION_MAJOR_DOUBLE_BUFFER       (EXTENSION_BASE + 5)
#define EXTENSION_MAJOR_DPMS                (EXTENSION_BASE + 6)
#define EXTENSION_MAJOR_DRI2                (EXTENSION_BASE + 7)
#define EXTENSION_MAJOR_DRI3                (EXTENSION_BASE + 8)
#define EXTENSION_MAJOR_GENERIC_EVENT       (EXTENSION_BASE + 9)
#define EXTENSION_MAJOR_GLX                 (EXTENSION_BASE + 10)
#define EXTENSION_MAJOR_MIT_SCREEN_SAVER    (EXTENSION_BASE + 11)
#define EXTENSION_MAJOR_NAMESPACE           (EXTENSION_BASE + 12)
#define EXTENSION_MAJOR_PRESENT             (EXTENSION_BASE + 13)
#define EXTENSION_MAJOR_RANDR               (EXTENSION_BASE + 14)
#define EXTENSION_MAJOR_RECORD              (EXTENSION_BASE + 15)
#define EXTENSION_MAJOR_RENDER              (EXTENSION_BASE + 16)
#define EXTENSION_MAJOR_SECURITY            (EXTENSION_BASE + 17)
#define EXTENSION_MAJOR_SELINUX             (EXTENSION_BASE + 18)
#define EXTENSION_MAJOR_SHAPE               (EXTENSION_BASE + 19)
#define EXTENSION_MAJOR_SHM                 (EXTENSION_BASE + 20)
#define EXTENSION_MAJOR_SYNC                (EXTENSION_BASE + 21)
#define EXTENSION_MAJOR_WINDOWS_DRI         (EXTENSION_BASE + 22)
#define EXTENSION_MAJOR_XFIXES              (EXTENSION_BASE + 23)
#define EXTENSION_MAJOR_XF86_BIGFONT        (EXTENSION_BASE + 24)
#define EXTENSION_MAJOR_XF86_DGA            (EXTENSION_BASE + 25)
#define EXTENSION_MAJOR_XF86_DRI            (EXTENSION_BASE + 26)
#define EXTENSION_MAJOR_XF86_VIDMODE        (EXTENSION_BASE + 27)
#define EXTENSION_MAJOR_XC_MISC             (EXTENSION_BASE + 28)
#define EXTENSION_MAJOR_XINPUT              (EXTENSION_BASE + 29)
#define EXTENSION_MAJOR_XINERAMA            (EXTENSION_BASE + 30)
#define EXTENSION_MAJOR_XKEYBOARD           (EXTENSION_BASE + 31)
#define EXTENSION_MAJOR_XRESOURCE           (EXTENSION_BASE + 32)
#define EXTENSION_MAJOR_XTEST               (EXTENSION_BASE + 33)
#define EXTENSION_MAJOR_XVIDEO              (EXTENSION_BASE + 34)
#define EXTENSION_MAJOR_XVMC                (EXTENSION_BASE + 35)

#define RESERVED_EXTENSIONS                 38

typedef struct {
    ClientPtr client;
    ExtensionEntry *ext;
    Mask access_mode;
    int status;
} ExtensionAccessCallbackParam;

extern CallbackListPtr ExtensionAccessCallback;
extern CallbackListPtr ExtensionDispatchCallback;

void EnableDisableExtensionError(const char *name, Bool enable);
void InitExtensions(int argc, char **argv);
void CloseDownExtensions(void);

ExtensionEntry *GetExtensionEntry(int major);

void NotImplemented(xEvent *, xEvent *) _X_NORETURN;

#endif /* _XSERVER_EXTENSION_PRIV_H */

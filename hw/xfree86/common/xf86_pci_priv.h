/* SPDX-License-Identifier: MIT OR X11
 *
 * Copyright © 2024 Enrico Weigelt, metux IT consult <info@metux.net>
 */
#ifndef _XSERVER_XF86_PCI_PRIV_H
#define _XSERVER_XF86_PCI_PRIV_H

#ifdef XSERVER_LIBPCIACCESS
#include <pciaccess.h>
#else
struct pci_device;
#endif

/*
 * placeholder for a new libpciaccess function in upcoming releases:
 * https://gitlab.freedesktop.org/xorg/lib/libpciaccess/-/merge_requests/39/
 *
 * callee code is already prepared for using it, but for the time being
 * we need a dummy - until the actual one is really there.
 */
#ifndef pci_device_is_boot_display
static inline int pci_device_is_boot_display(struct pci_device *dev)
{
    return 0;
}
#endif

#endif /* _XSERVER_XF86_PCI_PRIV_H */

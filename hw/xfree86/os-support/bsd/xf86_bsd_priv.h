/* SPDX-License-Identifier: MIT OR X11
 *
 * Copyright © 2024 Enrico Weigelt, metux IT consult <info@metux.net>
 */
#ifndef _XSERVER_XFREE86_OS_SUPPORT_BSD_PRIV_H
#define _XSERVER_XFREE86_OS_SUPPORT_BSD_PRIV_H

#ifdef __OpenBSD__
#define DEV_MEM "/dev/xf86"
#else
#define DEV_MEM "/dev/pmem"
#endif

#if defined(__NetBSD__) && !defined(MAP_FILE)
#define MAP_FLAGS MAP_SHARED
#else
#define MAP_FLAGS (MAP_FILE | MAP_SHARED)
#endif

#define DEV_APERTURE "/dev/xf86"

#endif /* _XSERVER_XFREE86_OS_SUPPORT_BSD_PRIV_H */

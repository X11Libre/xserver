/*
 * VNC Backend Header for RemoteDesktop Extension
 *
 * SPDX-License-Identifier: X11 OR MIT OR AGPL-3.0-or-later
 */

#ifndef _VNC_BACKEND_H_
#define _VNC_BACKEND_H_

#include "Xext/remotedesktop/remotedesktop_priv.h"

/* VNC backend registration */
void VNCBackendRegister(void);
void VNCBackendUnregister(void);

/* libvncserver availability */
#ifdef HAVE_LIBVNCSERVER
#define VNC_HAS_LIBVNCSERVER 1
#else
#define VNC_HAS_LIBVNCSERVER 0
#endif

#endif /* _VNC_BACKEND_H_ */
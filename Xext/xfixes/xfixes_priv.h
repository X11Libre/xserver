/* SPDX-License-Identifier: X11 OR MIT OR AGPL-3.0-or-later
 *
 * Copyright © 2024 Enrico Weigelt, metux IT consult <info@metux.net>
 */
#ifndef _XSERVER_XFIXES_PRIV_H
#define _XSERVER_XFIXES_PRIV_H

/* automatic server kill machinery, eg. for screen lockers */
bool XFixesMustTerminateServerOnDisconnect(ClientPtr client);

extern bool XFixesAllowForceTerminate;

#endif /* _XSERVER_XFIXES_PRIV_H */

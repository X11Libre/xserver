/* SPDX-License-Identifier: MIT OR X11
 *
 * Copyright © 2024 Enrico Weigelt, metux IT consult <info@metux.net>
 *
 * @brief: defines needed for SetNotifyFd() as well as ospoll
 */
#ifndef _XSERVER_INCLUDE_FDNOTIFY_H
#define _XSERVER_INCLUDE_FDNOTIFY_H

#define X_NOTIFY_NONE   0x0
#define X_NOTIFY_READ   0x1
#define X_NOTIFY_WRITE  0x2
#define X_NOTIFY_ERROR  0x4     /* don't need to select for, always reported */

#endif /* _XSERVER_INCLUDE_FDNOTIFY_H */

/* SPDX-License-Identifier: MIT OR X11
 *
 * Copyright © 2024 Enrico Weigelt, metux IT consult <info@metux.net>
 */
#include <dix-config.h>

#ifdef WIN32
#include <X11/Xwinsock.h>
#endif

#include "os/ossock.h"

void ossock_init(void)
{
#ifdef WIN32
    static WSADATA wsadata;
    if (!wsadata.wVersion)
        WSAStartup(0x0202, &wsadata);
#endif
}

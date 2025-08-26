/* SPDX-License-Identifier: MIT OR X11
 *
 * Copyright © 2024 Enrico Weigelt, metux IT consult <info@metux.net>
 */
#include <dix-config.h>

#include <unistd.h>

#ifdef WIN32
#include <X11/Xwinsock.h>
#else
#include <sys/ioctl.h>
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

int ossock_ioctl(int fd, unsigned long request, void *arg)
{
#ifdef WIN32
    int ret = ioctlsocket(fd, request, arg);
    if (ret == SOCKET_ERROR)
        ret = WSAGetLastError();
    return ret;
#else
    return ioctl(fd, request,arg);
#endif
}

int ossock_close(int fd)
{
#ifdef WIN32
    int ret = closesocket(fd);
    if (ret == SOCKET_ERROR)
        errno = WSAGetLastError();
    return ret;
#else
    return close(fd);
#endif
}

/* SPDX-License-Identifier: MIT OR X11
 *
 * Copyright © 2024 Enrico Weigelt, metux IT consult <info@metux.net>
 * Copyright © 2000 Jakub Jelinek (jakub@redhat.com)
 */
#ifndef _XSERVER_XF86_SBUSBUS_H
#define _XSERVER_XF86_SBUSBUS_H

#include <X11/Xdefs.h>

#include "xf86sbusBus.h"

struct sbus_devtable {
    int devId;
    int fbType;
    const char *promName;
    const char *driverName;
    const char *descr;
};

extern sbusDevicePtr *xf86SbusInfo;
extern struct sbus_devtable sbusDeviceTable[];

Bool xf86SbusConfigure(void *busData, sbusDevicePtr sBus);
void xf86SbusConfigureNewDev(void *busData, sbusDevicePtr sBus, GDevRec* GDev);
void xf86SbusProbe(void);

char *sparcPromGetProperty(sbusPromNodePtr pnode, const char *prop, int *lenp);
void sparcPromAssignNodes(void);
char *sparcPromNode2Pathname(sbusPromNodePtr pnode);
int sparcPromPathname2Node(const char *pathName);
char *sparcDriverName(void);

#endif /* _XSERVER_XF86_SBUSBUS_H */

/* SPDX-License-Identifier: MIT OR X11
 *
 * Copyright © 1987, 1998  The Open Group
 * Copyright © 2024 Enrico Weigelt, metux IT consult <info@metux.net>
 */
#ifndef _XSERVER_PROPERTY_PRIV_H
#define _XSERVER_PROPERTY_PRIV_H

#include <X11/X.h>

#include "dix.h"
#include "window.h"
#include "property.h"

typedef struct _PropertyStateRec {
    WindowPtr win;
    PropertyPtr prop;
    int state;
} PropertyStateRec;

extern CallbackListPtr PropertyStateCallback;

int dixLookupProperty(PropertyPtr *result, WindowPtr pWin, Atom proprty,
                      ClientPtr pClient, Mask access_mode);

void DeleteAllWindowProperties(WindowPtr pWin);

#endif /* _XSERVER_PROPERTY_PRIV_H */

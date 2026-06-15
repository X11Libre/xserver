/* SPDX-License-Identifier: MIT OR X11 OR AGPL-3.0-or-later
 *
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

typedef struct _PropertyFilterParam {
    // used by all requests
    ClientPtr client;
    Window window;
    Atom property;
    Atom type;

    // in case of RotateProperties
    Atom *atoms;
    size_t nAtoms;
    size_t nPositions;

    // caller notification
    Bool skip;                 // TRUE if the call shouldn't be executed
    int status;                // the status code to return when skip = TRUE
    Mask access_mode;

    int format;
    int mode;
    unsigned long len;
    const void *value;
    Bool sendevent;

    // only for GetProperty
    BOOL delete;
    CARD32 longOffset;
    CARD32 longLength;
} PropertyFilterParam;

extern CallbackListPtr PropertyFilterCallback;

int dixLookupProperty(PropertyPtr *result, WindowPtr pWin, Atom proprty,
                      ClientPtr pClient, Mask access_mode);

void DeleteAllWindowProperties(WindowPtr pWin);

int DeleteProperty(ClientPtr client, WindowPtr pWin, Atom propName);

#endif /* _XSERVER_PROPERTY_PRIV_H */

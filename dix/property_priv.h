/* SPDX-License-Identifier: MIT OR X11
 *
 * Copyright © 1987, 1998  The Open Group
 * Copyright © 2024 Enrico Weigelt, metux IT consult <info@metux.net>
 */
/***********************************************************

Copyright 1987, 1998  The Open Group

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation.

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from The Open Group.

Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the name of Digital not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.

DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

******************************************************************/
#ifndef _XSERVER_PROPERTY_PRIV_H
#define _XSERVER_PROPERTY_PRIV_H

#include <X11/X.h>

#include "include/privates.h"

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

/*
 * Free an individual property structure and related data.
 * Invalidates the passed pointer. Doesn't touch anything other in the list.
 *
 * @param pProp pointer to property structure to free
 */
void dixPropertyFree(PropertyPtr pProp);

/*
 * Create and fill a new property structure from given data. The `value`
 * is malloc()ed and copied over.
 *
 * @param type   AtomID of the property type
 * @param name   AtomID of the property name
 * @param format the property format (8/16/32 bits)
 * @param len    length in units defined by format
 * @param value  pointer to the (raw) property data)
 * @return pointer to newly created property structure (NULL on allocation failure)
 */
PropertyPtr dixPropertyCreate(Atom type, Atom name, int format,
                              size_t len, const void *value);

/*
 * Unlink a property structure from a property list. The given property's next
 * pointer is also cleared. The property structure itself is NOT destroyed.
 *
 * @param list pointer to the property list head (NULL on empty list)
 * @param prop pointer to the property to unlink.
 * @return on success, pointer to the property, NULL if not found in list
 */
PropertyPtr dixPropertyUnlinkPtr(PropertyPtr *list, PropertyPtr prop);

#endif /* _XSERVER_PROPERTY_PRIV_H */

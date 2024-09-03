/* SPDX-License-Identifier: MIT OR X11
 *
 * Copyright © 2024 Enrico Weigelt, metux IT consult <info@metux.net>
 *
 * @brief property management functions
 */
#include <dix-config.h>

#include "dix/dix_priv.h"
#include "dix/property_priv.h"
#include "include/propertyst.h"
#include "include/windowstr.h"

#include "xnest-xcb.h"

#include "Drawable.h"

static xcb_atom_t xnest_xlate_atom(struct xnest_upstream_info *upstream, ATOM outAtom)
{
    return xnest_intern_atom(upstream->conn, NameForAtom(outAtom));
}

static xcb_window_t xnest_xlate_window(Window window)
{
    WindowPtr pWin = dixLookupWindowByXID(window);
    return (pWin ? xnestWindow(pWin) : 0);
}

static void xnest_property_xlate(ScreenPtr pScreen, PropertyStateRec *rec, void *dataBuf)
{
    // must translate
    switch (rec->prop->type) {
        /* nothing to do for these */
        case XCB_ATOM_STRING:
        case XCB_ATOM_INTEGER:
        case XCB_ATOM_POINT:
        case XCB_ATOM_RECTANGLE:
        case XCB_ATOM_WM_SIZE_HINTS:
        case XCB_ATOM_CARDINAL:
        case XCB_ATOM_WM_HINTS:
            return;

        case XCB_ATOM_ATOM:
        {
            xcb_atom_t *atoms = dataBuf;
            for (int x=0; x<rec->prop->size; x++)
                atoms[x] = xnest_xlate_atom(&xnestUpstreamInfo, atoms[x]);
            return;
        }

        case XCB_ATOM_WINDOW:
        {
            xcb_window_t *windows = dataBuf;
            for (int x=0; x<rec->prop->size; x++)
                windows[x] = xnest_xlate_window(windows[x]);
            return;
        }
    }

    const char *atomName = NameForAtom(rec->prop->type);
    if (strcmp(atomName, "UTF8_STRING")==0)
        return;
    if (strcmp(atomName, "_MOTIF_DRAG_RECEIVER_INFO")==0)
        return;

    fprintf(stderr, "WARN: unknown atom type %s\n", atomName);
}

static void xnest_property_xlate_set(ScreenPtr pScreen, PropertyStateRec *rec)
{
    void *dataBuf = calloc(rec->prop->format, rec->prop->size);
    if (!dataBuf)
        return;

    memcpy(dataBuf, rec->prop->data, (rec->prop->format / 8) * rec->prop->size);

    xnest_property_xlate(pScreen, rec, dataBuf);

    xnest_upstream_set_property(&xnestUpstreamInfo,
                                xnestWindow(rec->win),
                                NameForAtom(rec->prop->propertyName),
                                NameForAtom(rec->prop->type),
                                rec->state,
                                rec->prop->format,
                                rec->prop->size,
                                dataBuf);

    free(dataBuf);
}

void xnest_property_state_callback(CallbackListPtr *pcbl, void *closure, void *calldata)
{
    ScreenPtr pScreen = closure;
    PropertyStateRec *rec = calldata;

    if (rec->win->drawable.pScreen != pScreen)
        return;

    if (rec->state == PropertyNewValue) {
        xnest_property_xlate_set(pScreen, rec);
        return;
    }

    xnest_upstream_set_property(&xnestUpstreamInfo,
                                xnestWindow(rec->win),
                                NameForAtom(rec->prop->propertyName),
                                NameForAtom(rec->prop->type),
                                rec->state,
                                rec->prop->format,
                                rec->prop->size,
                                rec->prop->data);
}

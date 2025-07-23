/*

Copyright 1993 by Davor Matic

Permission to use, copy, modify, distribute, and sell this software
and its documentation for any purpose is hereby granted without fee,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation.  Davor Matic makes no representations about
the suitability of this software for any purpose.  It is provided "as
is" without express or implied warranty.

*/
#include <dix-config.h>

#include <stddef.h>
#include <X11/X.h>
#include <X11/Xatom.h>
#include <X11/Xdefs.h>
#include <X11/Xproto.h>
#include <X11/fonts/font.h>
#include <X11/fonts/fontstruct.h>
#include <X11/fonts/libxfont2.h>

#include "dix/dix_priv.h"

#include "misc.h"
#include "regionstr.h"
#include "dixfontstr.h"
#include "scrnintstr.h"

#include "xnest-xcb.h"

#include "Display.h"
#include "XNFont.h"

int xnestFontPrivateIndex;

Bool
xnestRealizeFont(ScreenPtr pScreen, FontPtr pFont)
{
    int nprops;
    FontPropPtr props;
    int i;
    const char *name;

    xfont2_font_set_private(pFont, xnestFontPrivateIndex, NULL);

    Atom name_atom = dixAddAtom("FONT");
    Atom value_atom = 0L;

    nprops = pFont->info.nprops;
    props = pFont->info.props;

    for (i = 0; i < nprops; i++)
        if (props[i].name == name_atom) {
            value_atom = props[i].value;
            break;
        }

    if (!value_atom)
        return FALSE;

    name = NameForAtom(value_atom);

    if (!name)
        return FALSE;

    xnestPrivFont* priv = calloc(1, sizeof(xnestPrivFont));
    xfont2_font_set_private(pFont, xnestFontPrivateIndex, priv);

    priv->font_id = xcb_generate_id(xnestUpstreamInfo.conn);
    xcb_open_font(xnestUpstreamInfo.conn, priv->font_id, strlen(name), name);

    xcb_generic_error_t *err = NULL;
    priv->font_reply = xcb_query_font_reply(
        xnestUpstreamInfo.conn,
        xcb_query_font(xnestUpstreamInfo.conn, priv->font_id),
        &err);
    if (err) {
        ErrorF("failed to query font \"%s\": %d", name, err->error_code);
        free(err);
        return FALSE;
    }
    if (!priv->font_reply) {
        ErrorF("failed to query font \"%s\": no reply", name);
        return FALSE;
    }
    priv->chars_len = xcb_query_font_char_infos_length(priv->font_reply);
    priv->chars = xcb_query_font_char_infos(priv->font_reply);

    return TRUE;
}

Bool
xnestUnrealizeFont(ScreenPtr pScreen, FontPtr pFont)
{
    if (xnestFontPriv(pFont)) {
        xcb_close_font(xnestUpstreamInfo.conn, xnestFontPriv(pFont)->font_id);
        free(xnestFontPriv(pFont));
        xfont2_font_set_private(pFont, xnestFontPrivateIndex, NULL);
    }
    return TRUE;
}

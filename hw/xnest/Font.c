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

#ifdef HAVE_XNEST_CONFIG_H
#include <xnest-config.h>
#endif

#include <X11/X.h>
#include <X11/Xatom.h>
#include <X11/Xdefs.h>
#include <X11/Xproto.h>
#include <X11/fonts/font.h>
#include <X11/fonts/fontstruct.h>

#include "misc.h"
#include "regionstr.h"
#include "dixfontstr.h"
#include "scrnintstr.h"

#include "Xnest.h"

#include "Display.h"
#include "XNFont.h"

int xnestFontPrivateIndex;

Bool
xnestRealizeFont(ScreenPtr pScreen, FontPtr pFont)
{
    void *priv;
    Atom name_atom, value_atom;
    int nprops;
    FontPropPtr props;
    int i;
    const char *name;

    xfont2_font_set_private(pFont, xnestFontPrivateIndex, NULL);

    name_atom = MakeAtom("FONT", 4, TRUE);
    value_atom = 0L;

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

    priv = (void *) malloc(sizeof(xnestPrivFont));
    xfont2_font_set_private(pFont, xnestFontPrivateIndex, priv);

    if (!(xnestFontPriv(pFont)->font_struct = XLoadQueryFont(xnestDisplay, name)))
        return FALSE;

    return TRUE;
}

Bool
xnestUnrealizeFont(ScreenPtr pScreen, FontPtr pFont)
{
    if (xnestFontPriv(pFont)) {
        if (xnestFontPriv(pFont)->font_struct)
            XFreeFont(xnestDisplay, xnestFontPriv(pFont)->font_struct);
        free(xnestFontPriv(pFont));
        xfont2_font_set_private(pFont, xnestFontPrivateIndex, NULL);
    }
    return TRUE;
}

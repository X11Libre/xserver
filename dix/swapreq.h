/************************************************************

Copyright 1996 by Thomas E. Dickey <dickey@clark.net>

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the name of the above listed
copyright holder(s) not be used in advertising or publicity pertaining
to distribution of the software without specific, written prior
permission.

THE ABOVE LISTED COPYRIGHT HOLDER(S) DISCLAIM ALL WARRANTIES WITH REGARD
TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
AND FITNESS, IN NO EVENT SHALL THE ABOVE LISTED COPYRIGHT HOLDER(S) BE
LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

********************************************************/

#ifndef SWAPREQ_H
#define SWAPREQ_H 1

extern void SwapColorItem(xColorItem * /* pItem */ );

extern void SwapConnClientPrefix(xConnClientPrefix * /* pCCP */ );

int SProcAllocColorCells(ClientPtr client);
int SProcAllocColorPlanes(ClientPtr client);
int SProcAllocNamedColor(ClientPtr client);
int SProcChangeGC(ClientPtr client);
int SProcChangeHosts(ClientPtr client);
int SProcChangeKeyboardControl(ClientPtr client);
int SProcChangeKeyboardMapping(ClientPtr client);
int SProcChangePointerControl(ClientPtr client);
int SProcChangeWindowAttributes(ClientPtr client);
int SProcClearToBackground(ClientPtr client);
int SProcConfigureWindow(ClientPtr client);
int SProcConvertSelection(ClientPtr client);
int SProcCopyArea(ClientPtr client);
int SProcCopyColormapAndFree(ClientPtr client);
int SProcCopyGC(ClientPtr client);
int SProcCopyPlane(ClientPtr client);
int SProcCreateColormap(ClientPtr client);
int SProcCreateCursor(ClientPtr client);
int SProcCreateGC(ClientPtr client);
int SProcCreatePixmap(ClientPtr client);
int SProcCreateWindow(ClientPtr client);
int SProcFillPoly(ClientPtr client);
int SProcFreeColors(ClientPtr client);
int SProcGetImage(ClientPtr client);
int SProcGetMotionEvents(ClientPtr client);
int SProcGetProperty(ClientPtr client);
int SProcImageText(ClientPtr client);
int SProcListFonts(ClientPtr client);
int SProcListFontsWithInfo(ClientPtr client);
int SProcLookupColor(ClientPtr client);
int SProcOpenFont(ClientPtr client);
int SProcPutImage(ClientPtr client);
int SProcQueryBestSize(ClientPtr client);
int SProcQueryColors(ClientPtr client);
int SProcReparentWindow(ClientPtr client);
int SProcRotateProperties(ClientPtr client);
int SProcSetClipRectangles(ClientPtr client);
int SProcSetDashes(ClientPtr client);
int SProcSetFontPath(ClientPtr client);
int SProcStoreColors(ClientPtr client);
int SProcStoreNamedColor(ClientPtr client);
int SProcTranslateCoords(ClientPtr client);

#endif                          /* SWAPREQ_H */

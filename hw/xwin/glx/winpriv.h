/*
 * Export window information for the Windows-OpenGL GLX implementation.
 *
 * Authors: Alexander Gottwald
 */

#ifndef XSERVER_XWIN_WINPRIV_H
#define XSERVER_XWIN_WINPRIV_H

#include <X11/Xwindows.h>
#include <windowstr.h>

HWND winGetWindowInfo(WindowPtr pWin);
Bool winCheckScreenAiglxIsSupported(ScreenPtr pScreen);
void winSetScreenAiglxIsActive(ScreenPtr pScreen);

#endif /* XSERVER_XWIN_WINPRIV_H */

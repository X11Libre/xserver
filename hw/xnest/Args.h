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

#ifndef XNESTARGS_H
#define XNESTARGS_H

#include <X11/X.h>
#include <X11/Xdefs.h>

#include <xcb/xcb.h>

extern char *xnestDisplayName;
extern Bool xnestFullGeneration;
extern int xnestDefaultClass;
extern Bool xnestUserDefaultClass;
extern int xnestDefaultDepth;
extern Bool xnestUserDefaultDepth;
extern Bool xnestSoftwareScreenSaver;
extern xRectangle xnestGeometry;
extern int xnestUserGeometry;
extern int xnestBorderWidth;
extern Bool xnestUserBorderWidth;
extern char *xnestWindowName;
extern int xnestNumScreens;
extern Bool xnestDoDirectColormaps;
extern xcb_window_t xnestParentWindow;

#endif                          /* XNESTARGS_H */

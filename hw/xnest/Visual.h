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

#ifndef XNESTVISUAL_H
#define XNESTVISUAL_H

#include <X11/X.h>

Visual *xnestVisual(VisualPtr pVisual);
Visual *xnestVisualFromID(ScreenPtr pScreen, VisualID visual);
Colormap xnestDefaultVisualColormap(Visual * visual);

VisualPtr ScreenGetVisual(ScreenPtr pScreen, VisualID visual);

#endif                          /* XNESTVISUAL_H */

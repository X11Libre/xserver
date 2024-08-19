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

#ifndef XNESTCOMMON_H
#define XNESTCOMMON_H

#include "colormap.h"

#define UNDEFINED -1

#define MAXDEPTH 32
#define MAXVISUALSPERDEPTH 256

extern int xnestNumPixmapFormats;
extern Drawable xnestDefaultDrawables[MAXDEPTH + 1];
extern Pixmap xnestIconBitmap;
extern Pixmap xnestScreenSaverPixmap;
extern uint32_t xnestBitmapGC;
extern uint32_t xnestEventMask;

void xnestOpenDisplay(int argc, char *argv[]);
void xnestCloseDisplay(void);

#endif                          /* XNESTCOMMON_H */

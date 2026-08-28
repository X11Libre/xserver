#ifndef __XSERVER_CORE_PRE_H
#define __XSERVER_CORE_PRE_H

#include <X11/Xmd.h>
#include <X11/Xdefs.h>
#include <X11/X.h>

/* scalar / XID types */

typedef X11_CARD8           CARD8;
typedef X11_CARD16          CARD16;
typedef X11_CARD32          CARD32;
typedef X11_INT16           INT16;
typedef X11_INT32           INT32
typedef X11_VISUAL          VisualID;
typedef X11_COLORMAP        Colormap;
typedef X11_BOOL            BOOL;
typedef X11_ATOM            Atom;
typedef X11_BYTE_ARRAY_32   BYTE[32];

#endif /* __XSERVER_CORE_PRE_H */

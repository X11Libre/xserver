/* SPDX-License-Identifier: MIT OR X11
 *
 * Copyright © 2024 Enrico Weigelt, metux IT consult <info@metux.net>
 */
#ifndef _XSERVER_XKB_XKBFOUT_PRIV_H
#define _XSERVER_XKB_XKBFOUT_PRIV_H

#include <stdio.h>
#include <X11/X.h>
#include <X11/Xdefs.h>

#include "xkbstr.h"

typedef void (*XkbFileAddOnFunc) (FILE *file,
                                  XkbDescPtr result,
                                  Bool topLevel,
                                  Bool showImplicit,
                                  int fileSection,
                                  void *priv);

Bool XkbWriteXKBKeyTypes(FILE *file, XkbDescPtr result, Bool topLevel,
                         Bool showImplicit, XkbFileAddOnFunc addOn, void *priv);
Bool XkbWriteXKBKeycodes(FILE *file, XkbDescPtr result, Bool topLevel,
                         Bool showImplicit, XkbFileAddOnFunc addOn, void *priv);
Bool XkbWriteXKBCompatMap(FILE *file, XkbDescPtr result, Bool topLevel,
                          Bool showImplicit, XkbFileAddOnFunc addOn, void *priv);
Bool XkbWriteXKBSymbols(FILE *file, XkbDescPtr result, Bool topLevel,
                        Bool showImplicit, XkbFileAddOnFunc addOn, void *priv);
Bool XkbWriteXKBGeometry(FILE *file, XkbDescPtr result, Bool topLevel,
                         Bool showImplicit, XkbFileAddOnFunc addOn, void *priv);

#endif /* _XSERVER_XKB_XKBFOUT_PRIV_H */

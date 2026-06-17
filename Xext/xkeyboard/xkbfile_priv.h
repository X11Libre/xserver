/* SPDX-License-Identifier: MIT OR X11
 *
 * Copyright © 2024 Enrico Weigelt, metux IT consult <info@metux.net>
 * Copyright © 1994 by Silicon Graphics Computer Systems, Inc.
 */
#ifndef _XSERVER_XKB_XKBFILE_PRIV_H
#define _XSERVER_XKB_XKBFILE_PRIV_H

#include <stdio.h>
#include <X11/X.h>
#include <X11/Xdefs.h>

#include "xkbstr.h"

/* XKB error codes */
#define _XkbErrMissingNames		1
#define _XkbErrMissingTypes		2
#define _XkbErrMissingReqTypes		3
#define _XkbErrMissingSymbols		4
#define _XkbErrMissingCompatMap		7
#define _XkbErrMissingGeometry		9
#define _XkbErrIllegalContents		12
#define _XkbErrBadValue			16
#define _XkbErrBadMatch			17
#define _XkbErrBadTypeName		18
#define _XkbErrBadTypeWidth		19
#define _XkbErrBadFileType		20
#define _XkbErrBadFileVersion		21
#define _XkbErrBadAlloc			23
#define _XkbErrBadLength		24
#define _XkbErrBadImplementation	26

/*
 * read xkm file
 *
 * @param file the FILE to read from
 * @param need mask of needed elements (fails if some are missing)
 * @param want mask of wanted elements
 * @param result pointer to xkb descriptor to load the data into
 * @return mask of elements missing (from need | want)
 */
unsigned XkmReadFile(FILE *file, unsigned need, unsigned want,
                     XkbDescPtr *result);

#endif /* _XSERVER_XKB_XKBFILE_PRIV_H */

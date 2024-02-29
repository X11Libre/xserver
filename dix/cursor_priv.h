/* SPDX-License-Identifier: MIT OR X11
 *
 * Copyright © 2024 Enrico Weigelt, metux IT consult <info@metux.net>
 */
#ifndef _XSERVER_DIX_CURSOR_PRIV_H
#define _XSERVER_DIX_CURSOR_PRIV_H

#include "include/cursor.h"

extern CursorPtr rootCursor;

/* reference counting */
CursorPtr RefCursor(CursorPtr cursor);
CursorPtr UnrefCursor(CursorPtr cursor);
int CursorRefCount(const CursorPtr cursor);

#endif /* _XSERVER_DIX_CURSOR_PRIV_H */

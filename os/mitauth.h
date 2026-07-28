/* SPDX-License-Identifier: X11 OR MIT OR AGPL-3.0-or-later */
/* Copyright (C) 2026 Enrico Weigelt, metux IT consult <info@metux.net> */
#ifndef _XSERVER_OS_MITAUTH_H
#define _XSERVER_OS_MITAUTH_H

#include "auth.h"

XID MitCheckCookie(AuthCheckArgs);
XID MitGenerateCookie(AuthGenCArgs);
XID MitAddCookie(AuthAddCArgs);
int MitFromID(AuthFromIDArgs);
int MitRemoveCookie(AuthRemCArgs);
int MitResetCookie(AuthRstCArgs);

#endif /* _XSERVER_OS_MITAUTH_H */

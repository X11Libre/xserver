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

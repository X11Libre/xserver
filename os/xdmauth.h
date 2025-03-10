#ifndef _XSERVER_OS_XDMAUTH_H
#define _XSERVER_OS_XDMAUTH_H

#include "auth.h"

XID XdmCheckCookie(AuthCheckArgs);
XID XdmAddCookie(AuthAddCArgs);
int XdmFromID(AuthFromIDArgs);
int XdmRemoveCookie(AuthRemCArgs);
int XdmResetCookie(AuthRstCArgs);
void XdmAuthenticationInit(const char *cookie, int cookie_length);

#endif /* _XSERVER_OS_XDMAUTH_H */

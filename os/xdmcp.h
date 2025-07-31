#ifndef _XSERVER_OS_XDMCP_H
#define _XSERVER_OS_XDMCP_H

#include <X11/Xdmcp.h>

#include "osdep.h"

typedef Bool (*ValidatorFunc) (ARRAY8Ptr Auth, ARRAY8Ptr Data, int packet_type);
typedef Bool (*GeneratorFunc) (ARRAY8Ptr Auth, ARRAY8Ptr Data, int packet_type);
typedef Bool (*AddAuthorFunc) (unsigned name_length, const char *name,
                               unsigned data_length, char *data);

/* in xdmcp.c */
void XdmcpUseMsg(void);
int XdmcpOptions(int argc, char **argv, int i);
void XdmcpRegisterConnection(int type, const char *address, int addrlen);
void XdmcpRegisterAuthorizations(void);
void XdmcpRegisterAuthorization(const char *name);
void XdmcpInit(void);
void XdmcpReset(void);
void XdmcpOpenDisplay(int sock);
void XdmcpCloseDisplay(int sock);
void XdmcpRegisterAuthentication(const char *name,
                                 int namelen,
                                 const char *data,
                                 int datalen,
                                 ValidatorFunc Validator,
                                 GeneratorFunc Generator,
                                 AddAuthorFunc AddAuth);

struct sockaddr_in;
void XdmcpRegisterBroadcastAddress(const struct sockaddr_in *addr);

#endif /* _XSERVER_OS_XDMCP_H */

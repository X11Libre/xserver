/* SPDX-License-Identifier: MIT OR X11
 *
 * Copyright © 2024 Enrico Weigelt, metux IT consult <info@metux.net>
 */
#ifndef __XORG_OS_IO_H
#define __XORG_OS_IO_H

#include <X11/Xdefs.h>

#include "include/dix.h" /* ClientPtr */

struct _XtransConnInfo;

typedef struct _connectionInput *ConnectionInputPtr;
typedef struct _connectionOutput *ConnectionOutputPtr;

typedef struct {
    int fd;
    ConnectionInputPtr input;
    ConnectionOutputPtr output;
    XID auth_id;
    CARD32 conn_time;
    struct _XtransConnInfo *trans_conn;
    int flags;
} OsCommRec, *OsCommPtr;

int FlushClient(ClientPtr who, OsCommPtr oc);
void FreeOsBuffers(OsCommPtr oc);
void CloseDownFileDescriptor(OsCommPtr oc);

#endif /* __XORG_OS_IO_H */

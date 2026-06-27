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

/**
 * @brief write @p count bytes from @p buf into the client's output buffer
 *
 * This is the internal worker behind the exported WriteToClient() frontend
 * and does the actual buffering / flushing. All in-tree callers should use
 * this directly instead of the exported WriteToClient().
 *
 * @note Even though this is an internal API, the symbol is exported
 *       (_X_EXPORT) because in-tree modules that may be built as separate
 *       shared objects (e.g. GLX) need to link against it. It is NOT meant
 *       to be used by external drivers / modules — those keep using the
 *       legacy WriteToClient() entry point.
 *
 * @param who    the client to write to
 * @param count  number of bytes to write
 * @param buf    data to write
 * @return       number of bytes buffered, 0 on no-op, -1 on error
 */
_X_EXPORT int dixWriteToClient(ClientPtr who, int count, const void *buf);

int FlushClient(ClientPtr who, OsCommPtr oc);
void FreeOsBuffers(OsCommPtr oc);
void CloseDownFileDescriptor(OsCommPtr oc);

#endif /* __XORG_OS_IO_H */

/*

Copyright 1993, 1994, 1998  The Open Group

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation.

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall
not be used in advertising or otherwise to promote the sale, use or
other dealings in this Software without prior written authorization
from The Open Group.

 * Copyright 1993, 1994 NCR Corporation - Dayton, Ohio, USA
 *
 * All Rights Reserved
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name NCR not be used in advertising
 * or publicity pertaining to distribution of the software without specific,
 * written prior permission.  NCR makes no representations about the
 * suitability of this software for any purpose.  It is provided "as is"
 * without express or implied warranty.
 *
 * NCR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN
 * NO EVENT SHALL NCR BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef _XTRANS_H_
#define _XTRANS_H_

#include <stdint.h>

#include <X11/Xfuncproto.h>
#include <X11/Xos.h>
#include <X11/Xmd.h>

#ifndef WIN32
#include <sys/socket.h>
#endif

#ifdef __clang__
/* Not all clients make use of all provided statics */
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-function"
#endif

/*
 * Set the functions names according to where this code is being compiled.
 */

#ifdef XTRANSDEBUG
static const char *__xtransname = "_XSERVTrans";
#endif

#ifdef __clang__
#pragma clang diagnostic pop
#endif

/*
 * Create a single address structure that can be used wherever
 * an address structure is needed. struct sockaddr is not big enough
 * to hold a sockadd_un, so we create this definition to have a single
 * structure that is big enough for all the structures we might need.
 *
 * This structure needs to be independent of the socket/TLI interface used.
 */

/* Temporary workaround for consumers whose configure scripts were
   generated with pre-1.6 versions of xtrans.m4 */
#if defined(IPv6) && !defined(HAVE_STRUCT_SOCKADDR_STORAGE)
#define HAVE_STRUCT_SOCKADDR_STORAGE
#endif

#ifdef HAVE_STRUCT_SOCKADDR_STORAGE
typedef struct sockaddr_storage Xtransaddr;
#else
#define XTRANS_MAX_ADDR_LEN	128	/* large enough to hold sun_path */

typedef	struct {
    unsigned char	addr[XTRANS_MAX_ADDR_LEN];
} Xtransaddr;
#endif

#ifdef LONG64
typedef int BytesReadable_t;
#else
typedef long BytesReadable_t;
#endif


#if defined(WIN32)

/*
 *      _XSERVTransWritev use struct iovec, normally found
 *      in Berkeley systems in <sys/uio.h>.  See the readv(2) and writev(2)
 *      manual pages for details.
 */

struct iovec {
    caddr_t iov_base;
    int iov_len;
};

#else
#include <sys/uio.h>
#endif

typedef struct _XtransConnInfo *XtransConnInfo;


/*
 * Transport Option definitions
 */

#define TRANS_NONBLOCKING	1
#define	TRANS_CLOSEONEXEC	2


/*
 * Return values of Connect (0 is success)
 */

#define TRANS_CONNECT_FAILED 	-1
#define TRANS_TRY_CONNECT_AGAIN -2
#define TRANS_IN_PROGRESS	-3


/*
 * Return values of CreateListener (0 is success)
 */

#define TRANS_CREATE_LISTENER_FAILED 	-1
#define TRANS_ADDR_IN_USE		-2


/*
 * Return values of Accept (0 is success)
 */

#define TRANS_ACCEPT_BAD_MALLOC			-1
#define TRANS_ACCEPT_FAILED 			-2
#define TRANS_ACCEPT_MISC_ERROR			-3


/*
 * ResetListener return values
 */

#define TRANS_RESET_NOOP	1
#define TRANS_RESET_NEW_FD	2
#define TRANS_RESET_FAILURE	3


/*
 * Function prototypes for the exposed interface
 */

void _XSERVTransFreeConnInfo (
    XtransConnInfo 	/* ciptr */
);

XtransConnInfo _XSERVTransOpenCOTSServer(
    const char *	/* address */
);

XtransConnInfo _XSERVTransReopenCOTSServer(
    int,		/* trans_id */
    int,		/* fd */
    const char *	/* port */
);

int _XSERVTransSetOption(
    XtransConnInfo,	/* ciptr */
    int,		/* option */
    int			/* arg */
);

int _XSERVTransCreateListener(
    XtransConnInfo,	/* ciptr */
    const char *,	/* port */
    unsigned int	/* flags */
);

int _XSERVTransReceived (
    const char*         /* protocol*/
);

int _XSERVTransNoListen (
    const char*         /* protocol*/
);

int _XSERVTransListen (
    const char*         /* protocol*/
);

int _XSERVTransIsListening (
    const char*         /* protocol*/
);

int _XSERVTransResetListener (
    XtransConnInfo	/* ciptr */
);

XtransConnInfo _XSERVTransAccept (
    XtransConnInfo,	/* ciptr */
    int *		/* status */
);

int _XSERVTransBytesReadable (
    XtransConnInfo,	/* ciptr */
    BytesReadable_t *	/* pend */
);

int _XSERVTransRead (
    XtransConnInfo,	/* ciptr */
    char *,		/* buf */
    int			/* size */
);

int _XSERVTransWrite (
    XtransConnInfo,	/* ciptr */
    const char *,	/* buf */
    int			/* size */
);

int _XSERVTransWritev (
    XtransConnInfo,	/* ciptr */
    struct iovec *,	/* buf */
    int			/* size */
);

int _XSERVTransSendFd (XtransConnInfo ciptr, int fd, int do_close);

int _XSERVTransRecvFd (XtransConnInfo ciptr);

int _XSERVTransDisconnect (
    XtransConnInfo	/* ciptr */
);

int _XSERVTransClose (
    XtransConnInfo	/* ciptr */
);

int _XSERVTransCloseForCloning (
    XtransConnInfo	/* ciptr */
);

int _XSERVTransIsLocal (
    XtransConnInfo	/* ciptr */
);

int _XSERVTransGetPeerAddr (
    XtransConnInfo,	/* ciptr */
    int *,		/* familyp */
    int *,		/* addrlenp */
    Xtransaddr **	/* addrp */
);

int _XSERVTransGetConnectionNumber (
    XtransConnInfo	/* ciptr */
);

int _XSERVTransMakeAllCOTSServerListeners (
    const char *,	/* port */
    int *,		/* partial */
    uint32_t *,		/* count_ret */
    XtransConnInfo **	/* ciptrs_ret */
);

/*
 * Function Prototypes for Utility Functions.
 */

int _XSERVTransConvertAddress (
    int *,		/* familyp */
    int *,		/* addrlenp */
    Xtransaddr **	/* addrp */
);

int _XSERVTransGetHostname (
    char *	/* buf */,
    int 	/* maxlen */
);

#if defined(WIN32) && defined(TCPCONN)
int _XSERVTransWSAStartup(void);
#endif

#endif /* _XTRANS_H_ */

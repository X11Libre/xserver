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

/*
 *
 * The connection code/ideas in lib/X and server/os for SVR4/Intel
 * environments was contributed by the following companies/groups:
 *
 *	MetroLink Inc
 *	NCR
 *	Pittsburgh Powercomputing Corporation (PPc)/Quarterdeck Office Systems
 *	SGCS
 *	Unix System Laboratories (USL) / Novell
 *	XFree86
 *
 * The goal is to have common connection code among all SVR4/Intel vendors.
 *
 * ALL THE ABOVE COMPANIES DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS
 * SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS,
 * IN NO EVENT SHALL THESE COMPANIES * BE LIABLE FOR ANY SPECIAL, INDIRECT
 * OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
 * OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE
 * OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <errno.h>
#include <ctype.h>
#include <sys/signal.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#if defined(SVR4) || defined(__SVR4)
#include <sys/filio.h>
#endif
# include <stropts.h>
#include <sys/wait.h>
#include <sys/types.h>

/*
 * The local transports should be treated the same as a UNIX domain socket
 * wrt authentication, etc. Because of this, we will use struct sockaddr_un
 * for the address format. This will simplify the code in other places like
 * The X Server.
 */

#include <sys/socket.h>
#include <sys/un.h>


/* Types of local connections supported:
 *  - PTS
 *  - named pipes
 */
#if defined(SVR4) || defined(__SVR4)
# define LOCAL_TRANS_NAMED
#endif

static int TRANS(LocalClose)(XtransConnInfo ciptr);

/*
 * These functions actually implement the local connection mechanisms.
 */

/* Type Not Supported */

static int
TRANS(OpenFail)(XtransConnInfo ciptr _X_UNUSED, const char *port _X_UNUSED)

{
    return -1;
}

static int
TRANS(ReopenFail)(XtransConnInfo ciptr _X_UNUSED, int fd _X_UNUSED,
                  const char *port _X_UNUSED)

{
    return 0;
}

#if XTRANS_SEND_FDS
static int
TRANS(LocalRecvFdInvalid)(XtransConnInfo ciptr)
{
    errno = EINVAL;
    return -1;
}

static int
TRANS(LocalSendFdInvalid)(XtransConnInfo ciptr, int fd, int do_close)
{
    errno = EINVAL;
    return -1;
}
#endif


static int
TRANS(FillAddrInfo)(XtransConnInfo ciptr,
                    const char *sun_path, const char *peer_sun_path)

{
    struct sockaddr_un	*sunaddr;
    struct sockaddr_un	*p_sunaddr;

    ciptr->family = AF_UNIX;
    ciptr->addrlen = sizeof (struct sockaddr_un);

    if ((sunaddr = malloc (ciptr->addrlen)) == NULL)
    {
	prmsg(1,"FillAddrInfo: failed to allocate memory for addr\n");
	return 0;
    }

    sunaddr->sun_family = AF_UNIX;

    if (strlen(sun_path) > sizeof(sunaddr->sun_path) - 1) {
	prmsg(1, "FillAddrInfo: path too long\n");
	free((char *) sunaddr);
	return 0;
    }
    strcpy (sunaddr->sun_path, sun_path);
#if defined(BSD44SOCKETS)
    sunaddr->sun_len = strlen (sunaddr->sun_path);
#endif

    ciptr->addr = (char *) sunaddr;

    ciptr->peeraddrlen = sizeof (struct sockaddr_un);

    if ((p_sunaddr = malloc (ciptr->peeraddrlen)) == NULL)
    {
	prmsg(1,
	   "FillAddrInfo: failed to allocate memory for peer addr\n");
	free (sunaddr);
	ciptr->addr = NULL;

	return 0;
    }

    p_sunaddr->sun_family = AF_UNIX;

    if (strlen(peer_sun_path) > sizeof(p_sunaddr->sun_path) - 1) {
	prmsg(1, "FillAddrInfo: peer path too long\n");
	free((char *) p_sunaddr);
	return 0;
    }
    strcpy (p_sunaddr->sun_path, peer_sun_path);
#if defined(BSD44SOCKETS)
    p_sunaddr->sun_len = strlen (p_sunaddr->sun_path);
#endif

    ciptr->peeraddr = (char *) p_sunaddr;

    return 1;
}


#define X_STREAMS_DIR	"/tmp/.X11-pipe"

#define NAMEDNODENAME "/tmp/.X11-pipe/X"

#ifdef LOCAL_TRANS_NAMED

/* NAMED */

#ifdef NAMEDNODENAME
static int
TRANS(NAMEDOpenPipe)(const char *server_path)
{
    int			fd, pipefd[2];
    struct stat		sbuf;
    int			mode;

    prmsg(2,"NAMEDOpenPipe(%s)\n", server_path);

#ifdef HAS_STICKY_DIR_BIT
    mode = 01777;
#else
    mode = 0777;
#endif
    if (trans_mkdir(X_STREAMS_DIR, mode) == -1) {
	prmsg (1, "NAMEDOpenPipe: mkdir(%s) failed, errno = %d\n",
	       X_STREAMS_DIR, errno);
	return(-1);
    }

    if(stat(server_path, &sbuf) != 0) {
	if (errno == ENOENT) {
	    if ((fd = creat(server_path, (mode_t)0666)) == -1) {
		prmsg(1, "NAMEDOpenPipe: Can't open %s\n", server_path);
		return(-1);
	    }
	    if (fchmod(fd, (mode_t)0666) < 0) {
		prmsg(1, "NAMEDOpenPipe: Can't chmod %s\n", server_path);
		close(fd);
		return(-1);
	    }
	    close(fd);
	} else {
	    prmsg(1, "NAMEDOpenPipe: stat on %s failed\n", server_path);
	    return(-1);
	}
    }

    if( pipe(pipefd) != 0) {
	prmsg(1, "NAMEDOpenPipe: pipe() failed, errno=%d\n",errno);
	return(-1);
    }

    if( ioctl(pipefd[0], I_PUSH, "connld") != 0) {
	prmsg(1, "NAMEDOpenPipe: ioctl(I_PUSH,\"connld\") failed, errno=%d\n",errno);
	close(pipefd[0]);
	close(pipefd[1]);
	return(-1);
    }

    if( fattach(pipefd[0], server_path) != 0) {
	prmsg(1, "NAMEDOpenPipe: fattach(%s) failed, errno=%d\n", server_path,errno);
	close(pipefd[0]);
	close(pipefd[1]);
	return(-1);
    }

    return(pipefd[1]);
}
#endif

static int
TRANS(NAMEDOpenServer)(XtransConnInfo ciptr, const char *port)
{
#ifdef NAMEDNODENAME
    int			fd;
    char		server_path[64];
#endif

    prmsg(2,"NAMEDOpenServer(%s)\n", port);

#if !defined(NAMEDNODENAME)
    prmsg(1,"NAMEDOpenServer: Protocol is not supported by a NAMED connection\n");
    return -1;
#else
    if ( port && *port ) {
	if( *port == '/' ) { /* A full pathname */
	    (void) snprintf(server_path, sizeof(server_path), "%s", port);
	} else {
	    (void) snprintf(server_path, sizeof(server_path), "%s%s",
			    NAMEDNODENAME, port);
	}
    } else {
	(void) snprintf(server_path, sizeof(server_path), "%s%ld",
		       NAMEDNODENAME, (long)getpid());
    }

    fd = TRANS(NAMEDOpenPipe)(server_path);
    if (fd < 0) {
	return -1;
    }

    /*
     * Everything looks good: fill in the XtransConnInfo structure.
     */

    if (TRANS(FillAddrInfo) (ciptr, server_path, server_path) == 0)
    {
	prmsg(1,"NAMEDOpenServer: failed to fill in addr info\n");
	TRANS(LocalClose)(ciptr);
	return -1;
    }

    return fd;

#endif /* !NAMEDNODENAME */
}

static int
TRANS(NAMEDResetListener) (XtransConnInfo ciptr)

{
  struct sockaddr_un      *sockname=(struct sockaddr_un *) ciptr->addr;
  struct stat     statb;

  prmsg(2,"NAMEDResetListener(%p, %d)\n", (void *) ciptr, ciptr->fd);

  if (ciptr->fd != -1) {
    /*
     * see if the pipe has disappeared
     */

    if (stat (sockname->sun_path, &statb) == -1 ||
	(statb.st_mode & S_IFMT) != S_IFIFO) {
      prmsg(3, "Pipe %s trashed, recreating\n", sockname->sun_path);
      TRANS(LocalClose)(ciptr);
      ciptr->fd = TRANS(NAMEDOpenPipe)(sockname->sun_path);
      if (ciptr->fd >= 0)
	  return TRANS_RESET_NEW_FD;
      else
	  return TRANS_CREATE_LISTENER_FAILED;
    }
  }
  return TRANS_RESET_NOOP;
}

static int
TRANS(NAMEDAccept)(XtransConnInfo ciptr, XtransConnInfo newciptr, int *status)

{
    struct strrecvfd str;

    prmsg(2,"NAMEDAccept(%p->%d)\n", (void *) ciptr, ciptr->fd);

    if( ioctl(ciptr->fd, I_RECVFD, &str ) < 0 ) {
	prmsg(1, "NAMEDAccept: ioctl(I_RECVFD) failed, errno=%d\n", errno);
	*status = TRANS_ACCEPT_MISC_ERROR;
	return(-1);
    }

    /*
     * Everything looks good: fill in the XtransConnInfo structure.
     */
    newciptr->family=ciptr->family;
    newciptr->addrlen=ciptr->addrlen;
    if( (newciptr->addr = malloc(newciptr->addrlen)) == NULL ) {
	prmsg(1,
	      "NAMEDAccept: failed to allocate memory for pipe addr\n");
	close(str.fd);
	*status = TRANS_ACCEPT_BAD_MALLOC;
	return -1;
    }

    memcpy(newciptr->addr,ciptr->addr,newciptr->addrlen);

    newciptr->peeraddrlen=newciptr->addrlen;
    if( (newciptr->peeraddr = malloc(newciptr->peeraddrlen)) == NULL ) {
	prmsg(1,
	"NAMEDAccept: failed to allocate memory for peer addr\n");
	free(newciptr->addr);
	close(str.fd);
	*status = TRANS_ACCEPT_BAD_MALLOC;
	return -1;
    }

    memcpy(newciptr->peeraddr,newciptr->addr,newciptr->peeraddrlen);

    *status = 0;

    return str.fd;
}

#endif /* LOCAL_TRANS_NAMED */

#ifdef LOCAL_TRANS_NAMED

static int
TRANS(NAMEDReopenServer)(XtransConnInfo ciptr, int fd _X_UNUSED, const char *port)

{
#ifdef NAMEDNODENAME
    char server_path[64];
#endif

    prmsg(2,"NAMEDReopenServer(%s)\n", port);

#if !defined(NAMEDNODENAME)
    prmsg(1,"NAMEDReopenServer: Protocol is not supported by a NAMED connection\n");
    return 0;
#else
    if ( port && *port ) {
	if( *port == '/' ) { /* A full pathname */
	    snprintf(server_path, sizeof(server_path),"%s", port);
	} else {
	    snprintf(server_path, sizeof(server_path), "%s%s",
		     NAMEDNODENAME, port);
	}
    } else {
	snprintf(server_path, sizeof(server_path), "%s%ld",
		NAMEDNODENAME, (long)getpid());
    }

    if (TRANS(FillAddrInfo) (ciptr, server_path, server_path) == 0)
    {
	prmsg(1,"NAMEDReopenServer: failed to fill in addr info\n");
	return 0;
    }

    return 1;

#endif /* !NAMEDNODENAME */
}

#endif /* LOCAL_TRANS_NAMED */

/*
 * This table contains all of the entry points for the different local
 * connection mechanisms.
 */

typedef struct _LOCALtrans2dev {
    const char	*transname;
    int	(*devcotsopenserver)(
	XtransConnInfo, const char * /*port*/
);
    int	(*devcltsopenserver)(
	XtransConnInfo, const char * /*port*/
);

    int	(*devcotsreopenserver)(
	XtransConnInfo,
	int, 	/* fd */
	const char * 	/* port */
);

    int	(*devcltsreopenserver)(
	XtransConnInfo,
	int, 	/* fd */
	const char *	/* port */
);

    int (*devreset)(
	XtransConnInfo /* ciptr */
);

    int	(*devaccept)(
	XtransConnInfo, XtransConnInfo, int *
);

} LOCALtrans2dev;

static LOCALtrans2dev LOCALtrans2devtab[] = {
{"",
     TRANS(NAMEDOpenServer),
     TRANS(OpenFail),
     TRANS(NAMEDReopenServer),
     TRANS(ReopenFail),
     TRANS(NAMEDResetListener),
     TRANS(NAMEDAccept)
},

{"local",
     TRANS(NAMEDOpenServer),
     TRANS(OpenFail),
     TRANS(NAMEDReopenServer),
     TRANS(ReopenFail),
     TRANS(NAMEDResetListener),
     TRANS(NAMEDAccept)
},

#ifdef LOCAL_TRANS_NAMED
{"named",
     TRANS(NAMEDOpenServer),
     TRANS(OpenFail),
     TRANS(NAMEDReopenServer),
     TRANS(ReopenFail),
     TRANS(NAMEDResetListener),
     TRANS(NAMEDAccept)
},

{"pipe",
     TRANS(NAMEDOpenServer),
     TRANS(OpenFail),
     TRANS(NAMEDReopenServer),
     TRANS(ReopenFail),
     TRANS(NAMEDResetListener),
     TRANS(NAMEDAccept)
},
#endif /* LOCAL_TRANS_NAMED */


};

#define NUMTRANSPORTS	(sizeof(LOCALtrans2devtab)/sizeof(LOCALtrans2dev))

static const char	*XLOCAL=NULL;
static	char	*workingXLOCAL=NULL;
static	char	*freeXLOCAL=NULL;

#define DEF_XLOCAL "UNIX:NAMED"

static void
TRANS(LocalInitTransports)(const char *protocol)

{
    prmsg(3,"LocalInitTransports(%s)\n", protocol);

    if( strcmp(protocol,"local") && strcmp(protocol,"LOCAL") )
    {
	workingXLOCAL = freeXLOCAL = strdup (protocol);
    }
    else {
	XLOCAL = getenv("XLOCAL");
	if(XLOCAL==NULL)
	    XLOCAL=DEF_XLOCAL;
	workingXLOCAL = freeXLOCAL = strdup (XLOCAL);
    }
}

static void
TRANS(LocalEndTransports)(void)

{
    prmsg(3,"LocalEndTransports()\n");
    free(freeXLOCAL);
    freeXLOCAL = NULL;
}

#define TYPEBUFSIZE	32

static XtransConnInfo
TRANS(LocalOpenServer)(int type, const char *protocol,
                       const char *host _X_UNUSED, const char *port)

{
    XtransConnInfo ciptr;

    prmsg(2,"LocalOpenServer(%d,%s,%s)\n", type, protocol, port);

    /*
     * For X11, the port will be in the format xserverN where N is the
     * display number. All of the local connections just need to know
     * the display number because they don't do any name resolution on
     * the port. This just truncates port to the display portion.
     */

    if( (ciptr = calloc(1,sizeof(struct _XtransConnInfo))) == NULL )
    {
	prmsg(1,"LocalOpenServer: calloc(1,%lu) failed\n",
	      sizeof(struct _XtransConnInfo));
	return NULL;
    }

    for (unsigned int i = 1; i < NUMTRANSPORTS; i++)
    {
	if( strcmp(protocol,LOCALtrans2devtab[i].transname) != 0 )
	    continue;
	switch( type )
	{
	case XTRANS_OPEN_COTS_CLIENT:
	    prmsg(1,
		  "LocalOpenServer: Should not be opening a client with this function\n");
	    break;
	case XTRANS_OPEN_COTS_SERVER:
	    ciptr->fd=LOCALtrans2devtab[i].devcotsopenserver(ciptr,port);
	    break;
	default:
	    prmsg(1,"LocalOpenServer: Unknown Open type %d\n",
		  type );
	}
	if( ciptr->fd >= 0 ) {
	    ciptr->priv=(char *)&LOCALtrans2devtab[i];
	    ciptr->index=i;
	    ciptr->flags = 1 | (ciptr->flags & TRANS_KEEPFLAGS);
	    return ciptr;
	}
    }

    free(ciptr);
    return NULL;
}

static XtransConnInfo
TRANS(LocalReopenServer)(int type, int index, int fd, const char *port)

{
    XtransConnInfo ciptr;
    int stat = 0;

    prmsg(2,"LocalReopenServer(%d,%d,%d)\n", type, index, fd);

    if( (ciptr = calloc(1,sizeof(struct _XtransConnInfo))) == NULL )
    {
	prmsg(1,"LocalReopenServer: calloc(1,%lu) failed\n",
	      sizeof(struct _XtransConnInfo));
	return NULL;
    }

    ciptr->fd = fd;

    switch( type )
    {
    case XTRANS_OPEN_COTS_SERVER:
	stat = LOCALtrans2devtab[index].devcotsreopenserver(ciptr,fd,port);
	break;
    default:
	prmsg(1,"LocalReopenServer: Unknown Open type %d\n",
	  type );
    }

    if( stat > 0 ) {
	ciptr->priv=(char *)&LOCALtrans2devtab[index];
	ciptr->index=index;
	ciptr->flags = 1 | (ciptr->flags & TRANS_KEEPFLAGS);
	return ciptr;
    }

    free(ciptr);
    return NULL;
}

/*
 * This is the Local implementation of the X Transport service layer
 */

static XtransConnInfo
TRANS(LocalOpenCOTSServer)(Xtransport *thistrans, const char *protocol,
			   const char *host, const char *port)

{
    char *typetocheck = NULL;
    int found = 0;

    prmsg(2,"LocalOpenCOTSServer(%s,%s,%s)\n",protocol,host,port);

    /* Check if this local type is in the XLOCAL list */
    TRANS(LocalInitTransports)("local");
    typetocheck = workingXLOCAL;
    while (typetocheck && !found) {
#ifndef HAVE_STRCASECMP
	int j;
	char typebuf[TYPEBUFSIZE];
#endif

	workingXLOCAL = strchr(workingXLOCAL, ':');
	if (workingXLOCAL && *workingXLOCAL)
	    *workingXLOCAL++ = '\0';
#ifndef HAVE_STRCASECMP
	strncpy(typebuf, typetocheck, TYPEBUFSIZE);
	for (j = 0; j < TYPEBUFSIZE; j++)
	    if (isupper(typebuf[j]))
		typebuf[j] = tolower(typebuf[j]);
	if (!strcmp(thistrans->TransName, typebuf))
#else
	if (!strcasecmp(thistrans->TransName, typetocheck))
#endif
	    found = 1;
	typetocheck = workingXLOCAL;
    }
    TRANS(LocalEndTransports)();

    if (!found) {
	prmsg(3,"LocalOpenCOTSServer: disabling %s\n",thistrans->TransName);
	thistrans->flags |= TRANS_DISABLED;
	return NULL;
    }

    return TRANS(LocalOpenServer)(XTRANS_OPEN_COTS_SERVER, protocol, host, port);
}

static XtransConnInfo
TRANS(LocalReopenCOTSServer)(Xtransport *thistrans, int fd, const char *port)

{
    unsigned int index;

    prmsg(2,"LocalReopenCOTSServer(%d,%s)\n", fd, port);

    for(index=1;index<NUMTRANSPORTS;index++)
    {
	if( strcmp(thistrans->TransName,
	    LOCALtrans2devtab[index].transname) == 0 )
	    break;
    }

    if (index >= NUMTRANSPORTS)
    {
	return (NULL);
    }

    return TRANS(LocalReopenServer)(XTRANS_OPEN_COTS_SERVER,
	index, fd, port);
}

static int
TRANS(LocalSetOption)(XtransConnInfo ciptr, int option, int arg)

{
    prmsg(2,"LocalSetOption(%d,%d,%d)\n",ciptr->fd,option,arg);

    return -1;
}

static int
TRANS(LocalCreateListener)(XtransConnInfo ciptr, const char *port,
                           unsigned int flags _X_UNUSED)

{
    prmsg(2,"LocalCreateListener(%p->%d,%s)\n", (void *) ciptr, ciptr->fd, port);

    return 0;
}

static int
TRANS(LocalResetListener)(XtransConnInfo ciptr)

{
    LOCALtrans2dev	*transptr;

    prmsg(2,"LocalResetListener(%p)\n", (void *) ciptr);

    transptr=(LOCALtrans2dev *)ciptr->priv;
    if (transptr->devreset != NULL) {
	return transptr->devreset(ciptr);
    }
    return TRANS_RESET_NOOP;
}


static XtransConnInfo
TRANS(LocalAccept)(XtransConnInfo ciptr, int *status)

{
    XtransConnInfo	newciptr;
    LOCALtrans2dev	*transptr;

    prmsg(2,"LocalAccept(%p->%d)\n", (void *) ciptr, ciptr->fd);

    transptr=(LOCALtrans2dev *)ciptr->priv;

    if( (newciptr = calloc(1,sizeof(struct _XtransConnInfo)))==NULL )
    {
	prmsg(1,"LocalAccept: calloc(1,%lu) failed\n",
	      sizeof(struct _XtransConnInfo));
	*status = TRANS_ACCEPT_BAD_MALLOC;
	return NULL;
    }

    newciptr->fd=transptr->devaccept(ciptr,newciptr,status);

    if( newciptr->fd < 0 )
    {
	free(newciptr);
	return NULL;
    }

    newciptr->priv=(char *)transptr;
    newciptr->index = ciptr->index;

    *status = 0;

    return newciptr;
}

static int
TRANS(LocalBytesReadable)(XtransConnInfo ciptr, BytesReadable_t *pend )

{
    prmsg(2,"LocalBytesReadable(%p->%d,%p)\n",
          (void *) ciptr, ciptr->fd, (void *) pend);

    return ioctl(ciptr->fd, FIONREAD, (char *)pend);
}

static int
TRANS(LocalRead)(XtransConnInfo ciptr, char *buf, int size)

{
    prmsg(2,"LocalRead(%d,%p,%d)\n", ciptr->fd, (void *) buf, size );

    return read(ciptr->fd,buf,size);
}

static int
TRANS(LocalWrite)(XtransConnInfo ciptr, const char *buf, int size)

{
    prmsg(2,"LocalWrite(%d,%p,%d)\n", ciptr->fd, (const void *) buf, size );

    return write(ciptr->fd,buf,size);
}

static int
TRANS(LocalReadv)(XtransConnInfo ciptr, struct iovec *buf, int size)

{
    prmsg(2,"LocalReadv(%d,%p,%d)\n", ciptr->fd, (void *) buf, size );

    return READV(ciptr,buf,size);
}

static int
TRANS(LocalWritev)(XtransConnInfo ciptr, struct iovec *buf, int size)

{
    prmsg(2,"LocalWritev(%d,%p,%d)\n", ciptr->fd, (const void *) buf, size );

    return WRITEV(ciptr,buf,size);
}

static int
TRANS(LocalDisconnect)(XtransConnInfo ciptr)

{
    prmsg(2,"LocalDisconnect(%p->%d)\n", (void *) ciptr, ciptr->fd);

    return 0;
}

static int
TRANS(LocalClose)(XtransConnInfo ciptr)

{
    struct sockaddr_un      *sockname=(struct sockaddr_un *) ciptr->addr;
    int	ret;

    prmsg(2,"LocalClose(%p->%d)\n", (void *) ciptr, ciptr->fd );

    ret=close(ciptr->fd);

    if(ciptr->flags
       && sockname
       && sockname->sun_family == AF_UNIX
       && sockname->sun_path[0] )
    {
	if (!(ciptr->flags & TRANS_NOUNLINK))
	    unlink(sockname->sun_path);
    }

    return ret;
}

static int
TRANS(LocalCloseForCloning)(XtransConnInfo ciptr)

{
    int ret;

    prmsg(2,"LocalCloseForCloning(%p->%d)\n", (void *) ciptr, ciptr->fd );

    /* Don't unlink path */

    ret=close(ciptr->fd);

    return ret;
}


/*
 * MakeAllCOTSServerListeners() will go through the entire Xtransports[]
 * array defined in Xtrans.c and try to OpenCOTSServer() for each entry.
 * We will add duplicate entries to that table so that the OpenCOTSServer()
 * function will get called once for each type of local transport.
 *
 * The TransName is in lowercase, so it will never match during a normal
 * call to SelectTransport() in Xtrans.c.
 */
static const char * local_aliases[] = {
				  "named",
				  "pipe", /* compatibility with Solaris Xlib */
				  NULL };

static Xtransport	TRANS(LocalFuncs) = {
	/* Local Interface */
	"local",
	TRANS_ALIAS | TRANS_LOCAL,
	local_aliases,
	TRANS(LocalOpenCOTSServer),
	TRANS(LocalReopenCOTSServer),
	TRANS(LocalSetOption),
	TRANS(LocalCreateListener),
	TRANS(LocalResetListener),
	TRANS(LocalAccept),
	TRANS(LocalBytesReadable),
	TRANS(LocalRead),
	TRANS(LocalWrite),
	TRANS(LocalReadv),
	TRANS(LocalWritev),
#if XTRANS_SEND_FDS
	TRANS(LocalSendFdInvalid),
	TRANS(LocalRecvFdInvalid),
#endif
	TRANS(LocalDisconnect),
	TRANS(LocalClose),
	TRANS(LocalCloseForCloning),
};


#ifdef LOCAL_TRANS_NAMED

static Xtransport	TRANS(NAMEDFuncs) = {
	/* Local Interface */
	"named",
	TRANS_LOCAL,
	NULL,
	TRANS(LocalOpenCOTSServer),
	TRANS(LocalReopenCOTSServer),
	TRANS(LocalSetOption),
	TRANS(LocalCreateListener),
	TRANS(LocalResetListener),
	TRANS(LocalAccept),
	TRANS(LocalBytesReadable),
	TRANS(LocalRead),
	TRANS(LocalWrite),
	TRANS(LocalReadv),
	TRANS(LocalWritev),
#if XTRANS_SEND_FDS
	TRANS(LocalSendFdInvalid),
	TRANS(LocalRecvFdInvalid),
#endif
	TRANS(LocalDisconnect),
	TRANS(LocalClose),
	TRANS(LocalCloseForCloning),
};

static Xtransport	TRANS(PIPEFuncs) = {
	/* Local Interface */
	"pipe",
	TRANS_ALIAS | TRANS_LOCAL,
	NULL,
	TRANS(LocalOpenCOTSServer),
	TRANS(LocalReopenCOTSServer),
	TRANS(LocalSetOption),
	TRANS(LocalCreateListener),
	TRANS(LocalResetListener),
	TRANS(LocalAccept),
	TRANS(LocalBytesReadable),
	TRANS(LocalRead),
	TRANS(LocalWrite),
	TRANS(LocalReadv),
	TRANS(LocalWritev),
#if XTRANS_SEND_FDS
	TRANS(LocalSendFdInvalid),
	TRANS(LocalRecvFdInvalid),
#endif
	TRANS(LocalDisconnect),
	TRANS(LocalClose),
	TRANS(LocalCloseForCloning),
};
#endif /* LOCAL_TRANS_NAMED */

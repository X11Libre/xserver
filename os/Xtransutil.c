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
 * NCRS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN
 * NO EVENT SHALL NCR BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/*
 * These are some utility functions created for convenience or to provide
 * an interface that is similar to an existing interface. These are built
 * only using the Transport Independent API, and have no knowledge of
 * the internal implementation.
 */

#ifdef XTHREADS
#include <X11/Xthreads.h>
#endif
#ifdef WIN32
#include <X11/Xlibint.h>
#include <X11/Xwinsock.h>
#endif

#if defined(IPv6) && !defined(AF_INET6)
#error "Cannot build IPv6 support without AF_INET6"
#endif

/* Temporary workaround for consumers whose configure scripts were
   generated with pre-1.6 versions of xtrans.m4 */
#if defined(IPv6) && !defined(HAVE_INET_NTOP)
#define HAVE_INET_NTOP
#endif

/*
 * These values come from X.h and Xauth.h, and MUST match them. Some
 * of these values are also defined by the ChangeHost protocol message.
 */

#define FamilyInternet		0	/* IPv4 */
#define FamilyDECnet		1
#define FamilyChaos		2
#define FamilyInternet6		6
#define FamilyAmoeba		33
#define FamilyLocalHost		252
#define FamilyKrb5Principal	253
#define FamilyNetname		254
#define FamilyLocal		256
#define FamilyWild		65535

/*
 * _XSERVTransConvertAddress converts a sockaddr based address to an
 * X authorization based address. Some of this is defined as part of
 * the ChangeHost protocol. The rest is just done in a consistent manner.
 */

int _XSERVTransConvertAddress(int *familyp, int *addrlenp, Xtransaddr **addrp)
{
    prmsg(2,"ConvertAddress(%d,%d,%p)\n",*familyp,*addrlenp,*addrp);

    switch( *familyp )
    {
#if defined(TCPCONN)
    case AF_INET:
    {
	/*
	 * Check for the BSD hack localhost address 127.0.0.1.
	 * In this case, we are really FamilyLocal.
	 */

	struct sockaddr_in saddr;
	int len = sizeof(saddr.sin_addr.s_addr);
	char *cp = (char *) &saddr.sin_addr.s_addr;

	memcpy (&saddr, *addrp, sizeof (struct sockaddr_in));

	if ((len == 4) && (cp[0] == 127) && (cp[1] == 0) &&
	    (cp[2] == 0) && (cp[3] == 1))
	{
	    *familyp=FamilyLocal;
	}
	else
	{
	    *familyp=FamilyInternet;
	    *addrlenp=len;
	    memcpy(*addrp,&saddr.sin_addr,len);
	}
	break;
    }

#ifdef IPv6
    case AF_INET6:
    {
	struct sockaddr_in6 saddr6;

	memcpy (&saddr6, *addrp, sizeof (struct sockaddr_in6));

	if (IN6_IS_ADDR_LOOPBACK(&saddr6.sin6_addr))
	{
	    *familyp=FamilyLocal;
	}
	else if (IN6_IS_ADDR_V4MAPPED(&(saddr6.sin6_addr))) {
	    char *cp = (char *) &saddr6.sin6_addr.s6_addr[12];

	    if ((cp[0] == 127) && (cp[1] == 0) &&
	      (cp[2] == 0) && (cp[3] == 1))
	    {
		*familyp=FamilyLocal;
	    }
	    else
	    {
		*familyp=FamilyInternet;
		*addrlenp = sizeof (struct in_addr);
		memcpy(*addrp,cp,*addrlenp);
	    }
	}
	else
	{
	    *familyp=FamilyInternet6;
	    *addrlenp=sizeof(saddr6.sin6_addr);
	    memcpy(*addrp,&saddr6.sin6_addr,sizeof(saddr6.sin6_addr));
	}
	break;
    }
#endif /* IPv6 */
#endif /* defined(TCPCONN) */


#if defined(UNIXCONN) || defined(LOCALCONN)
    case AF_UNIX:
    {
	*familyp=FamilyLocal;
	break;
    }
#endif /* defined(UNIXCONN) || defined(LOCALCONN) */


    default:
	prmsg(1,"ConvertAddress: Unknown family type %d\n",
	      *familyp);
	return -1;
    }


    if (*familyp == FamilyLocal)
    {
	/*
	 * In the case of a local connection, we need to get the
	 * host name for authentication.
	 */

	char hostnamebuf[256];
	int len = _XSERVTransGetHostname (hostnamebuf, sizeof hostnamebuf);

	if (len > 0) {
	    if (*addrp && *addrlenp < (len + 1))
	    {
		free (*addrp);
		*addrp = NULL;
	    }
	    if (!*addrp)
		*addrp = malloc (len + 1);
	    if (*addrp) {
		strcpy ((char *) *addrp, hostnamebuf);
		*addrlenp = len;
	    } else {
		*addrlenp = 0;
	    }
	}
	else
	{
	    if (*addrp)
		free (*addrp);
	    *addrp = NULL;
	    *addrlenp = 0;
	}
    }

    return 0;
}

#if defined(WIN32) && defined(TCPCONN)
int _XSERVTransWSAStartup (void)
{
    static WSADATA wsadata;

    prmsg (2,"WSAStartup()\n");

    if (!wsadata.wVersion && WSAStartup(MAKEWORD(2,2), &wsadata))
        return 1;
    return 0;
}
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

#if !defined(S_IFLNK) && !defined(S_ISLNK)
#undef lstat
#define lstat(a,b) stat(a,b)
#endif

#define FAIL_IF_NOMODE  1
#define FAIL_IF_NOT_ROOT 2
#define WARN_NO_ACCESS 4

/*
 * We make the assumption that when the 'sticky' (t) bit is requested
 * it's not save if the directory has non-root ownership or the sticky
 * bit cannot be set and fail.
 */
int trans_mkdir(const char *path, int mode)
{
    struct stat buf;

    if (lstat(path, &buf) != 0) {
	if (errno != ENOENT) {
	    prmsg(1, "mkdir: ERROR: (l)stat failed for %s (%d)\n",
		  path, errno);
	    return -1;
	}
	/* Dir doesn't exist. Try to create it */

#if !defined(WIN32) && !defined(__CYGWIN__)
	/*
	 * 'sticky' bit requested: assume application makes
	 * certain security implications. If effective user ID
	 * is != 0: fail as we may not be able to meet them.
	 */
	if (geteuid() != 0) {
	    if (mode & 01000) {
		prmsg(1, "mkdir: ERROR: euid != 0,"
		      "directory %s will not be created.\n",
		      path);
#ifdef FAIL_HARD
		return -1;
#endif
	    } else {
		prmsg(1, "mkdir: Cannot create %s with root ownership\n",
		      path);
	    }
	}
#endif

#ifndef WIN32
	if (mkdir(path, mode) == 0) {
	    if (chmod(path, mode)) {
		prmsg(1, "mkdir: ERROR: Mode of %s should be set to %04o\n",
		      path, mode);
#ifdef FAIL_HARD
		return -1;
#endif
	    }
#else
	if (mkdir(path) == 0) {
#endif
	} else {
	    prmsg(1, "mkdir: ERROR: Cannot create %s\n",
		  path);
	    return -1;
	}

	return 0;

    } else {
	if (S_ISDIR(buf.st_mode)) {
	    int updateOwner = 0;
	    int updateMode = 0;
	    int updatedOwner = 0;
	    int updatedMode = 0;
	    int status = 0;
	    /* Check if the directory's ownership is OK. */
	    if (buf.st_uid != 0)
		updateOwner = 1;

	    /*
	     * Check if the directory's mode is OK.  An exact match isn't
	     * required, just a mode that isn't more permissive than the
	     * one requested.
	     */
	    if ((~mode) & 0077 & buf.st_mode)
		updateMode = 1;

	    /*
	     * If the directory is not writeable not everybody may
	     * be able to create sockets. Therefore warn if mode
	     * cannot be fixed.
	     */
	    if ((~buf.st_mode) & 0022 & mode) {
		updateMode = 1;
		status |= WARN_NO_ACCESS;
	    }

	    /*
	     * If 'sticky' bit is requested fail if owner isn't root
	     * as we assume the caller makes certain security implications
	     */
	    if (mode & 01000) {
		status |= FAIL_IF_NOT_ROOT;
		if (!(buf.st_mode & 01000)) {
		    status |= FAIL_IF_NOMODE;
		    updateMode = 1;
		}
	    }

#ifdef HAS_FCHOWN
	    /*
	     * If fchown(2) and fchmod(2) are available, try to correct the
	     * directory's owner and mode.  Otherwise it isn't safe to attempt
	     * to do this.
	     */
	    if (updateMode || updateOwner) {
		int fd = -1;
		struct stat fbuf;
		if ((fd = open(path, O_RDONLY)) != -1) {
		    if (fstat(fd, &fbuf) == -1) {
			prmsg(1, "mkdir: ERROR: fstat failed for %s (%d)\n",
			      path, errno);
			close(fd);
			return -1;
		    }
		    /*
		     * Verify that we've opened the same directory as was
		     * checked above.
		     */
		    if (!S_ISDIR(fbuf.st_mode) ||
			buf.st_dev != fbuf.st_dev ||
			buf.st_ino != fbuf.st_ino) {
			prmsg(1, "mkdir: ERROR: inode for %s changed\n",
			      path);
			close(fd);
			return -1;
		    }
		    if (updateOwner && fchown(fd, 0, 0) == 0)
			updatedOwner = 1;
		    if (updateMode && fchmod(fd, mode) == 0)
			updatedMode = 1;
		    close(fd);
		}
	    }
#endif

	    if (updateOwner && !updatedOwner) {
#ifdef FAIL_HARD
		if (status & FAIL_IF_NOT_ROOT) {
		    prmsg(1, "mkdir: ERROR: Owner of %s must be set to root\n",
			  path);
		    return -1;
		}
#endif
#if !defined(__APPLE_CC__) && !defined(__CYGWIN__)
		prmsg(1, "mkdir: Owner of %s should be set to root\n",
		      path);
#endif
	    }

	    if (updateMode && !updatedMode) {
#ifdef FAIL_HARD
		if (status & FAIL_IF_NOMODE) {
		    prmsg(1, "mkdir: ERROR: Mode of %s must be set to %04o\n",
			  path, mode);
		    return -1;
		}
#endif
		prmsg(1, "mkdir: Mode of %s should be set to %04o\n",
		      path, mode);
		if (status & WARN_NO_ACCESS) {
		    prmsg(1, "mkdir: this may cause subsequent errors\n");
		}
	    }
	    return 0;
	}
	return -1;
    }

    /* In all other cases, fail */
    return -1;
}

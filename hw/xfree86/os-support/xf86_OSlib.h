/*
 * Copyright 1990, 1991 by Thomas Roell, Dinkelscherben, Germany
 * Copyright 1992 by David Dawes <dawes@XFree86.org>
 * Copyright 1992 by Jim Tsillas <jtsilla@damon.ccs.northeastern.edu>
 * Copyright 1992 by Rich Murphey <Rich@Rice.edu>
 * Copyright 1992 by Robert Baron <Robert.Baron@ernst.mach.cs.cmu.edu>
 * Copyright 1992 by Orest Zborowski <obz@eskimo.com>
 * Copyright 1993 by Vrije Universiteit, The Netherlands
 * Copyright 1993 by David Wexelblat <dwex@XFree86.org>
 * Copyright 1994, 1996 by Holger Veit <Holger.Veit@gmd.de>
 * Copyright 1997 by Takis Psarogiannakopoulos <takis@dpmms.cam.ac.uk>
 * Copyright 1994-2003 by The XFree86 Project, Inc
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the names of the above listed copyright holders
 * not be used in advertising or publicity pertaining to distribution of
 * the software without specific, written prior permission.  The above listed
 * copyright holders make no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without express or
 * implied warranty.
 *
 * THE ABOVE LISTED COPYRIGHT HOLDERS DISCLAIM ALL WARRANTIES WITH REGARD
 * TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS, IN NO EVENT SHALL THE ABOVE LISTED COPYRIGHT HOLDERS BE
 * LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY
 * DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER
 * IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING
 * OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 */

/*
 * The ARM32 code here carries the following copyright:
 *
 * Copyright 1997
 * Digital Equipment Corporation. All rights reserved.
 * This software is furnished under license and may be used and copied only in
 * accordance with the following terms and conditions.  Subject to these
 * conditions, you may download, copy, install, use, modify and distribute
 * this software in source and/or binary form. No title or ownership is
 * transferred hereby.
 *
 * 1) Any source code used, modified or distributed must reproduce and retain
 *    this copyright notice and list of conditions as they appear in the
 *    source file.
 *
 * 2) No right is granted to use any trade name, trademark, or logo of Digital
 *    Equipment Corporation. Neither the "Digital Equipment Corporation"
 *    name nor any trademark or logo of Digital Equipment Corporation may be
 *    used to endorse or promote products derived from this software without
 *    the prior written permission of Digital Equipment Corporation.
 *
 * 3) This software is provided "AS-IS" and any express or implied warranties,
 *    including but not limited to, any implied warranties of merchantability,
 *    fitness for a particular purpose, or non-infringement are disclaimed.
 *    In no event shall DIGITAL be liable for any damages whatsoever, and in
 *    particular, DIGITAL shall not be liable for special, indirect,
 *    consequential, or incidental damages or damages for lost profits, loss
 *    of revenue or loss of use, whether such damages arise in contract,
 *    negligence, tort, under statute, in equity, at law or otherwise, even
 *    if advised of the possibility of such damage.
 *
 */

/*
 * This is private, and should not be included by any drivers.  Drivers
 * may include xf86_OSproc.h to get prototypes for public interfaces.
 */

#ifndef _XF86_OSLIB_H
#define _XF86_OSLIB_H

#include <X11/Xos.h>
#include <X11/Xfuncproto.h>

#include <stdio.h>
#include <ctype.h>
#include <stddef.h>

/**************************************************************************/
/* Solaris or illumos-based system                                        */
/**************************************************************************/
#if defined(__SVR4) && defined(__sun)
#include <sys/ioctl.h>
#include <signal.h>
#include <termio.h>
#include <sys/types.h>

#include <errno.h>

#ifdef HAVE_SYS_VT_H
#define HAS_USL_VTS
#endif
#ifdef HAS_USL_VTS
#include <sys/kd.h>
#include <sys/vt.h>
#endif

#define CLEARDTR_SUPPORT

#endif                          /* SVR4 && __sun */

/**************************************************************************/
/* Linux or Glibc-based system                                            */
/**************************************************************************/
#if defined(__linux__) || defined(__GLIBC__) || defined(__CYGWIN__)
#include <sys/ioctl.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/types.h>
#include <assert.h>

#include <termios.h>
#ifdef __sparc__
#include <sys/param.h>
#endif

#include <errno.h>

#ifdef __linux__
#define HAS_USL_VTS
#include <sys/kd.h>
#include <sys/vt.h>
#define LDGMAP GIO_SCRNMAP
#define LDSMAP PIO_SCRNMAP
#define LDNMAP LDSMAP
#define CLEARDTR_SUPPORT
#endif

#endif                          /* __linux__ || __GLIBC__ */

/**************************************************************************/
/* System is BSD-like                                                     */
/**************************************************************************/

#ifdef CSRG_BASED
#include <sys/ioctl.h>
#include <signal.h>

#include <termios.h>
#define termio termios

#include <errno.h>

#include <sys/types.h>

#endif                          /* CSRG_BASED */

/**************************************************************************/
/* Kernel of *BSD                                                         */
/**************************************************************************/
#if defined(__FreeBSD__) || defined(__FreeBSD_kernel__) || \
 defined(__NetBSD__) || defined(__OpenBSD__) || defined(__DragonFly__)

#include <sys/param.h>
#if defined(__FreeBSD_version) && !defined(__FreeBSD_kernel_version)
#define __FreeBSD_kernel_version __FreeBSD_version
#endif

#ifdef SYSCONS_SUPPORT
#define COMPAT_SYSCONS
#if defined(__FreeBSD__) || defined(__FreeBSD_kernel__) || defined(__DragonFly__)
#if defined(__DragonFly__)  || (__FreeBSD_kernel_version >= 410000)
#include <sys/consio.h>
#include <sys/kbio.h>
#else
#include <machine/console.h>
#endif                          /* FreeBSD 4.1 RELEASE or lator */
#else
#include <sys/console.h>
#endif
#endif                          /* SYSCONS_SUPPORT */
#if defined(PCVT_SUPPORT) && !defined(__NetBSD__) && !defined(__OpenBSD__)
#if !defined(SYSCONS_SUPPORT)
      /* no syscons, so include pcvt specific header file */
#if defined(__FreeBSD__) || defined(__FreeBSD_kernel__)
#include <machine/pcvt_ioctl.h>
#else
#include <sys/pcvt_ioctl.h>
#endif                          /* __FreeBSD_kernel__ */
#else                           /* pcvt and syscons: hard-code the ID magic */
#define VGAPCVTID _IOWR('V',113, struct pcvtid)
struct pcvtid {
    char name[16];
    int rmajor, rminor;
};
#endif                          /* PCVT_SUPPORT && SYSCONS_SUPPORT */
#endif                          /* PCVT_SUPPORT */
#ifdef WSCONS_SUPPORT
#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wsdisplay_usl_io.h>
#endif                          /* WSCONS_SUPPORT */
#if defined(__FreeBSD__) || defined(__FreeBSD_kernel__) || defined(__DragonFly__)
#include <sys/mouse.h>
#endif
    /* Include these definitions in case ioctl_pc.h didn't get included */
#ifndef CONSOLE_X_BELL
#define CONSOLE_X_BELL _IOW('t',123,int[2])
#endif

#define CLEARDTR_SUPPORT

#endif                          /* __FreeBSD__ || __NetBSD__ || __OpenBSD__ || __DragonFly__ */

/**************************************************************************/
/* IRIX                                                                   */
/**************************************************************************/

/**************************************************************************/
/* Generic                                                                */
/**************************************************************************/

/* For PATH_MAX */
#include "misc.h"

/*
 * Hack originally for ISC 2.2 POSIX headers, but may apply elsewhere,
 * and it's safe, so just do it.
 */
#if !defined(O_NDELAY) && defined(O_NONBLOCK)
#define O_NDELAY O_NONBLOCK
#endif                          /* !O_NDELAY && O_NONBLOCK */

#if !defined(MAXHOSTNAMELEN)
#define MAXHOSTNAMELEN 32
#endif                          /* !MAXHOSTNAMELEN */

#include <limits.h>

#ifndef MAP_FAILED
#define MAP_FAILED ((void *)-1)
#endif

#define SYSCALL(call) while(((call) == -1) && (errno == EINTR))

#include "xf86_OSproc.h"

#endif                          /* _XF86_OSLIB_H */

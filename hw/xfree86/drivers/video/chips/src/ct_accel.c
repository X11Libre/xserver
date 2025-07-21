/*
 * Copyright 1996, 1997, 1998 by David Bateman <dbateman@ee.uts.edu.au>
 *   Modified 1997, 1998 by Nozomi Ytow
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of the authors not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  The authors makes no representations
 * about the suitability of this software for any purpose.  It is provided
 * "as is" without express or implied warranty.
 *
 * THE AUTHORS DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL THE AUTHORS BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */
#include <dix-config.h>

/*
 * When monochrome tiles/stipples are cached on the HiQV chipsets the
 * pitch of the monochrome data is the displayWidth. The HiQV manuals
 * state that the source pitch is ignored with monochrome data, and so
 * "officially" there the XAA cached monochrome data can't be used. But
 * it appears that by not setting the monochrome source alignment in
 * BR03, the monochrome source pitch is forced to the displayWidth!!
 *
 * To enable the use of this undocumented feature, uncomment the define
 * below.
 */
#define UNDOCUMENTED_FEATURE

/* All drivers should typically include these */
#include "xf86.h"
#include "xf86_OSproc.h"
#include "compiler.h"

/* Drivers that need to access the PCI config space directly need this */
#include "xf86Pci.h"

/* Drivers that use XAA need this */
#include "xf86fbman.h"

/* Our driver specific include file */
#include "ct_driver.h"

#define CATNAME(prefix,subname) prefix##subname

#ifdef CHIPS_MMIO
#ifdef CHIPS_HIQV
#include "ct_BltHiQV.h"
#define CTNAME(subname) CATNAME(CHIPSHiQV,subname)
#else
#include "ct_BlitMM.h"
#define CTNAME(subname) CATNAME(CHIPSMMIO,subname)
#endif
#else
#include "ct_Blitter.h"
#define CTNAME(subname) CATNAME(CHIPS,subname)
#endif

Bool 
CTNAME(AccelInit)(ScreenPtr pScreen)
{
    return FALSE;
}

void
CTNAME(Sync)(ScrnInfoPtr pScrn)
{
}


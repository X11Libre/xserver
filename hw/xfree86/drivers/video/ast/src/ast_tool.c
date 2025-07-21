/*
 * Copyright (c) 2005 ASPEED Technology Inc.
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "xf86.h"
#include "xf86_OSproc.h"
#include "xf86cmap.h"
#include "compiler.h"
#include "vgaHW.h"
#include "mipointer.h"
#include "micmap.h"

#include "fb.h"
#include "regionstr.h"
#include "xf86xv.h"
#include <X11/extensions/Xv.h>

#include "xf86Pci.h"

/* framebuffer offscreen manager */
#include "xf86fbman.h"

/* include xaa includes */
#include "xaarop.h"

/* H/W cursor support */
#include "xf86Cursor.h"

/* Driver specific headers */
#include "ast.h"

Bool
ASTMapMem(ScrnInfoPtr pScrn)
{
   ASTRecPtr pAST = ASTPTR(pScrn);

#ifndef XSERVER_LIBPCIACCESS
   pAST->FBVirtualAddr = xf86MapPciMem(pScrn->scrnIndex, VIDMEM_FRAMEBUFFER,
				 pAST->PciTag,
				 pAST->FBPhysAddr, pAST->FbMapSize);
#else
   {
     void** result = (void**)&pAST->FBVirtualAddr;
     int err = pci_device_map_range(pAST->PciInfo,
				    pAST->FBPhysAddr,
				    pAST->FbMapSize,
				    PCI_DEV_MAP_FLAG_WRITABLE |
				    PCI_DEV_MAP_FLAG_WRITE_COMBINE,
				    result);

     if (err)
			return FALSE;
   }
#endif

   if (!pAST->FBVirtualAddr)
      return FALSE;

   return TRUE;
}

Bool
ASTUnmapMem(ScrnInfoPtr pScrn)
{
   ASTRecPtr pAST = ASTPTR(pScrn);

#ifndef XSERVER_LIBPCIACCESS
   xf86UnMapVidMem(pScrn->scrnIndex, (pointer) pAST->FBVirtualAddr,
		   pAST->FbMapSize);
#else
   pci_device_unmap_range(pAST->PciInfo, pAST->FBVirtualAddr, pAST->FbMapSize);
#endif

   pAST->FBVirtualAddr = 0;

   return TRUE;
}

Bool
ASTMapMMIO(ScrnInfoPtr pScrn)
{
   ASTRecPtr pAST = ASTPTR(pScrn);
#ifndef XSERVER_LIBPCIACCESS
   int mmioFlags;

#if !defined(__alpha__)
   mmioFlags = VIDMEM_MMIO | VIDMEM_READSIDEEFFECT;
#else
   mmioFlags = VIDMEM_MMIO | VIDMEM_READSIDEEFFECT | VIDMEM_SPARSE;
#endif


   pAST->MMIOVirtualAddr = xf86MapPciMem(pScrn->scrnIndex, mmioFlags,
				         pAST->PciTag,
				         pAST->MMIOPhysAddr, pAST->MMIOMapSize);

#else
   {
     void** result = (void**)&pAST->MMIOVirtualAddr;
     int err = pci_device_map_range(pAST->PciInfo,
				    pAST->MMIOPhysAddr,
				    pAST->MMIOMapSize,
				    PCI_DEV_MAP_FLAG_WRITABLE,
				    result);

     if (err)
			return FALSE;
   }

#endif
   if (!pAST->MMIOVirtualAddr)
      return FALSE;

   return TRUE;
}

void
ASTUnmapMMIO(ScrnInfoPtr pScrn)
{
   ASTRecPtr pAST = ASTPTR(pScrn);

#ifndef XSERVER_LIBPCIACCESS
   xf86UnMapVidMem(pScrn->scrnIndex, (pointer) pAST->MMIOVirtualAddr,
		   pAST->MMIOMapSize);
#else
   pci_device_unmap_range(pAST->PciInfo, pAST->MMIOVirtualAddr, pAST->MMIOMapSize);
#endif
   pAST->MMIOVirtualAddr = 0;

}
 


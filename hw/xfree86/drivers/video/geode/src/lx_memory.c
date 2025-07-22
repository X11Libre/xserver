/* Copyright (c) 2008 Advanced Micro Devices, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *
 * Neither the name of the Advanced Micro Devices, Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from this
 * software without specific prior written permission.
 */
#if HAVE_CONFIG_H
#include "config.h"
#endif

#include "xf86.h"
#include "geode.h"
#include "cim/cim_regs.h"

#define ALIGN(x,y)   (((x) + (y) - 1) / (y) * (y))
#define LX_CB_PITCH   544

/* Geode offscreen memory allocation functions.  This is
   overengineered for the simple hardware that we have, but
   there are multiple functions that may want to independently
   allocate and free memory (crtc->shadow_alloc and Xv).  This
   provides a semi-robust mechanism for doing that.
*/

/* Return the number of free bytes */

unsigned int
GeodeOffscreenFreeSize(GeodeRec *pGeode)
{
    GeodeMemPtr ptr = pGeode->offscreenList;

    if (ptr == NULL)
        return pGeode->offscreenSize;

    for (; ptr->next; ptr = ptr->next);
    return (pGeode->offscreenStart + pGeode->offscreenSize)
        - (ptr->offset + ptr->size);
}

void
GeodeFreeOffscreen(GeodeRec *pGeode, GeodeMemPtr ptr)
{
    /* There is a clear memory leak here, but
     * but it is unlikely that the first block of
     * "allocated" memory is going to be released
     * individually.
     */

    if (ptr->prev == NULL)
        pGeode->offscreenList = ptr->next;
    else
        ptr->prev->next = ptr->next;

    if (ptr->next)
        ptr->next->prev = ptr->prev;

    free(ptr);
}

/* Allocate the "rest" of the offscreen memory - this is for
   situations where we have very little video memory, and we
   want to take as much of it as we can for EXA
*/

static GeodeMemPtr
GeodeAllocRemainder(GeodeRec *pGeode)
{
    GeodeMemPtr nptr, ptr = pGeode->offscreenList;

    if (!pGeode->offscreenList) {
        pGeode->offscreenList = calloc(1, sizeof(*nptr));
        pGeode->offscreenList->offset = pGeode->offscreenStart;
        pGeode->offscreenList->size = pGeode->offscreenSize;
        pGeode->offscreenList->next = NULL;
        pGeode->offscreenList->prev = NULL;

        return pGeode->offscreenList;
    }

    /* Go to the end of the list of allocated stuff */
    for (; ptr->next; ptr = ptr->next);

    nptr = calloc(1, sizeof(*nptr));
    nptr->offset = ptr->offset + ptr->size;
    nptr->size = pGeode->offscreenSize -
        (nptr->offset - pGeode->offscreenStart);

    nptr->next = ptr->next;
    nptr->prev = ptr;
    ptr->next = nptr;

    return nptr;
}

/* Allocate 'size' bytes of offscreen memory.
*/

GeodeMemPtr
GeodeAllocOffscreen(GeodeRec *pGeode, int size, int align)
{
    GeodeMemPtr ptr = pGeode->offscreenList;
    GeodeMemPtr nptr;

    unsigned int offset;

    if (!pGeode->offscreenList) {

        if (size > pGeode->offscreenSize)
            return NULL;

        offset = ALIGN(pGeode->offscreenStart, align);

        pGeode->offscreenList = calloc(1, sizeof(*nptr));
        pGeode->offscreenList->offset = offset;
        pGeode->offscreenList->size = size;
        pGeode->offscreenList->next = NULL;

        return pGeode->offscreenList;
    }

    while (ptr) {
        unsigned int gap;

        if (ptr->next == NULL)
            gap = pGeode->offscreenSize + pGeode->offscreenStart;

        else
            gap = ptr->next->offset;

        gap = gap - (ptr->offset + ptr->size);
        gap = ALIGN(gap, align);

        if (size < gap) {
            offset = ptr->offset + ptr->size;
            offset = ALIGN(ptr->offset + ptr->size, align);

            nptr = calloc(1, sizeof(*nptr));
            nptr->offset = offset;
            nptr->size = size;
            nptr->next = ptr->next;
            nptr->prev = ptr;
            ptr->next = nptr;

            return nptr;
        }

        ptr = ptr->next;
    }

    return NULL;
}

/* Carve out the space for the visible screen, and carve out
   the usual suspects that need offscreen memory
*/

#define MAX(a,b) ((a) > (b) ? (a) : (b))

void
LXInitOffscreen(ScrnInfoPtr pScrni)
{
    GeodeRec *pGeode = GEODEPTR(pScrni);
    unsigned int fbavail;
    GeodeMemPtr ptr;

    /* The scratch buffer is always used */
    fbavail = pGeode->FBAvail - GP3_SCRATCH_BUFFER_SIZE;

    pGeode->displaySize = MAX(pScrni->virtualX, pScrni->virtualY)
        * pGeode->Pitch;

    pGeode->offscreenStart = pGeode->displaySize;
    pGeode->offscreenSize = fbavail - pGeode->displaySize;

    /* Allocate the usual memory suspects */
    if (pGeode->tryCompression) {
        int size = pScrni->virtualY * LX_CB_PITCH;

        /* The compression buffer needs to be 16 byte aligned */
        ptr = GeodeAllocOffscreen(pGeode, size, 16);

        if (ptr != NULL) {
            pGeode->CBData.compression_offset = ptr->offset;
            pGeode->CBData.size = LX_CB_PITCH;
            pGeode->CBData.pitch = LX_CB_PITCH;

            pGeode->Compression = TRUE;
        }
        else {
            xf86DrvMsg(pScrni->scrnIndex, X_ERROR,
                       "Not enough memory for compression\n");
            pGeode->Compression = FALSE;
        }
    }

    if (pGeode->tryHWCursor) {
        ptr = GeodeAllocOffscreen(pGeode,
                                  LX_CURSOR_HW_WIDTH * 4 * LX_CURSOR_HW_HEIGHT,
                                  4);

        if (ptr != NULL) {
            pGeode->CursorStartOffset = ptr->offset;
            pGeode->HWCursor = TRUE;
        }
        else {
            xf86DrvMsg(pScrni->scrnIndex, X_ERROR,
                       "Not enough memory for the hardware cursor\n");
            pGeode->HWCursor = FALSE;
        }
    }

    if (!pGeode->NoAccel && pGeode->pExa) {
        int size;

        /* Try to get the scratch buffer for blending */
        pGeode->exaBfrOffset = 0;

        if (pGeode->exaBfrSz > 0) {
            ptr = GeodeAllocOffscreen(pGeode, pGeode->exaBfrSz, 4);
            if (ptr != NULL)
                pGeode->exaBfrOffset = ptr->offset;
        }

        pGeode->pExa->offScreenBase = 0;
        pGeode->pExa->memorySize = 0;

        /* This might cause complaints - in order to avoid using
           xorg.conf as much as possible, we make assumptions about
           what a "default" memory map would look like.  After
           discussion, we agreed that the default driver should assume
           the user will want to use rotation and video overlays, and
           EXA will get whatever is leftover. 
         */

        /* Get the amount of offscreen memory still left */
        size = GeodeOffscreenFreeSize(pGeode);

        /* Align the size to a K boundary */
        size &= ~1023;

        /* Allocate the EXA offscreen space */
        ptr = GeodeAllocOffscreen(pGeode, size, 4);

        if (ptr == NULL) {
            /* If we couldn't allocate what we wanted,
             * then allocate what's left */

            ptr = GeodeAllocRemainder(pGeode);
        }

        if (ptr != NULL) {
            pGeode->pExa->offScreenBase = ptr->offset;
            pGeode->pExa->memorySize = ptr->offset + ptr->size;
        }
    }

    /* Show the memory map for diagnostic purposes */

    xf86DrvMsg(pScrni->scrnIndex, X_INFO, "LX video memory:\n");
    xf86DrvMsg(pScrni->scrnIndex, X_INFO, " Display: 0x%x bytes\n",
               pGeode->displaySize);

    if (pGeode->Compression)
        xf86DrvMsg(pScrni->scrnIndex, X_INFO, " Compression: 0x%x bytes\n",
                   pScrni->virtualY * LX_CB_PITCH);

    if (pGeode->HWCursor)
        xf86DrvMsg(pScrni->scrnIndex, X_INFO, " Cursor: 0x%x bytes\n",
                   LX_CURSOR_HW_WIDTH * 4 * LX_CURSOR_HW_HEIGHT);

    if (pGeode->exaBfrSz)
        xf86DrvMsg(pScrni->scrnIndex, X_INFO, " ExaBfrSz: 0x%x bytes\n",
                   pGeode->exaBfrSz);

    if (pGeode->pExa && pGeode->pExa->offScreenBase)
        xf86DrvMsg(pScrni->scrnIndex, X_INFO, " EXA: 0x%x bytes\n",
                   (unsigned int) (pGeode->pExa->memorySize -
                                   pGeode->pExa->offScreenBase));

    xf86DrvMsg(pScrni->scrnIndex, X_INFO, " FREE: 0x%x bytes\n",
               GeodeOffscreenFreeSize(pGeode));
}

/* Called as we go down, so blitz everybody */

void
GeodeCloseOffscreen(ScrnInfoPtr pScrni)
{
    GeodeRec *pGeode = GEODEPTR(pScrni);
    GeodeMemPtr ptr = pGeode->offscreenList;
    GeodeMemPtr nptr;

    while (ptr) {
        nptr = ptr->next;
        free(ptr);
        ptr = nptr;
    }

    pGeode->offscreenList = NULL;
}

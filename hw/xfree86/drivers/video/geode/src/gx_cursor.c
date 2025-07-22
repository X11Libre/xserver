/* Copyright (c) 2003-2006 Advanced Micro Devices, Inc.
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
 * */

/*
 * File Contents:   Xfree cursor implementation routines for geode HWcursor
 *                  init.setting cursor color,image etc. are done here.
 *
 * Project:         Geode Xfree Frame buffer device driver.
 * */
#include <xorg-config.h>

#include "xf86.h"
#include "xf86_OSproc.h"
#include "xf86Pci.h"
#include "geode.h"

/* Forward declarations of the functions */
static void GXSetCursorColors(ScrnInfoPtr pScrni, int bg, int fg);
static void GXSetCursorPosition(ScrnInfoPtr pScrni, int x, int y);
static Bool GXUseHWCursor(ScreenPtr pScrn, CursorPtr pCurs);
extern void GXSetVideoPosition(int x, int y, int width, int height,
                               short src_w, short src_h, short drw_w,
                               short drw_h, int id, int offset,
                               ScrnInfoPtr pScrn);

/*----------------------------------------------------------------------------
 * GXHWCursorInit.
 *
 * Description	:This function sets the cursor information by probing the
 * 				hardware.
 *
 * Parameters.
 *     pScrn	:Screeen pointer structure.
 *
 * Returns		:TRUE on success and FALSE on Failure
 *
 * Comments		:Geode supports the hardware_cursor,no need to enable SW
 *              cursor.
 *----------------------------------------------------------------------------
 */
Bool
GXHWCursorInit(ScreenPtr pScrn)
{
    ScrnInfoPtr pScrni = xf86ScreenToScrn(pScrn);
    GeodeRec *pGeode = GEODEPTR(pScrni);
    xf86CursorInfoPtr infoPtr;

    infoPtr = xf86CreateCursorInfoRec();
    if (!infoPtr)
        return FALSE;
    /* the geode structure is initialized with the cursor infoRec */
    pGeode->CursorInfo = infoPtr;
    infoPtr->MaxWidth = 32;
    infoPtr->MaxHeight = 32;
    /* setting up the cursor flags */
    infoPtr->Flags = HARDWARE_CURSOR_BIT_ORDER_MSBFIRST |
        HARDWARE_CURSOR_TRUECOLOR_AT_8BPP |
        HARDWARE_CURSOR_SOURCE_MASK_NOT_INTERLEAVED;
    /* cursor info ptr is initialized with the values obtained from
     * * durnago calls
     */
    infoPtr->SetCursorColors = GXSetCursorColors;
    infoPtr->SetCursorPosition = GXSetCursorPosition;
    infoPtr->LoadCursorImage = GXLoadCursorImage;
    infoPtr->HideCursor = GXHideCursor;
    infoPtr->ShowCursor = GXShowCursor;
    infoPtr->UseHWCursor = GXUseHWCursor;
    return (xf86InitCursor(pScrn, infoPtr));
}

/*----------------------------------------------------------------------------
 * GXSetCursorColors.
 *
 * Description	:This function sets the cursor foreground and background
 *              colors
 * Parameters:
 *    pScrn:	Screeen pointer structure.
 *    bg:		Specifies the color value of cursor background color.
 *    fg:		Specifies the color value of cursor foreground color.
 *
 * Returns:	none.
 *
 * Comments:	The integer color value passed by this function is
 *              converted into  * RGB  value by the gfx_set_color routines.
 *----------------------------------------------------------------------------
 */
static void
GXSetCursorColors(ScrnInfoPtr pScrni, int bg, int fg)
{
    GFX(set_cursor_colors(bg, fg));
}

/*----------------------------------------------------------------------------
 * GXSetCursorPosition.
 *
 * Description	:This function sets the cursor co -ordinates and enable the
 *               cursor.
 *
 * Parameters:
 *		pScrn: 	Screeen pointer structure.
 *    	    x:  Specifies the x-cordinates of the cursor.
 *    	    y: 	Specifies the y coordinate of the cursor.
 *
 * Returns: none.
 *
 *----------------------------------------------------------------------------
 */
static void
GXSetCursorPosition(ScrnInfoPtr pScrni, int x, int y)
{
    static unsigned long panOffset = 0;
    GeodeRec *pGeode = GEODEPTR(pScrni);
    int savex, savey;
    int newX, newY;

    /* Adjust xf86HWCursor messing about */

    savex = x + pScrni->frameX0;
    savey = y + pScrni->frameY0;

    switch (pGeode->rotation) {
    default:
        ErrorF("%s:%d invalid rotation %d\n", __func__, __LINE__,
               pGeode->rotation);
    case RR_Rotate_0:
        newX = savex;
        newY = savey;
        break;

    case RR_Rotate_90:
        newX = savey;
        newY = pScrni->pScreen->width - savex;
        break;

    case RR_Rotate_180:
        newX = pScrni->pScreen->width - savex;
        newY = pScrni->pScreen->height - savey;
        break;

    case RR_Rotate_270:
        newX = pScrni->pScreen->height - savey;
        newY = savex;
        break;
    }

    newX += pScrni->frameX0;
    newY += pScrni->frameY0;

    //ErrorF("Turned (%d,%d) into (%d,%d)\n", x,y,newX, newY);

    if (newX < -31)
        newX = -31;
    if (newY < -31)
        newY = -31;

    gfx_set_cursor_position(pGeode->CursorStartOffset, newX + 31, newY + 31,
                            31, 31);
    gfx_set_cursor_enable(1);

    if ((pGeode->OverlayON) && (pGeode->Panel)) {
        pGeode->PrevDisplayOffset = gfx_get_display_offset();
        if (pGeode->PrevDisplayOffset != panOffset) {
            GXSetVideoPosition(pGeode->video_x, pGeode->video_y,
                               pGeode->video_w, pGeode->video_h,
                               pGeode->video_srcw, pGeode->video_srch,
                               pGeode->video_dstw, pGeode->video_dsth,
                               pGeode->video_id, pGeode->video_offset,
                               pGeode->video_scrnptr);
            panOffset = pGeode->PrevDisplayOffset;
        }
    }
}

/*----------------------------------------------------------------------------
 * GXLoadCursorImage
 *
 * Description:	This function loads the 32x32 cursor pattern.The shape
 *              and color is set by AND and XOR masking of arrays of 32
 *              DWORD.
 * Parameters:
 *    pScrn: 	Screeen pointer structure.
 *    src: 		Specifies cursor data.
 *
 * Returns: 	none
 *
 *----------------------------------------------------------------------------
 */
void
GXLoadCursorImage(ScrnInfoPtr pScrni, unsigned char *src)
{
    int i, n, x, y, newX, newY;
    unsigned long andMask[32], xorMask[32];
    GeodeRec *pGeode = GEODEPTR(pScrni);
    unsigned long mskb, rowb;
    unsigned char *rowp = &src[0];
    unsigned char *mskp = &src[128];

    if (src != NULL) {
        mskb = rowb = 0;
        for (y = 32; --y >= 0;)
            andMask[y] = xorMask[y] = 0;
        for (y = 0; y < 32; ++y) {
            for (x = 0; x < 32; ++x) {
                if ((i = x & 7) == 0) {
                    rowb = (*rowp & *mskp);
                    mskb = ~(*mskp);
                    ++rowp;
                    ++mskp;
                }

                switch (pGeode->rotation) {
                default:
                    ErrorF("%s:%d invalid rotation %d\n", __func__, __LINE__,
                           pGeode->rotation);
                case RR_Rotate_0:
                    newX = x;
                    newY = y;
                    break;
                case RR_Rotate_90:
                    newX = y;
                    newY = 31 - x;
                    break;
                case RR_Rotate_180:
                    newX = 31 - x;
                    newY = 31 - y;
                    break;
                case RR_Rotate_270:
                    newX = 31 - y;
                    newY = x;
                    break;
                }

                i = 7 - i;
                n = 31 - newX;
                andMask[newY] |= (((mskb >> i) & 1) << n);
                xorMask[newY] |= (((rowb >> i) & 1) << n);
            }
        }
    }
    else {
        for (y = 32; --y >= 0;) {
            andMask[y] = ~0;
            xorMask[y] = 0;
        }
    }

    gfx_set_cursor_shape32(pGeode->CursorStartOffset, &andMask[0], &xorMask[0]);
}

/*----------------------------------------------------------------------------
 * GXHideCursor.
 *
 * Description:	This function will disable the cursor.
 *
 * Parameters:
 *    	pScrn: 	Handles to the Screeen pointer structure.
 *
 * Returns: 	none.
 *
 * Comments:	gfx_set_cursor enable function is hardcoded to disable
 *				the cursor.
 *----------------------------------------------------------------------------
 */
void
GXHideCursor(ScrnInfoPtr pScrni)
{
    gfx_set_cursor_enable(0);
}

/*----------------------------------------------------------------------------
 * GXShowCursor
 *
 * Description	:This function will enable  the cursor.
 *
 * Parameters:
 *	pScrn		:Handles to the Screeen pointer structure.
 *
 * Returns      :none
 *
 * Comments		:gfx_set_cursor enable function is hardcoded to enable the
 * 				cursor
 *----------------------------------------------------------------------------
 */
void
GXShowCursor(ScrnInfoPtr pScrni)
{
    GFX(set_cursor_enable(1));
}

/*----------------------------------------------------------------------------
 * GXUseHwCursor.
 *
 * Description	:This function will sets the hardware cursor flag in
 *              pscreen  structure.
 *
 * Parameters.
 *		pScrn	:Handles to the Screeen pointer structure.
 *
 * Returns		:none
 *
 * Comments		:none
 *
 *----------------------------------------------------------------------------
 */
static Bool
GXUseHWCursor(ScreenPtr pScrn, CursorPtr pCurs)
{
    ScrnInfoPtr pScrni = XF86SCRNINFO(pScrn);
    GeodeRec *pGeode = GEODEPTR(pScrni);

    return pGeode->HWCursor;
}

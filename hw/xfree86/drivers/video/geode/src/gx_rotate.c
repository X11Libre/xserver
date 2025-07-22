/* Copyrightg (c) 2006 Advanced Micro Devices, Inc.
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
#include <xorg-config.h>

#include "xf86.h"
#include "shadow.h"
#include "geode.h"

static void *
GXWindowLinear(ScreenPtr pScreen, CARD32 row, CARD32 offset, int mode,
               CARD32 *size, void *closure)
{
    ScrnInfoPtr pScrni = xf86ScreenToScrn(pScreen);
    GeodeRec *pGeode = GEODEPTR(pScrni);

    *size = pGeode->displayPitch;

    return (pGeode->FBBase + pGeode->displayOffset) +
        row * pGeode->displayPitch + offset;
}

static void
GXUpdate(ScreenPtr pScreen, shadowBufPtr pBuf)
{
    ScrnInfoPtr pScrni = xf86ScreenToScrn(pScreen);
    GeodeRec *pGeode = GEODEPTR(pScrni);
    int rotate = pGeode->rotation;

    switch (rotate) {
    case RR_Rotate_90:

        if (pScrni->bitsPerPixel == 8)
            shadowUpdateRotate8_90(pScreen, pBuf);
        else if (pScrni->bitsPerPixel == 16)
            shadowUpdateRotate16_90(pScreen, pBuf);
        else
            shadowUpdateRotate32_90(pScreen, pBuf);

        break;

    case RR_Rotate_180:

        if (pScrni->bitsPerPixel == 8)
            shadowUpdateRotate8_180(pScreen, pBuf);
        else if (pScrni->bitsPerPixel == 16)
            shadowUpdateRotate16_180(pScreen, pBuf);
        else
            shadowUpdateRotate32_180(pScreen, pBuf);

        break;

    case RR_Rotate_270:
        if (pScrni->bitsPerPixel == 8)
            shadowUpdateRotate8_270(pScreen, pBuf);
        else if (pScrni->bitsPerPixel == 16)
            shadowUpdateRotate16_270(pScreen, pBuf);
        else
            shadowUpdateRotate32_270(pScreen, pBuf);

        break;
    }
}

Bool
GXRotate(ScrnInfoPtr pScrni, DisplayModePtr mode)
{
    GeodeRec *pGeode = GEODEPTR(pScrni);
    Rotation curr = pGeode->rotation;
    unsigned int curdw = pScrni->displayWidth;
    PixmapPtr pPixmap;
    BOOL ret;

    pPixmap = pScrni->pScreen->GetScreenPixmap(pScrni->pScreen);
    pGeode->rotation = GXGetRotation(pScrni->pScreen);

    /* Leave if we have nothing to do */

    if (pGeode->rotation == curr && pGeode->curMode == mode) {
        return TRUE;
    }

    shadowRemove(pScrni->pScreen, NULL);

    switch (pGeode->rotation) {
    case RR_Rotate_0:
        ErrorF("Rotate to 0 degrees\n");
        pScrni->displayWidth = pGeode->displayWidth;
        pGeode->Pitch = pGeode->displayPitch;
        break;

    case RR_Rotate_90:
        ErrorF("Rotate to 90 degrees\n");
        pScrni->displayWidth = pScrni->pScreen->width;
        break;

    case RR_Rotate_180:
        ErrorF("Rotate to 180 degrees\n");
        pScrni->displayWidth = pGeode->displayWidth;
        break;

    case RR_Rotate_270:
        ErrorF("Rotate to 270 degrees\n");
        pScrni->displayWidth = pScrni->pScreen->width;
        break;
    }

    if (pGeode->rotation != RR_Rotate_0) {

        ret =
            shadowAdd(pScrni->pScreen, pPixmap, GXUpdate, GXWindowLinear,
                      pGeode->rotation, NULL);

        if (!ret) {
            ErrorF("shadowAdd failed\n");
            goto error;
        }
    }

    if (pGeode->rotation == RR_Rotate_0)
        pScrni->fbOffset = pGeode->displayOffset;
    else
        pScrni->fbOffset = pGeode->shadowOffset;

    pScrni->pScreen->ModifyPixmapHeader(pPixmap,
                                        pScrni->pScreen->width,
                                        pScrni->pScreen->height,
                                        pScrni->pScreen->rootDepth,
                                        pScrni->bitsPerPixel,
                                        PixmapBytePad(pScrni->displayWidth,
                                                      pScrni->pScreen->
                                                      rootDepth),
                                        (pGeode->FBBase + pScrni->fbOffset));

    return TRUE;

 error:
    /* Restore the old rotation */
    pScrni->displayWidth = curdw;

    if (curr & (RR_Rotate_0 | RR_Rotate_180)) {
        pScrni->pScreen->width = pScrni->virtualX;
        pScrni->pScreen->height = pScrni->virtualY;
    }
    else {
        pScrni->pScreen->width = pScrni->virtualY;
        pScrni->pScreen->height = pScrni->virtualX;
    }

    pGeode->rotation = curr;
    return FALSE;
}

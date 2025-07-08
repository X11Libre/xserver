/*

Copyright 1989, 1998  The Open Group

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation.

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from The Open Group.
*/

/*
 * misprite.h
 *
 * software-sprite/sprite drawing interface spec
 *
 * mi versions of these routines exist.
 */

#ifndef XSERVER_MISPRITE_H
#define XSERVER_MISPRITE_H

Bool miSpriteInitialize(ScreenPtr pScreen, miPointerScreenFuncPtr screenFuncs);

Bool miDCRealizeCursor(ScreenPtr pScreen, CursorPtr pCursor);
Bool miDCUnrealizeCursor(ScreenPtr pScreen, CursorPtr pCursor);
Bool miDCPutUpCursor(DeviceIntPtr pDev, ScreenPtr pScreen,
                     CursorPtr pCursor, int x, int y,
                     unsigned long source, unsigned long mask);
Bool miDCSaveUnderCursor(DeviceIntPtr pDev, ScreenPtr pScreen,
                         int x, int y, int w, int h);
Bool miDCRestoreUnderCursor(DeviceIntPtr pDev, ScreenPtr pScreen,
                            int x, int y, int w, int h);
Bool miDCDeviceInitialize(DeviceIntPtr pDev, ScreenPtr pScreen);
void miDCDeviceCleanup(DeviceIntPtr pDev, ScreenPtr pScreen);

#endif /* XSERVER_MISPRITE_H */

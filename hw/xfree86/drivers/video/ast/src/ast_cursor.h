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

#define	MAX_HWC_WIDTH		64
#define	MAX_HWC_HEIGHT		64
#define HWC_SIZE		(MAX_HWC_WIDTH*MAX_HWC_HEIGHT*2)
#define	HWC_SIGNATURE_SIZE	32
#define	HWC_ALIGN		32

#define HWC_MONO		0
#define HWC_COLOR		1

/* define for signature structure */
#define HWC_SIGNATURE_CHECKSUM	0x00
#define HWC_SIGNATURE_SizeX     0x04
#define HWC_SIGNATURE_SizeY     0x08
#define HWC_SIGNATURE_X         0x0C
#define HWC_SIGNATURE_Y         0x10
#define HWC_SIGNATURE_HOTSPOTX  0x14
#define HWC_SIGNATURE_HOTSPOTY  0x18

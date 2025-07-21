/*
 * Created 1998 by David Bateman <dbateman@eng.uts.edu.au>
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
#include "config.h"
#endif

/*
 * The functions in this file are used to read/write the C&T extension register
 * and supply MMIO replacements of the VGA register access functions in
 * vgaHW.c for chips that support MMIO access (eg 69000). Unlike the MGA
 * chips, for instance, the C&T chipsets don't have a direct mapping between
 * the MMIO mapped vga registers and the PIO versions.
 *
 * In General, these are the ONLY supported way of access the video processors
 * registers. Exception are
 *
 * 1) chipsFindIsaDevice, where we don't know the chipset and so we don't know
 *    if the chipset supports MMIO access to its VGA registers, and we don't
 *    know the chips MemBase address and so can't map the VGA registers even
 *    if the chip did support MMIO. This effectively limits the use of non-PCI
 *    MMIO and multihead to a single card accessing 0x3D6/0x3D7. I.E. You can
 *    only have a single C&T card in a non-PCI multihead arrangement. Also as
 *    ISA has no method to disable I/O access to a card ISA multihead will
 *    never be supported.
 *
 * 2) ct_Blitter.h, ct_BlitMM.h and ct_BltHiQV.h, where speed is crucial and 
 *    we know exactly whether we are using MMIO or PIO.
 *
 * 3) The 6554x 32bit DRxx in ct_cursor.c where the choice between MMIO and
 *    PIO is made explicitly
 */


/* All drivers should typically include these */
#include "xf86.h"
#include "xf86_OSproc.h"

/* Everything using inb/outb, etc needs "compiler.h" */
#include "compiler.h"

/* Drivers that need to access the PCI config space directly need this */
#include "xf86Pci.h"

/* Driver specific headers */
#include "ct_driver.h"

#define CHIPS_MONO_STAT_1		0x3BA
#define CHIPS_STAT_0			0x3BA
#define CHIPS_MSS			0x3CB
#define CHIPS_IOSS			0x3CD
#define CHIPS_FR_INDEX			0x3D0
#define CHIPS_FR_DATA			0x3D1
#define CHIPS_MR_INDEX			0x3D2
#define CHIPS_MR_DATA			0x3D3
#define CHIPS_XR_INDEX			0x3D6
#define CHIPS_XR_DATA			0x3D7
#define CHIPS_COLOR_STAT_1		0x3DA

#define CHIPS_MMIO_MONO_CRTC_INDEX	0x768
#define CHIPS_MMIO_MONO_CRTC_DATA	0x769
#define CHIPS_MMIO_MONO_STAT_1		0x774
#define CHIPS_MMIO_ATTR_INDEX		0x780
#define CHIPS_MMIO_ATTR_DATA_W		0x780
#define CHIPS_MMIO_ATTR_DATA_R		0x781
#define CHIPS_MMIO_STAT_0		0x784
#define CHIPS_MMIO_MISC_OUT_W		0x784
#define CHIPS_MMIO_SEQ_INDEX		0x788
#define CHIPS_MMIO_SEQ_DATA		0x789
#define CHIPS_MMIO_DAC_MASK		0x78C
#define CHIPS_MMIO_DAC_READ_ADDR	0x78D
#define CHIPS_MMIO_DAC_WRITE_ADDR	0x790
#define CHIPS_MMIO_DAC_DATA		0x791
#define CHIPS_MMIO_FEATURE_R		0x794
#define CHIPS_MMIO_MSS			0x795
#define CHIPS_MMIO_MISC_OUT_R		0x798
#define CHIPS_MMIO_IOSS			0x799
#define CHIPS_MMIO_GRAPH_INDEX		0x79C
#define CHIPS_MMIO_GRAPH_DATA		0x79D
#define CHIPS_MMIO_FR_INDEX		0x7A0
#define CHIPS_MMIO_FR_DATA		0x7A1
#define CHIPS_MMIO_MR_INDEX		0x7A4
#define CHIPS_MMIO_MR_DATA		0x7A5
#define CHIPS_MMIO_COLOR_CRTC_INDEX	0x7A8
#define CHIPS_MMIO_COLOR_CRTC_DATA	0x7A9
#define CHIPS_MMIO_XR_INDEX		0x7AC
#define CHIPS_MMIO_XR_DATA		0x7AD
#define CHIPS_MMIO_COLOR_STAT_1		0x7B4

/*
 * PIO Access to the C&T extension registers
 */
static void
chipsStdWriteXR(CHIPSPtr cPtr, CARD8 index, CARD8 value)
{
    outb(cPtr->PIOBase + CHIPS_XR_INDEX, index);
    outb(cPtr->PIOBase + CHIPS_XR_DATA, value);
}

static CARD8
chipsStdReadXR(CHIPSPtr cPtr, CARD8 index)
{
    outb(cPtr->PIOBase + CHIPS_XR_INDEX, index);
    return inb(cPtr->PIOBase + CHIPS_XR_DATA);
}

static void
chipsStdWriteFR(CHIPSPtr cPtr, CARD8 index, CARD8 value)
{
    outb(cPtr->PIOBase + CHIPS_FR_INDEX, index);
    outb(cPtr->PIOBase + CHIPS_FR_DATA, value);
}

static CARD8
chipsStdReadFR(CHIPSPtr cPtr, CARD8 index)
{
    outb(cPtr->PIOBase + CHIPS_FR_INDEX, index);
    return inb(cPtr->PIOBase + CHIPS_FR_DATA);
}

static void
chipsStdWriteMR(CHIPSPtr cPtr, CARD8 index, CARD8 value)
{
    outb(cPtr->PIOBase + CHIPS_MR_INDEX, index);
    outb(cPtr->PIOBase + CHIPS_MR_DATA, value);
}

static CARD8
chipsStdReadMR(CHIPSPtr cPtr, CARD8 index)
{
    outb(cPtr->PIOBase + CHIPS_MR_INDEX, index);
    return inb(cPtr->PIOBase + CHIPS_MR_DATA);
}

static void
chipsStdWriteMSS(CHIPSPtr cPtr, vgaHWPtr hwp, CARD8 value)
{
    outb(cPtr->PIOBase + CHIPS_MSS, value);
}

static CARD8
chipsStdReadMSS(CHIPSPtr cPtr)
{
    return inb(cPtr->PIOBase + CHIPS_MSS);
}

static void
chipsStdWriteIOSS(CHIPSPtr cPtr, CARD8 value)
{
    outb(cPtr->PIOBase + CHIPS_IOSS, value);
}

static CARD8
chipsStdReadIOSS(CHIPSPtr cPtr)
{
    return inb(cPtr->PIOBase + CHIPS_IOSS);
}

void
CHIPSSetStdExtFuncs(CHIPSPtr cPtr)
{
    cPtr->writeFR		= chipsStdWriteFR;
    cPtr->readFR		= chipsStdReadFR;
    cPtr->writeMR		= chipsStdWriteMR;
    cPtr->readMR		= chipsStdReadMR;
    cPtr->writeXR		= chipsStdWriteXR;
    cPtr->readXR		= chipsStdReadXR;
    cPtr->writeMSS		= chipsStdWriteMSS;
    cPtr->readMSS		= chipsStdReadMSS;
    cPtr->writeIOSS		= chipsStdWriteIOSS;
    cPtr->readIOSS		= chipsStdReadIOSS;
}

/*
 * MMIO Access to the C&T extension registers
 */

#define chipsminb(p) MMIO_IN8(cPtr->MMIOBaseVGA, (p))
#define chipsmoutb(p,v) MMIO_OUT8(cPtr->MMIOBaseVGA, (p),(v))

static void
chipsMmioWriteXR(CHIPSPtr cPtr, CARD8 index, CARD8 value)
{
    chipsmoutb(CHIPS_MMIO_XR_INDEX, index);
    chipsmoutb(CHIPS_MMIO_XR_DATA, value);
}

static CARD8
chipsMmioReadXR(CHIPSPtr cPtr, CARD8 index)
{
    chipsmoutb(CHIPS_MMIO_XR_INDEX, index);
    return chipsminb(CHIPS_MMIO_XR_DATA);
}

static void
chipsMmioWriteFR(CHIPSPtr cPtr, CARD8 index, CARD8 value)
{
    chipsmoutb(CHIPS_MMIO_FR_INDEX, index);
    chipsmoutb(CHIPS_MMIO_FR_DATA, value);
}

static CARD8
chipsMmioReadFR(CHIPSPtr cPtr, CARD8 index)
{
    chipsmoutb(CHIPS_MMIO_FR_INDEX, index);
    return chipsminb(CHIPS_MMIO_FR_DATA);
}

static void
chipsMmioWriteMR(CHIPSPtr cPtr, CARD8 index, CARD8 value)
{
    chipsmoutb(CHIPS_MMIO_MR_INDEX, index);
    chipsmoutb(CHIPS_MMIO_MR_DATA, value);
}

static CARD8
chipsMmioReadMR(CHIPSPtr cPtr, CARD8 index)
{
    chipsmoutb(CHIPS_MMIO_MR_INDEX, index);
    return chipsminb(CHIPS_MMIO_MR_DATA);
}

static void
chipsMmioWriteMSS(CHIPSPtr cPtr, vgaHWPtr hwp, CARD8 value)
{
    /* 69030 MMIO Fix.
     *
     * <value> determines which MMIOBase to use; either
     * Pipe A or Pipe B. -GHB
     */
    if ((value & MSS_SHADOW) == MSS_PIPE_B)
	cPtr->MMIOBaseVGA = cPtr->MMIOBasePipeB;
    else
	cPtr->MMIOBaseVGA = cPtr->MMIOBasePipeA;

    hwp->MMIOBase = cPtr->MMIOBaseVGA;

    /* Since our Pipe constants don't set bit 3 of MSS, the value
     * written here has no effect on the hardware's behavior. It
     * does allow us to use the value returned by readMSS() to key
     * the above logic, though. -GHB
     */
    chipsmoutb(CHIPS_MMIO_MSS, value);
}

static CARD8
chipsMmioReadMSS(CHIPSPtr cPtr)
{
    return chipsminb(CHIPS_MMIO_MSS);
}

static void
chipsMmioWriteIOSS(CHIPSPtr cPtr, CARD8 value)
{
    chipsmoutb(CHIPS_MMIO_IOSS, value);
}

static CARD8
chipsMmioReadIOSS(CHIPSPtr cPtr)
{
    return chipsminb(CHIPS_MMIO_IOSS);
}

void
CHIPSSetMmioExtFuncs(CHIPSPtr cPtr)
{
    cPtr->writeFR		= chipsMmioWriteFR;
    cPtr->readFR		= chipsMmioReadFR;
    cPtr->writeMR		= chipsMmioWriteMR;
    cPtr->readMR		= chipsMmioReadMR;
    cPtr->writeXR		= chipsMmioWriteXR;
    cPtr->readXR		= chipsMmioReadXR;
    cPtr->writeMSS		= chipsMmioWriteMSS;
    cPtr->readMSS		= chipsMmioReadMSS;
    cPtr->writeIOSS		= chipsMmioWriteIOSS;
    cPtr->readIOSS		= chipsMmioReadIOSS;
}

/*
 * MMIO versions of the VGA register access functions.
 */

#define minb(p) MMIO_IN8(hwp->MMIOBase, (p))
#define moutb(p,v) MMIO_OUT8(hwp->MMIOBase, (p),(v))

static void
chipsMmioWriteCrtc(vgaHWPtr hwp, CARD8 index, CARD8 value)
{
    if (hwp->IOBase == VGA_IOBASE_MONO) {
	moutb(CHIPS_MMIO_MONO_CRTC_INDEX, index);
    	moutb(CHIPS_MMIO_MONO_CRTC_DATA, value);
    } else {
	moutb(CHIPS_MMIO_COLOR_CRTC_INDEX, index);
    	moutb(CHIPS_MMIO_COLOR_CRTC_DATA, value);
    }
}

static CARD8
chipsMmioReadCrtc(vgaHWPtr hwp, CARD8 index)
{
    if (hwp->IOBase == VGA_IOBASE_MONO) {
	moutb(CHIPS_MMIO_MONO_CRTC_INDEX, index);
    	return minb(CHIPS_MMIO_MONO_CRTC_DATA);
    } else {
	moutb(CHIPS_MMIO_COLOR_CRTC_INDEX, index);
    	return minb(CHIPS_MMIO_COLOR_CRTC_DATA);
    }
}

static void
chipsMmioWriteGr(vgaHWPtr hwp, CARD8 index, CARD8 value)
{
    moutb(CHIPS_MMIO_GRAPH_INDEX, index);
    moutb(CHIPS_MMIO_GRAPH_DATA, value);
}

static CARD8
chipsMmioReadGr(vgaHWPtr hwp, CARD8 index)
{
    moutb(CHIPS_MMIO_GRAPH_INDEX, index);
    return minb(CHIPS_MMIO_GRAPH_DATA);
}

static void
chipsMmioWriteSeq(vgaHWPtr hwp, CARD8 index, CARD8 value)
{
    moutb(CHIPS_MMIO_SEQ_INDEX, index);
    moutb(CHIPS_MMIO_SEQ_DATA, value);
}

static CARD8
chipsMmioReadSeq(vgaHWPtr hwp, CARD8 index)
{
    moutb(CHIPS_MMIO_SEQ_INDEX, index);
    return minb(CHIPS_MMIO_SEQ_DATA);
}

static void
chipsMmioWriteAttr(vgaHWPtr hwp, CARD8 index, CARD8 value)
{
    if (hwp->paletteEnabled)
	index &= ~0x20;
    else
	index |= 0x20;

    if (hwp->IOBase == VGA_IOBASE_MONO)
	(void) minb(CHIPS_MMIO_MONO_STAT_1);
    else
	(void) minb(CHIPS_MMIO_COLOR_STAT_1);
    moutb(CHIPS_MMIO_ATTR_INDEX, index);
    moutb(CHIPS_MMIO_ATTR_DATA_W, value);
}

static CARD8
chipsMmioReadAttr(vgaHWPtr hwp, CARD8 index)
{
    if (hwp->paletteEnabled)
	index &= ~0x20;
    else
	index |= 0x20;

    if (hwp->IOBase == VGA_IOBASE_MONO)
	(void) minb(CHIPS_MMIO_MONO_STAT_1);
    else
	(void) minb(CHIPS_MMIO_COLOR_STAT_1);
    moutb(CHIPS_MMIO_ATTR_INDEX, index);
    return minb(CHIPS_MMIO_ATTR_DATA_R);
}

static void
chipsMmioWriteMiscOut(vgaHWPtr hwp, CARD8 value)
{
    moutb(CHIPS_MMIO_MISC_OUT_W, value);
}

static CARD8
chipsMmioReadMiscOut(vgaHWPtr hwp)
{
    return minb(CHIPS_MMIO_MISC_OUT_R);
}

static void
chipsMmioEnablePalette(vgaHWPtr hwp)
{
    if (hwp->IOBase == VGA_IOBASE_MONO)
	(void) minb(CHIPS_MMIO_MONO_STAT_1);
    else
	(void) minb(CHIPS_MMIO_COLOR_STAT_1);
    moutb(CHIPS_MMIO_ATTR_INDEX, 0x00);
    hwp->paletteEnabled = TRUE;
}

static void
chipsMmioDisablePalette(vgaHWPtr hwp)
{
    if (hwp->IOBase == VGA_IOBASE_MONO)
	(void) minb(CHIPS_MMIO_MONO_STAT_1);
    else
	(void) minb(CHIPS_MMIO_COLOR_STAT_1);
    moutb(CHIPS_MMIO_ATTR_INDEX, 0x20);
    hwp->paletteEnabled = FALSE;
}

static void
chipsMmioWriteDacMask(vgaHWPtr hwp, CARD8 value)
{
    moutb(CHIPS_MMIO_DAC_MASK, value);
}

static CARD8
chipsMmioReadDacMask(vgaHWPtr hwp)
{
    return minb(CHIPS_MMIO_DAC_MASK);
}

static void
chipsMmioWriteDacReadAddr(vgaHWPtr hwp, CARD8 value)
{
    moutb(CHIPS_MMIO_DAC_READ_ADDR, value);
}

static void
chipsMmioWriteDacWriteAddr(vgaHWPtr hwp, CARD8 value)
{
    moutb(CHIPS_MMIO_DAC_WRITE_ADDR, value);
}

static void
chipsMmioWriteDacData(vgaHWPtr hwp, CARD8 value)
{
    moutb(CHIPS_MMIO_DAC_DATA, value);
}

static CARD8
chipsMmioReadDacData(vgaHWPtr hwp)
{
    return minb(CHIPS_MMIO_DAC_DATA);
}

static CARD8
chipsMmioReadST00(vgaHWPtr hwp)
{
    return minb(CHIPS_MMIO_STAT_0);
}

static CARD8
chipsMmioReadST01(vgaHWPtr hwp)
{
    if (hwp->IOBase == VGA_IOBASE_MONO)
	return minb(CHIPS_MMIO_MONO_STAT_1);
    else
	return minb(CHIPS_MMIO_COLOR_STAT_1);
}

static CARD8  
chipsMmioReadFCR(vgaHWPtr hwp)
{
    return minb(CHIPS_MMIO_FEATURE_R);
}

static void
chipsMmioWriteFCR(vgaHWPtr hwp, CARD8 value)
{
    if (hwp->IOBase == VGA_IOBASE_MONO) {
    	moutb(CHIPS_MMIO_MONO_STAT_1, value);
    } else {
    	moutb(CHIPS_MMIO_COLOR_STAT_1, value);
    }
}

void
CHIPSHWSetMmioFuncs(ScrnInfoPtr pScrn, CARD8 *base, int offset)
{
    vgaHWPtr hwp = VGAHWPTR(pScrn);

    hwp->writeCrtc		= chipsMmioWriteCrtc;
    hwp->readCrtc		= chipsMmioReadCrtc;
    hwp->writeGr		= chipsMmioWriteGr;
    hwp->readGr			= chipsMmioReadGr;
    hwp->writeAttr		= chipsMmioWriteAttr;
    hwp->readAttr		= chipsMmioReadAttr;
    hwp->writeSeq		= chipsMmioWriteSeq;
    hwp->readSeq		= chipsMmioReadSeq;
    hwp->writeMiscOut		= chipsMmioWriteMiscOut;
    hwp->readMiscOut		= chipsMmioReadMiscOut;
    hwp->enablePalette		= chipsMmioEnablePalette;
    hwp->disablePalette		= chipsMmioDisablePalette;
    hwp->writeDacMask		= chipsMmioWriteDacMask;
    hwp->readDacMask		= chipsMmioReadDacMask;
    hwp->writeDacWriteAddr	= chipsMmioWriteDacWriteAddr;
    hwp->writeDacReadAddr	= chipsMmioWriteDacReadAddr;
    hwp->writeDacData		= chipsMmioWriteDacData;
    hwp->readDacData		= chipsMmioReadDacData;
    hwp->readST00               = chipsMmioReadST00;
    hwp->readST01               = chipsMmioReadST01;
    hwp->readFCR               = chipsMmioReadFCR;
    hwp->writeFCR              = chipsMmioWriteFCR;
    hwp->MMIOBase		= base;
    hwp->MMIOOffset		= offset;
}





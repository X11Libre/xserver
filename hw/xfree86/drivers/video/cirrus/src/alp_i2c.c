/* (c) Itai Nahshon */

#include <xorg-config.h>

#include "xf86.h"
#include "xf86_OSproc.h"
#include "compiler.h"

#include "xf86Pci.h"

#include "vgaHW.h"

#include "cir.h"
#define _ALP_PRIVATE_
#include "alp.h"


static char strI2CBus1[]	= "I2C bus 1";
static char strI2CBus2[]	= "I2C bus 2";


/*
 * Switch between internal I2C bus and external (DDC) bus.
 * There is one I2C port controlled bu SR08 and the programmable
 * outputs control a multiplexer.
 */
static Bool
AlpI2CSwitchToBus(I2CBusPtr b)
{
	CirPtr pCir = ((CirPtr)b->DriverPrivate.ptr);
	vgaHWPtr hwp = VGAHWPTR(pCir->pScrn);
	CARD8 reg = hwp->readGr(hwp, 0x17);
	if (b == pCir->I2CPtr1) {
	    if ((reg & 0x60) == 0)
  		return TRUE;
	    reg &= ~0x60;
	}
	else if(b == pCir->I2CPtr2) {
	    if ((reg & 0x60) != 0)
  		return TRUE;
	    reg |= 0x60;
	} else 	return FALSE;

	/* ErrorF("AlpI2CSwitchToBus: \"%s\"\n", b->BusName); */
	hwp->writeGr(hwp, 0x17, reg);
	return TRUE;
}

static void
AlpI2CPutBits(I2CBusPtr b, int clock,  int data)
{
	unsigned int reg = 0xfc;
	CirPtr pCir = ((CirPtr)b->DriverPrivate.ptr);
	vgaHWPtr hwp = VGAHWPTR(pCir->pScrn);

	if (!AlpI2CSwitchToBus(b))
		return;

	if (clock) reg |= 1;
	if (data)  reg |= 2;
	hwp->writeSeq(hwp, 0x08, reg);
	/* ErrorF("AlpI2CPutBits: %d %d\n", clock, data); */
}

static void
AlpI2CGetBits(I2CBusPtr b, int *clock, int *data)
{
	unsigned int reg;
	CirPtr pCir = ((CirPtr)b->DriverPrivate.ptr);
	vgaHWPtr hwp = VGAHWPTR(pCir->pScrn);

	if (!AlpI2CSwitchToBus(b))
		return;

	reg = hwp->readSeq(hwp, 0x08);
	*clock = (reg & 0x04) != 0;
	*data  = (reg & 0x80) != 0;
	/* ErrorF("AlpI2CGetBits: %d %d\n", *clock, *data); */
}

Bool
AlpI2CInit(ScrnInfoPtr pScrn)
{
	CirPtr pCir = CIRPTR(pScrn);
	I2CBusPtr I2CPtr;

#ifdef ALP_DEBUG
	ErrorF("AlpI2CInit\n");
#endif

	switch(pCir->Chipset) {
	case PCI_CHIP_GD5446:
	case PCI_CHIP_GD5480:
		break;
	default:
		return FALSE;
	}


	I2CPtr = xf86CreateI2CBusRec();
	if (!I2CPtr) return FALSE;

	pCir->I2CPtr1 = I2CPtr;

	I2CPtr->BusName    = strI2CBus1;
	I2CPtr->scrnIndex  = pScrn->scrnIndex;
	I2CPtr->I2CPutBits = AlpI2CPutBits;
	I2CPtr->I2CGetBits = AlpI2CGetBits;
	I2CPtr->DriverPrivate.ptr = pCir;

	if (!xf86I2CBusInit(I2CPtr))
		return FALSE;

	I2CPtr = xf86CreateI2CBusRec();
	if (!I2CPtr) return FALSE;

	pCir->I2CPtr2 = I2CPtr;

	I2CPtr->BusName    = strI2CBus2;
	I2CPtr->scrnIndex  = pScrn->scrnIndex;
	I2CPtr->I2CPutBits = AlpI2CPutBits;
	I2CPtr->I2CGetBits = AlpI2CGetBits;
	I2CPtr->DriverPrivate.ptr = pCir;

	if (!xf86I2CBusInit(I2CPtr))
		return FALSE;

	return TRUE;
}

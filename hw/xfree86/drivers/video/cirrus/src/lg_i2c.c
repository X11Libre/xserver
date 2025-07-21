/* (c) Itai Nahshon */

#include <dix-config.h>

#include "xf86.h"
#include "xf86_OSproc.h"
#include "compiler.h"

#include "xf86Pci.h"

#include "vgaHW.h"

#include "cir.h"
#define _LG_PRIVATE_
#include "lg.h"


static char strI2CBus1[]    = "I2C bus 1";
static char strI2CBus2[]    = "I2C bus 2";


static void
LgI2CPutBits(I2CBusPtr b, int clock, int data)
{
    unsigned int regval, regno;
    CirPtr pCir = ((CirPtr) b->DriverPrivate.ptr);
    if (b == pCir->I2CPtr1)
        regno = 0x280;
    else if (b == pCir->I2CPtr2)
        regno = 0x282;
    else
        return;

    regval = 0xff7e;
    if (clock)
        regval |= 0x0080;
    if (data)
        regval |= 0x0001;
    memww(regno, regval);
    /*
    ErrorF("LgI2CPutBits: %d %d\n", clock, data);
    */
}

static void
LgI2CGetBits(I2CBusPtr b, int *clock, int *data)
{
    unsigned int regval, regno;
    CirPtr pCir = ((CirPtr) b->DriverPrivate.ptr);
    if (b == pCir->I2CPtr1)
        regno = 0x280;
    else if (b == pCir->I2CPtr2)
        regno = 0x282;
    else
        return;

    regval = memrw(regno);
    *clock = (regval & 0x8000) != 0;
    *data = (regval & 0x0100) != 0;
    /*
    ErrorF("LgI2CGetBits: %d %d\n", *clock, *data);
    */
}

Bool
LgI2CInit(ScrnInfoPtr pScrn)
{
    CirPtr pCir = CIRPTR(pScrn);
    I2CBusPtr I2CPtr;

#ifdef LG_DEBUG
    ErrorF("LgI2CInit\n");
#endif

    I2CPtr = xf86CreateI2CBusRec();
    if (!I2CPtr)
        return FALSE;

    pCir->I2CPtr1 = I2CPtr;

    I2CPtr->BusName             = strI2CBus1;
    I2CPtr->scrnIndex           = pScrn->scrnIndex;
    I2CPtr->I2CPutBits          = LgI2CPutBits;
    I2CPtr->I2CGetBits          = LgI2CGetBits;
    I2CPtr->DriverPrivate.ptr   = pCir;

    if (!xf86I2CBusInit(I2CPtr))
        return FALSE;

    I2CPtr = xf86CreateI2CBusRec();
    if (!I2CPtr)
        return FALSE;

    pCir->I2CPtr2 = I2CPtr;

    I2CPtr->BusName             = strI2CBus2;
    I2CPtr->scrnIndex           = pScrn->scrnIndex;
    I2CPtr->I2CPutBits          = LgI2CPutBits;
    I2CPtr->I2CGetBits          = LgI2CGetBits;
    I2CPtr->DriverPrivate.ptr   = pCir;

    if (!xf86I2CBusInit(I2CPtr))
        return FALSE;

    return TRUE;
}


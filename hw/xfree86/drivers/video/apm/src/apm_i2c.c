
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "apm.h"
#include "apm_regs.h"

/* Inline functions */
static __inline__ void
WaitForFifo(ApmPtr pApm, int slots)
{
  if (!pApm->UsePCIRetry) {
    volatile int i;
#define MAXLOOP 1000000

    for(i = 0; i < MAXLOOP; i++) {
      if ((STATUS_IOP() & STATUS_FIFO) >= slots)
	break;
    }
    if (i == MAXLOOP) {
      unsigned int status = STATUS_IOP();

      WRXB_IOP(0x1FF, 0);
      FatalError("Hung in WaitForFifo() (Status = 0x%08X)\n", status);
    }
  }
}

static void
ApmI2CPutBits(I2CBusPtr b, int clock,  int data)
{
    unsigned int	reg;
    unsigned char	lock;
    ApmPtr pApm = ((ApmPtr)b->DriverPrivate.ptr);

    lock = rdinx(pApm->xport, 0x10);
    wrinx(pApm->xport, 0x10, 0x12);
    WaitForFifo(pApm, 2);
    reg = (RDXB_IOP(0xD0) & 0x07) | 0x60;
    if(clock) reg |= 0x08;
    if(data)  reg |= 0x10;
    WRXB_IOP(0xD0, reg);
    if (lock)
	wrinx(pApm->xport, 0x10, 0);
}

static void
ApmI2CGetBits(I2CBusPtr b, int *clock, int *data)
{
    unsigned int	reg;
    unsigned char	lock;
    ApmPtr pApm = ((ApmPtr)b->DriverPrivate.ptr);
    unsigned char	tmp;

    lock = rdinx(pApm->xport, 0x10);
    wrinx(pApm->xport, 0x10, 0x12);
    WaitForFifo(pApm, 2);
    tmp = RDXB_IOP(0xD0);
    WRXB_IOP(0xD0, tmp & 0x07);
    reg = STATUS_IOP();
    *clock = (reg & STATUS_SCL) != 0;
    *data  = (reg & STATUS_SDA) != 0;
    if (lock)
	wrinx(pApm->xport, 0x10, 0);
}

Bool 
ApmI2CInit(ScrnInfoPtr pScrn)
{
    APMDECL(pScrn);
    I2CBusPtr I2CPtr;

    I2CPtr = xf86CreateI2CBusRec();
    if(!I2CPtr) return FALSE;

    pApm->I2CPtr	= I2CPtr;

    I2CPtr->BusName    = "Alliance bus";
    I2CPtr->scrnIndex  = pScrn->scrnIndex;
    I2CPtr->I2CPutBits = ApmI2CPutBits;
    I2CPtr->I2CGetBits = ApmI2CGetBits;
    I2CPtr->DriverPrivate.ptr = pApm;
    
    if(!xf86I2CBusInit(I2CPtr))
       return FALSE;

    return TRUE;
}

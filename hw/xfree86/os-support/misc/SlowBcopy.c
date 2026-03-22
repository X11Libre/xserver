/*******************************************************************************
  for Alpha Linux
*******************************************************************************/

/*
 *   Create a dependency that should be immune from the effect of register
 *   renaming as is commonly seen in superscalar processors.  This should
 *   insert a minimum of 100-ns delays between reads/writes at clock rates
 *   up to 100 MHz---GGL
 *
 *   Slowbcopy(char *src, char *dst, int count)
 *
 */
#include <xorg-config.h>

#include <X11/X.h>
#include "xf86.h"
#include "xf86Priv.h"
#include "xf86_OSlib.h"
#include "compiler.h"

/* The outb() isn't needed on my machine, but who knows ... -- ost */
void
xf86SlowBcopy(unsigned char *src, unsigned char *dst, int len)
{
    while (len--)
        *dst++ = *src++;
}

#ifdef __alpha__

#ifdef __linux__

unsigned long _bus_base(void);

#define useSparse() (!_bus_base())

#define SPARSE (7)

#else

#define useSparse() 0

#define SPARSE 0

#endif

void
xf86SlowBCopyFromBus(unsigned char *src, unsigned char *dst, int count)
{
    if (useSparse()) {
        unsigned long addr;
        long result;

        addr = (unsigned long) src;
        while (count) {
            result = *(volatile int *) addr;
            result >>= ((addr >> SPARSE) & 3) * 8;
            *dst++ = (unsigned char) (0xffUL & result);
            addr += 1 << SPARSE;
            count--;
            outb(0x80, 0x00);
        }
    }
    else
        xf86SlowBcopy(src, dst, count);
}

void
xf86SlowBCopyToBus(unsigned char *src, unsigned char *dst, int count)
{
    if (useSparse()) {
        unsigned long addr;

        addr = (unsigned long) dst;
        while (count) {
            *(volatile unsigned int *) addr =
                (unsigned short) (*src) * 0x01010101;
            src++;
            addr += 1 << SPARSE;
            count--;
            outb(0x80, 0x00);
        }
    }
    else
        xf86SlowBcopy(src, dst, count);
}
#endif

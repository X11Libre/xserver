/* Copyright (c) 2003-2007 Advanced Micro Devices, Inc.
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "xf86.h"
#include "xf86Modes.h"
#include "compiler.h"
#include "geode.h"

#ifdef XSERVER_LIBPCIACCESS
#include <pciaccess.h>
#endif

/* GPIO Register defines from the CS5536 datasheet */

#define GPIO_OUT        0x00
#define GPIO_OUT_ENABLE 0x04
#define GPIO_OUT_AUX1   0x10
#define GPIO_IN_ENABLE  0x20
#define GPIO_IN         0x30
#define GPIO_IN_AUX1    0x34

/* The DDC pins are defined to be on GPIO pins 3 and 4 */
#define DDC_SCL_PIN  (1 << 3)
#define DDC_SDA_PIN  (1 << 4)

#define DDC_DATA_HIGH    DDC_SDA_PIN
#define DDC_DATA_LOW     (DDC_SDA_PIN << 16)

#define DDC_CLK_HIGH     DDC_SCL_PIN
#define DDC_CLK_LOW      (DDC_SCL_PIN << 16)

#define CS5536_ISA_DEVICE 0x20901022
#define CS5535_ISA_DEVICE 0x002b100b

static unsigned short
geode_gpio_iobase(void)
{
#ifdef XSERVER_LIBPCIACCESS
    struct pci_device *pci;

    /* The CS5536 GPIO device is always in the same slot: 00:0f.0 */
    /* The CS5535 device should be in same slot as well */

    pci = pci_device_find_by_slot(0, 0, 0xF, 0x0);

    if (pci == NULL)
        return 0;

    if (pci_device_probe(pci) != 0)
        return 0;

    /* The GPIO I/O address is in resource 1 */
    return (unsigned short) pci->regions[1].base_addr;
#else
    PCITAG Tag;

    Tag = pciFindFirst(CS5536_ISA_DEVICE, 0xFFFFFFFF);

    if (Tag == PCI_NOT_FOUND) {
        Tag = pciFindFirst(CS5535_ISA_DEVICE, 0xFFFFFFFF);

        if (Tag == PCI_NOT_FOUND)
            return 0;
    }

    /* The GPIO I/O address is in resource 1 */
    return (unsigned short) (pciReadLong(Tag, 0x14) & ~1);
#endif
}

static void
geode_ddc_putbits(I2CBusPtr b, int scl, int sda)
{
    unsigned long iobase = (unsigned long) b->DriverPrivate.ptr;
    unsigned long dat;

    dat = scl ? DDC_CLK_HIGH : DDC_CLK_LOW;
    dat |= sda ? DDC_DATA_HIGH : DDC_DATA_LOW;

    outl(iobase + GPIO_OUT, dat);
}

static void
geode_ddc_getbits(I2CBusPtr b, int *scl, int *sda)
{
    unsigned long iobase = (unsigned long) b->DriverPrivate.ptr;
    unsigned long dat = inl(iobase + GPIO_IN);

    *scl = (dat & DDC_CLK_HIGH) ? 1 : 0;
    *sda = (dat & DDC_DATA_HIGH) ? 1 : 0;
}

Bool
GeodeI2CInit(ScrnInfoPtr pScrni, I2CBusPtr *ptr, const char *name)
{
    I2CBusPtr bus;
    unsigned int ddciobase;

    ddciobase = geode_gpio_iobase();

    if (ddciobase == 0) {
        xf86DrvMsg(pScrni->scrnIndex, X_ERROR,
                   "Could not find the GPIO I/O base\n");
        return FALSE;
    }

    /* The GPIO pins for DDC are multiplexed with a
     * serial port.  If that serial port is enabled, then
     * assume that there is no DDC on the board
     */

    if ((inl(ddciobase + GPIO_IN_AUX1) & DDC_CLK_HIGH) ||
        (inl(ddciobase + GPIO_OUT_AUX1) & DDC_DATA_HIGH)) {
        xf86DrvMsg(pScrni->scrnIndex, X_ERROR,
                   "GPIO pins are in serial mode.  Assuming no DDC\n");
        return FALSE;
    }

    outl(ddciobase + GPIO_OUT_ENABLE, DDC_DATA_HIGH | DDC_CLK_HIGH);
    outl(ddciobase + GPIO_IN_ENABLE, DDC_DATA_HIGH | DDC_CLK_HIGH);

    bus = xf86CreateI2CBusRec();

    if (!bus)
        return FALSE;

    bus->BusName = (char *) name;
    bus->scrnIndex = pScrni->scrnIndex;

    bus->I2CGetBits = geode_ddc_getbits;
    bus->I2CPutBits = geode_ddc_putbits;
    bus->DriverPrivate.ptr = (void *) (unsigned long) (ddciobase);

    if (!xf86I2CBusInit(bus))
        return FALSE;

    *ptr = bus;
    return TRUE;
}

static xf86MonPtr
GeodeGetDDC(ScrnInfoPtr pScrni)
{
    xf86MonPtr mon = NULL;
    I2CBusPtr bus;

    if (!GeodeI2CInit(pScrni, &bus, "CS5536 DDC BUS"))
        return NULL;

    mon = xf86DoEDID_DDC2(pScrni, bus);

    if (mon)
        xf86DDCApplyQuirks(pScrni->scrnIndex, mon);

    xf86DestroyI2CBusRec(bus, FALSE, FALSE);

    return mon;
}

void
GeodeProbeDDC(ScrnInfoPtr pScrni, int index)
{
    ConfiguredMonitor = GeodeGetDDC(pScrni);
}

xf86MonPtr
GeodeDoDDC(ScrnInfoPtr pScrni, int index)
{
    xf86MonPtr info = NULL;

    info = GeodeGetDDC(pScrni);
    xf86PrintEDID(info);
    xf86SetDDCproperties(pScrni, info);
    return info;
}

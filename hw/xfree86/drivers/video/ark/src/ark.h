/*
 * ark
 */

#ifndef _ARK_H
#define _ARK_H

#include "xf86.h"
#include "xf86Pci.h"
#include "vgaHW.h"

#define PCI_VENDOR_ARK			0xEDD8
#define PCI_CHIP_1000PV			0xA091
#define PCI_CHIP_2000PV			0xA099
#define PCI_CHIP_2000MT			0xA0A1
#define PCI_CHIP_2000MI			0xA0A9

typedef struct _ARKRegRec {
	unsigned char		sr10, sr11, sr12, sr13, sr14,
				sr15, sr16, sr17, sr18, sr20,
				sr21, sr22, sr23, sr24, sr25,
				sr26, sr27, sr28, sr29, sr2a,
				sr2b;
	unsigned char		sr1c, sr1d;
	unsigned char		cr40, cr41, cr42, cr44, cr46;
	unsigned char		dac_command;
	unsigned char		stg_17xx[3];
	unsigned char		gendac[6];
} ARKRegRec, *ARKRegPtr;


typedef struct _ARKRec {
#ifndef XSERVER_LIBPCIACCESS	
	pciVideoPtr		PciInfo;
	PCITAG			PciTag;
#else
	struct pci_device       *PciInfo;
#endif
	EntityInfoPtr		pEnt;
	CARD32			IOAddress;
	pointer			FBBase;
	pointer			MMIOBase;
	unsigned long		videoRam;
	OptionInfoPtr		Options;
	unsigned int		Flags;
	Bool			NoAccel;
	CARD32			Bus;
	int			Chipset, ChipRev;
	int			clock_mult;
	int			dac_width;
	int			multiplex_threshold;
	int			ramdac;
	ARKRegRec		SavedRegs;	/* original mode */
	ARKRegRec		ModeRegs;	/* current mode */
	Bool			(*CloseScreen)(ScreenPtr pScreen);
} ARKRec, *ARKPtr;


#define ARKPTR(p)	((ARKPtr)((p)->driverPrivate))


#define DRIVER_NAME	"ark"
#define DRIVER_VERSION	PACKAGE_VERSION
#define VERSION_MAJOR	PACKAGE_VERSION_MAJOR
#define VERSION_MINOR	PACKAGE_VERSION_MINOR
#define PATCHLEVEL	PACKAGE_VERSION_PATCHLEVEL
#define ARK_VERSION	((VERSION_MAJOR << 24) | \
			 (VERSION_MINOR << 16) | \
			  PATCHLEVEL)

#define	ZOOMDAC		0x404
#define ATT490		0x101

#define rdinx(port, ind)	(outb((port), (ind)), inb((port) + 1))
#define wrinx(port, ind, val)						\
    do {								\
	outb((port), (ind));  outb((port) + 1, (val));			\
    } while(0)
#define modinx(port, ind, mask, bits)					\
    do {								\
	unsigned char tmpc;						\
	tmpc = (rdinx((port), (ind)) & ~(mask)) | ((bits) & (mask));	\
	wrinx((port), (ind), tmpc);					\
    } while(0)

#endif /* _ARK_H */

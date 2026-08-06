/*
 *                   XFree86 int10 module
 *   execute BIOS int 10h calls in x86 real mode environment
 *                 Copyright 1999 Egbert Eich
 */

#ifndef _XF86INT10_H
#define _XF86INT10_H

#include <X11/Xmd.h>
#include <X11/Xdefs.h>
#include "xf86Pci.h"

#define SEG_ADDR(x) (((x) >> 4) & 0x00F000)
#define SEG_OFF(x) ((x) & 0x0FFFF)

#define SET_BIOS_SCRATCH     0x1
#define RESTORE_BIOS_SCRATCH 0x2

/* int10 info structure */
typedef struct {
    int entityIndex;
    uint16_t BIOSseg;
    uint16_t inb40time;
    ScrnInfoPtr pScrn;
    void *cpuRegs;
    char *BIOSScratch;
    int Flags;
    void *private;
    struct _int10Mem *mem;
    int num;
    int ax;
    int bx;
    int cx;
    int dx;
    int si;
    int di;
    int es;
    int bp;
    int flags;
    int stackseg;
    struct pci_device *dev;
    struct pci_io_handle *io;
} xf86Int10InfoRec, *xf86Int10InfoPtr;

typedef struct _int10Mem {
    uint8_t (*rb) (xf86Int10InfoPtr, int);
    uint16_t (*rw) (xf86Int10InfoPtr, int);
    uint32_t (*rl) (xf86Int10InfoPtr, int);
    void (*wb) (xf86Int10InfoPtr, int, uint8_t);
    void (*ww) (xf86Int10InfoPtr, int, uint16_t);
    void (*wl) (xf86Int10InfoPtr, int, uint32_t);
} int10MemRec, *int10MemPtr;

/* OS dependent functions */
extern _X_EXPORT xf86Int10InfoPtr xf86InitInt10(int entityIndex);
extern _X_EXPORT xf86Int10InfoPtr xf86ExtendedInitInt10(int entityIndex,
                                                        int Flags);
extern _X_EXPORT void xf86FreeInt10(xf86Int10InfoPtr pInt);
extern _X_EXPORT void *xf86Int10AllocPages(xf86Int10InfoPtr pInt, int num,
                                           int *off);
extern _X_EXPORT void xf86Int10FreePages(xf86Int10InfoPtr pInt, void *pbase,
                                         int num);
extern _X_EXPORT void *xf86int10Addr(xf86Int10InfoPtr pInt, uint32_t addr);

/* x86 executor related functions */
extern _X_EXPORT void xf86ExecX86int10(xf86Int10InfoPtr pInt);

#endif                          /* _XF86INT10_H */
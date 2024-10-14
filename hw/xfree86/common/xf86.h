/*
 * Copyright (c) 1997-2003 by The XFree86 Project, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the name of the copyright holder(s)
 * and author(s) shall not be used in advertising or otherwise to promote
 * the sale, use or other dealings in this Software without prior written
 * authorization from the copyright holder(s) and author(s).
 */

/*
 * This file contains declarations for public XFree86 functions and variables,
 * and definitions of public macros.
 *
 * "public" means available to video drivers.
 */

#ifndef _XF86_H
#define _XF86_H

#ifdef HAVE_XORG_CONFIG_H
#include <xorg-config.h>
#endif

#include "xf86str.h"
#include "xf86Opt.h"
#include <X11/Xfuncproto.h>
#include <stdarg.h>
#ifdef RANDR
#include <X11/extensions/randr.h>
#endif

/* General parameters */
extern _X_EXPORT Bool xorgHWAccess;

extern _X_EXPORT DevPrivateKeyRec xf86ScreenKeyRec;

#define xf86ScreenKey (&xf86ScreenKeyRec)

extern _X_EXPORT ScrnInfoPtr *xf86Screens;      /* List of pointers to ScrnInfoRecs */
extern _X_EXPORT const unsigned char byte_reversed[256];

#define XF86SCRNINFO(p) xf86ScreenToScrn(p)

/* Compatibility functions for pre-input-thread drivers */
static inline _X_DEPRECATED int xf86BlockSIGIO(void) { input_lock(); return 0; }
static inline _X_DEPRECATED void xf86UnblockSIGIO(int wasset) { input_unlock(); }

/* PCI related */
#ifdef XSERVER_LIBPCIACCESS
#include <pciaccess.h>
extern _X_EXPORT Bool xf86CheckPciSlot(const struct pci_device *);
extern _X_EXPORT int xf86ClaimPciSlot(struct pci_device *, DriverPtr drvp,
                                      int chipset, GDevPtr dev, Bool active);
extern _X_EXPORT void xf86UnclaimPciSlot(struct pci_device *, GDevPtr dev);
extern _X_EXPORT Bool xf86ParsePciBusString(const char *busID, int *bus,
                                            int *device, int *func);
extern _X_EXPORT Bool xf86IsPrimaryPci(struct pci_device *pPci);
extern _X_EXPORT Bool xf86CheckPciMemBase(struct pci_device *pPci,
                                          memType base);
extern _X_EXPORT struct pci_device *xf86GetPciInfoForEntity(int entityIndex);
extern _X_EXPORT int xf86MatchPciInstances(const char *driverName,
                                           int vendorID, SymTabPtr chipsets,
                                           PciChipsets * PCIchipsets,
                                           GDevPtr * devList, int numDevs,
                                           DriverPtr drvp, int **foundEntities);
extern _X_EXPORT ScrnInfoPtr xf86ConfigPciEntity(ScrnInfoPtr pScrn,
                                                 int scrnFlag, int entityIndex,
                                                 PciChipsets * p_chip,
                                                 void *dummy, EntityProc init,
                                                 EntityProc enter,
                                                 EntityProc leave,
                                                 void *private);
#endif

/* xf86Bus.c */

extern _X_EXPORT int xf86ClaimFbSlot(DriverPtr drvp, int chipset, GDevPtr dev,
                                     Bool active);
extern _X_EXPORT int xf86ClaimNoSlot(DriverPtr drvp, int chipset, GDevPtr dev,
                                     Bool active);
extern _X_EXPORT void xf86AddEntityToScreen(ScrnInfoPtr pScrn, int entityIndex);
extern _X_EXPORT void xf86SetEntityInstanceForScreen(ScrnInfoPtr pScrn,
                                                     int entityIndex,
                                                     int instance);
extern _X_EXPORT int xf86GetNumEntityInstances(int entityIndex);
extern _X_EXPORT GDevPtr xf86GetDevFromEntity(int entityIndex, int instance);
extern _X_EXPORT EntityInfoPtr xf86GetEntityInfo(int entityIndex);

#define xf86SetLastScrnFlag(e, s) do { } while (0)

extern _X_EXPORT Bool xf86IsEntityShared(int entityIndex);
extern _X_EXPORT void xf86SetEntityShared(int entityIndex);
extern _X_EXPORT Bool xf86IsEntitySharable(int entityIndex);
extern _X_EXPORT void xf86SetEntitySharable(int entityIndex);
extern _X_EXPORT Bool xf86IsPrimInitDone(int entityIndex);
extern _X_EXPORT void xf86SetPrimInitDone(int entityIndex);
extern _X_EXPORT void xf86ClearPrimInitDone(int entityIndex);
extern _X_EXPORT int xf86AllocateEntityPrivateIndex(void);
extern _X_EXPORT DevUnion *xf86GetEntityPrivate(int entityIndex, int privIndex);

/* xf86Configure.c */
extern _X_EXPORT GDevPtr xf86AddBusDeviceToConfigure(const char *driver,
                                                     BusType bus, void *busData,
                                                     int chipset);

/* xf86Cursor.c */

extern _X_EXPORT void xf86SetViewport(ScreenPtr pScreen, int x, int y);
extern _X_EXPORT Bool xf86SwitchMode(ScreenPtr pScreen, DisplayModePtr mode);
extern _X_EXPORT void *xf86GetPointerScreenFuncs(void);
extern _X_EXPORT void xf86ReconfigureLayout(void);

/* xf86DPMS.c */

extern _X_EXPORT Bool xf86DPMSInit(ScreenPtr pScreen, DPMSSetProcPtr set,
                                   int flags);

/* xf86DGA.c */

#ifdef XFreeXDGA
extern _X_EXPORT Bool DGAInit(ScreenPtr pScreen, DGAFunctionPtr funcs,
                              DGAModePtr modes, int num);
extern _X_EXPORT Bool DGAReInitModes(ScreenPtr pScreen, DGAModePtr modes,
                                     int num);
extern _X_EXPORT xf86SetDGAModeProc xf86SetDGAMode;
#endif

/* xf86Events.c */

typedef struct _InputInfoRec *InputInfoPtr;

extern _X_EXPORT void SetTimeSinceLastInputEvent(void);
extern _X_EXPORT void *xf86AddGeneralHandler(int fd, InputHandlerProc proc,
                                               void *data);
extern _X_EXPORT int xf86RemoveGeneralHandler(void *handler);

/* xf86Helper.c */

extern _X_EXPORT void xf86AddDriver(DriverPtr driver, void *module,
                                    int flags);
extern _X_EXPORT ScrnInfoPtr xf86AllocateScreen(DriverPtr drv, int flags);
extern _X_EXPORT int xf86AllocateScrnInfoPrivateIndex(void);
extern _X_EXPORT Bool xf86SetDepthBpp(ScrnInfoPtr scrp, int depth, int bpp,
                                      int fbbpp, int depth24flags);
extern _X_EXPORT void xf86PrintDepthBpp(ScrnInfoPtr scrp);
extern _X_EXPORT Bool xf86SetWeight(ScrnInfoPtr scrp, rgb weight, rgb mask);
extern _X_EXPORT Bool xf86SetDefaultVisual(ScrnInfoPtr scrp, int visual);
extern _X_EXPORT Bool xf86SetGamma(ScrnInfoPtr scrp, Gamma newGamma);
extern _X_EXPORT void xf86SetDpi(ScrnInfoPtr pScrn, int x, int y);
extern _X_EXPORT void xf86SetBlackWhitePixels(ScreenPtr pScreen);
extern _X_EXPORT void xf86EnableDisableFBAccess(ScrnInfoPtr pScrn, Bool enable);
extern _X_EXPORT void
xf86VDrvMsgVerb(int scrnIndex, MessageType type, int verb,
                const char *format, va_list args)
_X_ATTRIBUTE_PRINTF(4, 0);
extern _X_EXPORT void
xf86DrvMsgVerb(int scrnIndex, MessageType type, int verb,
               const char *format, ...)
_X_ATTRIBUTE_PRINTF(4, 5);
extern _X_EXPORT void
xf86DrvMsg(int scrnIndex, MessageType type, const char *format, ...)
_X_ATTRIBUTE_PRINTF(3, 4);
extern _X_EXPORT void
xf86ErrorFVerb(int verb, const char *format, ...)
_X_ATTRIBUTE_PRINTF(2, 3);
extern _X_EXPORT void
xf86ErrorF(const char *format, ...)
_X_ATTRIBUTE_PRINTF(1, 2);
extern _X_EXPORT const char *
xf86TokenToString(SymTabPtr table, int token);
extern _X_EXPORT int
xf86StringToToken(SymTabPtr table, const char *string);
extern _X_EXPORT void
xf86ShowClocks(ScrnInfoPtr scrp, MessageType from);
extern _X_EXPORT void
xf86PrintChipsets(const char *drvname, const char *drvmsg, SymTabPtr chips);
extern _X_EXPORT int
xf86MatchDevice(const char *drivername, GDevPtr ** driversectlist);
extern _X_EXPORT const char *
xf86GetVisualName(int visual);
extern _X_EXPORT int
xf86GetVerbosity(void);
extern _X_EXPORT Gamma
xf86GetGamma(void);
extern _X_EXPORT Bool
xf86ServerIsExiting(void);
extern _X_EXPORT Bool
xf86ServerIsOnlyDetecting(void);
extern _X_EXPORT Bool
xf86GetAllowMouseOpenFail(void);
extern _X_EXPORT CARD32
xorgGetVersion(void);
extern _X_EXPORT CARD32
xf86GetModuleVersion(void *module);
extern _X_EXPORT void *
xf86LoadDrvSubModule(DriverPtr drv, const char *name);
extern _X_EXPORT void *
xf86LoadSubModule(ScrnInfoPtr pScrn, const char *name);
extern _X_EXPORT void *
xf86LoadOneModule(const char *name, void *optlist);
extern _X_EXPORT void
xf86UnloadSubModule(void *mod);
extern _X_EXPORT Bool
xf86LoaderCheckSymbol(const char *name);
extern _X_EXPORT void
xf86SetBackingStore(ScreenPtr pScreen);
extern _X_EXPORT void
xf86SetSilkenMouse(ScreenPtr pScreen);
extern _X_EXPORT ScrnInfoPtr
xf86ConfigFbEntity(ScrnInfoPtr pScrn, int scrnFlag,
                   int entityIndex, EntityProc init,
                   EntityProc enter, EntityProc leave, void *private);

extern _X_EXPORT Bool
xf86IsUnblank(int mode);

/* xf86Init.c */

extern _X_EXPORT PixmapFormatPtr
xf86GetPixFormat(ScrnInfoPtr pScrn, int depth);
extern _X_EXPORT int
xf86GetBppFromDepth(ScrnInfoPtr pScrn, int depth);

/* xf86Mode.c */

extern _X_EXPORT ModeStatus
xf86CheckModeForMonitor(DisplayModePtr mode, MonPtr monitor);
extern _X_EXPORT int
xf86ValidateModes(ScrnInfoPtr scrp, DisplayModePtr availModes,
                  const char **modeNames, ClockRangePtr clockRanges,
                  int *linePitches, int minPitch, int maxPitch,
                  int minHeight, int maxHeight, int pitchInc,
                  int virtualX, int virtualY, int apertureSize,
                  LookupModeFlags strategy);
extern _X_EXPORT void
xf86DeleteMode(DisplayModePtr * modeList, DisplayModePtr mode);
extern _X_EXPORT void
xf86PruneDriverModes(ScrnInfoPtr scrp);
extern _X_EXPORT void
xf86SetCrtcForModes(ScrnInfoPtr scrp, int adjustFlags);
extern _X_EXPORT void
xf86PrintModes(ScrnInfoPtr scrp);

/* xf86Option.c */

extern _X_EXPORT void
xf86CollectOptions(ScrnInfoPtr pScrn, XF86OptionPtr extraOpts);

/* convert ScreenPtr to ScrnInfoPtr */
extern _X_EXPORT ScrnInfoPtr xf86ScreenToScrn(ScreenPtr pScreen);
/* convert ScrnInfoPtr to ScreenPtr */
extern _X_EXPORT ScreenPtr xf86ScrnToScreen(ScrnInfoPtr pScrn);

#define XF86_HAS_SCRN_CONV 1 /* define for drivers to use in api compat */

#define XF86_SCRN_INTERFACE 1 /* define for drivers to use in api compat */

/* flags passed to xf86 allocate screen */
#define XF86_ALLOCATE_GPU_SCREEN 1

/* only for backwards (source) compatibility */
#define xf86MsgVerb LogMessageVerb
#define xf86Msg(type, ...) LogMessageVerb(type, 1, __VA_ARGS__)

#endif                          /* _XF86_H */

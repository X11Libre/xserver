
#ifndef _FBDEVHW_H_
#define _FBDEVHW_H_

#include "xf86str.h"

#define FBDEVHW_PACKED_PIXELS		0       /* Packed Pixels        */
#define FBDEVHW_INTERLEAVED_PLANES	2       /* Interleaved planes   */
#define FBDEVHW_TEXT			3       /* Text/attributes      */
#define FBDEVHW_VGA_PLANES		4       /* EGA/VGA planes       */

extern _X_EXPORT Bool fbdevHWProbe(struct pci_device *pPci, const char *device,
                                   char **namep);
extern _X_EXPORT Bool fbdevHWInit(ScrnInfoPtr pScrn, struct pci_device *pPci,
                                  const char *device);

extern _X_EXPORT char *fbdevHWGetName(ScrnInfoPtr pScrn);
extern _X_EXPORT int fbdevHWGetDepth(ScrnInfoPtr pScrn, int *fbbpp);
extern _X_EXPORT int fbdevHWGetLineLength(ScrnInfoPtr pScrn);
extern _X_EXPORT int fbdevHWGetType(ScrnInfoPtr pScrn);
extern _X_EXPORT int fbdevHWGetVidmem(ScrnInfoPtr pScrn);

extern _X_EXPORT void *fbdevHWMapVidmem(ScrnInfoPtr pScrn);
extern _X_EXPORT int fbdevHWLinearOffset(ScrnInfoPtr pScrn);
extern _X_EXPORT Bool fbdevHWUnmapVidmem(ScrnInfoPtr pScrn);
extern _X_EXPORT void *fbdevHWMapMMIO(ScrnInfoPtr pScrn);
extern _X_EXPORT Bool fbdevHWUnmapMMIO(ScrnInfoPtr pScrn);

extern _X_EXPORT void fbdevHWSetVideoModes(ScrnInfoPtr pScrn);
extern _X_EXPORT void fbdevHWUseBuildinMode(ScrnInfoPtr pScrn);
extern _X_EXPORT Bool fbdevHWModeInit(ScrnInfoPtr pScrn, DisplayModePtr mode);
extern _X_EXPORT void fbdevHWSave(ScrnInfoPtr pScrn);
extern _X_EXPORT void fbdevHWRestore(ScrnInfoPtr pScrn);

extern _X_EXPORT void fbdevHWLoadPalette(ScrnInfoPtr pScrn, int numColors,
                                         int *indices, LOCO * colors,
                                         VisualPtr pVisual);

extern _X_EXPORT ModeStatus fbdevHWValidMode(ScrnInfoPtr pScrn, DisplayModePtr mode,
                                             Bool verbose, int flags);
extern _X_EXPORT Bool fbdevHWSwitchMode(ScrnInfoPtr pScrn, DisplayModePtr mode);
extern _X_EXPORT void fbdevHWAdjustFrame(ScrnInfoPtr pScrn, int x, int y);
extern _X_EXPORT Bool fbdevHWEnterVT(ScrnInfoPtr pScrn);
extern _X_EXPORT void fbdevHWLeaveVT(ScrnInfoPtr pScrn);
extern _X_EXPORT void fbdevHWDPMSSet(ScrnInfoPtr pScrn, int mode, int flags);

extern _X_EXPORT Bool fbdevHWSaveScreen(ScreenPtr pScreen, int mode);

extern _X_EXPORT xf86SwitchModeProc *fbdevHWSwitchModeWeak(void);
extern _X_EXPORT xf86AdjustFrameProc *fbdevHWAdjustFrameWeak(void);
extern _X_EXPORT xf86LeaveVTProc *fbdevHWLeaveVTWeak(void);
extern _X_EXPORT xf86ValidModeProc *fbdevHWValidModeWeak(void);
extern _X_EXPORT xf86DPMSSetProc *fbdevHWDPMSSetWeak(void);
extern _X_EXPORT xf86LoadPaletteProc *fbdevHWLoadPaletteWeak(void);

#endif

#ifndef _PANORAMIXSRV_H_
#define _PANORAMIXSRV_H_

#include "panoramiX.h"

extern int PanoramiXNumScreens;
extern int PanoramiXPixWidth;
extern int PanoramiXPixHeight;
extern RegionRec PanoramiXScreenRegion;

// exported for nvidia
_X_EXPORT VisualID PanoramiXTranslateVisualID(int screen, VisualID orig);

void PanoramiXConsolidate(void);
Bool PanoramiXCreateConnectionBlock(void);
PanoramiXRes *PanoramiXFindIDByScrnum(RESTYPE, XID, int);
Bool XineramaRegisterConnectionBlockCallback(void (*func) (void));
int XineramaDeleteResource(void *, XID);

/* only exported for Nvidia legacy. This really shouldn't be used by drivers */
extern _X_EXPORT RESTYPE XRC_DRAWABLE;

extern RESTYPE XRT_WINDOW;
extern RESTYPE XRT_PIXMAP;
extern RESTYPE XRT_GC;
extern RESTYPE XRT_COLORMAP;
extern RESTYPE XRT_PICTURE;

/*
 * Drivers are allowed to wrap this function.  Each wrapper can decide that the
 * two visuals are unequal, but if they are deemed equal, the wrapper must call
 * down and return FALSE if the wrapped function does.  This ensures that all
 * layers agree that the visuals are equal.  The first visual is always from
 * screen 0.
 */
typedef Bool (*XineramaVisualsEqualProcPtr) (VisualPtr, ScreenPtr, VisualPtr);

void XineramaGetImageData(DrawablePtr *pDrawables,
                          int left,
                          int top,
                          int width,
                          int height,
                          unsigned int format,
                          unsigned long planemask,
                          char *data, int pitch, Bool isRoot);

static inline void
panoramix_setup_ids(PanoramiXRes * resource, ClientPtr client, XID base_id)
{
    int j;

    resource->info[0].id = base_id;
    FOR_NSCREENS_FORWARD(j) {
        if (j) /* skip screen #0 */
            resource->info[j].id = FakeClientID(client->index);
    }
}

#endif                          /* _PANORAMIXSRV_H_ */

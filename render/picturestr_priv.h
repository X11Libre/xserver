/* SPDX-License-Identifier: MIT OR X11
 *
 * Copyright © 2024 Enrico Weigelt, metux IT consult <info@metux.net>
 * Copyright © 2000 SuSE, Inc.
 */
#ifndef _XSERVER_PICTURESTR_PRIV_H_
#define _XSERVER_PICTURESTR_PRIV_H_

#include "picturestr.h"
#include "scrnintstr.h"
#include "glyphstr.h"
#include "resource.h"
#include "privates.h"

#define PICT_GRADIENT_STOPTABLE_SIZE 1024

extern RESTYPE PictureType;
extern RESTYPE PictFormatType;
extern RESTYPE GlyphSetType;

#define VERIFY_PICTURE(pPicture, pid, client, mode) {\
    int tmprc = dixLookupResourceByType((void *)&(pPicture), pid,\
	                                PictureType, client, mode);\
    if (tmprc != Success)\
	return tmprc;\
}

#define VERIFY_ALPHA(pPicture, pid, client, mode) {\
    if (pid == None) \
	pPicture = 0; \
    else { \
	VERIFY_PICTURE(pPicture, pid, client, mode); \
    } \
} \

Bool AnimCurInit(ScreenPtr pScreen);

int AnimCursorCreate(CursorPtr *cursors, CARD32 *deltas, int ncursor,
                     CursorPtr *ppCursor, ClientPtr client, XID cid);

#ifdef XINERAMA
void PanoramiXRenderInit(void);
void PanoramiXRenderReset(void);
#endif /* XINERAMA */

int PictureGetSubpixelOrder(ScreenPtr pScreen);
Bool PictureInit(ScreenPtr pScreen, PictFormatPtr formats, int nformats);
int PictureGetFilterId(const char *filter, int len, Bool makeit);
int PictureAddFilter(ScreenPtr pScreen, const char *filter,
                     PictFilterValidateParamsProcPtr ValidateParams,
                     int width, int height);
Bool PictureSetFilterAlias(ScreenPtr pScreen, const char *filter,
                           const char *alias);
Bool PictureSetDefaultFilters(ScreenPtr pScreen);
void PictureResetFilters(ScreenPtr pScreen);
Bool PictureFinishInit(void);
int SetPictureClipRects(PicturePtr pPicture, int xOrigin, int yOrigin,
                        int nRect, xRectangle *rects);
int SetPictureClipRegion(PicturePtr pPicture, int xOrigin, int yOrigin,
                         RegionPtr pRegion);
void CompositeGlyphs(CARD8 op, PicturePtr pSrc, PicturePtr pDst,
                     PictFormatPtr maskFormat, INT16 xSrc, INT16 ySrc,
                     int nlist, GlyphListPtr lists, GlyphPtr *glyphs);
void CompositeRects(CARD8 op, PicturePtr pDst, xRenderColor *color,
                    int nRect, xRectangle *rects);
void CompositeTrapezoids(CARD8 op, PicturePtr pSrc, PicturePtr pDst,
                         PictFormatPtr maskFormat, INT16 xSrc, INT16 ySrc,
                         int ntrap, xTrapezoid *traps);
void CompositeTriangles(CARD8 op, PicturePtr pSrc, PicturePtr pDst,
                        PictFormatPtr maskFormat, INT16 xSrc, INT16 ySrc,
                        int ntriangles, xTriangle * triangles);
void CompositeTriStrip(CARD8 op, PicturePtr pSrc, PicturePtr pDst,
                       PictFormatPtr maskFormat, INT16 xSrc, INT16 ySrc,
                       int npoints, xPointFixed * points);
void CompositeTriFan(CARD8 op, PicturePtr pSrc, PicturePtr pDst,
                     PictFormatPtr maskFormat, INT16 xSrc, INT16 ySrc,
                     int npoints, xPointFixed * points);
void AddTraps(PicturePtr pPicture, INT16 xOff, INT16 yOff, int ntraps,
              xTrap *traps);
PicturePtr CreateLinearGradientPicture(Picture pid, xPointFixed *p1,
                                       xPointFixed *p2, int nStops,
                                       xFixed *stops, xRenderColor *colors,
                                       int *error);
PicturePtr CreateRadialGradientPicture(Picture pid, xPointFixed *inner,
                                       xPointFixed *outer, xFixed innerRadius,
                                       xFixed outerRadius, int nStops,
                                       xFixed *stops, xRenderColor *colors,
                                       int *error);
PicturePtr CreateConicalGradientPicture(Picture pid, xPointFixed *center,
                                        xFixed angle, int nStops, xFixed *stops,
                                        xRenderColor *colors, int *error);
void PictTransform_from_xRenderTransform(PictTransformPtr pict,
                                         xRenderTransform *render);
void xRenderTransform_from_PictTransform(xRenderTransform *render,
                                         PictTransformPtr pict);
Bool PictureTransformPoint3d(PictTransformPtr transform,
                             PictVectorPtr vector);

/* these need to be _X_EXPORT'ed for Nvidia proprietary, but should not
   be used by external drivers anymore */
_X_EXPORT PictFormatPtr PictureMatchVisual(ScreenPtr pScreen, int depth,
                                           VisualPtr pVisual);
_X_EXPORT PictFilterPtr PictureFindFilter(ScreenPtr pScreen, char *name,
                                          int len);
_X_EXPORT int SetPictureFilter(PicturePtr pPicture, char *name, int len,
                               xFixed *params, int nparams);

#endif /* _XSERVER_PICTURESTR_PRIV_H_ */

/*
 *	Server dispatcher function replacements
 */

#ifndef XSERVER_PANORAMIXH_H
#define XSERVER_PANORAMIXH_H

int PanoramiXCreateWindow(ClientPtr client);
int PanoramiXChangeWindowAttributes(ClientPtr client);
int PanoramiXDestroyWindow(ClientPtr client);
int PanoramiXDestroySubwindows(ClientPtr client);
int PanoramiXChangeSaveSet(ClientPtr client);
int PanoramiXReparentWindow(ClientPtr client);
int PanoramiXMapWindow(ClientPtr client);
int PanoramiXMapSubwindows(ClientPtr client);
int PanoramiXUnmapWindow(ClientPtr client);
int PanoramiXUnmapSubwindows(ClientPtr client);
int PanoramiXConfigureWindow(ClientPtr client);
int PanoramiXCirculateWindow(ClientPtr client);
int PanoramiXGetGeometry(ClientPtr client);
int PanoramiXTranslateCoords(ClientPtr client);
int PanoramiXCreatePixmap(ClientPtr client);
int PanoramiXFreePixmap(ClientPtr client);
int PanoramiXChangeGC(ClientPtr client);
int PanoramiXCopyGC(ClientPtr client);
int PanoramiXCopyColormapAndFree(ClientPtr client);
int PanoramiXCreateGC(ClientPtr client);
int PanoramiXSetDashes(ClientPtr client);
int PanoramiXSetClipRectangles(ClientPtr client);
int PanoramiXFreeGC(ClientPtr client);
int PanoramiXClearToBackground(ClientPtr client);
int PanoramiXCopyArea(ClientPtr client);
int PanoramiXCopyPlane(ClientPtr client);
int PanoramiXPolyPoint(ClientPtr client);
int PanoramiXPolyLine(ClientPtr client);
int PanoramiXPolySegment(ClientPtr client);
int PanoramiXPolyRectangle(ClientPtr client);
int PanoramiXPolyArc(ClientPtr client);
int PanoramiXFillPoly(ClientPtr client);
int PanoramiXPolyFillArc(ClientPtr client);
int PanoramiXPolyFillRectangle(ClientPtr client);
int PanoramiXPutImage(ClientPtr client);
int PanoramiXGetImage(ClientPtr client);
int PanoramiXPolyText8(ClientPtr client);
int PanoramiXPolyText16(ClientPtr client);
int PanoramiXImageText8(ClientPtr client);
int PanoramiXImageText16(ClientPtr client);
int PanoramiXCreateColormap(ClientPtr client);
int PanoramiXFreeColormap(ClientPtr client);
int PanoramiXInstallColormap(ClientPtr client);
int PanoramiXUninstallColormap(ClientPtr client);
int PanoramiXAllocColor(ClientPtr client);
int PanoramiXAllocNamedColor(ClientPtr client);
int PanoramiXAllocColorCells(ClientPtr client);
int PanoramiXStoreNamedColor(ClientPtr client);
int PanoramiXFreeColors(ClientPtr client);
int PanoramiXStoreColors(ClientPtr client);
int PanoramiXAllocColorPlanes(ClientPtr client);

int ProcPanoramiXQueryVersion(ClientPtr client);
int ProcPanoramiXGetState(ClientPtr client);
int ProcPanoramiXGetScreenCount(ClientPtr client);
int ProcPanoramiXGetScreenSize(ClientPtr client);

int ProcXineramaQueryScreens(ClientPtr client);
int ProcXineramaIsActive(ClientPtr client);

extern int connBlockScreenStart;
extern xConnSetupPrefix connSetupPrefix;

extern int (*SavedProcVector[256]) (ClientPtr client);

#endif /* XSERVER_PANORAMIXH_H */

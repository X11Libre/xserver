#ifndef __MIOVERLAY_H
#define __MIOVERLAY_H

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

typedef void (*miOverlayTransFunc) (ScreenPtr, int, BoxPtr);
typedef Bool (*miOverlayInOverlayFunc) (WindowPtr);

#endif                          /* __MIOVERLAY_H */

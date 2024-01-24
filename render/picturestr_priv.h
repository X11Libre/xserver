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

#endif /* _XSERVER_PICTURESTR_PRIV_H_ */

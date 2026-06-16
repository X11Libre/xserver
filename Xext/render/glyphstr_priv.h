/* SPDX-License-Identifier: MIT OR X11
 *
 * Copyright © 2024 Enrico Weigelt, metux IT consult <info@metux.net>
 * Copyright © 2000 SuSE, Inc.
 */
#ifndef _XSERVER_GLYPHSTR_PRIV_H_
#define _XSERVER_GLYPHSTR_PRIV_H_

#include <X11/extensions/renderproto.h>
#include "glyphstr.h"
#include "picture.h"
#include "screenint.h"
#include "regionstr.h"
#include "miscstruct.h"
#include "privates.h"

#define GlyphPicture(glyph) ((PicturePtr *) ((glyph) + 1))

typedef struct {
    CARD32 signature;
    GlyphPtr glyph;
} GlyphRefRec, *GlyphRefPtr;

#define DeletedGlyph	((GlyphPtr) 1)

typedef struct {
    CARD32 entries;
    CARD32 size;
    CARD32 rehash;
} GlyphHashSetRec, *GlyphHashSetPtr;

typedef struct {
    GlyphRefPtr table;
    GlyphHashSetPtr hashSet;
    CARD32 tableEntries;
} GlyphHashRec, *GlyphHashPtr;

typedef struct {
    CARD32 refcnt;
    int fdepth;
    PictFormatPtr format;
    GlyphHashRec hash;
    PrivateRec *devPrivates;
} GlyphSetRec, *GlyphSetPtr;

#define GlyphSetGetPrivate(pGlyphSet,k) \
    dixLookupPrivate(&(pGlyphSet)->devPrivates, k)

#define GlyphSetSetPrivate(pGlyphSet,k,ptr) \
    dixSetPrivate(&(pGlyphSet)->devPrivates, k, ptr)

void GlyphUninit(ScreenPtr pScreen);
GlyphPtr FindGlyphByHash(unsigned char sha1[20], int format);
int HashGlyph(xGlyphInfo * gi, CARD8 *bits, unsigned long size, unsigned char sha1[20]);
void AddGlyph(GlyphSetPtr glyphSet, GlyphPtr glyph, Glyph id);
Bool DeleteGlyph(GlyphSetPtr glyphSet, Glyph id);
GlyphPtr FindGlyph(GlyphSetPtr glyphSet, Glyph id);
GlyphPtr AllocateGlyph(xGlyphInfo * gi, int format);
void FreeGlyph(GlyphPtr glyph, int format);
Bool ResizeGlyphSet(GlyphSetPtr glyphSet, CARD32 change);
GlyphSetPtr AllocateGlyphSet(int fdepth, PictFormatPtr format);
int FreeGlyphSet(void *value, XID gid);

#endif /* _XSERVER_GLYPHSTR_PRIV_H_ */

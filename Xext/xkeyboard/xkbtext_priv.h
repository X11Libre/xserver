/* SPDX-License-Identifier: MIT OR X11
 *
 * Copyright © 2024 Enrico Weigelt, metux IT consult <info@metux.net>
 */
#ifndef _XSERVER_XKB_XKBTEXT_PRIV_H
#define _XSERVER_XKB_XKBTEXT_PRIV_H

#include <X11/X.h>

#include "xkbstr.h"

#define XkbXKMFile      0
#define XkbCFile        1
#define XkbXKBFile      2
#define XkbMessage      3

char *XkbIndentText(unsigned size);
char *XkbAtomText(Atom atm, unsigned format);
char *XkbKeysymText(KeySym sym, unsigned format);
char *XkbStringText(char *str, unsigned format);
char *XkbKeyNameText(char *name, unsigned format);
char *XkbModIndexText(unsigned ndx, unsigned format);
char *XkbModMaskText(unsigned mask, unsigned format);
char *XkbVModIndexText(XkbDescPtr xkb, unsigned ndx, unsigned format);
char *XkbVModMaskText(XkbDescPtr xkb, unsigned modMask, unsigned mask,
                      unsigned format);
char *XkbConfigText(unsigned config, unsigned format);
const char *XkbSIMatchText(unsigned type, unsigned format);
char *XkbIMWhichStateMaskText(unsigned use_which, unsigned format);
char *XkbControlsMaskText(unsigned ctrls, unsigned format);
char *XkbGeomFPText(int val, unsigned format);
char *XkbDoodadTypeText(unsigned type, unsigned format);
const char *XkbActionTypeText(unsigned type, unsigned format);
char *XkbActionText(XkbDescPtr xkb, XkbAction *action, unsigned format);
char *XkbBehaviorText(XkbDescPtr xkb, XkbBehavior *behavior, unsigned format);

#endif /* _XSERVER_XKB_XKBTEXT_PRIV_H */

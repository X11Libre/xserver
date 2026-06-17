/* SPDX-License-Identifier: MIT OR X11
 *
 * Copyright © 2024 Enrico Weigelt, metux IT consult <info@metux.net>
 */
#ifndef _XSERVER_XKB_XKBFMISC_PRIV_H
#define _XSERVER_XKB_XKBFMISC_PRIV_H

/* needed for X11/keysymdef.h to define all symdefs */
#define XK_MISCELLANY

#include <stdio.h>
#include <X11/X.h>
#include <X11/Xdefs.h>
#include <X11/keysymdef.h>

#include "xkbstr.h"

/*
 * return mask bits for _XkbKSCheckCase()
 */
#define _XkbKSLower     (1<<0)
#define _XkbKSUpper     (1<<1)

/*
 * check whether given KeySym is a upper or lower case key
 *
 * @param sym the KeySym to check
 * @return mask of _XkbKS* flags
 */
unsigned int _XkbKSCheckCase(KeySym sym);

/*
 * check whether given KeySym is an lower case key
 *
 * @param k the KeySym to check
 * @return TRUE if k is a lower case key
 */
static inline Bool XkbKSIsLower(KeySym k) { return _XkbKSCheckCase(k)&_XkbKSLower; }

/*
 * check whether given KeySym is an upper case key
 *
 * @param k the KeySym to check
 * @return TRUE if k is a upper case key
 */
static inline Bool XkbKSIsUpper(KeySym k) { return _XkbKSCheckCase(k)&_XkbKSUpper; }

/*
 * check whether given KeySym is an keypad key
 *
 * @param k the KeySym to check
 * @return TRUE if k is a keypad key
 */
static inline Bool XkbKSIsKeypad(KeySym k) { return (((k)>=XK_KP_Space)&&((k)<=XK_KP_Equal)); }

/*
 * find a keycode by its name
 *
 * @param xkb pointer to xkb descriptor
 * @param name the key name
 * @param use_aliases TRUE if aliases should be resolved
 * @return keycode ID
 */
int XkbFindKeycodeByName(XkbDescPtr xkb, char *name, Bool use_aliases);

/*
 * write keymap for given component names
 *
 * @param file the FILE to write to
 * @param names pointer to list of keymap component names to write out
 * @param xkb pointer to xkb descriptor
 * @param want bitmask of wanted elements
 * @param need bitmask of needed elements
 * @return TRUE if succeeded
*/
Bool XkbWriteXKBKeymapForNames(FILE *file, XkbComponentNamesPtr names,
                               XkbDescPtr xkb, unsigned want, unsigned need);

#endif /* _XSERVER_XKB_XKBFMISC_PRIV_H */

/* SPDX-License-Identifier: MIT OR X11
 *
 * Copyright © 2024 Enrico Weigelt, metux IT consult <info@metux.net>
 */
#ifndef _XSERVER_XKB_XKBRULES_PRIV_H
#define _XSERVER_XKB_XKBRULES_PRIV_H

#include <stdio.h>
#include <stdlib.h>
#include <X11/Xdefs.h>

#include "include/xkbrules.h"

typedef struct _XkbRF_VarDefs {
    const char *model;
    const char *layout;
    const char *variant;
    const char *options;
} XkbRF_VarDefsRec, *XkbRF_VarDefsPtr;

typedef struct _XkbRF_Rule {
    int number;
    int layout_num;
    int variant_num;
    const char *model;
    const char *layout;
    const char *variant;
    const char *option;
    /* yields */
    const char *keycodes;
    const char *symbols;
    const char *types;
    const char *compat;
    const char *geometry;
    unsigned flags;
} XkbRF_RuleRec, *XkbRF_RulePtr;

typedef struct _XkbRF_Group {
    int number;
    const char *name;
    char *words;
} XkbRF_GroupRec, *XkbRF_GroupPtr;

typedef struct _XkbRF_Rules {
    unsigned short sz_rules;
    unsigned short num_rules;
    XkbRF_RulePtr rules;
    unsigned short sz_groups;
    unsigned short num_groups;
    XkbRF_GroupPtr groups;
} XkbRF_RulesRec, *XkbRF_RulesPtr;

struct _XkbComponentNames;

Bool XkbRF_GetComponents(XkbRF_RulesPtr rules,
                         XkbRF_VarDefsPtr var_defs,
                         struct _XkbComponentNames *names);

Bool XkbRF_LoadRules(FILE *file, XkbRF_RulesPtr rules);

static inline XkbRF_RulesPtr XkbRF_Create(void)
{
    return calloc(1, sizeof(XkbRF_RulesRec));
}

void XkbRF_Free(XkbRF_RulesPtr rules);

#endif /* _XSERVER_XKB_XKBRULES_PRIV_H */

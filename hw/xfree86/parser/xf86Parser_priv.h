/* SPDX-License-Identifier: MIT OR X11
 *
 * Copyright © 2024 Enrico Weigelt, metux IT consult <info@metux.net>
 * Copyright © 1997 Metro Link Incorporated
 */
#ifndef _XSERVER_XF86_PARSER_PRIV
#define _XSERVER_XF86_PARSER_PRIV

#include "xf86Parser.h"

void xf86initConfigFiles(void);
char *xf86openConfigFile(const char *path,
                         const char *cmdline,
                         const char *projroot);
char *xf86openConfigDirFiles(const char *path,
                             const char *cmdline,
                             const char *projroot);
void xf86setBuiltinConfig(const char *config[]);
XF86ConfigPtr xf86readConfigFile(void);
void xf86closeConfigFile(void);
XF86ConfigPtr xf86allocateConfig(void);
void xf86freeConfig(XF86ConfigPtr p);
int xf86writeConfigFile(const char *filename, XF86ConfigPtr cptr);
int xf86layoutAddInputDevices(XF86ConfigPtr config, XF86ConfLayoutPtr layout);

#endif /* _XSERVER_XF86_PARSER_PRIV */

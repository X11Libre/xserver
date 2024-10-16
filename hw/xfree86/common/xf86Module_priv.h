/* SPDX-License-Identifier: MIT OR X11
 *
 * Copyright © 2024 Enrico Weigelt, metux IT consult <info@metux.net>
 */
#ifndef _XORG_XF86MODULE_PRIV_H
#define _XORG_XF86MODULE_PRIV_H

/*
 * unload a previously loaded module
 *
 * @param mod the module to unload
 */
void UnloadModule(ModuleDescPtr mod);

/*
 * unload a previously loaded sun-module
 *
 * @param mod the sub-module to unload
 */
void UnloadSubModule(ModuleDescPtr mod);

#endif /* _XORG_XF86MODULE_PRIV_H */

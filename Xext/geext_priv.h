/* SPDX-License-Identifier: MIT OR X11
 *
 * Copyright © 2024 Enrico Weigelt, metux IT consult <info@metux.net>
 */
#ifndef _XORG_GEEXT_PRIV_H

#include <X11/Xproto.h>

typedef void (*XorgGESwapProcPtr) (xGenericEvent *from, xGenericEvent *to);

/*
 * Register generic event extension dispatch handler
 *
 * @param extension base opcode
 * @param event swap handler function
 */
void GERegisterExtension(int extension, XorgGESwapProcPtr swap_handler);

#endif /* _XORG_GEEXT_PRIV_H */

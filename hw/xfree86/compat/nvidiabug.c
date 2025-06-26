/* SPDX-License-Identifier: MIT OR X11
 *
 * Copyright © 2024 Enrico Weigelt, metux IT consult <info@metux.net>
 */
#include <dix-config.h>

#include "include/os.h"

#include "xf86_compat.h"

void xf86NVidiaBug(void)
{
    LogMessageVerb(X_WARNING, 0, "[DRIVER BUG] file a bug report to driver vendor or use a free Xlibre driver.\n");
    LogMessageVerb(X_WARNING, 0, "[DRIVER BUG] Proprietary drivers are inherently unstable, they just can't be done right.\n");
    LogMessageVerb(X_WARNING, 0, "[DRIVER BUG] For NVidia report here: https://forums.developer.nvidia.com/c/gpu-graphics/linux/148\n");
    LogMessageVerb(X_WARNING, 0, "[DRIVER BUG] And better don't buy NVidia HW until they've fixed their mess.\n");
}

void xf86NVidiaBugInternalFunc(const char* name)
{
    LogMessageVerb(X_WARNING, 0, "[DRIVER BUG] calling internal function: %s\n", name);
    LogMessageVerb(X_WARNING, 0, "[DRIVER BUG] this function is not supposed to be by drivers ever\n");
    xf86NVidiaBug();
}

void xf86NVidiaBugObsoleteFunc(const char* name)
{
    LogMessageVerb(X_WARNING, 0, "[DRIVER BUG] calling obsolete function: %s\n", name);
    LogMessageVerb(X_WARNING, 0, "[DRIVER BUG] this function is not supposed to be by drivers ever\n");
    xf86NVidiaBug();
}

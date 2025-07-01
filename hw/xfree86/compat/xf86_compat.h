/* SPDX-License-Identifier: MIT OR X11
 *
 * Copyright © 2024 Enrico Weigelt, metux IT consult <info@metux.net>
 */
#ifndef __XFREE86_COMPAT_XF86_COMPAT_H
#define __XFREE86_COMPAT_XF86_COMPAT_H

void xf86NVidiaBug(void);
void xf86NVidiaBugInternalFunc(const char* name);
void xf86NVidiaBugObsoleteFunc(const char* name);

#endif /* __XFREE86_COMPAT_XF86_COMPAT_H */

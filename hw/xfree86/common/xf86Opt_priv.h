/* SPDX-License-Identifier: MIT OR X11 OR AGPL-3.0-or-later
 *
 * Copyright © 2024 Enrico Weigelt, metux IT consult <info@metux.net>
 */
#ifndef _XORG_XF86OPTION_PRIV_H
#define _XORG_XF86OPTION_PRIV_H

#include "xf86Opt.h"

void xf86OptionListReport(XF86OptionPtr parm);
void xf86MarkOptionUsed(XF86OptionPtr option);

#endif /* _XORG_XF86OPTION_PRIV_H */

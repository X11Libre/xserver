/* SPDX-License-Identifier: X11 OR MIT OR AGPL-3.0-or-later
 *
 * Copyright © 2026 Enrico Weigelt, metux IT consult <info@metux.net>
 */
#ifndef _XSERVER_FALLTHROUGH_H
#define _XSERVER_FALLTHROUGH_H

/*
 * Fallthrough hint.
 *
 * Upstream xorg/main replaced the plain fallthrough comments with
 * _X_FALLTHROUGH, which is provided by xorgproto >= 2025.1. We are still on
 * xorgproto 2024.1 / 7.0.33, where the macro does not exist, so provide a
 * local definition instead of stalling the conversion.
 *
 * Private on purpose. This header is not installed, is not part of the SDK
 * header set, and is therefore not part of the module API/ABI: nothing is
 * exposed to external drivers or extension modules.
 *
 * DELETE THIS FILE as soon as xorgproto >= 2025.1 is in use. The real
 * _X_FALLTHROUGH from xorgproto then becomes visible, and this definition
 * must be removed rather than overridden, so the two can never disagree.
 */
#ifndef _X_FALLTHROUGH
# if defined(__has_c_attribute)
#  if __has_c_attribute(fallthrough)
#   define _X_FALLTHROUGH [[fallthrough]]
#  endif
# endif
# ifndef _X_FALLTHROUGH
#  if defined(__GNUC__) && __GNUC__ >= 7
#   define _X_FALLTHROUGH __attribute__((fallthrough))
#  else
#   define _X_FALLTHROUGH ((void)0)
#  endif
# endif
#endif

#endif /* _XSERVER_FALLTHROUGH_H */

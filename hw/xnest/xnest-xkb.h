/* SPDX-License-Identifier: MIT OR X11
 *
 * Copyright © 2024 Enrico Weigelt, metux IT consult <info@metux.net>
 */
#ifndef __XNEST__XKB_H
#define __XNEST__XKB_H

#include <xcb/xcb.h>
#include <xcb/xkb.h>

xcb_xkb_get_kbd_by_name_cookie_t
xcb_xkb_get_kbd_by_name_2 (xcb_connection_t      *c,
                         xcb_xkb_device_spec_t  deviceSpec,
                         uint16_t               need,
                         uint16_t               want,
                         uint8_t                load,
                         uint32_t               data_len,
                         const uint8_t         *data);

xcb_xkb_get_kbd_by_name_cookie_t
xcb_xkb_get_kbd_by_name_2_unchecked (xcb_connection_t      *c,
                                   xcb_xkb_device_spec_t  deviceSpec,
                                   uint16_t               need,
                                   uint16_t               want,
                                   uint8_t                load,
                                   uint32_t               data_len,
                                   const uint8_t         *data);

#endif /* __XNEST__XKB_H */

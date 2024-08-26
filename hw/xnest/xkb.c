#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stddef.h>  /* for offsetof() */

#include <xcb/xcbext.h>
#include <xcb/xkb.h>
#include <xcb/xproto.h>

#include "xnest-xkb.h"

xcb_xkb_get_kbd_by_name_cookie_t
xcb_xkb_get_kbd_by_name_2 (xcb_connection_t      *c,
                         xcb_xkb_device_spec_t  deviceSpec,
                         uint16_t               need,
                         uint16_t               want,
                         uint8_t                load,
                         uint32_t               data_len,
                         const uint8_t         *data)
{
    static const xcb_protocol_request_t xcb_req = {
        .count = 4,
        .ext = &xcb_xkb_id,
        .opcode = XCB_XKB_GET_KBD_BY_NAME,
        .isvoid = 0
    };

    struct iovec xcb_parts[6];
    xcb_xkb_get_kbd_by_name_cookie_t xcb_ret;
    xcb_xkb_get_kbd_by_name_request_t xcb_out;

    xcb_out.deviceSpec = deviceSpec;
    xcb_out.need = need;
    xcb_out.want = want;
    xcb_out.load = load;
    xcb_out.pad0 = 0;

    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;
    /* uint8_t data */
    xcb_parts[4].iov_base = (char *) data;
    xcb_parts[4].iov_len = data_len * sizeof(uint8_t);
    xcb_parts[5].iov_base = 0;
    xcb_parts[5].iov_len = -xcb_parts[4].iov_len & 3;

    xcb_ret.sequence = xcb_send_request(c, XCB_REQUEST_CHECKED, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}

xcb_xkb_get_kbd_by_name_cookie_t
xcb_xkb_get_kbd_by_name_2_unchecked (xcb_connection_t      *c,
                                   xcb_xkb_device_spec_t  deviceSpec,
                                   uint16_t               need,
                                   uint16_t               want,
                                   uint8_t                load,
                                   uint32_t               data_len,
                                   const uint8_t         *data)
{
    static const xcb_protocol_request_t xcb_req = {
        .count = 4,
        .ext = &xcb_xkb_id,
        .opcode = XCB_XKB_GET_KBD_BY_NAME,
        .isvoid = 0
    };

    struct iovec xcb_parts[6];
    xcb_xkb_get_kbd_by_name_cookie_t xcb_ret;
    xcb_xkb_get_kbd_by_name_request_t xcb_out;

    xcb_out.deviceSpec = deviceSpec;
    xcb_out.need = need;
    xcb_out.want = want;
    xcb_out.load = load;
    xcb_out.pad0 = 0;

    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;
    /* uint8_t data */
    xcb_parts[4].iov_base = (char *) data;
    xcb_parts[4].iov_len = data_len * sizeof(uint8_t);
    xcb_parts[5].iov_base = 0;
    xcb_parts[5].iov_len = -xcb_parts[4].iov_len & 3;

    xcb_ret.sequence = xcb_send_request(c, 0, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}

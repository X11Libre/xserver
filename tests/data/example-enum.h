/* SCANNER TEST */

#ifndef WAYLAND_ENUM_PROTOCOL_H
#define WAYLAND_ENUM_PROTOCOL_H

#ifdef  __cplusplus
extern "C" {
#endif

#ifndef WL_DISPLAY_ERROR_ENUM
#define WL_DISPLAY_ERROR_ENUM
/**
 * @ingroup iface_wl_display
 * global error values
 *
 * These errors are global and can be emitted in response to any
 * server request.
 */
enum wl_display_error {
	/**
	 * server couldn't find object
	 */
	WL_DISPLAY_ERROR_INVALID_OBJECT = 0,
	/**
	 * method doesn't exist on the specified interface
	 */
	WL_DISPLAY_ERROR_INVALID_METHOD = 1,
	/**
	 * server is out of memory
	 */
	WL_DISPLAY_ERROR_NO_MEMORY = 2,
};
#endif /* WL_DISPLAY_ERROR_ENUM */

#ifndef WL_SHM_ERROR_ENUM
#define WL_SHM_ERROR_ENUM
/**
 * @ingroup iface_wl_shm
 * wl_shm error values
 *
 * These errors can be emitted in response to wl_shm requests.
 */
enum wl_shm_error {
	/**
	 * buffer format is not known
	 */
	WL_SHM_ERROR_INVALID_FORMAT = 0,
	/**
	 * invalid size or stride during pool or buffer creation
	 */
	WL_SHM_ERROR_INVALID_STRIDE = 1,
	/**
	 * mmapping the file descriptor failed
	 */
	WL_SHM_ERROR_INVALID_FD = 2,
};
#endif /* WL_SHM_ERROR_ENUM */

#ifndef WL_SHM_FORMAT_ENUM
#define WL_SHM_FORMAT_ENUM
/**
 * @ingroup iface_wl_shm
 * pixel formats
 *
 * This describes the memory layout of an individual pixel.
 *
 * All renderers should support argb8888 and xrgb8888 but any other
 * formats are optional and may not be supported by the particular
 * renderer in use.
 *
 * The drm format codes match the macros defined in drm_fourcc.h.
 * The formats actually supported by the compositor will be
 * reported by the format event.
 */
enum wl_shm_format {
	/**
	 * 32-bit ARGB format, [31:0] A:R:G:B 8:8:8:8 little endian
	 */
	WL_SHM_FORMAT_ARGB8888 = 0,
	/**
	 * 32-bit RGB format, [31:0] x:R:G:B 8:8:8:8 little endian
	 */
	WL_SHM_FORMAT_XRGB8888 = 1,
	/**
	 * 8-bit color index format, [7:0] C
	 */
	WL_SHM_FORMAT_C8 = 0x20203843,
	/**
	 * 8-bit RGB format, [7:0] R:G:B 3:3:2
	 */
	WL_SHM_FORMAT_RGB332 = 0x38424752,
	/**
	 * 8-bit BGR format, [7:0] B:G:R 2:3:3
	 */
	WL_SHM_FORMAT_BGR233 = 0x38524742,
	/**
	 * 16-bit xRGB format, [15:0] x:R:G:B 4:4:4:4 little endian
	 */
	WL_SHM_FORMAT_XRGB4444 = 0x32315258,
	/**
	 * 16-bit xBGR format, [15:0] x:B:G:R 4:4:4:4 little endian
	 */
	WL_SHM_FORMAT_XBGR4444 = 0x32314258,
	/**
	 * 16-bit RGBx format, [15:0] R:G:B:x 4:4:4:4 little endian
	 */
	WL_SHM_FORMAT_RGBX4444 = 0x32315852,
	/**
	 * 16-bit BGRx format, [15:0] B:G:R:x 4:4:4:4 little endian
	 */
	WL_SHM_FORMAT_BGRX4444 = 0x32315842,
	/**
	 * 16-bit ARGB format, [15:0] A:R:G:B 4:4:4:4 little endian
	 */
	WL_SHM_FORMAT_ARGB4444 = 0x32315241,
	/**
	 * 16-bit ABGR format, [15:0] A:B:G:R 4:4:4:4 little endian
	 */
	WL_SHM_FORMAT_ABGR4444 = 0x32314241,
	/**
	 * 16-bit RBGA format, [15:0] R:G:B:A 4:4:4:4 little endian
	 */
	WL_SHM_FORMAT_RGBA4444 = 0x32314152,
	/**
	 * 16-bit BGRA format, [15:0] B:G:R:A 4:4:4:4 little endian
	 */
	WL_SHM_FORMAT_BGRA4444 = 0x32314142,
	/**
	 * 16-bit xRGB format, [15:0] x:R:G:B 1:5:5:5 little endian
	 */
	WL_SHM_FORMAT_XRGB1555 = 0x35315258,
	/**
	 * 16-bit xBGR 1555 format, [15:0] x:B:G:R 1:5:5:5 little endian
	 */
	WL_SHM_FORMAT_XBGR1555 = 0x35314258,
	/**
	 * 16-bit RGBx 5551 format, [15:0] R:G:B:x 5:5:5:1 little endian
	 */
	WL_SHM_FORMAT_RGBX5551 = 0x35315852,
	/**
	 * 16-bit BGRx 5551 format, [15:0] B:G:R:x 5:5:5:1 little endian
	 */
	WL_SHM_FORMAT_BGRX5551 = 0x35315842,
	/**
	 * 16-bit ARGB 1555 format, [15:0] A:R:G:B 1:5:5:5 little endian
	 */
	WL_SHM_FORMAT_ARGB1555 = 0x35315241,
	/**
	 * 16-bit ABGR 1555 format, [15:0] A:B:G:R 1:5:5:5 little endian
	 */
	WL_SHM_FORMAT_ABGR1555 = 0x35314241,
	/**
	 * 16-bit RGBA 5551 format, [15:0] R:G:B:A 5:5:5:1 little endian
	 */
	WL_SHM_FORMAT_RGBA5551 = 0x35314152,
	/**
	 * 16-bit BGRA 5551 format, [15:0] B:G:R:A 5:5:5:1 little endian
	 */
	WL_SHM_FORMAT_BGRA5551 = 0x35314142,
	/**
	 * 16-bit RGB 565 format, [15:0] R:G:B 5:6:5 little endian
	 */
	WL_SHM_FORMAT_RGB565 = 0x36314752,
	/**
	 * 16-bit BGR 565 format, [15:0] B:G:R 5:6:5 little endian
	 */
	WL_SHM_FORMAT_BGR565 = 0x36314742,
	/**
	 * 24-bit RGB format, [23:0] R:G:B little endian
	 */
	WL_SHM_FORMAT_RGB888 = 0x34324752,
	/**
	 * 24-bit BGR format, [23:0] B:G:R little endian
	 */
	WL_SHM_FORMAT_BGR888 = 0x34324742,
	/**
	 * 32-bit xBGR format, [31:0] x:B:G:R 8:8:8:8 little endian
	 */
	WL_SHM_FORMAT_XBGR8888 = 0x34324258,
	/**
	 * 32-bit RGBx format, [31:0] R:G:B:x 8:8:8:8 little endian
	 */
	WL_SHM_FORMAT_RGBX8888 = 0x34325852,
	/**
	 * 32-bit BGRx format, [31:0] B:G:R:x 8:8:8:8 little endian
	 */
	WL_SHM_FORMAT_BGRX8888 = 0x34325842,
	/**
	 * 32-bit ABGR format, [31:0] A:B:G:R 8:8:8:8 little endian
	 */
	WL_SHM_FORMAT_ABGR8888 = 0x34324241,
	/**
	 * 32-bit RGBA format, [31:0] R:G:B:A 8:8:8:8 little endian
	 */
	WL_SHM_FORMAT_RGBA8888 = 0x34324152,
	/**
	 * 32-bit BGRA format, [31:0] B:G:R:A 8:8:8:8 little endian
	 */
	WL_SHM_FORMAT_BGRA8888 = 0x34324142,
	/**
	 * 32-bit xRGB format, [31:0] x:R:G:B 2:10:10:10 little endian
	 */
	WL_SHM_FORMAT_XRGB2101010 = 0x30335258,
	/**
	 * 32-bit xBGR format, [31:0] x:B:G:R 2:10:10:10 little endian
	 */
	WL_SHM_FORMAT_XBGR2101010 = 0x30334258,
	/**
	 * 32-bit RGBx format, [31:0] R:G:B:x 10:10:10:2 little endian
	 */
	WL_SHM_FORMAT_RGBX1010102 = 0x30335852,
	/**
	 * 32-bit BGRx format, [31:0] B:G:R:x 10:10:10:2 little endian
	 */
	WL_SHM_FORMAT_BGRX1010102 = 0x30335842,
	/**
	 * 32-bit ARGB format, [31:0] A:R:G:B 2:10:10:10 little endian
	 */
	WL_SHM_FORMAT_ARGB2101010 = 0x30335241,
	/**
	 * 32-bit ABGR format, [31:0] A:B:G:R 2:10:10:10 little endian
	 */
	WL_SHM_FORMAT_ABGR2101010 = 0x30334241,
	/**
	 * 32-bit RGBA format, [31:0] R:G:B:A 10:10:10:2 little endian
	 */
	WL_SHM_FORMAT_RGBA1010102 = 0x30334152,
	/**
	 * 32-bit BGRA format, [31:0] B:G:R:A 10:10:10:2 little endian
	 */
	WL_SHM_FORMAT_BGRA1010102 = 0x30334142,
	/**
	 * packed YCbCr format, [31:0] Cr0:Y1:Cb0:Y0 8:8:8:8 little endian
	 */
	WL_SHM_FORMAT_YUYV = 0x56595559,
	/**
	 * packed YCbCr format, [31:0] Cb0:Y1:Cr0:Y0 8:8:8:8 little endian
	 */
	WL_SHM_FORMAT_YVYU = 0x55595659,
	/**
	 * packed YCbCr format, [31:0] Y1:Cr0:Y0:Cb0 8:8:8:8 little endian
	 */
	WL_SHM_FORMAT_UYVY = 0x59565955,
	/**
	 * packed YCbCr format, [31:0] Y1:Cb0:Y0:Cr0 8:8:8:8 little endian
	 */
	WL_SHM_FORMAT_VYUY = 0x59555956,
	/**
	 * packed AYCbCr format, [31:0] A:Y:Cb:Cr 8:8:8:8 little endian
	 */
	WL_SHM_FORMAT_AYUV = 0x56555941,
	/**
	 * 2 plane YCbCr Cr:Cb format, 2x2 subsampled Cr:Cb plane
	 */
	WL_SHM_FORMAT_NV12 = 0x3231564e,
	/**
	 * 2 plane YCbCr Cb:Cr format, 2x2 subsampled Cb:Cr plane
	 */
	WL_SHM_FORMAT_NV21 = 0x3132564e,
	/**
	 * 2 plane YCbCr Cr:Cb format, 2x1 subsampled Cr:Cb plane
	 */
	WL_SHM_FORMAT_NV16 = 0x3631564e,
	/**
	 * 2 plane YCbCr Cb:Cr format, 2x1 subsampled Cb:Cr plane
	 */
	WL_SHM_FORMAT_NV61 = 0x3136564e,
	/**
	 * 3 plane YCbCr format, 4x4 subsampled Cb (1) and Cr (2) planes
	 */
	WL_SHM_FORMAT_YUV410 = 0x39565559,
	/**
	 * 3 plane YCbCr format, 4x4 subsampled Cr (1) and Cb (2) planes
	 */
	WL_SHM_FORMAT_YVU410 = 0x39555659,
	/**
	 * 3 plane YCbCr format, 4x1 subsampled Cb (1) and Cr (2) planes
	 */
	WL_SHM_FORMAT_YUV411 = 0x31315559,
	/**
	 * 3 plane YCbCr format, 4x1 subsampled Cr (1) and Cb (2) planes
	 */
	WL_SHM_FORMAT_YVU411 = 0x31315659,
	/**
	 * 3 plane YCbCr format, 2x2 subsampled Cb (1) and Cr (2) planes
	 */
	WL_SHM_FORMAT_YUV420 = 0x32315559,
	/**
	 * 3 plane YCbCr format, 2x2 subsampled Cr (1) and Cb (2) planes
	 */
	WL_SHM_FORMAT_YVU420 = 0x32315659,
	/**
	 * 3 plane YCbCr format, 2x1 subsampled Cb (1) and Cr (2) planes
	 */
	WL_SHM_FORMAT_YUV422 = 0x36315559,
	/**
	 * 3 plane YCbCr format, 2x1 subsampled Cr (1) and Cb (2) planes
	 */
	WL_SHM_FORMAT_YVU422 = 0x36315659,
	/**
	 * 3 plane YCbCr format, non-subsampled Cb (1) and Cr (2) planes
	 */
	WL_SHM_FORMAT_YUV444 = 0x34325559,
	/**
	 * 3 plane YCbCr format, non-subsampled Cr (1) and Cb (2) planes
	 */
	WL_SHM_FORMAT_YVU444 = 0x34325659,
};
#endif /* WL_SHM_FORMAT_ENUM */

#ifndef WL_DATA_OFFER_ERROR_ENUM
#define WL_DATA_OFFER_ERROR_ENUM
enum wl_data_offer_error {
	/**
	 * finish request was called untimely
	 */
	WL_DATA_OFFER_ERROR_INVALID_FINISH = 0,
	/**
	 * action mask contains invalid values
	 */
	WL_DATA_OFFER_ERROR_INVALID_ACTION_MASK = 1,
	/**
	 * action argument has an invalid value
	 */
	WL_DATA_OFFER_ERROR_INVALID_ACTION = 2,
	/**
	 * offer doesn't accept this request
	 */
	WL_DATA_OFFER_ERROR_INVALID_OFFER = 3,
};
#endif /* WL_DATA_OFFER_ERROR_ENUM */

#ifndef WL_DATA_SOURCE_ERROR_ENUM
#define WL_DATA_SOURCE_ERROR_ENUM
enum wl_data_source_error {
	/**
	 * action mask contains invalid values
	 */
	WL_DATA_SOURCE_ERROR_INVALID_ACTION_MASK = 0,
	/**
	 * source doesn't accept this request
	 */
	WL_DATA_SOURCE_ERROR_INVALID_SOURCE = 1,
};
#endif /* WL_DATA_SOURCE_ERROR_ENUM */

#ifndef WL_DATA_DEVICE_ERROR_ENUM
#define WL_DATA_DEVICE_ERROR_ENUM
enum wl_data_device_error {
	/**
	 * given wl_surface has another role
	 */
	WL_DATA_DEVICE_ERROR_ROLE = 0,
};
#endif /* WL_DATA_DEVICE_ERROR_ENUM */

#ifndef WL_DATA_DEVICE_MANAGER_DND_ACTION_ENUM
#define WL_DATA_DEVICE_MANAGER_DND_ACTION_ENUM
/**
 * @ingroup iface_wl_data_device_manager
 * drag and drop actions
 *
 * This is a bitmask of the available/preferred actions in a
 * drag-and-drop operation.
 *
 * In the compositor, the selected action is a result of matching the
 * actions offered by the source and destination sides.  "action" events
 * with a "none" action will be sent to both source and destination if
 * there is no match. All further checks will effectively happen on
 * (source actions ∩ destination actions).
 *
 * In addition, compositors may also pick different actions in
 * reaction to key modifiers being pressed. One common design that
 * is used in major toolkits (and the behavior recommended for
 * compositors) is:
 *
 * - If no modifiers are pressed, the first match (in bit order)
 * will be used.
 * - Pressing Shift selects "move", if enabled in the mask.
 * - Pressing Control selects "copy", if enabled in the mask.
 *
 * Behavior beyond that is considered implementation-dependent.
 * Compositors may for example bind other modifiers (like Alt/Meta)
 * or drags initiated with other buttons than BTN_LEFT to specific
 * actions (e.g. "ask").
 */
enum wl_data_device_manager_dnd_action {
	/**
	 * no action
	 */
	WL_DATA_DEVICE_MANAGER_DND_ACTION_NONE = 0,
	/**
	 * copy action
	 */
	WL_DATA_DEVICE_MANAGER_DND_ACTION_COPY = 1,
	/**
	 * move action
	 */
	WL_DATA_DEVICE_MANAGER_DND_ACTION_MOVE = 2,
	/**
	 * ask action
	 */
	WL_DATA_DEVICE_MANAGER_DND_ACTION_ASK = 4,
};
#endif /* WL_DATA_DEVICE_MANAGER_DND_ACTION_ENUM */

#ifndef WL_SHELL_ERROR_ENUM
#define WL_SHELL_ERROR_ENUM
enum wl_shell_error {
	/**
	 * given wl_surface has another role
	 */
	WL_SHELL_ERROR_ROLE = 0,
};
#endif /* WL_SHELL_ERROR_ENUM */

#ifndef WL_SHELL_SURFACE_RESIZE_ENUM
#define WL_SHELL_SURFACE_RESIZE_ENUM
/**
 * @ingroup iface_wl_shell_surface
 * edge values for resizing
 *
 * These values are used to indicate which edge of a surface
 * is being dragged in a resize operation. The server may
 * use this information to adapt its behavior, e.g. choose
 * an appropriate cursor image.
 */
enum wl_shell_surface_resize {
	/**
	 * no edge
	 */
	WL_SHELL_SURFACE_RESIZE_NONE = 0,
	/**
	 * top edge
	 */
	WL_SHELL_SURFACE_RESIZE_TOP = 1,
	/**
	 * bottom edge
	 */
	WL_SHELL_SURFACE_RESIZE_BOTTOM = 2,
	/**
	 * left edge
	 */
	WL_SHELL_SURFACE_RESIZE_LEFT = 4,
	/**
	 * top and left edges
	 */
	WL_SHELL_SURFACE_RESIZE_TOP_LEFT = 5,
	/**
	 * bottom and left edges
	 */
	WL_SHELL_SURFACE_RESIZE_BOTTOM_LEFT = 6,
	/**
	 * right edge
	 */
	WL_SHELL_SURFACE_RESIZE_RIGHT = 8,
	/**
	 * top and right edges
	 */
	WL_SHELL_SURFACE_RESIZE_TOP_RIGHT = 9,
	/**
	 * bottom and right edges
	 */
	WL_SHELL_SURFACE_RESIZE_BOTTOM_RIGHT = 10,
};
#endif /* WL_SHELL_SURFACE_RESIZE_ENUM */

#ifndef WL_SHELL_SURFACE_TRANSIENT_ENUM
#define WL_SHELL_SURFACE_TRANSIENT_ENUM
/**
 * @ingroup iface_wl_shell_surface
 * details of transient behaviour
 *
 * These flags specify details of the expected behaviour
 * of transient surfaces. Used in the set_transient request.
 */
enum wl_shell_surface_transient {
	/**
	 * do not set keyboard focus
	 */
	WL_SHELL_SURFACE_TRANSIENT_INACTIVE = 0x1,
};
#endif /* WL_SHELL_SURFACE_TRANSIENT_ENUM */

#ifndef WL_SHELL_SURFACE_FULLSCREEN_METHOD_ENUM
#define WL_SHELL_SURFACE_FULLSCREEN_METHOD_ENUM
/**
 * @ingroup iface_wl_shell_surface
 * different method to set the surface fullscreen
 *
 * Hints to indicate to the compositor how to deal with a conflict
 * between the dimensions of the surface and the dimensions of the
 * output. The compositor is free to ignore this parameter.
 */
enum wl_shell_surface_fullscreen_method {
	/**
	 * no preference, apply default policy
	 */
	WL_SHELL_SURFACE_FULLSCREEN_METHOD_DEFAULT = 0,
	/**
	 * scale, preserve the surface's aspect ratio and center on output
	 */
	WL_SHELL_SURFACE_FULLSCREEN_METHOD_SCALE = 1,
	/**
	 * switch output mode to the smallest mode that can fit the surface, add black borders to compensate size mismatch
	 */
	WL_SHELL_SURFACE_FULLSCREEN_METHOD_DRIVER = 2,
	/**
	 * no upscaling, center on output and add black borders to compensate size mismatch
	 */
	WL_SHELL_SURFACE_FULLSCREEN_METHOD_FILL = 3,
};
#endif /* WL_SHELL_SURFACE_FULLSCREEN_METHOD_ENUM */

#ifndef WL_SURFACE_ERROR_ENUM
#define WL_SURFACE_ERROR_ENUM
/**
 * @ingroup iface_wl_surface
 * wl_surface error values
 *
 * These errors can be emitted in response to wl_surface requests.
 */
enum wl_surface_error {
	/**
	 * buffer scale value is invalid
	 */
	WL_SURFACE_ERROR_INVALID_SCALE = 0,
	/**
	 * buffer transform value is invalid
	 */
	WL_SURFACE_ERROR_INVALID_TRANSFORM = 1,
};
#endif /* WL_SURFACE_ERROR_ENUM */

#ifndef WL_SEAT_CAPABILITY_ENUM
#define WL_SEAT_CAPABILITY_ENUM
/**
 * @ingroup iface_wl_seat
 * seat capability bitmask
 *
 * This is a bitmask of capabilities this seat has; if a member is
 * set, then it is present on the seat.
 */
enum wl_seat_capability {
	/**
	 * the seat has pointer devices
	 */
	WL_SEAT_CAPABILITY_POINTER = 1,
	/**
	 * the seat has one or more keyboards
	 */
	WL_SEAT_CAPABILITY_KEYBOARD = 2,
	/**
	 * the seat has touch devices
	 */
	WL_SEAT_CAPABILITY_TOUCH = 4,
};
#endif /* WL_SEAT_CAPABILITY_ENUM */

#ifndef WL_POINTER_ERROR_ENUM
#define WL_POINTER_ERROR_ENUM
enum wl_pointer_error {
	/**
	 * given wl_surface has another role
	 */
	WL_POINTER_ERROR_ROLE = 0,
};
#endif /* WL_POINTER_ERROR_ENUM */

#ifndef WL_POINTER_BUTTON_STATE_ENUM
#define WL_POINTER_BUTTON_STATE_ENUM
/**
 * @ingroup iface_wl_pointer
 * physical button state
 *
 * Describes the physical state of a button that produced the button
 * event.
 */
enum wl_pointer_button_state {
	/**
	 * the button is not pressed
	 */
	WL_POINTER_BUTTON_STATE_RELEASED = 0,
	/**
	 * the button is pressed
	 */
	WL_POINTER_BUTTON_STATE_PRESSED = 1,
};
#endif /* WL_POINTER_BUTTON_STATE_ENUM */

#ifndef WL_POINTER_AXIS_ENUM
#define WL_POINTER_AXIS_ENUM
/**
 * @ingroup iface_wl_pointer
 * axis types
 *
 * Describes the axis types of scroll events.
 */
enum wl_pointer_axis {
	/**
	 * vertical axis
	 */
	WL_POINTER_AXIS_VERTICAL_SCROLL = 0,
	/**
	 * horizontal axis
	 */
	WL_POINTER_AXIS_HORIZONTAL_SCROLL = 1,
};
#endif /* WL_POINTER_AXIS_ENUM */

#ifndef WL_POINTER_AXIS_SOURCE_ENUM
#define WL_POINTER_AXIS_SOURCE_ENUM
/**
 * @ingroup iface_wl_pointer
 * axis source types
 *
 * Describes the source types for axis events. This indicates to the
 * client how an axis event was physically generated; a client may
 * adjust the user interface accordingly. For example, scroll events
 * from a "finger" source may be in a smooth coordinate space with
 * kinetic scrolling whereas a "wheel" source may be in discrete steps
 * of a number of lines.
 */
enum wl_pointer_axis_source {
	/**
	 * a physical wheel rotation
	 */
	WL_POINTER_AXIS_SOURCE_WHEEL = 0,
	/**
	 * finger on a touch surface
	 */
	WL_POINTER_AXIS_SOURCE_FINGER = 1,
	/**
	 * continuous coordinate space
	 *
	 * A device generating events in a continuous coordinate space,
	 * but using something other than a finger. One example for this
	 * source is button-based scrolling where the vertical motion of a
	 * device is converted to scroll events while a button is held
	 * down.
	 */
	WL_POINTER_AXIS_SOURCE_CONTINUOUS = 2,
	/**
	 * a physical wheel tilt
	 *
	 * Indicates that the actual device is a wheel but the scroll
	 * event is not caused by a rotation but a (usually sideways) tilt
	 * of the wheel.
	 * @since 6
	 */
	WL_POINTER_AXIS_SOURCE_WHEEL_TILT = 3,
};
/**
 * @ingroup iface_wl_pointer
 */
#define WL_POINTER_AXIS_SOURCE_WHEEL_TILT_SINCE_VERSION 6
#endif /* WL_POINTER_AXIS_SOURCE_ENUM */

#ifndef WL_KEYBOARD_KEYMAP_FORMAT_ENUM
#define WL_KEYBOARD_KEYMAP_FORMAT_ENUM
/**
 * @ingroup iface_wl_keyboard
 * keyboard mapping format
 *
 * This specifies the format of the keymap provided to the
 * client with the wl_keyboard.keymap event.
 */
enum wl_keyboard_keymap_format {
	/**
	 * no keymap; client must understand how to interpret the raw keycode
	 */
	WL_KEYBOARD_KEYMAP_FORMAT_NO_KEYMAP = 0,
	/**
	 * libxkbcommon compatible; to determine the xkb keycode, clients must add 8 to the key event keycode
	 */
	WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1 = 1,
};
#endif /* WL_KEYBOARD_KEYMAP_FORMAT_ENUM */

#ifndef WL_KEYBOARD_KEY_STATE_ENUM
#define WL_KEYBOARD_KEY_STATE_ENUM
/**
 * @ingroup iface_wl_keyboard
 * physical key state
 *
 * Describes the physical state of a key that produced the key event.
 */
enum wl_keyboard_key_state {
	/**
	 * key is not pressed
	 */
	WL_KEYBOARD_KEY_STATE_RELEASED = 0,
	/**
	 * key is pressed
	 */
	WL_KEYBOARD_KEY_STATE_PRESSED = 1,
};
#endif /* WL_KEYBOARD_KEY_STATE_ENUM */

#ifndef WL_OUTPUT_SUBPIXEL_ENUM
#define WL_OUTPUT_SUBPIXEL_ENUM
/**
 * @ingroup iface_wl_output
 * subpixel geometry information
 *
 * This enumeration describes how the physical
 * pixels on an output are laid out.
 */
enum wl_output_subpixel {
	/**
	 * unknown geometry
	 */
	WL_OUTPUT_SUBPIXEL_UNKNOWN = 0,
	/**
	 * no geometry
	 */
	WL_OUTPUT_SUBPIXEL_NONE = 1,
	/**
	 * horizontal RGB
	 */
	WL_OUTPUT_SUBPIXEL_HORIZONTAL_RGB = 2,
	/**
	 * horizontal BGR
	 */
	WL_OUTPUT_SUBPIXEL_HORIZONTAL_BGR = 3,
	/**
	 * vertical RGB
	 */
	WL_OUTPUT_SUBPIXEL_VERTICAL_RGB = 4,
	/**
	 * vertical BGR
	 */
	WL_OUTPUT_SUBPIXEL_VERTICAL_BGR = 5,
};
#endif /* WL_OUTPUT_SUBPIXEL_ENUM */

#ifndef WL_OUTPUT_TRANSFORM_ENUM
#define WL_OUTPUT_TRANSFORM_ENUM
/**
 * @ingroup iface_wl_output
 * transform from framebuffer to output
 *
 * This describes the transform that a compositor will apply to a
 * surface to compensate for the rotation or mirroring of an
 * output device.
 *
 * The flipped values correspond to an initial flip around a
 * vertical axis followed by rotation.
 *
 * The purpose is mainly to allow clients to render accordingly and
 * tell the compositor, so that for fullscreen surfaces, the
 * compositor will still be able to scan out directly from client
 * surfaces.
 */
enum wl_output_transform {
	/**
	 * no transform
	 */
	WL_OUTPUT_TRANSFORM_NORMAL = 0,
	/**
	 * 90 degrees counter-clockwise
	 */
	WL_OUTPUT_TRANSFORM_90 = 1,
	/**
	 * 180 degrees counter-clockwise
	 */
	WL_OUTPUT_TRANSFORM_180 = 2,
	/**
	 * 270 degrees counter-clockwise
	 */
	WL_OUTPUT_TRANSFORM_270 = 3,
	/**
	 * 180 degree flip around a vertical axis
	 */
	WL_OUTPUT_TRANSFORM_FLIPPED = 4,
	/**
	 * flip and rotate 90 degrees counter-clockwise
	 */
	WL_OUTPUT_TRANSFORM_FLIPPED_90 = 5,
	/**
	 * flip and rotate 180 degrees counter-clockwise
	 */
	WL_OUTPUT_TRANSFORM_FLIPPED_180 = 6,
	/**
	 * flip and rotate 270 degrees counter-clockwise
	 */
	WL_OUTPUT_TRANSFORM_FLIPPED_270 = 7,
};
#endif /* WL_OUTPUT_TRANSFORM_ENUM */

#ifndef WL_OUTPUT_MODE_ENUM
#define WL_OUTPUT_MODE_ENUM
/**
 * @ingroup iface_wl_output
 * mode information
 *
 * These flags describe properties of an output mode.
 * They are used in the flags bitfield of the mode event.
 */
enum wl_output_mode {
	/**
	 * indicates this is the current mode
	 */
	WL_OUTPUT_MODE_CURRENT = 0x1,
	/**
	 * indicates this is the preferred mode
	 */
	WL_OUTPUT_MODE_PREFERRED = 0x2,
};
#endif /* WL_OUTPUT_MODE_ENUM */

#ifndef WL_SUBCOMPOSITOR_ERROR_ENUM
#define WL_SUBCOMPOSITOR_ERROR_ENUM
enum wl_subcompositor_error {
	/**
	 * the to-be sub-surface is invalid
	 */
	WL_SUBCOMPOSITOR_ERROR_BAD_SURFACE = 0,
};
#endif /* WL_SUBCOMPOSITOR_ERROR_ENUM */

#ifndef WL_SUBSURFACE_ERROR_ENUM
#define WL_SUBSURFACE_ERROR_ENUM
enum wl_subsurface_error {
	/**
	 * wl_surface is not a sibling or the parent
	 */
	WL_SUBSURFACE_ERROR_BAD_SURFACE = 0,
};
#endif /* WL_SUBSURFACE_ERROR_ENUM */

#ifdef  __cplusplus
}
#endif

#endif

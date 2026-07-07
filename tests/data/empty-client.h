/* SCANNER TEST */

#ifndef EMPTY_CLIENT_PROTOCOL_H
#define EMPTY_CLIENT_PROTOCOL_H

#include <stdint.h>
#include <stddef.h>
#include "wayland-client.h"

#ifdef  __cplusplus
extern "C" {
#endif

/**
 * @page page_empty The empty protocol
 * @section page_ifaces_empty Interfaces
 * - @subpage page_iface_empty - 
 */
struct empty;

#ifndef EMPTY_INTERFACE
#define EMPTY_INTERFACE
/**
 * @page page_iface_empty empty
 * @section page_iface_empty_api API
 * See @ref iface_empty.
 */
/**
 * @defgroup iface_empty The empty interface
 */
extern const struct wl_interface empty_interface;
#endif

#define EMPTY_EMPTY 0


/**
 * @ingroup iface_empty
 */
#define EMPTY_EMPTY_SINCE_VERSION 1

/** @ingroup iface_empty */
static inline void
empty_set_user_data(struct empty *empty, void *user_data)
{
	wl_proxy_set_user_data((struct wl_proxy *) empty, user_data);
}

/** @ingroup iface_empty */
static inline void *
empty_get_user_data(struct empty *empty)
{
	return wl_proxy_get_user_data((struct wl_proxy *) empty);
}

static inline uint32_t
empty_get_version(struct empty *empty)
{
	return wl_proxy_get_version((struct wl_proxy *) empty);
}

/** @ingroup iface_empty */
static inline void
empty_destroy(struct empty *empty)
{
	wl_proxy_destroy((struct wl_proxy *) empty);
}

/**
 * @ingroup iface_empty
 */
static inline void
empty_empty(struct empty *empty)
{
	wl_proxy_marshal_flags((struct wl_proxy *) empty,
			 EMPTY_EMPTY, NULL, wl_proxy_get_version((struct wl_proxy *) empty), 0);
}

#ifdef  __cplusplus
}
#endif

#endif

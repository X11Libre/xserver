/* SCANNER TEST */

#ifndef EMPTY_SERVER_PROTOCOL_H
#define EMPTY_SERVER_PROTOCOL_H

#include <stdint.h>
#include <stddef.h>
#include "wayland-server.h"

#ifdef  __cplusplus
extern "C" {
#endif

struct wl_client;
struct wl_resource;

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

/**
 * @ingroup iface_empty
 * @struct empty_interface
 */
struct empty_interface {
	/**
	 */
	void (*empty)(struct wl_client *client,
		      struct wl_resource *resource);
};


/**
 * @ingroup iface_empty
 */
#define EMPTY_EMPTY_SINCE_VERSION 1

#ifdef  __cplusplus
}
#endif

#endif

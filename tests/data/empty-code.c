/* SCANNER TEST */

#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include "wayland-util.h"


static const struct wl_interface *empty_types[] = {
};

static const struct wl_message empty_requests[] = {
	{ "empty", "", empty_types + 0 },
};

WL_EXPORT const struct wl_interface empty_interface = {
	"empty", 1,
	1, empty_requests,
	0, NULL,
};


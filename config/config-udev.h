#ifndef _XSERVER_CONFIG_UDEV_H
#define _XSERVER_CONFIG_UDEV_H

#ifdef CONFIG_UDEV

#include "config/hotplug_priv.h"

int config_udev_init(void);
void config_udev_fini(void);
int config_udev_pre_init(void);
void config_udev_odev_probe(config_odev_probe_proc_ptr probe_callback);

#else

static inline int config_udev_init(void) { return 1; }
static inline void config_udev_fini(void) {}
static inline int config_udev_pre_init(void) { return 1; }

#endif

#endif /* _XSERVER_CONFIG_UDEV_H */

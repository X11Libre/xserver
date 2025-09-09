#ifndef _XSERVER_CONFIG_HAL_H
#define _XSERVER_CONFIG_HAL_H

#ifdef CONFIG_HAL

int config_hal_init(void);
void config_hal_fini(void);

#else

static inline int config_hal_init(void) { return 1; }
static inline void config_hal_fini(void) {}

#endif

#endif /* _XSERVER_CONFIG_HAL_H */

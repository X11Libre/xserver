#ifndef _XSERVER_CONFIG_WSCONS_H
#define _XSERVER_CONFIG_WSCONS_H

#ifdef CONFIG_WSCONS

int config_wscons_init(void);
void config_wscons_fini(void);

#else

static inline int config_wscons_init(void) { return 1; }
static inline void config_wscons_fini(void) {}

#endif

#endif /* _XSERVER_CONFIG_WSCONS_H */

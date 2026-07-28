/* SPDX-License-Identifier: X11 OR MIT OR AGPL-3.0-or-later */
/* Copyright (C) 2026 Enrico Weigelt, metux IT consult <info@metux.net> */
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

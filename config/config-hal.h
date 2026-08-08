/* SPDX-License-Identifier: X11 OR MIT OR AGPL-3.0-or-later */
/* Copyright (C) 2026 Enrico Weigelt, metux IT consult <info@metux.net> */
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

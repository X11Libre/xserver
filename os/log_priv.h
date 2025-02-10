/* SPDX-License-Identifier: MIT OR X11
 *
 * Copyright © 2024 Enrico Weigelt, metux IT consult <info@metux.net>
 */
#ifndef _XSERVER_LOG_PRIV_H
#define _XSERVER_LOG_PRIV_H

#include <sys/types.h>
#include <X11/Xdefs.h>

/**
 * @brief force fsync() on each log write
 *
 * If set to TRUE, force fsync() on each log write.
 */
extern Bool logSync;

#endif /* _XSERVER_LOG_PRIV_H */

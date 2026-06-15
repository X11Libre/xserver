/* SPDX-License-Identifier: MIT OR X11 OR AGPL-3.0-or-later
 *
 * Copyright © 2024 Enrico Weigelt, metux IT consult <info@metux.net>
 */
#ifndef _XSERVER_DIX_ATOM_PRIV_H
#define _XSERVER_DIX_ATOM_PRIV_H

/*
 * @brief initialize atom table
 */
void InitAtoms(void);

/*
 * @brief free all atoms and atom table
 */
void FreeAllAtoms(void);

#endif /* _XSERVER_DIX_ATOM_PRIV_H */

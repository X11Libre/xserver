/* SPDX-License-Identifier: MIT OR X11 OR AGPL-3.0-or-later
 *
 * Copyright © 2024 Enrico Weigelt, metux IT consult <info@metux.net>
 */
#ifndef _XSERVER_SERVERLOCK_H
#define _XSERVER_SERVERLOCK_H

void LockServer(void);
void UnlockServer(void);
void DisableServerLock(void);
void LockServerUseMsg(void);

#endif /* _XSERVER_SERVERLOCK_H */

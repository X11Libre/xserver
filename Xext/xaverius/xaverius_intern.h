/* SPDX-License-Identifier: X11 OR MIT OR AGPL-3.0-or-later
 *
 * Copyright © 2026 Enrico Weigelt, metux IT consult <info@metux.net>
 *
 * Xaverius internal definitions shared between source files within the extension.
 */
#ifndef _XLIBRE_XAVERIUS_INTERN_H
#define _XLIBRE_XAVERIUS_INTERN_H

#include <X11/Xdefs.h>

#include "include/privates.h"
#include "include/xlibre_ptrtypes.h"

extern int XaveriusEventBase;

extern DevPrivateKeyRec XaveriusScreenPrivateKeyRec;

/* Global property atoms (initialized in init.c) */
extern Atom xaverius_atom_device_uuid;
extern Atom xaverius_atom_driver_uuid;
extern Atom xaverius_atom_max_work_group_invocations;
extern Atom xaverius_atom_max_work_group_size;
extern Atom xaverius_atom_timeline_semaphore;

int ProcXaveriusDispatch(ClientPtr client);

#endif /* _XLIBRE_XAVERIUS_INTERN_H */

/* SPDX-License-Identifier: MIT OR X11
 *
 * Copyright © 2024 Enrico Weigelt, metux IT consult <info@metux.net>
 */
#ifndef _XSERVER__XF86XINPUT_H
#define _XSERVER__XF86XINPUT_H

#include "xf86Xinput.h"

extern InputInfoPtr xf86InputDevs;

int xf86NewInputDevice(InputInfoPtr pInfo, DeviceIntPtr *pdev, BOOL is_auto);
InputInfoPtr xf86AllocateInput(void);

void xf86InputEnableVTProbe(void);

InputDriverPtr xf86LookupInputDriver(const char *name);

InputInfoPtr xf86LookupInput(const char *name);

void xf86AddInputEventDrainCallback(CallbackProcPtr callback, void *param);

void xf86RemoveInputEventDrainCallback(CallbackProcPtr callback, void *param);

Bool MatchAttrToken(const char *attr, struct xorg_list *groups);

#endif /* _XSERVER__XF86XINPUT_H */

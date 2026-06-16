/* SPDX-License-Identifier: MIT OR X11
 *
 * Copyright © 2024 Enrico Weigelt, metux IT consult <info@metux.net>
 *
 * @brief: prototypes for the individual request handlers
 */
#ifndef _XSERVER_RANDR_RRDISPATCH_H
#define _XSERVER_RANDR_RRDISPATCH_H

#include "include/dix.h"

/* screen related dispatch */
int ProcRRGetScreenSizeRange(ClientPtr client);
int ProcRRSetScreenSize(ClientPtr client);
int ProcRRGetScreenResources(ClientPtr client);
int ProcRRGetScreenResourcesCurrent(ClientPtr client);
int ProcRRSetScreenConfig(ClientPtr client);
int ProcRRGetScreenInfo(ClientPtr client);

/* crtc related dispatch */
int ProcRRGetCrtcInfo(ClientPtr client);
int ProcRRSetCrtcConfig(ClientPtr client);
int ProcRRGetCrtcGammaSize(ClientPtr client);
int ProcRRGetCrtcGamma(ClientPtr client);
int ProcRRSetCrtcGamma(ClientPtr client);
int ProcRRSetCrtcTransform(ClientPtr client);
int ProcRRGetCrtcTransform(ClientPtr client);

/* mode related dispatch */
int ProcRRCreateMode(ClientPtr client);
int ProcRRDestroyMode(ClientPtr client);
int ProcRRAddOutputMode(ClientPtr client);
int ProcRRDeleteOutputMode(ClientPtr client);

/* output related dispatch */
int ProcRRGetOutputInfo(ClientPtr client);
int ProcRRSetOutputPrimary(ClientPtr client);
int ProcRRGetOutputPrimary(ClientPtr client);
int ProcRRChangeOutputProperty(ClientPtr client);
int ProcRRGetOutputProperty(ClientPtr client);
int ProcRRListOutputProperties(ClientPtr client);
int ProcRRQueryOutputProperty(ClientPtr client);
int ProcRRConfigureOutputProperty(ClientPtr client);
int ProcRRDeleteOutputProperty(ClientPtr client);

/* provider related dispatch */
int ProcRRGetProviders(ClientPtr client);
int ProcRRGetProviderInfo(ClientPtr client);
int ProcRRSetProviderOutputSource(ClientPtr client);
int ProcRRSetProviderOffloadSink(ClientPtr client);
int ProcRRGetProviderProperty(ClientPtr client);
int ProcRRListProviderProperties(ClientPtr client);
int ProcRRQueryProviderProperty(ClientPtr client);
int ProcRRConfigureProviderProperty(ClientPtr client);
int ProcRRChangeProviderProperty(ClientPtr client);
int ProcRRDeleteProviderProperty(ClientPtr client);

/* monitor related dispatch */
int ProcRRGetMonitors(ClientPtr client);
int ProcRRSetMonitor(ClientPtr client);
int ProcRRDeleteMonitor(ClientPtr client);

int ProcRRGetPanning(ClientPtr client);
int ProcRRSetPanning(ClientPtr client);

int ProcRRCreateLease(ClientPtr client);
int ProcRRFreeLease(ClientPtr client);

int ProcRRQueryVersion(ClientPtr client);
int ProcRRSelectInput(ClientPtr client);

int ProcRRDispatch(ClientPtr client);

#endif /* _XSERVER_RANDR_RRDISPATCH_H */

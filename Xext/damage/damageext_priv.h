/* SPDX-License-Identifier: MIT OR X11
 *
 * @copyright Enrico Weigelt, metux IT consult <info@metux.net>
 *
 * Entry points for the generic damage extension.
 * (not part of SDK, not available to external modules).
 */
#ifndef __XLIBRE_XEXT_DAMAGEEXT_PRIV_H
#define __XLIBRE_XEXT_DAMAGEEXT_PRIV_H

#include <stdbool.h>

#include "include/dix.h"

/*
 * Tell damage extension that upcoming damage events for given clients
 * are critical output (thus need to be sent out fast) - or reverse it.
 *
 * Internally maintains a counter that's either increased or decreased
 * by each call of this function. If the counter is above zero, events
 * are sent as critical output.
 *
 * @param pClient   pointer to the affected client
 * @param critical  "true" - increase the counter, otherwise decrease it.
 */
void DamageExtSetCritical(ClientPtr pClient, bool critical);

/*
 * Initialize PanoramiX specific data structures for the damage extension.
 * Only called by PanoramiX extension, when it's initialized and ready run.
 */
void PanoramiXDamageInit(void);

/*
 * Reset/De-Init PanoramiX specific data strucures for the damage extension.
 * Only called by PanoramiX extension, right before it's shutting down.
 */
void PanoramiXDamageReset(void);

#endif /* __XLIBRE_XEXT_DAMAGEEXT_PRIV_H */

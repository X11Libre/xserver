/* SPDX-License-Identifier: MIT OR X11
 *
 * Copyright © 2024 Enrico Weigelt, metux IT consult <info@metux.net>
 *
 * @brief
 * This header holds forward definitions for pointer types used in many places.
 * Helpful for uncluttering the includes a bit, so we have less complex dependencies.
 *
 * External drivers rarely have a reason for directly including it.
 */
#ifndef _XLIBRE_SDK_PTRTYPES_H
#define _XLIBRE_SDK_PTRTYPES_H

struct _Client;
typedef struct _Client *ClientPtr;
typedef struct _Client ClientRec;

struct _ClientId;
typedef struct _ClientId *ClientIdPtr;

struct _Window;
typedef struct _Window *WindowPtr;
typedef struct _Window WindowRec;

struct _ScrnInfoRec;
typedef struct _ScrnInfoRec *ScrnInfoPtr;
typedef struct _ScrnInfoRec ScrnInfoRec;

#endif /* _XLIBRE_SDK_PTRTYPES_H */

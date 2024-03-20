/* SPDX-License-Identifier: MIT OR X11
 *
 * Copyright © 2024 Enrico Weigelt, metux IT consult <info@metux.net>
 * Copyright © 2006-2011 Simon Thum             simon dot thum at gmx dot de
 */
#ifndef _XSERVER_POINTERVELOCITY_PRIV_H
#define _XSERVER_POINTERVELOCITY_PRIV_H

#include <input.h>

#include "ptrveloc.h"

/* fwd */
struct _DeviceVelocityRec;

/**
 * a motion history, with just enough information to
 * calc mean velocity and decide which motion was along
 * a more or less straight line
 */
struct _MotionTracker {
    double dx, dy;              /* accumulated delta for each axis */
    int time;                   /* time of creation */
    int dir;                    /* initial direction bitfield */
};

/**
 * contains the run-time data for the predictable scheme, that is, a
 * DeviceVelocityPtr and the property handlers.
 */
typedef struct _PredictableAccelSchemeRec {
    DeviceVelocityPtr vel;
    long *prop_handlers;
    int num_prop_handlers;
} PredictableAccelSchemeRec, *PredictableAccelSchemePtr;

void AccelerationDefaultCleanup(DeviceIntPtr dev);

Bool InitPredictableAccelerationScheme(DeviceIntPtr dev,
                                       struct _ValuatorAccelerationRec *protoScheme);

void acceleratePointerPredictable(DeviceIntPtr dev, ValuatorMask *val,
                                  CARD32 evtime);

void acceleratePointerLightweight(DeviceIntPtr dev, ValuatorMask *val,
                                  CARD32 evtime);

void InitTrackers(DeviceVelocityPtr vel, int ntracker);

#endif /* _XSERVER_POINTERVELOCITY_PRIV_H */

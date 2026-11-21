/* SPDX-License-Identifier: MIT */

/*
 * Private helpers for the XTEST extension.
 */

#ifndef XTEST_PRIV_H
#define XTEST_PRIV_H

#include <X11/X.h>
#include <X11/extensions/XIproto.h>

#include "inputstr.h"

/*
 * Validate and copy one DeviceValuator event. Keep all bounds checks here,
 * before the first write to the fixed-size valuators[] array. This helper is
 * shared with the XTEST unit tests so the MAX_VALUATORS boundary can be
 * exercised without requiring a physical 36-axis input device.
 */
static inline int
XTestCopyValuators(int *valuators, int firstValuator, int numAxes,
                   int numValuators, const deviceValuator *dv)
{
    int base;

    if (firstValuator < 0 || firstValuator >= numAxes ||
        firstValuator >= MAX_VALUATORS ||
        numValuators < 0 ||
        numValuators > numAxes - firstValuator ||
        numValuators > MAX_VALUATORS - firstValuator)
        return BadValue;

    if (dv->num_valuators < 1 || dv->num_valuators > 6)
        return BadValue;

    if (dv->num_valuators > numAxes - firstValuator - numValuators ||
        dv->num_valuators > MAX_VALUATORS - firstValuator - numValuators)
        return BadValue;

    base = firstValuator + numValuators;
    switch (dv->num_valuators) {
    case 6:
        valuators[base + 5] = dv->valuator5;
        /* fallthrough */
    case 5:
        valuators[base + 4] = dv->valuator4;
        /* fallthrough */
    case 4:
        valuators[base + 3] = dv->valuator3;
        /* fallthrough */
    case 3:
        valuators[base + 2] = dv->valuator2;
        /* fallthrough */
    case 2:
        valuators[base + 1] = dv->valuator1;
        /* fallthrough */
    case 1:
        valuators[base] = dv->valuator0;
        break;
    }

    return Success;
}

#endif /* XTEST_PRIV_H */

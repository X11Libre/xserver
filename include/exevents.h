/************************************************************

Copyright 1996 by Thomas E. Dickey <dickey@clark.net>

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the name of the above listed
copyright holder(s) not be used in advertising or publicity pertaining
to distribution of the software without specific, written prior
permission.

THE ABOVE LISTED COPYRIGHT HOLDER(S) DISCLAIM ALL WARRANTIES WITH REGARD
TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
AND FITNESS, IN NO EVENT SHALL THE ABOVE LISTED COPYRIGHT HOLDER(S) BE
LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

********************************************************/

/********************************************************************
 * Interface of 'exevents.c'
 */

#ifndef EXEVENTS_H
#define EXEVENTS_H

#include <X11/extensions/XIproto.h>
#include "inputstr.h"

/***************************************************************
 *              Interface available to drivers                 *
 ***************************************************************/

/**
 * Scroll flags for ::SetScrollValuator.
 */
enum ScrollFlags {
    SCROLL_FLAG_NONE = 0,
    /**
     * Do not emulate legacy button events for valuator events on this axis.
     */
    SCROLL_FLAG_DONT_EMULATE = (1 << 1),
    /**
     * This axis is the preferred axis for valuator emulation for this axis'
     * scroll type.
     */
    SCROLL_FLAG_PREFERRED = (1 << 2)
};

extern _X_EXPORT int InitProximityClassDeviceStruct(DeviceIntPtr /* dev */ );

extern _X_EXPORT Bool InitValuatorAxisStruct(DeviceIntPtr /* dev */ ,
                                             int /* axnum */ ,
                                             Atom /* label */ ,
                                             int /* minval */ ,
                                             int /* maxval */ ,
                                             int /* resolution */ ,
                                             int /* min_res */ ,
                                             int /* max_res */ ,
                                             int /* mode */ );

extern _X_EXPORT Bool SetScrollValuator(DeviceIntPtr /* dev */ ,
                                        int /* axnum */ ,
                                        enum ScrollType /* type */ ,
                                        double /* increment */ ,
                                        int /* flags */ );

/* Input device properties */
extern _X_EXPORT void XIDeleteAllDeviceProperties(DeviceIntPtr  /* device */
    );

extern _X_EXPORT int XIDeleteDeviceProperty(DeviceIntPtr /* device */ ,
                                            Atom /* property */ ,
                                            Bool        /* fromClient */
    );

extern _X_EXPORT int XIChangeDeviceProperty(DeviceIntPtr /* dev */ ,
                                            Atom /* property */ ,
                                            Atom /* type */ ,
                                            int /* format */ ,
                                            int /* mode */ ,
                                            unsigned long /* len */ ,
                                            const void * /* value */ ,
                                            Bool        /* sendevent */
    );

extern _X_EXPORT int XIGetDeviceProperty(DeviceIntPtr /* dev */ ,
                                         Atom /* property */ ,
                                         XIPropertyValuePtr *   /* value */
    );

extern _X_EXPORT int XISetDevicePropertyDeletable(DeviceIntPtr /* dev */ ,
                                                  Atom /* property */ ,
                                                  Bool  /* deletable */
    );

extern _X_EXPORT long XIRegisterPropertyHandler(DeviceIntPtr dev,
                                                int (*SetProperty) (DeviceIntPtr
                                                                    dev,
                                                                    Atom
                                                                    property,
                                                                    XIPropertyValuePtr
                                                                    prop,
                                                                    BOOL
                                                                    checkonly),
                                                int (*GetProperty) (DeviceIntPtr
                                                                    dev,
                                                                    Atom
                                                                    property),
                                                int (*DeleteProperty)
                                                (DeviceIntPtr dev,
                                                 Atom property)
    );

extern _X_EXPORT void XIUnregisterPropertyHandler(DeviceIntPtr dev, long id);

extern _X_EXPORT Atom XIGetKnownProperty(const char *name);

extern _X_EXPORT DeviceIntPtr XIGetDevice(xEvent *ev);

/****************************************************************************
 *                      End of driver interface                             *
 ****************************************************************************/

#endif                          /* EXEVENTS_H */

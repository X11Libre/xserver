/* SPDX-License-Identifier: MIT OR X11
 *
 * Copyright © 2024 Enrico Weigelt, metux IT consult <info@metux.net>
 */
#ifndef _XSERVER_DIXGRABS_PRIV_H_
#define _XSERVER_DIXGRABS_PRIV_H_

#include <X11/extensions/XIproto.h>

#include "misc.h"
#include "window.h"
#include "input.h"
#include "cursor.h"

struct _GrabParameters;

/**
 * @brief Print current device grab information for specific device
 *
 * Walks through all active grabs and dumps them into the Xserver's error log.
 * This is usually for debugging and troubleshooting. Will also be called by
 * UngrabAllDevices().
 *
 * @param dev the device to act on
 */
void PrintDeviceGrabInfo(DeviceIntPtr dev);

/**
 * @brief Forcefully remove _all_ device grabs
 *
 * Forcefully remove all device grabs on all devices. Optionally kill the
 * clients holding a grab
 *
 * @param kill_client TRUE if clients holding a grab should be killed
 */
void UngrabAllDevices(Bool kill_client);

/**
 * @brief Allocate new grab, optionally copy from existing
 *
 * Allocate a new grab structure. If src is non-null, copy parameters from
 * the existing grab.
 *
 * Returns NULL in case of OOM or when src grab is given, but copy failed.
 *
 * @param src optional grab to copy from (NULL = don't copy)
 * @return pointer to new grab. Must be freed via ::FreeGrab().
 */
GrabPtr AllocGrab(const GrabPtr src);

/**
 * @brief Free a grab
 *
 * Free a grab (that had been allocated by ::AllocGrab()). If the grab has
 * a cursor, this will also be unref'ed / free'd.
 *
 * @param grab pointer to the grab to be freed. Tolerates NULL.
 */
void FreeGrab(GrabPtr grab);

/**
 * @brief create a new grab for given client
 *
 * Create a new grab for given client, with given parameters.
 * Returns NULL on OOM.
 *
 * @param pClient ClientPtr to the client who will hold the grab
 * @param device Device that's being grabbed
 * @param modDevice Device whose modifiers are used (NULL = use core keyboard)
 * @param window the window getting the events
 * @param grabtype type of grab (see ::"enum InputLevel")
 * @param mask mask for fields used from param
 * @param param pointer to struct holding additional parameters
 * @param eventType type of event to grab on (eg. DeviceButtonPress)
 * @param keyCode KeyCode of key or button to grab
 * @param confineTo window to restrict device into (may be NULL)
 * @param cursor cursor to be used while grabbed (may be NULL)
 * @return newly created grab. Must be freed by ::FreeGrab()
 */
GrabPtr CreateGrab(ClientPtr pClient,
                   DeviceIntPtr device,
                   DeviceIntPtr modDevice,
                   WindowPtr window,
                   enum InputLevel grabtype,
                   GrabMask *mask,
                   struct _GrabParameters *param,
                   int eventType,
                   KeyCode keycode,
                   WindowPtr confineTo,
                   CursorPtr cursor);

/**
 * @brief check whether it is a pointer grab
 *
 * @param grab pointer to the grab structure to check
 * @return TRUE if grabbed a pointer
 */
Bool GrabIsPointerGrab(GrabPtr grab);

/**
 * @brief check whether it is a keyboard grab
 *
 * @param grab pointer to the grab structure to check
 * @return TRUE if grabbed a keyboard
 */
Bool GrabIsKeyboardGrab(GrabPtr grab);

/**
 * @brief check whether it is a gesture grab
 *
 * @param grab pointer to the grab structure to check
 * @return TRUE if grabbed a gesture
 */
Bool GrabIsGestureGrab(GrabPtr grab);

/**
 * @brief destructor for X11_RESTYPE_PASSIVEGRAB resource type
 *
 * Destructor for the X11_RESTYPE_PASSIVEGRAB resource type.
 * Should not be used anywhere else
 *
 * @param value pointer to the resource data object
 * @param XID the X11 ID of the resource object
 * @return result code (always Success)
 */
int DeletePassiveGrab(void *value, XID id);

/*
 * @brief compare to grabs
 *
 * Check whether two grabs match each other: grabbing the same events
 * and (optional) grabbing on the same device.
 *
 * @param pFirstGrab first grab to compare
 * @param pSecondGrab second grab to compare
 * @param ignoreDevice TRUE if devices don't need to match
 * @return TRUE if both grabs are having the same claims
 */
Bool GrabMatchesSecond(GrabPtr pFirstGrab,
                       GrabPtr pSecondGrab,
                       Bool ignoreDevice);

/**
 * @brief add passive grab to a client
 *
 * Prepend a grab to the clients's list of passive grabs.
 * Previously existing matching ones are deleted.
 * On conflict with another client's grabs, return BadAccess.
 *
 * @param client pointer to the client the new grab is added to
 * @param pGrab pointer to the grab to be added.
 * @return X11 error code: BadAccess on conflict, otherwise Success
 */
int AddPassiveGrabToList(ClientPtr client, GrabPtr pGrab);

/**
 * @brief delete grab claims from a window's passive grabs list
 *
 * Delete the items affected by given grab from the currently existing
 * passive grabs on a window. This walk through list of passive grabs
 * of the associated window and delete the claims matching this one's.
 *
 * The grab structure passed in here is just used as a vehicle for
 * specifying which claims should be deleted (on which window).
 *
 * @param pMinuedGrab GrabRec structure specifying which claims to delete
 * @return TRUE if succeeded (FALSE usually indicated allocation failure)
 */
Bool DeletePassiveGrabFromList(GrabPtr pMinuendGrab);

#endif /* _XSERVER_DIXGRABS_PRIV_H_ */

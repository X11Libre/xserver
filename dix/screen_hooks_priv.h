/* SPDX-License-Identifier: MIT OR X11
 *
 * Copyright © 2024 Enrico Weigelt, metux IT consult <info@metux.net>
 *
 * @brief exported API entry points for hooking into screen operations
 *
 * These hooks are replacing the old, complicated approach of wrapping
 * ScreenRec's proc vectors. Unlike the wrapping, these hooks are designed
 * to be safe against changes in setup/teardown order and are called
 * independently of the ScreenProc call vectors. It is guaranteed that the
 * objects to operate on already/still exist (eg. destructors are callled
 * before the object is actually destroyed, while post-create hooks are
 * called after the object is created)
 *
 * Main consumers are extensions that need to associate extra data or
 * doing other things additional to the original operation. In some cases
 * they might even be used in drivers (in order to split device specific
 * from generic logic)
 */
#ifndef XORG_DIX_SCREEN_HOOKS_H
#define XORG_DIX_SCREEN_HOOKS_H

#include <X11/Xfuncproto.h>

#include "include/callback.h" /* CallbackListPtr */
#include "include/screenint.h" /* ScreenPtr */
#include "include/window.h" /* WindowPtr */

/* prototype of a window destructor */
typedef void (*XorgScreenWindowDestroyProcPtr)(CallbackListPtr *pcbl,
                                               ScreenPtr pScreen,
                                               WindowPtr pWindow);

/**
 * @brief register a window on the given screen
 *
 * @param pScreen pointer to the screen to register the destructor into
 * @param func pointer to the window destructor function
 * @param arg opaque pointer passed to the destructor
 *
 * Window destructors are the replacement for fragile and complicated wrapping of
 * pScreen->DestroyWindow(): extensions can safely register there custom destructors
 * here, without ever caring about anybody else.
 +
 * The destructors are run right before pScreen->DestroyWindow() - when the window
 * is already removed from hierarchy (thus cannot receive any events anymore) and
 * most of it's data already destroyed - and supposed to do necessary per-extension
 * cleanup duties. Their execution order is *unspecified*.
 *
 * Screen drivers (DDX'es, xf86 video drivers, ...) shall not use these, but still
 * set the pScreen->DestroyWindow pointer - and these should be the *only* ones
 * ever setting it.
 *
 * When registration fails, the server aborts.
 *
 **/
void dixScreenHookWindowDestroy(ScreenPtr pScreen,
                                XorgScreenWindowDestroyProcPtr func);

/**
 * @brief unregister a window destructor on the given screen
 *
 * @param pScreen pointer to the screen to unregister the destructor from
 * @param func pointer to the window destructor function
 * @param arg opaque pointer passed to the destructor
 *
 * @see dixScreenHookWindowDestroy
 *
 * Unregister a window destructor hook registered via @ref dixScreenHookWindowDestroy
 **/
void dixScreenUnhookWindowDestroy(ScreenPtr pScreen,
                                  XorgScreenWindowDestroyProcPtr func);

#endif /* DIX_SCREEN_HOOKS_H */

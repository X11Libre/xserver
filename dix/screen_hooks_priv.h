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
#include "include/pixmap.h" /* PixmapPtr */
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

typedef struct {
    WindowPtr window;
    int32_t x;
    int32_t y;
} XorgScreenWindowPositionParamRec;

/* prototype of a window move notification handler */
typedef void (*XorgScreenWindowPositionProcPtr)(CallbackListPtr *pcbl,
                                                ScreenPtr pScreen,
                                                XorgScreenWindowPositionParamRec *param);

/**
 * @brief register a position notify hook on the given screen
 *
 * @param pScreen pointer to the screen to register the notify hook into
 * @param func pointer to the window hook function
 * @param arg opaque pointer passed to the hook
 *
 * When registration fails, the server aborts.
 *
 **/
void dixScreenHookWindowPosition(ScreenPtr pScreen,
                                           XorgScreenWindowPositionProcPtr func);

/**
 * @brief unregister a window position notify hook on the given screen
 *
 * @param pScreen pointer to the screen to unregister the hook from
 * @param func pointer to the hook function
 * @param arg opaque pointer passed to the destructor
 *
 * @see dixScreenHookWindowPosition
 *
 * Unregister a window position notify hook registered via @ref dixScreenHookWindowPosition
 **/
void dixScreenUnhookWindowPosition(ScreenPtr pScreen,
                                             XorgScreenWindowPositionProcPtr func);

/* prototype of screen close notification handler */
typedef void (*XorgScreenCloseProcPtr)(CallbackListPtr *pcbl,
                                       ScreenPtr pScreen,
                                       void *unused);

/**
 * @brief register a screen close notify hook on the given screen
 *
 * @param pScreen pointer to the screen to register the notify hook into
 * @param func pointer to the hook function
 *
 * When registration fails, the server aborts.
 *
 * NOTE: only exported for libglamoregl, not supposed to be used by drivers.
 **/
_X_EXPORT
void dixScreenHookClose(ScreenPtr pScreen,
                        XorgScreenCloseProcPtr func);

/**
 * @brief unregister a screen close notify hook on the given screen
 *
 * @param pScreen pointer to the screen to unregister the hook from
 * @param func pointer to the hook function
 * @param arg opaque pointer passed to the destructor
 *
 * @see dixScreenHookClose
 *
 * Unregister a screen close notify hook registered via @ref dixScreenHookClose
 *
 * NOTE: only exported for libglamoregl, not supposed to be used by drivers.
 **/
_X_EXPORT
void dixScreenUnhookClose(ScreenPtr pScreen,
                          XorgScreenCloseProcPtr func);

/* prototype of pixmap destroy notification handler */
typedef void (*XorgScreenPixmapDestroyProcPtr)(CallbackListPtr *pcbl,
                                               ScreenPtr pScreen,
                                               PixmapPtr pPixmap);

/**
 * @brief register a pixmap destroy hook on the given screen
 *
 * @param pScreen pointer to the screen to register the notify hook into
 * @param func pointer to the hook function
 * @param arg opaque pointer passed to the hook
 *
 * When registration fails, the server aborts.
 * This hook is called only when the pixmap is really to be destroyed,
 * (unlike ScreenRec->DestroyPixmap())
 *
 * NOTE: only exported for libglamoregl, not supposed to be used by drivers.
 **/
_X_EXPORT
void dixScreenHookPixmapDestroy(ScreenPtr pScreen,
                                XorgScreenPixmapDestroyProcPtr func);

/**
 * @brief unregister a pixmap destroy notify hook on the given screen
 *
 * @param pScreen pointer to the screen to unregister the hook from
 * @param func pointer to the hook function
 * @param arg opaque pointer passed to the destructor
 *
 * @see dixScreenHookClose
 *
 * Unregister a screen close notify hook registered via @ref dixScreenHookPixmapDestroy
 *
 * NOTE: only exported for libglamoregl, not supposed to be used by drivers.
 **/
_X_EXPORT
void dixScreenUnhookPixmapDestroy(ScreenPtr pScreen,
                                  XorgScreenPixmapDestroyProcPtr func);

#endif /* DIX_SCREEN_HOOKS_H */

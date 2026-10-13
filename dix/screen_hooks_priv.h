/* SPDX-License-Identifier: X11 OR MIT OR AGPL-3.0-or-later
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

#include <X11/Xdefs.h>
#include <X11/Xfuncproto.h>

#include "include/callback.h" /* CallbackListPtr */
#include "include/pixmap.h" /* PixmapPtr */
#include "include/screenint.h" /* ScreenPtr */
#include "include/validate.h" /* VTKind */
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

/* prototype of a window realize hook */
typedef void (*XorgScreenWindowRealizeProcPtr)(CallbackListPtr *pcbl,
                                               ScreenPtr pScreen,
                                               WindowPtr pWindow);

/**
 * @brief register a window realize hook on the given screen
 *
 * Window realize hooks are the replacement for fragile and complicated wrapping of
 * pScreen->RealizeWindow(): extensions can safely register there custom handlers
 * here, without ever caring about anybody else. They are called right before the
 * actual pScreen->RealizeWindow proc runs.
 *
 * When registration fails, the server aborts.
 **/
void dixScreenHookWindowRealize(ScreenPtr pScreen,
                                XorgScreenWindowRealizeProcPtr func);

/**
 * @brief unregister a window realize hook on the given screen
 * @see dixScreenHookWindowRealize
 **/
void dixScreenUnhookWindowRealize(ScreenPtr pScreen,
                                  XorgScreenWindowRealizeProcPtr func);

/* prototype of a window unrealize hook */
typedef void (*XorgScreenWindowUnrealizeProcPtr)(CallbackListPtr *pcbl,
                                                 ScreenPtr pScreen,
                                                 WindowPtr pWindow);

/**
 * @brief register a window unrealize hook on the given screen
 *
 * Window unrealize hooks are the replacement for fragile and complicated wrapping of
 * pScreen->UnrealizeWindow(): extensions can safely register there custom handlers
 * here, without ever caring about anybody else. They are called right before the
 * actual pScreen->UnrealizeWindow proc runs.
 *
 * When registration fails, the server aborts.
 **/
void dixScreenHookWindowUnrealize(ScreenPtr pScreen,
                                  XorgScreenWindowUnrealizeProcPtr func);

/**
 * @brief unregister a window unrealize hook on the given screen
 * @see dixScreenHookWindowUnrealize
 **/
void dixScreenUnhookWindowUnrealize(ScreenPtr pScreen,
                                    XorgScreenWindowUnrealizeProcPtr func);

/* pre/post hook for window moves (replaces wrapping MoveWindow) */
typedef struct {
    WindowPtr window;
    int32_t x;
    int32_t y;
    WindowPtr sibling;
    VTKind kind;
} XorgScreenWindowMoveParamRec;

typedef void (*XorgScreenWindowMoveProcPtr)(CallbackListPtr *pcbl,
                                            ScreenPtr pScreen,
                                            XorgScreenWindowMoveParamRec *param);

void dixScreenHookWindowMove(ScreenPtr pScreen, XorgScreenWindowMoveProcPtr func);
void dixScreenUnhookWindowMove(ScreenPtr pScreen, XorgScreenWindowMoveProcPtr func);
void dixScreenHookPostWindowMove(ScreenPtr pScreen, XorgScreenWindowMoveProcPtr func);
void dixScreenUnhookPostWindowMove(ScreenPtr pScreen, XorgScreenWindowMoveProcPtr func);

/* pre/post hook for window resizes (replaces wrapping ResizeWindow) */
typedef struct {
    WindowPtr window;
    int32_t x;
    int32_t y;
    uint32_t w;
    uint32_t h;
    WindowPtr sibling;
} XorgScreenWindowResizeParamRec;

typedef void (*XorgScreenWindowResizeProcPtr)(CallbackListPtr *pcbl,
                                              ScreenPtr pScreen,
                                              XorgScreenWindowResizeParamRec *param);

void dixScreenHookWindowResize(ScreenPtr pScreen, XorgScreenWindowResizeProcPtr func);
void dixScreenUnhookWindowResize(ScreenPtr pScreen, XorgScreenWindowResizeProcPtr func);
void dixScreenHookPostWindowResize(ScreenPtr pScreen, XorgScreenWindowResizeProcPtr func);
void dixScreenUnhookPostWindowResize(ScreenPtr pScreen, XorgScreenWindowResizeProcPtr func);

/* pre/post hook for window border-width changes (replaces wrapping ChangeBorderWidth) */
typedef struct {
    WindowPtr window;
    uint32_t borderWidth;
} XorgScreenWindowChangeBorderWidthParamRec;

typedef void (*XorgScreenWindowChangeBorderWidthProcPtr)(CallbackListPtr *pcbl,
                                                         ScreenPtr pScreen,
                                                         XorgScreenWindowChangeBorderWidthParamRec *param);

void dixScreenHookWindowChangeBorderWidth(ScreenPtr pScreen, XorgScreenWindowChangeBorderWidthProcPtr func);
void dixScreenUnhookWindowChangeBorderWidth(ScreenPtr pScreen, XorgScreenWindowChangeBorderWidthProcPtr func);
void dixScreenHookPostWindowChangeBorderWidth(ScreenPtr pScreen, XorgScreenWindowChangeBorderWidthProcPtr func);
void dixScreenUnhookPostWindowChangeBorderWidth(ScreenPtr pScreen, XorgScreenWindowChangeBorderWidthProcPtr func);

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

/**
 * @brief register a screen post close notify hook on the given screen
 *
 * @param pScreen pointer to the screen to register the notify hook into
 * @param func pointer to the hook function
 *
 * In contrast to Close hook, it's called *after* the driver's CloseScreen()
 * proc had been called.
 *
 * When registration fails, the server aborts.
 *
 * NOTE: only exported for libglamoregl, not supposed to be used by drivers.
 **/
_X_EXPORT
void dixScreenHookPostClose(ScreenPtr pScreen,
                            XorgScreenCloseProcPtr func);

/**
 * @brief unregister a screen close notify hook on the given screen
 *
 * @param pScreen pointer to the screen to unregister the hook from
 * @param func pointer to the hook function
 * @param arg opaque pointer passed to the destructor
 *
 * @see dixScreenHookPostClose
 *
 * Unregister a screen close notify hook registered via @ref dixScreenHookPostClose
 *
 * In contrast to Close hook, it's called *after* the driver's CloseScreen()
 * proc had been called.
 *
 * NOTE: only exported for libglamoregl, not supposed to be used by drivers.
 **/
_X_EXPORT
void dixScreenUnhookPostClose(ScreenPtr pScreen,
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

/* prototype of screen close notification handler */
typedef void (*XorgScreenPostCreateResourcesProcPtr)(CallbackListPtr *pcbl,
                                                     ScreenPtr pScreen,
                                                     Bool *ret);

/**
 * @brief register post-CreateScreenResources hook on the given screen
 *
 * @param pScreen pointer to the screen to register the notify hook into
 * @param func pointer to the hook function
 * @param arg opaque pointer passed to the hook
 *
 * This hook is called when CreateScreenResources() had been called
 * and returned TRUE.
 **/
_X_EXPORT
void dixScreenHookPostCreateResources(ScreenPtr pScreen,
                                      XorgScreenPostCreateResourcesProcPtr func);

/**
 * @brief unregister a post-CreateScreenResources hook on the given screen
 *
 * @param pScreen pointer to the screen to unregister the hook from
 * @param func pointer to the hook function
 * @param arg opaque pointer passed to the destructor
 *
 * @see dixScreenHookPostCreateResources
 **/
_X_EXPORT
void dixScreenUnhookPostCreateResources(ScreenPtr pScreen,
                                        XorgScreenPostCreateResourcesProcPtr func);

#endif /* DIX_SCREEN_HOOKS_H */

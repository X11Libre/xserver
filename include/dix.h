/***********************************************************

Copyright 1987, 1998  The Open Group

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation.

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from The Open Group.

Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the name of Digital not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.

DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

******************************************************************/

#ifndef DIX_H
#define DIX_H

#include "callback.h"
#include "gc.h"
#include "window.h"
#include "input.h"
#include "cursor.h"
#include "events.h"
#include <X11/extensions/XI.h>

#define EARLIER -1
#define SAMETIME 0
#define LATER 1

#define NullClient ((ClientPtr) 0)

#define REQUEST(type)                                                   \
    type * stuff = (type *)client->requestBuffer;

#define ARRAY_SIZE(a)  (sizeof((a)) / sizeof((a)[0]))

#define REQUEST_SIZE_MATCH(req)                                         \
    do {                                                                \
        if ((sizeof(req) >> 2) != client->req_len)                      \
            return(BadLength);                                          \
    } while (0)

#define REQUEST_AT_LEAST_SIZE(req)                                      \
    do {                                                                \
        if ((sizeof(req) >> 2) > client->req_len)                       \
            return(BadLength);                                          \
    } while (0)

#define REQUEST_AT_LEAST_EXTRA_SIZE(req, extra)                         \
    do {                                                                \
        if (((sizeof(req) + ((uint64_t) (extra))) >> 2) > client->req_len) \
            return(BadLength);                                          \
    } while (0)

#define REQUEST_FIXED_SIZE(req, n)                                      \
    do {                                                                \
        if ((((sizeof(req)) >> 2) > client->req_len) ||            \
            (((n) >> 2) >= client->req_len) ||                         \
            ((((uint64_t) sizeof(req) + (n) + 3) >> 2) != (uint64_t) client->req_len)) \
            return(BadLength);                                          \
    } while (0)

#define LEGAL_NEW_RESOURCE(id,client)           \
    do {                                        \
        if (!LegalNewID((id), (client))) {      \
            (client)->errorValue = (id);        \
            return BadIDChoice;                 \
        }                                       \
    } while (0)

#define WriteReplyToClient(pClient, size, pReply)                       \
    do {                                                                \
        if ((pClient)->swapped)                                         \
            (*ReplySwapVector[((xReq *)(pClient)->requestBuffer)->reqType]) \
                (pClient, (int)(size), pReply);                         \
        else                                                            \
            WriteToClient(pClient, (int)(size), (pReply));              \
    } while (0)

#define WriteSwappedDataToClient(pClient, size, pbuf)                   \
    do {                                                                \
        if ((pClient)->swapped)                                         \
            (*(pClient)->pSwapReplyFunc)(pClient, (int)(size), pbuf);   \
        else                                                            \
            WriteToClient(pClient, (int)(size), (pbuf));                \
    } while (0)

typedef struct _TimeStamp *TimeStampPtr;

#ifndef _XTYPEDEF_CLIENTPTR
typedef struct _Client *ClientPtr;

#define _XTYPEDEF_CLIENTPTR
#endif

typedef struct _WorkQueue *WorkQueuePtr;

extern _X_EXPORT ClientPtr clients[MAXCLIENTS];
extern _X_EXPORT ClientPtr serverClient;
extern _X_EXPORT int currentMaxClients;

typedef struct _TimeStamp {
    CARD32 months;              /* really ~49.7 days */
    CARD32 milliseconds;
} TimeStamp;

typedef int HWEventQueueType;
typedef HWEventQueueType *HWEventQueuePtr;

/* dispatch.c */
extern _X_EXPORT void SetInputCheck(HWEventQueuePtr /*c0 */ ,
                                    HWEventQueuePtr /*c1 */ );

extern _X_EXPORT void UpdateCurrentTime(void);

extern _X_EXPORT void UpdateCurrentTimeIf(void);

/*
 * @brief dereference a pixmap and destroy it when not used anymore
 *
 * Despite the name, this function unref's the pixmap, and only destroys it when
 * the pixmap isn't used anymore. (perhaps it should be renamed to dixUnrefPixmap())
 *
 * Note: it's also used as resource destructor callback, hence that strange args.
 * (not actually finest art, but for now a good compromise, since it's already
 *  existing and exported, thus can easily be used by drivers, w/o breaking compat)
 *
 * @param pPixmap pointer to pixmap (PixmapPtr) that should be unref'ed
 * @param unused ignored, only for matching the resource destructor prototype
 */
_X_EXPORT int dixDestroyPixmap(void *pPixmap, XID unused);

extern _X_EXPORT ClientPtr NextAvailableClient(void *ospriv);

extern _X_EXPORT void SendErrorToClient(ClientPtr /*client */ ,
                                        unsigned int /*majorCode */ ,
                                        unsigned int /*minorCode */ ,
                                        XID /*resId */ ,
                                        int /*errorCode */ );

extern _X_EXPORT void MarkClientException(ClientPtr /*client */ );

/* dixutils.c */

extern _X_EXPORT int dixLookupWindow(WindowPtr *result,
                                     XID id,
                                     ClientPtr client, Mask access_mode);

extern _X_EXPORT int dixLookupDrawable(DrawablePtr *result,
                                       XID id,
                                       ClientPtr client,
                                       Mask type_mask, Mask access_mode);

extern _X_EXPORT int dixLookupFontable(FontPtr *result,
                                       XID id,
                                       ClientPtr client, Mask access_mode);

extern _X_EXPORT void NoopDDA(void);

extern _X_EXPORT int AlterSaveSetForClient(ClientPtr /*client */ ,
                                           WindowPtr /*pWin */ ,
                                           unsigned /*mode */ ,
                                           Bool /*toRoot */ ,
                                           Bool /*map */ );

extern _X_EXPORT void BlockHandler(void *timeout);

extern _X_EXPORT void WakeupHandler(int result);

typedef void (*ServerBlockHandlerProcPtr) (void *blockData,
                                           void *timeout);

typedef void (*ServerWakeupHandlerProcPtr) (void *blockData,
                                            int result);

extern _X_EXPORT Bool RegisterBlockAndWakeupHandlers(ServerBlockHandlerProcPtr blockHandler,
                                                     ServerWakeupHandlerProcPtr wakeupHandler,
                                                     void *blockData);

extern _X_EXPORT void RemoveBlockAndWakeupHandlers(ServerBlockHandlerProcPtr blockHandler,
                                                   ServerWakeupHandlerProcPtr wakeupHandler,
                                                   void *blockData);

extern _X_EXPORT void InitBlockAndWakeupHandlers(void);

extern _X_EXPORT Bool QueueWorkProc(Bool (*function)(ClientPtr clientUnused,
                                                     void *closure),
                                    ClientPtr client,
                                    void *closure);

typedef Bool (*ClientSleepProcPtr) (ClientPtr client,
                                    void *closure);

extern _X_EXPORT Bool ClientSleep(ClientPtr client,
                                  ClientSleepProcPtr function,
                                  void *closure);

#ifndef ___CLIENTSIGNAL_DEFINED___
#define ___CLIENTSIGNAL_DEFINED___
extern _X_EXPORT Bool ClientSignal(ClientPtr /*client */ );
#endif                          /* ___CLIENTSIGNAL_DEFINED___ */

#define CLIENT_SIGNAL_ANY ((void *)-1)
extern _X_EXPORT int ClientSignalAll(ClientPtr /*client*/,
                                     ClientSleepProcPtr /*function*/,
                                     void * /*closure*/);

extern _X_EXPORT void ClientWakeup(ClientPtr /*client */ );

extern _X_EXPORT Bool ClientIsAsleep(ClientPtr /*client */ );

extern _X_EXPORT void SendGraphicsExpose(ClientPtr /*client */ ,
                                         RegionPtr /*pRgn */ ,
                                         XID /*drawable */ ,
                                         int /*major */ ,
                                         int  /*minor */);

/* atom.c */

extern _X_EXPORT Atom MakeAtom(const char * /*string */ ,
                               unsigned /*len */ ,
                               Bool /*makeit */ );

extern _X_EXPORT Bool ValidAtom(Atom /*atom */ );

extern _X_EXPORT const char *NameForAtom(Atom /*atom */ );

extern _X_EXPORT void
AtomError(void)
    _X_NORETURN;

extern _X_EXPORT void
FreeAllAtoms(void);

extern _X_EXPORT void
InitAtoms(void);

/* main.c */

extern _X_EXPORT void
SetVendorRelease(int release);

/* events.c */

extern _X_EXPORT void
ConfineToShape(DeviceIntPtr /* pDev */ ,
               RegionPtr /* shape */ ,
               int * /* px */ ,
               int * /* py */ );

extern _X_EXPORT Bool
IsParent(WindowPtr /* maybeparent */ ,
         WindowPtr /* child */ );

extern _X_EXPORT WindowPtr
GetCurrentRootWindow(DeviceIntPtr pDev);

extern _X_EXPORT WindowPtr
GetSpriteWindow(DeviceIntPtr pDev);

extern _X_EXPORT void
NoticeTime(const DeviceIntPtr dev,
           TimeStamp time);

extern _X_EXPORT void
NoticeEventTime(InternalEvent *ev,
                DeviceIntPtr dev);

extern _X_EXPORT TimeStamp
LastEventTime(int deviceid);

extern _X_EXPORT Bool
LastEventTimeWasReset(int deviceid);

extern _X_EXPORT void
LastEventTimeToggleResetFlag(int deviceid, Bool state);

extern _X_EXPORT void
LastEventTimeToggleResetAll(Bool state);

extern _X_EXPORT int
DeliverEventsToWindow(DeviceIntPtr /* pWin */ ,
                      WindowPtr /* pWin */ ,
                      xEventPtr /* pEvents */ ,
                      int /* count */ ,
                      Mask /* filter */ ,
                      GrabPtr /* grab */ );

extern _X_EXPORT void
DeliverRawEvent(RawDeviceEvent * /* ev */ ,
                DeviceIntPtr    /* dev */
    );

extern _X_EXPORT void
WindowHasNewCursor(WindowPtr /* pWin */ );

extern _X_EXPORT int
OtherClientGone(void *value,
                XID id);

extern _X_EXPORT int
DeliverEvents(WindowPtr /*pWin */ ,
              xEventPtr /*xE */ ,
              int /*count */ ,
              WindowPtr /*otherParent */ );

extern _X_EXPORT void
WriteEventsToClient(ClientPtr /*pClient */ ,
                    int /*count */ ,
                    xEventPtr /*events */ );

extern _X_EXPORT int
TryClientEvents(ClientPtr /*client */ ,
                DeviceIntPtr /* device */ ,
                xEventPtr /*pEvents */ ,
                int /*count */ ,
                Mask /*mask */ ,
                Mask /*filter */ ,
                GrabPtr /*grab */ );

extern _X_EXPORT void
WindowsRestructured(void);

extern _X_EXPORT DeviceIntPtr
PickPointer(ClientPtr /* client */ );

extern _X_EXPORT DeviceIntPtr
PickKeyboard(ClientPtr /* client */ );

#ifdef RANDR
extern _X_EXPORT void
ScreenRestructured(ScreenPtr pScreen);
#endif

/*
 *  ServerGrabCallback stuff
 */

extern _X_EXPORT CallbackListPtr ServerGrabCallback;

typedef enum { SERVER_GRABBED, SERVER_UNGRABBED,
    CLIENT_PERVIOUS, CLIENT_IMPERVIOUS
} ServerGrabState;

typedef struct {
    ClientPtr client;
    ServerGrabState grabstate;
} ServerGrabInfoRec;

/*
 *  EventCallback stuff
 */

extern _X_EXPORT CallbackListPtr EventCallback;

typedef struct {
    ClientPtr client;
    xEventPtr events;
    int count;
} EventInfoRec;

/*
 *  DeviceEventCallback stuff
 */

extern _X_EXPORT CallbackListPtr DeviceEventCallback;

typedef struct {
    InternalEvent *event;
    DeviceIntPtr device;
} DeviceEventInfoRec;

extern _X_EXPORT CallbackListPtr RootWindowFinalizeCallback;

extern _X_EXPORT Bool
IsPointerDevice(DeviceIntPtr dev);

extern _X_EXPORT Bool
IsKeyboardDevice(DeviceIntPtr dev);

extern _X_EXPORT Bool
IsMaster(DeviceIntPtr dev);

extern _X_EXPORT Bool
IsFloating(DeviceIntPtr dev);

extern _X_EXPORT void *lastGLContext;

/**
 * @brief get display string for given screen
 *
 * Entry point for drivers/modules that really need to know what
 * display ID we're running on (eg. xrdp).
 *
 * @param pScreen pointer to ScreenRec to query.
 * @return pointer to string, valid as long as the pScreen is, owned by DIX.
 */
_X_EXPORT const char *dixGetDisplayName(ScreenPtr *pScreen);

#endif                          /* DIX_H */

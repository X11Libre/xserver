/*
 *
Copyright 1992, 1998  The Open Group

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
 *
 * Author:  Keith Packard, MIT X Consortium
 */

/* dixsleep.c - implement millisecond timeouts for X clients */

#include <dix-config.h>

#include "sleepuntil.h"
#include <X11/X.h>
#include <X11/Xmd.h>
#include "misc.h"
#include "windowstr.h"
#include "dixstruct.h"
#include "pixmapstr.h"
#include "scrnintstr.h"

typedef struct _Sertafied {
    struct _Sertafied *next;
    TimeStamp revive;
    ClientPtr pClient;
    XID id;
    void (*notifyFunc) (ClientPtr /* client */ ,
                        void *    /* closure */
        );

    void *closure;
} SertafiedRec, *SertafiedPtr;

static SertafiedPtr pending;
static RESTYPE SertafiedResType;
static Bool BlockHandlerRegistered;

static void ClientAwaken(ClientPtr /* client */ ,
                         void *    /* closure */
    );
static int SertafiedDelete(void *  /* value */ ,
                           XID     /* id */
    );
static void SertafiedBlockHandler(void *data,
                                  void *timeout);

static void SertafiedWakeupHandler(void *data,
                                   int i);

int
ClientSleepUntil(ClientPtr client,
                 TimeStamp *revive,
                 void (*notifyFunc) (ClientPtr, void *), void *closure)
{
    SertafiedResType = CreateNewResourceType(SertafiedDelete,
                                             "ClientSleep");
    if (!SertafiedResType)
        return FALSE;
    BlockHandlerRegistered = FALSE;

    SertafiedPtr pRequest = calloc(1, sizeof(SertafiedRec));
    if (!pRequest)
        return FALSE;
    pRequest->pClient = client;
    pRequest->revive = *revive;
    pRequest->id = FakeClientID(client->index);
    pRequest->closure = closure;
    if (!BlockHandlerRegistered) {
        if (!RegisterBlockAndWakeupHandlers(SertafiedBlockHandler,
                                            SertafiedWakeupHandler,
                                            (void *) 0)) {
            free(pRequest);
            return FALSE;
        }
        BlockHandlerRegistered = TRUE;
    }
    pRequest->notifyFunc = 0;
    if (!AddResource(pRequest->id, SertafiedResType, (void *) pRequest))
        return FALSE;
    if (!notifyFunc)
        notifyFunc = ClientAwaken;
    pRequest->notifyFunc = notifyFunc;
    /* Insert into time-ordered queue, with earliest activation time coming first. */

    SertafiedPtr walk, pPrev = NULL;
    for (walk = pending; walk; walk = walk->next) {
        if (CompareTimeStamps(walk->revive, *revive) == LATER)
            break;
        pPrev = walk;
    }
    if (pPrev)
        pPrev->next = pRequest;
    else
        pending = pRequest;
    pRequest->next = walk;
    IgnoreClient(client);
    return TRUE;
}

static void
ClientAwaken(ClientPtr client, void *closure)
{
    AttendClient(client);
}

static int
SertafiedDelete(void *value, XID id)
{
    SertafiedPtr pRequest = (SertafiedPtr) value;

    SertafiedPtr walk, pPrev = NULL;
    for (walk = pending; walk; pPrev = walk, walk = walk->next)
        if (walk == pRequest) {
            if (pPrev)
                pPrev->next = walk->next;
            else
                pending = walk->next;
            break;
        }
    if (pRequest->notifyFunc)
        (*pRequest->notifyFunc) (pRequest->pClient, pRequest->closure);
    free(pRequest);
    return TRUE;
}

static void
SertafiedBlockHandler(void *data, void *wt)
{
    unsigned long delay;
    TimeStamp now;

    if (!pending)
        return;
    now.milliseconds = GetTimeInMillis();
    now.months = currentTime.months;
    if ((int) (now.milliseconds - currentTime.milliseconds) < 0)
        now.months++;

    SertafiedPtr walk, pNext;
    for (walk = pending; walk; walk = pNext) {
        pNext = walk->next;
        if (CompareTimeStamps(walk->revive, now) == LATER)
            break;
        FreeResource(walk->id, X11_RESTYPE_NONE);

        /* AttendClient() may have been called via the resource delete
         * function so a client may have input to be processed and so
         *  set delay to 0 to prevent blocking in WaitForSomething().
         */
        AdjustWaitForDelay(wt, 0);
    }

    if (pending) {
        delay = pending->revive.milliseconds - now.milliseconds;
        AdjustWaitForDelay(wt, delay);
    }
}

static void
SertafiedWakeupHandler(void *data, int i)
{
    TimeStamp now;

    now.milliseconds = GetTimeInMillis();
    now.months = currentTime.months;
    if ((int) (now.milliseconds - currentTime.milliseconds) < 0)
        now.months++;

    SertafiedPtr walk, pNext;
    for (walk = pending; walk; walk = pNext) {
        pNext = walk->next;
        if (CompareTimeStamps(walk->revive, now) == LATER)
            break;
        FreeResource(walk->id, X11_RESTYPE_NONE);
    }
    if (!pending) {
        RemoveBlockAndWakeupHandlers(SertafiedBlockHandler,
                                     SertafiedWakeupHandler, (void *) 0);
        BlockHandlerRegistered = FALSE;
    }
}

/* SPDX-License-Identifier: MIT OR X11
 *
 * Copyright © 2024 Enrico Weigelt, metux IT consult <info@metux.net>
 */
#ifndef _XSERVER_DIX_PRIV_H
#define _XSERVER_DIX_PRIV_H

#include <stdbool.h>
#include <X11/Xdefs.h>

/* This file holds global DIX settings to be used inside the Xserver,
 *  but NOT supposed to be accessed directly by external server modules like
 *  drivers or extension modules. Thus the definitions here are not part of the
 *  Xserver's module API/ABI.
 */

#include <X11/Xdefs.h>
#include <X11/Xfuncproto.h>

#include "include/dix.h"
#include "include/gc.h"
#include "include/window.h"

/* server setting: maximum size for big requests */
#define MAX_BIG_REQUEST_SIZE 4194303
extern long maxBigRequestSize;

extern char dispatchExceptionAtReset;
extern int terminateDelay;
extern Bool touchEmulatePointer;

extern HWEventQueuePtr checkForInput[2];

static inline _X_NOTSAN Bool
InputCheckPending(void)
{
    return (*checkForInput[0] != *checkForInput[1]);
}

void ClearWorkQueue(void);
void ProcessWorkQueue(void);
void ProcessWorkQueueZombies(void);

void CloseDownClient(ClientPtr client);
ClientPtr GetCurrentClient(void);
void InitClient(ClientPtr client, int i, void *ospriv);

/* lookup builtin color by name */
Bool dixLookupBuiltinColor(int screen,
                           char *name,
                           unsigned len,
                           unsigned short *pred,
                           unsigned short *pgreen,
                           unsigned short *pblue);

void DeleteWindowFromAnySaveSet(WindowPtr pWin);

#define VALIDATE_DRAWABLE_AND_GC(drawID, pDraw, mode)                   \
    do {                                                                \
        int tmprc = dixLookupDrawable(&(pDraw), drawID, client, M_ANY, mode); \
        if (tmprc != Success)                                           \
            return tmprc;                                               \
        tmprc = dixLookupGC(&(pGC), stuff->gc, client, DixUseAccess);   \
        if (tmprc != Success)                                           \
            return tmprc;                                               \
        if ((pGC->depth != pDraw->depth) || (pGC->pScreen != pDraw->pScreen)) \
            return BadMatch;                                            \
        if (pGC->serialNumber != pDraw->serialNumber)                   \
            ValidateGC(pDraw, pGC);                                     \
    } while (0)

int dixLookupGC(GCPtr *result,
                XID id,
                ClientPtr client,
                Mask access_mode);

int dixLookupClient(ClientPtr *result,
                    XID id,
                    ClientPtr client,
                    Mask access_mode);

 /* -retro mode */
extern bool party_like_its_1989;

#endif /* _XSERVER_DIX_PRIV_H */

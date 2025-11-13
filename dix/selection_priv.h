/* SPDX-License-Identifier: MIT OR X11
 *
 * Copyright © 2024 Enrico Weigelt, metux IT consult <info@metux.net>
 */

#ifndef _XSERVER_DIX_SELECTION_PRIV_H
#define _XSERVER_DIX_SELECTION_PRIV_H

#include <X11/Xdefs.h>
#include <X11/Xproto.h>

#include "include/dixstruct.h"
#include "include/privates.h"

struct _Selection {
    Atom selection;
    TimeStamp lastTimeChanged;
    Window window;
    WindowPtr pWin;
    ClientPtr client;
    struct _Selection *next;
    PrivateRec *devPrivates;
};

typedef enum {
    SelectionSetOwner,
    SelectionWindowDestroy,
    SelectionClientClose
} SelectionCallbackKind;

typedef struct {
    struct _Selection *selection;
    ClientPtr client;
    SelectionCallbackKind kind;
} SelectionInfoRec;

#define SELECTION_FILTER_GETOWNER       1
#define SELECTION_FILTER_SETOWNER       2
#define SELECTION_FILTER_CONVERT        3
#define SELECTION_FILTER_LISTEN         4
#define SELECTION_FILTER_EV_REQUEST     5
#define SELECTION_FILTER_EV_CLEAR       6
#define SELECTION_FILTER_NOTIFY         7

typedef struct {
    int op;
    Bool skip;
    int status;
    Atom selection;
    ClientPtr client;       // initiating client
    ClientPtr recvClient;   // client receiving event
    Time time;              // request time stamp
    Window requestor;
    Window owner;
    Atom property;
    Atom target;
} SelectionFilterParamRec, *SelectionFilterParamPtr;

extern CallbackListPtr SelectionFilterCallback;

void InitSelections(void);
void DeleteClientFromAnySelections(ClientPtr client);

#endif /* _XSERVER_DIX_SELECTION_PRIV_H */

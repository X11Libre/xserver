/*
 * Copyright (c) 1997-1999 by The XFree86 Project, Inc.
 */

#ifndef _X_MOUSEPRIV_H
#define _X_MOUSEPRIV_H

#if 0
# define MOUSEINITDEBUG
# define MOUSEDATADEBUG
#endif

#include "mouse.h"
#include "xf86Xinput.h"
/* Private interface for the mouse driver. */

typedef enum  {
    AUTOPROBE_H_NOPROTO,
    AUTOPROBE_H_GOOD,
    AUTOPROBE_H_AUTODETECT,
    AUTOPROBE_H_VALIDATE1,
    AUTOPROBE_H_VALIDATE2,
    AUTOPROBE_H_SETPROTO,
    AUTOPROBE_NOPROTO,
    AUTOPROBE_COLLECT,
    AUTOPROBE_CREATE_PROTOLIST,
    AUTOPROBE_GOOD,
    AUTOPROBE_AUTODETECT,
    AUTOPROBE_VALIDATE1,
    AUTOPROBE_VALIDATE2,
    AUTOPROBE_SWITCHSERIAL,
    AUTOPROBE_SWITCH_PROTOCOL
} mseAutoProbeStates;

typedef struct {
    const char *        name;
    int                 class;
    const char **       defaults;
    MouseProtocolID     id;
} MouseProtocolRec, *MouseProtocolPtr;

#define NUM_MSE_AUTOPROBE_BYTES 24  /* multiple of 3,4 and 6 byte packages */
#define NUM_MSE_AUTOPROBE_TOTAL 64
#define NUM_AUTOPROBE_PROTOS 17


typedef struct {
    int         current;
    Bool        inReset;
    CARD32      lastEvent;
    CARD32      expires;
    Bool        soft;
    int         goodCount;
    int         badCount;
    int         protocolID;
    int         count;
    char        data[NUM_MSE_AUTOPROBE_TOTAL];
    mseAutoProbeStates autoState;
    MouseProtocolID protoList[NUM_AUTOPROBE_PROTOS];
    int         serialDefaultsNum;
    int         prevDx, prevDy;
    int         accDx, accDy;
    int         acc;
    CARD32      pnpLast;
    Bool        disablePnPauto;
    float       fracdx,fracdy;
    float       sensitivity;
} mousePrivRec, *mousePrivPtr;

/* mouse proto flags */
#define MPF_NONE                0x00
#define MPF_SAFE                0x01

/* pnp.c */
MouseProtocolID MouseGetPnpProtocol(InputInfoPtr pInfo);
Bool ps2Reset(InputInfoPtr pInfo);
Bool ps2EnableDataReporting(InputInfoPtr pInfo);
Bool ps2SendPacket(InputInfoPtr pInfo, unsigned char *bytes, int len);
int ps2GetDeviceID(InputInfoPtr pInfo);

#endif /* _X_MOUSE_H */

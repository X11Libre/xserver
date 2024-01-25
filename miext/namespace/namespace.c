
// 2do: set namespace properties on windows

#include <dix-config.h>

#include <stdio.h> // printf // FIXME: use Xorg debug functions
#include <stdbool.h>

//#include "scrnintstr.h"
//#include "inputstr.h"
#include "windowstr.h"
#include "propertyst.h"
//#include "colormapst.h"
//#include "privates.h"
//#include "registry.h"
#include "dix_priv.h"
#include "xacestr.h"
#include "registry.h"
//#include "securitysrv.h"
//#include <X11/extensions/securproto.h>
////#include "extinit.h"
#include "extinit.h"
#include "extnsionst.h"
#include "protocol-versions.h"

#include "namespace.h"
#include "clientpriv.h"
#include "hooks.h"

/* Extension stuff */
//static int SecurityErrorBase;   /* first Security error number */
//static int SecurityEventBase;   /* first Security event number */

//RESTYPE SecurityAuthorizationResType;   /* resource type for authorizations */
//static RESTYPE RTEventClient;

//static CallbackListPtr SecurityValidateGroupCallback = NULL;

/* retrieve per client private structure */
//static inline struct client_priv *clientPriv(ClientPtr client) {
//    return dixLookupPrivate(&client->devPrivates, &stateKeyRec);
//}

/* The only extensions that untrusted clients have access to */
//static const char *SecurityTrustedExtensions[] = {
//    "XC-MISC",
//    "BIG-REQUESTS",
//    NULL
//};

/*
 * Access modes that untrusted clients are allowed on trusted objects.
 */
//static const Mask SecurityResourceMask =
//    DixGetAttrAccess | DixReceiveAccess | DixListPropAccess |
//    DixGetPropAccess | DixListAccess;
//static const Mask SecurityWindowExtraMask = DixRemoveAccess;
//static const Mask SecurityRootWindowExtraMask =
//    DixReceiveAccess | DixSendAccess | DixAddAccess | DixRemoveAccess;
//static const Mask SecurityDeviceMask =
//    DixGetAttrAccess | DixReceiveAccess | DixGetFocusAccess |
//    DixGrabAccess | DixSetAttrAccess | DixUseAccess;
//static const Mask SecurityServerMask = DixGetAttrAccess | DixGrabAccess;
//static const Mask SecurityClientMask = DixGetAttrAccess;

/* SecurityAudit
 *
 * Arguments:
 *	format is the formatting string to be used to interpret the
 *	  remaining arguments.
 *
 * Returns: nothing.
 *
 * Side Effects:
 *	Writes the message to the log file if security logging is on.
 */

//static void
//_X_ATTRIBUTE_PRINTF(1, 2)
//SecurityAudit(const char *format, ...)
//{
//    va_list args;
//
//    if (auditTrailLevel < SECURITY_AUDIT_LEVEL)
//        return;
//    va_start(args, format);
//    VAuditF(format, args);
//    va_end(args);
//}                               /* SecurityAudit */

/*
 * Performs a Security permission check.
 */
//static int
//SecurityDoCheck(struct client_priv *subj, SecurityStateRec * obj,
//                Mask requested, Mask allowed)
//{
//    if (!subj->haveState || !obj->haveState)
//        return Success;
//    if (subj->trustLevel == XSecurityClientTrusted)
//        return Success;
//    if (obj->trustLevel != XSecurityClientTrusted)
//        return Success;
//    if ((requested | allowed) == allowed)
//        return Success;
//
//    return BadAccess;
//}

/*
 * Looks up a request name
 */
static inline const char *
ContainerLookupRequestName(ClientPtr client)
{
    return LookupRequestName(client->majorOp, client->minorOp);
}


/* ---- helpers ----- */
//bool isRootWindow(WindowPtr win) {
    

//}



/* SecurityDeleteAuthorization
 *
 * Arguments:
 *	value is the authorization to delete.
 *	id is its resource ID.
 *
 * Returns: Success.
 *
 * Side Effects:
 *	Frees everything associated with the authorization.
 */

//static int
//SecurityDeleteAuthorization(void *value, XID id)
//{
//    SecurityAuthorizationPtr pAuth = (SecurityAuthorizationPtr) value;
//    unsigned short name_len, data_len;
//    const char *name;
//    char *data;
//    int status;
//    int i;
//    OtherClientsPtr pEventClient;
//
//    /* Remove the auth using the os layer auth manager */
//
//    status = AuthorizationFromID(pAuth->id, &name_len, &name, &data_len, &data);
//    assert(status);
//    status = RemoveAuthorization(name_len, name, data_len, data);
//    assert(status);
//    (void) status;
//
//    /* free the auth timer if there is one */

//    if (pAuth->timer)
//        TimerFree(pAuth->timer);
//
//    /* send revoke events */
//
//    while ((pEventClient = pAuth->eventClients)) {
        /* send revocation event event */
//        xSecurityAuthorizationRevokedEvent are = {
//            .type = SecurityEventBase + XSecurityAuthorizationRevoked,
//            .authId = pAuth->id
//        };
//        WriteEventsToClient(rClient(pEventClient), 1, (xEvent *) &are);
//        FreeResource(pEventClient->resource, RT_NONE);
//    }

    /* kill all clients using this auth */

//    for (i = 1; i < currentMaxClients; i++)
//        if (clients[i]) {
//            struct client_priv *priv = clientPriv(clients[i]->devPrivates);
//            if (priv->haveState && priv->authId == pAuth->id)
//                CloseDownClient(clients[i]);
//        }
//
//    SecurityAudit("revoked authorization ID %lu\n", (unsigned long)pAuth->id);
//    free(pAuth);
//    return Success;
//
//}                               /* SecurityDeleteAuthorization */

/* resource delete function for RTEventClient */
//static int
//SecurityDeleteAuthorizationEventClient(void *value, XID id)
//{
//    OtherClientsPtr pEventClient, prev = NULL;
//    SecurityAuthorizationPtr pAuth = (SecurityAuthorizationPtr) value;
//
//    for (pEventClient = pAuth->eventClients;
//         pEventClient; pEventClient = pEventClient->next) {
//        if (pEventClient->resource == id) {
//            if (prev)
//                prev->next = pEventClient->next;
//            else
//                pAuth->eventClients = pEventClient->next;
//            free(pEventClient);
//            return Success;
//        }
//        prev = pEventClient;
//    }
//     /*NOTREACHED*/ return -1;  /* make compiler happy */
//}                               /* SecurityDeleteAuthorizationEventClient */
//
/* SecurityComputeAuthorizationTimeout
 *
 * Arguments:
 *	pAuth is the authorization for which we are computing the timeout
 *	seconds is the number of seconds we want to wait
 *
 * Returns:
 *	the number of milliseconds that the auth timer should be set to
 *
 * Side Effects:
 *	Sets pAuth->secondsRemaining to any "overflow" amount of time
 *	that didn't fit in 32 bits worth of milliseconds
 */

//static CARD32
//SecurityComputeAuthorizationTimeout(SecurityAuthorizationPtr pAuth,
//                                    unsigned int seconds)
//{
//    /* maxSecs is the number of full seconds that can be expressed in
//     * 32 bits worth of milliseconds
//     */
//    CARD32 maxSecs = (CARD32) (~0) / (CARD32) MILLI_PER_SECOND;
//
//    if (seconds > maxSecs) {    /* only come here if we want to wait more than 49 days */
//        pAuth->secondsRemaining = seconds - maxSecs;
//        return maxSecs * MILLI_PER_SECOND;
//    }
//    else {                      /* by far the common case */
//        pAuth->secondsRemaining = 0;
//        return seconds * MILLI_PER_SECOND;
//    }
//}                               /* SecurityStartAuthorizationTimer */

/* SecurityAuthorizationExpired
 *
 * This function is passed as an argument to TimerSet and gets called from
 * the timer manager in the os layer when its time is up.
 *
 * Arguments:
 *	timer is the timer for this authorization.
 *	time is the current time.
 *	pval is the authorization whose time is up.
 *
 * Returns:
 *	A new time delay in milliseconds if the timer should wait some
 *	more, else zero.
 *
 * Side Effects:
 *	Frees the authorization resource if the timeout period is really
 *	over, otherwise recomputes pAuth->secondsRemaining.
 */

//static CARD32
//SecurityAuthorizationExpired(OsTimerPtr timer, CARD32 time, void *pval)
//{
//    SecurityAuthorizationPtr pAuth = (SecurityAuthorizationPtr) pval;
//
//    assert(pAuth->timer == timer);
//
//    if (pAuth->secondsRemaining) {
//        return SecurityComputeAuthorizationTimeout(pAuth,
//                                                   pAuth->secondsRemaining);
//    }
//    else {
//        FreeResource(pAuth->id, RT_NONE);
//        return 0;
//    }
//}                               /* SecurityAuthorizationExpired */

/* SecurityStartAuthorizationTimer
 *
 * Arguments:
 *	pAuth is the authorization whose timer should be started.
 *
 * Returns: nothing.
 *
 * Side Effects:
 *	A timer is started, set to expire after the timeout period for
 *	this authorization.  When it expires, the function
 *	SecurityAuthorizationExpired will be called.
 */

//static void
//SecurityStartAuthorizationTimer(SecurityAuthorizationPtr pAuth)
//{
//    pAuth->timer = TimerSet(pAuth->timer, 0,
//                            SecurityComputeAuthorizationTimeout(pAuth,
//                                                                pAuth->timeout),
//                            SecurityAuthorizationExpired, pAuth);
//}                               /* SecurityStartAuthorizationTimer */

/* Proc functions all take a client argument, execute the request in
 * client->requestBuffer, and return a protocol error status.
 */

//static int
//ProcSecurityQueryVersion(ClientPtr client)
//{
//    /* REQUEST(xSecurityQueryVersionReq); */
//    xSecurityQueryVersionReply rep = {
//        .type = X_Reply,
//        .sequenceNumber = client->sequence,
//        .length = 0,
//        .majorVersion = SERVER_SECURITY_MAJOR_VERSION,
//        .minorVersion = SERVER_SECURITY_MINOR_VERSION
//    };
//
//    REQUEST_SIZE_MATCH(xSecurityQueryVersionReq);
//
//    if (client->swapped) {
//        swaps(&rep.sequenceNumber);
//        swaps(&rep.majorVersion);
////        swaps(&rep.minorVersion);
//    }
//    WriteToClient(client, SIZEOF(xSecurityQueryVersionReply), &rep);
//    return Success;
//}                               /* ProcSecurityQueryVersion */

//static int
//SecurityEventSelectForAuthorization(SecurityAuthorizationPtr pAuth,
//                                    ClientPtr client, Mask mask)
//{
//    OtherClients *pEventClient;
//
//    for (pEventClient = pAuth->eventClients;
//         pEventClient; pEventClient = pEventClient->next) {
//        if (SameClient(pEventClient, client)) {
//            if (mask == 0)
//                FreeResource(pEventClient->resource, RT_NONE);
//            else
//                pEventClient->mask = mask;
//            return Success;
//        }
//    }
//
//    pEventClient = malloc(sizeof(OtherClients));
//    if (!pEventClient)
//        return BadAlloc;
//    pEventClient->mask = mask;
//    pEventClient->resource = FakeClientID(client->index);
//    pEventClient->next = pAuth->eventClients;
//    if (!AddResource(pEventClient->resource, RTEventClient, (void *) pAuth)) {
//        free(pEventClient);
//        return BadAlloc;
//    }
//    pAuth->eventClients = pEventClient;
//
//    return Success;
//}                               /* SecurityEventSelectForAuthorization */

//static int
//ProcSecurityGenerateAuthorization(ClientPtr client)
//{
//    REQUEST(xSecurityGenerateAuthorizationReq);
//    int len;                    /* request length in CARD32s */
//    Bool removeAuth = FALSE;    /* if bailout, call RemoveAuthorization? */
//    SecurityAuthorizationPtr pAuth = NULL;      /* auth we are creating */
//    int err;                    /* error to return from this function */
//    XID authId;                 /* authorization ID assigned by os layer */
//    xSecurityGenerateAuthorizationReply rep;    /* reply struct */
//    unsigned int trustLevel;    /* trust level of new auth */
//    XID group;                  /* group of new auth */
//    CARD32 timeout;             /* timeout of new auth */
//    CARD32 *values;             /* list of supplied attributes */
//    char *protoname;            /* auth proto name sent in request */
//    char *protodata;            /* auth proto data sent in request */
//    unsigned int authdata_len;  /* # bytes of generated auth data */
//    char *pAuthdata;            /* generated auth data */
//    Mask eventMask;             /* what events on this auth does client want */

    /* check request length */

//    REQUEST_AT_LEAST_SIZE(xSecurityGenerateAuthorizationReq);
//    len = bytes_to_int32(SIZEOF(xSecurityGenerateAuthorizationReq));
//    len += bytes_to_int32(stuff->nbytesAuthProto);
//    len += bytes_to_int32(stuff->nbytesAuthData);
//    values = ((CARD32 *) stuff) + len;
//    len += Ones(stuff->valueMask);
//    if (client->req_len != len)
//        return BadLength;

    /* check valuemask */
//    if (stuff->valueMask & ~XSecurityAllAuthorizationAttributes) {
//        client->errorValue = stuff->valueMask;
//        return BadValue;
//    }

    /* check timeout */
//    timeout = 60;
//    if (stuff->valueMask & XSecurityTimeout) {
//        timeout = *values++;
//    }

    /* check trustLevel */
//    trustLevel = XSecurityClientUntrusted;
//    if (stuff->valueMask & XSecurityTrustLevel) {
//        trustLevel = *values++;
//        if (trustLevel != XSecurityClientTrusted &&
//            trustLevel != XSecurityClientUntrusted) {
//            client->errorValue = trustLevel;
//            return BadValue;
//        }
//    }

    /* check group */
//    group = None;
//    if (stuff->valueMask & XSecurityGroup) {
//        group = *values++;
//        if (SecurityValidateGroupCallback) {
//            SecurityValidateGroupInfoRec vgi;

//            vgi.group = group;
//            vgi.valid = FALSE;
//            CallCallbacks(&SecurityValidateGroupCallback, (void *) &vgi);

            /* if nobody said they recognized it, it's an error */

//            if (!vgi.valid) {
//                client->errorValue = group;
//                return BadValue;
//            }
//        }
//    }

    /* check event mask */
//    eventMask = 0;
//    if (stuff->valueMask & XSecurityEventMask) {
//        eventMask = *values++;
//        if (eventMask & ~XSecurityAllEventMasks) {
//            client->errorValue = eventMask;
//            return BadValue;
//        }
//    }

//    protoname = (char *) &stuff[1];
//    protodata = protoname + bytes_to_int32(stuff->nbytesAuthProto);

    /* call os layer to generate the authorization */

//    authId = GenerateAuthorization(stuff->nbytesAuthProto, protoname,
//                                   stuff->nbytesAuthData, protodata,
//                                   &authdata_len, &pAuthdata);
//    if ((XID) ~0L == authId) {
//        err = SecurityErrorBase + XSecurityBadAuthorizationProtocol;
//        goto bailout;
//    }

    /* now that we've added the auth, remember to remove it if we have to
     * abort the request for some reason (like allocation failure)
     */
//    removeAuth = TRUE;

    /* associate additional information with this auth ID */

//    pAuth = malloc(sizeof(SecurityAuthorizationRec));
//    if (!pAuth) {
//        err = BadAlloc;
//        goto bailout;
//    }

    /* fill in the auth fields */

//    pAuth->id = authId;
//    pAuth->timeout = timeout;
//    pAuth->group = group;
//    pAuth->trustLevel = trustLevel;
//    pAuth->refcnt = 0;          /* the auth was just created; nobody's using it yet */
//    pAuth->secondsRemaining = 0;
//    pAuth->timer = NULL;
//    pAuth->eventClients = NULL;

    /* handle event selection */
//    if (eventMask) {
//        err = SecurityEventSelectForAuthorization(pAuth, client, eventMask);
//        if (err != Success)
//            goto bailout;
//    }

//    if (!AddResource(authId, SecurityAuthorizationResType, pAuth)) {
//        err = BadAlloc;
//        goto bailout;
//    }

    /* start the timer ticking */

//    if (pAuth->timeout != 0)
//        SecurityStartAuthorizationTimer(pAuth);
//
    /* tell client the auth id and data */

//    rep = (xSecurityGenerateAuthorizationReply) {
//        .type = X_Reply,
//        .sequenceNumber = client->sequence,
//        .length = bytes_to_int32(authdata_len),
//        .authId = authId,
//        .dataLength = authdata_len
//    };

//    if (client->swapped) {
//        swapl(&rep.length);
//        swaps(&rep.sequenceNumber);
//        swapl(&rep.authId);
//        swaps(&rep.dataLength);
//    }

//    WriteToClient(client, SIZEOF(xSecurityGenerateAuthorizationReply), &rep);
//    WriteToClient(client, authdata_len, pAuthdata);

//    SecurityAudit
//        ("client %d generated authorization %lu trust %d timeout %lu group %lu events %lu\n",
//         client->index, (unsigned long)pAuth->id, pAuth->trustLevel, (unsigned long)pAuth->timeout,
//         (unsigned long)pAuth->group, (unsigned long)eventMask);

    /* the request succeeded; don't call RemoveAuthorization or free pAuth */
//    return Success;
//
// bailout:
//    if (removeAuth)
//        RemoveAuthorization(stuff->nbytesAuthProto, protoname,
//                            authdata_len, pAuthdata);
//    free(pAuth);
//    return err;

//}                               /* ProcSecurityGenerateAuthorization */

//static int
//ProcSecurityRevokeAuthorization(ClientPtr client)
//{
//    REQUEST(xSecurityRevokeAuthorizationReq);
//    SecurityAuthorizationPtr pAuth;
//    int rc;
//
//    REQUEST_SIZE_MATCH(xSecurityRevokeAuthorizationReq);
//
//    rc = dixLookupResourceByType((void **) &pAuth, stuff->authId,
//                                 SecurityAuthorizationResType, client,
//                                 DixDestroyAccess);
//    if (rc != Success)
//        return rc;
//
//    FreeResource(stuff->authId, RT_NONE);
//    return Success;
//}                               /* ProcSecurityRevokeAuthorization */

//static int
//ProcSecurityDispatch(ClientPtr client)
//{
//    REQUEST(xReq);
//
//    switch (stuff->data) {
//    case X_SecurityQueryVersion:
//        return ProcSecurityQueryVersion(client);
//    case X_SecurityGenerateAuthorization:
//        return ProcSecurityGenerateAuthorization(client);
//    case X_SecurityRevokeAuthorization:
//        return ProcSecurityRevokeAuthorization(client);
//    default:
//        return BadRequest;
//    }
//}                               /* ProcSecurityDispatch */

//static int _X_COLD
//SProcSecurityQueryVersion(ClientPtr client)
//{
//    REQUEST(xSecurityQueryVersionReq);
//
//    swaps(&stuff->length);
//    REQUEST_SIZE_MATCH(xSecurityQueryVersionReq);
//    swaps(&stuff->majorVersion);
//    swaps(&stuff->minorVersion);
//    return ProcSecurityQueryVersion(client);
//}                               /* SProcSecurityQueryVersion */

//static int _X_COLD
//SProcSecurityGenerateAuthorization(ClientPtr client)
//{
//    REQUEST(xSecurityGenerateAuthorizationReq);
//    CARD32 *values;
//    unsigned long nvalues;
//    int values_offset;
//
//    swaps(&stuff->length);
//    REQUEST_AT_LEAST_SIZE(xSecurityGenerateAuthorizationReq);
//    swaps(&stuff->nbytesAuthProto);
//    swaps(&stuff->nbytesAuthData);
//    swapl(&stuff->valueMask);
//    values_offset = bytes_to_int32(stuff->nbytesAuthProto) +
//        bytes_to_int32(stuff->nbytesAuthData);
//    if (values_offset >
//        stuff->length - bytes_to_int32(sz_xSecurityGenerateAuthorizationReq))
//        return BadLength;
//    values = (CARD32 *) (&stuff[1]) + values_offset;
//    nvalues = (((CARD32 *) stuff) + stuff->length) - values;
//    SwapLongs(values, nvalues);
//    return ProcSecurityGenerateAuthorization(client);
//}                               /* SProcSecurityGenerateAuthorization */

//static int _X_COLD
//SProcSecurityRevokeAuthorization(ClientPtr client)
//{
//    REQUEST(xSecurityRevokeAuthorizationReq);

//    swaps(&stuff->length);
//    REQUEST_SIZE_MATCH(xSecurityRevokeAuthorizationReq);
//    swapl(&stuff->authId);
//    return ProcSecurityRevokeAuthorization(client);
//}                               /* SProcSecurityRevokeAuthorization */

//static int _X_COLD
//SProcSecurityDispatch(ClientPtr client)
//{
//    REQUEST(xReq);

//    switch (stuff->data) {
//    case X_SecurityQueryVersion:
//        return SProcSecurityQueryVersion(client);
//    case X_SecurityGenerateAuthorization:
//        return SProcSecurityGenerateAuthorization(client);
//    case X_SecurityRevokeAuthorization:
//        return SProcSecurityRevokeAuthorization(client);
//    default:
//        return BadRequest;
//    }
//}                               /* SProcSecurityDispatch */

//static void _X_COLD
//SwapSecurityAuthorizationRevokedEvent(xSecurityAuthorizationRevokedEvent * from,
//                                      xSecurityAuthorizationRevokedEvent * to)
//{
//    to->type = from->type;
//    to->detail = from->detail;
//    cpswaps(from->sequenceNumber, to->sequenceNumber);
//    cpswapl(from->authId, to->authId);
//}

/* SecurityCheckDeviceAccess
 *
 * Arguments:
 *	client is the client attempting to access a device.
 *	dev is the device being accessed.
 *	fromRequest is TRUE if the device access is a direct result of
 *	  the client executing some request and FALSE if it is a
 *	  result of the server trying to send an event (e.g. KeymapNotify)
 *	  to the client.
 * Returns:
 *	TRUE if the device access should be allowed, else FALSE.
 *
 * Side Effects:
 *	An audit message is generated if access is denied.
 */

static const char * devPermittedRequests[] = {
    "X11:QueryPointer",
    "X11:GetInputFocus",
    "X11:QueryPointer",
    NULL
};

static int devRequestPermitted(const char* reqName) {
    for (int x=0; devPermittedRequests[x] != NULL; x++) {
        if (strcmp(reqName, devPermittedRequests[x])==0) {
            return 1;
        }
    }
    return 0;
}

static void
hookDevice(CallbackListPtr *pcbl, void *unused, void *calldata)
{
    XaceDeviceAccessRec *rec = calldata;
    ClientPtr client = rec->client;
    struct client_priv *subj = clientPriv(client);
    struct client_priv *obj = clientPriv(serverClient);

//    Mask requested = rec->access_mode;
//    Mask allowed = SecurityDeviceMask;
//
//    if (rec->dev != inputInfo.keyboard)
//        /* this extension only supports the core keyboard */
//        allowed = requested;
//
//    if (SecurityDoCheck(subj, obj, requested, allowed) != Success) {
//        rec->status = BadAccess;
//    }

    // server can do anything
    if (clientSameNS(subj, obj))
        return;

    // FIXME: that's not very efficient, but didn't find a better way to do it yet
    const char* reqName = LookupRequestName(client->majorOp, client->minorOp);

    // FIXME: let pass X11:QueryPointer on allow_peek_mouse
    if (devRequestPermitted(reqName))
        return;

    printf("BLOCKED device: client %d ns %s keyboard access on request %s obj %d target ns %s\n",
        client->index,
        subj->ns->id,
        reqName,
        serverClient->index, obj->ns->id);

    rec->status = BadAccess;
}

/* hookResourceAccess
 *
 * This function gets plugged into client->CheckAccess and is called from
 * SecurityLookupIDByType/Class to determine if the client can access the
 * resource.
 *
 * Arguments:
 *	client is the client doing the resource access.
 *	id is the resource id.
 *	rtype is its type or class.
 *	access_mode represents the intended use of the resource; see
 *	  resource.h.
 *	res is a pointer to the resource structure for this resource.
 *
 * Returns:
 *	If access is granted, the value of rval that was passed in, else FALSE.
 *
 * Side Effects:
 *	Disallowed resource accesses are audited.
 */

static void
hookResourceAccess(CallbackListPtr *pcbl, void *unused, void *calldata)
{
    XaceResourceAccessRec *rec = calldata;
    ClientPtr client = rec->client;
    int owner_id = CLIENT_ID(rec->id); // FIXME: need to check it for illegal value ?
    struct client_priv *subj = clientPriv(client);
    struct client_priv *obj = clientPriv(clients[owner_id]);
//    Mask requested = rec->access_mode;
    char buffer[128];

    // resource access inside same container is always permitted
    if (clientSameNS(subj, obj)) {
        return;
    }

//    Mask allowed = SecurityResourceMask;
//
//    /* disable background None for untrusted windows */
//    if ((requested & DixCreateAccess) && (rec->rtype == RT_WINDOW))
//        if (subj->haveState && subj->trustLevel != XSecurityClientTrusted)
//            ((WindowPtr) rec->res)->forcedBG = TRUE;
//
//    /* additional permissions for specific resource types */
//    if (rec->rtype == RT_WINDOW)
//        allowed |= SecurityWindowExtraMask;
//
//    /* special checks for server-owned resources */
//    if (cid == 0) {
//        if (rec->rtype & RC_DRAWABLE)
//            /* additional operations allowed on root windows */
//            allowed |= SecurityRootWindowExtraMask;
//
//        else if (rec->rtype == RT_COLORMAP)
//            /* allow access to default colormaps */
//            allowed = requested;
//
//        else
//            /* allow read access to other server-owned resources */
//            allowed |= DixReadAccess;
//    }
//
//    if (clients[cid] != NULL) {
//        obj = clientPriv(clients[cid]);
//        if (SecurityDoCheck(subj, obj, requested, allowed) == Success)
//            return;
//    }
//
//    rec->status = BadAccess;    /* deny access */
    LookupDixAccessName(rec->access_mode, (char*)&buffer, sizeof(buffer));
    printf("%20s() client %-5d access 0x%07lx %s to resource 0x%06lx of client %d on request %s\n",
        __func__,
        client->index, // calling client
        (unsigned long)rec->access_mode, buffer,
        (unsigned long)rec->id,
        owner_id, // resource owner
        LookupRequestName(client->majorOp, client->minorOp));
}

//static void
//SecurityExtension(CallbackListPtr *pcbl, void *unused, void *calldata)
//{
//    XaceExtAccessRec *rec = calldata;
//    struct client_priv *subj = clientPriv(rec->client);
//    int i = 0;
//
//    if (subj->haveState && subj->trustLevel == XSecurityClientTrusted)
//        return;
//
//    while (SecurityTrustedExtensions[i])
//        if (!strcmp(SecurityTrustedExtensions[i++], rec->ext->name))
//            return;
//
//    SecurityAudit("Security: denied client %d access to extension "
//                  "%s on request %s\n",
//                  rec->client->index, rec->ext->name,
//                  SecurityLookupRequestName(rec->client));
//    rec->status = BadAccess;
//}

static void
hookServerAccess(CallbackListPtr *pcbl, void *unused, void *calldata)
{
    XaceServerAccessRec *rec = calldata;
    ClientPtr client = rec->client;
    struct client_priv *subj = clientPriv(rec->client);
    struct client_priv *obj = clientPriv(serverClient);

//    Mask requested = rec->access_mode;
//    Mask allowed = SecurityServerMask;
//
//    if (SecurityDoCheck(subj, obj, requested, allowed) != Success) {
//        rec->status = BadAccess;
//    }

    printf("handleServer: client %-5d access to server configuration request %s\n",
        rec->client->index,
        LookupRequestName(client->majorOp, client->minorOp));
}

static void
hookClient(CallbackListPtr *pcbl, void *unused, void *calldata)
{
    XaceClientAccessRec *rec = calldata;
    ClientPtr client = rec->client;
    struct client_priv *subj = clientPriv(client);
//    struct client_priv *obj = clientPriv(target);

//    Mask requested = rec->access_mode;
//    Mask allowed = SecurityClientMask;
//
//    if (SecurityDoCheck(subj, obj, requested, allowed) != Success) {
//        rec->status = BadAccess;
//    }

    printf("handleClient: client %-5d access to client %d on request %s\n",
        rec->client->index,
        rec->target->index,
        LookupRequestName(client->majorOp, client->minorOp));
}

static void
hookPropertyAccess(CallbackListPtr *pcbl, void *unused, void *calldata)
{
    XacePropertyAccessRec *rec = calldata;
    ClientPtr client = rec->client;
    struct client_priv *subj = clientPriv(client);
    struct client_priv *obj = clientPriv(wClient(rec->pWin));

    ATOM name = (*rec->ppProp)->propertyName;
//    Mask requested = rec->access_mode;
//    Mask allowed = SecurityResourceMask | DixReadAccess;
//
//    if (SecurityDoCheck(subj, obj, requested, allowed) != Success) {
//        rec->status = BadAccess;
//    }

    if (clientSameNS(subj, obj))
        return;

    printf("property: client %d access to property %s (atom 0x%x) window 0x%lx of client %d on request %s\n",
        client->index,
        NameForAtom(name),
        name,
        (unsigned long)rec->pWin->drawable.id,
        wClient(rec->pWin)->index,
        LookupRequestName(client->majorOp, client->minorOp));
}

#if 0
/* extReset
 *
 * Arguments:
 *	extEntry is the extension information for the security extension.
 *
 * Returns: nothing.
 *
 * Side Effects:
 *	Performs any cleanup needed by Security at server shutdown time.
 */

static void extReset(ExtensionEntry * extEntry)
{
    /* Unregister callbacks */
    DeleteCallback(&ClientStateCallback, hookClientState, NULL);

//    XaceDeleteCallback(XACE_EXT_DISPATCH, ContainerExtension, NULL);
    XaceDeleteCallback(XACE_RESOURCE_ACCESS, hookResourceAccess, NULL);
    XaceDeleteCallback(XACE_DEVICE_ACCESS, hookDevice, NULL);
    XaceDeleteCallback(XACE_PROPERTY_ACCESS, hookPropertyAccess, NULL);
    XaceDeleteCallback(XACE_SEND_ACCESS, hookSend, NULL);
//    XaceDeleteCallback(XACE_RECEIVE_ACCESS, ContainerReceive, NULL);
//    XaceDeleteCallback(XACE_CLIENT_ACCESS, hookClient, NULL);
//    XaceDeleteCallback(XACE_EXT_ACCESS, ContainerExtension, NULL);
    XaceDeleteCallback(XACE_SERVER_ACCESS, hookServerAccess, NULL);
}
#endif

/* SecurityExtensionInit
 *
 * Arguments: none.
 *
 * Returns: nothing.
 *
 * Side Effects:
 *	Enables the Security extension if possible.
 */

void
NamespaceExtensionInit(void)
{
//    ExtensionEntry *extEntry;
    int ret = TRUE;

//    SecurityAuthorizationResType =
//        CreateNewResourceType(SecurityDeleteAuthorization,
//                              "SecurityAuthorization");

//    RTEventClient =
//        CreateNewResourceType(SecurityDeleteAuthorizationEventClient,
//                              "SecurityEventClient");

//    if (!SecurityAuthorizationResType || !RTEventClient)
//        return;
//
//    RTEventClient |= RC_NEVERRETAIN;

    /* load configuration */
    loadConfig();

    /* Allocate the private storage */
    if (!dixRegisterPrivateKey
        (&stateKeyRec, PRIVATE_CLIENT, sizeof(struct client_priv)))
        FatalError("ContainerExtensionSetup: Can't allocate client private.\n");

    /* Register callbacks */
    ret &= AddCallback(&ClientStateCallback, hookClientState, NULL);

//    ret &= XaceRegisterCallback(XACE_EXT_DISPATCH, SecurityExtension, NULL);
    ret &= XaceRegisterCallback(XACE_RESOURCE_ACCESS, hookResourceAccess, NULL);
    ret &= XaceRegisterCallback(XACE_DEVICE_ACCESS, hookDevice, NULL);
    ret &= XaceRegisterCallback(XACE_PROPERTY_ACCESS, hookPropertyAccess, NULL);
    ret &= XaceRegisterCallback(XACE_SEND_ACCESS, hookSend, NULL);
    ret &= XaceRegisterCallback(XACE_RECEIVE_ACCESS, handleReceive, NULL);
    ret &= XaceRegisterCallback(XACE_CLIENT_ACCESS, hookClient, NULL);
//    ret &= XaceRegisterCallback(XACE_EXT_ACCESS, ContainerExtension, NULL);
    ret &= XaceRegisterCallback(XACE_SERVER_ACCESS, hookServerAccess, NULL);

    if (!ret)
        FatalError("SecurityExtensionSetup: Failed to register callbacks\n");

    /* Add extension to server */
//    extEntry = AddExtension(CONTAINER_EXTENSION_NAME,
//                            XContainerNumberEvents, XContainerNumberErrors,
//                            ProcContainerDispatch, SProcContainerDispatch,
//                            extReset, StandardMinorOpcode);
//
//    ContainerErrorBase = extEntry->errorBase;
//    ContainerEventBase = extEntry->eventBase;

//    EventSwapVector[ContainerEventBase + XContainerAuthorizationRevoked] =
//        (EventSwapPtr) SwapSecurityAuthorizationRevokedEvent;

//    SetResourceTypeErrorValue(SecurityAuthorizationResType,
//                              SecurityErrorBase + XSecurityBadAuthorization);

    /* Label objects that were created before we could register ourself */
    struct client_priv *srv= clientPriv(serverClient);

    /* Do the serverClient */
    srv->isServer = TRUE;
    clientAssignNSByName(srv, "root");
}

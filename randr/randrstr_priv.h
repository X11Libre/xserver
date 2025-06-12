/*
 * Copyright © 2000 Compaq Computer Corporation
 * Copyright © 2002 Hewlett-Packard Company
 * Copyright © 2006 Intel Corporation
 * Copyright © 2008 Red Hat, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that copyright
 * notice and this permission notice appear in supporting documentation, and
 * that the name of the copyright holders not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  The copyright holders make no representations
 * about the suitability of this software for any purpose.  It is provided "as
 * is" without express or implied warranty.
 *
 * THE COPYRIGHT HOLDERS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
 * OF THIS SOFTWARE.
 *
 * Author:  Jim Gettys, Hewlett-Packard Company, Inc.
 *	    Keith Packard, Intel Corporation
 */

#ifndef _XSERVER_RANDRSTR_PRIV_H_
#define _XSERVER_RANDRSTR_PRIV_H_

#include <X11/Xdefs.h>

#include "randrstr.h"

extern int RREventBase, RRErrorBase;

extern RESTYPE RRClientType, RREventType;     /* resource types for event masks */
extern DevPrivateKeyRec RRClientPrivateKeyRec;

/* see also randr/randrstr.h: some still need to be exported for legacy Nvidia */
extern RESTYPE RRProviderType;  /* X resource type: Randr PROVIDER */
extern RESTYPE RRLeaseType;     /* X resource type: Randr LEASE */

#define RRClientPrivateKey (&RRClientPrivateKeyRec)

#define VERIFY_RR_OUTPUT(id, ptr, a)\
    {\
	int rc = dixLookupResourceByType((void **)&(ptr), id,\
	                                 RROutputType, client, a);\
	if (rc != Success) {\
	    client->errorValue = id;\
	    return rc;\
	}\
    }

#define VERIFY_RR_CRTC(id, ptr, a)\
    {\
	int rc = dixLookupResourceByType((void **)&(ptr), id,\
	                                 RRCrtcType, client, a);\
	if (rc != Success) {\
	    client->errorValue = id;\
	    return rc;\
	}\
    }

#define VERIFY_RR_MODE(id, ptr, a)\
    {\
	int rc = dixLookupResourceByType((void **)&(ptr), id,\
	                                 RRModeType, client, a);\
	if (rc != Success) {\
	    client->errorValue = id;\
	    return rc;\
	}\
    }

#define VERIFY_RR_PROVIDER(id, ptr, a)\
    {\
        int rc = dixLookupResourceByType((void **)&(ptr), id,\
                                         RRProviderType, client, a);\
        if (rc != Success) {\
            client->errorValue = id;\
            return rc;\
        }\
    }

#define VERIFY_RR_LEASE(id, ptr, a)\
    {\
        int rc = dixLookupResourceByType((void **)&(ptr), id,\
                                         RRLeaseType, client, a);\
        if (rc != Success) {\
            client->errorValue = id;\
            return rc;\
        }\
    }

#define GetRRClient(pClient)    ((RRClientPtr)dixLookupPrivate(&(pClient)->devPrivates, RRClientPrivateKey))
#define rrClientPriv(pClient)	RRClientPtr pRRClient = GetRRClient(pClient)

void RRConstrainCursorHarder(DeviceIntPtr, ScreenPtr, int, int *, int *);

/* rrlease.c */
void RRDeliverLeaseEvent(ClientPtr client, WindowPtr window);

void RRTerminateLease(RRLeasePtr lease);

Bool RRLeaseInit(void);

/* rrprovider.c */
#define PRIME_SYNC_PROP         "PRIME Synchronization"

void RRMonitorInit(ScreenPtr screen);

Bool RRMonitorMakeList(ScreenPtr screen, Bool get_active, RRMonitorPtr *monitors_ret, int *nmon_ret);

int RRMonitorCountList(ScreenPtr screen);

void RRMonitorFreeList(RRMonitorPtr monitors, int nmon);

void RRMonitorClose(ScreenPtr screen);

RRMonitorPtr RRMonitorAlloc(int noutput);

int RRMonitorAdd(ClientPtr client, ScreenPtr screen, RRMonitorPtr monitor);

void RRMonitorFree(RRMonitorPtr monitor);

/*
 * Deliver a ScreenChangeNotity event to given client
 *
 * @param pClient the client to notify
 * @param pWin    the window to refer to in the event
 * @param pScreen the screen where the change happened
 */
void RRDeliverScreenEvent(ClientPtr pClient, WindowPtr pWin, ScreenPtr pScreen);

/*
 * Mark screen resources as changed, so listeners will get updates on them.
 *
 * @param pScreen the screen where changes occoured
 */
void RRResourcesChanged(ScreenPtr pScreen);

/*
 * Initialize randr subsystem
 *
 * @return TRUE on success
 */
Bool RRInit(void);

/*
 * Retrieve the first enabled CRTC on given screen
 *
 * @param pScreen the screen to query
 * @return pointer to CRTC structure or NULL
 */
RRCrtcPtr RRFirstEnabledCrtc(ScreenPtr pScreen);

/*
 * Compute vertical refresh rate from randr mode information
 *
 * @param mode pointer to randr mode info
 * @return vertical refresh rate
 */
CARD16 RRVerticalRefresh(xRRModeInfo * mode);

/*
 * Tests if findCrtc belongs to pScreen or secondary screens
 *
 * @param pScreen the screen to check on
 * @param findCrtc the Crtc to check for
 * @return TRUE if given CRTC belongs to pScreen / secondard screens
 */
Bool RRCrtcExists(ScreenPtr pScreen, RRCrtcPtr findCrtc);

/*
 * Deliver CRTC update event to given client
 *
 * @param pClient the client to send event to
 * @param pWin    the window whose screen had been changed
 * @param crtc    the CRTC that had been changed
 */
void RRDeliverCrtcEvent(ClientPtr pClient, WindowPtr pWin, RRCrtcPtr crtc);

/*
 * Destroy a Crtc at shutdown
 *
 * @param crtc    the CRTC to destroy
 */
void RRCrtcDestroy(RRCrtcPtr crtc);

/*
 * Initialize crtc resource type
 */
Bool RRCrtcInit(void);


/*
 * Initialize crtc type error value
 */
void RRCrtcInitErrorValue(void);

/*
 * Handler for the ReplaceScanoutPixmap screen proc
 * Should not be called directly.
 */
Bool RRReplaceScanoutPixmap(DrawablePtr pDrawable, PixmapPtr pPixmap, Bool enable);

/*
 * Check whether given screen has any scanout pixmap attached
 *
 * @param pScreen the screen to check
 * @return TRUE if the screen has a scanout pixmap attached
 */
Bool RRHasScanoutPixmap(ScreenPtr pScreen);

/*
 * Called by DIX to notify RANDR extension that a lease had been terminated.
 *
 * @param lease   the lease that had been terminated
 */
void RRLeaseTerminated(RRLeasePtr lease);

/*
 * Free a RRLease structure
 *
 * @param lease   pointer to the lease to be freed
 */
void RRLeaseFree(RRLeasePtr lease);

/*
 * Check whether given CRTC has an active lease
 *
 * @param crtc    the CRTC to check
 * @return TRUE if there is any active lease on that CRTC
 */
Bool RRCrtcIsLeased(RRCrtcPtr crtc);

/*
 * Check whether given output is leased
 *
 * @param output  the output to check
 * @return TRUE if theere is any active lease on that output
 */
Bool RROutputIsLeased(RROutputPtr output);

/*
 * Query a list of modes valid for some output in given screen
 *
 + The list is allocated by that function and must be freed by caller.
 * `num_ret` holds the number of entries (the buffer might be larger)
 *
 * @param pScreen the screen to query
 * @param num_ret return buffer for number of returned modes
 * @return pointer to array of RRModePtr's
 */
RRModePtr *RRModesForScreen(ScreenPtr pScreen, int *num_ret);

/*
 * Initialize mode resource type
 *
 * @return TRUE on success
 */
Bool RRModeInit(void);

/*
 * Initialize mode type error value
 *
 * @return TRUE on success
 */
void RRModeInitErrorValue(void);

/*
 * Add user-given mode to output
 *
 * @param output  the output where to which a mode should be added
 * @param mode    the mode to add to the output
 * @return X error code
 */
int RROutputAddUserMode(RROutputPtr output, RRModePtr mode);

/*
 * Delete user-given mode (that had been added via RROutputAddUserMode)
 * from output.
 *
 * @param output  the output from which the mode is to be removed
 * @param mode    the mode to be removed from output
 * @return X error code
 */
int RROutputDeleteUserMode(RROutputPtr output, RRModePtr mode);

/*
 * Set subpixel order on given output
 *
 * @param output  the output to set subpixel order on
 * @param order   subpixel order value to set
 */
void RROutputSetSubpixelOrder(RROutputPtr output, int order);

/*
 * Deliver RROutputChangeNotify event to client
 *
 * @param pClient the client to send notify even to
 * @param pWin    the window who's screen is acted on
 * @param output  the output who's changes are delivered
 */
void RRDeliverOutputEvent(ClientPtr pClient, WindowPtr pWin, RROutputPtr output);

/*
 * Initialize output resource type
 *
 * @return TRUE on success
 */
Bool RROutputInit(void);

/*
 * Initialize output type error value
 */
void RROutputInitErrorValue(void);

/*
 * When the screen is reconfigured, move all pointers to the nearest
 * CRTC
 *
 * @param pScreen the screen that had been reconfigured
 */
void RRPointerScreenConfigured(ScreenPtr pScreen);

/*
 * Retrieve output property value
 *
 * @param output  the output to query
 * @param property Atom ID of the property to retrieve
 * @param pending  retrieve pending instead of current value
 * @return pointer to property value or NULL (if not found)
 */
RRPropertyValuePtr RRGetOutputProperty(RROutputPtr output, Atom property, Bool pending);

/*
 * Retrieve full property structure from output
 *
 * @param output    the output to query
 * @param property  Atom ID of the property to query
 * @return pointer to property structure, or NULL if not found
 */
RRPropertyPtr RRQueryOutputProperty(RROutputPtr output, Atom property);

/*
 * Delete all properties on given output
 *
 * @param output  the output whose properties shall be deleted
 */
void RRDeleteAllOutputProperties(RROutputPtr output);

/*
 * Initialize render provider resource type
 *
 * @return TRUE on success
 */
Bool RRProviderInit(void);

/*
 * Initialize RR provider error values
 */
void RRProviderInitErrorValue(void);

/*
 * Destroy a provider and free it's memory
 *
 * @param provider  the provider to be destroyed
 */
void RRProviderDestroy (RRProviderPtr provider);

/*
 * Deliver provider ProviderChangeNotify event to client
 *
 * @param pClient   the client to send even to
 * @param pWin      the window whose screen was changed
 * @param provider  the provider which was changed
 */
void RRDeliverProviderEvent(ClientPtr pClient, WindowPtr pWin, RRProviderPtr provider);

/*
 * Auto configure a GPU screen
 *
 * @param pScreen         the GPU screen to configure
 * @param primaryScreen   the associated primary screen
 */
void RRProviderAutoConfigGpuScreen(ScreenPtr pScreen, ScreenPtr primaryScreen);

/*
 * Retrieve property value from provider
 *
 * @param provider  the provider to query
 * @param property  Atom ID of the property to retrieve
 * @param pending   TRUE if pending (instead of current) value shall be fetched
 * @return pointer to property value if found, otherwise NULL
 */
RRPropertyValuePtr RRGetProviderProperty(RRProviderPtr provider, Atom property, Bool pending);

/*
 * Retrieve full property structure
 * (instead of just the value -- @see RRGetProviderProperty)
 *
 * @param provider  the provider to query
 * @param property  Atom ID of the property to retrieve
 * @return pointer to render property structure if found, otherwise NULL
 */
RRPropertyPtr  RRQueryProviderProperty(RRProviderPtr provider, Atom property);

/*
 * Delete property from provider
 *
 * @param provider  the provider to remove property from
 * @param property  Atom ID of the property to remove
 */
void RRDeleteProviderProperty(RRProviderPtr provider, Atom property);

/*
 * Change property of provider
 *
 * @param provider  the provider to change property on
 * @param property  Atom ID of the property to change
 * @param type      type Atom ID of the new property value
 * @param format    format (8/16/32) of the new property value
 * @param len       length in (format specific) units of the new property value
 * @param value     pointer to value data
 * @param sendevent TRUE when change notify event shall be sent
 * @param pending   TRUE when pending instead of current value shall be changed
 * @return X error code
 */
int RRChangeProviderProperty(RRProviderPtr provider, Atom property, Atom type,
                             int format, int mode, unsigned long len,
                             void *value, Bool sendevent, Bool pending);

/*
 * Configure a (custom) property in given provider
 *
 * @param provider  the provider to configure property in
 * @param property  Atom ID of the property
 * @param pending   TRUE on pending value
 * @param range     TRUE when limited range
 * @param immutable TRUE when it's immutable
 * @param num_values number of allowed values
 * @param values     allowed values (array of num_values length)
 */
int RRConfigureProviderProperty(RRProviderPtr provider, Atom property,
                                Bool pending, Bool range, Bool immutable,
                                int num_values, INT32 *values);

/*
 * Init xinerama specific extension parts
 */
void RRXineramaExtensionInit(void);

/*
 * Init transform structure
 *
 * @param transform   the transform structure to initialized
 */
void RRTransformInit(RRTransformPtr transform);

/*
 * Compare two transform structures
 *
 * @param a   first transform
 * @param b   second transform
 * @return TRUE if both transforms are equal
 */
Bool RRTransformEqual(RRTransformPtr a, RRTransformPtr b);

/*
 * Copy transform structure to another
 *
 * @param dst destination structure pointer
 * @param src source structure pointer
 * @return TRUE on success
 */
Bool RRTransformCopy(RRTransformPtr dst, RRTransformPtr src);

#endif /* _XSERVER_RANDRSTR_PRIV_H_ */

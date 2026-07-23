/* SPDX-License-Identifier: MIT OR X11 OR AGPL-3.0-or-later
 *
 * DPMS (Display Power Management Signaling) — interface between the
 * DPMS extension and other parts (DIX, DDX, OS).
 *
 * Copyright © 2026 Enrico Weigelt, metux IT consult <info@metux.net>
 *
 * This header is NOT part of the driver API.  Drivers must NOT
 * include it.  It is for use by the extension, DIX, and DDX only.
 *
 * DDX is assumed to provide entry point via ScreenRec->DPMS() and set
 * several variables (eg. DPMSStandbyTime, DPMSDisabledSwitch, etc)
 * on startup.
 *
 * Timeout variables (DPMSStandbyTime, DPMSSuspendTime, DPMSOffTime)
 * are in milliseconds.  They are initialised to ~0 (UINT32_MAX) in
 * Xext/dpms/dpms.c, which the extension treats as "use ScreenSaverTime".
 * The DDX may override them during InitOutput (e.g. xf86Config.c
 * reads them from the command line / config file and converts from
 * minutes to milliseconds).
 *
 * Power levels are defined in <X11/extensions/dpmsconst.h>:
 *   DPMSModeOn      0   full power
 *   DPMSModeStandby 1   minimal signal, fast resume
 *   DPMSModeSuspend 2   deeper sleep, slower resume
 *   DPMSModeOff     3   most power saving
 */

#ifndef _DPMS_PRIV_H_
#define _DPMS_PRIV_H_

#include <dix-config.h>

#include <stdbool.h>

#include "dixstruct.h"

/**
 * Set a new DPMS power level for every screen.
 *
 * Called by:
 *   - WaitFor.c   (timer-driven automatic transitions)
 *   - mieq.c      (any input event wakes the displays)
 *   - xf86Init.c  (abort path — restore video before dying)
 *   - xf86Events.c(VT switch — restore before leaving VT)
 *   - dpms.c      (Disable, ForceLevel, CloseDown)
 *
 * Implementation:
 *   Saves old DPMSPowerLevel, updates it to @level, co-ordinates
 *   with the screen saver (blank/unblank if needed), then calls
 *   walkScreen->DPMS(walkScreen, level) for every screen whose
 *   DPMS callback is non-NULL.
 *
 * @client  the client requesting the change (may be serverClient)
 * @level   one of DPMSModeOn/Standby/Suspend/Off
 * @return  Success on success, or a dixSaveScreens error code.
 */
extern int DPMSSet(ClientPtr client, int level);

/**
 * Query whether DPMS is supported by any screen.
 *
 * Returns TRUE if at least one screen (or GPU screen) has a non-NULL
 * DPMS function pointer.  Called during DPMSExtensionInit to decide
 * whether to register the extension at all, and by the Info request
 * to report the DPMS state.
 */
extern Bool DPMSSupported(void);

/**
 * Standby timeout in milliseconds.
 *
 * The idle time after which the server transitions to DPMS standby
 * mode.  -1 (UINT32_MAX) at init; DPMSExtensionInit falls back to
 * ScreenSaverTime.  The DDX may set this via configuration
 * (e.g. xf86Config reads the "StandbyTime" option).
 */
extern CARD32 DPMSStandbyTime;

/**
 * Suspend timeout in milliseconds.
 *
 * The idle time after which the server transitions to DPMS suspend
 * mode.  Semantics match DPMSStandbyTime.
 */
extern CARD32 DPMSSuspendTime;

/**
 * Off timeout in milliseconds.
 *
 * The idle time after which the server transitions to DPMS off
 * mode.  Semantics match DPMSStandbyTime.
 */
extern CARD32 DPMSOffTime;

/**
 * Current DPMS power level.
 *
 * One of DPMSModeOn (0), DPMSModeStandby (1), DPMSModeSuspend (2),
 * or DPMSModeOff (3).  Modified by DPMSSet() and queried by
 * WaitFor.c (for timeout scheduling), mieq.c (wake on input),
 * Xext/saver.c (coexistence with screen saver), xf86Init.c and
 * xf86Events.c (restore before abort/VT switch).
 */
extern CARD16 DPMSPowerLevel;

/**
 * Whether the DPMS extension is currently enabled.
 *
 * Set to DPMSSupported() during DPMSExtensionInit.  May be toggled
 * by the DPMS Enable/Disable protocol requests.  When FALSE, the
 * timeout machinery in WaitFor.c skips DPMS transitions entirely.
 */
extern bool DPMSEnabled;

/**
 * Set TRUE when the "-dpms" command-line flag is used.
 *
 * Checked by the DDX (xf86DPMSInit) to suppress DPMS initialisation
 * even when the config file requests it.  Set by ddxProcessArgument().
 */
extern bool DPMSDisabledSwitch;

#endif

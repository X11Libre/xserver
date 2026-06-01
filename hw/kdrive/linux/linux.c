/*
 * Copyright © 1999 Keith Packard
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Keith Packard not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Keith Packard makes no
 * representations about the suitability of this software for any purpose.  It
 * is provided "as is" without express or implied warranty.
 *
 * KEITH PACKARD DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL KEITH PACKARD BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#include <kdrive-config.h>
#include "kdrive.h"
#include <errno.h>
#include <linux/vt.h>
#include <linux/kd.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <X11/keysym.h>
#include <linux/apm_bios.h>

#include "dix/dix_priv.h" /* for dispatchException */
#include "os/osdep.h"
#include "os/ddx_priv.h"

#ifdef KDRIVE_MOUSE
extern KdPointerDriver LinuxMouseDriver;
extern KdPointerDriver Ps2MouseDriver;
extern KdPointerDriver MsMouseDriver;
#endif
#ifdef KDRIVE_TSLIB
extern KdPointerDriver TsDriver;
#endif
#ifdef KDRIVE_EVDEV
extern KdPointerDriver LinuxEvdevMouseDriver;
extern KdKeyboardDriver LinuxEvdevKeyboardDriver;
#endif
#ifdef KDRIVE_KBD
extern KdKeyboardDriver LinuxKeyboardDriver;
#endif

/* Implemented by the X server */
extern void LinuxLogInit(void);

static int vtno;
int LinuxConsoleFd;
int LinuxApmFd = -1;
static int activeVT;
static Bool enabled;

static void
LinuxVTRequest(int sig)
{
    kdSwitchPending = TRUE;
}

/* Check before chowning -- this avoids touching the file system */
static void
LinuxCheckChown(const char *file)
{
    struct stat st;
    int r;

    if (stat(file, &st) < 0)
        return;
    uid_t u = getuid();
    gid_t g = getgid();
    if (st.st_uid != u || st.st_gid != g) {
        r = chown(file, u, g);
        (void) r;
    }
}

static int
LinuxInit(void)
{
    int fd = -1;
    char vtname[11];
    struct vt_stat vts;

    LinuxConsoleFd = -1;
    /* check if we're run with euid==0 */
    if (geteuid() != 0) {
        FatalError("LinuxInit: Server must be suid root\n");
    }

    if (kdVirtualTerminal >= 0)
        vtno = kdVirtualTerminal;
    else {
        if ((fd = open("/dev/tty0", O_WRONLY, 0)) < 0) {
            FatalError("LinuxInit: Cannot open /dev/tty0 (%s)\n",
                       strerror(errno));
        }
        if ((ioctl(fd, VT_OPENQRY, &vtno) < 0) || (vtno == -1)) {
            FatalError("xf86OpenConsole: Cannot find a free VT\n");
        }
        close(fd);
    }

    snprintf(vtname, sizeof(vtname), "/dev/tty%d", vtno);       /* /dev/tty1-64 */

    if ((LinuxConsoleFd = open(vtname, O_RDWR | O_NDELAY, 0)) < 0) {
        FatalError("LinuxInit: Cannot open %s (%s)\n", vtname, strerror(errno));
    }

    /* change ownership of the vt */
    LinuxCheckChown(vtname);

    /*
     * the current VT device we're running on is not "console", we want
     * to grab all consoles too
     *
     * Why is this needed?
     */
    LinuxCheckChown("/dev/tty0");
    /*
     * Linux doesn't switch to an active vt after the last close of a vt,
     * so we do this ourselves by remembering which is active now.
     */
    memset(&vts, '\0', sizeof(vts));    /* valgrind */
    if (ioctl(LinuxConsoleFd, VT_GETSTATE, &vts) == 0) {
        activeVT = vts.v_active;
    }

    return 1;
}

static void
LinuxSetSwitchMode(int mode)
{
    struct vt_mode VT;

    if (ioctl(LinuxConsoleFd, VT_GETMODE, &VT) < 0) {
        FatalError("LinuxInit: VT_GETMODE failed\n");
    }

    if (mode == VT_PROCESS) {
        OsSignal(SIGUSR1, LinuxVTRequest);

        VT.mode = mode;
        VT.relsig = SIGUSR1;
        VT.acqsig = SIGUSR1;
    }
    else {
        OsSignal(SIGUSR1, SIG_IGN);

        VT.mode = mode;
        VT.relsig = 0;
        VT.acqsig = 0;
    }
    if (ioctl(LinuxConsoleFd, VT_SETMODE, &VT) < 0) {
        FatalError("LinuxInit: VT_SETMODE failed\n");
    }
}

static Bool LinuxApmRunning;

static void
LinuxApmNotify(int fd, int mask, void *blockData)
{
    apm_event_t event;
    Bool running = LinuxApmRunning;
    int cmd = APM_IOC_SUSPEND;

    while (read(fd, &event, sizeof(event)) == sizeof(event)) {
        switch (event) {
        case APM_SYS_STANDBY:
        case APM_USER_STANDBY:
            running = FALSE;
            cmd = APM_IOC_STANDBY;
            break;
        case APM_SYS_SUSPEND:
        case APM_USER_SUSPEND:
        case APM_CRITICAL_SUSPEND:
            running = FALSE;
            cmd = APM_IOC_SUSPEND;
            break;
        case APM_NORMAL_RESUME:
        case APM_CRITICAL_RESUME:
        case APM_STANDBY_RESUME:
            running = TRUE;
            break;
        }
    }
    if (running && !LinuxApmRunning) {
        KdResume();
        LinuxApmRunning = TRUE;
    }
    else if (!running && LinuxApmRunning) {
        KdSuspend(FALSE);
        LinuxApmRunning = FALSE;
        ioctl(fd, cmd, 0);
    }
}

#ifdef FNONBLOCK
#define NOBLOCK FNONBLOCK
#else
#define NOBLOCK FNDELAY
#endif

static void
LinuxEnable(void)
{
    if (enabled)
        return;
    if (kdSwitchPending) {
        kdSwitchPending = FALSE;
        ioctl(LinuxConsoleFd, VT_RELDISP, VT_ACKACQ);
    }
    /*
     * Open the APM driver
     */
    LinuxApmFd = open("/dev/apm_bios", 2);
    if (LinuxApmFd < 0 && errno == ENOENT)
        LinuxApmFd = open("/dev/misc/apm_bios", 2);
    if (LinuxApmFd >= 0) {
        LinuxApmRunning = TRUE;
        fcntl(LinuxApmFd, F_SETFL, fcntl(LinuxApmFd, F_GETFL) | NOBLOCK);
        SetNotifyFd(LinuxApmFd, LinuxApmNotify, X_NOTIFY_READ, NULL);
    }

    /*
     * now get the VT
     */
    LinuxSetSwitchMode(VT_AUTO);
    if (ioctl(LinuxConsoleFd, VT_ACTIVATE, vtno) != 0) {
        FatalError("LinuxInit: VT_ACTIVATE failed\n");
    }
    if (ioctl(LinuxConsoleFd, VT_WAITACTIVE, vtno) != 0) {
        FatalError("LinuxInit: VT_WAITACTIVE failed\n");
    }
    LinuxSetSwitchMode(VT_PROCESS);
    if (ioctl(LinuxConsoleFd, KDSETMODE, KD_GRAPHICS) < 0) {
        FatalError("LinuxInit: KDSETMODE KD_GRAPHICS failed\n");
    }
    enabled = TRUE;
}

static Bool
LinuxSwitchToVT(int con)
{
    struct vt_stat vts;

    memset(&vts, '\0', sizeof(vts));    /* valgrind */
    ioctl(LinuxConsoleFd, VT_GETSTATE, &vts);

    /* Only switch to active vt's */
    if (con != vts.v_active && (vts.v_state & (1 << con))) {
        ioctl(LinuxConsoleFd, VT_ACTIVATE, con);
        return TRUE;
    }
    return FALSE;
}

static Bool
LinuxSpecialKey(KdKeyboardInfo *ki, unsigned char scan_code)
{
    const char* driver_name = ki->driver->name;

    /**
     * Scancodes are taken from https://aeb.win.tue.nl/linux/kbd/scancodes-1.html
     * These scancodes can be also found at https://wiki.osdev.org/PS/2_Keyboard
     *
     * Only a few keys we are interested in are listed here.
     * If we ever need more keys, we can add them later.
     */

#define KEY_BACKSPACE 0x0E
#define KEY_F1 0x3B
#define KEY_F2 0x3C
#define KEY_F3 0x3D
#define KEY_F4 0x3E
#define KEY_F5 0x3F
#define KEY_F6 0x40
#define KEY_F7 0x41
#define KEY_F8 0x42
#define KEY_F9 0x43
#define KEY_F10 0x44

#define KEY_KP_DEL 0x53

#define KEY_F11 0x57
#define KEY_F12 0x58

/* This one is taken from the linux keyboard driver */
#define KEY_DEL 0x63

    switch(scan_code) {
        case KEY_F1:
            return LinuxSwitchToVT(1);
        case KEY_F2:
            return LinuxSwitchToVT(2);
        case KEY_F3:
            return LinuxSwitchToVT(3);
        case KEY_F4:
            return LinuxSwitchToVT(4);
        case KEY_F5:
            return LinuxSwitchToVT(5);
        case KEY_F6:
            return LinuxSwitchToVT(6);
        case KEY_F7:
            return LinuxSwitchToVT(7);
        case KEY_F8:
            return LinuxSwitchToVT(8);
        case KEY_F9:
            return LinuxSwitchToVT(9);
        case KEY_F10:
            return LinuxSwitchToVT(10);
        case KEY_F11:
            return LinuxSwitchToVT(11);
        case KEY_F12:
            return LinuxSwitchToVT(12);
        case KEY_DEL:
            if (!driver_name || strcmp(driver_name, "keyboard")) {
                /* Only fallthrough for the non-evdev keyboard driver */
                return FALSE;
            }
        case KEY_BACKSPACE:
        case KEY_KP_DEL:
            /*
             * Set the dispatch exception flag so the server will terminate the
             * next time through the dispatch loop.
             */
            if (kdAllowZap) {
                dispatchException |= DE_TERMINATE;
            }
    }
    return FALSE;
}

static void
LinuxDisable(void)
{
    ioctl(LinuxConsoleFd, KDSETMODE, KD_TEXT);  /* Back to text mode ... */
    if (kdSwitchPending) {
        kdSwitchPending = FALSE;
        ioctl(LinuxConsoleFd, VT_RELDISP, 1);
    }
    enabled = FALSE;
    if (LinuxApmFd >= 0) {
        RemoveNotifyFd(LinuxApmFd);
        close(LinuxApmFd);
        LinuxApmFd = -1;
    }
}

static void
LinuxFini(void)
{
    struct vt_mode VT;
    struct vt_stat vts;
    int fd;

    if (LinuxConsoleFd < 0)
        return;

    if (ioctl(LinuxConsoleFd, VT_GETMODE, &VT) != -1) {
        VT.mode = VT_AUTO;
        ioctl(LinuxConsoleFd, VT_SETMODE, &VT); /* set dflt vt handling */
    }
    memset(&vts, '\0', sizeof(vts));    /* valgrind */
    ioctl(LinuxConsoleFd, VT_GETSTATE, &vts);
    if (vtno == vts.v_active) {
        /*
         * Find a legal VT to switch to, either the one we started from
         * or the lowest active one that isn't ours
         */
        if (activeVT < 0 ||
            activeVT == vts.v_active || !(vts.v_state & (1 << activeVT))) {
            for (activeVT = 1; activeVT < 16; activeVT++)
                if (activeVT != vtno && (vts.v_state & (1 << activeVT)))
                    break;
            if (activeVT == 16)
                activeVT = -1;
        }
        /*
         * Perform a switch back to the active VT when we were started
         */
        if (activeVT >= -1) {
            ioctl(LinuxConsoleFd, VT_ACTIVATE, activeVT);
            ioctl(LinuxConsoleFd, VT_WAITACTIVE, activeVT);
            activeVT = -1;
        }
    }
    close(LinuxConsoleFd);      /* make the vt-manager happy */
    LinuxConsoleFd = -1;
    fd = open("/dev/tty0", O_RDWR | O_NDELAY, 0);
    if (fd >= 0) {
        memset(&vts, '\0', sizeof(vts));        /* valgrind */
        ioctl(fd, VT_GETSTATE, &vts);
        if (ioctl(fd, VT_DISALLOCATE, vtno) < 0)
            fprintf(stderr, "Can't deallocate console %d %s\n", vtno,
                    strerror(errno));
        close(fd);
    }
    return;
}

void
KdOsAddInputDrivers(void)
{
#ifdef KDRIVE_MOUSE
    KdAddPointerDriver(&LinuxMouseDriver);
    KdAddPointerDriver(&MsMouseDriver);
    KdAddPointerDriver(&Ps2MouseDriver);
#endif
#ifdef KDRIVE_TSLIB
    KdAddPointerDriver(&TsDriver);
#endif
#ifdef KDRIVE_EVDEV
    KdAddPointerDriver(&LinuxEvdevMouseDriver);
    KdAddKeyboardDriver(&LinuxEvdevKeyboardDriver);
#endif
#ifdef KDRIVE_KBD
    KdAddKeyboardDriver(&LinuxKeyboardDriver);
#endif
}

static void
LinuxBell(int volume, int pitch, int duration)
{
    if (volume && pitch)
        ioctl(LinuxConsoleFd, KDMKTONE, ((1193190 / pitch) & 0xffff) |
              (((unsigned long) duration * volume / 50) << 16));
}

KdOsFuncs LinuxFuncs = {
    .Init = LinuxInit,
    .Enable = LinuxEnable,
    .SpecialKey = LinuxSpecialKey,
    .Disable = LinuxDisable,
    .Fini = LinuxFini,
    .Bell = LinuxBell,
};

void
OsVendorInit(void)
{
    LinuxLogInit();
    KdOsInit(&LinuxFuncs);
}

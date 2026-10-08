/* SPDX-License-Identifier: MIT OR X11 OR AGPLv3
 *
 * Copyright © 2026 Enrico Weigelt, metux IT consult <info@metux.net>
 *
 * ScreenRec modification monitor for detecting NVIDIA driver alterations.
 * Tracks changes to screen structures during driver setup.
 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <X11/X.h>
#include "xf86.h"
#include "xf86Module.h"
#include "xf86Loader.h"
#include "xf86Priv.h"
#include "xf86Errors.h"
#include "xorg-server.h"
#include <X11/X.h>  /* for MAXFORMATS */
#include "screenint.h"
#include "scrnintstr.h"
#include "os.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <signal.h>
#include <stddef.h>
#include "privates.h"

#define MAX_SCREENS 256  /* Arbitrary limit */

typedef struct {
    int screen_count;
    ScreenRec *screens[MAX_SCREENS];
    size_t *struct_sizes;  /* size of each ScreenRec */
    unsigned char **snapshots;  /* raw snapshots, one per screen */
    unsigned long timestamp_ms;
    const char *module_name;
} ScreenRecSnapshot;

static ScreenRecSnapshot initial_snap;
static ScreenRecSnapshot last_check;
static Bool screenrec_monitor_enabled = FALSE;
static Bool monitor_setup_only = FALSE;
static int screenrec_monitor_interval_ms = 5000;
static TimerPtr screenrec_timer = NULL;
static Bool mprotect_enabled = FALSE;
static void **screenrec_mprotect_bases = NULL;
static size_t screenrec_mprotect_count = 0;

static const char *screenrec_logfile = "/tmp/screenrec_changes.log";
static FILE *logfp = NULL;

/* Field metadata for ScreenRec */
typedef struct {
    const char *name;
    size_t offset;
    size_t size;
    int array_count;  /* >1 if it's an array, 0 for variable-length */
} ScreenField;

/* Fields to monitor - defined in order of appearance in ScreenRec */
static const ScreenField screen_fields[] = {
    {"myNum", offsetof(ScreenRec, myNum), sizeof(int), 0},
    {"id", offsetof(ScreenRec, id), sizeof(ATOM), 0},
    {"x", offsetof(ScreenRec, x), sizeof(short), 0},
    {"y", offsetof(ScreenRec, y), sizeof(short), 0},
    {"width", offsetof(ScreenRec, width), sizeof(short), 0},
    {"height", offsetof(ScreenRec, height), sizeof(short), 0},
    {"mmWidth", offsetof(ScreenRec, mmWidth), sizeof(short), 0},
    {"mmHeight", offsetof(ScreenRec, mmHeight), sizeof(short), 0},
    {"numDepths", offsetof(ScreenRec, numDepths), sizeof(short), 0},
    {"rootDepth", offsetof(ScreenRec, rootDepth), sizeof(unsigned char), 0},
    {"allowedDepths", offsetof(ScreenRec, allowedDepths), sizeof(DepthPtr), 0},
    {"rootVisual", offsetof(ScreenRec, rootVisual), sizeof(unsigned long), 0},
    {"defColormap", offsetof(ScreenRec, defColormap), sizeof(unsigned long), 0},
    {"minInstalledCmaps", offsetof(ScreenRec, minInstalledCmaps), sizeof(short), 0},
    {"maxInstalledCmaps", offsetof(ScreenRec, maxInstalledCmaps), sizeof(short), 0},
    {"backingStoreSupport", offsetof(ScreenRec, backingStoreSupport), sizeof(char), 0},
    {"saveUnderSupport", offsetof(ScreenRec, saveUnderSupport), sizeof(char), 0},
    {"whitePixel", offsetof(ScreenRec, whitePixel), sizeof(unsigned long), 0},
    {"blackPixel", offsetof(ScreenRec, blackPixel), sizeof(unsigned long), 0},
    {"GCperDepth", offsetof(ScreenRec, GCperDepth), sizeof(GCPtr), MAXFORMATS+1},
    {"defaultStipple", offsetof(ScreenRec, defaultStipple), sizeof(PixmapPtr), 0},
    {"devPrivate", offsetof(ScreenRec, devPrivate), sizeof(void*), 0},
    {"numVisuals", offsetof(ScreenRec, numVisuals), sizeof(short), 0},
    {"visuals", offsetof(ScreenRec, visuals), sizeof(VisualPtr), 0},
    {"root", offsetof(ScreenRec, root), sizeof(WindowPtr), 0},
    /* Skip screenSaverStuffRec (pointer complicated) */
    {"screenSpecificPrivates", offsetof(ScreenRec, screenSpecificPrivates), sizeof(PrivateRec[PRIVATE_LAST]), PRIVATE_LAST},
    /* function pointers - we'll monitor selected ones */
    {"CloseScreen", offsetof(ScreenRec, CloseScreen), sizeof(CloseScreenProcPtr), 0},
    {"QueryBestSize", offsetof(ScreenRec, QueryBestSize), sizeof(QueryBestSizeProcPtr), 0},
    {"SaveScreen", offsetof(ScreenRec, SaveScreen), sizeof(SaveScreenProcPtr), 0},
    {"GetImage", offsetof(ScreenRec, GetImage), sizeof(GetImageProcPtr), 0},
    {"CreateWindow", offsetof(ScreenRec, CreateWindow), sizeof(CreateWindowProcPtr), 0},
    {"DestroyWindow", offsetof(ScreenRec, DestroyWindow), sizeof(DestroyWindowProcPtr), 0},
    {"MapWindow", offsetof(ScreenRec, MapWindow), sizeof(MapWindowProcPtr), 0},
    {"UnmapWindow", offsetof(ScreenRec, UnmapWindow), sizeof(UnmapWindowProcPtr), 0},
    {"CreatePixmap", offsetof(ScreenRec, CreatePixmap), sizeof(CreatePixmapProcPtr), 0},
    {"DestroyPixmap", offsetof(ScreenRec, DestroyPixmap), sizeof(DestroyPixmapProcPtr), 0},
    {"CreateGC", offsetof(ScreenRec, CreateGC), sizeof(CreateGCProcPtr), 0},
    {"InstallColormap", offsetof(ScreenRec, InstallColormap), sizeof(InstallColormapProcPtr), 0},
    {"StoreColors", offsetof(ScreenRec, StoreColors), sizeof(StoreColorsProcPtr), 0},
    {"canDoBGNoneRoot", offsetof(ScreenRec, canDoBGNoneRoot), sizeof(Bool), 0},
    {"isGPU", offsetof(ScreenRec, isGPU), sizeof(Bool), 0},
#ifdef CONFIG_LEGACY_NVIDIA_PADDING
    {"reserved_for_nvidia_470_and_390", offsetof(ScreenRec, reserved_for_nvidia_470_and_390), sizeof(void*), 0},
#endif
    {"totalPixmapSize", offsetof(ScreenRec, totalPixmapSize), sizeof(unsigned int), 0},
    {"BitmapToRegion", offsetof(ScreenRec, BitmapToRegion), sizeof(BitmapToRegionProcPtr), 0},
    {"BlockHandler", offsetof(ScreenRec, BlockHandler), sizeof(ScreenBlockHandlerProcPtr), 0},
    {"WakeupHandler", offsetof(ScreenRec, WakeupHandler), sizeof(ScreenWakeupHandlerProcPtr), 0},
    {"CreateScreenResources", offsetof(ScreenRec, CreateScreenResources), sizeof(CreateScreenResourcesProcPtr), 0},
    {"DPMS", offsetof(ScreenRec, DPMS), sizeof(DPMSProcPtr), 0},
};

static void
screenrec_take_snapshot(ScreenRecSnapshot *snap)
{
    int i;
    if (!snap)
        return;

    snap->screen_count = screenInfo.numScreens;
    for (i = 0; i < screenInfo.numScreens && i < MAX_SCREENS; i++) {
        ScreenRec *scr = screenInfo.screens[i];
        if (!scr) continue;
        snap->screens[i] = scr;
        /* We'll allocate snapshots lazily */
    }
    snap->timestamp_ms = GetTimeInMillis();
}

static void
screenrec_alloc_snapshots(ScreenRecSnapshot *snap)
{
    int i;
    if (!snap) return;

    /* Free existing snapshots if any */
    if (snap->struct_sizes) {
        free(snap->struct_sizes);
        snap->struct_sizes = NULL;
    }
    if (snap->snapshots) {
        for (i = 0; i < snap->screen_count; i++) {
            if (snap->snapshots[i])
                free(snap->snapshots[i]);
        }
        free(snap->snapshots);
        snap->snapshots = NULL;
    }

    if (snap->screen_count == 0)
        return;

    snap->struct_sizes = calloc(snap->screen_count, sizeof(size_t));
    snap->snapshots = calloc(snap->screen_count, sizeof(unsigned char*));
    if (!snap->struct_sizes || !snap->snapshots) {
        LogMessage(X_WARNING, "ScreenRec monitor: out of memory allocating snapshots\n");
        return;
    }

    for (i = 0; i < snap->screen_count; i++) {
        if (snap->screens[i]) {
            snap->struct_sizes[i] = sizeof(ScreenRec);  /* Could be dynamic if future changes */
            snap->snapshots[i] = malloc(snap->struct_sizes[i]);
            if (snap->snapshots[i]) {
                memcpy(snap->snapshots[i], snap->screens[i], snap->struct_sizes[i]);
            }
        }
    }
}

static void
screenrec_free_snapshots(ScreenRecSnapshot *snap)
{
    if (!snap) return;
    if (snap->struct_sizes) {
        free(snap->struct_sizes);
        snap->struct_sizes = NULL;
    }
    if (snap->snapshots) {
        for (int i = 0; i < snap->screen_count; i++) {
            if (snap->snapshots[i])
                free(snap->snapshots[i]);
        }
        free(snap->snapshots);
        snap->snapshots = NULL;
    }
}

static void
screenrec_log_change(ScreenRecSnapshot *old, ScreenRecSnapshot *new,
                     const char *reason)
{
    FILE *fp = logfp ? logfp : stderr;

    fprintf(fp, "\n=== ScreenRec Modification Detected [%lu ms] ===\n",
            new->timestamp_ms - old->timestamp_ms);
    fprintf(fp, "Reason: %s\n", reason);
    if (new->module_name)
        fprintf(fp, "Module: %s\n", new->module_name);

    for (int s = 0; s < new->screen_count && s < old->screen_count; s++) {
        ScreenRec *new_scr = new->screens[s];
        ScreenRec *old_scr = old->screens[s];
        unsigned char *old_snap = old->snapshots[s];
        unsigned char *new_snap = new->snapshots[s];

        if (!new_scr || !old_scr || !old_snap || !new_snap)
            continue;

        if (memcmp(old_snap, new_snap, sizeof(ScreenRec)) != 0) {
            fprintf(fp, "\nScreen %d changes:\n", s);

            /* Compare each monitored field */
            for (size_t i = 0; i < sizeof(screen_fields)/sizeof(screen_fields[0]); i++) {
                const ScreenField *f = &screen_fields[i];
                void *old_field = (unsigned char*)old_scr + f->offset;
                void *new_field = (unsigned char*)new_scr + f->offset;

                if (f->array_count > 1) {
                    size_t total = f->size * f->array_count;
                    if (memcmp(old_field, new_field, total) != 0) {
                        fprintf(fp, "  %s[]: array of %zu elements changed\n", f->name, f->array_count);
                        /* Could dump individual elements if needed */
                    }
                } else {
                    if (memcmp(old_field, new_field, f->size) != 0) {
                        if (f->size == sizeof(void*)) {
                            fprintf(fp, "  %s: 0x%016lx -> 0x%016lx\n",
                                    f->name,
                                    (unsigned long)*(void**)old_field,
                                    (unsigned long)*(void**)new_field);
                        } else if (f->size == sizeof(int)) {
                            fprintf(fp, "  %s: %d -> %d\n",
                                    f->name,
                                    *(int*)old_field,
                                    *(int*)new_field);
                        } else if (f->size == sizeof(short)) {
                            fprintf(fp, "  %s: %hd -> %hd\n",
                                    f->name,
                                    *(short*)old_field,
                                    *(short*)new_field);
                        } else if (f->size == sizeof(unsigned char)) {
                            fprintf(fp, "  %s: %u -> %u\n",
                                    f->name,
                                    *(unsigned char*)old_field,
                                    *(unsigned char*)new_field);
                        } else if (f->size == sizeof(unsigned long)) {
                            fprintf(fp, "  %s: 0x%016lx -> 0x%016lx\n",
                                    f->name,
                                    *(unsigned long*)old_field,
                                    *(unsigned long*)new_field);
                        } else if (f->size == sizeof(Bool)) {
                            fprintf(fp, "  %s: %s -> %s\n",
                                    f->name,
                                    *(Bool*)old_field ? "true" : "false",
                                    *(Bool*)new_field ? "true" : "false");
                        } else {
                            fprintf(fp, "  %s: size %zu bytes changed\n", f->name, f->size);
                        }
                    }
                }
            }
        }
    }

    fflush(fp);
}

static void
screenrec_check_and_log(const char *reason)
{
    ScreenRecSnapshot current;
    memset(&current, 0, sizeof(current));
    screenrec_take_snapshot(&current);
    screenrec_alloc_snapshots(&current);

    if (memcmp(&last_check, &current, sizeof(ScreenRecSnapshot)) != 0) {
        screenrec_log_change(&last_check, &current, reason);
    }

    /* Update last_check with new snapshots */
    screenrec_free_snapshots(&last_check);
    last_check = current;
}

static CARD32
screenrec_timer_callback(OsTimerPtr timer, CARD32 now, pointer arg)
{
    if (!screenrec_monitor_enabled)
        return 0;

    screenrec_check_and_log("Periodic check");
    return screenrec_monitor_interval_ms;
}

static void
screenrec_segv_handler(int sig, siginfo_t *info, void *ucontext)
{
    if (!mprotect_enabled)
        return;

    for (int i = 0; i < screenrec_mprotect_count; i++) {
        if (screenrec_mprotect_bases[i] &&
            info->si_addr >= screenrec_mprotect_bases[i] &&
            info->si_addr < (char*)screenrec_mprotect_bases[i] + getpagesize()) {
            const char *msg = "ScreenRec: Detected write at ";
            write(STDERR_FILENO, msg, strlen(msg));
            char buf[32];
            int len = snprintf(buf, sizeof(buf), "%p - allowing write\n", info->si_addr);
            if (len > 0)
                write(STDERR_FILENO, buf, len);
            /* Make all pages writable and disable further trapping */
            for (int j = 0; j < screenrec_mprotect_count; j++) {
                if (screenrec_mprotect_bases[j])
                    mprotect(screenrec_mprotect_bases[j], getpagesize(), PROT_READ | PROT_WRITE);
            }
            mprotect_enabled = FALSE;
            return;
        }
    }
}

static void
screenrec_setup_mprotect(void)
{
    /* For each screen, mprotect its ScreenRec */
    screenrec_mprotect_count = screenInfo.numScreens;
    if (screenrec_mprotect_count == 0)
        return;

    screenrec_mprotect_bases = calloc(screenrec_mprotect_count, sizeof(void*));
    if (!screenrec_mprotect_bases)
        return;

    uintptr_t page_size = getpagesize();

    for (int i = 0; i < screenInfo.numScreens; i++) {
        ScreenRec *scr = screenInfo.screens[i];
        if (!scr)
            continue;

        uintptr_t start = (uintptr_t)scr;
        uintptr_t end = start + sizeof(ScreenRec);
        uintptr_t aligned_start = start & ~(page_size - 1);
        uintptr_t aligned_end = (end + page_size - 1) & ~(page_size - 1);

        void *base = (void*)aligned_start;
        size_t size = aligned_end - aligned_start;

        if (mprotect(base, size, PROT_READ) == 0) {
            screenrec_mprotect_bases[i] = base;
            LogMessage(X_INFO, "ScreenRec monitor: mprotect enabled for screen %d at 0x%lx size %lu\n",
                       i, (unsigned long)base, size);
        }
    }

    if (screenrec_mprotect_bases[0]) {
        mprotect_enabled = TRUE;
        /* Install signal handler */
        struct sigaction sa;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = SA_SIGINFO | SA_RESTART;
        sa.sa_sigaction = screenrec_segv_handler;
        if (sigaction(SIGSEGV, &sa, NULL) != 0) {
            LogMessage(X_WARNING, "ScreenRec monitor: failed to install SIGSEGV handler\n");
        }
    }
}

/* Global signal handler for ScreenRec writes */
static void
screenrec_segv_handler(int sig, siginfo_t *info, void *ucontext)
{
    if (!mprotect_enabled)
        return;

    for (int i = 0; i < screenrec_mprotect_count; i++) {
        if (screenrec_mprotect_bases[i] &&
            info->si_addr >= screenrec_mprotect_bases[i] &&
            info->si_addr < (char*)screenrec_mprotect_bases[i] + getpagesize()) {
            const char *msg = "ScreenRec: Detected write at ";
            write(STDERR_FILENO, msg, strlen(msg));
            char buf[32];
            int len = snprintf(buf, sizeof(buf), "%p - allowing write\n", info->si_addr);
            if (len > 0)
                write(STDERR_FILENO, buf, len);
            /* Make all pages writable and disable further trapping */
            for (int j = 0; j < screenrec_mprotect_count; j++) {
                if (screenrec_mprotect_bases[j])
                    mprotect(screenrec_mprotect_bases[j], getpagesize(), PROT_READ | PROT_WRITE);
            }
            mprotect_enabled = FALSE;
            return;
        }
    }
}

void
ScreenRecMonitorInit(void)
{
    const char *env = getenv("SCREENREC_MONITOR");
    if (!env || strcmp(env, "1") != 0)
        return;

    if (getenv("SCREENREC_MONITOR_SETUP_ONLY"))
        monitor_setup_only = TRUE;

    screenrec_monitor_enabled = TRUE;

    logfp = fopen(screenrec_logfile, "w");
    if (!logfp)
        LogMessage(X_WARNING, "ScreenRec monitor: cannot open log file %s\n", screenrec_logfile);

    screenrec_take_snapshot(&initial_snap);
    screenrec_alloc_snapshots(&initial_snap);
    last_check = initial_snap;  /* deep copy */

    LogMessage(X_INFO, "ScreenRec monitor: enabled, logging to %s\n", screenrec_logfile);
    if (monitor_setup_only)
        LogMessage(X_INFO, "ScreenRec monitor: setup-only mode\n");

    screenrec_check_and_log("After initialization");

    if (!monitor_setup_only) {
        screenrec_timer = TimerSet(screenrec_timer, 0, screenrec_monitor_interval_ms, screenrec_timer_callback, NULL);
    }

    if (getenv("SCREENREC_MPROTECT")) {
        screenrec_setup_mprotect();
    }
}

void
ScreenRecMonitorCheckAfterModule(const char *module_name)
{
    if (!screenrec_monitor_enabled)
        return;

    ScreenRecSnapshot before_module = last_check;
    screenrec_check_and_log("After module load");

    if (memcmp(&before_module, &last_check, sizeof(ScreenRecSnapshot)) != 0) {
        LogMessage(X_INFO, "ScreenRec was modified by module: %s\n", module_name);
    }
}

void
ScreenRecMonitorFini(void)
{
    if (screenrec_timer) {
        TimerCancel(screenrec_timer);
        TimerFree(screenrec_timer);
        screenrec_timer = NULL;
    }
    if (logfp) {
        fclose(logfp);
        logfp = NULL;
    }
    screenrec_free_snapshots(&initial_snap);
    screenrec_free_snapshots(&last_check);
    if (screenrec_mprotect_bases) {
        free(screenrec_mprotect_bases);
        screenrec_mprotect_bases = NULL;
    }
    screenrec_monitor_enabled = FALSE;
}

/* SPDX-License-Identifier: MIT OR X11 OR AGPLv3
 *
 * Copyright © 2026 Enrico Weigelt, metux IT consult <info@metux.net>
 *
 * ProcVector modification monitor for detecting NVIDIA driver alterations.
 * This is used for compatibility analysis and stability testing.
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
#include <X11/Xproto.h>
#include "dix/dix_priv.h"
#include "dix/reqhandlers_priv.h"
#include "dispatch.h"
#include "os.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <signal.h>

#define PROC_VECTOR_SIZE 256

typedef struct {
    uintptr_t proc_vector[PROC_VECTOR_SIZE];
    uintptr_t swapped_proc_vector[PROC_VECTOR_SIZE];
    unsigned long timestamp_ms;
    Bool nvidia_module_loaded;
    const char *module_name;
} ProcVectorSnapshot;

static ProcVectorSnapshot initial_snapshot;
static ProcVectorSnapshot last_check;
static Bool procvector_monitor_enabled = FALSE;
static Bool monitor_setup_only = FALSE; /* Only check during module setup */
static int procvector_monitor_interval_ms = 5000; /* Check every 5 seconds */
static TimerPtr procvector_timer = NULL;
static Bool mprotect_enabled = FALSE;
static void *procvector_mprotect_base = NULL;
static size_t procvector_mprotect_size = 0;
static void *original_procvector_protection = NULL;

/* File to log modifications */
static const char *procvector_logfile = "/tmp/procvector_changes.log";
static FILE *logfp = NULL;

static void
procvector_take_snapshot(ProcVectorSnapshot *snap, const char *module)
{
    int i;
    if (!snap)
        return;

    for (i = 0; i < PROC_VECTOR_SIZE; i++) {
        snap->proc_vector[i] = (uintptr_t)ProcVector[i];
        snap->swapped_proc_vector[i] = (uintptr_t)SwappedProcVector[i];
    }
    snap->timestamp_ms = GetTimeInMillis();
    snap->nvidia_module_loaded = FALSE; /* Will be set by caller if nvidia */
    snap->module_name = module;
}

static void
procvector_log_change(ProcVectorSnapshot *old, ProcVectorSnapshot *new,
                      const char *reason)
{
    int i;
    FILE *fp = logfp ? logfp : stderr;

    fprintf(fp, "\n=== ProcVector/SwappedProcVector Modification Detected [%lu ms] ===\n",
            new->timestamp_ms - old->timestamp_ms);
    fprintf(fp, "Reason: %s\n", reason);
    if (new->module_name)
        fprintf(fp, "Module: %s\n", new->module_name);

    for (i = 0; i < PROC_VECTOR_SIZE; i++) {
        if (old->proc_vector[i] != new->proc_vector[i]) {
            fprintf(fp, "  ProcVector[%3d] changed: 0x%016lx -> 0x%016lx\n",
                    i, (unsigned long)old->proc_vector[i], (unsigned long)new->proc_vector[i]);
        }
        if (old->swapped_proc_vector[i] != new->swapped_proc_vector[i]) {
            fprintf(fp, "  SwappedProcVector[%3d] changed: 0x%016lx -> 0x%016lx\n",
                    i, (unsigned long)old->swapped_proc_vector[i], (unsigned long)new->swapped_proc_vector[i]);
        }
    }

    fflush(fp);
}

static void
procvector_check_and_log(const char *reason)
{
    ProcVectorSnapshot current;
    procvector_take_snapshot(&current, NULL);

    if (memcmp(&last_check, &current, sizeof(ProcVectorSnapshot)) != 0) {
        procvector_log_change(&last_check, &current, reason);
        last_check = current;
    }
}

static CARD32
procvector_timer_callback(OsTimerPtr timer, CARD32 now, pointer arg)
{
    if (!procvector_monitor_enabled)
        return 0;

    procvector_check_and_log("Periodic check");
    return procvector_monitor_interval_ms;
}

/* Attempt to set memory protection to catch writes */
static void
procvector_setup_mprotect(void)
{
    /* Align to page boundary */
    uintptr_t start = (uintptr_t)ProcVector;
    uintptr_t end = (uintptr_t)SwappedProcVector + sizeof(SwappedProcVector);
    uintptr_t page_size = getpagesize();

    uintptr_t aligned_start = start & ~(page_size - 1);
    uintptr_t aligned_end = (end + page_size - 1) & ~(page_size - 1);

    procvector_mprotect_base = (void *)aligned_start;
    procvector_mprotect_size = aligned_end - aligned_start;

    if (mprotect(procvector_mprotect_base, procvector_mprotect_size, PROT_READ) == 0) {
        LogMessage(X_INFO, "ProcVector monitor: mprotect enabled on 0x%lx size %lu\n",
                   (unsigned long)procvector_mprotect_base, procvector_mprotect_size);
        mprotect_enabled = TRUE;

        /* Set up signal handler to catch writes */
        struct sigaction sa;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = SA_SIGINFO | SA_RESTART;
        sa.sa_sigaction = procvector_segv_handler;
        if (sigaction(SIGSEGV, &sa, NULL) != 0) {
            LogMessage(X_WARNING, "ProcVector monitor: failed to install SIGSEGV handler\n");
        }
    } else {
        LogMessage(X_WARNING, "ProcVector monitor: mprotect failed (permissions?)\n");
    }
}

/* Signal handler to catch SIGSEGV from mprotect */
static void
procvector_segv_handler(int sig, siginfo_t *info, void *ucontext)
{
    if (mprotect_enabled && info->si_addr >= procvector_mprotect_base &&
        info->si_addr < (char *)procvector_mprotect_base + procvector_mprotect_size) {

        /* Async-signal-safe logging */
        const char *msg1 = "ProcVector: Detected write at ";
        const char *msg2 = " - mprotect disabled, allowing write to proceed.\n";
        write(STDERR_FILENO, msg1, strlen(msg1));

        /* Simple hex output of address (async-signal-safe) */
        uintptr_t addr = (uintptr_t)info->si_addr;
        char hex[2];
        const char *hexdigits = "0123456789abcdef";
        /* Print in reverse order to buffer then write out correctly */
        char buf[sizeof(void*) * 2 + 2];
        int i = 0;
        for (int nibble = sizeof(void*)*2 - 1; nibble >= 0; nibble--) {
            buf[i++] = hexdigits[(addr >> (nibble * 4)) & 0xf];
        }
        buf[i] = '\n';
        write(STDERR_FILENO, buf, i+1);

        write(STDERR_FILENO, msg2, strlen(msg2));

        /* Make the region writable so the instruction can complete */
        mprotect(procvector_mprotect_base, procvector_mprotect_size, PROT_READ | PROT_WRITE);
        mprotect_enabled = FALSE; /* Disable further mprotect trapping */
    }
}

/* Enable monitoring - call from InitOutput or after ProcVector is initialized */
void
ProcVectorMonitorInit(void)
{
    const char *env = getenv("PROCVECTOR_MONITOR");
    if (!env || strcmp(env, "1") != 0)
        return;

    /* Check if we should only monitor during driver setup (not periodically) */
    if (getenv("PROCVECTOR_MONITOR_SETUP_ONLY"))
        monitor_setup_only = TRUE;

    procvector_monitor_enabled = TRUE;

    logfp = fopen(procvector_logfile, "w");
    if (!logfp)
        LogMessage(X_WARNING, "ProcVector monitor: cannot open log file %s\n", procvector_logfile);

    procvector_take_snapshot(&initial_snapshot, "initial");
    last_check = initial_snapshot;

    LogMessage(X_INFO, "ProcVector monitor: enabled, logging to %s\n", procvector_logfile);
    if (monitor_setup_only)
        LogMessage(X_INFO, "ProcVector monitor: setup-only mode (no periodic checks)\n");

    /* Check immediately after initialization */
    procvector_check_and_log("After initialization");

    /* Set up periodic timer only if not in setup-only mode */
    if (!monitor_setup_only) {
        if (!procvector_timer) {
            procvector_timer = TimerSet(procvector_timer, 0, procvector_monitor_interval_ms, procvector_timer_callback, NULL);
        }
    }

    /* Optional mprotect-based detection */
    if (getenv("PROCVECTOR_MPROTECT")) {
        procvector_setup_mprotect();
    }
}

/* Call this after loading a module (e.g., from LoaderLoadModule) */
void
ProcVectorMonitorCheckAfterModule(const char *module_name)
{
    if (!procvector_monitor_enabled)
        return;

    char reason[256];
    snprintf(reason, sizeof(reason), "After loading module: %s", module_name);

    ProcVectorSnapshot before_module = last_check;
    procvector_check_and_log(reason);

    if (memcmp(&before_module, &last_check, sizeof(ProcVectorSnapshot)) != 0) {
        LogMessage(X_INFO, "ProcVector was modified by module: %s\n", module_name);
    }
}

/* Explicit check for NVIDIA driver - call after nvidia module load */
void
ProcVectorMonitorCheckNVIDIA(void)
{
    ProcVectorMonitorCheckAfterModule("nvidia");
}

/* Disable monitoring (cleanup) */
void
ProcVectorMonitorFini(void)
{
    if (procvector_timer) {
        TimerFree(procvector_timer);
        procvector_timer = NULL;
    }
    if (logfp) {
        fclose(logfp);
        logfp = NULL;
    }
    procvector_monitor_enabled = FALSE;
}

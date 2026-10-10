/* SPDX-License-Identifier: MIT OR X11
 *
 * Copyright © 2026 stefan11111 <stefan11111@shitposting.expert>
 */

#include <kdrive-config.h>

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#include "kdrive.h"
#include "evdev.h"

#define EVDEV_FMT "/dev/input/event%d"

#define PROC_DEVICES "/proc/bus/input/devices"

#define PHYS_MAX 64 /* Busid + device id */
#define EVDEV_NAME_MAX 256

#define MOUSE_EV (1 << 2)
#define KBD_EV 0x120013

enum {
    EVDEV_KEYBOARD = 0,
    EVDEV_MOUSE = 1,
};

#define NUM_FALLBACK_EVDEV 32

/* Simple fallback that was already here */
static char*
FallbackEvdevCheck(void)
{
    char fallback_dev[] = "/dev/input/eventxx";

    for (int i = 0; i < NUM_FALLBACK_EVDEV; i++) {
        sprintf(fallback_dev, EVDEV_FMT, i);
        int fd = open(fallback_dev, O_RDWR);
        if (fd >= 0) {
            close(fd);
            return strdup(fallback_dev);
        }
    }

    return NULL;
}

/* All numbers read are in base 16 */
static inline uint64_t
read_val(const char *val)
{
    return strtol(val, NULL, 16);
}

typedef struct {
    uint32_t Bus; /* Bus= */
    uint32_t Vendor; /* Vendor= */
    uint32_t Product; /* Product= */
    uint32_t Version; /* Version= */
} EvdevOptionalInfo;

typedef struct {
/**
 * Info that should be unique across physical devices,
 * but not across logical devices.
 */
    EvdevOptionalInfo info; /* I: */
    char Phys[PHYS_MAX]; /* P: Phys = */
 /* char *Sysfs; */ /* S: Sysfs= */
    uint64_t Uniq; /* U: Uniq= */

    char Name[EVDEV_NAME_MAX]; /* N: Name = */

    int EventNo; /* H: Handlers=... eventxx ... */

    /* If checking for these 2 ever causes problems, remove them */
    int is_mouse; /* H: Handlers=... mousexx ... */
    int is_kbd; /* H: handlers=... kbd ... */

    uint64_t EV; /* B: EV= */
    int is_read;
} EventDevice;

static EventDevice DefaultPtr = {0};
static EventDevice DefaultKbd = {0};

static inline void
ReadOptInfo(EvdevOptionalInfo *dst, const char* data)
{
    const char *val = NULL;

    val = strstr(data, "Bus=");
    if (val) {
        val += sizeof("Bus=") - 1;
        dst->Bus = read_val(val);
    }

    val = strstr(data, "Vendor=");
    if (val) {
        val += sizeof("Vendor=") - 1;
        dst->Vendor = read_val(val);
    }

    val = strstr(data, "Product=");
    if (val) {
        val += sizeof("Product=") - 1;
        dst->Product = read_val(val);
    }

    val = strstr(data, "Version=");
    if (val) {
        val += sizeof("Version=") - 1;
        dst->Version = read_val(val);
    }
}

static inline void
ReadName(char *dst, const char* data)
{
    char *p = dst;

    data = strstr(data, "Name=");
    if (!data) {
        return;
    }

    data = strchr(data, '"');
    if (!data) {
        return;
    }
    data++;

    while (*data && *data != '"' && p - dst < EVDEV_NAME_MAX - 1) {
        *p = *data;
        p++;
        data++;
    }
    *p = '\0';
}

static inline void
ReadPhys(char *dst, const char* data)
{
    char *p = dst;

    data = strstr(data, "Phys=");
    if (!data) {
        return;
    }

    data += sizeof("Phys=") - 1;
    while (*data && *data != '/' && p - dst < PHYS_MAX - 1) {
        *p = *data;
        p++;
        data++;
    }
    *p = '\0';
}

static inline void
ReadUniq(uint64_t *dst, const char* data)
{
    data = strstr(data, "Uniq=");
    if (!data) {
        return;
    }

    data += sizeof("Uniq=") - 1;
    *dst = read_val(data);
}

static inline void
ReadHandlers(EventDevice *dst, const char* data)
{
    dst->is_mouse = !!strstr(data, "mouse");
    dst->is_kbd = !!strstr(data, "kbd");

    data = strstr(data, "event");
    if (!data) {
        /* If this one is missing, we really can't do anything */
        dst->EventNo = -1;
    }

    data += sizeof("event") - 1;

    /* This one is base10 */
    dst->EventNo = strtol(data, NULL, 10);
}

static inline void
ReadEV(uint64_t *EV, const char* data)
{
    data = strstr(data, "EV=");
    if (!data) {
        return;
    }

    data += sizeof("EV=") - 1;
    *EV = read_val(data);
}

static Bool
ReadEvdev(EventDevice *dst, FILE *f)
{
    for (;;) {
        char *line = NULL;
        char *end = NULL;
        char *data = NULL;
        size_t unused = 0;

        if (getline(&line, &unused, f) < 0) {
            free(line);
            return FALSE;
        }
        end = strchr(line, '\n');
        if (end) {
            *end = '\0';
        }

        if (line[0] == '\0') {
            free(line);
            dst->is_read = TRUE;
            return TRUE;
        }

        if (line[1] != ':'  ||
            line[2] == '\0' || line[3] == '\0') {
            /* Skip this line */
            free(line);
            continue;
        }

        data = line + 3;

        switch (line[0]) {
            case 'I': /* Optional info I: */
                ReadOptInfo(&dst->info, data);
                break;
            case 'N': /* N: Name="..." */
                ReadName(dst->Name, data);
                break;
            case 'P': /* P: Phys= */
                ReadPhys(dst->Phys, data);
                break;
            case 'U': /* U; Uniq= */
                ReadUniq(&dst->Uniq, data);
                break;
            case 'H': /* H: Handlers= */
                ReadHandlers(dst, data);
                break;
            case 'B': /* B: ... */
                ReadEV(&dst->EV, data);
                break;
        }

        free(line);
    }
}

static inline Bool
EvdevIsKbd(EventDevice *dev)
{
    return dev->is_kbd && ((dev->EV & KBD_EV) == KBD_EV);
}

static inline Bool
EvdevIsPtr(EventDevice *dev)
{
    return dev->is_mouse && ((dev->EV & MOUSE_EV) == MOUSE_EV);
}

static Bool
EvdevDifferentDevices(EventDevice *a, EventDevice *b)
{
#define IS_DIFFERENT(x, y) ((x) && (y) && (x) != (y))
#define IS_DIFF(f) if (IS_DIFFERENT(a->f, b->f)) { return TRUE; }

    if (!a->is_read || !b->is_read) {
        return TRUE;
    }

    IS_DIFF(Uniq);

    IS_DIFF(info.Bus);
    IS_DIFF(info.Vendor);
    IS_DIFF(info.Product);
    IS_DIFF(info.Version);

    if (a->Phys[0] && b->Phys[0] && strcmp(a->Phys, b->Phys)) {
        return TRUE;
    }

    return FALSE;
}

static char*
EvdevDefaultDevice(char **name, int type)
{
    char *ret = NULL;
    FILE *f = NULL;
    EventDevice read_dev = {0};

    EventDevice *desired = (type == EVDEV_KEYBOARD) ?
                           &DefaultKbd : &DefaultPtr;

    EventDevice *other = (type == EVDEV_KEYBOARD) ?
                         &DefaultPtr : &DefaultKbd;

    if (desired->is_read) {
        if (asprintf(&ret, EVDEV_FMT, desired->EventNo) < 0) {
            return FallbackEvdevCheck();
        }
        return ret;
    }

    f = fopen(PROC_DEVICES, "r");
    if (!f) {
        return FallbackEvdevCheck();
    }

    for (;;) {
        if (feof(f)) {
            fclose(f);
            return FallbackEvdevCheck();
        }

        memset(&read_dev, 0, sizeof(read_dev));

        if (!ReadEvdev(&read_dev, f)) {
            fclose(f);
            return FallbackEvdevCheck();
        }

        if (read_dev.EventNo == -1) {
            continue;
        }

        if (type == EVDEV_KEYBOARD && !EvdevIsKbd(&read_dev)) {
            continue;
        }

        if (type == EVDEV_MOUSE && !EvdevIsPtr(&read_dev)) {
            continue;
        }

        /**
         * Sometimes, modern mice advertise themselved as keyboards.
         * As such, we have to check that the mouse and keyboard
         * are separate physical devices.
         *
         * Keyboards rarely advertise themselves as mice,
         * but it doesn't hurt to check them too.
         */
        if (EvdevDifferentDevices(&read_dev, other)) {
            memcpy(desired, &read_dev, sizeof(read_dev));
            fclose(f);
            if (asprintf(&ret, EVDEV_FMT, desired->EventNo) < 0) {
                return FallbackEvdevCheck();
            }
            if (name && read_dev.Name[0] != '\0') {
                char *old_name = *name;
                *name = strdup(read_dev.Name);
                if (*name) {
                    free(old_name);
                } else {
                    *name = old_name;
                }
            }
            return ret;
        }
    }

    /* Unreachable */
    fclose(f);
    return FallbackEvdevCheck();
}

char*
EvdevDefaultKbd(char **name)
{
    return EvdevDefaultDevice(name, EVDEV_KEYBOARD);
}

char*
EvdevDefaultPtr(char **name)
{
    return EvdevDefaultDevice(name, EVDEV_MOUSE);
}

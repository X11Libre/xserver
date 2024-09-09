/*
 * Copyright © 2011-2014 Intel Corporation
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without
 * fee, provided that the above copyright notice appear in all copies
 * and that both that copyright notice and this permission notice
 * appear in supporting documentation, and that the name of the
 * copyright holders not be used in advertising or publicity
 * pertaining to distribution of the software without specific,
 * written prior permission.  The copyright holders make no
 * representations about the suitability of this software for any
 * purpose.  It is provided "as is" without express or implied
 * warranty.
 *
 * THE COPYRIGHT HOLDERS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS
 * SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS, IN NO EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
 * AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING
 * OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 */

#include <xwayland-config.h>

#include <sys/mman.h>

#include <drm_fourcc.h>
#include <wayland-util.h>

#include "os/log_priv.h"

#include "xwayland-dmabuf.h"
#include "xwayland-glamor-gbm.h"
#include "xwayland-screen.h"
#include "xwayland-types.h"
#include "xwayland-window-buffers.h"

#include "drm-client-protocol.h"
#include "linux-dmabuf-unstable-v1-client-protocol.h"

void
xwl_device_formats_destroy(struct xwl_device_formats *dev_formats)
{
    for (int j = 0; j < dev_formats->num_formats; j++)
        free(dev_formats->formats[j].modifiers);
    free(dev_formats->formats);
    drmFreeDevice(&dev_formats->drm_dev);
}

void
xwl_dmabuf_feedback_clear_dev_formats(struct xwl_dmabuf_feedback *xwl_feedback)
{
    if (xwl_feedback->dev_formats_len == 0)
        return;

    for (int i = 0; i < xwl_feedback->dev_formats_len; i++) {
        struct xwl_device_formats *dev_format = &xwl_feedback->dev_formats[i];
        xwl_device_formats_destroy(dev_format);
    }
    free(xwl_feedback->dev_formats);
    xwl_feedback->dev_formats = NULL;
    xwl_feedback->dev_formats_len = 0;
}

void
xwl_dmabuf_feedback_destroy(struct xwl_dmabuf_feedback *xwl_feedback)
{
    munmap(xwl_feedback->format_table.entry,
           xwl_feedback->format_table.len * sizeof(struct xwl_format_table_entry));
    xwl_dmabuf_feedback_clear_dev_formats(xwl_feedback);

    if (xwl_feedback->dmabuf_feedback)
        zwp_linux_dmabuf_feedback_v1_destroy(xwl_feedback->dmabuf_feedback);

    xwl_feedback->dmabuf_feedback = NULL;
    drmFreeDevice(&xwl_feedback->main_dev);
}

static Bool
xwl_glamor_is_modifier_supported_in_formats(struct xwl_format *formats, int num_formats,
                                            uint32_t format, uint64_t modifier)
{
    struct xwl_format *xwl_format = NULL;
    int i;

    for (i = 0; i < num_formats; i++) {
        if (formats[i].format == format) {
            xwl_format = &formats[i];
            break;
        }
    }

    if (xwl_format) {
        for (i = 0; i < xwl_format->num_modifiers; i++) {
            if (xwl_format->modifiers[i] == modifier) {
                return TRUE;
            }
        }
    }

    return FALSE;
}

static Bool
xwl_feedback_is_modifier_supported(struct xwl_dmabuf_feedback *xwl_feedback,
                                   uint32_t format, uint64_t modifier,
                                   int supports_scanout)
{
    for (int i = 0; i < xwl_feedback->dev_formats_len; i++) {
        struct xwl_device_formats *dev_formats = &xwl_feedback->dev_formats[i];

        if (supports_scanout && !dev_formats->supports_scanout)
            continue;

        if (xwl_glamor_is_modifier_supported_in_formats(dev_formats->formats,
                                                        dev_formats->num_formats,
                                                        format, modifier))
            return TRUE;
    }

    return FALSE;
}

Bool
xwl_glamor_is_modifier_supported(struct xwl_screen *xwl_screen,
                                 uint32_t format, uint64_t modifier)
{
    struct xwl_window *xwl_window;

    /*
     * If we are using dmabuf v4, then we need to check in the main
     * device and per-window format lists. For older protocol
     * versions we can just check the list returned by the dmabuf.modifier
     * events in xwl_screen
     */
    if (xwl_screen->dmabuf_protocol_version < 4) {
        return xwl_glamor_is_modifier_supported_in_formats(xwl_screen->formats,
                                                           xwl_screen->num_formats,
                                                           format, modifier);
    }

    if (xwl_feedback_is_modifier_supported(&xwl_screen->default_feedback, format, modifier, FALSE))
        return TRUE;

    xorg_list_for_each_entry(xwl_window, &xwl_screen->window_list, link_window) {
        if (xwl_feedback_is_modifier_supported(&xwl_window->feedback, format, modifier, FALSE))
            return TRUE;
    }

    return FALSE;
}

uint32_t
wl_drm_format_for_depth(int depth)
{
    switch (depth) {
    case 15:
        return WL_DRM_FORMAT_XRGB1555;
    case 16:
        return WL_DRM_FORMAT_RGB565;
    case 24:
        return WL_DRM_FORMAT_XRGB8888;
    case 30:
        return WL_DRM_FORMAT_ARGB2101010;
    default:
        ErrorF("unexpected depth: %d\n", depth);
    case 32:
        return WL_DRM_FORMAT_ARGB8888;
    }
}

static Bool
xwl_dmabuf_get_formats(struct xwl_format *format_array, int format_array_len,
                       CARD32 *num_formats, CARD32 **formats)
{
    *num_formats = 0;
    *formats = NULL;

    if (format_array_len == 0)
       return TRUE;

    *formats = calloc(format_array_len, sizeof(CARD32));
    if (*formats == NULL)
        return FALSE;

    for (int i = 0; i < format_array_len; i++)
       (*formats)[i] = format_array[i].format;
    *num_formats = format_array_len;

    return TRUE;
}

static Bool
xwl_dmabuf_get_formats_for_device(struct xwl_dmabuf_feedback *xwl_feedback, drmDevice *device,
                                  CARD32 *num_formats, CARD32 **formats)
{
    CARD32 *ret = NULL;
    uint32_t count = 0;

    /* go through all matching sets of tranches for the window's device */
    for (int i = 0; i < xwl_feedback->dev_formats_len; i++) {
        if (drmDevicesEqual(xwl_feedback->dev_formats[i].drm_dev, device)) {
            struct xwl_device_formats *dev_formats = &xwl_feedback->dev_formats[i];

            /* Append the formats from this tranche to the list */
            ret = XNFreallocarray(ret, count + dev_formats->num_formats, sizeof(CARD32));

            for (int j = 0; j < dev_formats->num_formats; j++) {
                Bool found = FALSE;

                /* Check if this format is already present in the list */
                for (int k = 0; k < count; k++) {
                    if (ret[k] == dev_formats->formats[j].format) {
                        found = TRUE;
                        break;
                    }
                }

                /* If this format has not yet been added, do so now */
                if (!found)
                    ret[count++] = dev_formats->formats[j].format;
            }
        }
    }

    *num_formats = count;
    *formats = ret;

    return TRUE;
}

Bool
xwl_glamor_get_formats(ScreenPtr screen,
                       CARD32 *num_formats, CARD32 **formats)
{
    struct xwl_screen *xwl_screen = xwl_screen_get(screen);

    /* Explicitly zero the count as the caller may ignore the return value */
    *num_formats = 0;

    if (!xwl_screen->dmabuf)
        return FALSE;

    if (xwl_screen->dmabuf_protocol_version >= 4) {
        drmDevice *main_dev = xwl_gbm_get_main_device(xwl_screen);

        return xwl_dmabuf_get_formats_for_device(&xwl_screen->default_feedback, main_dev,
                                          num_formats, formats);
    }

    return xwl_dmabuf_get_formats(xwl_screen->formats, xwl_screen->num_formats,
                           num_formats, formats);
}

static Bool
xwl_dmabuf_get_modifiers_for_format(struct xwl_format *format_array, int num_formats,
                                    uint32_t format, uint32_t *num_modifiers,
                                    uint64_t **modifiers)
{
    struct xwl_format *xwl_format = NULL;
    int i;

    *num_modifiers = 0;
    *modifiers = NULL;

    if (num_formats == 0)
       return TRUE;

    for (i = 0; i < num_formats; i++) {
       if (format_array[i].format == format) {
          xwl_format = &format_array[i];
          break;
       }
    }

    if (!xwl_format ||
        (xwl_format->num_modifiers == 1 &&
         xwl_format->modifiers[0] == DRM_FORMAT_MOD_INVALID))
        return FALSE;

    *modifiers = calloc(xwl_format->num_modifiers, sizeof(uint64_t));
    if (*modifiers == NULL)
        return FALSE;

    for (i = 0; i < xwl_format->num_modifiers; i++)
       (*modifiers)[i] = xwl_format->modifiers[i];
    *num_modifiers = xwl_format->num_modifiers;

    return TRUE;
}

static Bool
xwl_dmabuf_get_modifiers_for_device(struct xwl_dmabuf_feedback *feedback,
                                    drmDevice *device,
                                    uint32_t format, uint32_t *num_modifiers,
                                    uint64_t **modifiers,
                                    Bool *supports_scanout)
{
    /* Now try to find a matching set of tranches for the window's device */
    for (int i = 0; i < feedback->dev_formats_len; i++) {
        struct xwl_device_formats *dev_formats = &feedback->dev_formats[i];

        if (drmDevicesEqual(dev_formats->drm_dev, device) &&
            xwl_dmabuf_get_modifiers_for_format(dev_formats->formats,
                                                dev_formats->num_formats,
                                                format, num_modifiers, modifiers)) {
            if (supports_scanout)
                *supports_scanout = !!dev_formats->supports_scanout;
            return TRUE;
        }
    }

    return FALSE;
}

Bool
xwl_glamor_get_modifiers(ScreenPtr screen, uint32_t format,
                         uint32_t *num_modifiers, uint64_t **modifiers)
{
    struct xwl_screen *xwl_screen = xwl_screen_get(screen);
    drmDevice *main_dev;

    /* Explicitly zero the count as the caller may ignore the return value */
    *num_modifiers = 0;
    *modifiers = NULL;

    if (!xwl_screen->dmabuf)
        return FALSE;

    if (xwl_screen->dmabuf_protocol_version >= 4) {
        main_dev = xwl_gbm_get_main_device(xwl_screen);

        return xwl_dmabuf_get_modifiers_for_device(&xwl_screen->default_feedback, main_dev,
                                                   format, num_modifiers, modifiers, NULL);
    } else {
        return xwl_dmabuf_get_modifiers_for_format(xwl_screen->formats, xwl_screen->num_formats,
                                                   format, num_modifiers, modifiers);
    }
}

Bool
xwl_glamor_get_drawable_modifiers_and_scanout(DrawablePtr drawable,
                                              uint32_t format,
                                              uint32_t *num_modifiers,
                                              uint64_t **modifiers,
                                              Bool *supports_scanout)
{
    struct xwl_screen *xwl_screen = xwl_screen_get(drawable->pScreen);
    struct xwl_window *xwl_window;
    drmDevice *main_dev;

    *num_modifiers = 0;
    *modifiers = NULL;
    if (supports_scanout)
        *supports_scanout = FALSE;

    /* We can only return per-drawable modifiers if the compositor supports feedback */
    if (xwl_screen->dmabuf_protocol_version < 4)
        return TRUE;

    if (drawable->type != DRAWABLE_WINDOW || !xwl_screen->dmabuf)
        return FALSE;

    xwl_window = xwl_window_from_window((WindowPtr)drawable);

    /* couldn't find drawable for window */
    if (!xwl_window)
        return FALSE;

    main_dev = xwl_gbm_get_main_device(xwl_screen);

    return xwl_dmabuf_get_modifiers_for_device(&xwl_window->feedback, main_dev,
                                               format, num_modifiers, modifiers,
                                               supports_scanout);

}

Bool
xwl_glamor_get_drawable_modifiers(DrawablePtr drawable, uint32_t format,
                                  uint32_t *num_modifiers, uint64_t **modifiers)
{
    return xwl_glamor_get_drawable_modifiers_and_scanout(drawable,
                                                         format, num_modifiers,
                                                         modifiers, NULL);

}

static void
xwl_dmabuf_handle_format(void *data, struct zwp_linux_dmabuf_v1 *dmabuf,
                         uint32_t format)
{
}

static void
xwl_add_format_and_mod_to_list(struct xwl_format **formats,
                               uint32_t *num_formats,
                               uint32_t format,
                               uint64_t modifier)
{
    struct xwl_format *xwl_format = NULL;
    int i;

    for (i = 0; i < *num_formats; i++) {
        if ((*formats)[i].format == format) {
            xwl_format = &(*formats)[i];
            break;
        }
    }

    if (xwl_format == NULL) {
        (*num_formats)++;
        *formats = XNFrealloc(*formats, *num_formats * sizeof(*xwl_format));
        xwl_format = &(*formats)[*num_formats - 1];
        xwl_format->format = format;
        xwl_format->num_modifiers = 0;
        xwl_format->modifiers = NULL;
    }

    for (i = 0; i < xwl_format->num_modifiers; i++) {
        /* don't add it if the modifier already exists */
        if (xwl_format->modifiers[i] == modifier)
            return;
    }

    xwl_format->num_modifiers++;
    xwl_format->modifiers = XNFrealloc(xwl_format->modifiers,
                                       xwl_format->num_modifiers * sizeof(uint64_t));
    xwl_format->modifiers[xwl_format->num_modifiers - 1]  = modifier;
}

static void
xwl_dmabuf_handle_modifier(void *data, struct zwp_linux_dmabuf_v1 *dmabuf,
                           uint32_t format, uint32_t modifier_hi,
                           uint32_t modifier_lo)
{
    struct xwl_screen *xwl_screen = data;

    xwl_add_format_and_mod_to_list(&xwl_screen->formats, &xwl_screen->num_formats,
                                   format,
                                   ((uint64_t)modifier_hi << 32 | (uint64_t)modifier_lo));
}

static const struct zwp_linux_dmabuf_v1_listener xwl_dmabuf_listener = {
    .format = xwl_dmabuf_handle_format,
    .modifier = xwl_dmabuf_handle_modifier
};

/*
 * We need to check if the compositor is resending all of the tranche
 * information. Each tranche event will call this method to see
 * if the existing format info should be cleared before refilling.
 */
static void
xwl_check_reset_tranche_info(struct xwl_dmabuf_feedback *xwl_feedback)
{
    if (!xwl_feedback->feedback_done)
        return;

    xwl_feedback->feedback_done = FALSE;

    xwl_dmabuf_feedback_clear_dev_formats(xwl_feedback);
}

static void
xwl_dmabuf_feedback_main_device(void *data,
                                struct zwp_linux_dmabuf_feedback_v1 *dmabuf_feedback,
                                struct wl_array *dev)
{
    struct xwl_dmabuf_feedback *xwl_feedback = data;
    dev_t devid;

    xwl_check_reset_tranche_info(xwl_feedback);

    assert(dev->size == sizeof(dev_t));
    memcpy(&devid, dev->data, sizeof(dev_t));

    drmFreeDevice(&xwl_feedback->main_dev);

    if (drmGetDeviceFromDevId(devid, 0, &xwl_feedback->main_dev) != 0)
        ErrorF("linux_dmabuf_feedback.main_device: Failed to fetch DRM device\n");
}

static void
xwl_dmabuf_feedback_tranche_target_device(void *data,
                                          struct zwp_linux_dmabuf_feedback_v1 *dmabuf_feedback,
                                          struct wl_array *dev)
{
    struct xwl_dmabuf_feedback *xwl_feedback = data;
    dev_t devid;

    xwl_check_reset_tranche_info(xwl_feedback);

    assert(dev->size == sizeof(dev_t));
    memcpy(&devid, dev->data, sizeof(dev_t));

    if (drmGetDeviceFromDevId(devid, 0, &xwl_feedback->tmp_tranche.drm_dev) != 0)
        ErrorF("linux_dmabuf_feedback.tranche_target_device: Failed to fetch DRM device\n");
}

static void
xwl_dmabuf_feedback_tranche_flags(void *data,
                                  struct zwp_linux_dmabuf_feedback_v1 *dmabuf_feedback,
                                  uint32_t flags)
{
    struct xwl_dmabuf_feedback *xwl_feedback = data;

    xwl_check_reset_tranche_info(xwl_feedback);

    if (flags & ZWP_LINUX_DMABUF_FEEDBACK_V1_TRANCHE_FLAGS_SCANOUT)
        xwl_feedback->tmp_tranche.supports_scanout = TRUE;
}

static void
xwl_dmabuf_feedback_tranche_formats(void *data,
                                    struct zwp_linux_dmabuf_feedback_v1 *dmabuf_feedback,
                                    struct wl_array *indices)
{
    struct xwl_dmabuf_feedback *xwl_feedback = data;
    struct xwl_device_formats *tranche = &xwl_feedback->tmp_tranche;
    uint16_t *index;

    xwl_check_reset_tranche_info(xwl_feedback);

    wl_array_for_each(index, indices) {
        if (*index >= xwl_feedback->format_table.len) {
            ErrorF("linux_dmabuf_feedback.tranche_formats: Index given to us by the compositor"
                   " is too large to fit in the format table\n");
            continue;
        }

        /* Look up this format/mod in the format table */
        struct xwl_format_table_entry *entry = &xwl_feedback->format_table.entry[*index];

        /* Add it to the in-progress tranche */
        xwl_add_format_and_mod_to_list(&tranche->formats, &tranche->num_formats,
                                       entry->format,
                                       entry->modifier);
    }
}

static void
xwl_append_to_tranche(struct xwl_device_formats *dst, struct xwl_device_formats *src)
{
    struct xwl_format *format;

    for (int i = 0; i < src->num_formats; i++) {
        format = &src->formats[i];

        for (int j = 0; j < format->num_modifiers; j++)
            xwl_add_format_and_mod_to_list(&dst->formats, &dst->num_formats,
                                           format->format,
                                           format->modifiers[j]);
    }
}

static void
xwl_dmabuf_feedback_tranche_done(void *data,
                                 struct zwp_linux_dmabuf_feedback_v1 *dmabuf_feedback)
{
    struct xwl_dmabuf_feedback *xwl_feedback = data;
    struct xwl_device_formats *tranche;
    int appended = FALSE;

    /*
     * No need to call xwl_check_reset_tranche_info, the other events should have been
     * triggered first
     */

    if (xwl_feedback->tmp_tranche.drm_dev == NULL) {
        xwl_device_formats_destroy(&xwl_feedback->tmp_tranche);
        goto out;
    }

    /*
     * First check if there is an existing tranche for this device+flags combo. We
     * will combine it with this tranche, since we can only send one modifier list
     * in DRI3 but the compositor may report multiple tranches per device (KDE
     * does this)
     */
    for (int i = 0; i < xwl_feedback->dev_formats_len; i++) {
        tranche = &xwl_feedback->dev_formats[i];
        if (tranche->supports_scanout == xwl_feedback->tmp_tranche.supports_scanout &&
            drmDevicesEqual(tranche->drm_dev, xwl_feedback->tmp_tranche.drm_dev)) {
            appended = TRUE;

            /* Add all format/mods to this tranche */
            xwl_append_to_tranche(tranche, &xwl_feedback->tmp_tranche);

            /* Now free our temp tranche's allocations */
            xwl_device_formats_destroy(&xwl_feedback->tmp_tranche);

            break;
        }
    }

    if (!appended) {
        xwl_feedback->dev_formats_len++;
        xwl_feedback->dev_formats = XNFrealloc(xwl_feedback->dev_formats,
                                               sizeof(struct xwl_device_formats) *
                                               xwl_feedback->dev_formats_len);

        /* copy the temporary tranche into the official array */
        memcpy(&xwl_feedback->dev_formats[xwl_feedback->dev_formats_len - 1],
               &xwl_feedback->tmp_tranche,
               sizeof(struct xwl_device_formats));
    }

out:
    /* reset the tranche */
    memset(&xwl_feedback->tmp_tranche, 0, sizeof(struct xwl_device_formats));
}

static void
xwl_dmabuf_feedback_done(void *data, struct zwp_linux_dmabuf_feedback_v1 *dmabuf_feedback)
{
    struct xwl_dmabuf_feedback *xwl_feedback = data;

    xwl_feedback->feedback_done = TRUE;
    xwl_feedback->unprocessed_feedback_pending = TRUE;
}

static void
xwl_dmabuf_feedback_format_table(void *data,
                                 struct zwp_linux_dmabuf_feedback_v1 *zwp_linux_dmabuf_feedback_v1,
                                 int32_t fd, uint32_t size)
{
    struct xwl_dmabuf_feedback *xwl_feedback = data;
    /* Unmap the old table */
    if (xwl_feedback->format_table.entry) {
        munmap(xwl_feedback->format_table.entry,
               xwl_feedback->format_table.len * sizeof(struct xwl_format_table_entry));
    }

    assert(size % sizeof(struct xwl_format_table_entry) == 0);
    xwl_feedback->format_table.len = size / sizeof(struct xwl_format_table_entry);
    xwl_feedback->format_table.entry = mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
    close(fd);

    if (xwl_feedback->format_table.entry == MAP_FAILED) {
        ErrorF("linux_dmabuf_feedback.format_table: Could not map the format"
               " table: Compositor bug or out of resources\n");
        xwl_feedback->format_table.len = 0;
    }
}

static const struct zwp_linux_dmabuf_feedback_v1_listener xwl_dmabuf_feedback_listener = {
    .done = xwl_dmabuf_feedback_done,
    .format_table = xwl_dmabuf_feedback_format_table,
    .main_device = xwl_dmabuf_feedback_main_device,
    .tranche_done = xwl_dmabuf_feedback_tranche_done,
    .tranche_target_device = xwl_dmabuf_feedback_tranche_target_device,
    .tranche_formats = xwl_dmabuf_feedback_tranche_formats,
    .tranche_flags = xwl_dmabuf_feedback_tranche_flags,
};

Bool
xwl_screen_set_dmabuf_interface(struct xwl_screen *xwl_screen,
                                uint32_t id, uint32_t version)
{
    /* We either support versions 3 or 4. 4 is needed for dmabuf feedback */
    int supported_version = version >= 4 ? 4 : 3;

    if (version < 3)
        return FALSE;

    xwl_screen->dmabuf =
        wl_registry_bind(xwl_screen->registry, id, &zwp_linux_dmabuf_v1_interface, supported_version);
    xwl_screen->dmabuf_protocol_version = supported_version;
    zwp_linux_dmabuf_v1_add_listener(xwl_screen->dmabuf, &xwl_dmabuf_listener, xwl_screen);

    /* If the compositor supports it, request the default feedback hints */
    if (version >= 4) {
        xwl_screen->default_feedback.dmabuf_feedback =
            zwp_linux_dmabuf_v1_get_default_feedback(xwl_screen->dmabuf);
        if (!xwl_screen->default_feedback.dmabuf_feedback)
            return FALSE;

        zwp_linux_dmabuf_feedback_v1_add_listener(xwl_screen->default_feedback.dmabuf_feedback,
                                                  &xwl_dmabuf_feedback_listener,
                                                  &xwl_screen->default_feedback);
    }

    return TRUE;
}

static void
xwl_window_dmabuf_feedback_main_device(void *data,
                                       struct zwp_linux_dmabuf_feedback_v1 *dmabuf_feedback,
                                       struct wl_array *dev)
{
    struct xwl_window *xwl_window = data;

    xwl_dmabuf_feedback_main_device(&xwl_window->feedback, dmabuf_feedback, dev);
}

static void
xwl_window_dmabuf_feedback_tranche_target_device(void *data,
                                                 struct zwp_linux_dmabuf_feedback_v1 *dmabuf_feedback,
                                                 struct wl_array *dev)
{
    struct xwl_window *xwl_window = data;

    xwl_dmabuf_feedback_tranche_target_device(&xwl_window->feedback, dmabuf_feedback, dev);
}

static void
xwl_window_dmabuf_feedback_tranche_flags(void *data,
                                         struct zwp_linux_dmabuf_feedback_v1 *dmabuf_feedback,
                                         uint32_t flags)
{
    struct xwl_window *xwl_window = data;

    xwl_dmabuf_feedback_tranche_flags(&xwl_window->feedback, dmabuf_feedback, flags);
}

static void
xwl_window_dmabuf_feedback_tranche_formats(void *data,
                                           struct zwp_linux_dmabuf_feedback_v1 *dmabuf_feedback,
                                           struct wl_array *indices)
{
    struct xwl_window *xwl_window = data;

    xwl_dmabuf_feedback_tranche_formats(&xwl_window->feedback, dmabuf_feedback, indices);
}

static void
xwl_window_dmabuf_feedback_tranche_done(void *data,
                                        struct zwp_linux_dmabuf_feedback_v1 *dmabuf_feedback)
{
    struct xwl_window *xwl_window = data;

    xwl_dmabuf_feedback_tranche_done(&xwl_window->feedback, dmabuf_feedback);
}

static void
xwl_window_dmabuf_feedback_done(void *data,
                                struct zwp_linux_dmabuf_feedback_v1 *dmabuf_feedback)
{
    struct xwl_window *xwl_window = data;
    uint32_t format = wl_drm_format_for_depth(xwl_window->surface_window->drawable.depth);

    xwl_dmabuf_feedback_done(&xwl_window->feedback, dmabuf_feedback);

    xwl_window->has_implicit_scanout_support =
        xwl_feedback_is_modifier_supported(&xwl_window->feedback, format,
                                           DRM_FORMAT_MOD_INVALID, TRUE);
    DebugF("XWAYLAND: Window 0x%x can%s get implicit scanout support\n",
            xwl_window->surface_window->drawable.id,
            xwl_window->has_implicit_scanout_support ? "" : "not");

    /* If the linux-dmabuf v4 per-surface feedback changed, make sure the
     * window buffers get re-created with appropriate parameters.
     */
    xwl_window_buffers_dispose(xwl_window, FALSE);
    xwl_window_realloc_pixmap(xwl_window);
}

static void
xwl_window_dmabuf_feedback_format_table(void *data,
                                        struct zwp_linux_dmabuf_feedback_v1 *dmabuf_feedback,
                                        int32_t fd, uint32_t size)
{
    struct xwl_window *xwl_window = data;

    xwl_dmabuf_feedback_format_table(&xwl_window->feedback, dmabuf_feedback, fd, size);
}

static const struct zwp_linux_dmabuf_feedback_v1_listener xwl_window_dmabuf_feedback_listener = {
    .done = xwl_window_dmabuf_feedback_done,
    .format_table = xwl_window_dmabuf_feedback_format_table,
    .main_device = xwl_window_dmabuf_feedback_main_device,
    .tranche_done = xwl_window_dmabuf_feedback_tranche_done,
    .tranche_target_device = xwl_window_dmabuf_feedback_tranche_target_device,
    .tranche_formats = xwl_window_dmabuf_feedback_tranche_formats,
    .tranche_flags = xwl_window_dmabuf_feedback_tranche_flags,
};

Bool
xwl_dmabuf_setup_feedback_for_window(struct xwl_window *xwl_window)
{
    struct xwl_screen *xwl_screen = xwl_window->xwl_screen;

    xwl_window->feedback.dmabuf_feedback =
        zwp_linux_dmabuf_v1_get_surface_feedback(xwl_screen->dmabuf, xwl_window->surface);

    if (!xwl_window->feedback.dmabuf_feedback)
        return FALSE;

    zwp_linux_dmabuf_feedback_v1_add_listener(xwl_window->feedback.dmabuf_feedback,
                                              &xwl_window_dmabuf_feedback_listener,
                                              xwl_window);

    return TRUE;
}

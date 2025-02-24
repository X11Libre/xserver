/*
 * Copyright © 2020 Drew Devault
 * Copyright © 2021 Xaver Hugl
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

#include "os/client_priv.h"

#ifdef WITH_LIBDRM
#include <xf86drm.h>
#include <xf86drmMode.h>
#endif

#include "randrstr_priv.h"
#include "xwayland-drm-lease.h"
#include "xwayland-screen.h"
#include "xwayland-output.h"

static void
xwl_randr_lease_cleanup_outputs(RRLeasePtr rrLease)
{
    struct xwl_output *output;
    int i;

    for (i = 0; i < rrLease->numOutputs; ++i) {
        output = rrLease->outputs[i]->devPrivate;
        if (output) {
            output->lease = NULL;
        }
    }
}

static void
xwl_randr_lease_free_outputs(RRLeasePtr rrLease)
{
    struct xwl_output *xwl_output;
    int i;

    for (i = 0; i < rrLease->numOutputs; ++i) {
        xwl_output = rrLease->outputs[i]->devPrivate;
        if (xwl_output && xwl_output->withdrawn_connector) {
            rrLease->outputs[i]->devPrivate = NULL;
            xwl_output_remove(xwl_output);
        }
    }
}

static void
drm_lease_handle_lease_fd(void *data,
                          struct wp_drm_lease_v1 *wp_drm_lease_v1,
                          int32_t lease_fd)
{
    struct xwl_drm_lease *lease = (struct xwl_drm_lease *)data;

    lease->fd = lease_fd;
    AttendClient(lease->client);
}

static void
drm_lease_handle_finished(void *data,
                          struct wp_drm_lease_v1 *wp_drm_lease_v1)
{
    struct xwl_drm_lease *lease = (struct xwl_drm_lease *)data;

    if (lease->fd >= 0) {
        RRTerminateLease(lease->rrLease);
    } else {
        AttendClient(lease->client);
        xwl_randr_lease_cleanup_outputs(lease->rrLease);
    }

    /* Free the xwl_outputs that have been withdrawn while lease-able */
    xwl_randr_lease_free_outputs(lease->rrLease);
}

static struct wp_drm_lease_v1_listener drm_lease_listener = {
    .lease_fd = drm_lease_handle_lease_fd,
    .finished = drm_lease_handle_finished,
};

void
xwl_randr_get_lease(ClientPtr client, ScreenPtr screen, RRLeasePtr *rrLease, int *fd)
{
    struct xwl_screen *xwl_screen;
    struct xwl_drm_lease *lease;
    xwl_screen = xwl_screen_get(screen);

    xorg_list_for_each_entry(lease, &xwl_screen->drm_leases, link) {
        if (lease->client == client) {
            *rrLease = lease->rrLease;
            *fd = lease->fd;
            if (lease->fd < 0)
                xorg_list_del(&lease->link);
            return;
        }
    }
    *rrLease = NULL;
    *fd = -1;
}

int
xwl_randr_request_lease(ClientPtr client, ScreenPtr screen, RRLeasePtr rrLease)
{
    struct xwl_screen *xwl_screen;
    struct wp_drm_lease_request_v1 *req;
    struct xwl_drm_lease *lease_private;
    struct xwl_drm_lease_device *lease_device = NULL;
    struct xwl_drm_lease_device *device_data;
    struct xwl_output *output;
    int i;

    xwl_screen = xwl_screen_get(screen);

    if (xorg_list_is_empty(&xwl_screen->drm_lease_devices)) {
        ErrorF("Attempted to create DRM lease without wp_drm_lease_device_v1\n");
        return BadMatch;
    }

    for (i = 0; i < rrLease->numOutputs; ++i) {
        output = rrLease->outputs[i]->devPrivate;
        if (!output || !output->lease_connector || output->lease) {
            return BadValue;
        }
    }

    xorg_list_for_each_entry(device_data, &xwl_screen->drm_lease_devices, link) {
        Bool connectors_of_device = FALSE;
        for (i = 0; i < rrLease->numOutputs; ++i) {
            output = rrLease->outputs[i]->devPrivate;
            if (output->lease_device == device_data) {
                connectors_of_device = TRUE;
                break;
            }
        }
        if (connectors_of_device) {
            if (lease_device != NULL) {
                ErrorF("Attempted to create DRM lease from multiple devices\n");
                return BadValue;
            }
            lease_device = device_data;
        }
    }

    req = wp_drm_lease_device_v1_create_lease_request(
            lease_device->drm_lease_device);
    if (!(lease_private = calloc(1, sizeof(struct xwl_drm_lease))))
        return BadAlloc;
    for (i = 0; i < rrLease->numOutputs; ++i) {
        output = rrLease->outputs[i]->devPrivate;
        output->lease = lease_private;
        wp_drm_lease_request_v1_request_connector(req, output->lease_connector);
    }
    lease_private->fd = -1;
    lease_private->lease = wp_drm_lease_request_v1_submit(req);
    lease_private->rrLease = rrLease;
    lease_private->client = client;
    rrLease->devPrivate = lease_private;

    wp_drm_lease_v1_add_listener(lease_private->lease,
                                  &drm_lease_listener, lease_private);
    xorg_list_add(&lease_private->link, &xwl_screen->drm_leases);

    ResetCurrentRequest(client);
    client->sequence--;
    IgnoreClient(client);

    return Success;
}

void
xwl_randr_terminate_lease(ScreenPtr screen, RRLeasePtr lease)
{
    struct xwl_drm_lease *lease_private = lease->devPrivate;

    if (lease_private) {
        xwl_randr_lease_cleanup_outputs(lease);
        xorg_list_del(&lease_private->link);
        if (lease_private->fd >= 0)
            close(lease_private->fd);
        wp_drm_lease_v1_destroy(lease_private->lease);
        free(lease_private);
        lease->devPrivate = NULL;
    }

    RRLeaseTerminated(lease);
}

static void
lease_connector_handle_name(void *data,
                            struct wp_drm_lease_connector_v1 *wp_drm_lease_connector_v1,
                            const char *name)
{
    struct xwl_output *xwl_output = data;
    char rr_output_name[MAX_OUTPUT_NAME] = { 0 };

    snprintf(rr_output_name, MAX_OUTPUT_NAME, "lease-%s", name);
    xwl_output_set_name(xwl_output, rr_output_name);
}

static void
lease_connector_handle_description(void *data,
                                   struct wp_drm_lease_connector_v1 *wp_drm_lease_connector_v1,
                                   const char *description)
{
    /* This space is deliberately left blank */
}

static RRModePtr *
xwl_get_rrmodes_from_connector_id(int drm, int32_t connector_id, int *nmode, int *npref)
{
#ifdef WITH_LIBDRM
    drmModeConnectorPtr conn;
    drmModeModeInfoPtr kmode;
    RRModePtr *rrmodes;
    int pref, i;

    *nmode = *npref = 0;

    conn = drmModeGetConnectorCurrent(drm, connector_id);
    if (!conn) {
        ErrorF("drmModeGetConnector for connector %d failed\n", connector_id);
        return NULL;
    }
    rrmodes = calloc(conn->count_modes, sizeof(RRModePtr));
    if (!rrmodes) {
        ErrorF("Failed to allocate connector modes\n");
        drmModeFreeConnector(conn);
        return NULL;
    }

    /* This spaghetti brought to you courtesey of xf86RandrR12.c
     * It adds preferred modes first, then non-preferred modes */
    for (pref = 1; pref >= 0; pref--) {
        for (i = 0; i < conn->count_modes; ++i) {
            kmode = &conn->modes[i];
            if ((pref != 0) == ((kmode->type & DRM_MODE_TYPE_PREFERRED) != 0)) {
                xRRModeInfo modeInfo;
                RRModePtr rrmode;

                modeInfo.nameLength = strlen(kmode->name);

                modeInfo.width = kmode->hdisplay;
                modeInfo.dotClock = kmode->clock * 1000;
                modeInfo.hSyncStart = kmode->hsync_start;
                modeInfo.hSyncEnd = kmode->hsync_end;
                modeInfo.hTotal = kmode->htotal;
                modeInfo.hSkew = kmode->hskew;

                modeInfo.height = kmode->vdisplay;
                modeInfo.vSyncStart = kmode->vsync_start;
                modeInfo.vSyncEnd = kmode->vsync_end;
                modeInfo.vTotal = kmode->vtotal;
                modeInfo.modeFlags = kmode->flags;

                rrmode = RRModeGet(&modeInfo, kmode->name);
                if (rrmode) {
                    rrmodes[*nmode] = rrmode;
                    *nmode = *nmode + 1;
                    *npref = *npref + pref;
                }
            }
        }
    }
    /* workaround: there could be no preferred mode that got added */
    if (*nmode > 0 && *npref == 0)
        *npref = 1;

    drmModeFreeConnector(conn);
    return rrmodes;
#else
    *nmode = *npref = 0;
    return NULL;
#endif
}

static void
lease_connector_handle_connector_id(void *data,
                                    struct wp_drm_lease_connector_v1 *wp_drm_lease_connector_v1,
                                    uint32_t connector_id)
{
    struct xwl_output *output;
    Atom name;
    INT32 value;
    int err;
    int nmode, npref;
    RRModePtr *rrmodes;

    value = connector_id;
    output = (struct xwl_output *)data;
    name = MakeAtom("CONNECTOR_ID", 12, TRUE);

    if (name != BAD_RESOURCE) {
        err = RRConfigureOutputProperty(output->randr_output, name,
                                        FALSE, FALSE, TRUE,
                                        1, &value);
        if (err != 0) {
            ErrorF("RRConfigureOutputProperty error, %d\n", err);
            return;
        }
        err = RRChangeOutputProperty(output->randr_output, name,
                                     XA_INTEGER, 32, PropModeReplace, 1,
                                     &value, FALSE, FALSE);
        if (err != 0) {
            ErrorF("RRChangeOutputProperty error, %d\n", err);
            return;
        }
    }
    rrmodes = xwl_get_rrmodes_from_connector_id(output->lease_device->drm_read_only_fd,
                                                connector_id, &nmode, &npref);

    if (rrmodes != NULL)
        RROutputSetModes(output->randr_output, rrmodes, nmode, npref);

    free(rrmodes);
}

static void
lease_connector_handle_done(void *data,
                            struct wp_drm_lease_connector_v1 *wp_drm_lease_connector_v1)
{
    /* This space is deliberately left blank */
}

static void
lease_connector_handle_withdrawn(void *data,
                                 struct wp_drm_lease_connector_v1 *wp_drm_lease_connector_v1)
{
    struct xwl_output *xwl_output = data;

    xwl_output->withdrawn_connector = TRUE;

    /* Not removing the xwl_output if currently leased with Wayland */
    if (xwl_output->lease)
        return;

    xwl_output_remove(xwl_output);
}

static const struct wp_drm_lease_connector_v1_listener lease_connector_listener = {
    .name = lease_connector_handle_name,
    .description = lease_connector_handle_description,
    .connector_id = lease_connector_handle_connector_id,
    .withdrawn = lease_connector_handle_withdrawn,
    .done = lease_connector_handle_done,
};

static void
drm_lease_device_handle_drm_fd(void *data,
                               struct wp_drm_lease_device_v1 *wp_drm_lease_device_v1,
                               int fd)
{
    ((struct xwl_drm_lease_device *)data)->drm_read_only_fd = fd;
}

static void
drm_lease_device_handle_connector(void *data,
                                  struct wp_drm_lease_device_v1 *wp_drm_lease_device_v1,
                                  struct wp_drm_lease_connector_v1 *connector)
{
    struct xwl_drm_lease_device *lease_device = data;
    struct xwl_screen *xwl_screen = lease_device->xwl_screen;
    struct xwl_output *xwl_output;
    char name[MAX_OUTPUT_NAME] = { 0 };

    xwl_output = calloc(1, sizeof *xwl_output);
    if (xwl_output == NULL) {
        ErrorF("%s ENOMEM\n", __func__);
        return;
    }

    xwl_output->lease_device = lease_device;
    xwl_output->xwl_screen = xwl_screen;
    xwl_output->lease_connector = connector;
    xwl_output->randr_crtc = RRCrtcCreate(xwl_screen->screen, xwl_output);
    if (!xwl_output->randr_crtc) {
        ErrorF("Failed creating RandR CRTC\n");
        goto err;
    }
    RRCrtcSetRotations(xwl_output->randr_crtc, ALL_ROTATIONS);
    xwl_output->randr_output = RROutputCreate(xwl_screen->screen,
                                              name, MAX_OUTPUT_NAME, xwl_output);
    snprintf(name, MAX_OUTPUT_NAME, "XWAYLAND%d",
             xwl_screen_get_next_output_serial(xwl_screen));
    xwl_output_set_name(xwl_output, name);

    if (!xwl_output->randr_output) {
        ErrorF("Failed creating RandR Output\n");
        goto err;
    }

    RRCrtcGammaSetSize(xwl_output->randr_crtc, 256);
    RROutputSetCrtcs(xwl_output->randr_output, &xwl_output->randr_crtc, 1);
    RROutputSetConnection(xwl_output->randr_output, RR_Connected);
    RROutputSetNonDesktop(xwl_output->randr_output, TRUE);
    xwl_output->randr_output->devPrivate = xwl_output;

    wp_drm_lease_connector_v1_add_listener(connector,
                                            &lease_connector_listener,
                                            xwl_output);

    xorg_list_append(&xwl_output->link, &xwl_screen->output_list);
    return;

err:
    if (xwl_output->randr_crtc)
        RRCrtcDestroy(xwl_output->randr_crtc);
    free(xwl_output);
}

static void
drm_lease_device_handle_released(void *data,
                                 struct wp_drm_lease_device_v1 *wp_drm_lease_device_v1)
{
    struct xwl_drm_lease_device *lease_device = data;
    xwl_screen_destroy_drm_lease_device(lease_device->xwl_screen, wp_drm_lease_device_v1);
}

static void
drm_lease_device_handle_done(void *data,
                             struct wp_drm_lease_device_v1 *wp_drm_lease_device_v1)
{
    /* This space is deliberately left blank */
}

static const struct wp_drm_lease_device_v1_listener drm_lease_device_listener = {
    .drm_fd = drm_lease_device_handle_drm_fd,
    .connector = drm_lease_device_handle_connector,
    .released = drm_lease_device_handle_released,
    .done = drm_lease_device_handle_done,
};

void
xwl_screen_add_drm_lease_device(struct xwl_screen *xwl_screen, uint32_t id)
{
    struct wp_drm_lease_device_v1 *lease_device = wl_registry_bind(
        xwl_screen->registry, id, &wp_drm_lease_device_v1_interface, 1);
    struct xwl_drm_lease_device *device_data = malloc(sizeof(struct xwl_drm_lease_device));

    device_data->drm_lease_device = lease_device;
    device_data->xwl_screen = xwl_screen;
    device_data->drm_read_only_fd = -1;
    device_data->id = id;
    xorg_list_add(&device_data->link, &xwl_screen->drm_lease_devices);
    wp_drm_lease_device_v1_add_listener(lease_device,
                                         &drm_lease_device_listener,
                                         device_data);
}

void
xwl_screen_destroy_drm_lease_device(struct xwl_screen *xwl_screen,
                                    struct wp_drm_lease_device_v1 *wp_drm_lease_device_v1)
{
    struct xwl_drm_lease_device *device_data;

    xorg_list_for_each_entry(device_data, &xwl_screen->drm_lease_devices, link) {
        if (device_data->drm_lease_device == wp_drm_lease_device_v1) {
            wp_drm_lease_device_v1_destroy(wp_drm_lease_device_v1);
            xorg_list_del(&device_data->link);
            if (device_data->drm_read_only_fd >= 0)
                close(device_data->drm_read_only_fd);
            free(device_data);
            return;
        }
    }
}

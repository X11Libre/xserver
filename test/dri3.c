/* SPDX-License-Identifier: MIT */

/* Test relies on assert() */
#undef NDEBUG

#include <dix-config.h>

#include <assert.h>

#include "tests-common.h"

#ifdef DRI3
#include "Xext/dri3/dri3_priv.h"

static int
dummy_fd_from_pixmap(ScreenPtr screen, PixmapPtr pixmap,
                     CARD16 *stride, CARD32 *size)
{
    return -1;
}

static PixmapPtr
dummy_pixmap_from_fds(ScreenPtr screen, CARD8 num_fds, const int *fds,
                      CARD16 width, CARD16 height, const CARD32 *strides,
                      const CARD32 *offsets, CARD8 depth, CARD8 bpp,
                      CARD64 modifier)
{
    return NULL;
}

static int
dummy_fds_from_pixmap(ScreenPtr screen, PixmapPtr pixmap, int *fds,
                      uint32_t *strides, uint32_t *offsets,
                      uint64_t *modifier)
{
    return 0;
}

static int
dummy_get_formats(ScreenPtr screen, CARD32 *num_formats, CARD32 **formats)
{
    return 0;
}

static int
dummy_get_modifiers(ScreenPtr screen, uint32_t format,
                    uint32_t *num_modifiers, uint64_t **modifiers)
{
    return 0;
}

static int
dummy_get_drawable_modifiers(DrawablePtr draw, uint32_t format,
                             uint32_t *num_modifiers, uint64_t **modifiers)
{
    return 0;
}

static struct dri3_syncobj *
dummy_import_syncobj(ClientPtr client, ScreenPtr screen, XID id, int fd)
{
    return NULL;
}

/* every version 0..4 and every subset of the seven callbacks */
static void
dri3_minor_version_test(void)
{
    assert(dri3_info_minor_version(NULL) == 0);

    for (uint32_t version = 0; version <= 4; version++) {
        for (unsigned mask = 0; mask < (1 << 7); mask++) {
            dri3_screen_info_rec info = { .version = version };

            if (mask & (1 << 0))
                info.fd_from_pixmap = dummy_fd_from_pixmap;
            if (mask & (1 << 1))
                info.pixmap_from_fds = dummy_pixmap_from_fds;
            if (mask & (1 << 2))
                info.fds_from_pixmap = dummy_fds_from_pixmap;
            if (mask & (1 << 3))
                info.get_formats = dummy_get_formats;
            if (mask & (1 << 4))
                info.get_modifiers = dummy_get_modifiers;
            if (mask & (1 << 5))
                info.get_drawable_modifiers = dummy_get_drawable_modifiers;
            if (mask & (1 << 6))
                info.import_syncobj = dummy_import_syncobj;

            /* a level needs its own version and callbacks and all lower levels */
            Bool one = version >= 1 && (mask & (1 << 0));
            Bool two = one && version >= 2 &&
                (mask & 0x3e) == 0x3e;  /* the five 1.2 callbacks */
            Bool four = two && version >= 4 && (mask & (1 << 6));
            /* 1.3 needs nothing from the driver beyond 1.2 */
            CARD32 expected = four ? 4 : two ? 3 : one ? 1 : 0;

            assert(dri3_info_minor_version(&info) == expected);
        }
    }
}
#endif /* DRI3 */

const testfunc_t*
dri3_test(void)
{
    static const testfunc_t testfuncs[] = {
#ifdef DRI3
        dri3_minor_version_test,
#endif
        NULL,
    };
    return testfuncs;
}

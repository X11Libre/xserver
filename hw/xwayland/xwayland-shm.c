/*
 * Copyright © 2014 Intel Corporation
 * Copyright © 2012 Collabora, Ltd.
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

#include "os.h"

#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

#include "mi/mi_priv.h"
#include "os/osdep.h"

#include "fb.h"
#include "pixmapstr.h"

#include "xwayland-pixmap.h"
#include "xwayland-screen.h"
#include "xwayland-shm.h"

struct xwl_pixmap {
    struct wl_buffer *buffer;
    void *data;
    size_t size;
};

#ifndef HAVE_MKOSTEMP
static int
set_cloexec_or_close(int fd)
{
    long flags;

    if (fd == -1)
        return -1;

    flags = fcntl(fd, F_GETFD);
    if (flags == -1)
        goto err;

    if (fcntl(fd, F_SETFD, flags | FD_CLOEXEC) == -1)
        goto err;

    return fd;

 err:
    close(fd);
    return -1;
}
#endif

static int
create_tmpfile_cloexec(char *tmpname)
{
    int fd;

#ifdef HAVE_MKOSTEMP
    fd = mkostemp(tmpname, O_CLOEXEC);
    if (fd >= 0)
        unlink(tmpname);
#else
    fd = mkstemp(tmpname);
    if (fd >= 0) {
        fd = set_cloexec_or_close(fd);
        unlink(tmpname);
    }
#endif

    return os_move_fd(fd);
}

/*
 * Create a new, unique, anonymous file of the given size, and
 * return the file descriptor for it. The file descriptor is set
 * CLOEXEC. The file is immediately suitable for mmap()'ing
 * the given size at offset zero.
 *
 * The file should not have a permanent backing store like a disk,
 * but may have if XDG_RUNTIME_DIR is not properly implemented in OS.
 *
 * The file name is deleted from the file system.
 *
 * The file is suitable for buffer sharing between processes by
 * transmitting the file descriptor over Unix sockets using the
 * SCM_RIGHTS methods.
 *
 * If the C library implements posix_fallocate(), it is used to
 * guarantee that disk space is available for the file at the
 * given size. If disk space is insufficient, errno is set to ENOSPC.
 * If posix_fallocate() is not supported, program may receive
 * SIGBUS on accessing mmap()'ed file contents instead.
 *
 * If the C library implements memfd_create(), it is used to create the
 * file purely in memory, without any backing file name on the file
 * system, and then sealing off the possibility of shrinking it.  This
 * can then be checked before accessing mmap()'ed file contents, to
 * make sure SIGBUS can't happen.  It also avoids requiring
 * XDG_RUNTIME_DIR.
 */
static int
os_create_anonymous_file(off_t size)
{
    static const char template[] = "/xwayland-shared-XXXXXX";
    const char *path;
    char *name;
    int fd;
    int ret;

#ifdef HAVE_MEMFD_CREATE
    fd = memfd_create("xwayland-shared", MFD_CLOEXEC | MFD_ALLOW_SEALING);
    if (fd >= 0) {
        /* We can add this seal before calling posix_fallocate(), as
         * the file is currently zero-sized anyway.
         *
         * There is also no need to check for the return value, we
         * couldn't do anything with it anyway.
         */
        fcntl(fd, F_ADD_SEALS, F_SEAL_SHRINK);
    } else
#endif
    {
        path = getenv("XDG_RUNTIME_DIR");
        if (!path) {
            errno = ENOENT;
            return -1;
        }

        name = malloc(strlen(path) + sizeof(template));
        if (!name)
            return -1;

        strcpy(name, path);
        strcat(name, template);

        fd = create_tmpfile_cloexec(name);

        free(name);

        if (fd < 0)
            return -1;
    }

#ifdef HAVE_POSIX_FALLOCATE
    /*
     * posix_fallocate does an explicit rollback if it gets EINTR.
     * Temporarily block signals to allow the call to succeed on
     * slow systems where the smart scheduler's SIGALRM prevents
     * large allocation attempts from ever succeeding.
     */
    OsBlockSignals();
    do {
        ret = posix_fallocate(fd, 0, size);
    } while (ret == EINTR);
    OsReleaseSignals();

    if (ret != 0) {
        close(fd);
        errno = ret;
        return -1;
    }
#else
    do {
        ret = ftruncate(fd, size);
    } while (ret == -1 && errno == EINTR);

    if (ret < 0) {
        close(fd);
        return -1;
    }
#endif

    return fd;
}

static uint32_t
shm_format_for_depth(int depth)
{
    switch (depth) {
    case 32:
        return WL_SHM_FORMAT_ARGB8888;
    case 24:
    default:
        return WL_SHM_FORMAT_XRGB8888;
#ifdef WL_SHM_FORMAT_RGB565
    case 16:
        /* XXX: Check run-time protocol version too */
        return WL_SHM_FORMAT_RGB565;
#endif
    }
}

static Bool
dimensions_match_toplevel_window(ScreenPtr screen, int width, int height)
{
    struct xwl_screen *xwl_screen = xwl_screen_get(screen);
    WindowPtr toplevel;

    if (xwl_screen->rootless)
        toplevel = screen->root->firstChild;
    else
        toplevel = screen->root;

    while (toplevel) {
        if (width == toplevel->drawable.width &&
            height == toplevel->drawable.height)
            return TRUE;

        toplevel = toplevel->nextSib;
    }

    return FALSE;
}

static const struct wl_buffer_listener xwl_shm_buffer_listener = {
    xwl_pixmap_buffer_release_cb,
};

PixmapPtr
xwl_shm_create_pixmap(ScreenPtr screen,
                      int width, int height, int depth, unsigned int hint)
{
    struct xwl_screen *xwl_screen = xwl_screen_get(screen);
    struct xwl_pixmap *xwl_pixmap;
    struct wl_shm_pool *pool;
    PixmapPtr pixmap;
    size_t size, stride;
    uint32_t format;
    int fd;

    if (hint == CREATE_PIXMAP_USAGE_GLYPH_PICTURE ||
        (width == 0 && height == 0) || depth < 15 ||
        (hint != CREATE_PIXMAP_USAGE_BACKING_PIXMAP &&
         !dimensions_match_toplevel_window(screen, width, height)))
        return fbCreatePixmap(screen, width, height, depth, hint);

    stride = PixmapBytePad(width, depth);
    size = stride * height;
    /* Size in the protocol is an integer, make sure we don't exceed
     * INT32_MAX or else the Wayland compositor will raise an error and
     * kill the Wayland connection!
     */
    if (size > INT32_MAX)
        return NULL;

    pixmap = fbCreatePixmap(screen, 0, 0, depth, hint);
    if (!pixmap)
        return NULL;

    xwl_pixmap = calloc(1, sizeof(*xwl_pixmap));
    if (xwl_pixmap == NULL)
        goto err_destroy_pixmap;

    xwl_pixmap->buffer = NULL;
    xwl_pixmap->size = size;
    fd = os_create_anonymous_file(size);
    if (fd < 0)
        goto err_free_xwl_pixmap;

    xwl_pixmap->data = mmap(NULL, size, PROT_READ | PROT_WRITE,
                                  MAP_SHARED, fd, 0);
    if (xwl_pixmap->data == MAP_FAILED)
        goto err_close_fd;

    if (!(*screen->ModifyPixmapHeader) (pixmap, width, height, depth,
                                        BitsPerPixel(depth),
                                        stride, xwl_pixmap->data))
        goto err_munmap;

    format = shm_format_for_depth(pixmap->drawable.depth);
    pool = wl_shm_create_pool(xwl_screen->shm, fd, xwl_pixmap->size);
    xwl_pixmap->buffer = wl_shm_pool_create_buffer(pool, 0,
                                                   pixmap->drawable.width,
                                                   pixmap->drawable.height,
                                                   pixmap->devKind, format);
    wl_shm_pool_destroy(pool);
    close(fd);

    wl_buffer_add_listener(xwl_pixmap->buffer,
                           &xwl_shm_buffer_listener, pixmap);

    xwl_pixmap_set_private(pixmap, xwl_pixmap);

    return pixmap;

 err_munmap:
    munmap(xwl_pixmap->data, size);
 err_close_fd:
    close(fd);
 err_free_xwl_pixmap:
    free(xwl_pixmap);
 err_destroy_pixmap:
    fbDestroyPixmap(pixmap);

    return NULL;
}

Bool
xwl_shm_destroy_pixmap(PixmapPtr pixmap)
{
    struct xwl_pixmap *xwl_pixmap = xwl_pixmap_get(pixmap);

    if (xwl_pixmap && pixmap->refcnt == 1) {
        xwl_pixmap_del_buffer_release_cb(pixmap);
        if (xwl_pixmap->buffer)
            wl_buffer_destroy(xwl_pixmap->buffer);
        munmap(xwl_pixmap->data, xwl_pixmap->size);
        free(xwl_pixmap);
    }

    return fbDestroyPixmap(pixmap);
}

struct wl_buffer *
xwl_shm_pixmap_get_wl_buffer(PixmapPtr pixmap)
{
    struct xwl_pixmap *xwl_pixmap = xwl_pixmap_get(pixmap);

    if (!xwl_pixmap)
        return NULL;

    return xwl_pixmap->buffer;
}

Bool
xwl_shm_create_screen_resources(ScreenPtr screen)
{
    struct xwl_screen *xwl_screen = xwl_screen_get(screen);

    if (!miCreateScreenResources(screen))
        return FALSE;

    if (xwl_screen->rootless)
        screen->devPrivate =
            fbCreatePixmap(screen, 0, 0, screen->rootDepth, 0);
    else
        screen->devPrivate =
            xwl_shm_create_pixmap(screen, screen->width, screen->height,
                                  screen->rootDepth,
                                  CREATE_PIXMAP_USAGE_BACKING_PIXMAP);

    SetRootClip(screen, xwl_screen->root_clip_mode);

    return screen->devPrivate != NULL;
}

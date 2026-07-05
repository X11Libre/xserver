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

#include <sys/ioctl.h>
#include <errno.h>

#include "fb/fb_priv.h"
#include "os/osdep.h"

#include "fbdev.h"

#ifdef XV
#include "kxv.h"
#endif

static Bool
fbdevInitialize(KdCardInfo * card, FbdevPriv * priv)
{
    unsigned long off;
    FbScreenConf *config = card->closure;

    if (config->fbdevDevicePath) {
        priv->fd = open(config->fbdevDevicePath, O_RDWR);
        if (priv->fd < 0) {
            ErrorF("Error opening framebuffer %s: %s\n",
                   config->fbdevDevicePath, strerror(errno));
            return FALSE;
        }
        LogMessage(X_INFO, "Xfbdev(%d): Using framebuffer device: %s\n",
                   card->mynum, config->fbdevDevicePath);
    } else {
        char devbuf[] = "/dev/fbxx";
        memcpy(devbuf, "/dev/fb", sizeof("/dev/fb"));
        priv->fd = open("/dev/fb", O_RDWR);
        for (int i = 0; i < 32 && (priv->fd < 0); i++) {
            snprintf(devbuf, sizeof(devbuf),
                     "/dev/fb%d", i);
            priv->fd = open(devbuf, O_RDWR);

            if (priv->fd >= 0) {
                struct fb_fix_screeninfo fix;
                memset(&fix, 0, sizeof(fix));
                if (ioctl(priv->fd, FBIOGET_FSCREENINFO, &fix) < 0) {
                    close(priv->fd);
                    priv->fd = -1;
                }
            }
        }
        if (priv->fd < 0) {
            ErrorF("Error opening framebuffers /dev/fb[0-31]\n");
            return FALSE;
        }
        LogMessage(X_INFO, "Xfbdev(%d): Using framebuffer device: %s\n", card->mynum, devbuf);
    }

    /* quiet valgrind */
    memset(&priv->fix, '\0', sizeof(priv->fix));
    if (ioctl(priv->fd, FBIOGET_FSCREENINFO, &priv->fix) < 0) {
        LogMessage(X_ERROR, "Xfbdev(%d): FBIOGET_FSCREENINFO: %s\n",
                   card->mynum, strerror(errno));
        close(priv->fd);
        return FALSE;
    }

    LogMessage(X_INFO, "Xfbdev(%d): Framebuffer device id: %s\n", card->mynum, priv->fix.id);

    /* quiet valgrind */
    memset(&priv->var, '\0', sizeof(priv->var));
    if (ioctl(priv->fd, FBIOGET_VSCREENINFO, &priv->var) < 0) {
        LogMessage(X_ERROR, "Xfbdev(%d): FBIOPUT_VSCREENINFO: %s\n",
                   card->mynum, strerror(errno));
        close(priv->fd);
        return FALSE;
    }

    priv->fb_base = (char *) mmap((caddr_t) NULL,
                                  priv->fix.smem_len,
                                  PROT_READ | PROT_WRITE,
                                  MAP_SHARED, priv->fd, 0);

    if (priv->fb_base == (char *) -1) {
        LogMessage(X_ERROR, "Xfbdev(%d): Could not mmap the framebuffer: %s\n",
                   card->mynum, strerror(errno));
        close(priv->fd);
        return FALSE;
    }
    off = (unsigned long) priv->fix.smem_start % (unsigned long) getpagesize();
    priv->fb = priv->fb_base + off;
    return TRUE;
}

Bool
fbdevCardInit(KdCardInfo * card)
{
    FbdevPriv *priv;

    priv = (FbdevPriv *) malloc(sizeof(FbdevPriv));
    if (!priv)
        return FALSE;

    if (!fbdevInitialize(card, priv)) {
        free(priv);
        return FALSE;
    }
    card->driver = priv;

    return TRUE;
}

static Pixel
fbdevMakeContig(Pixel orig, Pixel others)
{
    Pixel low;

    low = lowbit(orig) >> 1;
    while (low && (others & low) == 0) {
        orig |= low;
        low >>= 1;
    }
    return orig;
}

static Bool
fbdevModeSupported(KdScreenInfo * screen, const KdMonitorTiming * t)
{
    return TRUE;
}

static int
fbdevGetRefreshRate(const struct fb_var_screeninfo *var)
{
#define PICOS2HZ(a) (1.0e12/(a))
    long scanline = var->left_margin + var->xres + var->right_margin + var->hsync_len;
    long v_total = var->upper_margin + var->yres + var->lower_margin + var->vsync_len;
    long vblank = v_total * scanline * var->pixclock;

    long rate = vblank ? PICOS2HZ(vblank) : -1;

    /* Make sure the rate is reasonable */
    if (rate > 0 && rate <= 1000) {
        return rate;
    }

    /**
     * We could probe the refresh rate by doing FBIO_WAITFORVSYNC,
     * measuring the time between them, and matching that to a list of common rates.
     * However, if the rate reported by the driver is wrong, the driver probably doesn't care about
     * refresh rates.
     */
    return -1;
#undef PICOS2HZ
}

static void
fbdevConvertMonitorTiming(const KdMonitorTiming * t,
                          struct fb_var_screeninfo *var)
{
    memset(var, 0, sizeof(struct fb_var_screeninfo));

    var->xres = t->horizontal;
    var->yres = t->vertical;
    var->xres_virtual = t->horizontal;
    var->yres_virtual = t->vertical;
    var->xoffset = 0;
    var->yoffset = 0;
    var->pixclock = t->clock ? 1000000000 / t->clock : 0;
    var->left_margin = t->hbp;
    var->right_margin = t->hfp;
    var->upper_margin = t->vbp;
    var->lower_margin = t->vfp;
    var->hsync_len = t->hblank - t->hfp - t->hbp;
    var->vsync_len = t->vblank - t->vfp - t->vbp;

    var->sync = 0;
    var->vmode = 0;

    if (t->hpol == KdSyncPositive)
        var->sync |= FB_SYNC_HOR_HIGH_ACT;
    if (t->vpol == KdSyncPositive)
        var->sync |= FB_SYNC_VERT_HIGH_ACT;
}

static Bool
fbdevSetMode(KdScreenInfo *screen, const KdMonitorTiming *t)
{
    FbdevPriv *priv = screen->card->driver;
    struct fb_var_screeninfo var = {0};
    int depth;
    int k;

    k = ioctl(priv->fd, FBIOGET_VSCREENINFO, &var);

    screen->rate = t->rate;
    screen->width = t->horizontal;
    screen->height = t->vertical;

    if (k < 0 || (t->horizontal != var.xres) || (t->vertical != var.yres)) {
        fbdevConvertMonitorTiming(t, &var);
    }

    var.activate = FB_ACTIVATE_NOW;
    var.bits_per_pixel = screen->fb.depth;
    var.nonstd = 0;
    var.grayscale = 0;

    k = ioctl(priv->fd, FBIOPUT_VSCREENINFO, &var);
    if (k < 0) {
        LogMessage(X_ERROR, "Xfbdev(%d): FBIOPUT_VSCREENINFO: %s\n",
                   screen->card->mynum, strerror(errno));
    }

    /* Re-get the "fixed" parameters since they might have changed */
    k = ioctl(priv->fd, FBIOGET_FSCREENINFO, &priv->fix);
    if (k < 0) {
        LogMessage(X_ERROR, "Xfbdev(%d): FBIOGET_FSCREENINFO: %s\n",
                   screen->card->mynum, strerror(errno));
    }

    /* Now get the new screeninfo */
    k = ioctl(priv->fd, FBIOGET_VSCREENINFO, &priv->var);
    if (k >= 0) {
        /* Just because the ioctl didn't fail, it doesn't mean we could set the mode */
        LogMessage(X_INFO, "Xfbdev(%d): Current screen mode: width = %d, height = %d\n",
                   screen->card->mynum, priv->var.xres, priv->var.yres);
    }

    depth = priv->var.bits_per_pixel;

    /* Calculate fix.line_length if it's zero */
    if (!priv->fix.line_length)
        priv->fix.line_length = (priv->var.xres_virtual * depth + 7) / 8;

    return (k >= 0) && (t->horizontal == priv->var.xres) && (t->vertical == var.yres);
}

static void
fbdevConvertVarToTiming(const struct fb_var_screeninfo *var,
                        KdMonitorTiming * t)
{

    t->horizontal = var->xres;
    t->vertical = var->yres;
    t->clock = var->pixclock ? 1000000000 / var->pixclock : 0;
    t->hbp = var->left_margin;
    t->hfp = var->right_margin;
    t->vbp = var->upper_margin;
    t->vfp = var->lower_margin;
    t->hblank = var->hsync_len + t->hfp + t->hbp;
    t->vblank = var->vsync_len + t->vfp + t->vbp;

    t->rate = fbdevGetRefreshRate(var);

    t->hpol = (var->sync & FB_SYNC_HOR_HIGH_ACT) ? KdSyncPositive : KdSyncNegative;
    t->vpol = (var->sync & FB_SYNC_VERT_HIGH_ACT) ? KdSyncPositive : KdSyncNegative;
}

static Bool
fbdevScreenInitialize(KdScreenInfo * screen, FbdevScrPriv * scrpriv)
{
    FbdevPriv *priv = screen->card->driver;
    Pixel allbits;
    int depth;
    int rate;
    Bool want_rate = FALSE;
    Bool gray;
    struct fb_var_screeninfo var;
    const KdMonitorTiming *t;
    int k;

#define FB_DEFAULT_RATE 103 /* The highest rate in the modelist from kmode.c */

    k = ioctl(priv->fd, FBIOGET_VSCREENINFO, &var);

    if (!screen->width || !screen->height) {
        if (k >= 0) {
            screen->width = var.xres;
            screen->height = var.yres;
        } else {
            screen->width = 1024;
            screen->height = 768;
        }
    }
    if (!screen->rate) {
        screen->rate = (k >= 0) ? fbdevGetRefreshRate(&var) : FB_DEFAULT_RATE;
        if (screen->rate <= 0) {
            screen->rate = FB_DEFAULT_RATE;
        }
    } else {
        want_rate = TRUE;
    }
    if (!screen->fb.depth) {
        if (k >= 0)
            screen->fb.depth = var.bits_per_pixel;
        else
            screen->fb.depth = 16;
    }

    scrpriv->max_width = 0;
    scrpriv->max_height = 0;

    if (k >= 0) {
        KdMonitorTiming curr_mode = {0};

        int saved_width = screen->width;
        int saved_height = screen->height;

        scrpriv->max_width = var.xres;
        scrpriv->max_height = var.yres;

        /* See if the current size is known */
        screen->width = var.xres;
        screen->height = var.yres;
        rate = KdFindRate(screen, fbdevModeSupported);
        screen->width = saved_width;
        screen->height = saved_height;

        /* Add the current framebuffer mode */
        fbdevConvertVarToTiming(&var, &curr_mode);
        if (curr_mode.rate > 0) {
            KdAddMode(&curr_mode);
        } else if (!rate) {
            KdAddModeCVT(var.xres, var.yres, screen->rate);
        }
    }

    rate = KdFindRate(screen, fbdevModeSupported);
    if (!rate || want_rate || (k < 0) || (screen->width != var.xres) || (screen->height != var.yres)) {
        /* Add the desired framebuffer mode */
        KdAddModeCVT(screen->width, screen->height, screen->rate);
    }

    /* Fbdev rate isn't reliable, don't forbid modes based on it */
    if (!want_rate && (screen->rate < rate)) {
        screen->rate = rate;
    }

    t = KdFindMode(screen, fbdevModeSupported);

    /**
     * XXX The only way we can check what modes are supported is by actually setting them.
     *
     * We save the video card mode, probe the mode by setting it, and restore the video card mode.
     * The probed video move will be set by fbdevEnable.
     */

    /* KdTuneMode calls fbdevSetMode, which sets priv->fix, priv->var */
    fbdevPreserve(screen->card);
    KdTuneMode(screen, t, fbdevSetMode, fbdevModeSupported);
    fbdevRestore(screen->card);

    if (scrpriv->max_width < screen->width) {
        scrpriv->max_width = screen->width;
    }

    if (scrpriv->max_height < screen->height) {
        scrpriv->max_height = screen->height;
    }

    depth = priv->var.bits_per_pixel;
    gray = priv->var.grayscale;

    switch (priv->fix.visual) {
    case FB_VISUAL_MONO01:
    case FB_VISUAL_MONO10:
        screen->fb.visuals = (1 << StaticGray);
        break;
    case FB_VISUAL_PSEUDOCOLOR:
        screen->fb.visuals = (1 << StaticGray);
        if (priv->var.bits_per_pixel == 1) {
            /* Override to monochrome, to have preallocated black/white */
            priv->fix.visual = FB_VISUAL_MONO01;
        } else if (gray) {
            /* could also support GrayScale, but what's the point? */
        } else {
            screen->fb.visuals = ((1 << StaticGray) |
                                  (1 << GrayScale) |
                                  (1 << StaticColor) |
                                  (1 << PseudoColor) |
                                  (1 << TrueColor) | (1 << DirectColor));
        }
        screen->fb.blueMask = 0x00;
        screen->fb.greenMask = 0x00;
        screen->fb.redMask = 0x00;
        break;
    case FB_VISUAL_STATIC_PSEUDOCOLOR:
        if (gray) {
            screen->fb.visuals = (1 << StaticGray);
        }
        else {
            screen->fb.visuals = (1 << StaticColor);
        }
        screen->fb.blueMask = 0x00;
        screen->fb.greenMask = 0x00;
        screen->fb.redMask = 0x00;
        break;
    case FB_VISUAL_TRUECOLOR:
    case FB_VISUAL_DIRECTCOLOR:
        screen->fb.visuals = (1 << TrueColor);
#define Mask(o,l)   (((1 << l) - 1) << o)
        screen->fb.redMask = Mask (priv->var.red.offset, priv->var.red.length);
        screen->fb.greenMask =
            Mask (priv->var.green.offset, priv->var.green.length);
        screen->fb.blueMask =
            Mask (priv->var.blue.offset, priv->var.blue.length);

        /*
         * This is a kludge so that Render will work -- fill in the gaps
         * in the pixel
         */
        screen->fb.redMask = fbdevMakeContig(screen->fb.redMask,
                                             screen->fb.greenMask |
                                             screen->fb.blueMask);

        screen->fb.greenMask = fbdevMakeContig(screen->fb.greenMask,
                                               screen->fb.redMask |
                                               screen->fb.blueMask);

        screen->fb.blueMask = fbdevMakeContig(screen->fb.blueMask,
                                              screen->fb.redMask |
                                              screen->fb.greenMask);

        allbits =
            screen->fb.redMask | screen->fb.greenMask | screen->fb.blueMask;
        depth = 32;
        while (depth && !(allbits & (1 << (depth - 1))))
            depth--;
        break;
    default:
        return FALSE;
        break;
    }
    screen->fb.depth = depth;
    screen->fb.bitsPerPixel = priv->var.bits_per_pixel;

    scrpriv->randr = screen->randr;

    return fbdevMapFramebuffer(screen);
}

Bool
fbdevScreenInit(KdScreenInfo * screen)
{
    FbdevScrPriv *scrpriv;

    scrpriv = calloc(1, sizeof(FbdevScrPriv));
    if (!scrpriv)
        return FALSE;
    screen->driver = scrpriv;
    if (!fbdevScreenInitialize(screen, scrpriv)) {
        screen->driver = 0;
        free(scrpriv);
        return FALSE;
    }
    return TRUE;
}

static void *
fbdevWindowLinear(ScreenPtr pScreen,
                  CARD32 row,
                  CARD32 offset, int mode, CARD32 *size, void *closure)
{
    KdScreenPriv(pScreen);
    FbdevPriv *priv = pScreenPriv->card->driver;

    if (!pScreenPriv->enabled) {
        *size = 0;
        return NULL;
    }
    *size = priv->fix.line_length;
    return (CARD8 *) priv->fb + row * priv->fix.line_length + offset;
}

static void *
fbdevWindowAfb(ScreenPtr pScreen,
               CARD32 row,
               CARD32 offset, int mode, CARD32 *size, void *closure)
{
    KdScreenPriv(pScreen);
    FbdevPriv *priv = pScreenPriv->card->driver;

    if (!pScreenPriv->enabled) {
        *size = 0;
        return NULL;
    }
    /* offset to next plane */
    *size = priv->var.yres_virtual * priv->fix.line_length;
    return (CARD8 *) priv->fb + row * priv->fix.line_length + offset;
}

Bool
fbdevMapFramebuffer(KdScreenInfo * screen)
{
    FbdevScrPriv *scrpriv = screen->driver;
    KdPointerMatrix m;
    FbdevPriv *priv = screen->card->driver;
    FbScreenConf *config = screen->card->closure;

    if (!config->fbDisableShadow) {
        scrpriv->shadow = TRUE;
    } else if (scrpriv->randr != RR_Rotate_0 ||
        priv->fix.type != FB_TYPE_PACKED_PIXELS) {
        scrpriv->shadow = TRUE;
    } else {
        scrpriv->shadow = FALSE;
    }

    KdComputePointerMatrix(&m, scrpriv->randr, screen->width, screen->height);

    KdSetPointerMatrix(&m);

    screen->width = priv->var.xres;
    screen->height = priv->var.yres;

    if (scrpriv->shadow) {
        if (!KdShadowFbAlloc(screen,
                             scrpriv->randr & (RR_Rotate_90 | RR_Rotate_270)))
            return FALSE;
    }
    else {
        screen->fb.byteStride = priv->fix.line_length;
        screen->fb.pixelStride = (priv->fix.line_length * 8 /
                                  priv->var.bits_per_pixel);
        screen->fb.frameBuffer = (CARD8 *) (priv->fb);
    }

    return TRUE;
}

static void
fbdevSetScreenSizes(ScreenPtr pScreen)
{
    KdScreenPriv(pScreen);
    KdScreenInfo *screen = pScreenPriv->screen;
    FbdevScrPriv *scrpriv = screen->driver;
    FbdevPriv *priv = screen->card->driver;

    if (scrpriv->randr & (RR_Rotate_0 | RR_Rotate_180)) {
        pScreen->width = priv->var.xres;
        pScreen->height = priv->var.yres;
        pScreen->mmWidth = screen->width_mm;
        pScreen->mmHeight = screen->height_mm;
    }
    else {
        pScreen->width = priv->var.yres;
        pScreen->height = priv->var.xres;
        pScreen->mmWidth = screen->height_mm;
        pScreen->mmHeight = screen->width_mm;
    }
}

static void
fbdevClearFramebuffer(KdScreenInfo * screen)
{
#if 0 /* XXX Does not work reliably XXX */
    FbdevPriv *priv = screen->card->driver;
    memset(priv->fb_base, 0, priv->fix.smem_len);
    volatile char *clear_me = (volatile char*)priv->fb_base;
    for (int i = 0; i < priv->fix.smem_len; i++, clear_me[i] = 0);
#else
    kdOsFuncs->Disable();
    kdOsFuncs->Enable();
#endif
}

static Bool
fbdevUnmapFramebuffer(KdScreenInfo * screen)
{
    KdShadowFbFree(screen);
    return TRUE;
}

static Bool
fbdevSetShadow(ScreenPtr pScreen)
{
    KdScreenPriv(pScreen);
    KdScreenInfo *screen = pScreenPriv->screen;
    FbdevScrPriv *scrpriv = screen->driver;
    FbdevPriv *priv = screen->card->driver;
    ShadowUpdateProc update;
    ShadowWindowProc window;
    int useYX = 0;

#ifdef __arm__
    /* Use variant copy routines that always read left to right in the
       shadow framebuffer.  Reading vertical strips is exceptionally
       slow on XScale due to cache effects.  */
    useYX = 1;
#endif

    window = fbdevWindowLinear;
    update = 0;
    switch (priv->fix.type) {
    case FB_TYPE_PACKED_PIXELS:
        if (scrpriv->randr)
            if (priv->var.bits_per_pixel == 16) {
                switch (scrpriv->randr) {
                case RR_Rotate_90:
                    if (useYX)
                        update = shadowUpdateRotate16_90YX;
                    else
                        update = shadowUpdateRotate16_90;
                    break;
                case RR_Rotate_180:
                    update = shadowUpdateRotate16_180;
                    break;
                case RR_Rotate_270:
                    if (useYX)
                        update = shadowUpdateRotate16_270YX;
                    else
                        update = shadowUpdateRotate16_270;
                    break;
                default:
                    update = shadowUpdateRotate16;
                    break;
                }
            }
            else
                update = shadowUpdateRotatePacked;
        else
            update = shadowUpdatePacked;
        break;

    case FB_TYPE_PLANES:
        window = fbdevWindowAfb;
        switch (priv->var.bits_per_pixel) {
        case 4:
            update = shadowUpdateAfb4;
            break;

        case 8:
            update = shadowUpdateAfb8;
            break;

        default:
            FatalError("Bitplanes with bpp %u are not yet supported\n",
                       priv->var.bits_per_pixel);
        }
        break;

    case FB_TYPE_INTERLEAVED_PLANES:
        if (priv->fix.type_aux == 2) {
            switch (priv->var.bits_per_pixel) {
            case 4:
                update = shadowUpdateIplan2p4;
                break;

            case 8:
                update = shadowUpdateIplan2p8;
                break;

            default:
                FatalError("Atari interleaved bitplanes with bpp %u are not yet supported\n",
                           priv->var.bits_per_pixel);
            }
        } else {
            FatalError("Interleaved bitplanes with interleave %u are not yet supported\n",
                       priv->fix.type_aux);
        }
        break;

    case FB_TYPE_TEXT:
        FatalError("Text frame buffers are not yet supported\n");
        break;

    case FB_TYPE_VGA_PLANES:
        FatalError("VGA planes are not yet supported\n");
        break;

    default:
        FatalError("Unsupported frame buffer type %u\n", priv->fix.type);
        break;
    }

    return KdShadowSet(pScreen, scrpriv->randr, update, window);
}

#ifdef RANDR
static Bool
fbdevRandrModeSupported(ScreenPtr pScreen, const KdMonitorTiming *t)
{
    KdScreenPriv(pScreen);
    KdScreenInfo *screen = pScreenPriv->screen;
    FbdevScrPriv *scrpriv = screen->driver;

    return (t->horizontal <= scrpriv->max_width) && (t->vertical <= scrpriv->max_height);
}

static Bool
fbdevRandrModeChangeSupported(ScreenPtr pScreen, const KdMonitorTiming *t)
{
    return TRUE;
}

static Bool
fbdevRandRGetInfo(ScreenPtr pScreen, Rotation * rotations)
{
    KdScreenPriv(pScreen);
    KdScreenInfo *screen = pScreenPriv->screen;
    FbdevScrPriv *scrpriv = screen->driver;
    Rotation randr;
    int n;

    *rotations = RR_Rotate_All | RR_Reflect_All;

    for (n = 0; n < pScreen->numDepths; n++)
        if (pScreen->allowedDepths[n].numVids)
            break;
    if (n == pScreen->numDepths)
        return FALSE;

    randr = KdSubRotation(scrpriv->randr, screen->randr);

    return KdRandRGetInfo(pScreen, randr, fbdevRandrModeSupported);
}

static Bool
fbdevRandRSetConfig(ScreenPtr pScreen,
                    Rotation randr, int rate, RRScreenSizePtr pSize)
{
    KdScreenPriv(pScreen);
    KdScreenInfo *screen = pScreenPriv->screen;
    FbdevScrPriv *scrpriv = screen->driver;
    Bool wasEnabled = pScreenPriv->enabled;
    FbdevScrPriv oldscr;
    const KdMonitorTiming *t;
    int oldwidth;
    int oldheight;
    int oldmmwidth;
    int oldmmheight;
    int newwidth, newheight, newmmwidth, newmmheight;

    if (screen->randr & (RR_Rotate_0 | RR_Rotate_180)) {
        newwidth = pSize->width;
        newheight = pSize->height;
        newmmwidth = pSize->mmWidth;
        newmmheight = pSize->mmHeight;
    }
    else {
        newwidth = pSize->height;
        newheight = pSize->width;
        newmmwidth = pSize->mmHeight;
        newmmheight = pSize->mmWidth;
    }

    if (wasEnabled)
        KdDisableScreen(pScreen);

    oldscr = *scrpriv;

    oldwidth = screen->width;
    oldheight = screen->height;
    oldmmwidth = pScreen->mmWidth;
    oldmmheight = pScreen->mmHeight;

    /*
     * Set new configuration
     */

    scrpriv->randr = KdAddRotation(screen->randr, randr);
    pScreen->width = newwidth;
    pScreen->height = newheight;
    pScreen->mmWidth = newmmwidth;
    pScreen->mmHeight = newmmheight;

    fbdevUnmapFramebuffer(screen);

    t = KdRandRGetTiming(pScreen, fbdevRandrModeChangeSupported, rate, pSize);

    if (!t || !fbdevSetMode(screen, t))
        goto bail4;

    if (!fbdevMapFramebuffer(screen))
        goto bail4;

    KdShadowUnset(screen->pScreen);

    if (!fbdevSetShadow(screen->pScreen))
        goto bail4;

    fbdevSetScreenSizes(screen->pScreen);

    /*
     * Set frame buffer mapping
     */
    (*pScreen->ModifyPixmapHeader) (fbGetScreenPixmap(pScreen),
                                    pScreen->width,
                                    pScreen->height,
                                    screen->fb.depth,
                                    screen->fb.bitsPerPixel,
                                    screen->fb.byteStride,
                                    screen->fb.frameBuffer);

    /* set the subpixel order */

    KdSetSubpixelOrder(pScreen, scrpriv->randr);

    if (wasEnabled) {
        KdEnableScreen(pScreen);
        fbdevClearFramebuffer(screen);
    }

    return TRUE;

 bail4:
    fbdevUnmapFramebuffer(screen);
    *scrpriv = oldscr;
    (void) fbdevMapFramebuffer(screen);
    pScreen->width = oldwidth;
    pScreen->height = oldheight;
    pScreen->mmWidth = oldmmwidth;
    pScreen->mmHeight = oldmmheight;

    if (wasEnabled)
        KdEnableScreen(pScreen);
    return FALSE;
}

static Bool
fbdevRandRInit(ScreenPtr pScreen)
{
    rrScrPrivPtr pScrPriv;

    if (!RRScreenInit(pScreen))
        return FALSE;

    pScrPriv = rrGetScrPriv(pScreen);
    pScrPriv->rrGetInfo = fbdevRandRGetInfo;
    pScrPriv->rrSetConfig = fbdevRandRSetConfig;
    return TRUE;
}
#endif

static Bool
fbdevCreateColormap(ColormapPtr pmap)
{
    ScreenPtr pScreen = pmap->pScreen;

    KdScreenPriv(pScreen);
    FbdevPriv *priv = pScreenPriv->card->driver;
    VisualPtr pVisual;
    int i;
    int nent;
    xColorItem *pdefs;

    switch (priv->fix.visual) {
    case FB_VISUAL_MONO01:
        pScreen->whitePixel = 0;
        pScreen->blackPixel = 1;
        pmap->red[0].co.local.red = 65535;
        pmap->red[0].co.local.green = 65535;
        pmap->red[0].co.local.blue = 65535;
        pmap->red[1].co.local.red = 0;
        pmap->red[1].co.local.green = 0;
        pmap->red[1].co.local.blue = 0;
        return TRUE;
    case FB_VISUAL_MONO10:
        pScreen->blackPixel = 0;
        pScreen->whitePixel = 1;
        pmap->red[0].co.local.red = 0;
        pmap->red[0].co.local.green = 0;
        pmap->red[0].co.local.blue = 0;
        pmap->red[1].co.local.red = 65535;
        pmap->red[1].co.local.green = 65535;
        pmap->red[1].co.local.blue = 65535;
        return TRUE;
    case FB_VISUAL_STATIC_PSEUDOCOLOR:
        pVisual = pmap->pVisual;
        nent = pVisual->ColormapEntries;
        pdefs = calloc(nent, sizeof(xColorItem));
        if (!pdefs)
            return FALSE;
        for (i = 0; i < nent; i++)
            pdefs[i].pixel = i;
        fbdevGetColors(pScreen, nent, pdefs);
        for (i = 0; i < nent; i++) {
            pmap->red[i].co.local.red = pdefs[i].red;
            pmap->red[i].co.local.green = pdefs[i].green;
            pmap->red[i].co.local.blue = pdefs[i].blue;
        }
        free(pdefs);
        return TRUE;
    default:
        return fbInitializeColormap(pmap);
    }
}

Bool
fbdevInitScreen(ScreenPtr pScreen)
{
    pScreen->CreateColormap = fbdevCreateColormap;
    return TRUE;
}

Bool
fbdevFinishInitScreen(ScreenPtr pScreen)
{
    if (!shadowSetup(pScreen))
        return FALSE;

#ifdef RANDR
    if (!fbdevRandRInit(pScreen))
        return FALSE;
#endif

    return TRUE;
}

Bool
fbdevCreateResources(ScreenPtr pScreen)
{
    return fbdevSetShadow(pScreen);
}

void
fbdevPreserve(KdCardInfo * card)
{
    FbdevPriv *priv = card->driver;
    memset(&priv->saved_var, 0, sizeof(priv->saved_var));
    if (ioctl(priv->fd, FBIOGET_VSCREENINFO, &priv->saved_var) < 0) {
        LogMessage(X_INFO, "Xfbdev(%d): Failed to save the video card mode: %s\n",
                   card->mynum, strerror(errno));
        memset(&priv->saved_var, 0, sizeof(priv->saved_var));
    }
}

static int
fbdevUpdateFbColormap(FbdevPriv * priv, int minidx, int maxidx)
{
    struct fb_cmap cmap;

    cmap.start = minidx;
    cmap.len = maxidx - minidx + 1;
    cmap.red = &priv->red[minidx];
    cmap.green = &priv->green[minidx];
    cmap.blue = &priv->blue[minidx];
    cmap.transp = 0;

    return ioctl(priv->fd, FBIOPUTCMAP, &cmap);
}

Bool
fbdevEnable(ScreenPtr pScreen)
{
    KdScreenPriv(pScreen);
    FbdevPriv *priv = pScreenPriv->card->driver;

    int k;

    priv->var.activate = FB_ACTIVATE_NOW | FB_CHANGE_CMAP_VBL;

    /* display it on the LCD */
    k = ioctl(priv->fd, FBIOPUT_VSCREENINFO, &priv->var);
    if (k < 0) {
        LogMessage(X_ERROR, "Xfbdev(%d): FBIOPUT_VSCREENINFO: %s\n",
                   pScreen->myNum, strerror(errno));
        return FALSE;
    }

    if (priv->fix.visual == FB_VISUAL_DIRECTCOLOR) {
        int i;

        for (i = 0;
             i < (1 << priv->var.red.length) ||
             i < (1 << priv->var.green.length) ||
             i < (1 << priv->var.blue.length); i++) {
            priv->red[i] = i * 65535 / ((1 << priv->var.red.length) - 1);
            priv->green[i] = i * 65535 / ((1 << priv->var.green.length) - 1);
            priv->blue[i] = i * 65535 / ((1 << priv->var.blue.length) - 1);
        }

        fbdevUpdateFbColormap(priv, 0, i);
    }

#ifdef XV
    KdXVEnable (pScreen);
#endif
    return TRUE;
}

Bool
fbdevDPMS(ScreenPtr pScreen, int mode)
{
    KdScreenPriv(pScreen);
    FbdevPriv *priv = pScreenPriv->card->driver;

#ifdef FBIOPUT_POWERMODE
    if (ioctl(priv->fd, FBIOPUT_POWERMODE, &mode) >= 0) {
        return TRUE;
    }
#endif
#ifdef FBIOBLANK
    if (ioctl(priv->fd, FBIOBLANK, mode ? mode + 1 : 0) >= 0) {
        return TRUE;
    }
#endif
    return FALSE;
}

void
fbdevDisable(ScreenPtr pScreen)
{
#ifdef XV
    KdXVDisable (pScreen);
#endif
}

void
fbdevRestore(KdCardInfo * card)
{
    FbdevPriv *priv = card->driver;
    if (priv->saved_var.xres &&
        (ioctl(priv->fd, FBIOPUT_VSCREENINFO, &priv->saved_var) < 0)) {
        LogMessage(X_INFO, "Xfbdev(%d): Failed to restore the video card mode: %s\n",
                   card->mynum, strerror(errno));
    }
}

void
fbdevScreenFini(KdScreenInfo * screen)
{
}

void
fbdevCardFini(KdCardInfo * card)
{
    FbdevPriv *priv = card->driver;

    munmap(priv->fb_base, priv->fix.smem_len);
    close(priv->fd);
    free(priv);
    card->driver = NULL;

    free(card->closure);
    card->closure = NULL;
}

/*
 * Retrieve actual colormap and return selected n entries in pdefs.
 */
void
fbdevGetColors(ScreenPtr pScreen, int n, xColorItem * pdefs)
{
    KdScreenPriv(pScreen);
    FbdevPriv *priv = pScreenPriv->card->driver;
    struct fb_cmap cmap;
    int p;
    int k;
    int min, max;

    min = 256;
    max = 0;
    for (k = 0; k < n; k++) {
        if (pdefs[k].pixel < min)
            min = pdefs[k].pixel;
        if (pdefs[k].pixel > max)
            max = pdefs[k].pixel;
    }
    cmap.start = min;
    cmap.len = max - min + 1;
    cmap.red = &priv->red[min];
    cmap.green = &priv->green[min];
    cmap.blue = &priv->blue[min];
    cmap.transp = 0;
    k = ioctl(priv->fd, FBIOGETCMAP, &cmap);
    if (k < 0) {
        LogMessage(X_ERROR, "Xfbdev(%d): FBIOGETCMAP: %s\n",
                   pScreen->myNum, strerror(errno));
        return;
    }
    while (n--) {
        p = pdefs->pixel;
        pdefs->red = priv->red[p];
        pdefs->green = priv->green[p];
        pdefs->blue = priv->blue[p];
        pdefs++;
    }
}

/*
 * Change colormap by updating n entries described in pdefs.
 */
void
fbdevPutColors(ScreenPtr pScreen, int n, xColorItem * pdefs)
{
    KdScreenPriv(pScreen);
    FbdevPriv *priv = pScreenPriv->card->driver;
    int p;
    int min, max;

    min = 256;
    max = 0;
    while (n--) {
        p = pdefs->pixel;
        priv->red[p] = pdefs->red;
        priv->green[p] = pdefs->green;
        priv->blue[p] = pdefs->blue;
        if (p < min)
            min = p;
        if (p > max)
            max = p;
        pdefs++;
    }

    fbdevUpdateFbColormap(priv, min, max);
}

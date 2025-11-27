/*
 * Copyright © 2019 Red Hat, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *	Adam Jackson <ajax@redhat.com>
 */

/*
 * Sets up GLX capabilities based on the EGL capabilities of the glamor
 * renderer for the screen. Without this you will get whatever swrast
 * can do, which often does not include things like multisample visuals.
 */

#include <dix-config.h>

#include "dix/dix_priv.h"

#define MESA_EGL_NO_X11_HEADERS
#define EGL_NO_X11
#include <epoxy/egl.h>
#include "glxserver.h"
#include "glxutil.h"
#include "compint.h"
#include <X11/extensions/composite.h>
#include "glamor_priv.h"
#include "glamor.h"

/* Can't get these from <GL/glx.h> since it pulls in client headers */
#define GLX_RGBA_BIT		0x00000001
#define GLX_WINDOW_BIT		0x00000001
#define GLX_PIXMAP_BIT		0x00000002
#define GLX_PBUFFER_BIT		0x00000004
#define GLX_NONE                0x8000
#define GLX_SLOW_CONFIG         0x8001
#define GLX_TRUE_COLOR		0x8002
#define GLX_DIRECT_COLOR	0x8003
#define GLX_NON_CONFORMANT_CONFIG 0x800D
#define GLX_DONT_CARE           0xFFFFFFFF
#define GLX_RGBA_FLOAT_BIT_ARB  0x00000004
#define GLX_SWAP_UNDEFINED_OML  0x8063

struct egl_config {
    __GLXconfig base;
    EGLConfig config;
};

struct egl_screen {
    __GLXscreen base;
    EGLDisplay display;
    EGLConfig *configs;
};

static void
egl_screen_destroy(__GLXscreen *_screen)
{
    struct egl_screen *screen = (struct egl_screen *)_screen;

    /* XXX do we leak the fbconfig list? */

    free(screen->configs);
    __glXScreenDestroy(_screen);
    free(_screen);
}

static void
egl_drawable_destroy(__GLXdrawable *draw)
{
    free(draw);
}

static GLboolean
egl_drawable_swap_buffers(ClientPtr client, __GLXdrawable *draw)
{
    return GL_FALSE;
}

static void
egl_drawable_copy_sub_buffer(__GLXdrawable *draw, int x, int y, int w, int h)
{
}

static void
egl_drawable_wait_x(__GLXdrawable *draw)
{
    glamor_block_handler(draw->pDraw->pScreen);
}

static void
egl_drawable_wait_gl(__GLXdrawable *draw)
{
}

static __GLXdrawable *
egl_create_glx_drawable(ClientPtr client, __GLXscreen *screen,
                        DrawablePtr draw, XID drawid, int type,
                        XID glxdrawid, __GLXconfig *modes)
{
    __GLXdrawable *ret;

    ret = calloc(1, sizeof *ret);
    if (!ret)
        return NULL;

    if (!__glXDrawableInit(ret, screen, draw, type, glxdrawid, modes)) {
        free(ret);
        return NULL;
    }

    ret->destroy = egl_drawable_destroy;
    ret->swapBuffers = egl_drawable_swap_buffers;
    ret->copySubBuffer = egl_drawable_copy_sub_buffer;
    ret->waitX = egl_drawable_wait_x;
    ret->waitGL = egl_drawable_wait_gl;

    return ret;
}

/*
 * TODO:
 *
 * - bindToTextureTargets is suspicious
 * - better channel mask setup
 * - drawable type masks is suspicious
 */
static struct egl_config *
translate_eglconfig(ScreenPtr pScreen, struct egl_screen *screen, EGLConfig hc,
                    struct egl_config *chain, Bool direct_color,
                    Bool double_buffer, Bool duplicate_for_composite,
                    Bool srgb_only)
{
    EGLint value;
    bool valid_depth;
    int i;
    struct egl_config *c = calloc(1, sizeof *c);

    if (!c)
        return chain;

    /* constants.  changing these requires (at least) new EGL extensions */
    c->base.stereoMode = GL_FALSE;
    c->base.numAuxBuffers = 0;
    c->base.level = 0;
    c->base.transparentAlpha = 0;
    c->base.transparentIndex = 0;
    c->base.transparentPixel = GLX_NONE;
    c->base.visualSelectGroup = 0;
    c->base.indexBits = 0;
    c->base.optimalPbufferWidth = 0;
    c->base.optimalPbufferHeight = 0;
    c->base.bindToMipmapTexture = 0;
    c->base.bindToTextureTargets = GLX_DONT_CARE;
    c->base.swapMethod = GLX_SWAP_UNDEFINED_OML;

    /* this is... suspect */
    c->base.drawableType = GLX_WINDOW_BIT | GLX_PIXMAP_BIT | GLX_PBUFFER_BIT;

    /* hmm */
    c->base.bindToTextureRgb = GL_TRUE;
    c->base.bindToTextureRgba = GL_TRUE;

    /*
     * glx conformance failure: there's no such thing as accumulation
     * buffers in EGL.  they should be emulable with shaders and fbos,
     * but i'm pretty sure nobody's using this feature since it's
     * entirely software.  note that glx conformance merely requires
     * that an accum buffer _exist_, not a minimum bitness.
     */
    c->base.accumRedBits = 0;
    c->base.accumGreenBits = 0;
    c->base.accumBlueBits = 0;
    c->base.accumAlphaBits = 0;

    /* parametric state */
    if (direct_color)
        c->base.visualType = GLX_DIRECT_COLOR;
    else
        c->base.visualType = GLX_TRUE_COLOR;

    if (double_buffer)
        c->base.doubleBufferMode = GL_TRUE;
    else
        c->base.doubleBufferMode = GL_FALSE;

    /* direct-mapped state */
#define GET(attr, slot) \
    eglGetConfigAttrib(screen->display, hc, attr, &c->base.slot)
    GET(EGL_RED_SIZE, redBits);
    GET(EGL_GREEN_SIZE, greenBits);
    GET(EGL_BLUE_SIZE, blueBits);
    GET(EGL_ALPHA_SIZE, alphaBits);
    GET(EGL_BUFFER_SIZE, rgbBits);
    GET(EGL_DEPTH_SIZE, depthBits);
    GET(EGL_STENCIL_SIZE, stencilBits);
    GET(EGL_TRANSPARENT_RED_VALUE, transparentRed);
    GET(EGL_TRANSPARENT_GREEN_VALUE, transparentGreen);
    GET(EGL_TRANSPARENT_BLUE_VALUE, transparentBlue);
    GET(EGL_SAMPLE_BUFFERS, sampleBuffers);
    GET(EGL_SAMPLES, samples);
    if (c->base.renderType & GLX_PBUFFER_BIT) {
        GET(EGL_MAX_PBUFFER_WIDTH, maxPbufferWidth);
        GET(EGL_MAX_PBUFFER_HEIGHT, maxPbufferHeight);
        GET(EGL_MAX_PBUFFER_PIXELS, maxPbufferPixels);
    }
#undef GET

    /* Only expose this config if rgbBits matches a supported
     * depth value.
     */
    valid_depth = false;
    for (i = 0; i < pScreen->numDepths && !valid_depth; i++) {
        if (pScreen->allowedDepths[i].depth == c->base.rgbBits)
            valid_depth = true;
    }
    if (!valid_depth) {
        free(c);
        return chain;
    }

    /* derived state: config caveats */
    eglGetConfigAttrib(screen->display, hc, EGL_CONFIG_CAVEAT, &value);
    if (value == EGL_NONE)
        c->base.visualRating = GLX_NONE;
    else if (value == EGL_SLOW_CONFIG)
        c->base.visualRating = GLX_SLOW_CONFIG;
    else if (value == EGL_NON_CONFORMANT_CONFIG)
        c->base.visualRating = GLX_NON_CONFORMANT_CONFIG;
    /* else panic */

    /* derived state: float configs */
    c->base.renderType = GLX_RGBA_BIT;
    if (eglGetConfigAttrib(screen->display, hc, EGL_COLOR_COMPONENT_TYPE_EXT,
                           &value) == EGL_TRUE) {
        if (value == EGL_COLOR_COMPONENT_TYPE_FLOAT_EXT) {
            c->base.renderType = GLX_RGBA_FLOAT_BIT_ARB;
        }
        /* else panic */
    }

    /* derived state: sRGB. EGL doesn't put this in the fbconfig at all,
     * it's a property of the surface specified at creation time, so we have
     * to infer it from the GL's extensions. only makes sense at 8bpc though.
     */
    if (srgb_only) {
        if (c->base.redBits == 8) {
            c->base.sRGBCapable = GL_TRUE;
        } else {
            free(c);
            return chain;
        }
    }

    /* map to the backend's config */
    c->config = hc;

    /*
     * XXX do something less ugly
     */
    if (c->base.renderType == GLX_RGBA_BIT) {
        if (c->base.redBits == 5 &&
            (c->base.rgbBits == 15 || c->base.rgbBits == 16)) {
            c->base.blueMask  = 0x0000001f;
            if (c->base.alphaBits) {
                c->base.greenMask = 0x000003e0;
                c->base.redMask   = 0x00007c00;
                c->base.alphaMask = 0x00008000;
            } else {
                c->base.greenMask = 0x000007e0;
                c->base.redMask   = 0x0000f800;
                c->base.alphaMask = 0x00000000;
            }
        }
        else if (c->base.redBits == 8 &&
            (c->base.rgbBits == 24 || c->base.rgbBits == 32)) {
            c->base.blueMask  = 0x000000ff;
            c->base.greenMask = 0x0000ff00;
            c->base.redMask   = 0x00ff0000;
            if (c->base.alphaBits)
                /* assume all remaining bits are alpha */
                c->base.alphaMask = 0xff000000;
        }
        else if (c->base.redBits == 10 &&
            (c->base.rgbBits == 30 || c->base.rgbBits == 32)) {
            c->base.blueMask  = 0x000003ff;
            c->base.greenMask = 0x000ffc00;
            c->base.redMask   = 0x3ff00000;
            if (c->base.alphaBits)
                /* assume all remaining bits are alpha */
                c->base.alphaMask = 0xc000000;
        }
    }

    /*
     * Here we decide which fbconfigs will be duplicated for compositing.
     * fgbconfigs marked with duplicatedForComp will be reserved for
     * compositing visuals.
     * It might look strange to do this decision this late when translation
     * from an EGLConfig is already done, but using the EGLConfig
     * accessor functions becomes worse both with respect to code complexity
     * and CPU usage.
     */
    if (duplicate_for_composite &&
        (c->base.renderType == GLX_RGBA_FLOAT_BIT_ARB ||
         c->base.rgbBits != 32 ||
         c->base.redBits != 8 ||
         c->base.greenBits != 8 ||
         c->base.blueBits != 8 ||
         c->base.visualRating != GLX_NONE ||
         c->base.sampleBuffers != 0)) {
        free(c);
        return chain;
    }
    c->base.duplicatedForComp = duplicate_for_composite;

    c->base.next = chain ? &chain->base : NULL;
    return c;
}

static __GLXconfig *
egl_mirror_configs(ScreenPtr pScreen, struct egl_screen *screen)
{
    int i, j, k, nconfigs;
    struct egl_config *c = NULL;
    EGLConfig *host_configs = NULL;
    bool can_srgb = epoxy_has_gl_extension("GL_ARB_framebuffer_sRGB") ||
                    epoxy_has_gl_extension("GL_EXT_framebuffer_sRGB") ||
                    epoxy_has_gl_extension("GL_EXT_sRGB_write_control");

    eglGetConfigs(screen->display, NULL, 0, &nconfigs);
    if (!(host_configs = calloc(nconfigs, sizeof *host_configs)))
        return NULL;

    eglGetConfigs(screen->display, host_configs, nconfigs, &nconfigs);

    /* We walk the EGL configs backwards to make building the
     * ->next chain easier.
     */
    for (i = nconfigs - 1; i >= 0; i--)
        for (j = 0; j < 3; j++) /* direct_color */
            for (k = 0; k < 2; k++) /* double_buffer */ {
                if (can_srgb)
                    c = translate_eglconfig(pScreen, screen, host_configs[i], c,
                                            /* direct_color */ j == 1,
                                            /* double_buffer */ k > 0,
                                            /* duplicate_for_composite */ j == 0,
                                            /* srgb_only */ true);

                c = translate_eglconfig(pScreen, screen, host_configs[i], c,
                                        /* direct_color */ j == 1,
                                        /* double_buffer */ k > 0,
                                        /* duplicate_for_composite */ j == 0,
                                        /* srgb_only */ false);
            }

    screen->configs = host_configs;
    return c ? &c->base : NULL;
}

static __GLXscreen *
egl_screen_probe(ScreenPtr pScreen)
{
    struct egl_screen *screen;
    glamor_screen_private *glamor_screen;
    __GLXscreen *base;

    if (enableIndirectGLX)
        return NULL; /* not implemented */

    glamor_screen = glamor_get_screen_private(pScreen);
    if (!glamor_screen)
        return NULL;

    if (!(screen = calloc(1, sizeof *screen)))
        return NULL;

    base = &screen->base;
    base->destroy = egl_screen_destroy;
    base->createDrawable = egl_create_glx_drawable;
    /* base.swapInterval = NULL; */

    screen->display = glamor_screen->ctx.display;

    __glXInitExtensionEnableBits(screen->base.glx_enable_bits);
    __glXEnableExtension(base->glx_enable_bits, "GLX_ARB_context_flush_control");
    __glXEnableExtension(base->glx_enable_bits, "GLX_ARB_create_context");
    __glXEnableExtension(base->glx_enable_bits, "GLX_ARB_create_context_no_error");
    __glXEnableExtension(base->glx_enable_bits, "GLX_ARB_create_context_profile");
    __glXEnableExtension(base->glx_enable_bits, "GLX_ARB_create_context_robustness");
    __glXEnableExtension(base->glx_enable_bits, "GLX_ARB_fbconfig_float");
    __glXEnableExtension(base->glx_enable_bits, "GLX_EXT_create_context_es2_profile");
    __glXEnableExtension(base->glx_enable_bits, "GLX_EXT_create_context_es_profile");
    __glXEnableExtension(base->glx_enable_bits, "GLX_EXT_fbconfig_packed_float");
    __glXEnableExtension(base->glx_enable_bits, "GLX_EXT_framebuffer_sRGB");
    __glXEnableExtension(base->glx_enable_bits, "GLX_EXT_no_config_context");
    __glXEnableExtension(base->glx_enable_bits, "GLX_EXT_texture_from_pixmap");
    __glXEnableExtension(base->glx_enable_bits, "GLX_MESA_copy_sub_buffer");
    // __glXEnableExtension(base->glx_enable_bits, "GLX_SGI_swap_control");

    base->fbconfigs = egl_mirror_configs(pScreen, screen);
    if (!base->fbconfigs) {
        free(screen);
        return NULL;
    }

    if (!screen->base.glvnd && glamor_screen->glvnd_vendor)
        screen->base.glvnd = strdup(glamor_screen->glvnd_vendor);

    if (!screen->base.glvnd)
        screen->base.glvnd = strdup("mesa");

    __glXScreenInit(base, pScreen);
    __glXsetGetProcAddress(eglGetProcAddress);

    return base;
}

__GLXprovider glamor_provider = {
    egl_screen_probe,
    "glamor",
    NULL
};

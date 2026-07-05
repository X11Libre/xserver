/*
 * Copyright 1999 SuSE, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of SuSE not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  SuSE makes no representations about the
 * suitability of this software for any purpose.  It is provided "as is"
 * without express or implied warranty.
 *
 * SuSE DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING ALL
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL SuSE
 * BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Author:  Keith Packard, SuSE, Inc.
 */

#include <kdrive-config.h>
#include "kdrive.h"

#include <string.h>

/* If this is ever changed, update the mode list too */
static const KdMonitorTiming kdDefaultTiming =
    {800, 600, 72, 50000,       /* VESA */
     56, 64, 240, KdSyncPositive,       /* 48.077 */
     37, 23, 66, KdSyncPositive,        /* 72.188 */
     };

#define KD_EXTRA_TIMINGS_4 {0}, {0}, {0}, {0},
#define KD_EXTRA_TIMINGS_16 KD_EXTRA_TIMINGS_4 KD_EXTRA_TIMINGS_4 KD_EXTRA_TIMINGS_4 KD_EXTRA_TIMINGS_4
#define KD_EXTRA_TIMINGS_64 KD_EXTRA_TIMINGS_16 KD_EXTRA_TIMINGS_16 KD_EXTRA_TIMINGS_16 KD_EXTRA_TIMINGS_16

static KdMonitorTiming kdMonitorTimings[] = {
    /*  H       V       Hz      KHz */
    /*  FP      BP      BLANK   POLARITY */

    /* IPAQ modeline:
     *
     * Modeline "320x240"      5.7222 320 337 340 352   240 241 244 254"
     */
    {320, 240, 64, 16256,
     17, 12, 32, KdSyncNegative,
     1, 11, 14, KdSyncNegative,
     },

    /* Other VESA modes */
    {640, 350, 85, 31500,       /* VESA */
     32, 96, 192, KdSyncPositive,       /* 26.413 */
     32, 60, 95, KdSyncNegative,        /* 59.354 */
     },
    {640, 400, 60, 31500,       /* VESA */
     32, 96, 192, KdSyncNegative,       /* 26.413 */
     1, 41, 45, KdSyncPositive, /* 59.354 */
     },
    {720, 400, 85, 35500,       /* VESA */
     36, 108, 216, KdSyncNegative,      /* 37.927 */
     1, 42, 46, KdSyncPositive, /* 85.039 */
     },

    /* Modeline "720x576"     29.000 720  736  800  880   576  577  580  625 */
    {
     720, 576, 52, 32954,       /* PAL Video */
     16, 80, 160, KdSyncPositive,       /* 32.954 */
     1, 45, 49, KdSyncPositive, /* 52.727 */
     },

    /* 640x480 modes */
    {640, 480, 85, 36000,       /* VESA */
     56, 80, 192, KdSyncNegative,       /* 43.269 */
     1, 25, 29, KdSyncNegative, /* 85.008 */
     },
    {640, 480, 75, 31500,       /* VESA */
     16, 120, 200, KdSyncNegative,      /* 37.500 */
     1, 16, 20, KdSyncNegative, /* 75.000 */
     },
    {640, 480, 72, 31500,       /* VESA */
     16, 120, 176, KdSyncNegative,      /* 37.861 */
     1, 20, 24, KdSyncNegative, /* 72.809 */
     },
    {640, 480, 60, 25175,       /* VESA */
     16, 48, 160, KdSyncNegative,       /* 31.469 */
     10, 33, 45, KdSyncNegative,        /* 59.940 */
     },

    /* 800x600 modes */
    {800, 600, 85, 56250,       /* VESA */
     32, 152, 248, KdSyncPositive,      /* 53.674 */
     1, 27, 31, KdSyncPositive, /* 85.061 */
     },
    {800, 600, 75, 49500,       /* VESA */
     16, 160, 256, KdSyncPositive,      /* 46.875 */
     1, 21, 25, KdSyncPositive, /* 75.000 */
     },
    /* XXX DEFAULT XXX */
    {800, 600, 72, 50000,       /* VESA */
     56, 64, 240, KdSyncPositive,       /* 48.077 */
     37, 23, 66, KdSyncPositive,        /* 72.188 */
     },
    {800, 600, 60, 40000,       /* VESA */
     40, 88, 256, KdSyncPositive,       /* 37.879 */
     1, 23, 28, KdSyncPositive, /* 60.317 */
     },
    {800, 600, 56, 36000,       /* VESA */
     24, 128, 224, KdSyncPositive,      /* 35.156 */
     1, 22, 25, KdSyncPositive, /* 56.250 */
     },

    /* 1024x768 modes */
    {1024, 768, 85, 94500,      /* VESA */
     48, 208, 352, KdSyncPositive,      /* 68.677 */
     1, 36, 40, KdSyncPositive, /* 84.997 */
     },
    {1024, 768, 75, 78750,      /* VESA */
     16, 176, 288, KdSyncPositive,      /* 60.023 */
     1, 28, 32, KdSyncPositive, /* 75.029 */
     },
    {1024, 768, 70, 75000,      /* VESA */
     24, 144, 304, KdSyncNegative,      /* 56.476 */
     3, 29, 38, KdSyncNegative, /* 70.069 */
     },
    {1024, 768, 60, 65000,      /* VESA */
     24, 160, 320, KdSyncNegative,      /* 48.363 */
     3, 29, 38, KdSyncNegative, /* 60.004 */
     },

    /* 1152x864 mode */
    {1152, 864, 75, 108000,     /* VESA */
     64, 256, 448, KdSyncPositive,      /* 67.500 */
     1, 32, 36, KdSyncPositive, /* 75.000 */
     },

    /* 1152x900 modes */
    {1152, 900, 85, 122500,     /* ADDED */
     48, 208, 384, KdSyncPositive,      /* 79.753 */
     1, 32, 38, KdSyncPositive, /* 85.024 */
     },
    {1152, 900, 75, 108250,     /* ADDED */
     32, 208, 384, KdSyncPositive,      /* 70.475 */
     1, 32, 38, KdSyncPositive, /* 75.133 */
     },
    {1152, 900, 70, 100250,     /* ADDED */
     32, 208, 384, KdSyncPositive,      /* 65.267 */
     2, 32, 38, KdSyncPositive, /* 69.581 */
     },
    {1152, 900, 66, 95000,      /* ADDED */
     32, 208, 384, KdSyncPositive,      /* 61.849 */
     1, 32, 38, KdSyncPositive, /* 65.937 */
     },

    /* 1280x854 modes */
    {1280, 854, 103, 12500,     /* ADDED */
     56, 16, 128, KdSyncPositive,       /* 102.554 */
     1, 216, 12, KdSyncPositive,
     },

    /* 1280x960 modes */
    {1280, 960, 85, 148500,     /* VESA */
     64, 224, 448, KdSyncPositive,      /* 85.938 */
     1, 47, 51, KdSyncPositive, /* 85.002 */
     },
    {1280, 960, 60, 108000,     /* VESA */
     96, 312, 520, KdSyncPositive,      /* 60.000 */
     1, 36, 40, KdSyncPositive, /* 60.000 */
     },

    /* 1280x1024 modes */
    {1280, 1024, 85, 157500,    /* VESA */
     64, 224, 448, KdSyncPositive,      /* 91.146 */
     1, 44, 48, KdSyncPositive, /* 85.024 */
     },
    {1280, 1024, 75, 135000,    /* VESA */
     16, 248, 408, KdSyncPositive,      /* 79.976 */
     1, 38, 42, KdSyncPositive, /* 75.025 */
     },
    {1280, 1024, 60, 108000,    /* VESA */
     48, 248, 408, KdSyncPositive,      /* 63.981 */
     1, 38, 42, KdSyncPositive, /* 60.020 */
     },

    /* 1600x1200 modes */
    {1600, 1200, 85, 229500,    /* VESA */
     64, 304, 560, KdSyncPositive,      /* 106.250 */
     1, 46, 50, KdSyncPositive, /* 85.000 */
     },
    {1600, 1200, 75, 202500,    /* VESA */
     64, 304, 560, KdSyncPositive,      /* 93.750 */
     1, 46, 50, KdSyncPositive, /* 75.000 */
     },
    {1600, 1200, 70, 189000,    /* VESA */
     64, 304, 560, KdSyncPositive,      /* 87.500 */
     1, 46, 50, KdSyncPositive, /* 70.000 */
     },
    {1600, 1200, 65, 175500,    /* VESA */
     64, 304, 560, KdSyncPositive,      /* 81.250 */
     1, 46, 50, KdSyncPositive, /* 65.000 */
     },
    {1600, 1200, 60, 162000,    /* VESA */
     64, 304, 560, KdSyncPositive,      /* 75.000 */
     1, 46, 50, KdSyncPositive, /* 60.000 */
     },

    /* 1792x1344 modes */
    {1792, 1344, 85, 301500,    /* ADDED */
     96, 352, 672, KdSyncNegative,      /* 122.362 */
     1, 92, 96, KdSyncPositive, /* 84.974 */
     },
    {1792, 1344, 75, 261000,    /* VESA */
     96, 352, 664, KdSyncNegative,      /* 106.270 */
     1, 69, 73, KdSyncPositive, /* 74.997 */
     },
    {1792, 1344, 60, 204750,    /* VESA */
     128, 328, 656, KdSyncNegative,     /* 83.640 */
     1, 46, 50, KdSyncPositive, /* 60.000 */
     },

#if 0
    {1800, 1012, 75},
    {1906, 1072, 68},
#endif

    /* 1856x1392 modes */
    {1856, 1392, 85, 330500,    /* ADDED */
     160, 352, 736, KdSyncNegative,     /* 127.508 */
     1, 104, 108, KdSyncPositive,       /* 85.001 */
     },
    {1856, 1392, 75, 288000,    /* VESA */
     128, 352, 704, KdSyncNegative,     /* 112.500 */
     1, 104, 108, KdSyncPositive,       /* 75.000 */
     },
    {1856, 1392, 60, 218250,    /* VESA */
     96, 352, 672, KdSyncNegative,      /* 86.333 */
     1, 43, 47, KdSyncPositive, /* 59.995 */
     },

    /* 1920x1440 modes */
    {1920, 1440, 85, 341750,    /* ADDED */
     160, 352, 760, KdSyncNegative,     /* 127.512 */
     1, 56, 60, KdSyncPositive, /* 85.012 */
     },
    {1920, 1440, 75, 297000,    /* VESA */
     144, 352, 720, KdSyncNegative,     /* 112.500 */
     1, 56, 60, KdSyncPositive, /* 75.000 */
     },
    {1920, 1440, 60, 234000,    /* VESA */
     128, 244, 680, KdSyncNegative,     /* 90.000 */
     1, 56, 60, KdSyncPositive, /* 60.000 */
     },

    /* Space for extra modes */
#define NUM_FREE_TIMINGS 64
    KD_EXTRA_TIMINGS_64
};

#define NUM_MONITOR_TIMINGS (sizeof kdMonitorTimings/sizeof kdMonitorTimings[0])

static int kdNumFreeMonitorTimings = NUM_FREE_TIMINGS;
static int kdNumMonitorTimings = NUM_MONITOR_TIMINGS - NUM_FREE_TIMINGS;

int
KdFindRate(KdScreenInfo * screen,
           Bool (*supported) (KdScreenInfo *, const KdMonitorTiming *))
{
    int i;
    const KdMonitorTiming *t;

    for (i = 0, t = kdMonitorTimings; i < kdNumMonitorTimings; i++, t++) {
        if ((*supported) (screen, t) &&
            t->horizontal == screen->width &&
            t->vertical == screen->height) {
            return t->rate;
        }
    }

    return 0;
}

const KdMonitorTiming *
KdFindMode(KdScreenInfo * screen,
           Bool (*supported) (KdScreenInfo *, const KdMonitorTiming *))
{
    int i;
    const KdMonitorTiming *t;

    for (i = 0, t = kdMonitorTimings; i < kdNumMonitorTimings; i++, t++) {
        if ((*supported) (screen, t) &&
            t->horizontal == screen->width &&
            t->vertical == screen->height &&
            (!screen->rate || t->rate <= screen->rate)) {
            return t;
        }
    }
    ErrorF("Warning: mode not found, using default\n");
    return &kdDefaultTiming;
}

Bool
KdAddMode(const KdMonitorTiming *new)
{
    int i;
    KdMonitorTiming *t;

    for (i = kdNumMonitorTimings, t = kdMonitorTimings; i > 0; i--, t++) {
        /* Look if the mode already exists */
        if ((t->horizontal == new->horizontal) &&
            (t->vertical == new->vertical) &&
            (t->rate == new->rate)) {
            return TRUE;
        }

        if (t->horizontal > new->horizontal) {
            break;
        }

        if ((t->horizontal == new->horizontal) &&
            (t->vertical > new->vertical)) {
            break;
        }

        if ((t->horizontal == new->horizontal) &&
            (t->vertical == new->vertical) &&
            (t->rate < new->rate)) {
            break;
        }
    }

    if (!kdNumFreeMonitorTimings) {
        return FALSE;
    }

    memmove(t + 1, t, i * sizeof(*t));
    *t = *new;

    kdNumFreeMonitorTimings--;
    kdNumMonitorTimings++;
    return TRUE;
}

/* Based on the CVT 1.2 reduced blanking formula, see https://glenwing.github.io/docs/VESA-CVT-1.2.pdf for the constants below */
static KdMonitorTiming
KdGenerateModeCVT(int width, int height, int rate)
{
    KdMonitorTiming new = {0};
    float hperiod;

/**
 * 3.4.3 Reduced Blanking Timing Version 2
 *
 * The following sections describe new rules mandated by the reduced blanking timing v2. New reduced blank
 * DMT timings shall use the reduced blanking timing v2 rules.
 */

/**
 * 1. Pixel Clock Selection
 *
 * The new version shall support a resolution of 0.001MHz to produce more accurate refresh rate result
 * required in some application.

 * The target refresh rate is comprised of a nominal refresh rate and optionally a 1000/1001 multiplier
 * factor for video optimized rates (i.e. for 59.94Hz, it has 60Hz nominal refresh rate and a 1000/1001
 * factor).
 *
 * The following lists the steps taken to calculate the pixel clock for a given target refresh rate and
 * active H/V resolution; further details are in Section 5.4.
 *
 * a) First the nominal refresh rate is used to calculate the horizontal and vertical blank parameters,
 *
 * b) then calculate horizontal and vertical blank parameter along with required H/V active with the
 * target refresh rate (including 1000/1001 factor if required) is used to calculate the pixel clock.
 *
 * c) The result value is then rounded to nearest 0.001 pixel clock
 *
 * Using the nominal value in step (a) guarantees that the only difference in timing between a video
 * optimized timing vs. a non-video optimized timing for a given refresh rate is in pixel clock (i.e. all
 * other vertical and horizontal parameters are same).
 */
#define CVT_CLOCK_STEP 1 /* KHZ */ /* unused since the remainder when dividing by 1 is always 0 */

/**
 * 2. Vertical Refresh Rate
 *
 * The standard refresh rate for Reduced Blanking v2 timing is 60Hz however other progressive refresh
 * may be used depending on the application. Higher precision of the pixel clock step allows video
 * optimized refresh rates (i.e. 60*1000/1001Hz, 30*1000/1001Hz) to be supported with the new
 * version. A factor of 1000/1001 is applied to the nominal refresh rate if the video optimized target
 * refresh rate is required.
 */
#define CVT_RB_DEF_RATE 60 /* HZ */

/**
 * 3. Horizontal Counts
 *
 * As per rules of the Reduced Blanking v2 timings, Horizontal Timings may have a precision of 1
 * pixel. This allows timing for resolutions like 1366x768 to be defined with the new standard. No
 * longer is the Horizontal Timing, including the Horizontal Active pixels, Horizontal Total pixels, Sync
 * Pulse duration and “Front Porch” and “Back Porch” times required to be divisible by eight.
 */
#define CVT_CELL_GRAN 1 /* Pixels */ /* unused since the remainder when dividing by 1 is always 0 */

/**
 * 4. Horizontal Blanking Time
 *
 * For Reduced Blanking v2 timings, the Horizontal Blanking time will in all cases are fixed to 80 clock
 * cycles instead of 160 clock cycles required by earlier Reduced Blanking Timing.
 *
 * 5. Horizontal Sync Pulse Duration and Position
 *
 * The Horizontal Sync Pulse duration will in all cases be 32 pixel clocks in duration, with the position
 * set so that the trailing edge of the Horizontal Sync Pulse is located in the center of the Horizontal
 * Blanking period. This implies that for a fixed blank of 80 pixel clocks, the Horizontal Back Porch is
 * fixed to (80/2) 40 pixel clocks and the Horizontal Front Porch is fixed to (80-40-32) = 8 clock cycles.
 */
#define CVT_RB_HFP 8 /* Pixels */
#define CVT_RB_HBP 40 /* Pixels */
#define CVT_RB_HBLANK 80 /* Pixels */

/**
 * 6. Vertical Blanking Time
 *
 * The Vertical Blanking shall be the first multiple of integer Horizontal Lines that exceeds the
 * minimum requirement of 460 microseconds.
 */
#define CVT_RB_MIN_VBLANK 460 /* microseconds */
#define HZ2USEC(x) (1e6/(x))

/*
 * 7. Vertical Sync Pulse Duration and Position
 *
 * Vertical Sync Pulse is fixed at 8 lines indicating timing generated based on Reduced Blanking v2
 * timing rules and aspect ratio information is to be derived based on Vertical and Horizontal Active
 * Timing. This will allow any new timing with non-standard aspect ratio to be supported without any
 * update to the specification. The Vertical Back Porch shall in all cases be fixed to 6 lines. The Vertical
 * Front Porch shall be the remainder of the Vertical Blanking Time.
 */
#define CVT_RB_VSYNC 8
#define CVT_RB_VBP 6
/* VFB and VBLANK are calulated based on the above */

/* Don't leave the front porch 0 */
#define CVT_MIN_VFPORCH 1

    new.horizontal = width;
    new.vertical = height;
    new.rate = (rate > 0) ? rate : CVT_RB_DEF_RATE;

    new.hfp = CVT_RB_HFP;
    new.hbp = CVT_RB_HBP;
    new.hblank = CVT_RB_HBLANK;

    /* The polarities are flipped for the non-reduced blanking formula */
    new.hpol = KdSyncPositive;
    new.vpol = KdSyncNegative;

    new.vbp = CVT_RB_VBP;

    /* XXX adapted from libxcvt */
    /* 8. Estimate Horizontal period. */
    hperiod = ((float) (HZ2USEC(new.rate) - CVT_RB_MIN_VBLANK)) / new.vertical;
    if (hperiod <= 0) {
        hperiod = 1;
    }

    /* 9. Find number of lines in vertical blanking */
    new.vblank = ((float) CVT_RB_MIN_VBLANK) / hperiod + 1;

    /* 10. Check if vertical blanking is sufficient */
    if (new.vblank < (CVT_MIN_VFPORCH + CVT_RB_VSYNC + CVT_RB_VBP)) {
        new.vblank = CVT_MIN_VFPORCH + CVT_RB_VSYNC + CVT_RB_VBP;
    }

    new.vfp = new.vblank - CVT_RB_VBP - CVT_RB_VSYNC;

    /* 15/13. Find pixel clock frequency (kHz for xf86) */
    new.clock = (new.horizontal + CVT_RB_HBLANK) * 1000.0 / hperiod;

    return new;
}

/* Generate a mode based on the reduced blanking CVT formula and add it */
Bool
KdAddModeCVT(int width, int height, int rate)
{
    int i;
    KdMonitorTiming *t;
    KdMonitorTiming new;

    for (i = kdNumMonitorTimings, t = kdMonitorTimings; i > 0; i--, t++) {
        /* Look if the mode already exists */
        if ((t->horizontal == width) &&
            (t->vertical == height) &&
            (t->rate == rate)) {
            return TRUE;
        }
    }

    new = KdGenerateModeCVT(width, height, rate);
    return KdAddMode(&new);
}

static const KdMonitorTiming *
kdFindPrevSize(const KdMonitorTiming * old)
{
    const KdMonitorTiming *new;

    if (old == kdMonitorTimings)
        return 0;
    new = old;
    /*
     * Search for the previous size
     */
    while (new != kdMonitorTimings) {
        new--;
        if (new->horizontal != old->horizontal &&
            new->vertical != old->vertical) {
            break;
        }
    }
#if 0
    /*
     * Match the refresh rate (<=)
     */
    while (new != kdMonitorTimings) {
        const KdMonitorTiming *prev = new - 1;
        if (prev->horizontal == new->horizontal &&
            prev->vertical == new->vertical && prev->rate > old->rate) {
            break;
        }
        new--;
    }
#endif
    return new;
}

Bool
KdTuneMode(KdScreenInfo * screen, const KdMonitorTiming *m,
           Bool (*usable) (KdScreenInfo *, const KdMonitorTiming *),
           Bool (*supported) (KdScreenInfo *, const KdMonitorTiming *))
{
    const KdMonitorTiming *t = m;
    int depth = screen->fb.depth;

    if (t) {
        screen->width = t->horizontal;
        screen->height = t->vertical;
        screen->rate = t->rate;
    }

    while (!(*usable) (screen, t)) {
        /*
         * Fix requested depth and geometry until it works
         */
        if (screen->fb.depth > 16)
            screen->fb.depth = 16;
        else if (screen->fb.depth > 8)
            screen->fb.depth = 8;
        else {
            screen->fb.depth = depth;
            t = kdFindPrevSize(KdFindMode(screen, supported));
            if (!t)
                return FALSE;
            screen->width = t->horizontal;
            screen->height = t->vertical;
            screen->rate = t->rate;
        }
    }
    return TRUE;
}

#ifdef RANDR
Bool
KdRandRGetInfo(ScreenPtr pScreen,
               int randr,
               Bool (*supported) (ScreenPtr pScreen, const KdMonitorTiming *))
{
    KdScreenPriv(pScreen);
    KdScreenInfo *screen = pScreenPriv->screen;
    int i;
    const KdMonitorTiming *t;

    for (i = 0, t = kdMonitorTimings; i < kdNumMonitorTimings; i++, t++) {
        if ((*supported) (pScreen, t)) {
            RRScreenSizePtr pSize;

            pSize = RRRegisterSize(pScreen,
                                   t->horizontal,
                                   t->vertical,
                                   screen->width_mm, screen->height_mm);
            if (!pSize)
                return FALSE;
            if (!RRRegisterRate(pScreen, pSize, t->rate))
                return FALSE;
            if (t->horizontal == screen->width &&
                t->vertical == screen->height && t->rate == screen->rate)
                RRSetCurrentConfig(pScreen, randr, t->rate, pSize);
        }
    }

    return TRUE;
}

const KdMonitorTiming *
KdRandRGetTiming(ScreenPtr pScreen,
                 Bool (*supported) (ScreenPtr pScreen,
                                    const KdMonitorTiming *),
                 int rate, RRScreenSizePtr pSize)
{
    int i;
    const KdMonitorTiming *t;

    for (i = 0, t = kdMonitorTimings; i < kdNumMonitorTimings; i++, t++) {
        if (t->horizontal == pSize->width &&
            t->vertical == pSize->height &&
            t->rate == rate && (*supported) (pScreen, t))
            return t;
    }
    return 0;
}
#endif

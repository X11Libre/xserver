/*
 * Copyright (c) 2006 Advanced Micro Devices, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Neither the name of the Advanced Micro Devices, Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from this
 * software without specific prior written permission.
 */

 /*
  * Cimarron mode tables.
  */

/*-------------------------------*/
/*  PREDEFINED DISPLAY TIMINGS   */
/*-------------------------------*/

static const VG_DISPLAY_MODE CimarronDisplayModes[] = {
    /* 320 x 240 PANEL */

    {VG_SUPPORTFLAG_75HZ |      /* refresh rate = 75    */
     VG_SUPPORTFLAG_8BPP | VG_SUPPORTFLAG_12BPP | VG_SUPPORTFLAG_15BPP | VG_SUPPORTFLAG_16BPP | VG_SUPPORTFLAG_24BPP | VG_SUPPORTFLAG_32BPP | VG_SUPPORTFLAG_PANEL,     /* Panel Mode.          */
     VG_MODEFLAG_LOW_BAND |     /* Low bandwidth mode.  */
     VG_MODEFLAG_PANELOUT |     /* Panel output.        */
     VG_MODEFLAG_NEG_HSYNC | VG_MODEFLAG_NEG_VSYNC |    /* negative syncs */
     VG_MODEFLAG_QVGA,          /* QVGA Panel size.     */
     320, 240,                  /* No scaling.          */
     320, 240,                  /* 320x240 active       */
     320, 240,                  /* 320x240 panel        */
     0, 0, 0, 0, 0,             /* No panel registers   */
     0x0140, 0x0148, 0x0162, 0x0180, 0x0188, 0x0190,    /* horiz timings   */
     0x00F0, 0x00F4, 0x00F9, 0x00FD, 0x00FF, 0x0104,    /* vertical timings */
     (31L << 16) | ((2000L * 65536L) / 10000L), /* freq = 31.2000 MHz */
     }
    ,

    /* 640 x 400 */

    {VG_SUPPORTFLAG_70HZ |      /* refresh rate = 60    */
     VG_SUPPORTFLAG_8BPP | VG_SUPPORTFLAG_12BPP | VG_SUPPORTFLAG_15BPP
     | VG_SUPPORTFLAG_16BPP | VG_SUPPORTFLAG_24BPP | VG_SUPPORTFLAG_32BPP,
     VG_MODEFLAG_LOW_BAND |     /* Low bandwidth mode.  */
     VG_MODEFLAG_NEG_HSYNC,     /* negative HSYNC       */
     640, 400,                  /* No scaling.          */
     0, 0,                      /* No mode dimensions.  */
     0, 0,                      /* No panel dimensions. */
     0, 0, 0, 0, 0,             /* No panel registers   */
     0x280, 0x288, 0x290, 0x2F0, 0x318, 0x320,  /* horiz timings   */
     0x190, 0x197, 0x19C, 0x19E, 0x1BA, 0x1C1,  /* vertical timings */
     0, 0, 0, 0, 0, 0,          /* No even timings      */
     (25L << 16) | ((1750L * 65536L) / 10000L), /* freq = 25.175 MHz    */
     }
    ,

    /* 640x480 */

    {VG_SUPPORTFLAG_60HZ |      /* refresh rate = 60    */
     VG_SUPPORTFLAG_8BPP | VG_SUPPORTFLAG_12BPP | VG_SUPPORTFLAG_15BPP
     | VG_SUPPORTFLAG_16BPP | VG_SUPPORTFLAG_24BPP | VG_SUPPORTFLAG_32BPP,
     VG_MODEFLAG_LOW_BAND |     /* Low bandwidth mode.  */
     VG_MODEFLAG_NEG_HSYNC | VG_MODEFLAG_NEG_VSYNC,     /* negative syncs */
     640, 480,                  /* No scaling.          */
     0, 0,                      /* No mode dimensions.  */
     0, 0,                      /* No panel dimensions. */
     0, 0, 0, 0, 0,             /* No panel registers   */
     0x0280, 0x0288, 0x0290, 0x02E8, 0x0318, 0x0320,    /* horiz timings   */
     0x01E0, 0x01E8, 0x01EA, 0x01EC, 0x0205, 0x020D,    /* vertical timings */
     0, 0, 0, 0, 0, 0,          /* No even timings      */
     (25L << 16) | ((1750L * 65536L) / 10000L), /* freq = 25.175 MHz    */
     }
    ,

    {VG_SUPPORTFLAG_70HZ |      /* refresh rate = 70    */
     VG_SUPPORTFLAG_8BPP | VG_SUPPORTFLAG_12BPP | VG_SUPPORTFLAG_15BPP
     | VG_SUPPORTFLAG_16BPP | VG_SUPPORTFLAG_24BPP | VG_SUPPORTFLAG_32BPP,
     VG_MODEFLAG_LOW_BAND |     /* Low bandwidth mode.  */
     0,
     640, 480,                  /* No scaling.          */
     0, 0,                      /* No mode dimensions.  */
     0, 0,                      /* No panel dimensions. */
     0, 0, 0, 0, 0,             /* No panel registers   */
     0x0280, 0x0280, 0x0298, 0x02D8, 0x0330, 0x0330,    /* horiz timings   */
     0x01E0, 0x01E0, 0x01E2, 0x01E5, 0x01F4, 0x01F4,    /* vertical timings */
     0, 0, 0, 0, 0, 0,          /* No even timings      */
     (28L << 16) | ((5600L * 65536L) / 10000L), /* freq = 28.560 MHz    */
     }
    ,

    {VG_SUPPORTFLAG_72HZ |      /* refresh rate = 72    */
     VG_SUPPORTFLAG_8BPP | VG_SUPPORTFLAG_12BPP | VG_SUPPORTFLAG_15BPP
     | VG_SUPPORTFLAG_16BPP | VG_SUPPORTFLAG_24BPP | VG_SUPPORTFLAG_32BPP,
     VG_MODEFLAG_LOW_BAND |     /* Low bandwidth mode.  */
     VG_MODEFLAG_NEG_HSYNC | VG_MODEFLAG_NEG_VSYNC,     /* negative syncs */
     640, 480,                  /* No scaling.          */
     0, 0,                      /* No mode dimensions.  */
     0, 0,                      /* No panel dimensions. */
     0, 0, 0, 0, 0,             /* No panel registers   */
     0x0280, 0x0288, 0x0298, 0x02c0, 0x0338, 0x0340,    /* horiz timings   */
     0x01e0, 0x01e8, 0x01e9, 0x01ec, 0x0200, 0x0208,    /* vertical timings */
     0, 0, 0, 0, 0, 0,          /* No even timings      */
     (31L << 16) | ((5000L * 65536L) / 10000L), /* freq = 31.5 MHz      */
     }
    ,

    {VG_SUPPORTFLAG_75HZ |      /* refresh rate = 75    */
     VG_SUPPORTFLAG_8BPP | VG_SUPPORTFLAG_12BPP | VG_SUPPORTFLAG_15BPP
     | VG_SUPPORTFLAG_16BPP | VG_SUPPORTFLAG_24BPP | VG_SUPPORTFLAG_32BPP,
     VG_MODEFLAG_LOW_BAND |     /* Low bandwidth mode.  */
     VG_MODEFLAG_NEG_HSYNC | VG_MODEFLAG_NEG_VSYNC,     /* negative syncs */
     640, 480,                  /* No scaling.          */
     0, 0,                      /* No mode dimensions.  */
     0, 0,                      /* No panel dimensions. */
     0, 0, 0, 0, 0,             /* No panel registers   */
     0x0280, 0x0280, 0x0290, 0x02D0, 0x0348, 0x0348,    /* horiz timings   */
     0x01E0, 0x01E0, 0x01E1, 0x01E4, 0x01F4, 0x01F4,    /* vertical timings */
     0, 0, 0, 0, 0, 0,          /* No even timings      */
     (31L << 16) | ((5000L * 65536L) / 10000L), /* freq = 31.5 MHz      */
     }
    ,

    {VG_SUPPORTFLAG_85HZ |      /* refresh rate = 85    */
     VG_SUPPORTFLAG_8BPP | VG_SUPPORTFLAG_12BPP | VG_SUPPORTFLAG_15BPP
     | VG_SUPPORTFLAG_16BPP | VG_SUPPORTFLAG_24BPP | VG_SUPPORTFLAG_32BPP,
     VG_MODEFLAG_LOW_BAND |     /* Low bandwidth mode.  */
     VG_MODEFLAG_NEG_HSYNC | VG_MODEFLAG_NEG_VSYNC,     /* negative syncs */
     640, 480,                  /* No scaling.          */
     0, 0,                      /* No mode dimensions.  */
     0, 0,                      /* No panel dimensions. */
     0, 0, 0, 0, 0,             /* No panel registers   */
     0x0280, 0x0280, 0x02B8, 0x02F0, 0x0340, 0x0340,    /* horiz timings   */
     0x01E0, 0x01E0, 0x01E1, 0x01E4, 0x01FD, 0x01FD,    /* vertical timings */
     0, 0, 0, 0, 0, 0,          /* No even timings      */
     (36L << 16) | ((0000L * 65536L) / 10000L), /* freq = 36.0 MHz      */
     }
    ,

    {VG_SUPPORTFLAG_90HZ |      /* refresh rate = 90    */
     VG_SUPPORTFLAG_8BPP | VG_SUPPORTFLAG_12BPP | VG_SUPPORTFLAG_15BPP
     | VG_SUPPORTFLAG_16BPP | VG_SUPPORTFLAG_24BPP | VG_SUPPORTFLAG_32BPP,
     VG_MODEFLAG_LOW_BAND |     /* Low bandwidth mode.  */
     0,
     640, 480,                  /* No scaling.          */
     0, 0,                      /* No mode dimensions.  */
     0, 0,                      /* No panel dimensions. */
     0, 0, 0, 0, 0,             /* No panel registers   */
     0x0280, 0x0280, 0x02A0, 0x02E0, 0x0340, 0x0340,    /* horiz timings   */
     0x01E0, 0x01E0, 0x01E1, 0x01E4, 0x01FA, 0x01FA,    /* vertical timings */
     0, 0, 0, 0, 0, 0,          /* No even timings      */
     (37L << 16) | ((8890L * 65536L) / 10000L), /* freq = 37.889 MHz    */
     }
    ,

    {VG_SUPPORTFLAG_100HZ |     /* refresh rate = 100   */
     VG_SUPPORTFLAG_8BPP | VG_SUPPORTFLAG_12BPP | VG_SUPPORTFLAG_15BPP
     | VG_SUPPORTFLAG_16BPP | VG_SUPPORTFLAG_24BPP | VG_SUPPORTFLAG_32BPP,
     VG_MODEFLAG_LOW_BAND |     /* Low bandwidth mode.  */
     0,
     640, 480,                  /* No scaling.          */
     0, 0,                      /* No mode dimensions.  */
     0, 0,                      /* No panel dimensions. */
     0, 0, 0, 0, 0,             /* No panel registers   */
     0x0280, 0x0280, 0x02A8, 0x02E8, 0x0350, 0x0350,    /* horiz timings   */
     0x01E0, 0x01E0, 0x01E1, 0x01E4, 0x01FD, 0x01FD,    /* vertical timings */
     0, 0, 0, 0, 0, 0,          /* No even timings      */
     (43L << 16) | ((1630L * 65536L) / 10000L), /* freq = 43.163 MHz    */
     }
    ,

    /* 640 x 480 PANEL */

    {VG_SUPPORTFLAG_60HZ |      /* refresh rate = 60    */
     VG_SUPPORTFLAG_8BPP | VG_SUPPORTFLAG_12BPP | VG_SUPPORTFLAG_15BPP | VG_SUPPORTFLAG_16BPP | VG_SUPPORTFLAG_24BPP | VG_SUPPORTFLAG_32BPP | VG_SUPPORTFLAG_PANEL,     /* Panel Mode.          */
     VG_MODEFLAG_LOW_BAND |     /* Low bandwidth mode.  */
     VG_MODEFLAG_PANELOUT |     /* Panel output.        */
     VG_MODEFLAG_NEG_HSYNC | VG_MODEFLAG_NEG_VSYNC,     /* negative syncs */
     640, 480,                  /* No scaling.          */
     640, 480,                  /* 640x480 active       */
     640, 480,                  /* 640x480 panel        */
     0, 0, 0, 0, 0,             /* No panel registers   */
     0x0280, 0x0280, 0x0290, 0x02E8, 0x0318, 0x0320,    /* horiz timings   */
     0x01E0, 0x01E0, 0x01EA, 0x01EC, 0x0205, 0x020D,    /* vertical timings */
     0, 0, 0, 0, 0, 0,          /* No even timings      */
     (25L << 16) | ((1750L * 65536L) / 10000L), /* freq = 25.175 MHz    */
     }
    ,

    /* 800x600 */

    {VG_SUPPORTFLAG_56HZ |      /* refresh rate = 56    */
     VG_SUPPORTFLAG_8BPP | VG_SUPPORTFLAG_12BPP | VG_SUPPORTFLAG_15BPP
     | VG_SUPPORTFLAG_16BPP | VG_SUPPORTFLAG_24BPP | VG_SUPPORTFLAG_32BPP,
     VG_MODEFLAG_LOW_BAND |     /* Low bandwidth mode.  */
     0,
     800, 600,                  /* No scaling.          */
     0, 0,                      /* No mode dimensions.  */
     0, 0,                      /* No panel dimensions. */
     0, 0, 0, 0, 0,             /* No panel registers   */
     0x0320, 0x0320, 0x0338, 0x0380, 0x0400, 0x0400,    /* horiz timings   */
     0x0258, 0x0258, 0x0259, 0x025B, 0x0271, 0x0271,    /* vertical timings */
     0, 0, 0, 0, 0, 0,          /* No even timings      */
     (36L << 16) | ((0000L * 65536L) / 10000L), /* freq = 36.0 MHz      */
     }
    ,

    {VG_SUPPORTFLAG_60HZ |      /* refresh rate = 60    */
     VG_SUPPORTFLAG_8BPP | VG_SUPPORTFLAG_12BPP | VG_SUPPORTFLAG_15BPP
     | VG_SUPPORTFLAG_16BPP | VG_SUPPORTFLAG_24BPP | VG_SUPPORTFLAG_32BPP,
     VG_MODEFLAG_LOW_BAND |     /* Low bandwidth mode.  */
     0,
     800, 600,                  /* No scaling.          */
     0, 0,                      /* No mode dimensions.  */
     0, 0,                      /* No panel dimensions. */
     0, 0, 0, 0, 0,             /* No panel registers   */
     0x0320, 0x0328, 0x0348, 0x03D0, 0x0418, 0x0420,    /* horiz timings   */
     0x0258, 0x0258, 0x0259, 0x025D, 0x0274, 0x0274,    /* vertical timings */
     0, 0, 0, 0, 0, 0,          /* No even timings      */
     (40L << 16) | ((0000L * 65536L) / 10000L), /* freq = 40.00 MHz     */
     }
    ,

    {VG_SUPPORTFLAG_70HZ |      /* refresh rate = 70    */
     VG_SUPPORTFLAG_8BPP | VG_SUPPORTFLAG_12BPP | VG_SUPPORTFLAG_15BPP
     | VG_SUPPORTFLAG_16BPP | VG_SUPPORTFLAG_24BPP | VG_SUPPORTFLAG_32BPP,
     VG_MODEFLAG_LOW_BAND |     /* Low bandwidth mode.  */
     0,
     800, 600,                  /* No scaling.          */
     0, 0,                      /* No mode dimensions.  */
     0, 0,                      /* No panel dimensions. */
     0, 0, 0, 0, 0,             /* No panel registers   */
     0x0320, 0x0320, 0x0348, 0x0398, 0x0410, 0x0410,    /* horiz timings   */
     0x0258, 0x0258, 0x025c, 0x025F, 0x0274, 0x0274,    /* vertical timings */
     0, 0, 0, 0, 0, 0,          /* No even timings      */
     (45L << 16) | ((7200L * 65536L) / 10000L), /* freq = 45.72 MHz     */
     }
    ,

    {VG_SUPPORTFLAG_72HZ |      /* refresh rate = 72    */
     VG_SUPPORTFLAG_8BPP | VG_SUPPORTFLAG_12BPP | VG_SUPPORTFLAG_15BPP
     | VG_SUPPORTFLAG_16BPP | VG_SUPPORTFLAG_24BPP | VG_SUPPORTFLAG_32BPP,
     VG_MODEFLAG_LOW_BAND |     /* Low bandwidth mode.  */
     0,
     800, 600,                  /* No scaling.          */
     0, 0,                      /* No mode dimensions.  */
     0, 0,                      /* No panel dimensions. */
     0, 0, 0, 0, 0,             /* No panel registers   */
     0x0320, 0x0320, 0x0358, 0x03D0, 0x0410, 0x0410,    /* horiz timings   */
     0x0258, 0x0258, 0x027D, 0x0283, 0x029A, 0x029A,    /* vertical timings */
     0, 0, 0, 0, 0, 0,          /* No even timings      */
     (49L << 16) | ((5000L * 65536L) / 10000L), /* freq = 49.5 MHz      */
     }
    ,

    {VG_SUPPORTFLAG_75HZ |      /* refresh rate = 75    */
     VG_SUPPORTFLAG_8BPP | VG_SUPPORTFLAG_12BPP | VG_SUPPORTFLAG_15BPP
     | VG_SUPPORTFLAG_16BPP | VG_SUPPORTFLAG_24BPP | VG_SUPPORTFLAG_32BPP,
     VG_MODEFLAG_LOW_BAND |     /* Low bandwidth mode.  */
     0,
     800, 600,                  /* No scaling.          */
     0, 0,                      /* No mode dimensions.  */
     0, 0,                      /* No panel dimensions. */
     0, 0, 0, 0, 0,             /* No panel registers   */
     0x0320, 0x0320, 0x0330, 0x0380, 0x0420, 0x0420,    /* horiz timings   */
     0x0258, 0x0258, 0x0259, 0x025C, 0x0271, 0x0271,    /* vertical timings */
     0, 0, 0, 0, 0, 0,          /* No even timings      */
     (49L << 16) | ((5000L * 65536L) / 10000L), /* freq = 49.5 MHz      */
     }
    ,

    {VG_SUPPORTFLAG_85HZ |      /* refresh rate = 85    */
     VG_SUPPORTFLAG_8BPP | VG_SUPPORTFLAG_12BPP | VG_SUPPORTFLAG_15BPP
     | VG_SUPPORTFLAG_16BPP | VG_SUPPORTFLAG_24BPP | VG_SUPPORTFLAG_32BPP,
     VG_MODEFLAG_LOW_BAND |     /* Low bandwidth mode.  */
     0,
     800, 600,                  /* No scaling.          */
     0, 0,                      /* No mode dimensions.  */
     0, 0,                      /* No panel dimensions. */
     0, 0, 0, 0, 0,             /* No panel registers   */
     0x0320, 0x0320, 0x0340, 0x0380, 0x0418, 0x0418,    /* horiz timings   */
     0x0258, 0x0258, 0x0259, 0x025C, 0x0277, 0x0277,    /* vertical timings */
     0, 0, 0, 0, 0, 0,          /* No even timings      */
     (56L << 16) | ((2500L * 65536L) / 10000L), /* freq = 56.25 MHz     */
     }
    ,

    {VG_SUPPORTFLAG_90HZ |      /* refresh rate = 90    */
     VG_SUPPORTFLAG_8BPP | VG_SUPPORTFLAG_12BPP | VG_SUPPORTFLAG_15BPP
     | VG_SUPPORTFLAG_16BPP | VG_SUPPORTFLAG_24BPP | VG_SUPPORTFLAG_32BPP,
     VG_MODEFLAG_LOW_BAND |     /* Low bandwidth mode.  */
     0,
     800, 600,                  /* No scaling.          */
     0, 0,                      /* No mode dimensions.  */
     0, 0,                      /* No panel dimensions. */
     0, 0, 0, 0, 0,             /* No panel registers   */
     0x0320, 0x0320, 0x0348, 0x03A0, 0x0420, 0x0420,    /* horiz timings   */
     0x0258, 0x0258, 0x0259, 0x025C, 0x0278, 0x0278,    /* vertical timings */
     0, 0, 0, 0, 0, 0,          /* No even timings      */
     (60L << 16) | ((650L * 65536L) / 10000L),  /* freq = 60.065 MHz    */
     }
    ,

    {VG_SUPPORTFLAG_100HZ |     /* refresh rate = 100   */
     VG_SUPPORTFLAG_8BPP | VG_SUPPORTFLAG_12BPP | VG_SUPPORTFLAG_15BPP
     | VG_SUPPORTFLAG_16BPP | VG_SUPPORTFLAG_24BPP | VG_SUPPORTFLAG_32BPP,
     VG_MODEFLAG_LOW_BAND |     /* Low bandwidth mode.  */
     0,
     800, 600,                  /* No scaling.          */
     0, 0,                      /* No mode dimensions.  */
     0, 0,                      /* No panel dimensions. */
     0, 0, 0, 0, 0,             /* No panel registers   */
     0x0320, 0x0320, 0x0350, 0x03A8, 0x0430, 0x0430,    /* horiz timings   */
     0x0258, 0x0258, 0x0259, 0x025C, 0x0277, 0x027C,    /* vertical timings */
     0, 0, 0, 0, 0, 0,          /* No even timings      */
     (68L << 16) | ((1790L * 65536L) / 10000L), /* freq = 68.179 MHz    */
     }
    ,

    /* 800x600 PANEL */

    {VG_SUPPORTFLAG_60HZ |      /* refresh rate = 60    */
     VG_SUPPORTFLAG_8BPP | VG_SUPPORTFLAG_12BPP | VG_SUPPORTFLAG_15BPP | VG_SUPPORTFLAG_16BPP | VG_SUPPORTFLAG_24BPP | VG_SUPPORTFLAG_32BPP | VG_SUPPORTFLAG_PANEL,     /* Panel Mode.          */
     VG_MODEFLAG_LOW_BAND |     /* Low bandwidth mode.  */
     VG_MODEFLAG_PANELOUT |     /* Panel output.        */
     VG_MODEFLAG_NEG_HSYNC | VG_MODEFLAG_NEG_VSYNC,     /* negative syncs */
     800, 600,                  /* No scaling.          */
     800, 600,                  /* 800x600 active.      */
     800, 600,                  /* 800x600 panel        */
     0, 0, 0, 0, 0,             /* No panel registers   */
     0x0320, 0x0320, 0x0348, 0x03C8, 0x0420, 0x0420,    /* horiz timings   */
     0x0258, 0x0258, 0x0259, 0x025D, 0x0274, 0x0274,    /* vertical timings */
     0, 0, 0, 0, 0, 0,          /* No even timings      */
     (40L << 16) | ((0000L * 65536L) / 10000L), /* freq = 40.00 MHz     */
     }
    ,

    /* 1024x768 */

    {VG_SUPPORTFLAG_60HZ |      /* refresh rate = 60    */
     VG_SUPPORTFLAG_8BPP | VG_SUPPORTFLAG_12BPP | VG_SUPPORTFLAG_15BPP
     | VG_SUPPORTFLAG_16BPP | VG_SUPPORTFLAG_24BPP | VG_SUPPORTFLAG_32BPP,
     VG_MODEFLAG_AVG_BAND |     /* Avg bandwidth mode.  */
     VG_MODEFLAG_NEG_HSYNC | VG_MODEFLAG_NEG_VSYNC,     /* negative syncs */
     1024, 768,                 /* No scaling.          */
     0, 0,                      /* No mode dimensions.  */
     0, 0,                      /* No panel dimensions. */
     0, 0, 0, 0, 0,             /* No panel registers   */
     0x0400, 0x0400, 0x0418, 0x04A0, 0x0540, 0x0540,    /* horiz timings   */
     0x0300, 0x0300, 0x0303, 0x0309, 0x0326, 0x0326,    /* vertical timings */
     0, 0, 0, 0, 0, 0,          /* No even timings      */
     (65L << 16) | ((0000L * 65536L) / 10000L), /* freq = 65.00 MHz     */
     }
    ,

    {VG_SUPPORTFLAG_70HZ |      /* refresh rate = 70    */
     VG_SUPPORTFLAG_8BPP | VG_SUPPORTFLAG_12BPP | VG_SUPPORTFLAG_15BPP
     | VG_SUPPORTFLAG_16BPP | VG_SUPPORTFLAG_24BPP | VG_SUPPORTFLAG_32BPP,
     VG_MODEFLAG_AVG_BAND |     /* Avg bandwidth mode.  */
     VG_MODEFLAG_NEG_HSYNC | VG_MODEFLAG_NEG_VSYNC,     /* negative syncs */
     1024, 768,                 /* No scaling.          */
     0, 0,                      /* No mode dimensions.  */
     0, 0,                      /* No panel dimensions. */
     0, 0, 0, 0, 0,             /* No panel registers   */
     0x0400, 0x0400, 0x0418, 0x04A0, 0x0530, 0x0530,    /* horiz timings   */
     0x0300, 0x0300, 0x0303, 0x0309, 0x0326, 0x0326,    /* vertical timings */
     0, 0, 0, 0, 0, 0,          /* No even timings      */
     (75L << 16) | ((0000L * 65536L) / 10000L), /* freq = 75.0 MHz      */
     }
    ,

    {VG_SUPPORTFLAG_72HZ |      /* refresh rate = 72    */
     VG_SUPPORTFLAG_8BPP | VG_SUPPORTFLAG_12BPP | VG_SUPPORTFLAG_15BPP
     | VG_SUPPORTFLAG_16BPP | VG_SUPPORTFLAG_24BPP | VG_SUPPORTFLAG_32BPP,
     VG_MODEFLAG_AVG_BAND |     /* Avg bandwidth mode.  */
     0,
     1024, 768,                 /* No scaling.          */
     0, 0,                      /* No mode dimensions.  */
     0, 0,                      /* No panel dimensions. */
     0, 0, 0, 0, 0,             /* No panel registers   */
     0x0400, 0x0400, 0x0438, 0x04A8, 0x0550, 0x0550,    /* horiz timings   */
     0x0300, 0x0300, 0x0304, 0x0307, 0x0324, 0x0324,    /* vertical timings */
     0, 0, 0, 0, 0, 0,          /* No even timings      */
     (78L << 16) | ((7500L * 65536L) / 10000L), /* freq = 78.75 MHz     */
     }
    ,

    {VG_SUPPORTFLAG_75HZ |      /* refresh rate = 75    */
     VG_SUPPORTFLAG_8BPP | VG_SUPPORTFLAG_12BPP | VG_SUPPORTFLAG_15BPP
     | VG_SUPPORTFLAG_16BPP | VG_SUPPORTFLAG_24BPP | VG_SUPPORTFLAG_32BPP,
     VG_MODEFLAG_AVG_BAND |     /* Avg bandwidth mode.  */
     0,
     1024, 768,                 /* No scaling.          */
     0, 0,                      /* No mode dimensions.  */
     0, 0,                      /* No panel dimensions. */
     0, 0, 0, 0, 0,             /* No panel registers   */
     0x0400, 0x0400, 0x0410, 0x0470, 0x0520, 0x0520,    /* horiz timings   */
     0x0300, 0x0300, 0x0301, 0x0304, 0x0320, 0x0320,    /* vertical timings */
     0, 0, 0, 0, 0, 0,          /* No even timings      */
     (78L << 16) | ((7500L * 65536L) / 10000L), /* freq = 78.75 MHz     */
     }
    ,

    {VG_SUPPORTFLAG_85HZ |      /* refresh rate = 85    */
     VG_SUPPORTFLAG_8BPP | VG_SUPPORTFLAG_12BPP | VG_SUPPORTFLAG_15BPP
     | VG_SUPPORTFLAG_16BPP | VG_SUPPORTFLAG_24BPP | VG_SUPPORTFLAG_32BPP,
     VG_MODEFLAG_AVG_BAND |     /* Avg bandwidth mode.  */
     0,
     1024, 768,                 /* No scaling.          */
     0, 0,                      /* No mode dimensions.  */
     0, 0,                      /* No panel dimensions. */
     0, 0, 0, 0, 0,             /* No panel registers   */
     0x0400, 0x0400, 0x0430, 0x0490, 0x0560, 0x0560,    /* horiz timings   */
     0x0300, 0x0300, 0x0301, 0x0304, 0x0328, 0x0328,    /* vertical timings */
     0, 0, 0, 0, 0, 0,          /* No even timings      */
     (94L << 16) | ((5000L * 65536L) / 10000L), /* freq = 94.50 MHz     */
     }
    ,

    {VG_SUPPORTFLAG_90HZ |      /* refresh rate = 90    */
     VG_SUPPORTFLAG_8BPP | VG_SUPPORTFLAG_12BPP | VG_SUPPORTFLAG_15BPP
     | VG_SUPPORTFLAG_16BPP | VG_SUPPORTFLAG_24BPP | VG_SUPPORTFLAG_32BPP,
     VG_MODEFLAG_AVG_BAND |     /* Avg bandwidth mode.  */
     0,
     1024, 768,                 /* No scaling.          */
     0, 0,                      /* No mode dimensions.  */
     0, 0,                      /* No panel dimensions. */
     0, 0, 0, 0, 0,             /* No panel registers   */
     0x0400, 0x0400, 0x0440, 0x04B0, 0x0560, 0x0560,    /* horiz timings   */
     0x0300, 0x0300, 0x0301, 0x0304, 0x0329, 0x0329,    /* vertical timings */
     0, 0, 0, 0, 0, 0,          /* No even timings      */
     (100L << 16) | ((1870L * 65536L) / 10000L),        /* freq = 100.187 MHz   */
     }
    ,

    {VG_SUPPORTFLAG_100HZ |     /* refresh rate = 100   */
     VG_SUPPORTFLAG_8BPP | VG_SUPPORTFLAG_12BPP | VG_SUPPORTFLAG_15BPP
     | VG_SUPPORTFLAG_16BPP | VG_SUPPORTFLAG_24BPP | VG_SUPPORTFLAG_32BPP,
     VG_MODEFLAG_AVG_BAND |     /* Avg bandwidth mode.  */
     0,
     1024, 768,                 /* No scaling.          */
     0, 0,                      /* No mode dimensions.  */
     0, 0,                      /* No panel dimensions. */
     0, 0, 0, 0, 0,             /* No panel registers   */
     0x0400, 0x0400, 0x0448, 0x04B8, 0x0570, 0x0570,    /* horiz timings   */
     0x0300, 0x0300, 0x0301, 0x0304, 0x032E, 0x032E,    /* vertical timings */
     0, 0, 0, 0, 0, 0,          /* No even timings      */
     (113L << 16) | ((3100L * 65536L) / 10000L),        /* freq = 113.31 MHz    */
     }
    ,

    /* 1024x768 PANEL */

    {VG_SUPPORTFLAG_60HZ |      /* refresh rate = 60    */
     VG_SUPPORTFLAG_8BPP | VG_SUPPORTFLAG_12BPP | VG_SUPPORTFLAG_15BPP | VG_SUPPORTFLAG_16BPP | VG_SUPPORTFLAG_24BPP | VG_SUPPORTFLAG_32BPP | VG_SUPPORTFLAG_PANEL,     /* Panel Mode.          */
     VG_MODEFLAG_AVG_BAND |     /* Avg bandwidth mode.  */
     VG_MODEFLAG_PANELOUT |     /* Panel output.        */
     VG_MODEFLAG_NEG_HSYNC | VG_MODEFLAG_NEG_VSYNC,     /* negative syncs */
     1024, 768,                 /* No scaling.          */
     1024, 768,                 /* 1024x768 active.     */
     1024, 768,                 /* 1024x768 panel       */
     0, 0, 0, 0, 0,             /* No panel registers   */
     0x0400, 0x0400, 0x0418, 0x04A0, 0x0540, 0x0540,    /* horiz timings   */
     0x0300, 0x0300, 0x0303, 0x0309, 0x0326, 0x0326,    /* vertical timings */
     0, 0, 0, 0, 0, 0,          /* No even timings      */
     (65L << 16) | ((0000L * 65536L) / 10000L), /* freq = 65.00 MHz     */
     }
    ,

    /* 1152x864 */

    {VG_SUPPORTFLAG_60HZ |      /* refresh rate = 60    */
     VG_SUPPORTFLAG_8BPP | VG_SUPPORTFLAG_12BPP | VG_SUPPORTFLAG_15BPP
     | VG_SUPPORTFLAG_16BPP | VG_SUPPORTFLAG_24BPP | VG_SUPPORTFLAG_32BPP,
     VG_MODEFLAG_AVG_BAND |     /* Avg bandwidth mode.  */
     0,
     1152, 864,                 /* No scaling.          */
     0, 0,                      /* No mode dimensions.  */
     0, 0,                      /* No panel dimensions. */
     0, 0, 0, 0, 0,             /* No panel registers   */
     0x0480, 0x0480, 0x04C0, 0x0538, 0x05F0, 0x05F0,    /* horiz timings   */
     0x0360, 0x0360, 0x0361, 0x0364, 0x037F, 0x037F,    /* vertical timings */
     0, 0, 0, 0, 0, 0,          /* No even timings      */
     (81L << 16) | ((6000L * 65536L) / 10000L), /* freq = 81.60 MHz     */
     }
    ,

    {VG_SUPPORTFLAG_70HZ |      /* refresh rate = 70    */
     VG_SUPPORTFLAG_8BPP | VG_SUPPORTFLAG_12BPP | VG_SUPPORTFLAG_15BPP
     | VG_SUPPORTFLAG_16BPP | VG_SUPPORTFLAG_24BPP | VG_SUPPORTFLAG_32BPP,
     VG_MODEFLAG_AVG_BAND |     /* Avg bandwidth mode.  */
     0,
     1152, 864,                 /* No scaling.          */
     0, 0,                      /* No mode dimensions.  */
     0, 0,                      /* No panel dimensions. */
     0, 0, 0, 0, 0,             /* No panel registers   */
     0x0480, 0x0480, 0x04C8, 0x0540, 0x0600, 0x0600,    /* horiz timings   */
     0x0360, 0x0360, 0x0368, 0x036B, 0x038B, 0x038B,    /* vertical timings */
     0, 0, 0, 0, 0, 0,          /* No even timings      */
     (97L << 16) | ((5200L * 65536L) / 10000L), /* freq = 97.52 MHz     */
     }
    ,

    {VG_SUPPORTFLAG_72HZ |      /* refresh rate = 72    */
     VG_SUPPORTFLAG_8BPP | VG_SUPPORTFLAG_12BPP | VG_SUPPORTFLAG_15BPP
     | VG_SUPPORTFLAG_16BPP | VG_SUPPORTFLAG_24BPP | VG_SUPPORTFLAG_32BPP,
     VG_MODEFLAG_AVG_BAND |     /* Avg bandwidth mode.  */
     0,
     1152, 864,                 /* No scaling.          */
     0, 0,                      /* No mode dimensions.  */
     0, 0,                      /* No panel dimensions. */
     0, 0, 0, 0, 0,             /* No panel registers   */
     0x0480, 0x0480, 0x04C8, 0x0548, 0x0610, 0x0610,    /* horiz timings   */
     0x0360, 0x0360, 0x0367, 0x036A, 0x038B, 0x038B,    /* vertical timings */
     0, 0, 0, 0, 0, 0,          /* No even timings      */
     (101L << 16) | ((4200L * 65536L) / 10000L),        /* freq = 101.42 MHz    */
     }
    ,

    {VG_SUPPORTFLAG_75HZ |      /* refresh rate = 75    */
     VG_SUPPORTFLAG_8BPP | VG_SUPPORTFLAG_12BPP | VG_SUPPORTFLAG_15BPP
     | VG_SUPPORTFLAG_16BPP | VG_SUPPORTFLAG_24BPP | VG_SUPPORTFLAG_32BPP,
     VG_MODEFLAG_AVG_BAND |     /* Avg bandwidth mode.  */
     0,
     1152, 864,                 /* No scaling.          */
     0, 0,                      /* No mode dimensions.  */
     0, 0,                      /* No panel dimensions. */
     0, 0, 0, 0, 0,             /* No panel registers   */
     0x0480, 0x0480, 0x04C0, 0x0540, 0x0640, 0x0640,    /* horiz timings   */
     0x0360, 0x0360, 0x0361, 0x0364, 0x0384, 0x0384,    /* vertical timings */
     0, 0, 0, 0, 0, 0,          /* No even timings      */
     (108L << 16) | ((0000L * 65536L) / 10000L),        /* freq = 108.00 MHz    */
     }
    ,

    {VG_SUPPORTFLAG_85HZ |      /* refresh rate = 85    */
     VG_SUPPORTFLAG_8BPP | VG_SUPPORTFLAG_12BPP | VG_SUPPORTFLAG_15BPP
     | VG_SUPPORTFLAG_16BPP | VG_SUPPORTFLAG_24BPP | VG_SUPPORTFLAG_32BPP,
     VG_MODEFLAG_AVG_BAND |     /* Avg bandwidth mode.  */
     0,
     1152, 864,                 /* No scaling.          */
     0, 0,                      /* No mode dimensions.  */
     0, 0,                      /* No panel dimensions. */
     0, 0, 0, 0, 0,             /* No panel registers   */
     0x0480, 0x0480, 0x04C8, 0x0548, 0x0610, 0x0610,    /* horiz timings   */
     0x0360, 0x0360, 0x0363, 0x0366, 0x038B, 0x038B,    /* vertical timings */
     0, 0, 0, 0, 0, 0,          /* No even timings      */
     (119L << 16) | ((6500L * 65536L) / 10000L),        /* freq = 119.65 MHz    */
     }
    ,

    {VG_SUPPORTFLAG_90HZ |      /* refresh rate = 90    */
     VG_SUPPORTFLAG_8BPP | VG_SUPPORTFLAG_12BPP | VG_SUPPORTFLAG_15BPP
     | VG_SUPPORTFLAG_16BPP | VG_SUPPORTFLAG_24BPP | VG_SUPPORTFLAG_32BPP,
     VG_MODEFLAG_AVG_BAND |     /* Avg bandwidth mode.  */
     0,
     1152, 864,                 /* No scaling.          */
     0, 0,                      /* No mode dimensions.  */
     0, 0,                      /* No panel dimensions. */
     0, 0, 0, 0, 0,             /* No panel registers   */
     0x0480, 0x0480, 0x04D0, 0x0550, 0x0620, 0x0620,    /* horiz timings   */
     0x0360, 0x0360, 0x0369, 0x036C, 0x0396, 0x0396,    /* vertical timings */
     0, 0, 0, 0, 0, 0,          /* No even timings      */
     (129L << 16) | ((6000L * 65536L) / 10000L),        /* freq = 129.60 MHz    */
     }
    ,

    {VG_SUPPORTFLAG_100HZ |     /* refresh rate = 100   */
     VG_SUPPORTFLAG_8BPP | VG_SUPPORTFLAG_12BPP | VG_SUPPORTFLAG_15BPP
     | VG_SUPPORTFLAG_16BPP | VG_SUPPORTFLAG_24BPP | VG_SUPPORTFLAG_32BPP,
     VG_MODEFLAG_AVG_BAND |     /* Avg bandwidth mode.  */
     0,
     1152, 864,                 /* No scaling.          */
     0, 0,                      /* No mode dimensions.  */
     0, 0,                      /* No panel dimensions. */
     0, 0, 0, 0, 0,             /* No panel registers   */
     0x0480, 0x0480, 0x04D0, 0x0550, 0x0620, 0x0620,    /* horiz timings   */
     0x0360, 0x0360, 0x0363, 0x0366, 0x0396, 0x0396,    /* vertical timings */
     0, 0, 0, 0, 0, 0,          /* No even timings      */
     (144L << 16) | ((0000L * 65536L) / 10000L),        /* freq = 144.00 MHz    */
     }
    ,

    /* 1152x864 PANEL */

    {VG_SUPPORTFLAG_60HZ |      /* refresh rate = 60    */
     VG_SUPPORTFLAG_8BPP | VG_SUPPORTFLAG_12BPP | VG_SUPPORTFLAG_15BPP | VG_SUPPORTFLAG_16BPP | VG_SUPPORTFLAG_24BPP | VG_SUPPORTFLAG_32BPP | VG_SUPPORTFLAG_PANEL,     /* Panel Mode.          */
     VG_MODEFLAG_AVG_BAND |     /* Avg bandwidth mode.  */
     VG_MODEFLAG_PANELOUT |     /* Panel output.        */
     VG_MODEFLAG_NEG_HSYNC | VG_MODEFLAG_NEG_VSYNC,     /* negative syncs */
     1152, 864,                 /* No scaling.          */
     1152, 864,                 /* 1152x864 active.     */
     1152, 864,                 /* 1152x864 panel. */
     0, 0, 0, 0, 0,             /* No panel registers   */
     0x0480, 0x0480, 0x04C0, 0x0538, 0x05F0, 0x05F0,    /* horiz timings   */
     0x0360, 0x0360, 0x0361, 0x0364, 0x037F, 0x037F,    /* vertical timings */
     0, 0, 0, 0, 0, 0,          /* No even timings      */
     (81L << 16) | ((6000L * 65536L) / 10000L), /* freq = 81.60 MHz     */
     }
    ,

    /* 1280x1024 */

    {VG_SUPPORTFLAG_60HZ |      /* refresh rate = 60    */
     VG_SUPPORTFLAG_8BPP | VG_SUPPORTFLAG_12BPP | VG_SUPPORTFLAG_15BPP
     | VG_SUPPORTFLAG_16BPP | VG_SUPPORTFLAG_24BPP | VG_SUPPORTFLAG_32BPP,
     VG_MODEFLAG_AVG_BAND |     /* Avg bandwidth mode.  */
     0,
     1280, 1024,                /* No scaling.          */
     0, 0,                      /* No mode dimensions.  */
     0, 0,                      /* No panel dimensions. */
     0, 0, 0, 0, 0,             /* No panel registers   */
     0x0500, 0x0500, 0x0530, 0x05A0, 0x0698, 0x0698,    /* horiz timings   */
     0x0400, 0x0400, 0x0401, 0x0404, 0x042A, 0x042A,    /* vertical timings */
     0, 0, 0, 0, 0, 0,          /* No even timings      */
     (108L << 16) | ((0000L * 65536L) / 10000L),        /* freq = 108.00 MHz    */
     }
    ,

    {VG_SUPPORTFLAG_70HZ |      /* refresh rate = 70    */
     VG_SUPPORTFLAG_8BPP | VG_SUPPORTFLAG_12BPP | VG_SUPPORTFLAG_15BPP
     | VG_SUPPORTFLAG_16BPP | VG_SUPPORTFLAG_24BPP | VG_SUPPORTFLAG_32BPP,
     VG_MODEFLAG_AVG_BAND |     /* Avg bandwidth mode.  */
     0,
     1280, 1024,                /* No scaling.          */
     0, 0,                      /* No mode dimensions.  */
     0, 0,                      /* No panel dimensions. */
     0, 0, 0, 0, 0,             /* No panel registers   */
     0x0500, 0x0500, 0x0558, 0x05E0, 0x06C0, 0x06C0,    /* horiz timings   */
     0x0400, 0x0400, 0x0406, 0x0409, 0x042F, 0x042F,    /* vertical timings */
     0, 0, 0, 0, 0, 0,          /* No even timings      */
     (129L << 16) | ((6000L * 65536L) / 10000L),        /* freq = 129.60 MHz    */
     }
    ,

    {VG_SUPPORTFLAG_72HZ |      /* refresh rate = 72    */
     VG_SUPPORTFLAG_8BPP | VG_SUPPORTFLAG_12BPP | VG_SUPPORTFLAG_15BPP
     | VG_SUPPORTFLAG_16BPP | VG_SUPPORTFLAG_24BPP | VG_SUPPORTFLAG_32BPP,
     VG_MODEFLAG_AVG_BAND |     /* Avg bandwidth mode.  */
     0,
     1280, 1024,                /* No scaling.          */
     0, 0,                      /* No mode dimensions.  */
     0, 0,                      /* No panel dimensions. */
     0, 0, 0, 0, 0,             /* No panel registers   */
     0x0500, 0x0500, 0x0558, 0x05E0, 0x06C0, 0x06C0,    /* horiz timings   */
     0x0400, 0x0400, 0x0407, 0x040A, 0x0431, 0x0431,    /* vertical timings */
     0, 0, 0, 0, 0, 0,          /* No even timings      */
     (133L << 16) | ((5000L * 65536L) / 10000L),        /* freq = 133.50 MHz    */
     }
    ,

    {VG_SUPPORTFLAG_75HZ |      /* refresh rate = 75    */
     VG_SUPPORTFLAG_8BPP | VG_SUPPORTFLAG_12BPP | VG_SUPPORTFLAG_15BPP
     | VG_SUPPORTFLAG_16BPP | VG_SUPPORTFLAG_24BPP | VG_SUPPORTFLAG_32BPP,
     VG_MODEFLAG_AVG_BAND |     /* Avg bandwidth mode.  */
     0,
     1280, 1024,                /* No scaling.          */
     0, 0,                      /* No mode dimensions.  */
     0, 0,                      /* No panel dimensions. */
     0, 0, 0, 0, 0,             /* No panel registers   */
     0x0500, 0x0500, 0x0510, 0x05A0, 0x0698, 0x0698,    /* horiz timings   */
     0x0400, 0x0400, 0x0401, 0x0404, 0x042A, 0x042A,    /* vertical timings */
     0, 0, 0, 0, 0, 0,          /* No even timings      */
     (135L << 16) | ((0000L * 65536L) / 10000L),        /* freq = 135.0 MHz     */
     }
    ,

    {VG_SUPPORTFLAG_85HZ |      /* refresh rate = 85    */
     VG_SUPPORTFLAG_8BPP | VG_SUPPORTFLAG_12BPP | VG_SUPPORTFLAG_15BPP
     | VG_SUPPORTFLAG_16BPP | VG_SUPPORTFLAG_24BPP | VG_SUPPORTFLAG_32BPP,
     VG_MODEFLAG_AVG_BAND |     /* Avg bandwidth mode.  */
     0,
     1280, 1024,                /* No scaling.          */
     0, 0,                      /* No mode dimensions.  */
     0, 0,                      /* No panel dimensions. */
     0, 0, 0, 0, 0,             /* No panel registers   */
     0x0500, 0x0500, 0x0540, 0x05E0, 0x06C0, 0x06C0,    /* horiz timings   */
     0x0400, 0x0400, 0x0401, 0x0404, 0x0430, 0x0430,    /* vertical timings */
     0, 0, 0, 0, 0, 0,          /* No even timings      */
     (157L << 16) | ((5000L * 65536L) / 10000L),        /* freq = 157.5 MHz     */
     }
    ,

    {VG_SUPPORTFLAG_90HZ |      /* refresh rate = 90    */
     VG_SUPPORTFLAG_8BPP | VG_SUPPORTFLAG_12BPP | VG_SUPPORTFLAG_15BPP
     | VG_SUPPORTFLAG_16BPP | VG_SUPPORTFLAG_24BPP | VG_SUPPORTFLAG_32BPP,
     VG_MODEFLAG_AVG_BAND |     /* Avg bandwidth mode.  */
     0,
     1280, 1024,                /* No scaling.          */
     0, 0,                      /* No mode dimensions.  */
     0, 0,                      /* No panel dimensions. */
     0, 0, 0, 0, 0,             /* No panel registers   */
     0x0500, 0x0500, 0x0560, 0x05F0, 0x06E0, 0x06E0,    /* horiz timings   */
     0x0400, 0x0400, 0x040C, 0x040F, 0x0442, 0x0442,    /* vertical timings */
     0, 0, 0, 0, 0, 0,          /* No even timings      */
     (172L << 16) | ((8000L * 65536L) / 10000L),        /* freq = 172.80 MHz    */
     }
    ,

    {VG_SUPPORTFLAG_100HZ |     /* refresh rate = 100   */
     VG_SUPPORTFLAG_8BPP | VG_SUPPORTFLAG_12BPP | VG_SUPPORTFLAG_15BPP
     | VG_SUPPORTFLAG_16BPP | VG_SUPPORTFLAG_24BPP | VG_SUPPORTFLAG_32BPP,
     VG_MODEFLAG_AVG_BAND |     /* Avg bandwidth mode.  */
     0,
     1280, 1024,                /* No scaling.          */
     0, 0,                      /* No mode dimensions.  */
     0, 0,                      /* No panel dimensions. */
     0, 0, 0, 0, 0,             /* No panel registers   */
     0x0500, 0x0500, 0x0560, 0x05F0, 0x06E0, 0x06E0,    /* horiz timings   */
     0x0400, 0x0400, 0x0406, 0x0409, 0x0442, 0x0442,    /* vertical timings */
     0, 0, 0, 0, 0, 0,          /* No even timings      */
     (192L << 16) | ((0000L * 65536L) / 10000L),        /* freq = 192.00 MHz    */
     }
    ,

    /* 1280x1024 PANEL */

    {VG_SUPPORTFLAG_60HZ |      /* refresh rate = 60    */
     VG_SUPPORTFLAG_8BPP | VG_SUPPORTFLAG_12BPP | VG_SUPPORTFLAG_15BPP | VG_SUPPORTFLAG_16BPP | VG_SUPPORTFLAG_24BPP | VG_SUPPORTFLAG_32BPP | VG_SUPPORTFLAG_PANEL,     /* Panel Mode.          */
     VG_MODEFLAG_AVG_BAND |     /* Avg bandwidth mode.  */
     VG_MODEFLAG_PANELOUT |     /* Panel output.        */
     VG_MODEFLAG_NEG_HSYNC | VG_MODEFLAG_NEG_VSYNC,     /* negative syncs */
     1280, 1024,                /* No scaling.          */
     1280, 1024,                /* 1280x1024 active.    */
     1280, 1024,                /* 1280x1024 panel      */
     0, 0, 0, 0, 0,             /* No panel registers   */
     0x0500, 0x0500, 0x0530, 0x05A0, 0x0698, 0x0698,    /* horiz timings   */
     0x0400, 0x0400, 0x0401, 0x0404, 0x042A, 0x042A,    /* vertical timings */
     0, 0, 0, 0, 0, 0,          /* No even timings      */
     (108L << 16) | ((0000L * 65536L) / 10000L),        /* freq = 108.00 MHz    */
     }
    ,

    /* 1600 x 1200 */

    {VG_SUPPORTFLAG_60HZ |      /* refresh rate = 60    */
     VG_SUPPORTFLAG_8BPP | VG_SUPPORTFLAG_12BPP | VG_SUPPORTFLAG_15BPP
     | VG_SUPPORTFLAG_16BPP | VG_SUPPORTFLAG_24BPP | VG_SUPPORTFLAG_32BPP,
     VG_MODEFLAG_HIGH_BAND |    /* High bandwidth mode. */
     0,
     1600, 1200,                /* No scaling.          */
     0, 0,                      /* No mode dimensions.  */
     0, 0,                      /* No panel dimensions. */
     0, 0, 0, 0, 0,             /* No panel registers   */
     0x0640, 0x0640, 0x0680, 0x0740, 0x0870, 0x0870,    /* horiz timings   */
     0x04B0, 0x04B0, 0x04B1, 0x04B4, 0x04E2, 0x04E2,    /* vertical timings */
     0, 0, 0, 0, 0, 0,          /* No even timings      */
     (162L << 16) | ((0000L * 65536L) / 10000L),        /* freq = 162.0 MHz     */
     }
    ,

    {VG_SUPPORTFLAG_70HZ |      /* refresh rate = 70    */
     VG_SUPPORTFLAG_8BPP | VG_SUPPORTFLAG_12BPP | VG_SUPPORTFLAG_15BPP
     | VG_SUPPORTFLAG_16BPP | VG_SUPPORTFLAG_24BPP | VG_SUPPORTFLAG_32BPP,
     VG_MODEFLAG_HIGH_BAND |    /* High bandwidth mode. */
     0,
     1600, 1200,                /* No scaling.          */
     0, 0,                      /* No mode dimensions.  */
     0, 0,                      /* No panel dimensions. */
     0, 0, 0, 0, 0,             /* No panel registers   */
     0x0640, 0x0640, 0x0680, 0x0740, 0x0870, 0x0870,    /* horiz timings   */
     0x04B0, 0x04B0, 0x04B1, 0x04B4, 0x04E2, 0x04E2,    /* vertical timings */
     0, 0, 0, 0, 0, 0,          /* No even timings      */
     (189L << 16) | ((0000L * 65536L) / 10000L),        /* freq = 189.0 MHz     */
     }
    ,

    {VG_SUPPORTFLAG_72HZ |      /* refresh rate = 72    */
     VG_SUPPORTFLAG_8BPP | VG_SUPPORTFLAG_12BPP | VG_SUPPORTFLAG_15BPP
     | VG_SUPPORTFLAG_16BPP | VG_SUPPORTFLAG_24BPP | VG_SUPPORTFLAG_32BPP,
     VG_MODEFLAG_HIGH_BAND |    /* High bandwidth mode. */
     0,
     1600, 1200,                /* No scaling.          */
     0, 0,                      /* No mode dimensions.  */
     0, 0,                      /* No panel dimensions. */
     0, 0, 0, 0, 0,             /* No panel registers   */
     0x0640, 0x0640, 0x06B0, 0x0760, 0x0880, 0x0880,    /* horiz timings   */
     0x04B0, 0x04B0, 0x04BD, 0x04C0, 0x04EF, 0x04EF,    /* vertical timings */
     0, 0, 0, 0, 0, 0,          /* No even timings      */
     (198L << 16) | ((0000L * 65536L) / 10000L),        /* freq = 198.0 MHz     */
     }
    ,

    {VG_SUPPORTFLAG_75HZ |      /* refresh rate = 75    */
     VG_SUPPORTFLAG_8BPP | VG_SUPPORTFLAG_12BPP | VG_SUPPORTFLAG_15BPP
     | VG_SUPPORTFLAG_16BPP | VG_SUPPORTFLAG_24BPP | VG_SUPPORTFLAG_32BPP,
     VG_MODEFLAG_HIGH_BAND |    /* High bandwidth mode. */
     0,
     1600, 1200,                /* No scaling.          */
     0, 0,                      /* No mode dimensions.  */
     0, 0,                      /* No panel dimensions. */
     0, 0, 0, 0, 0,             /* No panel registers   */
     0x0640, 0x0640, 0x0680, 0x0740, 0x0870, 0x0870,    /* horiz timings   */
     0x04B0, 0x04B0, 0x04B1, 0x04B4, 0x04E2, 0x04E2,    /* vertical timings */
     0, 0, 0, 0, 0, 0,          /* No even timings      */
     (202L << 16) | ((5000L * 65536L) / 10000L),        /* freq = 202.5 MHz     */
     }
    ,

    {VG_SUPPORTFLAG_85HZ |      /* refresh rate = 85    */
     VG_SUPPORTFLAG_8BPP | VG_SUPPORTFLAG_12BPP | VG_SUPPORTFLAG_15BPP
     | VG_SUPPORTFLAG_16BPP | VG_SUPPORTFLAG_24BPP | VG_SUPPORTFLAG_32BPP,
     VG_MODEFLAG_HIGH_BAND |    /* High bandwidth mode. */
     0,
     1600, 1200,                /* No scaling.          */
     0, 0,                      /* No mode dimensions.  */
     0, 0,                      /* No panel dimensions. */
     0, 0, 0, 0, 0,             /* No panel registers   */
     0x0640, 0x0640, 0x0680, 0x0740, 0x0870, 0x0870,    /* horiz timings   */
     0x04B0, 0x04B0, 0x04B1, 0x04B4, 0x04E2, 0x04E2,    /* vertical timings */
     0, 0, 0, 0, 0, 0,          /* No even timings      */
     (229L << 16) | ((5000L * 65536L) / 10000L),        /* freq = 229.5 MHz     */
     }
    ,

    {VG_SUPPORTFLAG_90HZ |      /* refresh rate = 90    */
     VG_SUPPORTFLAG_8BPP | VG_SUPPORTFLAG_12BPP | VG_SUPPORTFLAG_15BPP
     | VG_SUPPORTFLAG_16BPP | VG_SUPPORTFLAG_24BPP | VG_SUPPORTFLAG_32BPP,
     VG_MODEFLAG_HIGH_BAND |    /* High bandwidth mode. */
     0,
     1600, 1200,                /* No scaling.          */
     0, 0,                      /* No mode dimensions.  */
     0, 0,                      /* No panel dimensions. */
     0, 0, 0, 0, 0,             /* No panel registers   */
     0x0640, 0x0640, 0x06C0, 0x0770, 0x08A0, 0x08A0,    /* horiz timings   */
     0x04B0, 0x04B0, 0x04B1, 0x04B4, 0x04F0, 0x04F0,    /* vertical timings */
     0, 0, 0, 0, 0, 0,          /* No even timings      */
     (251L << 16) | ((1820L * 65536L) / 10000L),        /* freq = 251.182 MHz   */
     }
    ,

    {VG_SUPPORTFLAG_100HZ |     /* refresh rate = 100   */
     VG_SUPPORTFLAG_8BPP | VG_SUPPORTFLAG_12BPP | VG_SUPPORTFLAG_15BPP
     | VG_SUPPORTFLAG_16BPP | VG_SUPPORTFLAG_24BPP | VG_SUPPORTFLAG_32BPP,
     VG_MODEFLAG_HIGH_BAND |    /* High bandwidth mode. */
     0,
     1600, 1200,                /* No scaling.          */
     0, 0,                      /* No mode dimensions.  */
     0, 0,                      /* No panel dimensions. */
     0, 0, 0, 0, 0,             /* No panel registers   */
     0x0640, 0x0640, 0x06C0, 0x0770, 0x08A0, 0x08A0,    /* horiz timings   */
     0x04B0, 0x04B0, 0x04B1, 0x04B4, 0x04F7, 0x04F7,    /* vertical timings */
     0, 0, 0, 0, 0, 0,          /* No even timings      */
     (280L << 16) | ((6400L * 65536L) / 10000L),        /* freq = 280.64 MHz    */
     }
    ,

    /* 1600 x 1200  PANEL */

    {VG_SUPPORTFLAG_60HZ |      /* refresh rate = 60    */
     VG_SUPPORTFLAG_8BPP | VG_SUPPORTFLAG_12BPP | VG_SUPPORTFLAG_15BPP | VG_SUPPORTFLAG_16BPP | VG_SUPPORTFLAG_24BPP | VG_SUPPORTFLAG_32BPP | VG_SUPPORTFLAG_PANEL,     /* Panel Mode.          */
     VG_MODEFLAG_HIGH_BAND |    /* High bandwidth mode. */
     VG_MODEFLAG_PANELOUT |     /* Panel output.        */
     VG_MODEFLAG_NEG_HSYNC | VG_MODEFLAG_NEG_VSYNC,     /* negative syncs */
     1600, 1200,                /* No scaling.          */
     1600, 1200,                /* 1600x1200 Active.    */
     1600, 1200,                /* 1600x1200 Panel.     */
     0, 0, 0, 0, 0,             /* No panel registers   */
     0x0640, 0x0640, 0x0680, 0x0740, 0x0870, 0x0870,    /* horiz timings   */
     0x04B0, 0x04B0, 0x04B1, 0x04B4, 0x04E2, 0x04E2,    /* vertical timings */
     0, 0, 0, 0, 0, 0,          /* No even timings      */
     (162L << 16) | ((0000L * 65536L) / 10000L),        /* freq = 162.0 MHz     */
     }
    ,

    /* 1920x1440 */

    {VG_SUPPORTFLAG_60HZ |      /* refresh rate = 60    */
     VG_SUPPORTFLAG_8BPP | VG_SUPPORTFLAG_12BPP | VG_SUPPORTFLAG_15BPP
     | VG_SUPPORTFLAG_16BPP | VG_SUPPORTFLAG_24BPP | VG_SUPPORTFLAG_32BPP,
     VG_MODEFLAG_HIGH_BAND |    /* High bandwidth mode. */
     0,
     1920, 1440,                /* No scaling.          */
     0, 0,                      /* No mode dimensions.  */
     0, 0,                      /* No panel dimensions. */
     0, 0, 0, 0, 0,             /* No panel registers   */
     0x0780, 0x0780, 0x0800, 0x08D0, 0x0A28, 0x0A28,    /* horiz timings   */
     0x05A0, 0x05A0, 0x05A1, 0x05A4, 0x05DC, 0x05DC,    /* vertical timings */
     0, 0, 0, 0, 0, 0,          /* No even timings      */
     (234L << 16) | ((0000L * 65536L) / 10000L),        /* freq = 234.0 MHz     */
     }
    ,

    {VG_SUPPORTFLAG_70HZ |      /* refresh rate = 70    */
     VG_SUPPORTFLAG_8BPP | VG_SUPPORTFLAG_12BPP | VG_SUPPORTFLAG_15BPP
     | VG_SUPPORTFLAG_16BPP | VG_SUPPORTFLAG_24BPP | VG_SUPPORTFLAG_32BPP,
     VG_MODEFLAG_HIGH_BAND |    /* High bandwidth mode. */
     0,
     1920, 1440,                /* No scaling.          */
     0, 0,                      /* No mode dimensions.  */
     0, 0,                      /* No panel dimensions. */
     0, 0, 0, 0, 0,             /* No panel registers   */
     0x0780, 0x0780, 0x0818, 0x08E8, 0x0A50, 0x0A50,    /* horiz timings   */
     0x05A0, 0x05A0, 0x05A8, 0x05AB, 0x05E2, 0x05E2,    /* vertical timings */
     0, 0, 0, 0, 0, 0,          /* No even timings      */
     (278L << 16) | ((4000L * 65536L) / 10000L),        /* freq = 278.4 MHz     */
     }
    ,

    {VG_SUPPORTFLAG_72HZ |      /* refresh rate = 70    */
     VG_SUPPORTFLAG_8BPP | VG_SUPPORTFLAG_12BPP | VG_SUPPORTFLAG_15BPP
     | VG_SUPPORTFLAG_16BPP | VG_SUPPORTFLAG_24BPP | VG_SUPPORTFLAG_32BPP,
     VG_MODEFLAG_HIGH_BAND |    /* High bandwidth mode. */
     0,
     1920, 1440,                /* No scaling.          */
     0, 0,                      /* No mode dimensions.  */
     0, 0,                      /* No panel dimensions. */
     0, 0, 0, 0, 0,             /* No panel registers   */
     0x0780, 0x0780, 0x0818, 0x08E8, 0x0A50, 0x0A50,    /* horiz timings   */
     0x05A0, 0x05A0, 0x05A4, 0x05A7, 0x05EB, 0x05EB,    /* vertical timings */
     0, 0, 0, 0, 0, 0,          /* No even timings      */
     (288L << 16) | ((0000L * 65536L) / 10000L),        /* freq = 288.0 MHz     */
     }
    ,

    {VG_SUPPORTFLAG_75HZ |      /* refresh rate = 75    */
     VG_SUPPORTFLAG_8BPP | VG_SUPPORTFLAG_12BPP | VG_SUPPORTFLAG_15BPP
     | VG_SUPPORTFLAG_16BPP | VG_SUPPORTFLAG_24BPP | VG_SUPPORTFLAG_32BPP,
     VG_MODEFLAG_HIGH_BAND |    /* High bandwidth mode. */
     0,
     1920, 1440,                /* No scaling.          */
     0, 0,                      /* No mode dimensions.  */
     0, 0,                      /* No panel dimensions. */
     0, 0, 0, 0, 0,             /* No panel registers   */
     0x0780, 0x0780, 0x0810, 0x08F0, 0x0A50, 0x0A50,    /* horiz timings   */
     0x05A0, 0x05A0, 0x05A1, 0x05A4, 0x05DC, 0x05DC,    /* vertical timings */
     0, 0, 0, 0, 0, 0,          /* No even timings      */
     (297L << 16) | ((0000L * 65536L) / 10000L),        /* freq = 297.0 MHz     */
     }
    ,

    {VG_SUPPORTFLAG_85HZ |      /* refresh rate = 85    */
     VG_SUPPORTFLAG_8BPP | VG_SUPPORTFLAG_12BPP | VG_SUPPORTFLAG_15BPP
     | VG_SUPPORTFLAG_16BPP | VG_SUPPORTFLAG_24BPP | VG_SUPPORTFLAG_32BPP,
     VG_MODEFLAG_HIGH_BAND |    /* High bandwidth mode. */
     0,
     1920, 1440,                /* No scaling.          */
     0, 0,                      /* No mode dimensions.  */
     0, 0,                      /* No panel dimensions. */
     0, 0, 0, 0, 0,             /* No panel registers   */
     0x0780, 0x0780, 0x0818, 0x08F0, 0x0A60, 0x0A60,    /* horiz timings   */
     0x05A0, 0x05A0, 0x05A1, 0x05A4, 0x05E8, 0x05E8,    /* vertical timings */
     0, 0, 0, 0, 0, 0,          /* No even timings      */
     (341L << 16) | ((3490L * 65536L) / 10000L),        /* freq = 341.35 MHz    */
     }
    ,

/*-------------------------------*/
/*     PREDEFINED TV TIMINGS     */
/*-------------------------------*/

    /* 720 x 480i NTSC */

    {VG_SUPPORTFLAG_60HZ |      /* refresh rate = 60    */
     VG_SUPPORTFLAG_8BPP | VG_SUPPORTFLAG_12BPP | VG_SUPPORTFLAG_15BPP | VG_SUPPORTFLAG_16BPP | VG_SUPPORTFLAG_24BPP | VG_SUPPORTFLAG_32BPP | VG_SUPPORTFLAG_NTSC |     /* NTSC Mode.           */
     VG_SUPPORTFLAG_ADV7171 | VG_SUPPORTFLAG_SAA7127 |
     VG_SUPPORTFLAG_ADV7300 | VG_SUPPORTFLAG_TVOUT,
     VG_MODEFLAG_LOW_BAND |     /* Low bandwidth mode.  */
     VG_MODEFLAG_INTERLACED | VG_MODEFLAG_TVOUT |       /* Interlaced TV output */
     VG_MODEFLAG_HALFCLOCK |    /* DotPLL = 1/2 VOP     */
     VG_MODEFLAG_INT_FLICKER,   /* Flicker Filter Out   */
     720, 480,                  /* No downscaling.      */
     0, 0,                      /* No mode dimensions.  */
     0, 0,                      /* No panel dimensions. */
     0, 0, 0, 0, 0,             /* No panel registers   */
     0x02D0, 0x02D0, 0x02E1, 0x0320, 0x035A, 0x035A,    /* horiz timings   */
     0x00F0, 0x00F0, 0x00F4, 0x00F8, 0x0107, 0x0107,    /* vertical timings */
     0x00F0, 0x00F0, 0x00F4, 0x00F8, 0x0106, 0x0106,    /*Even field timings */
     (27L << 16) | ((0000L * 65536L) / 10000L), /* freq = 27.0 MHz      */
     }
    ,

    {VG_SUPPORTFLAG_60HZ |      /* refresh rate = 60    */
     VG_SUPPORTFLAG_8BPP | VG_SUPPORTFLAG_12BPP | VG_SUPPORTFLAG_15BPP | VG_SUPPORTFLAG_16BPP | VG_SUPPORTFLAG_24BPP | VG_SUPPORTFLAG_32BPP | VG_SUPPORTFLAG_NTSC |     /* NTSC Mode.           */
     VG_SUPPORTFLAG_FS454 | VG_SUPPORTFLAG_TVOUT,       /* TV Mode. */
     VG_MODEFLAG_LOW_BAND |     /* Low bandwidth mode.  */
     VG_MODEFLAG_NEG_HSYNC | VG_MODEFLAG_NEG_VSYNC |    /* negative syncs */
     VG_MODEFLAG_PANELOUT | VG_MODEFLAG_INVERT_SHFCLK,  /* TFT Output. */
     640, 480,                  /* No downscaling.      */
     640, 480,                  /* 640x480 active.      */
     640, 480,                  /* 640x480 panel.       */
     0, 0, 0, 0, 0,             /* No panel registers   */
     0x0280, 0x0280, 0x0333, 0x0373, 0x03A8, 0x03A8,    /* horiz timings   */
     0x01E0, 0x01E0, 0x01F5, 0x01F7, 0x020D, 0x020D,    /* vertical timings */
     0, 0, 0, 0, 0, 0,          /* No even timings.     */
     (25L << 16) | ((1750L * 65536L) / 10000L), /* freq = 25.175 MHz    */
     }
    ,

    /* 800 x 600 NTSC */

    {VG_SUPPORTFLAG_60HZ |      /* refresh rate = 60    */
     VG_SUPPORTFLAG_8BPP | VG_SUPPORTFLAG_12BPP | VG_SUPPORTFLAG_15BPP | VG_SUPPORTFLAG_16BPP | VG_SUPPORTFLAG_24BPP | VG_SUPPORTFLAG_32BPP | VG_SUPPORTFLAG_8X6_NTSC | /* 800x600 NTSC.        */
     VG_SUPPORTFLAG_FS454 | VG_SUPPORTFLAG_TVOUT,       /* TV Mode. */
     VG_MODEFLAG_LOW_BAND |     /* Low bandwidth mode.  */
     VG_MODEFLAG_NEG_HSYNC | VG_MODEFLAG_NEG_VSYNC |    /* negative syncs */
     VG_MODEFLAG_PANELOUT | VG_MODEFLAG_INVERT_SHFCLK,  /* Panel output */
     800, 600,                  /* No downscaling.      */
     800, 600,                  /* 800x600 active.      */
     800, 600,                  /* 800x600 active.      */
     0, 0, 0, 0, 0,             /* No panel registers   */
     0x0320, 0x0320, 0x03A7, 0x03E7, 0x03F0, 0x03F0,    /* horiz timings   */
     0x0258, 0x0258, 0x026A, 0x0272, 0x028A, 0x028A,    /* vertical timings */
     0, 0, 0, 0, 0, 0,          /* No even timings.     */
     (40L << 16) | ((0000L * 65536L) / 10000L), /* freq = 40.000 MHz    */
     }
    ,

    /* 1024 x 768 NTSC */

    {VG_SUPPORTFLAG_60HZ |      /* refresh rate = 60    */
     VG_SUPPORTFLAG_8BPP | VG_SUPPORTFLAG_12BPP | VG_SUPPORTFLAG_15BPP | VG_SUPPORTFLAG_16BPP | VG_SUPPORTFLAG_24BPP | VG_SUPPORTFLAG_32BPP | VG_SUPPORTFLAG_10X7_NTSC |        /* 1024x768 NTSC.       */
     VG_SUPPORTFLAG_FS454 | VG_SUPPORTFLAG_TVOUT,       /* TV Mode. */
     VG_MODEFLAG_AVG_BAND |     /* Low bandwidth mode.  */
     VG_MODEFLAG_NEG_HSYNC | VG_MODEFLAG_NEG_VSYNC |    /* negative syncs */
     VG_MODEFLAG_PANELOUT | VG_MODEFLAG_INVERT_SHFCLK,  /* Panel output */
     1024, 768,                 /* No downscaling.      */
     1024, 768,                 /* 1024x768 active.     */
     1024, 768,                 /* 1024x768 active.     */
     0, 0, 0, 0, 0,             /* No panel registers   */
     0x0400, 0x0400, 0x0490, 0x04D0, 0x04E0, 0x04E0,    /* horiz timings   */
     0x0300, 0x0300, 0x031B, 0x031D, 0x0339, 0x0339,    /* vertical timings */
     0, 0, 0, 0, 0, 0,          /* No even timings.     */
     (65L << 16) | ((0000L * 65536L) / 10000L), /* freq = 65.00 MHz     */
     }
    ,

    /* 720 x 576i PAL */

    {VG_SUPPORTFLAG_8BPP | VG_SUPPORTFLAG_12BPP | VG_SUPPORTFLAG_15BPP | VG_SUPPORTFLAG_16BPP | VG_SUPPORTFLAG_24BPP | VG_SUPPORTFLAG_32BPP | VG_SUPPORTFLAG_PAL |      /* PAL Mode.            */
     VG_SUPPORTFLAG_ADV7171 | VG_SUPPORTFLAG_SAA7127 | VG_SUPPORTFLAG_ADV7300 | VG_SUPPORTFLAG_TVOUT,   /* TV Mode.             */
     VG_MODEFLAG_LOW_BAND |     /* Low bandwidth mode.  */
     VG_MODEFLAG_TVOUT | VG_MODEFLAG_INTERLACED |       /* Interlaced TV out. */
     VG_MODEFLAG_HALFCLOCK |    /* DotPLL = 1/2 VOP     */
     VG_MODEFLAG_INT_FLICKER,   /* Flicker Filter Out   */
     720, 576,                  /* No downscaling.      */
     0, 0,                      /* No mode dimensions.  */
     0, 0,                      /* No panel dimensions. */
     0, 0, 0, 0, 0,             /* No panel registers   */
     0x02D0, 0x02D0, 0x02E4, 0x0324, 0x0360, 0x0360,    /* horiz timings   */
     0x0120, 0x0120, 0x0123, 0x0127, 0x0139, 0x0139,    /* vertical timings */
     0x0120, 0x0120, 0x0123, 0x0127, 0x0138, 0x0138,    /* Even timings */
     (27L << 16) | ((0000L * 65536L) / 10000L), /* freq = 27.0 MHz      */
     }
    ,

    {VG_SUPPORTFLAG_8BPP | VG_SUPPORTFLAG_12BPP | VG_SUPPORTFLAG_15BPP | VG_SUPPORTFLAG_16BPP | VG_SUPPORTFLAG_24BPP | VG_SUPPORTFLAG_32BPP | VG_SUPPORTFLAG_PAL |      /* PAL Mode.            */
     VG_SUPPORTFLAG_FS454 | VG_SUPPORTFLAG_TVOUT,       /* TV Mode. */
     VG_MODEFLAG_LOW_BAND |     /* Low bandwidth mode.  */
     VG_MODEFLAG_NEG_HSYNC | VG_MODEFLAG_NEG_VSYNC |    /* negative syncs */
     VG_MODEFLAG_PANELOUT | VG_MODEFLAG_INVERT_SHFCLK,  /* Panel output */
     640, 480,                  /* No downscaling.      */
     640, 480,                  /* No mode dimensions.  */
     640, 480,                  /* 640x480 active.      */
     0, 0, 0, 0, 0,             /* 640x480 panel.       */
     0x0280, 0x0280, 0x030F, 0x034F, 0x0360, 0x0360,    /* horiz timings   */
     0x01E0, 0x01E0, 0x01F5, 0x01F7, 0x020D, 0x020D,    /* vertical timings */
     0, 0, 0, 0, 0, 0,          /* No even timings.     */
     (25L << 16) | ((1750L * 65536L) / 10000L), /* freq = 25.175 MHz    */
     }
    ,

    /* 800 x 600 PAL */

    {VG_SUPPORTFLAG_60HZ |      /* refresh rate = 60    */
     VG_SUPPORTFLAG_8BPP | VG_SUPPORTFLAG_12BPP | VG_SUPPORTFLAG_15BPP | VG_SUPPORTFLAG_16BPP | VG_SUPPORTFLAG_24BPP | VG_SUPPORTFLAG_32BPP | VG_SUPPORTFLAG_8X6_PAL |  /* 800x600 PAL.         */
     VG_SUPPORTFLAG_FS454 | VG_SUPPORTFLAG_TVOUT,       /* TV Mode. */
     VG_MODEFLAG_LOW_BAND |     /* Low bandwidth mode.  */
     VG_MODEFLAG_NEG_HSYNC | VG_MODEFLAG_NEG_VSYNC |    /* negative syncs */
     VG_MODEFLAG_PANELOUT | VG_MODEFLAG_INVERT_SHFCLK,  /* Panel output */
     800, 600,                  /* No downscaling.      */
     800, 600,                  /* 800x600 active.      */
     800, 600,                  /* 800x600 active.      */
     0, 0, 0, 0, 0,             /* No panel registers   */
     0x0320, 0x0320, 0x03A7, 0x03E7, 0x03F0, 0x03F0,    /* horiz timings   */
     0x0258, 0x0258, 0x0270, 0x0272, 0x028A, 0x028A,    /* vertical timings */
     0, 0, 0, 0, 0, 0,          /* No even timings.     */
     (40L << 16) | ((0000L * 65536L) / 10000L), /* freq = 40.000 MHz    */
     }
    ,

    /* 1024 x 768 PAL */

    {VG_SUPPORTFLAG_60HZ |      /* refresh rate = 60    */
     VG_SUPPORTFLAG_8BPP | VG_SUPPORTFLAG_12BPP | VG_SUPPORTFLAG_15BPP | VG_SUPPORTFLAG_16BPP | VG_SUPPORTFLAG_24BPP | VG_SUPPORTFLAG_32BPP | VG_SUPPORTFLAG_10X7_PAL | /* 1024x768 NTSC.       */
     VG_SUPPORTFLAG_FS454 | VG_SUPPORTFLAG_TVOUT,       /* TV Mode. */
     VG_MODEFLAG_AVG_BAND |     /* Low bandwidth mode.  */
     VG_MODEFLAG_NEG_HSYNC | VG_MODEFLAG_NEG_VSYNC |    /* negative syncs */
     VG_MODEFLAG_PANELOUT | VG_MODEFLAG_INVERT_SHFCLK,  /* Panel output */
     1024, 768,                 /* No downscaling.      */
     1024, 768,                 /* 1024x768 active.     */
     1024, 768,                 /* 1024x768 active.     */
     0, 0, 0, 0, 0,             /* No panel registers   */
     0x0400, 0x0400, 0x0490, 0x04d0, 0x04e0, 0x04e0,    /* horiz timings   */
     0x0300, 0x0300, 0x031b, 0x031d, 0x0339, 0x0339,    /* vertical timings */
     0, 0, 0, 0, 0, 0,          /* No even timings.     */
     (65L << 16) | ((0000L * 65536L) / 10000L), /* freq = 65.00 MHz     */
     }
    ,

    /* 720 x 480p HDTV */

    {VG_SUPPORTFLAG_60HZ |      /* refresh rate = 60    */
     VG_SUPPORTFLAG_8BPP | VG_SUPPORTFLAG_12BPP | VG_SUPPORTFLAG_15BPP | VG_SUPPORTFLAG_16BPP | VG_SUPPORTFLAG_24BPP | VG_SUPPORTFLAG_32BPP | VG_SUPPORTFLAG_480P |     /* 720x480P.            */
     VG_SUPPORTFLAG_ADV7300 | VG_SUPPORTFLAG_TVOUT,     /* TV Mode. */
     VG_MODEFLAG_LOW_BAND |     /* Low bandwidth mode.  */
     VG_MODEFLAG_TVOUT,         /* Progressive TV out.  */
     720, 480,                  /* No scaling.          */
     0, 0,                      /* No mode dimensions.  */
     0, 0,                      /* No panel dimensions. */
     0, 0, 0, 0, 0,             /* No panel registers   */
     0x02D0, 0x02D0, 0x02E0, 0x0328, 0x035A, 0x035A,    /* horiz timings   */
     0x01E0, 0x01E0, 0x01E1, 0x01E3, 0x020D, 0x020D,    /* vertical timings */
     0, 0, 0, 0, 0, 0,          /* No even timings.     */
     (27L << 16) | ((0000L * 65536L) / 10000L), /* freq = 27.0 MHz      */
     }
    ,

    {VG_SUPPORTFLAG_60HZ |      /* refresh rate = 60    */
     VG_SUPPORTFLAG_8BPP | VG_SUPPORTFLAG_12BPP | VG_SUPPORTFLAG_15BPP | VG_SUPPORTFLAG_16BPP | VG_SUPPORTFLAG_24BPP | VG_SUPPORTFLAG_32BPP | VG_SUPPORTFLAG_480P |     /* 720x480P.            */
     VG_SUPPORTFLAG_FS454 | VG_SUPPORTFLAG_TVOUT,       /* TV Mode. */
     VG_MODEFLAG_LOW_BAND |     /* Low bandwidth mode.  */
     VG_MODEFLAG_PANELOUT | VG_MODEFLAG_INVERT_SHFCLK,  /* Panel output */
     720, 480,                  /* No scaling.          */
     720, 480,                  /* 720x480 active.      */
     720, 480,                  /* 720x480 panel.       */
     0, 0, 0, 0, 0,             /* No panel registers   */
     0x02D0, 0x02D0, 0x02E3, 0x0323, 0x035A, 0x035A,    /* horiz timings   */
     0x01E0, 0x01E0, 0x01E4, 0x01EA, 0x020D, 0x020D,    /* vertical timings */
     0, 0, 0, 0, 0, 0,          /* No even timings.     */
     (27L << 16) | ((0000L * 65536L) / 10000L), /* freq = 27.0 MHz      */
     }
    ,

    /* 1280x720p HDTV */

    {VG_SUPPORTFLAG_8BPP | VG_SUPPORTFLAG_12BPP | VG_SUPPORTFLAG_15BPP | VG_SUPPORTFLAG_16BPP | VG_SUPPORTFLAG_24BPP | VG_SUPPORTFLAG_32BPP | VG_SUPPORTFLAG_720P |     /* 1280x720P            */
     VG_SUPPORTFLAG_ADV7300 | VG_SUPPORTFLAG_TVOUT,     /* TV Mode. */
     VG_MODEFLAG_AVG_BAND |     /* Avg bandwidth mode.  */
     VG_MODEFLAG_TVOUT,         /* Progressive TV out   */
     1280, 720,                 /* No scaling.          */
     0, 0,                      /* No mode dimensions.  */
     0, 0,                      /* No panel dimensions. */
     0, 0, 0, 0, 0,             /* No panel registers   */
     0x0500, 0x0500, 0x0546, 0x0596, 0x0672, 0x0672,    /* horiz timings   */
     0x02D0, 0x02D0, 0x02D3, 0x02D8, 0x02EE, 0x02EE,    /* vertical timings */
     0, 0, 0, 0, 0, 0,          /* even timings         */
     (74L << 16) | ((2500L * 65536L) / 10000L), /* freq = 74.25 MHz     */
     }
    ,

    {VG_SUPPORTFLAG_8BPP | VG_SUPPORTFLAG_12BPP | VG_SUPPORTFLAG_15BPP | VG_SUPPORTFLAG_16BPP | VG_SUPPORTFLAG_24BPP | VG_SUPPORTFLAG_32BPP | VG_SUPPORTFLAG_720P |     /* 1280x720P            */
     VG_SUPPORTFLAG_FS454 | VG_SUPPORTFLAG_TVOUT,       /* TV Mode. */
     VG_MODEFLAG_AVG_BAND |     /* Avg bandwidth mode.  */
     VG_MODEFLAG_PANELOUT | VG_MODEFLAG_INVERT_SHFCLK,  /* Panel output */
     1280, 720,                 /* No scaling.          */
     1280, 720,                 /* 1280x720 active.     */
     1280, 720,                 /* 1280x720 panel.      */
     0, 0, 0, 0, 0,             /* No panel registers   */
     0x0500, 0x0500, 0x0547, 0x0597, 0x0672, 0x0672,    /* horiz timings   */
     0x02D0, 0x02D0, 0x02D4, 0x02D9, 0x02EE, 0x02EE,    /* vertical timings */
     0, 0, 0, 0, 0, 0,          /* No even timings.     */
     (74L << 16) | ((2500L * 65536L) / 10000L), /* freq = 74.25 MHz     */
     }
    ,

    /* 1920x1080i HDTV */

    {VG_SUPPORTFLAG_60HZ |      /* refresh rate = 60    */
     VG_SUPPORTFLAG_8BPP | VG_SUPPORTFLAG_12BPP | VG_SUPPORTFLAG_15BPP | VG_SUPPORTFLAG_16BPP | VG_SUPPORTFLAG_24BPP | VG_SUPPORTFLAG_32BPP | VG_SUPPORTFLAG_1080I |    /* 1920x1080i Mode.     */
     VG_SUPPORTFLAG_ADV7300 | VG_SUPPORTFLAG_FS454 | VG_SUPPORTFLAG_TVOUT,
     VG_MODEFLAG_HIGH_BAND |    /* High bandwidth mode. */
     VG_MODEFLAG_INTERLACED | VG_MODEFLAG_TVOUT |       /* Interlaced TV out */
     /* Interlaced addressing */
     VG_MODEFLAG_INT_ADDRESS | VG_MODEFLAG_INVERT_SHFCLK,
     1920, 1080,                /* 2:1 downscaling.     */
     0, 0,                      /* No mode dimensions.  */
     0, 0,                      /* No panel dimensions. */
     0, 0, 0, 0, 0,             /* No panel registers   */
     0x0780, 0x0780, 0x07AD, 0x0805, 0x0898, 0x0898,    /* horiz timings   */
     0x021C, 0x021C, 0x021E, 0x0226, 0x0233, 0x0233,    /* vertical timings */
     0x021C, 0x021C, 0x021E, 0x0226, 0x0232, 0x0232,    /*even field timings */
     (74L << 16) | ((2500L * 65536L) / 10000L), /* freq = 74.25 MHz     */
     }
    ,
};

#define NUM_CIMARRON_DISPLAY_MODES sizeof(CimarronDisplayModes) / \
									sizeof(VG_DISPLAY_MODE)

/*-----------------------------------*/
/*  PREDEFINED PLL FREQUENCIES       */
/*-----------------------------------*/

static const PLL_FREQUENCY CimarronPLLFrequencies[] = {
    {0x000031AC, (24L << 16) | ((9230L * 65536L) / 10000L)}
    ,                           /* 24.9230, - 4,27,13 */
    {0x0000215D, (25L << 16) | ((1750L * 65536L) / 10000L)}
    ,                           /* 25.1750, - 3,22,14 */
    {0x00001087, (27L << 16) | ((0000L * 65536L) / 10000L)}
    ,                           /* 27.0000, - 2, 9, 8 */
    {0x0000216C, (28L << 16) | ((3220L * 65536L) / 10000L)}
    ,                           /* 28.3220, - 3,23,13 */
    {0x0000218D, (28L << 16) | ((5600L * 65536L) / 10000L)}
    ,                           /* 28.5600, - 3,25,14 */
    {0x000010C9, (31L << 16) | ((2000L * 65536L) / 10000L)}
    ,                           /* 31.2000, - 2,13,10 */
    {0x00003147, (31L << 16) | ((5000L * 65536L) / 10000L)}
    ,                           /* 31.5000, - 4,21, 8 */
    {0x000010A7, (33L << 16) | ((320L * 65536L) / 10000L)}
    ,                           /* 33.0320, - 2,11, 8 */
    {0x00002159, (35L << 16) | ((1120L * 65536L) / 10000L)}
    ,                           /* 35.1120, - 3,22,10 */
    {0x00004249, (35L << 16) | ((5000L * 65536L) / 10000L)}
    ,                           /* 35.5000, - 5,37,10 */
    {0x00000057, (36L << 16) | ((0000L * 65536L) / 10000L)}
    ,                           /* 36.0000, - 1, 6, 8 */
    {0x0000219A, (37L << 16) | ((8890L * 65536L) / 10000L)}
    ,                           /* 37.8890, - 3,26,11 */
    {0x00002158, (39L << 16) | ((1680L * 65536L) / 10000L)}
    ,                           /* 39.1680, - 3,22, 9 */
    {0x00000045, (40L << 16) | ((0000L * 65536L) / 10000L)}
    ,                           /* 40.0000, - 1, 5, 6 */
    {0x00000089, (43L << 16) | ((1630L * 65536L) / 10000L)}
    ,                           /* 43.1630, - 1, 9,10 */
    {0x000010E7, (44L << 16) | ((9000L * 65536L) / 10000L)}
    ,                           /* 44.9000, - 2,15, 8 */
    {0x00002136, (45L << 16) | ((7200L * 65536L) / 10000L)}
    ,                           /* 45.7200, - 3,20, 7 */
    {0x00003207, (49L << 16) | ((5000L * 65536L) / 10000L)}
    ,                           /* 49.5000, - 4,33, 8 */
    {0x00002187, (50L << 16) | ((0000L * 65536L) / 10000L)}
    ,                           /* 50.0000, - 3,25, 8 */
    {0x00004286, (56L << 16) | ((2500L * 65536L) / 10000L)}
    ,                           /* 56.2500, - 5,41, 7 */
    {0x000010E5, (60L << 16) | ((650L * 65536L) / 10000L)}
    ,                           /* 60.0650, - 2,15, 6 */
    {0x00004214, (65L << 16) | ((0000L * 65536L) / 10000L)}
    ,                           /* 65.0000, - 5,34, 5 */
    {0x00001105, (68L << 16) | ((1790L * 65536L) / 10000L)}
    ,                           /* 68.1790, - 2,17, 6 */
    {0x000031E4, (74L << 16) | ((2500L * 65536L) / 10000L)}
    ,                           /* 74.2500, - 4,31, 5 */
    {0x00003183, (75L << 16) | ((0000L * 65536L) / 10000L)}
    ,                           /* 75.0000, - 4,25, 4 */
    {0x00004284, (78L << 16) | ((7500L * 65536L) / 10000L)}
    ,                           /* 78.7500, - 5,41, 5 */
    {0x00001104, (81L << 16) | ((6000L * 65536L) / 10000L)}
    ,                           /* 81.6000, - 2,17, 5 */
    {0x00006363, (94L << 16) | ((5000L * 65536L) / 10000L)}
    ,                           /* 94.5000, - 7,55, 4 */
    {0x00005303, (97L << 16) | ((5200L * 65536L) / 10000L)}
    ,                           /* 97.5200, - 6,49, 4 */
    {0x00002183, (100L << 16) | ((1870L * 65536L) / 10000L)}
    ,                           /* 100.187, - 3,25, 4 */
    {0x00002122, (101L << 16) | ((4200L * 65536L) / 10000L)}
    ,                           /* 101.420, - 3,19, 3 */
    {0x000041B1, (106L << 16) | ((5000L * 65536L) / 10000L)}
    ,                           /* 106.50,  - 5,28, 2 */
    {0x00001081, (108L << 16) | ((0000L * 65536L) / 10000L)}
    ,                           /* 108.00,  - 2, 9, 2 */
    {0x00006201, (113L << 16) | ((3100L * 65536L) / 10000L)}
    ,                           /* 113.31,  - 7,33, 2 */
    {0x00000041, (119L << 16) | ((6500L * 65536L) / 10000L)}
    ,                           /* 119.65,  - 1, 5, 2 */
    {0x000041A1, (129L << 16) | ((6000L * 65536L) / 10000L)}
    ,                           /* 129.60,  - 5,27, 2 */
    {0x00002182, (133L << 16) | ((5000L * 65536L) / 10000L)}
    ,                           /* 133.50,  - 3,25, 3 */
    {0x000041B1, (135L << 16) | ((0000L * 65536L) / 10000L)}
    ,                           /* 135.00,  - 5,28, 2 */
    {0x00000051, (144L << 16) | ((0000L * 65536L) / 10000L)}
    ,                           /* 144.00,  - 1, 6, 2 */
    {0x000041E1, (148L << 16) | ((5000L * 65536L) / 10000L)}
    ,                           /* 148.50,  - 5,31, 2 */
    {0x000062D1, (157L << 16) | ((5000L * 65536L) / 10000L)}
    ,                           /* 157.50,  - 7,46, 2 */
    {0x000031A1, (162L << 16) | ((0000L * 65536L) / 10000L)}
    ,                           /* 162.00,  - 4,27, 2 */
    {0x00000061, (169L << 16) | ((2030L * 65536L) / 10000L)}
    ,                           /* 169.203, - 1, 7, 2 */
    {0x00004231, (172L << 16) | ((8000L * 65536L) / 10000L)}
    ,                           /* 172.800, - 5,36, 2 */
    {0x00002151, (175L << 16) | ((5000L * 65536L) / 10000L)}
    ,                           /* 175.50,  - 3,22, 2 */
    {0x000052E1, (189L << 16) | ((0000L * 65536L) / 10000L)}
    ,                           /* 189.00,  - 6,47, 2 */
    {0x00000071, (192L << 16) | ((0000L * 65536L) / 10000L)}
    ,                           /* 192.00,  - 1, 8, 2 */
    {0x00003201, (198L << 16) | ((0000L * 65536L) / 10000L)}
    ,                           /* 198.00,  - 4,33, 2 */
    {0x00004291, (202L << 16) | ((5000L * 65536L) / 10000L)}
    ,                           /* 202.50,  - 5,42, 2 */
    {0x00001101, (204L << 16) | ((7500L * 65536L) / 10000L)}
    ,                           /* 204.75,  - 2,17, 2 */
    {0x00007481, (218L << 16) | ((2500L * 65536L) / 10000L)}
    ,                           /* 218.25,  - 8,73, 2 */
    {0x00004170, (229L << 16) | ((5000L * 65536L) / 10000L)}
    ,                           /* 229.50,  - 5,24, 1 */
    {0x00006210, (234L << 16) | ((0000L * 65536L) / 10000L)}
    ,                           /* 234.00,  - 7,34, 1 */
    {0x00003140, (251L << 16) | ((1820L * 65536L) / 10000L)}
    ,                           /* 251.182, - 4,21, 1 */
    {0x00006250, (261L << 16) | ((0000L * 65536L) / 10000L)}
    ,                           /* 261.00,  - 7,38, 1 */
    {0x000041C0, (278L << 16) | ((4000L * 65536L) / 10000L)}
    ,                           /* 278.40,  - 5,29, 1 */
    {0x00005220, (280L << 16) | ((6400L * 65536L) / 10000L)}
    ,                           /* 280.64,  - 6,35, 1 */
    {0x00000050, (288L << 16) | ((0000L * 65536L) / 10000L)}
    ,                           /* 288.00,  - 1, 6, 1 */
    {0x000041E0, (297L << 16) | ((0000L * 65536L) / 10000L)}
    ,                           /* 297.00,  - 5,31, 1 */
    {0x00002130, (320L << 16) | ((2070L * 65536L) / 10000L)}
    ,                           /* 320.207, - 3,20, 1 */
    {0x00006310, (341L << 16) | ((3490L * 65536L) / 10000L)}
    /* 341.349, - 7,50, 1 */
};

#define NUM_CIMARRON_PLL_FREQUENCIES sizeof(CimarronPLLFrequencies) / \
										sizeof(PLL_FREQUENCY)

/*-----------------------------------*/
/*  PREDEFINED FILTER COEFFICIENTS   */
/*-----------------------------------*/

static const unsigned long CimarronHorizontalGraphicsFilter[][2] = {
    {0x1284A7D5, 0x000017D5},   /*  -43,  297,  296,  -43,    5  */
    {0x12A497D7, 0x000013D6},   /*  -41,  293,  298,  -42,    4  */
    {0x12D48BD7, 0x000013D6},   /*  -41,  290,  301,  -42,    4  */
    {0x13147FD7, 0x000013D5},   /*  -41,  287,  305,  -43,    4  */
    {0x133473D8, 0x000013D5},   /*  -40,  284,  307,  -43,    4  */
    {0x136467D8, 0x000013D5},   /*  -40,  281,  310,  -43,    4  */
    {0x13945FD8, 0x000013D4},   /*  -40,  279,  313,  -44,    4  */
    {0x13B453D9, 0x000013D4},   /*  -39,  276,  315,  -44,    4  */
    {0x13E447D9, 0x000013D4},   /*  -39,  273,  318,  -44,    4  */
    {0x14143BDA, 0x000013D3},   /*  -38,  270,  321,  -45,    4  */
    {0x143433DA, 0x000013D3},   /*  -38,  268,  323,  -45,    4  */
    {0x146427DA, 0x000013D3},   /*  -38,  265,  326,  -45,    4  */
    {0x14941BDB, 0x000013D2},   /*  -37,  262,  329,  -46,    4  */
    {0x14C40FDB, 0x000013D2},   /*  -37,  259,  332,  -46,    4  */
    {0x14F407DA, 0x000017D1},   /*  -38,  257,  335,  -47,    5  */
    {0x1503FBDC, 0x000013D2},   /*  -36,  254,  336,  -46,    4  */
    {0x1543F3DB, 0x000017D0},   /*  -37,  252,  340,  -48,    5  */
    {0x1563E3DD, 0x000013D1},   /*  -35,  248,  342,  -47,    4  */
    {0x1593D7DD, 0x000013D1},   /*  -35,  245,  345,  -47,    4  */
    {0x15B3CFDD, 0x000013D1},   /*  -35,  243,  347,  -47,    4  */
    {0x15E3C3DE, 0x000013D0},   /*  -34,  240,  350,  -48,    4  */
    {0x1613B7DE, 0x000013D0},   /*  -34,  237,  353,  -48,    4  */
    {0x1633ABDF, 0x000013D0},   /*  -33,  234,  355,  -48,    4  */
    {0x16639FDF, 0x000013D0},   /*  -33,  231,  358,  -48,    4  */
    {0x167397E0, 0x000013D0},   /*  -32,  229,  359,  -48,    4  */
    {0x16B38BE0, 0x000013CF},   /*  -32,  226,  363,  -49,    4  */
    {0x16E383DF, 0x000017CE},   /*  -33,  224,  366,  -50,    5  */
    {0x170373E1, 0x000013CF},   /*  -31,  220,  368,  -49,    4  */
    {0x17236BE1, 0x000013CF},   /*  -31,  218,  370,  -49,    4  */
    {0x17435FE2, 0x000013CF},   /*  -30,  215,  372,  -49,    4  */
    {0x177353E2, 0x000013CF},   /*  -30,  212,  375,  -49,    4  */
    {0x17B34BE1, 0x000017CD},   /*  -31,  210,  379,  -51,    5  */
    {0x17C33FE3, 0x000013CE},   /*  -29,  207,  380,  -50,    4  */
    {0x17F333E3, 0x000013CE},   /*  -29,  204,  383,  -50,    4  */
    {0x181327E4, 0x000013CE},   /*  -28,  201,  385,  -50,    4  */
    {0x18431FE3, 0x000017CD},   /*  -29,  199,  388,  -51,    5  */
    {0x186313E4, 0x000013CE},   /*  -28,  196,  390,  -50,    4  */
    {0x188307E5, 0x000013CE},   /*  -27,  193,  392,  -50,    4  */
    {0x18B2FBE5, 0x000013CE},   /*  -27,  190,  395,  -50,    4  */
    {0x18C2F3E6, 0x000013CE},   /*  -26,  188,  396,  -50,    4  */
    {0x18F2E7E6, 0x000013CE},   /*  -26,  185,  399,  -50,    4  */
    {0x1912DBE7, 0x000013CE},   /*  -25,  182,  401,  -50,    4  */
    {0x1952D3E6, 0x000017CC},   /*  -26,  180,  405,  -52,    5  */
    {0x1972CBE6, 0x000017CC},   /*  -26,  178,  407,  -52,    5  */
    {0x1992BFE7, 0x000017CC},   /*  -25,  175,  409,  -52,    5  */
    {0x19C2B3E7, 0x000017CC},   /*  -25,  172,  412,  -52,    5  */
    {0x19D2A7E9, 0x000013CD},   /*  -23,  169,  413,  -51,    4  */
    {0x1A029FE8, 0x000017CC},   /*  -24,  167,  416,  -52,    5  */
    {0x1A1293E9, 0x000013CE},   /*  -23,  164,  417,  -50,    4  */
    {0x1A3287EA, 0x000013CE},   /*  -22,  161,  419,  -50,    4  */
    {0x1A627FE9, 0x000017CD},   /*  -23,  159,  422,  -51,    5  */
    {0x1A7273EB, 0x000013CE},   /*  -21,  156,  423,  -50,    4  */
    {0x1AA267EB, 0x000013CE},   /*  -21,  153,  426,  -50,    4  */
    {0x1AC25FEB, 0x000013CE},   /*  -21,  151,  428,  -50,    4  */
    {0x1AE253EC, 0x000013CE},   /*  -20,  148,  430,  -50,    4  */
    {0x1B124BEB, 0x000017CD},   /*  -21,  146,  433,  -51,    5  */
    {0x1B223FED, 0x000013CE},   /*  -19,  143,  434,  -50,    4  */
    {0x1B5237EC, 0x000017CD},   /*  -20,  141,  437,  -51,    5  */
    {0x1B622BED, 0x000013CF},   /*  -19,  138,  438,  -49,    4  */
    {0x1B821FEE, 0x000013CF},   /*  -18,  135,  440,  -49,    4  */
    {0x1BA217EE, 0x000013CF},   /*  -18,  133,  442,  -49,    4  */
    {0x1BC20BEF, 0x000013CF},   /*  -17,  130,  444,  -49,    4  */
    {0x1BE203EF, 0x000013CF},   /*  -17,  128,  446,  -49,    4  */
    {0x1C01FBEE, 0x000017CF},   /*  -18,  126,  448,  -49,    5  */
    {0x1C11EFF0, 0x000013D0},   /*  -16,  123,  449,  -48,    4  */
    {0x1C41E7EF, 0x000017CF},   /*  -17,  121,  452,  -49,    5  */
    {0x1C61DFEF, 0x000017CF},   /*  -17,  119,  454,  -49,    5  */
    {0x1C61D3F1, 0x000013D1},   /*  -15,  116,  454,  -47,    4  */
    {0x1C91CBF0, 0x000017D0},   /*  -16,  114,  457,  -48,    5  */
    {0x1CA1BFF2, 0x000013D1},   /*  -14,  111,  458,  -47,    4  */
    {0x1CC1B3F2, 0x000013D2},   /*  -14,  108,  460,  -46,    4  */
    {0x1CE1AFF1, 0x000017D1},   /*  -15,  107,  462,  -47,    5  */
    {0x1CF1A3F3, 0x000013D2},   /*  -13,  104,  463,  -46,    4  */
    {0x1D1197F3, 0x000013D3},   /*  -13,  101,  465,  -45,    4  */
    {0x1D3197F2, 0x000013D2},   /*  -14,  101,  467,  -46,    4  */
    {0x1D518BF3, 0x000013D2},   /*  -13,   98,  469,  -46,    4  */
    {0x1D6183F3, 0x000013D3},   /*  -13,   96,  470,  -45,    4  */
    {0x1D817BF3, 0x000013D3},   /*  -13,   94,  472,  -45,    4  */
    {0x1D916FF4, 0x000013D4},   /*  -12,   91,  473,  -44,    4  */
    {0x1DB167F4, 0x000013D4},   /*  -12,   89,  475,  -44,    4  */
    {0x1DC15FF4, 0x000013D5},   /*  -12,   87,  476,  -43,    4  */
    {0x1DE153F5, 0x000013D5},   /*  -11,   84,  478,  -43,    4  */
    {0x1DF14BF5, 0x000013D6},   /*  -11,   82,  479,  -42,    4  */
    {0x1E1143F5, 0x000013D6},   /*  -11,   80,  481,  -42,    4  */
    {0x1E1137F7, 0x00000FD8},   /*   -9,   77,  481,  -40,    3  */
    {0x1E3133F6, 0x000013D7},   /*  -10,   76,  483,  -41,    4  */
    {0x1E412BF6, 0x000013D8},   /*  -10,   74,  484,  -40,    4  */
    {0x1E611FF7, 0x000013D8},   /*   -9,   71,  486,  -40,    4  */
    {0x1E7117F7, 0x000013D9},   /*   -9,   69,  487,  -39,    4  */
    {0x1E810FF7, 0x000013DA},   /*   -9,   67,  488,  -38,    4  */
    {0x1E9107F8, 0x000013DA},   /*   -8,   65,  489,  -38,    4  */
    {0x1EA0FFF8, 0x000013DB},   /*   -8,   63,  490,  -37,    4  */
    {0x1EB0F3F9, 0x00000FDD},   /*   -7,   60,  491,  -35,    3  */
    {0x1ED0EFF8, 0x000013DC},   /*   -8,   59,  493,  -36,    4  */
    {0x1EE0E7F9, 0x00000FDD},   /*   -7,   57,  494,  -35,    3  */
    {0x1EF0DFF9, 0x00000FDE},   /*   -7,   55,  495,  -34,    3  */
    {0x1F00D7F9, 0x00000FDF},   /*   -7,   53,  496,  -33,    3  */
    {0x1F10CFFA, 0x00000FDF},   /*   -6,   51,  497,  -33,    3  */
    {0x1F20C7FA, 0x00000FE0},   /*   -6,   49,  498,  -32,    3  */
    {0x1F20C3FA, 0x00000FE1},   /*   -6,   48,  498,  -31,    3  */
    {0x1F30BBFA, 0x00000FE2},   /*   -6,   46,  499,  -30,    3  */
    {0x1F40AFFB, 0x00000FE3},   /*   -5,   43,  500,  -29,    3  */
    {0x1F50A7FB, 0x00000FE4},   /*   -5,   41,  501,  -28,    3  */
    {0x1F60A3FB, 0x00000FE4},   /*   -5,   40,  502,  -28,    3  */
    {0x1F709BFB, 0x00000FE5},   /*   -5,   38,  503,  -27,    3  */
    {0x1F7093FC, 0x00000FE6},   /*   -4,   36,  503,  -26,    3  */
    {0x1F808FFC, 0x00000BE7},   /*   -4,   35,  504,  -25,    2  */
    {0x1F9087FC, 0x00000BE8},   /*   -4,   33,  505,  -24,    2  */
    {0x1F9083FC, 0x00000BE9},   /*   -4,   32,  505,  -23,    2  */
    {0x1FA077FD, 0x00000BEA},   /*   -3,   29,  506,  -22,    2  */
    {0x1FA073FD, 0x00000BEB},   /*   -3,   28,  506,  -21,    2  */
    {0x1FB06BFD, 0x00000BEC},   /*   -3,   26,  507,  -20,    2  */
    {0x1FC063FD, 0x00000BED},   /*   -3,   24,  508,  -19,    2  */
    {0x1FC05BFE, 0x00000BEE},   /*   -2,   22,  508,  -18,    2  */
    {0x1FC057FE, 0x00000BEF},   /*   -2,   21,  508,  -17,    2  */
    {0x1FD053FE, 0x000007F0},   /*   -2,   20,  509,  -16,    1  */
    {0x1FD04BFE, 0x000007F2},   /*   -2,   18,  509,  -14,    1  */
    {0x1FE043FE, 0x000007F3},   /*   -2,   16,  510,  -13,    1  */
    {0x1FE03BFF, 0x000007F4},   /*   -1,   14,  510,  -12,    1  */
    {0x1FE037FF, 0x000007F5},   /*   -1,   13,  510,  -11,    1  */
    {0x1FE033FF, 0x000007F6},   /*   -1,   12,  510,  -10,    1  */
    {0x1FF02BFF, 0x000007F7},   /*   -1,   10,  511,   -9,    1  */
    {0x1FF027FF, 0x000003F9},   /*   -1,    9,  511,   -7,    0  */
    {0x1FF01C00, 0x000003FA},   /*    0,    7,  511,   -6,    0  */
    {0x1FF01800, 0x000003FB},   /*    0,    6,  511,   -5,    0  */
    {0x1FF01400, 0x000003FC},   /*    0,    5,  511,   -4,    0  */
    {0x1FF00C00, 0x000003FE},   /*    0,    3,  511,   -2,    0  */
    {0x1FF00800, 0x000003FF},   /*    0,    2,  511,   -1,    0  */
    {0x1FF00400, 0x00000000},   /*    0,    1,  511,    0,    0  */
    {0x1FFFFC00, 0x00000002},   /*    0,   -1,  511,    2,    0  */
    {0x1FFFF800, 0x00000003},   /*    0,   -2,  511,    3,    0  */
    {0x1FFFF000, 0x00000005},   /*    0,   -4,  511,    5,    0  */
    {0x1FFFEC00, 0x00000006},   /*    0,   -5,  511,    6,    0  */
    {0x1FFFE800, 0x00000007},   /*    0,   -6,  511,    7,    0  */
    {0x1FFFE400, 0x000FFC09},   /*    0,   -7,  511,    9,   -1  */
    {0x1FFFDC01, 0x000FFC0A},   /*    1,   -9,  511,   10,   -1  */
    {0x1FEFDC01, 0x000FFC0B},   /*    1,   -9,  510,   11,   -1  */
    {0x1FEFD401, 0x000FFC0D},   /*    1,  -11,  510,   13,   -1  */
    {0x1FEFD001, 0x000FFC0E},   /*    1,  -12,  510,   14,   -1  */
    {0x1FEFCC01, 0x000FF810},   /*    1,  -13,  510,   16,   -2  */
    {0x1FDFCC01, 0x000FF811},   /*    1,  -13,  509,   17,   -2  */
    {0x1FDFC401, 0x000FF813},   /*    1,  -15,  509,   19,   -2  */
    {0x1FCFC002, 0x000FF814},   /*    2,  -16,  508,   20,   -2  */
    {0x1FCFB802, 0x000FF816},   /*    2,  -18,  508,   22,   -2  */
    {0x1FCFB402, 0x000FF418},   /*    2,  -19,  508,   24,   -3  */
    {0x1FBFB402, 0x000FF419},   /*    2,  -19,  507,   25,   -3  */
    {0x1FAFB002, 0x000FF41B},   /*    2,  -20,  506,   27,   -3  */
    {0x1FAFA802, 0x000FF41D},   /*    2,  -22,  506,   29,   -3  */
    {0x1F9FA802, 0x000FF01F},   /*    2,  -22,  505,   31,   -4  */
    {0x1F9FA402, 0x000FF020},   /*    2,  -23,  505,   32,   -4  */
    {0x1F8FA002, 0x000FF022},   /*    2,  -24,  504,   34,   -4  */
    {0x1F7F9803, 0x000FF024},   /*    3,  -26,  503,   36,   -4  */
    {0x1F7F9403, 0x000FEC26},   /*    3,  -27,  503,   38,   -5  */
    {0x1F6F9003, 0x000FEC28},   /*    3,  -28,  502,   40,   -5  */
    {0x1F5F9003, 0x000FEC29},   /*    3,  -28,  501,   41,   -5  */
    {0x1F4F8C03, 0x000FEC2B},   /*    3,  -29,  500,   43,   -5  */
    {0x1F3F8C03, 0x000FE82D},   /*    3,  -29,  499,   45,   -6  */
    {0x1F2F8803, 0x000FE82F},   /*    3,  -30,  498,   47,   -6  */
    {0x1F2F8003, 0x000FE831},   /*    3,  -32,  498,   49,   -6  */
    {0x1F1F7C03, 0x000FE833},   /*    3,  -33,  497,   51,   -6  */
    {0x1F0F7C03, 0x000FE435},   /*    3,  -33,  496,   53,   -7  */
    {0x1EFF7803, 0x000FE437},   /*    3,  -34,  495,   55,   -7  */
    {0x1EEF7403, 0x000FE439},   /*    3,  -35,  494,   57,   -7  */
    {0x1EDF7004, 0x000FE03B},   /*    4,  -36,  493,   59,   -8  */
    {0x1EBF7403, 0x000FE43C},   /*    3,  -35,  491,   60,   -7  */
    {0x1EAF6C04, 0x000FE03F},   /*    4,  -37,  490,   63,   -8  */
    {0x1E9F6804, 0x000FE041},   /*    4,  -38,  489,   65,   -8  */
    {0x1E8F6804, 0x000FDC43},   /*    4,  -38,  488,   67,   -9  */
    {0x1E7F6404, 0x000FDC45},   /*    4,  -39,  487,   69,   -9  */
    {0x1E6F6004, 0x000FDC47},   /*    4,  -40,  486,   71,   -9  */
    {0x1E4F6404, 0x000FD849},   /*    4,  -39,  484,   73,  -10  */
    {0x1E3F6004, 0x000FD84B},   /*    4,  -40,  483,   75,  -10  */
    {0x1E1F6003, 0x000FDC4D},   /*    3,  -40,  481,   77,   -9  */
    {0x1E1F5804, 0x000FD450},   /*    4,  -42,  481,   80,  -11  */
    {0x1DFF5804, 0x000FD452},   /*    4,  -42,  479,   82,  -11  */
    {0x1DEF5404, 0x000FD454},   /*    4,  -43,  478,   84,  -11  */
    {0x1DCF5804, 0x000FD056},   /*    4,  -42,  476,   86,  -12  */
    {0x1DBF5004, 0x000FD059},   /*    4,  -44,  475,   89,  -12  */
    {0x1D9F5004, 0x000FD05B},   /*    4,  -44,  473,   91,  -12  */
    {0x1D8F5004, 0x000FCC5D},   /*    4,  -44,  472,   93,  -13  */
    {0x1D6F5004, 0x000FCC5F},   /*    4,  -44,  470,   95,  -13  */
    {0x1D5F4804, 0x000FCC62},   /*    4,  -46,  469,   98,  -13  */
    {0x1D3F4C04, 0x000FC864},   /*    4,  -45,  467,  100,  -14  */
    {0x1D1F4C04, 0x000FCC65},   /*    4,  -45,  465,  101,  -13  */
    {0x1CFF4804, 0x000FCC68},   /*    4,  -46,  463,  104,  -13  */
    {0x1CEF4405, 0x000FC46B},   /*    5,  -47,  462,  107,  -15  */
    {0x1CCF4804, 0x000FC86C},   /*    4,  -46,  460,  108,  -14  */
    {0x1CAF4404, 0x000FC86F},   /*    4,  -47,  458,  111,  -14  */
    {0x1C9F4005, 0x000FC072},   /*    5,  -48,  457,  114,  -16  */
    {0x1C6F4404, 0x000FC474},   /*    4,  -47,  454,  116,  -15  */
    {0x1C6F3C05, 0x000FBC77},   /*    5,  -49,  454,  119,  -17  */
    {0x1C4F3C05, 0x000FBC79},   /*    5,  -49,  452,  121,  -17  */
    {0x1C1F4004, 0x000FC07B},   /*    4,  -48,  449,  123,  -16  */
    {0x1C0F3C05, 0x000FB87E},   /*    5,  -49,  448,  126,  -18  */
    {0x1BEF3C04, 0x000FBC80},   /*    4,  -49,  446,  128,  -17  */
    {0x1BCF3C04, 0x000FBC82},   /*    4,  -49,  444,  130,  -17  */
    {0x1BAF3C04, 0x000FB885},   /*    4,  -49,  442,  133,  -18  */
    {0x1B8F3C04, 0x000FB887},   /*    4,  -49,  440,  135,  -18  */
    {0x1B6F3C04, 0x000FB48A},   /*    4,  -49,  438,  138,  -19  */
    {0x1B5F3405, 0x000FB08D},   /*    5,  -51,  437,  141,  -20  */
    {0x1B2F3804, 0x000FB48F},   /*    4,  -50,  434,  143,  -19  */
    {0x1B1F3405, 0x000FAC92},   /*    5,  -51,  433,  146,  -21  */
    {0x1AEF3804, 0x000FB094},   /*    4,  -50,  430,  148,  -20  */
    {0x1ACF3804, 0x000FAC97},   /*    4,  -50,  428,  151,  -21  */
    {0x1AAF3804, 0x000FAC99},   /*    4,  -50,  426,  153,  -21  */
    {0x1A7F3804, 0x000FAC9C},   /*    4,  -50,  423,  156,  -21  */
    {0x1A6F3405, 0x000FA49F},   /*    5,  -51,  422,  159,  -23  */
    {0x1A3F3804, 0x000FA8A1},   /*    4,  -50,  419,  161,  -22  */
    {0x1A1F3804, 0x000FA4A4},   /*    4,  -50,  417,  164,  -23  */
    {0x1A0F3005, 0x000FA0A7},   /*    5,  -52,  416,  167,  -24  */
    {0x19DF3404, 0x000FA4A9},   /*    4,  -51,  413,  169,  -23  */
    {0x19CF3005, 0x000F9CAC},   /*    5,  -52,  412,  172,  -25  */
    {0x199F3005, 0x000F9CAF},   /*    5,  -52,  409,  175,  -25  */
    {0x197F3005, 0x000F98B2},   /*    5,  -52,  407,  178,  -26  */
    {0x195F3005, 0x000F98B4},   /*    5,  -52,  405,  180,  -26  */
    {0x191F3804, 0x000F9CB6},   /*    4,  -50,  401,  182,  -25  */
    {0x18FF3804, 0x000F98B9},   /*    4,  -50,  399,  185,  -26  */
    {0x18CF3804, 0x000F98BC},   /*    4,  -50,  396,  188,  -26  */
    {0x18BF3804, 0x000F94BE},   /*    4,  -50,  395,  190,  -27  */
    {0x188F3804, 0x000F94C1},   /*    4,  -50,  392,  193,  -27  */
    {0x186F3804, 0x000F90C4},   /*    4,  -50,  390,  196,  -28  */
    {0x184F3405, 0x000F8CC7},   /*    5,  -51,  388,  199,  -29  */
    {0x181F3804, 0x000F90C9},   /*    4,  -50,  385,  201,  -28  */
    {0x17FF3804, 0x000F8CCC},   /*    4,  -50,  383,  204,  -29  */
    {0x17CF3804, 0x000F8CCF},   /*    4,  -50,  380,  207,  -29  */
    {0x17BF3405, 0x000F84D2},   /*    5,  -51,  379,  210,  -31  */
    {0x177F3C04, 0x000F88D4},   /*    4,  -49,  375,  212,  -30  */
    {0x174F3C04, 0x000F88D7},   /*    4,  -49,  372,  215,  -30  */
    {0x172F3C04, 0x000F84DA},   /*    4,  -49,  370,  218,  -31  */
    {0x170F3C04, 0x000F84DC},   /*    4,  -49,  368,  220,  -31  */
    {0x16EF3805, 0x000F7CE0},   /*    5,  -50,  366,  224,  -33  */
    {0x16BF3C04, 0x000F80E2},   /*    4,  -49,  363,  226,  -32  */
    {0x167F4004, 0x000F80E5},   /*    4,  -48,  359,  229,  -32  */
    {0x166F4004, 0x000F7CE7},   /*    4,  -48,  358,  231,  -33  */
    {0x163F4004, 0x000F7CEA},   /*    4,  -48,  355,  234,  -33  */
    {0x161F4004, 0x000F78ED},   /*    4,  -48,  353,  237,  -34  */
    {0x15EF4004, 0x000F78F0},   /*    4,  -48,  350,  240,  -34  */
    {0x15BF4404, 0x000F74F3},   /*    4,  -47,  347,  243,  -35  */
    {0x159F4404, 0x000F74F5},   /*    4,  -47,  345,  245,  -35  */
    {0x156F4404, 0x000F74F8},   /*    4,  -47,  342,  248,  -35  */
    {0x154F4005, 0x000F6CFC},   /*    5,  -48,  340,  252,  -37  */
    {0x150F4804, 0x000F70FE},   /*    4,  -46,  336,  254,  -36  */
    {0x14FF4405, 0x000F6901},   /*    5,  -47,  335,  257,  -38  */
    {0x14CF4804, 0x000F6D03},   /*    4,  -46,  332,  259,  -37  */
    {0x149F4804, 0x000F6D06},   /*    4,  -46,  329,  262,  -37  */
    {0x146F4C04, 0x000F6909},   /*    4,  -45,  326,  265,  -38  */
    {0x143F4C04, 0x000F690C},   /*    4,  -45,  323,  268,  -38  */
    {0x141F4C04, 0x000F690E},   /*    4,  -45,  321,  270,  -38  */
    {0x13EF5004, 0x000F6511},   /*    4,  -44,  318,  273,  -39  */
    {0x13BF5004, 0x000F6514},   /*    4,  -44,  315,  276,  -39  */
    {0x139F5004, 0x000F6117},   /*    4,  -44,  313,  279,  -40  */
    {0x136F5404, 0x000F6119},   /*    4,  -43,  310,  281,  -40  */
    {0x133F5404, 0x000F611C},   /*    4,  -43,  307,  284,  -40  */
    {0x131F5404, 0x000F5D1F},   /*    4,  -43,  305,  287,  -41  */
    {0x12DF5C04, 0x000F5D21},   /*    4,  -41,  301,  289,  -41  */
    {0x12AF5C04, 0x000F5D24},   /*    4,  -41,  298,  292,  -41  */
};

static const unsigned long CimarronVerticalGraphicsFilter[] = {
    0x3F840D05,                 /* 261, 259,  -8  */
    0x3F841D01,                 /* 257, 263,  -8  */
    0x3F8428FE,                 /* 254, 266,  -8  */
    0x3F8438FA,                 /* 250, 270,  -8  */
    0x3F8444F7,                 /* 247, 273,  -8  */
    0x3F8450F4,                 /* 244, 276,  -8  */
    0x3F845CF1,                 /* 241, 279,  -8  */
    0x3F8468EE,                 /* 238, 282,  -8  */
    0x3F8474EB,                 /* 235, 285,  -8  */
    0x3F8480E8,                 /* 232, 288,  -8  */
    0x3F7490E5,                 /* 229, 292,  -9  */
    0x3F749CE2,                 /* 226, 295,  -9  */
    0x3F74ACDE,                 /* 222, 299,  -9  */
    0x3F74B8DB,                 /* 219, 302,  -9  */
    0x3F74C0D9,                 /* 217, 304,  -9  */
    0x3F74CCD6,                 /* 214, 307,  -9  */
    0x3F74D8D3,                 /* 211, 310,  -9  */
    0x3F74E8CF,                 /* 207, 314,  -9  */
    0x3F74F4CC,                 /* 204, 317,  -9  */
    0x3F7500C9,                 /* 201, 320,  -9  */
    0x3F750CC6,                 /* 198, 323,  -9  */
    0x3F7518C3,                 /* 195, 326,  -9  */
    0x3F7520C1,                 /* 193, 328,  -9  */
    0x3F7530BD,                 /* 189, 332,  -9  */
    0x3F753CBA,                 /* 186, 335,  -9  */
    0x3F7548B7,                 /* 183, 338,  -9  */
    0x3F6558B4,                 /* 180, 342, -10  */
    0x3F6560B2,                 /* 178, 344, -10  */
    0x3F656CAF,                 /* 175, 347, -10  */
    0x3F6578AC,                 /* 172, 350, -10  */
    0x3F6584A9,                 /* 169, 353, -10  */
    0x3F658CA7,                 /* 167, 355, -10  */
    0x3F6598A4,                 /* 164, 358, -10  */
    0x3F65A8A0,                 /* 160, 362, -10  */
    0x3F65B09E,                 /* 158, 364, -10  */
    0x3F65BC9B,                 /* 155, 367, -10  */
    0x3F65C499,                 /* 153, 369, -10  */
    0x3F65D096,                 /* 150, 372, -10  */
    0x3F55E093,                 /* 147, 376, -11  */
    0x3F55E891,                 /* 145, 378, -11  */
    0x3F55F48E,                 /* 142, 381, -11  */
    0x3F56008B,                 /* 139, 384, -11  */
    0x3F560C88,                 /* 136, 387, -11  */
    0x3F561486,                 /* 134, 389, -11  */
    0x3F562083,                 /* 131, 392, -11  */
    0x3F562881,                 /* 129, 394, -11  */
    0x3F56347E,                 /* 126, 397, -11  */
    0x3F56407B,                 /* 123, 400, -11  */
    0x3F564879,                 /* 121, 402, -11  */
    0x3F465876,                 /* 118, 406, -12  */
    0x3F466074,                 /* 116, 408, -12  */
    0x3F466872,                 /* 114, 410, -12  */
    0x3F46746F,                 /* 111, 413, -12  */
    0x3F467C6D,                 /* 109, 415, -12  */
    0x3F46846B,                 /* 107, 417, -12  */
    0x3F468C69,                 /* 105, 419, -12  */
    0x3F469866,                 /* 102, 422, -12  */
    0x3F46A064,                 /* 100, 424, -12  */
    0x3F46AC61,                 /*  97, 427, -12  */
    0x3F46B45F,                 /*  95, 429, -12  */
    0x3F46BC5D,                 /*  93, 431, -12  */
    0x3F46C45B,                 /*  91, 433, -12  */
    0x3F46CC59,                 /*  89, 435, -12  */
    0x3F36DC56,                 /*  86, 439, -13  */
    0x3F36E454,                 /*  84, 441, -13  */
    0x3F36EC52,                 /*  82, 443, -13  */
    0x3F36F450,                 /*  80, 445, -13  */
    0x3F36FC4E,                 /*  78, 447, -13  */
    0x3F37004D,                 /*  77, 448, -13  */
    0x3F370C4A,                 /*  74, 451, -13  */
    0x3F371448,                 /*  72, 453, -13  */
    0x3F371C46,                 /*  70, 455, -13  */
    0x3F372444,                 /*  68, 457, -13  */
    0x3F372C42,                 /*  66, 459, -13  */
    0x3F373440,                 /*  64, 461, -13  */
    0x3F37383F,                 /*  63, 462, -13  */
    0x3F37403D,                 /*  61, 464, -13  */
    0x3F37483B,                 /*  59, 466, -13  */
    0x3F375039,                 /*  57, 468, -13  */
    0x3F375438,                 /*  56, 469, -13  */
    0x3F375C36,                 /*  54, 471, -13  */
    0x3F376434,                 /*  52, 473, -13  */
    0x3F376833,                 /*  51, 474, -13  */
    0x3F377031,                 /*  49, 476, -13  */
    0x3F377430,                 /*  48, 477, -13  */
    0x3F377C2E,                 /*  46, 479, -13  */
    0x3F37842C,                 /*  44, 481, -13  */
    0x3F37882B,                 /*  43, 482, -13  */
    0x3F47882A,                 /*  42, 482, -12  */
    0x3F479028,                 /*  40, 484, -12  */
    0x3F479427,                 /*  39, 485, -12  */
    0x3F479C25,                 /*  37, 487, -12  */
    0x3F47A024,                 /*  36, 488, -12  */
    0x3F47A822,                 /*  34, 490, -12  */
    0x3F47AC21,                 /*  33, 491, -12  */
    0x3F47B020,                 /*  32, 492, -12  */
    0x3F57B01F,                 /*  31, 492, -11  */
    0x3F57B81D,                 /*  29, 494, -11  */
    0x3F57BC1C,                 /*  28, 495, -11  */
    0x3F57C01B,                 /*  27, 496, -11  */
    0x3F57C41A,                 /*  26, 497, -11  */
    0x3F67C818,                 /*  24, 498, -10  */
    0x3F67CC17,                 /*  23, 499, -10  */
    0x3F67D016,                 /*  22, 500, -10  */
    0x3F67D415,                 /*  21, 501, -10  */
    0x3F67D814,                 /*  20, 502, -10  */
    0x3F77D813,                 /*  19, 502,  -9  */
    0x3F77DC12,                 /*  18, 503,  -9  */
    0x3F77E011,                 /*  17, 504,  -9  */
    0x3F87E010,                 /*  16, 504,  -8  */
    0x3F87E40F,                 /*  15, 505,  -8  */
    0x3F87E80E,                 /*  14, 506,  -8  */
    0x3F97E80D,                 /*  13, 506,  -7  */
    0x3F97EC0C,                 /*  12, 507,  -7  */
    0x3F97F00B,                 /*  11, 508,  -7  */
    0x3FA7F00A,                 /*  10, 508,  -6  */
    0x3FA7F409,                 /*   9, 509,  -6  */
    0x3FB7F408,                 /*   8, 509,  -5  */
    0x3FB7F408,                 /*   8, 509,  -5  */
    0x3FC7F806,                 /*   6, 510,  -4  */
    0x3FC7F806,                 /*   6, 510,  -4  */
    0x3FD7F805,                 /*   5, 510,  -3  */
    0x3FD7FC04,                 /*   4, 511,  -3  */
    0x3FE7FC03,                 /*   3, 511,  -2  */
    0x3FE7FC03,                 /*   3, 511,  -2  */
    0x3FF7FC02,                 /*   2, 511,  -1  */
    0x3FF7FC02,                 /*   2, 511,  -1  */
    0x0007FC01,                 /*   1, 511,   0  */
    0x0007FC01,                 /*   1, 511,   0  */
    0x0007FC01,                 /*   1, 511,   0  */
    0x0027FFFF,                 /*  -1, 511,   2  */
    0x0027FFFF,                 /*  -1, 511,   2  */
    0x0037FFFE,                 /*  -2, 511,   3  */
    0x0037FFFE,                 /*  -2, 511,   3  */
    0x0047FFFD,                 /*  -3, 511,   4  */
    0x0047FBFE,                 /*  -2, 510,   4  */
    0x0057FBFD,                 /*  -3, 510,   5  */
    0x0067FBFC,                 /*  -4, 510,   6  */
    0x0077F7FC,                 /*  -4, 509,   7  */
    0x0077F7FC,                 /*  -4, 509,   7  */
    0x0087F7FB,                 /*  -5, 509,   8  */
    0x0097F3FB,                 /*  -5, 508,   9  */
    0x00A7F3FA,                 /*  -6, 508,  10  */
    0x00B7EFFA,                 /*  -6, 507,  11  */
    0x00C7EBFA,                 /*  -6, 506,  12  */
    0x00D7EBF9,                 /*  -7, 506,  13  */
    0x00E7E7F9,                 /*  -7, 505,  14  */
    0x00F7E3F9,                 /*  -7, 504,  15  */
    0x0107E3F8,                 /*  -8, 504,  16  */
    0x0117DFF8,                 /*  -8, 503,  17  */
    0x0127DBF8,                 /*  -8, 502,  18  */
    0x0137DBF7,                 /*  -9, 502,  19  */
    0x0147D7F7,                 /*  -9, 501,  20  */
    0x0157D3F7,                 /*  -9, 500,  21  */
    0x0167CFF7,                 /*  -9, 499,  22  */
    0x0177CBF7,                 /*  -9, 498,  23  */
    0x0197C7F6,                 /* -10, 497,  25  */
    0x01A7C3F6,                 /* -10, 496,  26  */
    0x01B7BFF6,                 /* -10, 495,  27  */
    0x01C7BBF6,                 /* -10, 494,  28  */
    0x01E7B3F6,                 /* -10, 492,  30  */
    0x01F7B3F5,                 /* -11, 492,  31  */
    0x0207AFF5,                 /* -11, 491,  32  */
    0x0217ABF5,                 /* -11, 490,  33  */
    0x0237A3F5,                 /* -11, 488,  35  */
    0x02479FF5,                 /* -11, 487,  36  */
    0x026797F5,                 /* -11, 485,  38  */
    0x027793F5,                 /* -11, 484,  39  */
    0x02978BF5,                 /* -11, 482,  41  */
    0x02A78BF4,                 /* -12, 482,  42  */
    0x02B787F4,                 /* -12, 481,  43  */
    0x02D77FF4,                 /* -12, 479,  45  */
    0x02F777F4,                 /* -12, 477,  47  */
    0x030773F4,                 /* -12, 476,  48  */
    0x03276BF4,                 /* -12, 474,  50  */
    0x033767F4,                 /* -12, 473,  51  */
    0x03575FF4,                 /* -12, 471,  53  */
    0x037757F4,                 /* -12, 469,  55  */
    0x038753F4,                 /* -12, 468,  56  */
    0x03A74BF4,                 /* -12, 466,  58  */
    0x03C743F4,                 /* -12, 464,  60  */
    0x03E73BF4,                 /* -12, 462,  62  */
    0x040737F3,                 /* -13, 461,  64  */
    0x04272FF3,                 /* -13, 459,  66  */
    0x044727F3,                 /* -13, 457,  68  */
    0x04671FF3,                 /* -13, 455,  70  */
    0x048717F3,                 /* -13, 453,  72  */
    0x04A70FF3,                 /* -13, 451,  74  */
    0x04C703F4,                 /* -12, 448,  76  */
    0x04D6FFF4,                 /* -12, 447,  77  */
    0x04F6F7F4,                 /* -12, 445,  79  */
    0x0516EFF4,                 /* -12, 443,  81  */
    0x0536E7F4,                 /* -12, 441,  83  */
    0x0556DFF4,                 /* -12, 439,  85  */
    0x0586CFF5,                 /* -11, 435,  88  */
    0x05A6C7F5,                 /* -11, 433,  90  */
    0x05C6BFF5,                 /* -11, 431,  92  */
    0x05F6B7F4,                 /* -12, 429,  95  */
    0x0616AFF4,                 /* -12, 427,  97  */
    0x0636A3F5,                 /* -11, 424,  99  */
    0x06569BF5,                 /* -11, 422, 101  */
    0x06868FF5,                 /* -11, 419, 104  */
    0x06A687F5,                 /* -11, 417, 106  */
    0x06C67FF5,                 /* -11, 415, 108  */
    0x06E677F5,                 /* -11, 413, 110  */
    0x07166BF5,                 /* -11, 410, 113  */
    0x073663F5,                 /* -11, 408, 115  */
    0x07665BF4,                 /* -12, 406, 118  */
    0x07964BF5,                 /* -11, 402, 121  */
    0x07B643F5,                 /* -11, 400, 123  */
    0x07D637F6,                 /* -10, 397, 125  */
    0x08062BF6,                 /* -10, 394, 128  */
    0x082623F6,                 /* -10, 392, 130  */
    0x085617F6,                 /* -10, 389, 133  */
    0x08760FF6,                 /* -10, 387, 135  */
    0x08B603F5,                 /* -11, 384, 139  */
    0x08D5F7F6,                 /* -10, 381, 141  */
    0x0905EBF6,                 /* -10, 378, 144  */
    0x0925E3F6,                 /* -10, 376, 146  */
    0x0955D3F7,                 /*  -9, 372, 149  */
    0x0985C7F7,                 /*  -9, 369, 152  */
    0x09A5BFF7,                 /*  -9, 367, 154  */
    0x09D5B3F7,                 /*  -9, 364, 157  */
    0x0A05ABF6,                 /* -10, 362, 160  */
    0x0A359BF7,                 /*  -9, 358, 163  */
    0x0A658FF7,                 /*  -9, 355, 166  */
    0x0A9587F6,                 /* -10, 353, 169  */
    0x0AB57BF7,                 /*  -9, 350, 171  */
    0x0AE56FF7,                 /*  -9, 347, 174  */
    0x0B1563F7,                 /*  -9, 344, 177  */
    0x0B455BF6,                 /* -10, 342, 180  */
    0x0B754BF7,                 /*  -9, 338, 183  */
    0x0BA53FF7,                 /*  -9, 335, 186  */
    0x0BD533F7,                 /*  -9, 332, 189  */
    0x0C0523F8,                 /*  -8, 328, 192  */
    0x0C251BF8,                 /*  -8, 326, 194  */
    0x0C550FF8,                 /*  -8, 323, 197  */
    0x0C9503F7,                 /*  -9, 320, 201  */
    0x0CC4F7F7,                 /*  -9, 317, 204  */
    0x0CF4EBF7,                 /*  -9, 314, 207  */
    0x0D24DBF8,                 /*  -8, 310, 210  */
    0x0D54CFF8,                 /*  -8, 307, 213  */
    0x0D84C3F8,                 /*  -8, 304, 216  */
    0x0DB4BBF7,                 /*  -9, 302, 219  */
    0x0DE4AFF7,                 /*  -9, 299, 222  */
    0x0E149FF8,                 /*  -8, 295, 225  */
    0x0E4493F8,                 /*  -8, 292, 228  */
    0x0E7483F9,                 /*  -7, 288, 231  */
    0x0EA477F9,                 /*  -7, 285, 234  */
    0x0ED46BF9,                 /*  -7, 282, 237  */
    0x0F045FF9,                 /*  -7, 279, 240  */
    0x0F4453F8,                 /*  -8, 276, 244  */
    0x0F7447F8,                 /*  -8, 273, 247  */
    0x0FA43BF8,                 /*  -8, 270, 250  */
    0x0FD42BF9,                 /*  -7, 266, 253  */
    0x10041FF9,                 /*  -7, 263, 256  */
};

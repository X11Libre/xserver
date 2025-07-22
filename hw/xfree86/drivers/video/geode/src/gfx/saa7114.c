/* Copyright (c) 2005 Advanced Micro Devices, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *
 * Neither the name of the Advanced Micro Devices, Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from this
 * software without specific prior written permission.
 * */

/* 
 * This file contains routines to control the Philips SAA7114 video decoder.
 * */

/*---------------------------*/
/*  TABLE OF DEFAULT VALUES  */
/*---------------------------*/

typedef struct tagGFX_SAA7114_INIT {
    unsigned char index;
    unsigned char value;
} GFX_SAA7114_INIT;

/* Task A is for VBI raw data and task B is for video */

GFX_SAA7114_INIT gfx_saa7114_init_values[] = {
    {0x01, 0x08}, {0x02, 0xC0}, {0x03, 0x00}, {0x04, 0x90},
    {0x05, 0x90}, {0x06, 0xEB}, {0x07, 0xE0}, {0x08, 0x88},
    {0x09, 0x40}, {0x0A, 0x80}, {0x0B, 0x44}, {0x0C, 0x40},
    {0x0D, 0x00}, {0x0E, 0x89}, {0x0F, 0x2E}, {0x10, 0x0E},
    {0x11, 0x00}, {0x12, 0x05}, {0x13, 0x00}, {0x14, 0x08},
    {0x15, 0x11}, {0x16, 0xFE}, {0x17, 0x00}, {0x18, 0x40},
    {0x19, 0x80}, {0x30, 0xBC}, {0x31, 0xDF}, {0x32, 0x02},
    {0x34, 0xCD}, {0x35, 0xCC}, {0x36, 0x3A}, {0x38, 0x03},
    {0x39, 0x10}, {0x3A, 0x00}, {0x40, 0x00}, {0x41, 0xFF},
    {0x42, 0xFF}, {0x43, 0xFF}, {0x44, 0xFF}, {0x45, 0xFF},
    {0x46, 0xFF}, {0x47, 0xFF}, {0x48, 0xFF}, {0x49, 0xFF},
    {0x4A, 0xFF}, {0x4B, 0xFF}, {0x4C, 0xFF}, {0x4D, 0xFF},
    {0x4E, 0xFF}, {0x4F, 0xFF}, {0x50, 0xFF}, {0x51, 0xFF},
    {0x52, 0xFF}, {0x53, 0xFF}, {0x54, 0xFF}, {0x55, 0xFF},
    {0x56, 0xFF}, {0x57, 0xFF}, {0x58, 0x00}, {0x59, 0x47},
    {0x5A, 0x06}, {0x5B, 0x43}, {0x5D, 0x3E}, {0x5E, 0x00},
    {0x80, 0x30}, {0x83, 0x00}, {0x84, 0x60}, {0x85, 0x00},
    {0x86, 0xE5}, {0x87, 0x01}, {0x88, 0xF8},

    /* VBI task */

    {0x90, 0x01}, {0x91, 0xC8}, {0x92, 0x08}, {0x93, 0x84},
    {0x94, 0x10}, {0x95, 0x00}, {0x96, 0xD0}, {0x97, 0x02},
    {0x98, 0x05}, {0x99, 0x00}, {0x9A, 0x0B}, {0x9B, 0x00},
    {0x9C, 0xA0}, {0x9D, 0x05}, {0x9E, 0x0B}, {0x9F, 0x00},
    {0xA0, 0x01}, {0xA1, 0x00}, {0xA2, 0x00}, {0xA4, 0x80},
    {0xA5, 0x40}, {0xA6, 0x40}, {0xA8, 0x00}, {0xA9, 0x02},
    {0xAA, 0x00}, {0xAC, 0x00}, {0xAD, 0x01}, {0xAE, 0x00},
    {0xB0, 0x00}, {0xB1, 0x04}, {0xB2, 0x00}, {0xB3, 0x04},
    {0xB4, 0x00}, {0xB8, 0x00}, {0xB9, 0x00}, {0xBA, 0x00},
    {0xBB, 0x00}, {0xBC, 0x00}, {0xBD, 0x00}, {0xBE, 0x00},
    {0xBF, 0x00},

    /* Video task */

    {0xC0, 0x80}, {0xC1, 0x08}, {0xC2, 0x00}, {0xC3, 0x80},
    {0xC4, 0x10}, {0xC5, 0x00}, {0xC6, 0xD0}, {0xC7, 0x02},
    {0xC8, 0x11}, {0xC9, 0x00}, {0xCA, 0xF1}, {0xCB, 0x00},
    {0xCC, 0xD0}, {0xCD, 0x02}, {0xCE, 0xF1}, {0xCF, 0x00},
    {0xD0, 0x01}, {0xD1, 0x00}, {0xD2, 0x00}, {0xD4, 0x80},
    {0xD5, 0x40}, {0xD6, 0x40}, {0xD8, 0x00}, {0xD9, 0x04},
    {0xDA, 0x00}, {0xDC, 0x00}, {0xDD, 0x02}, {0xDE, 0x00},
    {0xE0, 0x00}, {0xE1, 0x04}, {0xE2, 0x00}, {0xE3, 0x04},
    {0xE4, 0x00}, {0xE8, 0x00}, {0xE9, 0x00}, {0xEA, 0x00},
    {0xEB, 0x00}, {0xEC, 0x00}, {0xED, 0x00}, {0xEE, 0x00},
    {0xEF, 0x00},
};

#define GFX_NUM_SAA7114_INIT_VALUES 	\
		sizeof(gfx_saa7114_init_values)/sizeof(GFX_SAA7114_INIT)

/*-----------------------------------------------------*/
/*	  TABLE OF FIR PREFILTER RECOMMENDED VALUES        */
/*-----------------------------------------------------*/

int optimize_for_aliasing = 0;

typedef struct tagGFX_SAA7114_FIR_PREFILTER {
    unsigned char prescaler;
    unsigned char acl_low;
    unsigned char prefilter_low;
    unsigned char acl_high;
    unsigned char prefilter_high;
} GFX_SAA7114_FIR_PREFILTER;

GFX_SAA7114_FIR_PREFILTER gfx_saa7114_fir_values[] = {
    {0x01, 0x00, 0x00, 0x00, 0x00}, {0x02, 0x02, 0x5A, 0x01, 0x51},
    {0x03, 0x04, 0xAB, 0x03, 0xA2}, {0x04, 0x07, 0xA3, 0x04, 0xAB},
    {0x05, 0x08, 0xAC, 0x07, 0xA3}, {0x06, 0x08, 0xFC, 0x07, 0xF3},
    {0x07, 0x08, 0xFC, 0x07, 0xF3}, {0x08, 0x0F, 0xF4, 0x08, 0xFC},
    {0x09, 0x0F, 0xF4, 0x08, 0xFC}, {0x0A, 0x10, 0xFD, 0x08, 0xFC},
    {0x0B, 0x10, 0xFD, 0x08, 0xFC}, {0x0C, 0x10, 0xFD, 0x08, 0xFC},
    {0x0D, 0x10, 0xFD, 0x10, 0xFD}, {0x0E, 0x10, 0xFD, 0x10, 0xFD},
    {0x0F, 0x1F, 0xF5, 0x10, 0xFD}, {0x10, 0x20, 0xFE, 0x10, 0xFD},
    {0x11, 0x20, 0xFE, 0x10, 0xFD}, {0x12, 0x20, 0xFE, 0x10, 0xFD},
    {0x13, 0x20, 0xFE, 0x20, 0xFE}, {0x14, 0x20, 0xFE, 0x20, 0xFE},
    {0x15, 0x20, 0xFE, 0x20, 0xFE}, {0x16, 0x20, 0xFE, 0x20, 0xFE},
    {0x17, 0x20, 0xFE, 0x20, 0xFE}, {0x18, 0x20, 0xFE, 0x20, 0xFE},
    {0x19, 0x20, 0xFE, 0x20, 0xFE}, {0x1A, 0x20, 0xFE, 0x20, 0xFE},
    {0x1B, 0x20, 0xFE, 0x20, 0xFE}, {0x1C, 0x20, 0xFE, 0x20, 0xFE},
    {0x1D, 0x20, 0xFE, 0x20, 0xFE}, {0x1E, 0x20, 0xFE, 0x20, 0xFE},
    {0x1F, 0x20, 0xFE, 0x20, 0xFE}, {0x20, 0x3F, 0xFF, 0x20, 0xFE},
    {0x21, 0x3F, 0xFF, 0x20, 0xFE}, {0x22, 0x3F, 0xFF, 0x20, 0xFE},
    {0x23, 0x3F, 0xFF, 0x20, 0xFF}
};

int
saa7114_write_reg(unsigned char reg, unsigned char val)
{
    return gfx_i2c_write(2, SAA7114_CHIPADDR, reg, 1, &val);
}

int
saa7114_read_reg(unsigned char reg, unsigned char *val)
{
    return gfx_i2c_read(2, SAA7114_CHIPADDR, reg, 1, val);
}

/*----------------------------------------------------------------------------
 * gfx_set_decoder_vbi_upscale
 *
 * This routine configures the video decoder task A to upscale raw VBI data
 * horizontally to match a different system clock.
 * The upscale is from 13.5 MHz (SAA7114) to 14.318 MHz (Bt835).
 *----------------------------------------------------------------------------
 */
#if GFX_DECODER_DYNAMIC
int
saa7114_set_decoder_vbi_upscale(void)
#else
int
gfx_set_decoder_vbi_upscale(void)
#endif
{
    /* Set horizontal output length to 1528 (720 * 2 * 14.318 / 13.5) */
    saa7114_write_reg(SAA7114_TASK_A_HORZ_OUTPUT_LO, 0xF8);
    saa7114_write_reg(SAA7114_TASK_A_HORZ_OUTPUT_HI, 0x05);

    /* Set horizontal luminance scaling increment to 484 (1024 * 13.5 / 
     * 28.636) */
    saa7114_write_reg(SAA7114_TASK_A_HSCALE_LUMA_LO, 0xE4);
    saa7114_write_reg(SAA7114_TASK_A_HSCALE_LUMA_HI, 0x01);

    /* Set horizontal chrominance scaling increment to 242 */
    saa7114_write_reg(SAA7114_TASK_A_HSCALE_CHROMA_LO, 0xF2);
    saa7114_write_reg(SAA7114_TASK_A_HSCALE_CHROMA_HI, 0x00);

    return GFX_STATUS_OK;
}

/*----------------------------------------------------------------------------
 * gfx_decoder_software_reset
 *
 * This routine performs a software reset of the decoder.
 *----------------------------------------------------------------------------
 */
#if GFX_DECODER_DYNAMIC
int
saa7114_decoder_software_reset(void)
#else
int
gfx_decoder_software_reset(void)
#endif
{
    saa7114_write_reg(0x88, 0xC0);
    /* I2C-bus latency should be sufficient for resetting the internal state 
     * machine. */
    /* gfx_delay_milliseconds(10); */
    saa7114_write_reg(0x88, 0xF0);
    return GFX_STATUS_OK;
}

/*----------------------------------------------------------------------------
 * gfx_decoder_detect_macrovision
 *
 * This routine detects if macrovision exists in the input of the video 
 * decoder.
 *----------------------------------------------------------------------------
 */
#if GFX_DECODER_DYNAMIC
int
saa7114_decoder_detect_macrovision(void)
#else
int
gfx_decoder_detect_macrovision(void)
#endif
{
    unsigned char macrovision = 0xff;

    saa7114_read_reg(SAA7114_STATUS, &macrovision);
    return ((macrovision & 0x02) >> 1);
}

/*----------------------------------------------------------------------------
 * gfx_decoder_detect_video
 *
 * This routine detects if video exists in the input of the video decoder.
 *----------------------------------------------------------------------------
 */
#if GFX_DECODER_DYNAMIC
int
saa7114_decoder_detect_video(void)
#else
int
gfx_decoder_detect_video(void)
#endif
{
    unsigned char video = 0xff;

    saa7114_read_reg(SAA7114_STATUS, &video);
    return !((video & 0x40) >> 6);
}

/*----------------------------------------------------------------------------
 * gfx_set_decoder_defaults
 *
 * This routine is called to set the initial register values of the 
 * video decoder.
 *----------------------------------------------------------------------------
 */
#if GFX_DECODER_DYNAMIC
int
saa7114_set_decoder_defaults(void)
#else
int
gfx_set_decoder_defaults(void)
#endif
{
    unsigned int i;

    /* LOOP THROUGH INDEX/DATA PAIRS IN THE TABLE */

    for (i = 0; i < GFX_NUM_SAA7114_INIT_VALUES; i++) {
        saa7114_write_reg(gfx_saa7114_init_values[i].index,
                          gfx_saa7114_init_values[i].value);
    }

    gfx_decoder_software_reset();
    return (0);
}

/*----------------------------------------------------------------------------
 * gfx_set_decoder_analog_input
 *
 * This routine sets the analog input of the video decoder. 
 *----------------------------------------------------------------------------
 */
#if GFX_DECODER_DYNAMIC
int
saa7114_set_decoder_analog_input(unsigned char input)
#else
int
gfx_set_decoder_analog_input(unsigned char input)
#endif
{
    saa7114_write_reg(SAA7114_ANALOG_INPUT_CTRL1, input);
    return (0);
}

/*----------------------------------------------------------------------------
 * gfx_set_decoder_brightness
 *
 * This routine sets the brightness of the video decoder. 
 *----------------------------------------------------------------------------
 */
#if GFX_DECODER_DYNAMIC
int
saa7114_set_decoder_brightness(unsigned char brightness)
#else
int
gfx_set_decoder_brightness(unsigned char brightness)
#endif
{
    saa7114_write_reg(SAA7114_BRIGHTNESS, brightness);
    return (0);
}

/*----------------------------------------------------------------------------
 * gfx_set_decoder_contrast
 *
 * This routine sets the contrast of the video decoder. 
 *----------------------------------------------------------------------------
 */
#if GFX_DECODER_DYNAMIC
int
saa7114_set_decoder_contrast(unsigned char contrast)
#else
int
gfx_set_decoder_contrast(unsigned char contrast)
#endif
{
    saa7114_write_reg(SAA7114_CONTRAST, (unsigned char) (contrast >> 1));
    return (0);
}

/*----------------------------------------------------------------------------
 * gfx_set_decoder_hue
 *
 * This routine sets the hue control of the video decoder. 
 *----------------------------------------------------------------------------
 */
#if GFX_DECODER_DYNAMIC
int
saa7114_set_decoder_hue(char hue)
#else
int
gfx_set_decoder_hue(char hue)
#endif
{
    saa7114_write_reg(SAA7114_HUE, (unsigned char) hue);
    return (0);
}

/*----------------------------------------------------------------------------
 * gfx_set_decoder_saturation
 *
 * This routine sets the saturation adjustment of the video decoder. 
 *----------------------------------------------------------------------------
 */
#if GFX_DECODER_DYNAMIC
int
saa7114_set_decoder_saturation(unsigned char saturation)
#else
int
gfx_set_decoder_saturation(unsigned char saturation)
#endif
{
    saa7114_write_reg(SAA7114_SATURATION, (unsigned char) (saturation >> 1));
    return (0);
}

/*----------------------------------------------------------------------------
 * gfx_set_decoder_input_offset
 *
 * This routine sets the size of the decoder input window.
 *----------------------------------------------------------------------------
 */
#if GFX_DECODER_DYNAMIC
int
saa7114_set_decoder_input_offset(unsigned short x, unsigned short y)
#else
int
gfx_set_decoder_input_offset(unsigned short x, unsigned short y)
#endif
{
    /* SET THE INPUT WINDOW OFFSET */

    saa7114_write_reg(SAA7114_HORZ_OFFSET_LO, (unsigned char) (x & 0x00FF));
    saa7114_write_reg(SAA7114_HORZ_OFFSET_HI, (unsigned char) (x >> 8));
    saa7114_write_reg(SAA7114_VERT_OFFSET_LO, (unsigned char) (y & 0x00FF));
    saa7114_write_reg(SAA7114_VERT_OFFSET_HI, (unsigned char) (y >> 8));

    gfx_decoder_software_reset();
    return (0);
}

/*----------------------------------------------------------------------------
 * gfx_set_decoder_input_size
 *
 * This routine sets the size of the decoder input window.
 *----------------------------------------------------------------------------
 */
#if GFX_DECODER_DYNAMIC
int
saa7114_set_decoder_input_size(unsigned short width, unsigned short height)
#else
int
gfx_set_decoder_input_size(unsigned short width, unsigned short height)
#endif
{
    /* DIVIDE HEIGHT BY TWO FOR INTERLACING */

    height = (height + 1) >> 1;

    /* SET THE INPUT WINDOW SIZE */

    saa7114_write_reg(SAA7114_HORZ_INPUT_LO, (unsigned char) (width & 0x00FF));
    saa7114_write_reg(SAA7114_HORZ_INPUT_HI, (unsigned char) (width >> 8));
    saa7114_write_reg(SAA7114_VERT_INPUT_LO, (unsigned char) (height & 0x00FF));
    saa7114_write_reg(SAA7114_VERT_INPUT_HI, (unsigned char) (height >> 8));

    gfx_decoder_software_reset();
    return (0);
}

/*----------------------------------------------------------------------------
 * gfx_set_decoder_output_size
 *
 * This routine sets the size of the decoder output window.
 *----------------------------------------------------------------------------
 */
#if GFX_DECODER_DYNAMIC
int
saa7114_set_decoder_output_size(unsigned short width, unsigned short height)
#else
int
gfx_set_decoder_output_size(unsigned short width, unsigned short height)
#endif
{
    /* ROUND WIDTH UP TO EVEN NUMBER TO PREVENT DECODER BECOMING STUCK */

    width = ((width + 1) >> 1) << 1;

    /* DIVIDE HEIGHT BY TWO FOR INTERLACING */

    height = (height + 1) >> 1;

    /* SET THE OUTPUT WINDOW SIZE */

    saa7114_write_reg(SAA7114_HORZ_OUTPUT_LO, (unsigned char) (width & 0x00FF));
    saa7114_write_reg(SAA7114_HORZ_OUTPUT_HI, (unsigned char) (width >> 8));
    saa7114_write_reg(SAA7114_VERT_OUTPUT_LO,
                      (unsigned char) (height & 0x00FF));
    saa7114_write_reg(SAA7114_VERT_OUTPUT_HI, (unsigned char) (height >> 8));

    gfx_decoder_software_reset();
    return (0);
}

/*----------------------------------------------------------------------------
 * gfx_set_decoder_scale
 *
 * This routine sets the scaling of the video decoder.
 *----------------------------------------------------------------------------
 */
#if GFX_DECODER_DYNAMIC
int
saa7114_set_decoder_scale(unsigned short srcw, unsigned short srch,
                          unsigned short dstw, unsigned short dsth)
#else
int
gfx_set_decoder_scale(unsigned short srcw, unsigned short srch,
                      unsigned short dstw, unsigned short dsth)
#endif
{
    unsigned char prescale = 0;
    int scale = 0;

    /* SET THE HORIZONTAL PRESCALE */
    /* Downscale from 1 to 1/63 source size. */

    if (dstw)
        prescale = (unsigned char) (srcw / dstw);
    if (!prescale)
        prescale = 1;
    if (prescale > 63)
        return (1);
    saa7114_write_reg(SAA7114_HORZ_PRESCALER, prescale);

    /* USE FIR PREFILTER FUNCTIONALITY (OPTIMISATION) */

    if (prescale < 36) {
        if (optimize_for_aliasing) {
            saa7114_write_reg(SAA7114_HORZ_ACL,
                              gfx_saa7114_fir_values[prescale - 1].acl_low);
            saa7114_write_reg(SAA7114_HORZ_FIR_PREFILTER,
                              gfx_saa7114_fir_values[prescale -
                                                     1].prefilter_low);
        }
        else {
            saa7114_write_reg(SAA7114_HORZ_ACL,
                              gfx_saa7114_fir_values[prescale - 1].acl_high);
            saa7114_write_reg(SAA7114_HORZ_FIR_PREFILTER,
                              gfx_saa7114_fir_values[prescale -
                                                     1].prefilter_high);
        }
    }
    else {
        /* SAME SETTINGS FOR RATIO 1/35 DOWNTO 1/63 */
        if (optimize_for_aliasing) {
            saa7114_write_reg(SAA7114_HORZ_ACL,
                              gfx_saa7114_fir_values[34].acl_low);
            saa7114_write_reg(SAA7114_HORZ_FIR_PREFILTER,
                              gfx_saa7114_fir_values[34].prefilter_low);
        }
        else {
            saa7114_write_reg(SAA7114_HORZ_ACL,
                              gfx_saa7114_fir_values[34].acl_high);
            saa7114_write_reg(SAA7114_HORZ_FIR_PREFILTER,
                              gfx_saa7114_fir_values[34].prefilter_high);
        }
    }

    /* SET THE HORIZONTAL SCALING */

    if (!dstw)
        return (1);
    scale = ((1024 * srcw * 1000) / (dstw * prescale)) / 1000;
    if ((scale > 8191) || (scale < 300))
        return (1);
    saa7114_write_reg(SAA7114_HSCALE_LUMA_LO, (unsigned char) (scale & 0x00FF));
    saa7114_write_reg(SAA7114_HSCALE_LUMA_HI, (unsigned char) (scale >> 8));
    scale >>= 1;
    saa7114_write_reg(SAA7114_HSCALE_CHROMA_LO,
                      (unsigned char) (scale & 0x00FF));
    saa7114_write_reg(SAA7114_HSCALE_CHROMA_HI, (unsigned char) (scale >> 8));

    /* SET THE VERTICAL SCALING (INTERPOLATION MODE) */

    if (!dsth)
        return (1);

    /* ROUND DESTINATION HEIGHT UP TO EVEN NUMBER TO PREVENT DECODER BECOMING
     * STUCK */

    dsth = ((dsth + 1) >> 1) << 1;

    scale = (int) ((1024 * srch) / dsth);
    saa7114_write_reg(SAA7114_VSCALE_LUMA_LO, (unsigned char) (scale & 0x00FF));
    saa7114_write_reg(SAA7114_VSCALE_LUMA_HI, (unsigned char) (scale >> 8));
    saa7114_write_reg(SAA7114_VSCALE_CHROMA_LO,
                      (unsigned char) (scale & 0x00FF));
    saa7114_write_reg(SAA7114_VSCALE_CHROMA_HI, (unsigned char) (scale >> 8));

    if (dsth >= (srch >> 1)) {
        /* USE INTERPOLATION MODE FOR SCALE FACTOR ABOVE 0.5 */

        saa7114_write_reg(SAA7114_VSCALE_CONTROL, 0x00);

        /* SET VERTICAL PHASE REGISTER FOR CORRECT SCALED INTERLACED OUTPUT 
         * (OPTIMISATION) */
        /* THE OPTIMISATION IS BASED ON OFIDC = 0 (REG 90h[6] = 0 ) */
        saa7114_write_reg(SAA7114_VSCALE_CHROMA_OFFS0, SAA7114_VSCALE_PHO);
        saa7114_write_reg(SAA7114_VSCALE_CHROMA_OFFS1, SAA7114_VSCALE_PHO);
        saa7114_write_reg(SAA7114_VSCALE_CHROMA_OFFS2,
                          (unsigned char) (SAA7114_VSCALE_PHO + scale / 64 -
                                           16));
        saa7114_write_reg(SAA7114_VSCALE_CHROMA_OFFS3,
                          (unsigned char) (SAA7114_VSCALE_PHO + scale / 64 -
                                           16));

        saa7114_write_reg(SAA7114_VSCALE_LUMINA_OFFS0, SAA7114_VSCALE_PHO);
        saa7114_write_reg(SAA7114_VSCALE_LUMINA_OFFS1, SAA7114_VSCALE_PHO);
        saa7114_write_reg(SAA7114_VSCALE_LUMINA_OFFS2,
                          (unsigned char) (SAA7114_VSCALE_PHO + scale / 64 -
                                           16));
        saa7114_write_reg(SAA7114_VSCALE_LUMINA_OFFS3,
                          (unsigned char) (SAA7114_VSCALE_PHO + scale / 64 -
                                           16));

        /* RESTORE CONTRAST AND SATURATION FOR INTERPOLATION MODE */

        saa7114_write_reg(SAA7114_FILTER_CONTRAST, (unsigned char) 0x40);
        saa7114_write_reg(SAA7114_FILTER_SATURATION, (unsigned char) 0x40);
    }
    else {
        /* USE ACCUMULATION MODE FOR DOWNSCALING BY MORE THAN 2x */

        saa7114_write_reg(SAA7114_VSCALE_CONTROL, 0x01);

        /* SET VERTICAL PHASE OFFSETS OFF (OPTIMISATION) */
        saa7114_write_reg(SAA7114_VSCALE_CHROMA_OFFS0, 0x00);
        saa7114_write_reg(SAA7114_VSCALE_CHROMA_OFFS1, 0x00);
        saa7114_write_reg(SAA7114_VSCALE_CHROMA_OFFS2, 0x00);
        saa7114_write_reg(SAA7114_VSCALE_CHROMA_OFFS3, 0x00);

        saa7114_write_reg(SAA7114_VSCALE_LUMINA_OFFS0, 0x00);
        saa7114_write_reg(SAA7114_VSCALE_LUMINA_OFFS1, 0x00);
        saa7114_write_reg(SAA7114_VSCALE_LUMINA_OFFS2, 0x00);
        saa7114_write_reg(SAA7114_VSCALE_LUMINA_OFFS3, 0x00);

        /* ADJUST CONTRAST AND SATURATION FOR ACCUMULATION MODE */

        if (srch)
            scale = (64 * dsth) / srch;
        saa7114_write_reg(SAA7114_FILTER_CONTRAST, (unsigned char) scale);
        saa7114_write_reg(SAA7114_FILTER_SATURATION, (unsigned char) scale);
    }

    gfx_decoder_software_reset();
    return (0);
}

/*----------------------------------------------------------------------------
 * gfx_set_decoder_vbi_format
 *
 * This routine programs the decoder to produce the specified format of VBI
 * data for the specified lines.
 *----------------------------------------------------------------------------
 */
#if GFX_DECODER_DYNAMIC
int
saa7114_set_decoder_vbi_format(int start, int end, int format)
#else
int
gfx_set_decoder_vbi_format(int start, int end, int format)
#endif
{
    int i;
    unsigned char data;

    for (i = start; i <= end; i++) {
        switch (format) {
        case VBI_FORMAT_VIDEO:
            data = 0xFF;
            break;              /* Active video */
        case VBI_FORMAT_RAW:
            data = 0x77;
            break;              /* Raw VBI data */
        case VBI_FORMAT_CC:
            data = 0x55;
            break;              /* US CC        */
        case VBI_FORMAT_NABTS:
            data = 0xCC;
            break;              /* US NABTS     */
        default:
            return GFX_STATUS_BAD_PARAMETER;
        }
        saa7114_write_reg((unsigned char) (0x3F + i), data);
    }
    return GFX_STATUS_OK;
}

/*----------------------------------------------------------------------------
 * gfx_set_decoder_vbi_enable
 *
 * This routine enables or disables VBI transfer in the decoder.
 *----------------------------------------------------------------------------
 */
#if GFX_DECODER_DYNAMIC
int
saa7114_set_decoder_vbi_enable(int enable)
#else
int
gfx_set_decoder_vbi_enable(int enable)
#endif
{
    unsigned char data;

    saa7114_read_reg(SAA7114_IPORT_CONTROL, &data);
    if (enable)
        data |= 0x80;
    else
        data &= ~0x80;
    saa7114_write_reg(SAA7114_IPORT_CONTROL, data);
    return GFX_STATUS_OK;
}

/*----------------------------------------------------------------------------
 * gfx_set_decoder_TV_standard
 *
 * This routine configures the decoder for the required TV standard.
 *----------------------------------------------------------------------------
 */
#if GFX_DECODER_DYNAMIC
int
saa7114_set_decoder_TV_standard(TVStandardType TVStandard)
#else
int
gfx_set_decoder_TV_standard(TVStandardType TVStandard)
#endif
{
    switch (TVStandard) {
    case TV_STANDARD_NTSC:
        saa7114_write_reg(0x0E, 0x89);
        saa7114_write_reg(0x5A, 0x06);
        break;
    case TV_STANDARD_PAL:
        saa7114_write_reg(0x0E, 0x81);
        saa7114_write_reg(0x5A, 0x03);
        break;
    default:
        return GFX_STATUS_BAD_PARAMETER;
    }
    gfx_decoder_software_reset();
    return GFX_STATUS_OK;
}

/*----------------------------------------------------------------------------
 * gfx_set_decoder_luminance_filter
 *
 * This routine sets the hue control of the video decoder. 
 *----------------------------------------------------------------------------
 */
#if GFX_DECODER_DYNAMIC
int
saa7114_set_decoder_luminance_filter(unsigned char lufi)
#else
int
gfx_set_decoder_luminance_filter(unsigned char lufi)
#endif
{
    unsigned char data;

    saa7114_read_reg(SAA7114_LUMINANCE_CONTROL, &data);
    saa7114_write_reg(SAA7114_LUMINANCE_CONTROL,
                      (unsigned char) ((data & ~0x0F) | (lufi & 0x0F)));
    return (0);
}

/*************************************************************/
/*  READ ROUTINES  |  INCLUDED FOR DIAGNOSTIC PURPOSES ONLY  */
/*************************************************************/

#if GFX_READ_ROUTINES

/*----------------------------------------------------------------------------
 * gfx_get_decoder_brightness
 *
 * This routine returns the current brightness of the video decoder.  
 *----------------------------------------------------------------------------
 */
#if GFX_DECODER_DYNAMIC
unsigned char
saa7114_get_decoder_brightness(void)
#else
unsigned char
gfx_get_decoder_brightness(void)
#endif
{
    unsigned char brightness = 0;

    saa7114_read_reg(SAA7114_BRIGHTNESS, &brightness);
    return (brightness);
}

/*----------------------------------------------------------------------------
 * gfx_get_decoder_contrast
 *
 * This routine returns the current contrast of the video decoder.  
 *----------------------------------------------------------------------------
 */
#if GFX_DECODER_DYNAMIC
unsigned char
saa7114_get_decoder_contrast(void)
#else
unsigned char
gfx_get_decoder_contrast(void)
#endif
{
    unsigned char contrast = 0;

    saa7114_read_reg(SAA7114_CONTRAST, &contrast);
    contrast <<= 1;
    return (contrast);
}

/*----------------------------------------------------------------------------
 * gfx_get_decoder_hue
 *
 * This routine returns the current hue of the video decoder.  
 *----------------------------------------------------------------------------
 */
#if GFX_DECODER_DYNAMIC
char
saa7114_get_decoder_hue(void)
#else
char
gfx_get_decoder_hue(void)
#endif
{
    unsigned char hue = 0;

    saa7114_read_reg(SAA7114_HUE, &hue);
    return ((char) hue);
}

/*----------------------------------------------------------------------------
 * gfx_get_decoder_saturation
 *
 * This routine returns the current saturation of the video decoder.  
 *----------------------------------------------------------------------------
 */
#if GFX_DECODER_DYNAMIC
unsigned char
saa7114_get_decoder_saturation(void)
#else
unsigned char
gfx_get_decoder_saturation(void)
#endif
{
    unsigned char saturation = 0;

    saa7114_read_reg(SAA7114_SATURATION, &saturation);
    saturation <<= 1;
    return (saturation);
}

/*----------------------------------------------------------------------------
 * gfx_get_decoder_input_offset
 *
 * This routine returns the offset into the input window.
 *----------------------------------------------------------------------------
 */
#if GFX_DECODER_DYNAMIC
unsigned long
saa7114_get_decoder_input_offset(void)
#else
unsigned long
gfx_get_decoder_input_offset(void)
#endif
{
    unsigned long value = 0;
    unsigned char data;

    saa7114_read_reg(SAA7114_HORZ_OFFSET_LO, &data);
    value = (unsigned long) data;
    saa7114_read_reg(SAA7114_HORZ_OFFSET_HI, &data);
    value |= ((unsigned long) data) << 8;
    saa7114_read_reg(SAA7114_VERT_OFFSET_LO, &data);
    value |= ((unsigned long) data) << 16;
    saa7114_read_reg(SAA7114_VERT_OFFSET_HI, &data);
    value |= ((unsigned long) data) << 24;
    return (value);
}

/*----------------------------------------------------------------------------
 * gfx_get_decoder_input_size
 *
 * This routine returns the current size of the input window
 *----------------------------------------------------------------------------
 */
#if GFX_DECODER_DYNAMIC
unsigned long
saa7114_get_decoder_input_size(void)
#else
unsigned long
gfx_get_decoder_input_size(void)
#endif
{
    unsigned long value = 0;
    unsigned char data;

    saa7114_read_reg(SAA7114_HORZ_INPUT_LO, &data);
    value = (unsigned long) data;
    saa7114_read_reg(SAA7114_HORZ_INPUT_HI, &data);
    value |= ((unsigned long) data) << 8;
    saa7114_read_reg(SAA7114_VERT_INPUT_LO, &data);
    value |= ((unsigned long) data) << 17;
    saa7114_read_reg(SAA7114_VERT_INPUT_HI, &data);
    value |= ((unsigned long) data) << 25;
    return (value);
}

/*----------------------------------------------------------------------------
 * gfx_get_decoder_output_size
 *
 * This routine returns the current size of the output window.
 *----------------------------------------------------------------------------
 */
#if GFX_DECODER_DYNAMIC
unsigned long
saa7114_get_decoder_output_size(void)
#else
unsigned long
gfx_get_decoder_output_size(void)
#endif
{
    unsigned long value = 0;
    unsigned char data;

    saa7114_read_reg(SAA7114_HORZ_OUTPUT_LO, &data);
    value = (unsigned long) data;
    saa7114_read_reg(SAA7114_HORZ_OUTPUT_HI, &data);
    value |= ((unsigned long) data) << 8;
    saa7114_read_reg(SAA7114_VERT_OUTPUT_LO, &data);
    value |= ((unsigned long) data) << 17;
    saa7114_read_reg(SAA7114_VERT_OUTPUT_HI, &data);
    value |= ((unsigned long) data) << 25;
    return (value);
}

/*----------------------------------------------------------------------------
 * gfx_get_decoder_vbi_format
 *
 * This routine returns the current format of VBI data for the specified line.
 *----------------------------------------------------------------------------
 */
#if GFX_DECODER_DYNAMIC
int
saa7114_get_decoder_vbi_format(int line)
#else
int
gfx_get_decoder_vbi_format(int line)
#endif
{
    unsigned char format = 0, data;

    saa7114_read_reg((unsigned char) (0x3F + line), &data);
    switch (data) {
    case 0xFF:
        format = VBI_FORMAT_VIDEO;
        break;                  /* Active video */
    case 0x77:
        format = VBI_FORMAT_RAW;
        break;                  /* Raw VBI data */
    case 0x55:
        format = VBI_FORMAT_CC;
        break;                  /* US CC        */
    case 0xCC:
        format = VBI_FORMAT_NABTS;
        break;                  /* US NABTS     */
    }
    return (format);
}

#endif                          /* GFX_READ_ROUTINES */

/* END OF FILE */

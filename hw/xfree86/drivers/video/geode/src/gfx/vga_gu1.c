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
 * This file contains routines to set modes using the VGA registers.  
 * Since this file is for the first generation graphics unit, it interfaces
 * to SoftVGA registers.  It works for both VSA1 and VSA2.
 * */

/* SoftVGA Extended CRTC register indices and bit definitions */

#define CRTC_EXTENDED_REGISTER_LOCK		0x30
#define CRTC_MODE_SWITCH_CONTROL		0x3F

/* BIT DEFINITIONS */

#define CRTC_BIT_16BPP					0x01
#define CRTC_BIT_555					0x02

/* LOCAL ROUTINE DEFINITIONS */

int gu1_detect_vsa2(void);

/*---------------------------------*/
/*  MODE TABLES FOR VGA REGISTERS  */
/*---------------------------------*/

/* FOR SoftVGA, the CRTC_EXTENDED_ADDRESS_CONTROL (0x43) is always equal to 
 * 0x03 for a packed linear frame buffer organization.  The 
 * CRTC_EXTENDED_DAC_CONTROL (0x4B) is always equal to 0x03 to work with 
 * older versions of VSA1 (that needed to specify 8 or 16 bit bus to an
 * external RAMDAC.  This is not used in VSA2.  The clock frequency is 
 * specified in register 0x4D if clock control (0x4C) is set to 0x80.
 * Higher resolutions (1280x1024) use the CRTC_EXTENDED_VERTICAL_TIMING
 * register (index 0x41).
 */

gfx_vga_struct gfx_vga_modes[] = {
/*--------------------------------------------------------------------------*/
    {
     640, 480, 60,              /* 640x480                                                              */
     25,                        /* 25 MHz clock = 60 Hz refresh rate    */
     0xE3,                      /* miscOutput register                                  */
     {
      0x5F, 0x4F, 0x50, 0x82, 0x54, 0x80, 0x0B, 0x3E,
      /* standard CRTC                                                */
      0x80, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0xEA, 0x0C, 0xDF, 0x50, 0x00, 0xE7, 0x04, 0xE3, 0xFF},
     {
      0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00,
      /* extended CRTC                                                */
      0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00}
     },
/*--------------------------------------------------------------------------*/
    {
     640, 480, 72,              /* 640x480 */
     29,                        /* 29 MHz clock = 72 Hz refresh rate */
     0xE3,                      /* miscOutput register */
     {
      0x63, 0x4f, 0x50, 0x86, 0x55, 0x99, 0x06, 0x3e,
      /* standard CRTC */
      0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0xe9, 0x0c, 0xdf, 0x00, 0x00, 0xe7, 0x00, 0xe3, 0xff},
     {
      0x6D, 0x00, 0x00, 0x03, 0x00, 0x01, 0x01, 0x00,
      /* extended CRTC */
      0x00, 0x00, 0x01, 0x08, 0x80, 0x1F, 0x00, 0x4B}
     },
/*--------------------------------------------------------------------------*/
    {
     640, 480, 75,              /* 640x480 */
     31,
     /* 31.5 MHz clock = 75 Hz refresh rate */
     0xE3,                      /* miscOutput register */
     {
      0x64, 0x4F, 0x4F, 0x88, 0x54, 0x9B, 0xF2, 0x1F,
      /* standard CRTC */
      0x80, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0xE1, 0x04, 0xDF, 0x50, 0x00, 0xDF, 0xF3, 0xE3, 0xFF},
     {
      0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00,
      /* extended CRTC */
      0x00, 0x00, 0x00, 0x03, 0x80, 0x1F, 0x00, 0x00}
     },
/*--------------------------------------------------------------------------*/
    {
     800, 600, 60,              /* 800x600 */
     40,                        /* 40 MHz clock = 60 Hz refresh rate */
     0x23,                      /* miscOutput register */
     {
      0x7F, 0x63, 0x64, 0x82, 0x6B, 0x1B, 0x72, 0xF0,
      /* standard CRTC */
      0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x59, 0x0D, 0x57, 0x64, 0x00, 0x57, 0x73, 0xE3, 0xFF},
     {
      0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00,
      /* extended CRTC */
      0x00, 0x00, 0x00, 0x03, 0x80, 0x28, 0x00, 0x00}
     },
/*--------------------------------------------------------------------------*/
    {
     800, 600, 72,              /* 800x600 */
     47,                        /* 47 MHz clock = 72 Hz refresh rate */
     0x2B,                      /* miscOutput register */
     {
      0x7D, 0x63, 0x63, 0x81, 0x6D, 0x1B, 0x98, 0xF0,
      /* standard CRTC */
      0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x7D, 0x03, 0x57, 0x00, 0x00, 0x57, 0x9A, 0xE3, 0xFF},
     {
      0x6F, 0x00, 0x00, 0x03, 0x00, 0x01, 0x01, 0x00,
      /* extended CRTC */
      0x00, 0x00, 0x01, 0x08, 0x80, 0x32, 0x00, 0x4B}
     },
/*--------------------------------------------------------------------------*/
    {
     800, 600, 75,              /* 800x600 */
     49,
     /* 49.5 MHz clock = 75 Hz refresh rate */
     0x23,                      /* miscOutput register */
     {
      0x7F, 0x63, 0x63, 0x83, 0x68, 0x11, 0x6F, 0xF0,
      /* standard CRTC */
      0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x59, 0x1C, 0x57, 0x64, 0x00, 0x57, 0x70, 0xE3, 0xFF},
     {
      0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00,
      /* extended CRTC */
      0x00, 0x00, 0x00, 0x03, 0x80, 0x31, 0x00, 0x00}
     },
/*--------------------------------------------------------------------------*/
    {
     1024, 768, 60,             /* 1024x768 */
     65,                        /* 65 MHz clock = 60 Hz refresh rate */
     0xE3,                      /* miscOutput register */
     {
      0xA3, 0x7F, 0x80, 0x86, 0x85, 0x96, 0x24, 0xF5,
      /* standard CRTC */
      0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x04, 0x0A, 0xFF, 0x80, 0x00, 0xFF, 0x25, 0xE3, 0xFF},
     {
      0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00,
      /* extended CRTC */
      0x00, 0x00, 0x00, 0x03, 0x80, 0x41, 0x00, 0x00}
     },
/*--------------------------------------------------------------------------*/
    {
     1024, 768, 70,             /* 1024x768 */
     76,                        /* 76 MHz clock = 70 Hz refresh rate */
     0x2B,                      /* miscOutput register */
     {
      0xA1, 0x7F, 0x7F, 0x85, 0x85, 0x95, 0x24, 0xF5,
      /* standard CRTC */
      0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x03, 0x09, 0xFF, 0x00, 0x00, 0xFF, 0x26, 0xE3, 0xFF},
     {
      0x62, 0x00, 0x00, 0x03, 0x00, 0x01, 0x01, 0x00,
      /* extended CRTC */
      0x00, 0x00, 0x01, 0x02, 0x80, 0x4B, 0x00, 0x4B}
     },
/*--------------------------------------------------------------------------*/
    {
     1024, 768, 75,             /* 1024x768 */
     79,                        /* 79 MHz clock = 75 Hz refresh rate */
     0xE3,                      /* miscOutput register */
     {
      0x9F, 0x7F, 0x7F, 0x83, 0x84, 0x8F, 0x1E, 0xF5,
      /* standard CRTC */
      0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x01, 0x04, 0xFF, 0x80, 0x00, 0xFF, 0x1F, 0xE3, 0xFF},
     {
      0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00,
      /* extended CRTC */
      0x00, 0x00, 0x00, 0x03, 0x80, 0x4F, 0x00, 0x00}
     },
/*--------------------------------------------------------------------------*/
    {
     1280, 1024, 60,            /* 1280x1024 */
     108,
     /* 108 MHz clock = 60 Hz refresh rate */
     0x23,                      /* miscOutput register */
     {
      0xCF, 0x9F, 0xA0, 0x92, 0xAA, 0x19, 0x28, 0x52,
      /* standard CRTC */
      0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x01, 0x04, 0xFF, 0xA0, 0x00, 0x00, 0x29, 0xE3, 0xFF},
     {
      0x00, 0x51, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00,
      /* extended CRTC */
      0x00, 0x00, 0x00, 0x03, 0x80, 0x6C, 0x00, 0x00}
     },
/*--------------------------------------------------------------------------*/
    {
     1280, 1024, 75,            /* 1280x1024 */
     135,
     /* 135 MHz clock = 75 Hz refresh rate */
     0x23,                      /* miscOutput register */
     {
      0xCE, 0x9F, 0x9F, 0x92, 0xA4, 0x15, 0x28, 0x52,
      /* standard CRTC */
      0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x01, 0x04, 0xFF, 0xA0, 0x00, 0x00, 0x29, 0xE3, 0xFF},
     {
      0x00, 0x51, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00,
      /* extended CRTC */
      0x00, 0x00, 0x00, 0x03, 0x80, 0x87, 0x00, 0x00}
     },
/*--------------------------------------------------------------------------*/
    {
     1280, 1024, 85,            /* 1280x1024 */
     159,
     /* 159 MHz clock = 85 Hz refresh rate */
     0x2B,                      /* miscOutput register */
     {
      0xD3, 0x9F, 0xA0, 0x98, 0xA8, 0x9C, 0x2E, 0x5A,
      /* standard CRTC */
      0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x01, 0x04, 0xFF, 0x00, 0x00, 0xFF, 0x30, 0xE3, 0xFF},
     {
      0x6B, 0x41, 0x00, 0x03, 0x00, 0x01, 0x00, 0x00,
      /* extended CRTC */
      0x00, 0x00, 0x01, 0x00, 0x80, 0x9D, 0x00, 0x4B}
     },
/*--------------------------------------------------------------------------*/
};

#define GFX_VGA_MODES sizeof(gfx_vga_modes)/sizeof(gfx_vga_struct)

/*----------------------------------------------------------------------------
 * gfx_get_softvga_active
 *
 * This returns the active status of SoftVGA
 *----------------------------------------------------------------------------
 */
int
gfx_get_softvga_active(void)
{
    unsigned short crtcindex, crtcdata;

    if (gu1_detect_vsa2())
        return (gfx_get_vsa2_softvga_enable());

    crtcindex = (INB(0x3CC) & 0x01) ? 0x3D4 : 0x3B4;
    crtcdata = crtcindex + 1;

    OUTB(crtcindex, CRTC_MODE_SWITCH_CONTROL);
    return (INB(crtcdata) & 0x1);
}

/*----------------------------------------------------------------------------
 * gfx_vga_test_pci
 *
 * This routine looks for the VGA PCI header.  It checks to see that bit 1 
 * of the command register is writable to know that SoftVGA is trapping
 * the PCI config cuscles.  If SoftVGA is not emulating the header, the 
 * hardware will still respond with the proper device ID, etc.  
 * 
 * We need to know that SoftVGA is really there so that we can set the 
 * command register and have the proper effect (enable trapping of VGA).  
 * Otherwise, if we enable VGA via the PCI header, trapping really won't be 
 * enabled and the VGA register writes will go out to the external card.
 *----------------------------------------------------------------------------
 */
int
gfx_vga_test_pci(void)
{
    int softvga = 1;
    unsigned long value;

    value = gfx_pci_config_read(0x80009400);
    if ((value & 0x0000FFFF) != 0x1078)
        softvga = 0;
    else {
        value = gfx_pci_config_read(0x80009404);
        gfx_pci_config_write(0x80009404, value | 0x02);
        if (!(gfx_pci_config_read(0x80009404) & 0x02))
            softvga = 0;
        gfx_pci_config_write(0x80009404, value);
    }
    return (softvga);
}

/*----------------------------------------------------------------------------
 * gfx_vga_get_pci_command
 *
 * This routine returns the value of the PCI command register.
 *----------------------------------------------------------------------------
 */
unsigned char
gfx_vga_get_pci_command(void)
{
    unsigned long value;

    value = gfx_pci_config_read(0x80009404);
    return ((unsigned char) value);
}

/*----------------------------------------------------------------------------
 * gfx_vga_set_pci_command
 *
 * This routine writes the value of the PCI command register.  It is used 
 * to enable or disable SoftVGA.
 *
 *    Bit 0: Enable VGA IO
 *    Bit 1: Enable VGA memory
 *----------------------------------------------------------------------------
 */
int
gfx_vga_set_pci_command(unsigned char command)
{
    unsigned long value;

    value = gfx_pci_config_read(0x80009404) & 0xFFFFFF00;
    value |= (unsigned long) command;
    gfx_pci_config_write(0x80009404, value);
    return (GFX_STATUS_OK);
}

/*----------------------------------------------------------------------------
 * gfx_vga_seq_reset
 *
 * This routine enables or disables SoftVGA.  It is used to make SoftVGA 
 * "be quiet" and not interfere with any of the direct hardware access from 
 * Durango.  For VSA1, the sequencer is reset to stop text redraws.  VSA2 may 
 * provide a better way to have SoftVGA sit in the background.
 *----------------------------------------------------------------------------
 */
int
gfx_vga_seq_reset(int reset)
{
    OUTB(0x3C4, 0);
    OUTB(0x3C5, (unsigned char) (reset ? 0x00 : 0x03));
    return (GFX_STATUS_OK);
}

/*----------------------------------------------------------------------------
 * gfx_vga_set_graphics_bits
 *
 * This routine sets the standard VGA sequencer, graphics controller, and
 * attribute registers to appropriate values for a graphics mode (packed, 
 * 8 BPP or greater).  This is also known as "VESA" modes.  The timings for
 * a particular mode are handled by the CRTC registers, which are set by
 * the "gfx_vga_restore" routine.   Most OSs that use VGA to set modes save
 * and restore the standard VGA registers themselves, which is why these
 * registers are not part of the save/restore paradigm.
 *----------------------------------------------------------------------------
 */
int
gfx_vga_set_graphics_bits(void)
{
    /* SET GRAPHICS BIT IN GRAPHICS CONTROLLER REG 0x06 */

    OUTB(0x3CE, 0x06);
    OUTB(0x3CF, 0x01);

    /* SET GRAPHICS BIT IN ATTRIBUTE CONTROLLER REG 0x10 */

    INB(0x3BA);                 /* Reset flip-flop */
    INB(0x3DA);
    OUTB(0x3C0, 0x10);
    OUTB(0x3C0, 0x01);
    return (GFX_STATUS_OK);
}

/*----------------------------------------------------------------------------
 * gfx_vga_mode
 *
 * This routine searches the VGA mode table for a match of the specified 
 * mode and then fills in the VGA structure with the associated VGA register 
 * values.  The "gfx_vga_restore" routine can then be called to actually 
 * set the mode.
 *----------------------------------------------------------------------------
 */
int
gfx_vga_mode(gfx_vga_struct *vga, int xres, int yres, int bpp, int hz)
{
    unsigned int i;
    unsigned short pitch;

    for (i = 0; i < GFX_VGA_MODES; i++) {
        if ((gfx_vga_modes[i].xsize == xres) &&
            (gfx_vga_modes[i].ysize == yres) && (gfx_vga_modes[i].hz == hz)) {
            /* COPY ENTIRE STRUCTURE FROM THE TABLE */

            *vga = gfx_vga_modes[i];

            /* SET PITCH TO 1K OR 2K */
            /* CRTC_EXTENDED_OFFSET index is 0x45, so offset = 0x05 */

            pitch = (unsigned short) xres;
            if (bpp > 8)
                pitch <<= 1;
            if (pitch <= 1024)
                pitch = 1024 >> 3;
            else
                pitch = 2048 >> 3;
            vga->stdCRTCregs[0x13] = (unsigned char) pitch;
            vga->extCRTCregs[0x05] = (unsigned char) ((pitch >> 8) & 0x03);

            /* SET PROPER COLOR DEPTH VALUE */
            /* CRTC_EXTENDED_COLOR_CONTROL index is 0x46, so offset = 0x06 */

            switch (bpp) {
            case 15:
                vga->extCRTCregs[0x06] = CRTC_BIT_16BPP | CRTC_BIT_555;
                break;
            case 16:
                vga->extCRTCregs[0x06] = CRTC_BIT_16BPP;
                break;
            default:
                vga->extCRTCregs[0x06] = 0;
                break;
            }
            return (GFX_STATUS_OK);
        }
    }
    return (GFX_STATUS_UNSUPPORTED);
}

/*----------------------------------------------------------------------------
 * gfx_vga_pitch
 *
 * This routine updates the VGA regisers in the specified VGA structure for 
 * the specified pitch.  It does not program the hardware.
 *----------------------------------------------------------------------------
 */
int
gfx_vga_pitch(gfx_vga_struct *vga, unsigned short pitch)
{
    pitch >>= 3;
    vga->stdCRTCregs[0x13] = (unsigned char) pitch;
    vga->extCRTCregs[0x05] = (unsigned char) ((pitch >> 8) & 0x03);
    return (0);
}

/*----------------------------------------------------------------------------
 * gfx_vga_save
 *
 * This routine saves the state of the VGA registers into the specified 
 * structure.  Flags indicate what portions of the register state need to 
 * be saved.
 *----------------------------------------------------------------------------
 */
int
gfx_vga_save(gfx_vga_struct *vga, int flags)
{
    int i;
    unsigned short crtcindex, crtcdata;

    crtcindex = (INB(0x3CC) & 0x01) ? 0x3D4 : 0x3B4;
    crtcdata = crtcindex + 1;

    /* CHECK MISCELLANEOUS OUTPUT FLAG */

    if (flags & GFX_VGA_FLAG_MISC_OUTPUT) {
        /* SAVE MISCCELLANEOUS OUTPUT REGISTER */

        vga->miscOutput = INB(0x3CC);
    }

    /* CHECK STANDARD CRTC FLAG */

    if (flags & GFX_VGA_FLAG_STD_CRTC) {
        /* SAVE STANDARD CRTC REGISTERS */

        for (i = 0; i < GFX_STD_CRTC_REGS; i++) {
            OUTB(crtcindex, (unsigned char) i);
            vga->stdCRTCregs[i] = INB(crtcdata);
        }
    }

    /* CHECK EXTENDED CRTC FLAG */

    if (flags & GFX_VGA_FLAG_EXT_CRTC) {
        /* SAVE EXTENDED CRTC REGISTERS */

        for (i = 0; i < GFX_EXT_CRTC_REGS; i++) {
            OUTB(crtcindex, (unsigned char) (0x40 + i));
            vga->extCRTCregs[i] = INB(crtcdata);
        }
    }
    return (0);
}

/*----------------------------------------------------------------------------
 * gfx_vga_clear_extended
 *
 * This routine clears the extended SoftVGA register values to have SoftVGA
 * behave like standard VGA. 
 *----------------------------------------------------------------------------
 */
void
gfx_vga_clear_extended(void)
{
    int i;
    unsigned short crtcindex, crtcdata;

    crtcindex = (INB(0x3CC) & 0x01) ? 0x3D4 : 0x3B4;
    crtcdata = crtcindex + 1;

    OUTB(crtcindex, 0x30);
    OUTB(crtcdata, 0x57);
    OUTB(crtcdata, 0x4C);
    for (i = 0x40; i <= 0x4F; i++) {
        OUTB(crtcindex, (unsigned char) i);
        OUTB(crtcdata, 0);
    }
    OUTB(crtcindex, 0x30);
    OUTB(crtcdata, 0x00);
}

/*----------------------------------------------------------------------------
 * gfx_vga_restore
 *
 * This routine restores the state of the VGA registers from the specified
 * structure.  Flags indicate what portions of the register state need to 
 * be saved.
 *----------------------------------------------------------------------------
 */
int
gfx_vga_restore(gfx_vga_struct *vga, int flags)
{
    int i;
    unsigned short crtcindex, crtcdata;

    crtcindex = (INB(0x3CC) & 0x01) ? 0x3D4 : 0x3B4;
    crtcdata = crtcindex + 1;

    /* CHECK MISCELLANEOUS OUTPUT FLAG */

    if (flags & GFX_VGA_FLAG_MISC_OUTPUT) {
        /* RESTORE MISCELLANEOUS OUTPUT REGISTER VALUE */

        OUTB(0x3C2, vga->miscOutput);
    }

    /* CHECK STANDARD CRTC FLAG */

    if (flags & GFX_VGA_FLAG_STD_CRTC) {
        /* UNLOCK STANDARD CRTC REGISTERS */

        OUTB(crtcindex, 0x11);
        OUTB(crtcdata, 0);

        /* RESTORE STANDARD CRTC REGISTERS */

        for (i = 0; i < GFX_STD_CRTC_REGS; i++) {
            OUTB(crtcindex, (unsigned char) i);
            OUTB(crtcdata, vga->stdCRTCregs[i]);
        }
    }

    /* CHECK EXTENDED CRTC FLAG */

    if (flags & GFX_VGA_FLAG_EXT_CRTC) {
        /* UNLOCK EXTENDED CRTC REGISTERS */

        OUTB(crtcindex, 0x30);
        OUTB(crtcdata, 0x57);
        OUTB(crtcdata, 0x4C);

        /* RESTORE EXTENDED CRTC REGISTERS */

        for (i = 0; i < GFX_EXT_CRTC_REGS; i++) {
            OUTB(crtcindex, (unsigned char) (0x40 + i));
            OUTB(crtcdata, vga->extCRTCregs[i]);
        }

        /* LOCK EXTENDED CRTC REGISTERS */

        OUTB(crtcindex, 0x30);
        OUTB(crtcdata, 0x00);

        /* CHECK IF DIRECT FRAME BUFFER MODE (VESA MODE) */

        if (vga->extCRTCregs[0x03] & 1) {
            /* SET BORDER COLOR TO BLACK */
            /* This really should be another thing saved/restored, but */
            /* Durango currently doesn't do the attr controller registers. */

            INB(0x3BA);         /* Reset flip-flop */
            INB(0x3DA);
            OUTB(0x3C0, 0x11);
            OUTB(0x3C0, 0x00);
        }
    }
    return (0);
}

/*----------------------------------------------------------------------------
 * gfx_vga_mode_switch
 *
 * This routine programs the SoftVGA register to indicate that a mode switch
 * is in progress.  This results in a cleaner mode switch since SoftVGA will 
 * not validate the hardware with intermediate values.
 *----------------------------------------------------------------------------
 */
int
gfx_vga_mode_switch(int active)
{
    unsigned short crtcindex, crtcdata;

    crtcindex = (INB(0x3CC) & 0x01) ? 0x3D4 : 0x3B4;
    crtcdata = crtcindex + 1;

    /* UNLOCK EXTENDED CRTC REGISTERS */

    OUTB(crtcindex, CRTC_EXTENDED_REGISTER_LOCK);
    OUTB(crtcdata, 0x57);
    OUTB(crtcdata, 0x4C);

    /* SIGNAL THE BEGINNING OR END OF THE MODE SWITCH */
    /* SoftVGA will hold off validating the back end hardware. */

    OUTB(crtcindex, CRTC_MODE_SWITCH_CONTROL);
    active = active ? 1 : 0;
    OUTB(crtcdata, (unsigned char) active);

    /* WAIT UNTIL SOFTVGA HAS VALIDATED MODE IF ENDING MODE SWITCH */
    /* This is for VSA1 only, where SoftVGA waits until the next */
    /* vertical blank to validate the hardware state. */

    if ((!active) && (!(gu1_detect_vsa2()))) {
        OUTB(crtcindex, 0x33);
        while (INB(crtcdata) & 0x80);
    }

    /* LOCK EXTENDED CRTC REGISTERS */

    OUTB(crtcindex, CRTC_EXTENDED_REGISTER_LOCK);
    OUTB(crtcdata, 0x00);
    return (0);
}

/*----------------------------------------------------------------------------
 * gu1_detect_vsa2
 *
 * This routine detects if VSA2 is present.  The interface to SoftVGA 
 * changed slightly.
 *----------------------------------------------------------------------------
 */
int
gu1_detect_vsa2(void)
{
    unsigned short crtcindex, crtcdata;

    crtcindex = (INB(0x3CC) & 0x01) ? 0x3D4 : 0x3B4;
    crtcdata = crtcindex + 1;
    OUTB(crtcindex, 0x35);
    if (INB(crtcdata) != 'C')
        return (0);
    OUTB(crtcindex, 0x36);
    if (INB(crtcdata) != 'X')
        return (0);
    return (1);
}

/* END OF FILE */

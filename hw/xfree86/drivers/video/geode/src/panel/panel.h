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
 * File Contents:   This file contains the Geode frame buffer panel functions
 *                  prototypes and it includes panel definitions header file.
 * 
 * SubModule:       Geode FlatPanel library
 * */

/* COMPILER OPTION FOR C++ PROGRAMS */

#ifndef _panel_h
#define _panel_h

#include "pnl_defs.h"

#ifdef __cplusplus
extern "C" {
#endif

/* CLOSE BRACKET FOR C++ COMPLILATION */

    void Pnl_SetPlatform(int platform);
    int Pnl_GetPlatform(void);
    int Pnl_IsPanelPresent(void);
    void Pnl_SetPanelPresent(int present);
    void Pnl_SetPanelChip(int panelChip);
    int Pnl_GetPanelChip(void);
    void Pnl_SetPanelParam(Pnl_PanelParams * pParam);
    void Pnl_GetPanelParam(Pnl_PanelParams * pParam);
    int Pnl_InitPanel(Pnl_PanelParams * pParam);
    int Detect_Platform(void);
    void Pnl_SavePanelState(void);
    void Pnl_RestorePanelState(void);
    void Pnl_PowerUp(void);
    void Pnl_PowerDown(void);

    int Pnl_IsPanelEnabledInBIOS(void);
    void Pnl_GetPanelInfoFromBIOS(int *xres, int *yres, int *bpp, int *hz);
/* from durango */

#if defined(_WIN32)             /* windows */
    extern void gfx_delay_milliseconds(unsigned long milliseconds);
    extern unsigned long gfx_ind(unsigned short port);
    extern void gfx_outd(unsigned short port, unsigned long data);
    extern unsigned char gfx_inb(unsigned short port);
    extern void gfx_outb(unsigned short port, unsigned char data);
#endif

#ifdef __cplusplus
}
#endif
#endif                          /* !_panel_h */
/* END OF FILE */

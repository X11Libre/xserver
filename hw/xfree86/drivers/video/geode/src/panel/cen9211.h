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
 * File Contents:   This header file defines the Durango routines and 
 *                  variables used to access the memory mapped regions.
 * 
 * SubModule:       Geode FlatPanel library
 * */

#include "92xx.h"
#include "panel.h"

#ifndef _CEN9211_h
#define _CEN9211_h

/* Centaurus/97317 GPIO PORT defines  */

#define CENT_97317_CLOCK_PORT		0xE0
#define CENT_97317_DATA_OUTPORT		0xE0
#define CENT_97317_CHIP_SELECT		0xE4
#define CENT_97317_DATA_INPORT		0xE4
#define CENT_PORT1_DIRECTION		0xE1
#define CENT_PORT2_DIRECTION		0xE5
#define CENT_PORT1_OUTPUT_TYPE		0xE2
#define CENT_PORT2_OUTPUT_TYPE		0xE6

/*
 * Centaurus/97317 GPIO bit masks. 
 * In and out are with respect to the 97317. 
 */

#define CENT_97317_CLOCK_MASK		0x10
#define CENT_97317_CHIP_SEL_MASK	0x01
#define CENT_97317_DATA_IN_MASK		0x02
#define CENT_97317_DATA_OUT_MASK	0x80

#define CENT_PASS    1
#define CENT_FAIL    0
#define CENT_READ    0x0000
#define CENT_WRITE   0x0001
#define CENT_NO_DATA 0

#define CENT_CONFIG_INDEX  0x2E
#define CENT_SIOC2         0x22
#define CENT_CONFIG_DATA   0x2F
#define CENT_GPIO_BANK_SELECT	0x80

#define CENT_NUM_READ_CLOCK_TOGGLES	1
#define CENT_NUM_WRITE_CLOCK_TOGGLES	4

/* local functions */
void set_Centaurus_92xx_mode_params(int mode);
void enable_Centaurus_9211_chip_select(void);
void disable_Centaurus_9211_chip_select(void);
void toggle_Centaurus_9211_clock(void);
void write_Centaurus_CX9211_GPIO(unsigned char databit);
void write_Centaurus_CX9211_DWdata(unsigned long data);
void Centaurus_write_gpio(int width, unsigned long address, unsigned long data);
void Centaurus_Power_Up(void);
void Centaurus_Power_Down(void);
unsigned long Centaurus_read_gpio(int width, unsigned long address);
unsigned char read_Centaurus_CX9211_GPIO(void);
unsigned long read_Centaurus_CX9211_DWdata(void);
unsigned char restore_Centaurus_97317_SIOC2(void);
unsigned char init_Centaurus_GPIO(void);
unsigned char init_Centaurus_9211(void);
unsigned char set_Centaurus_92xx_mode(Pnl_PanelStat * pstat);
void CentaurusProgramFRMload(void);
void Centaurus_Get_9211_Details(unsigned long flags, Pnl_PanelParams * pParam);
void Centaurus_Save_Panel_State(void);
void Centaurus_Restore_Panel_State(void);
void Centaurus_9211init(Pnl_PanelStat * pstat);

#endif                          /* !_CEN9211_h */

/* END OF FILE */

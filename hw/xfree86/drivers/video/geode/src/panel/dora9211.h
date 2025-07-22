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

#include "panel.h"
#include "92xx.h"

#ifndef _DORA9211_h
#define _DORA9211_h

/* 9211 Rev.C3 Dorado */

/* GPIO Pin Configuration Registers */

#define DRD_GEODE_GPPIN_SEL	0x20    /* GPIO Pin Configuration Select        */
#define DRD_GEODE_GPPIN_CFG	0x24    /* GPIO Pin Configuration Access        */
#define DRD_GEODE_GPPIN_RESET	0x28    /* GPIO Pin Reset                                       */

#define DRD_GEODE_GPIO_BASE	0x6400  /* F0 GPIO, IO mapped                           */
#define DRD_GEODE_GPDI0		0x04    /* GPIO Data In 0                                       */
#define DRD_GEODE_GPDO0		0x00    /* GPIO Data Out 0                                      */

/* Data Ports in */
#define	DRD_CLOCKP9211IN	DRD_GEODE_GPIO_BASE + DRD_GEODE_GPDI0
#define	DRD_DATAINP9211IN	DRD_GEODE_GPIO_BASE + DRD_GEODE_GPDI0
#define	DRD_DATAOUTP9211IN	DRD_GEODE_GPIO_BASE + DRD_GEODE_GPDI0
#define	DRD_CSP9211IN		DRD_GEODE_GPIO_BASE + DRD_GEODE_GPDI0

/* Data Ports out */
#define	DRD_CLOCKP9211OUT	DRD_GEODE_GPIO_BASE + DRD_GEODE_GPDO0
#define	DRD_DATAINP9211OUT	DRD_GEODE_GPIO_BASE + DRD_GEODE_GPDO0
#define	DRD_DATAOUTP9211OUT	DRD_GEODE_GPIO_BASE + DRD_GEODE_GPDO0
#define DRD_CSP9211OUT		DRD_GEODE_GPIO_BASE + DRD_GEODE_GPDO0

/* Pin MASKS */
#define	DRD_CLOCK9211	0x00000080      /* gpio 7, clock output to 9211         */
#define	DRD_DATAIN9211	0x00040000      /* gpio 18, data output to 9211         */
#define	DRD_DATAOUT9211	0x00000800      /* gpio 11, data input from 9211        */
#define	DRD_CS9211	0x00000200      /* gpio 9, chip select output to 9211
                                         * */

/* Gpio CFG values to select in  */
#define	DRD_CLOCK9211CFG	0x00000007      /* gpio 7                                                       */
#define	DRD_DATAIN9211CFG	0x00000012      /* gpio 18                                                  */
#define	DRD_DATAOUT9211CFG	0x0000000B      /* gpio 11                                                  */
#define	DRD_CS9211CFG		0x00000009      /* gpio 9                                                   */

#define DRD_LCDRESGPIO1		0x00
#define DRD_LCDRESGPIO2		0x01
#define DRD_LCDRESGPIO3		0x02
#define DRD_LCDRESGPIO4		0x04

void Dorado9211SetCS(void);
void Dorado9211ClearCS(void);
void Dorado9211SetDataOut(void);
void Dorado9211ClearDataOut(void);
unsigned char Dorado9211ReadDataIn(void);
void Dorado9211ToggleClock(void);
void Dorado9211SetClock(void);
void Dorado9211ClearClock(void);
void Dorado9211GpioInit(void);
unsigned long Dorado9211ReadReg(unsigned short index);
void Dorado9211WriteReg(unsigned short index, unsigned long data);
void DoradoProgramFRMload(void);
void Dorado_Get_9211_Details(unsigned long flags, Pnl_PanelParams * pParam);
void Dorado_Power_Up(void);
void Dorado_Power_Down(void);
void Dorado_Save_Panel_State(void);
void Dorado_Restore_Panel_State(void);
void Dorado9211Init(Pnl_PanelStat * pstat);

#endif                          /* !_DORA9211_h */

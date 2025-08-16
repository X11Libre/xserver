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
 * File Contents:   This file contains the panel library files to the 
 *                  platforms with 9210, and 9211 support.
 *                	
 * SubModule:       Geode FlatPanel library
 * */

#include "drac9210.h"

#define	CS9210			0x40    /* Chip select pin       */

/* 9210 on Draco */
#define CLOCK9210		0x04    /* Clock pin             */
#define	DATAIN9210		0x20    /* Data from 9210        */
#define	DATAOUT9210		0x80    /* Data to 9210          */

static void DracoWriteData(unsigned char data);
static void DracoReadData(unsigned char *data);
static void Draco9210GpioInit();
static void Draco9210SetCS(void);
static unsigned char Draco9210ReadReg(unsigned char index);
static void Draco9210WriteReg(unsigned char index, unsigned char data);
static void Draco9210ClearCS(void);
static void Draco9210SetDataOut(void);
static void Draco9210ClearDataOut(void);
static unsigned char Draco9210ReadDataIn(void);
static void Draco9210ToggleClock(void);

void
Draco9210Init(Pnl_PanelStat *pstat)
{
    unsigned char panelvalues[] = {
        0x2, 0x80,
        0x2, 0x24,
        0x03, 0x00,
        0xc0, 0x00,
        0xc1, 0x00,
        0xc2, 0x00,
        0xc3, 0x00,
        0xc4, 0x00,
        0xc5, 0x01,
        0xc6, 0xff,
        0xc7, 0xff,
        0xc8, 0x3,
        0xc9, 0xfe,
        0xca, 0x0,
        0xcb, 0x3f,
        0xcc, 0xc,
        0xcd, 0x1,
        0xce, 0xff,
        0xcf, 0xc1,
        0xd0, 0x0,
        0xd1, 0x7e,
        0xd2, 0x3,
        0xd3, 0xfe,
        0xd4, 0x3,
        0xd5, 0x81,
        0xd6, 0xfc,
        0xd7, 0x3f,
        0xd8, 0x14,
        0xd9, 0x1e,
        0xda, 0x0f,
        0xdb, 0xc7,
        0xdc, 0x29,
        0xdd, 0xe1,
        0xde, 0xf1,
        0xdf, 0xf9,
        0xe0, 0x2,
        0xe1, 0xe,
        0xe2, 0x1e,
        0xe3, 0x3e,
        0xe4, 0x04,
        0xe5, 0x71,
        0xe6, 0xe3,
        0xe7, 0xcf,
        0xe8, 0x1,
        0xe9, 0x86,
        0xea, 0x3c,
        0xeb, 0xf3,
        0xec, 0xa,
        0xed, 0x39,
        0xee, 0xc7,
        0xef, 0x3d,

        0xf0, 0x14,
        0xf1, 0xc6,
        0xf2, 0x39,
        0xf3, 0xce,
        0xf4, 0x3,
        0xf5, 0x19,
        0xf6, 0xce,
        0xf7, 0x77,
        0xf8, 0x0,
        0xf9, 0x66,
        0xfa, 0x33,
        0xfb, 0xbb,
        0xfc, 0x2d,
        0xfd, 0x99,
        0xfe, 0xdd,
        0xff, 0xdd,

        0x3, 0x1,
        0xc0, 0x2,
        0xc1, 0x22,
        0xc2, 0x66,
        0xc3, 0x66,
        0xc4, 0x0,
        0xc5, 0xcd,
        0xc6, 0x99,
        0xc7, 0xbb,
        0xc8, 0x5,
        0xc9, 0x32,
        0xca, 0x66,
        0xcb, 0xdd,
        0xcc, 0x1a,
        0xcd, 0x4d,
        0xce, 0x9b,
        0xcf, 0x6f,
        0xd0, 0x0,
        0xd1, 0x92,
        0xd2, 0x6d,
        0xd3, 0xb6,
        0xd4, 0x5,
        0xd5, 0x25,
        0xd6, 0xb6,
        0xd7, 0xdb,
        0xd8, 0x2,
        0xd9, 0x5a,
        0xda, 0x4b,
        0xdb, 0x6d,
        0xdc, 0x29,
        0xdd, 0xa5,
        0xde, 0xb5,
        0xdf, 0xb7,
        0xe0, 0x4,
        0xe1, 0x4a,
        0xe2, 0x5a,
        0xe3, 0xda,
        0xe4, 0x12,
        0xe5, 0x95,
        0xe6, 0xad,
        0xe7, 0x6f,
        0xe8, 0x1,
        0xe9, 0x2a,
        0xea, 0x56,
        0xeb, 0xb5,
        0xec, 0xe,
        0xed, 0x55,
        0xee, 0xab,
        0xef, 0x5f,
        0xf0, 0x0,
        0xf1, 0xaa,
        0xf2, 0x55,
        0xf3, 0xea,
        0xf4, 0x1,
        0xf5, 0x55,
        0xf6, 0xaa,
        0xf7, 0xbf,
        0xf8, 0x6,
        0xf9, 0xaa,
        0xfa, 0x55,
        0xfb, 0x55,
        0xfc, 0x39,
        0xfd, 0x55,
        0xfe, 0xff,
        0xff, 0xff,

        0x3, 0x2,
        0xc0, 0x0,
        0xc1, 0x0,
        0xc2, 0xaa,
        0xc3, 0xaa,
        0xc4, 0x6,
        0xc5, 0xab,
        0xc6, 0x55,
        0xc7, 0x55,
        0xc8, 0x01,
        0xc9, 0x54,
        0xca, 0xaa,
        0xcb, 0xbf,
        0xcc, 0x8,
        0xcd, 0xab,
        0xce, 0x55,
        0xcf, 0xeb,
        0xd0, 0x6,
        0xd1, 0x54,
        0xd2, 0xab,
        0xd3, 0x5e,
        0xd4, 0x1,
        0xd5, 0x2b,
        0xd6, 0x56,
        0xd7, 0xb5,
        0xd8, 0x12,
        0xd9, 0x94,
        0xda, 0xad,
        0xdb, 0x6f,
        0xdc, 0x2d,
        0xdd, 0x4b,
        0xde, 0x5b,
        0xdf, 0xdb,
        0xe0, 0x0,
        0xe1, 0xa4,
        0xe2, 0xb4,
        0xe3, 0xb6,
        0xe4, 0x2,
        0xe5, 0x5b,
        0xe6, 0x4b,
        0xe7, 0x6d,
        0xe8, 0x5,
        0xe9, 0x24,
        0xea, 0xb6,
        0xeb, 0xdb,
        0xec, 0x8,
        0xed, 0x93,
        0xee, 0x6d,
        0xef, 0xb7,
        0xf0, 0x12,
        0xf1, 0x4c,
        0xf2, 0x9b,
        0xf3, 0x6e,
        0xf4, 0x5,
        0xf5, 0x33,
        0xf6, 0x66,
        0xf7, 0xdd,
        0xf8, 0x0,
        0xf9, 0xcc,
        0xfa, 0x99,
        0xfb, 0xbb,
        0xfc, 0x2b,
        0xfd, 0x33,
        0xfe, 0x77,
        0xff, 0x77,

        0x3, 0x3,
        0xc0, 0x4,
        0xc1, 0x88,
        0xc2, 0xcc,
        0xc3, 0xcc,
        0xc4, 0x0,
        0xc5, 0x67,
        0xc6, 0x33,
        0xc7, 0xbb,
        0xc8, 0x3,
        0xc9, 0x18,
        0xca, 0xce,
        0xcb, 0x77,
        0xcc, 0x1c,
        0xcd, 0xc7,
        0xce, 0x39,
        0xcf, 0xcf,

        0xd0, 0x2,
        0xd1, 0x38,
        0xd2, 0xc7,
        0xd3, 0x3c,
        0xd4, 0x1,
        0xd5, 0x87,
        0xd6, 0x3c,
        0xd7, 0xf3,
        0xd8, 0x4,
        0xd9, 0x70,
        0xda, 0xe3,
        0xdb, 0xcf,
        0xdc, 0x2b,
        0xdd, 0xf,
        0xde, 0x1f,
        0xdf, 0x3f,
        0xe0, 0x00,
        0xe1, 0xe0,
        0xe2, 0xf0,
        0xe3, 0xf8,
        0xe4, 0x14,
        0xe5, 0x1f,
        0xe6, 0xf,
        0xe7, 0xc7,
        0xe8, 0x3,
        0xe9, 0x80,
        0xea, 0xfc,
        0xeb, 0x3f,
        0xec, 0x8,
        0xed, 0x7f,
        0xee, 0x3,
        0xef, 0xff,
        0xf0, 0x4,
        0xf1, 0x0,
        0xf2, 0xff,
        0xf3, 0xc0,
        0xf4, 0x3,
        0xf5, 0xff,
        0xf6, 0x0,
        0xf7, 0x3f,
        0xf8, 0x0,
        0xf9, 0x0,
        0xfa, 0xff,
        0xfb, 0xff,
        0xfc, 0x3f,
        0xfd, 0xff,
        0xfe, 0xff,
        0xff, 0xff,
        0x3, 0x4,

        /* Setup the Diter to Pattern33 */
        0x80, 0xdd,
        0x81, 0xdd,
        0x82, 0x33,
        0x83, 0x33,
        0x84, 0xdd,
        0x85, 0xdd,
        0x86, 0x33,
        0x87, 0x33,
        0x88, 0x33,
        0x89, 0x33,
        0x8a, 0x77,
        0x8b, 0x77,
        0x8c, 0x33,
        0x8d, 0x33,
        0x8e, 0x77,
        0x8f, 0x77,
        0x90, 0xdd,
        0x91, 0xdd,
        0x92, 0x33,
        0x93, 0x33,
        0x94, 0xdd,
        0x95, 0xdd,
        0x96, 0x33,
        0x97, 0x33,
        0x98, 0x33,
        0x99, 0x33,
        0x9a, 0x77,
        0x9b, 0x77,
        0x9c, 0x33,
        0x9d, 0x33,
        0x9e, 0x77,
        0x9f, 0x77,

        0x4, 0x20,
        0x5, 0x3,
        0x6, 0x56,
        0x7, 0x2,
        0x8, 0x1c,
        0x9, 0x0,
        0xa, 0x26,
        0xb, 0x0,
        0xc, 0x15,
        0xd, 0x4,
        0xe, 0x50,
        0xf, 0x4,
        0x10, 0xfa,
        0x11, 0x0,
        0x12, 0xc8,
        0x13, 0x0,
        0x14, 0x31,
        0x15, 0x23,
        0x16, 0x0,

        /* Enable DSTN panel */
        0x2, 0x64
    };
    unsigned char index, data;
    int i;

    gfx_delay_milliseconds(100);
    Draco9210GpioInit();
    Draco9210SetCS();
    Draco9210ToggleClock();
    Draco9210ToggleClock();
    Draco9210ToggleClock();
    Draco9210ToggleClock();
    Draco9210ClearCS();

#if defined(_WIN32)             /* For Windows   */
    for (i = 0; i < 10; i++) {
        _asm {
        out 0ED h, al}
    }

#elif defined(linux)            /* Linux                 */

#endif

    for (i = 0; i < 630; i += 2) {
        index = panelvalues[i];
        data = panelvalues[i + 1];
        Draco9210WriteReg(index, data);
    }

}

static void
DracoWriteData(unsigned char data)
{
    int i;
    unsigned char mask = 0x80, databit;

    for (i = 0; i < 8; i++) {

        databit = data & mask;
        if (data & mask) {
            Draco9210SetDataOut();
        }
        else {
            Draco9210ClearDataOut();
        }
        mask >>= 1;
        Draco9210ToggleClock();
    }
}

static void
DracoReadData(unsigned char *data)
{
    int i;
    unsigned char tmp = 0, readbit;

    Draco9210ClearDataOut();
    Draco9210ToggleClock();
    for (i = 0; i < 7; i++) {
        readbit = Draco9210ReadDataIn();
        tmp |= (readbit & 0x1);
        tmp <<= 1;
        Draco9210ToggleClock();
    }
    readbit = Draco9210ReadDataIn();
    tmp |= (readbit & 0x1);
    *data = tmp;
}

#if defined(_WIN32)             /* For Windows */

void
Draco9210GpioInit()
{
    _asm {
    pushf
            cli
            mov dx, 0 CF8h
            mov eax, CX55x0_ID + 090 h
            out dx, eax
            mov dx, 0 CFCh mov al, 0 CFh mov ah, 00 h out dx, ax popf}
}

void
Draco9210SetCS()
{
    _asm {
        pushf;
        Point to PCI address register mov dx, 0 CF8h;
        55 XX GPIO data register mov eax, CX55x0_ID + 090 h out dx, eax;
        Point to PCI data register (CFCh)
mov dx, 0 CFCh
            in ax, dx
            and ah, 30 h
            mov ah, c92DataReg
            or ah, CS9210 mov c92DataReg, ah out dx, ax popf}} void
Draco9210ClearCS()
{
    _asm {
        pushf;
        Point to PCI address register mov dx, 0 CF8h;
        55 XX GPIO data register mov eax, CX55x0_ID + 090 h out dx, eax;
        Point to PCI data register (CFCh)
        mov dx, 0 CFCh;
Set CS LOW
            in ax, dx
            mov ah, c92DataReg
            and ah, NOT CS9210 mov c92DataReg, ah out dx, ax popf}} void
Draco9210SetDataOut()
{
    _asm {
        pushf;
        Point to PCI address register mov dx, 0 CF8h;
        55 XX GPIO data register mov eax, CX55x0_ID + 090 h out dx, eax;
        Point to PCI data register (CFCh)
        mov dx, 0 CFCh;
Set DATA HIGH
            in ax, dx
            mov ah, c92DataReg
            or ah, DATAOUT9210 mov c92DataReg, ah out dx, ax popf}} void
Draco9210ClearDataOut()
{
    _asm {
        pushf;
        Point to PCI address register
        mov dx, 0 CF8h mov eax, CX55x0_ID + 090 h;

        ;
        55 XX GPIO data register out dx, eax;
        Point to PCI data register (CFCh)
        mov dx, 0 CFCh;
Set Data LOW
            in ax, dx
            mov ah, c92DataReg
            and ah, NOT DATAOUT9210 mov c92DataReg, ah out dx, ax popf}}
    unsigned char
Draco9210ReadDataIn()
{
    unsigned char readdata;

    _asm {
        pushf;
        Point to PCI address register mov dx, 0 CF8h;
        55 XX GPIO data register mov eax, CX55x0_ID + 090 h out dx, eax;
        Point to PCI data register (CFCh)
        mov dx, 0F Ch in ax, dx;
        Preserve just Data IN bit and ah, DATAIN9210 mov al, ah cmp al, 0;
        Is it LOW ? je readDataLow;
    must be HIGH mov al, 1 readDataLow:mov readdata, al popf} return (readdata);
}

void
Draco9210ToggleClock()
{
    _asm {
        pushf;
        Point to PCI address register mov dx, 0 CF8h;
        55 XX GPIO data register mov eax, CX55x0_ID + 090 h;
        Point to PCI data register (CFCh)
        out dx, eax mov dx, 0 CFCh;
        SET CLOCK in ax, dx mov ah, c92DataReg or ah, CLOCK9210 mov c92DataReg, ah out dx, ax out 0ED h, al     /* IOPAUSE      */
        ;
        Point to PCI address register mov dx, 0 CF8h;
        55 XX GPIO data register mov eax, CX55x0_ID + 090 h out dx, eax;
        Point to PCI data register (CFCh)
        mov dx, 0 CFCh;

        ;
CLEAR CLOCK
            in ax, dx
            mov ah, c92DataReg
            and ah, NOT CLOCK9210 mov c92DataReg, ah out dx, ax popf}}
#elif defined(linux)            /* Linux         */

void
Draco9210GpioInit()
{
}

void
Draco9210SetCS()
{
}

void
Draco9210ClearCS()
{
}

void
Draco9210SetDataOut()
{
}

void
Draco9210ClearDataOut()
{
}

unsigned char
Draco9210ReadDataIn()
{
}

void
Draco9210ToggleClock()
{
}

#endif

unsigned char
Draco9210ReadReg(unsigned char index)
{
    unsigned char data;

    Draco9210SetCS();
    Draco9210ToggleClock();
    Draco9210SetDataOut();
    Draco9210ToggleClock();
    Draco9210ClearDataOut();
    Draco9210ToggleClock();
    Draco9210ClearDataOut();
    Draco9210ToggleClock();
    Draco9210ClearDataOut();
    Draco9210ToggleClock();

    DracoWriteData(index);
    DracoReadData(&data);

    return (data);
}

void
Draco9210WriteReg(unsigned char index, unsigned char data)
{

    Draco9210SetCS();
    Draco9210ToggleClock();

    Draco9210SetDataOut();
    Draco9210ToggleClock();

    Draco9210ClearDataOut();
    Draco9210ToggleClock();

    Draco9210ClearDataOut();
    Draco9210ToggleClock();

    Draco9210SetDataOut();
    Draco9210ToggleClock();

    DracoWriteData(index);
    DracoWriteData(data);

    Draco9210ClearDataOut();
    Draco9210ToggleClock();

    Draco9210ClearCS();
    Draco9210ToggleClock();
    Draco9210ToggleClock();

}

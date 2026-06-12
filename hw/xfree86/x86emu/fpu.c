/* SPDX-License-Identifier: HPND-sell-variant */
/****************************************************************************
*
*						Realmode X86 Emulator Library
*
*            	Copyright (C) 1996-1999 SciTech Software, Inc.
* 				     Copyright (C) David Mosberger-Tang
* 					   Copyright (C) 1999 Egbert Eich
*
*  ========================================================================
*
*  Permission to use, copy, modify, distribute, and sell this software and
*  its documentation for any purpose is hereby granted without fee,
*  provided that the above copyright notice appear in all copies and that
*  both that copyright notice and this permission notice appear in
*  supporting documentation, and that the name of the authors not be used
*  in advertising or publicity pertaining to distribution of the software
*  without specific, written prior permission.  The authors makes no
*  representations about the suitability of this software for any purpose.
*  It is provided "as is" without express or implied warranty.
*
*  THE AUTHORS DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
*  INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
*  EVENT SHALL THE AUTHORS BE LIABLE FOR ANY SPECIAL, INDIRECT OR
*  CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF
*  USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
*  OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
*  PERFORMANCE OF THIS SOFTWARE.
*
*  ========================================================================
*
* Language:		ANSI C
* Environment:	Any
* Developer:    Kendall Bennett
*
* Description:  This file contains the code to implement the decoding and
*               emulation of the FPU instructions.
*
****************************************************************************/

#include "x86emu/x86emui.h"

/*----------------------------- Implementation ----------------------------*/

/* opcode=0xd8 */
void
x86emuOp_esc_coprocess_d8(u8 X86EMU_UNUSED(op1))
{
    START_OF_INSTR();
    DECODE_PRINTF("ESC D8\n");
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR_NO_TRACE();
}

#ifdef DEBUG

static const char *x86emu_fpu_op_d9_tab[] = {
    "FLD\tDWORD PTR ", "ESC_D9\t", "FST\tDWORD PTR ", "FSTP\tDWORD PTR ",
    "FLDENV\t", "FLDCW\t", "FSTENV\t", "FSTCW\t",

    "FLD\tDWORD PTR ", "ESC_D9\t", "FST\tDWORD PTR ", "FSTP\tDWORD PTR ",
    "FLDENV\t", "FLDCW\t", "FSTENV\t", "FSTCW\t",

    "FLD\tDWORD PTR ", "ESC_D9\t", "FST\tDWORD PTR ", "FSTP\tDWORD PTR ",
    "FLDENV\t", "FLDCW\t", "FSTENV\t", "FSTCW\t",
};

static const char *x86emu_fpu_op_d9_tab1[] = {
    "FLD\t", "FLD\t", "FLD\t", "FLD\t",
    "FLD\t", "FLD\t", "FLD\t", "FLD\t",

    "FXCH\t", "FXCH\t", "FXCH\t", "FXCH\t",
    "FXCH\t", "FXCH\t", "FXCH\t", "FXCH\t",

    "FNOP", "ESC_D9", "ESC_D9", "ESC_D9",
    "ESC_D9", "ESC_D9", "ESC_D9", "ESC_D9",

    "FSTP\t", "FSTP\t", "FSTP\t", "FSTP\t",
    "FSTP\t", "FSTP\t", "FSTP\t", "FSTP\t",

    "FCHS", "FABS", "ESC_D9", "ESC_D9",
    "FTST", "FXAM", "ESC_D9", "ESC_D9",

    "FLD1", "FLDL2T", "FLDL2E", "FLDPI",
    "FLDLG2", "FLDLN2", "FLDZ", "ESC_D9",

    "F2XM1", "FYL2X", "FPTAN", "FPATAN",
    "FXTRACT", "ESC_D9", "FDECSTP", "FINCSTP",

    "FPREM", "FYL2XP1", "FSQRT", "ESC_D9",
    "FRNDINT", "FSCALE", "ESC_D9", "ESC_D9",
};

#endif                          /* DEBUG */

/* opcode=0xd9 */
void
x86emuOp_esc_coprocess_d9(u8 X86EMU_UNUSED(op1))
{
    int mod, rl, rh;
    uint destoffset = 0;
    u8 stkelem = 0;

    START_OF_INSTR();
    FETCH_DECODE_MODRM(mod, rh, rl);
#ifdef DEBUG
    if (mod != 3) {
        DECODE_PRINTINSTR32(x86emu_fpu_op_d9_tab, mod, rh, rl);
    }
    else {
        DECODE_PRINTF(x86emu_fpu_op_d9_tab1[(rh << 3) + rl]);
    }
#endif
    switch (mod) {
    case 0:
        destoffset = decode_rm00_address(rl);
        DECODE_PRINTF("\n");
        break;
    case 1:
        destoffset = decode_rm01_address(rl);
        DECODE_PRINTF("\n");
        break;
    case 2:
        destoffset = decode_rm10_address(rl);
        DECODE_PRINTF("\n");
        break;
    case 3:                    /* register to register */
        stkelem = (u8) rl;
        if (rh < 4) {
            DECODE_PRINTF2("ST(%d)\n", stkelem);
        }
        else {
            DECODE_PRINTF("\n");
        }
        break;
    }
    (void) destoffset;
    (void) stkelem;
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR_NO_TRACE();
}

#ifdef DEBUG

static const char *x86emu_fpu_op_da_tab[] = {
    "FIADD\tDWORD PTR ", "FIMUL\tDWORD PTR ", "FICOM\tDWORD PTR ",
    "FICOMP\tDWORD PTR ",
    "FISUB\tDWORD PTR ", "FISUBR\tDWORD PTR ", "FIDIV\tDWORD PTR ",
    "FIDIVR\tDWORD PTR ",

    "FIADD\tDWORD PTR ", "FIMUL\tDWORD PTR ", "FICOM\tDWORD PTR ",
    "FICOMP\tDWORD PTR ",
    "FISUB\tDWORD PTR ", "FISUBR\tDWORD PTR ", "FIDIV\tDWORD PTR ",
    "FIDIVR\tDWORD PTR ",

    "FIADD\tDWORD PTR ", "FIMUL\tDWORD PTR ", "FICOM\tDWORD PTR ",
    "FICOMP\tDWORD PTR ",
    "FISUB\tDWORD PTR ", "FISUBR\tDWORD PTR ", "FIDIV\tDWORD PTR ",
    "FIDIVR\tDWORD PTR ",

    "ESC_DA ", "ESC_DA ", "ESC_DA ", "ESC_DA ",
    "ESC_DA     ", "ESC_DA ", "ESC_DA   ", "ESC_DA ",
};

#endif                          /* DEBUG */

/* opcode=0xda */
void
x86emuOp_esc_coprocess_da(u8 X86EMU_UNUSED(op1))
{
    int mod, rl, rh;
    uint destoffset = 0;
    u8 stkelem = 0;

    START_OF_INSTR();
    FETCH_DECODE_MODRM(mod, rh, rl);
    DECODE_PRINTINSTR32(x86emu_fpu_op_da_tab, mod, rh, rl);
    switch (mod) {
    case 0:
        destoffset = decode_rm00_address(rl);
        DECODE_PRINTF("\n");
        break;
    case 1:
        destoffset = decode_rm01_address(rl);
        DECODE_PRINTF("\n");
        break;
    case 2:
        destoffset = decode_rm10_address(rl);
        DECODE_PRINTF("\n");
        break;
    case 3:                    /* register to register */
        stkelem = (u8) rl;
        DECODE_PRINTF2("\tST(%d),ST\n", stkelem);
        break;
    }
    (void) destoffset;
    (void) stkelem;
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR_NO_TRACE();
}

#ifdef DEBUG

static const char *x86emu_fpu_op_db_tab[] = {
    "FILD\tDWORD PTR ", "ESC_DB\t19", "FIST\tDWORD PTR ", "FISTP\tDWORD PTR ",
    "ESC_DB\t1C", "FLD\tTBYTE PTR ", "ESC_DB\t1E", "FSTP\tTBYTE PTR ",

    "FILD\tDWORD PTR ", "ESC_DB\t19", "FIST\tDWORD PTR ", "FISTP\tDWORD PTR ",
    "ESC_DB\t1C", "FLD\tTBYTE PTR ", "ESC_DB\t1E", "FSTP\tTBYTE PTR ",

    "FILD\tDWORD PTR ", "ESC_DB\t19", "FIST\tDWORD PTR ", "FISTP\tDWORD PTR ",
    "ESC_DB\t1C", "FLD\tTBYTE PTR ", "ESC_DB\t1E", "FSTP\tTBYTE PTR ",
};

#endif                          /* DEBUG */

/* opcode=0xdb */
void
x86emuOp_esc_coprocess_db(u8 X86EMU_UNUSED(op1))
{
    int mod, rl, rh;
    uint destoffset = 0;

    START_OF_INSTR();
    FETCH_DECODE_MODRM(mod, rh, rl);
#ifdef DEBUG
    if (mod != 3) {
        DECODE_PRINTINSTR32(x86emu_fpu_op_db_tab, mod, rh, rl);
    }
    else if (rh == 4) {         /* === 11 10 0 nnn */
        switch (rl) {
        case 0:
            DECODE_PRINTF("FENI\n");
            break;
        case 1:
            DECODE_PRINTF("FDISI\n");
            break;
        case 2:
            DECODE_PRINTF("FCLEX\n");
            break;
        case 3:
            DECODE_PRINTF("FINIT\n");
            break;
        }
    }
    else {
        DECODE_PRINTF2("ESC_DB %0x\n", (mod << 6) + (rh << 3) + (rl));
    }
#endif                          /* DEBUG */
    switch (mod) {
    case 0:
        destoffset = decode_rm00_address(rl);
        break;
    case 1:
        destoffset = decode_rm01_address(rl);
        break;
    case 2:
        destoffset = decode_rm10_address(rl);
        break;
    case 3:                    /* register to register */
        break;
    }
    (void) destoffset;
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR_NO_TRACE();
}

#ifdef DEBUG
static const char *x86emu_fpu_op_dc_tab[] = {
    "FADD\tQWORD PTR ", "FMUL\tQWORD PTR ", "FCOM\tQWORD PTR ",
    "FCOMP\tQWORD PTR ",
    "FSUB\tQWORD PTR ", "FSUBR\tQWORD PTR ", "FDIV\tQWORD PTR ",
    "FDIVR\tQWORD PTR ",

    "FADD\tQWORD PTR ", "FMUL\tQWORD PTR ", "FCOM\tQWORD PTR ",
    "FCOMP\tQWORD PTR ",
    "FSUB\tQWORD PTR ", "FSUBR\tQWORD PTR ", "FDIV\tQWORD PTR ",
    "FDIVR\tQWORD PTR ",

    "FADD\tQWORD PTR ", "FMUL\tQWORD PTR ", "FCOM\tQWORD PTR ",
    "FCOMP\tQWORD PTR ",
    "FSUB\tQWORD PTR ", "FSUBR\tQWORD PTR ", "FDIV\tQWORD PTR ",
    "FDIVR\tQWORD PTR ",

    "FADD\t", "FMUL\t", "FCOM\t", "FCOMP\t",
    "FSUBR\t", "FSUB\t", "FDIVR\t", "FDIV\t",
};
#endif                          /* DEBUG */

/* opcode=0xdc */
void
x86emuOp_esc_coprocess_dc(u8 X86EMU_UNUSED(op1))
{
    int mod, rl, rh;
    uint destoffset = 0;
    u8 stkelem = 0;

    START_OF_INSTR();
    FETCH_DECODE_MODRM(mod, rh, rl);
    DECODE_PRINTINSTR32(x86emu_fpu_op_dc_tab, mod, rh, rl);
    switch (mod) {
    case 0:
        destoffset = decode_rm00_address(rl);
        DECODE_PRINTF("\n");
        break;
    case 1:
        destoffset = decode_rm01_address(rl);
        DECODE_PRINTF("\n");
        break;
    case 2:
        destoffset = decode_rm10_address(rl);
        DECODE_PRINTF("\n");
        break;
    case 3:                    /* register to register */
        stkelem = (u8) rl;
        DECODE_PRINTF2("\tST(%d),ST\n", stkelem);
        break;
    }
    (void) destoffset;
    (void) stkelem;
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR_NO_TRACE();
}

#ifdef DEBUG

static const char *x86emu_fpu_op_dd_tab[] = {
    "FLD\tQWORD PTR ", "ESC_DD\t29,", "FST\tQWORD PTR ", "FSTP\tQWORD PTR ",
    "FRSTOR\t", "ESC_DD\t2D,", "FSAVE\t", "FSTSW\t",

    "FLD\tQWORD PTR ", "ESC_DD\t29,", "FST\tQWORD PTR ", "FSTP\tQWORD PTR ",
    "FRSTOR\t", "ESC_DD\t2D,", "FSAVE\t", "FSTSW\t",

    "FLD\tQWORD PTR ", "ESC_DD\t29,", "FST\tQWORD PTR ", "FSTP\tQWORD PTR ",
    "FRSTOR\t", "ESC_DD\t2D,", "FSAVE\t", "FSTSW\t",

    "FFREE\t", "FXCH\t", "FST\t", "FSTP\t",
    "ESC_DD\t2C,", "ESC_DD\t2D,", "ESC_DD\t2E,", "ESC_DD\t2F,",
};

#endif                          /* DEBUG */

/* opcode=0xdd */
void
x86emuOp_esc_coprocess_dd(u8 X86EMU_UNUSED(op1))
{
    int mod, rl, rh;
    uint destoffset = 0;
    u8 stkelem = 0;

    START_OF_INSTR();
    FETCH_DECODE_MODRM(mod, rh, rl);
    DECODE_PRINTINSTR32(x86emu_fpu_op_dd_tab, mod, rh, rl);
    switch (mod) {
    case 0:
        destoffset = decode_rm00_address(rl);
        DECODE_PRINTF("\n");
        break;
    case 1:
        destoffset = decode_rm01_address(rl);
        DECODE_PRINTF("\n");
        break;
    case 2:
        destoffset = decode_rm10_address(rl);
        DECODE_PRINTF("\n");
        break;
    case 3:                    /* register to register */
        stkelem = (u8) rl;
        DECODE_PRINTF2("\tST(%d),ST\n", stkelem);
        break;
    }
    (void) destoffset;
    (void) stkelem;
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR_NO_TRACE();
}

#ifdef DEBUG

static const char *x86emu_fpu_op_de_tab[] = {
    "FIADD\tWORD PTR ", "FIMUL\tWORD PTR ", "FICOM\tWORD PTR ",
    "FICOMP\tWORD PTR ",
    "FISUB\tWORD PTR ", "FISUBR\tWORD PTR ", "FIDIV\tWORD PTR ",
    "FIDIVR\tWORD PTR ",

    "FIADD\tWORD PTR ", "FIMUL\tWORD PTR ", "FICOM\tWORD PTR ",
    "FICOMP\tWORD PTR ",
    "FISUB\tWORD PTR ", "FISUBR\tWORD PTR ", "FIDIV\tWORD PTR ",
    "FIDIVR\tWORD PTR ",

    "FIADD\tWORD PTR ", "FIMUL\tWORD PTR ", "FICOM\tWORD PTR ",
    "FICOMP\tWORD PTR ",
    "FISUB\tWORD PTR ", "FISUBR\tWORD PTR ", "FIDIV\tWORD PTR ",
    "FIDIVR\tWORD PTR ",

    "FADDP\t", "FMULP\t", "FCOMP\t", "FCOMPP\t",
    "FSUBRP\t", "FSUBP\t", "FDIVRP\t", "FDIVP\t",
};

#endif                          /* DEBUG */

/* opcode=0xde */
void
x86emuOp_esc_coprocess_de(u8 X86EMU_UNUSED(op1))
{
    int mod, rl, rh;
    uint destoffset = 0;
    u8 stkelem = 0;

    START_OF_INSTR();
    FETCH_DECODE_MODRM(mod, rh, rl);
    DECODE_PRINTINSTR32(x86emu_fpu_op_de_tab, mod, rh, rl);
    switch (mod) {
    case 0:
        destoffset = decode_rm00_address(rl);
        DECODE_PRINTF("\n");
        break;
    case 1:
        destoffset = decode_rm01_address(rl);
        DECODE_PRINTF("\n");
        break;
    case 2:
        destoffset = decode_rm10_address(rl);
        DECODE_PRINTF("\n");
        break;
    case 3:                    /* register to register */
        stkelem = (u8) rl;
        DECODE_PRINTF2("\tST(%d),ST\n", stkelem);
        break;
    }
    (void) destoffset;
    (void) stkelem;
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR_NO_TRACE();
}

#ifdef DEBUG

static const char *x86emu_fpu_op_df_tab[] = {
    /* mod == 00 */
    "FILD\tWORD PTR ", "ESC_DF\t39\n", "FIST\tWORD PTR ", "FISTP\tWORD PTR ",
    "FBLD\tTBYTE PTR ", "FILD\tQWORD PTR ", "FBSTP\tTBYTE PTR ",
    "FISTP\tQWORD PTR ",

    /* mod == 01 */
    "FILD\tWORD PTR ", "ESC_DF\t39 ", "FIST\tWORD PTR ", "FISTP\tWORD PTR ",
    "FBLD\tTBYTE PTR ", "FILD\tQWORD PTR ", "FBSTP\tTBYTE PTR ",
    "FISTP\tQWORD PTR ",

    /* mod == 10 */
    "FILD\tWORD PTR ", "ESC_DF\t39 ", "FIST\tWORD PTR ", "FISTP\tWORD PTR ",
    "FBLD\tTBYTE PTR ", "FILD\tQWORD PTR ", "FBSTP\tTBYTE PTR ",
    "FISTP\tQWORD PTR ",

    /* mod == 11 */
    "FFREE\t", "FXCH\t", "FST\t", "FSTP\t",
    "ESC_DF\t3C,", "ESC_DF\t3D,", "ESC_DF\t3E,", "ESC_DF\t3F,"
};

#endif                          /* DEBUG */

/* opcode=0xdf */
void
x86emuOp_esc_coprocess_df(u8 X86EMU_UNUSED(op1))
{
    int mod, rl, rh;
    uint destoffset = 0;
    u8 stkelem = 0;

    START_OF_INSTR();
    FETCH_DECODE_MODRM(mod, rh, rl);
    DECODE_PRINTINSTR32(x86emu_fpu_op_df_tab, mod, rh, rl);
    switch (mod) {
    case 0:
        destoffset = decode_rm00_address(rl);
        DECODE_PRINTF("\n");
        break;
    case 1:
        destoffset = decode_rm01_address(rl);
        DECODE_PRINTF("\n");
        break;
    case 2:
        destoffset = decode_rm10_address(rl);
        DECODE_PRINTF("\n");
        break;
    case 3:                    /* register to register */
        stkelem = (u8) rl;
        DECODE_PRINTF2("\tST(%d)\n", stkelem);
        break;
    }
    (void) destoffset;
    (void) stkelem;
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR_NO_TRACE();
}

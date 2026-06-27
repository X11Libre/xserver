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
* Description:  Header file for FPU register definitions.
*
****************************************************************************/

#ifndef __X86EMU_FPU_REGS_H
#define __X86EMU_FPU_REGS_H

#ifdef DEBUG
#define DECODE_PRINTINSTR32(t,mod,rh,rl)     	\
	DECODE_PRINTF((t)[((mod)<<3)+(rh)]);
#define DECODE_PRINTINSTR256(t,mod,rh,rl)    	\
	DECODE_PRINTF((t)[((mod)<<6)+((rh)<<3)+(rl)]);
#else
#define DECODE_PRINTINSTR32(t,mod,rh,rl)
#define DECODE_PRINTINSTR256(t,mod,rh,rl)
#endif

#endif                          /* __X86EMU_FPU_REGS_H */

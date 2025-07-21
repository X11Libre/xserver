
#ifndef _ARK_REG_H
#define _ARK_REG_H

/* and let there be no 'u' in color */
#define BG_COLOR		0x00
#define FG_COLOR		0x02
#define BG_COLOR_HI		0x04
#define FG_COLOR_HI		0x06
#define TRANS_COLOR		0x08
#define TRANS_COLOR_MSK		0x0a
#define TRANS_COLOR_HI		0x0c
#define TRANS_COLOR_MSK_HI	0x0e
#define COLOR_MIX_SEL		0x18
#define WRITE_PLANEMASK		0x1a
#define	ERROR_TERM		0x50
#define AXIAL_ERROR_INC		0x54
#define STENCIL_PITCH		0x60
#define SRC_PITCH		0x62
#define DST_PITCH		0x64
#define STENCIL_ADDR		0x68
#define	STENCIL_X		0x68
#define STENCIL_Y		0x6a
#define	SRC_ADDR		0x6c
#define SRC_X			0x6c
#define SRC_Y			0x6e
#define DST_ADDR		0x70
#define DST_X			0x70
#define DST_Y			0x72
#define WIDTH			0x74
#define HEIGHT			0x76
#define BITMAP_CONFIG		0x7c
#define COMMAND			0x7e


/* constants for COMMAND register */

#define DRAWSTEP		0x0000
#define LINEDRAW		0x1000
#define BITBLT			0x2000
#define TEXTBITBLT		0x3000
#define USE_PLANEMASK		0x0000
#define DISABLE_PLANEMASK	0x0800
#define PATTERN_8X8		0x0400
#define SELECT_BG_COLOR		0x0000
#define BG_BITMAP		0x0200
#define SELECT_FG_COLOR		0x0000
#define FG_BITMAP		0x0100
#define STENCIL_ONES		0x0000
#define STENCIL_GENERATED	0x0040
#define STENCIL_BITMAP		0x0080
#define LINE_DRAWALL		0x0000
#define LINE_SKIP_FIRST		0x0010
#define LINE_SKIP_LAST		0x0020
#define ENABLE_CLIPPING		0x0000
#define DISABLE_CLIPPING	0x0008
#undef DOWN
#define DOWN			0x0000
#undef UP
#define UP			0x0002
#define RIGHT			0x0000
#define LEFT			0x0004
#define DX_GREATER_THAN_DY	0x0000
#define DY_GREATER_THAN_DX	0x0001


/* constants for bitmap config register */

#define SWAP_NIBLES		0x2000
#define SWAP_BITS		0x1000
#define SYSTEM_STENCIL		0x0200
#define LINEAR_STENCIL_ADDR	0x0100
#define SYSTEM_SRC		0x0020
#define LINEAR_SRC_ADDR		0x0010
#define SYSTEM_DST		0x0002
#define LINEAR_DST_ADDR		0x0001


/* IO macros */

#define OUTREG16(offset, value)	\
	*(volatile unsigned short *)(pARK->MMIOBase + offset) = value
#define OUTREG(offset, value) \
	*(volatile unsigned int *)(pARK->MMIOBase + offset) = value


#endif /* _ARK_REG_H */

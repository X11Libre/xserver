


#define curr08		(curr - 0x30)
#define curr16		((CARD16 *)(curr - 0x30))
#define curr32		((CARD32 *)(curr - 0x30))
#define check08(addr, val)						\
    ((addr) >= 0x80 || (((addr)&0xF8) == 0x48) || curr08[(addr)] != (val))
#define check16(addr, val)						\
    ((addr) >= 0x80 || (((addr)&0xF8) == 0x48) || curr16[(addr) / 2] != (val)||\
	((addr) == 0x50 && curr32[0x40 / 4] & DEC_QUICKSTART_ONSOURCE)||\
	((addr) == 0x52 && curr32[0x40 / 4] & DEC_QUICKSTART_ONSOURCE)||\
	((addr) == 0x54 && curr32[0x40 / 4] & DEC_QUICKSTART_ONDEST) ||	\
	((addr) == 0x56 && curr32[0x40 / 4] & DEC_QUICKSTART_ONDEST) ||	\
	((addr) == 0x58 && curr32[0x40 / 4] & DEC_QUICKSTART_ONDIMX))
#define check32(addr, val)						\
    ((addr) >= 0x80 || (((addr)&0xF8) == 0x48) || curr32[(addr) / 4] != (val)||\
	((addr) == 0x50 && curr32[0x40 / 4] & DEC_QUICKSTART_ONSOURCE)||\
	((addr) == 0x54 && curr32[0x40 / 4] & DEC_QUICKSTART_ONDEST) ||	\
	((addr) == 0x58 && curr32[0x40 / 4] & DEC_QUICKSTART_ONDIMX) ||	\
	((addr) == 0x40 && (val) & DEC_START))

#ifndef MIN
#define MIN(a,b)	((a) < (b) ? (a) : (b))
#endif
/* Memory mapped access to extended registers */
#define RDXB_M(addr)     (MMIO_IN8 (pApm->MemMap, (addr)))
#define RDXW_M(addr)     (MMIO_IN16(pApm->MemMap, (addr)))
#define RDXL_M(addr)     (MMIO_IN32(pApm->MemMap, (addr)))
#define WRXB_M(addr,val)  do { if (check08((addr), (val))) { \
			MMIO_OUT8 (pApm->MemMap, (addr), (val));	\
			/*xf86DrvMsg(xf86ScreenToScrn(pApm->pScreen)->scrnIndex, X_INFO, \
				    "File %s, line %d,	%02X <-       %02X\n", \
				    __FILE__, __LINE__, (addr), (val) & 255); */\
			curr08[MIN((addr), 0x80)] = (val); }} while (0)
#define WRXW_M(addr,val)  do { if (check16((addr), (val))) { \
			MMIO_OUT16(pApm->MemMap, (addr), (val));	\
			/*xf86DrvMsg(xf86ScreenToScrn(pApm->pScreen)->scrnIndex, X_INFO, \
				    "File %s, line %d,	%02X <-     %04X\n", \
				    __FILE__, __LINE__, (addr), (val)&65535); */\
			curr16[MIN(((addr) / 2), 0x40)] = (val); }} while (0)
#define WRXL_M(addr,val)  do { if (check32((addr), (val))) { \
			MMIO_OUT32(pApm->MemMap, (addr), (val));	\
			/*xf86DrvMsg(xf86ScreenToScrn(pApm->pScreen)->scrnIndex, X_INFO, \
				    "File %s, line %d,	%02X <- %08X\n", \
				    __FILE__, __LINE__, (addr), (val)); */\
			curr32[MIN(((addr) / 4), 0x20)] = (val); }} while (0)

/* IO port access to extended registers */
#define RDXB_IOP(addr)     (wrinx(pApm->xport, 0x1D, (addr) >> 2), \
			    inb(pApm->xbase + ((addr) & 3)))
#define RDXW_IOP(addr)     (wrinx(pApm->xport, 0x1D, (addr) >> 2), \
			    inw(pApm->xbase + ((addr) & 2)))
#define RDXL_IOP(addr)     (wrinx(pApm->xport, 0x1D, (addr) >> 2), \
			    inl(pApm->xbase))
#define WRXB_IOP(addr,val)					   \
    do { 							   \
	if (check08((addr), (val))) {				   \
	    wrinx(pApm->xport, 0x1D, (addr) >> 2);		   \
	    outb(pApm->xbase + ((addr) & 3), (val));		   \
	    curr08[MIN((addr), 0x80)] = (val);			   \
	    break;						   \
	}							   \
    } while (1)
#define WRXW_IOP(addr,val)					   \
    do {							   \
	if (check16((addr), (val))) {				   \
	    wrinx(pApm->xport, 0x1D, (addr) >> 2);		   \
	    outw(pApm->xbase + ((addr) & 2), (val));		   \
	    curr16[MIN(((addr) / 2), 0x40)] = (val);		   \
	    break;						   \
	}							   \
    } while (1)
#define WRXL_IOP(addr,val)					   \
    do {							   \
	if (check32((addr), (val))) {				   \
	    wrinx(pApm->xport, 0x1D, (addr) >> 2);		   \
	    outl(pApm->xbase, (val));				   \
	    curr32[MIN(((addr) / 4), 0x20)] = (val);		   \
	    break;						   \
	}							   \
    } while (1)

#define WRXL	WRXL_M
#define WRXW	WRXW_M
#define WRXB	WRXB_M
#define RDXL	RDXL_M
#define RDXW	RDXW_M
#define RDXB	RDXB_M
#define UPDATEDEST(x,y)		(void)(curr32[0x54 / 4] = ((y) << 16) | ((x) & 0xFFFF))

/* Memory mapped access to VGA registers */
#define APMVGAB(idx)		(((volatile unsigned char  *)pApm->VGAMap)[idx])
#define APMVGAS(idx)		(((volatile unsigned short *)pApm->VGAMap)[(idx) >> 1])
#define APMVGAW(idx)		(((volatile unsigned int   *)pApm->VGAMap)[(idx) >> 2])
#define ApmWriteCrtc(idx, val)	do { APMVGAS(0x3D4) = ((val) << 8) | ((idx) & 0xFF); break; } while(1)
#define ApmReadCrtc(idx)	((APMVGAB(0x3D4) = (idx)), APMVGAB(0x3D5))
#define ApmWriteGr(idx, val)	do { APMVGAS(0x3CE) = ((val) << 8) | ((idx) & 0xFF); break; } while(1)
#define ApmReadGr(idx)	((APMVGAB(0x3CE) = (idx)), APMVGAB(0x3CF))
#define ApmWriteSeq(idx, val)	do { APMVGAB(0x3C4) = (idx); APMVGAB(0x3C5) = (val); break; } while(1)
#define ApmReadSeq(idx)	((APMVGAB(0x3C4) = (idx)), APMVGAB(0x3C5))
#define ApmWriteAttr(idx, val)	do { int tmp = APMVGAB(0x3DA); APMVGAB(0x3C0) = (idx); APMVGAB(0x3C0) = (val); break; } while(1)
#define ApmReadAttr(idx)	(APMVGAB(0x3DA), (APMVGAB(0x3C0) = (idx)), APMVGAB(0x3C1))
#define ApmWriteMiscOut(val)	do { APMVGAB(0x3C2) = (val); break; } while(1)
#define ApmReadMiscOut()	APMVGAB(0x3CC)
#define ApmWriteDacMask(val)	do { APMVGAB(0x3C6) = (val); break; } while(1)
#define ApmReadDacMask()	APMVGAB(0x3C6)
#define ApmWriteDacReadAddr(val)do { APMVGAB(0x3C7) = (val); break; } while(1)
#define ApmWriteDacWriteAddr(val)do{ APMVGAB(0x3C8) = (val); break; } while(1)
#define ApmWriteDacData(val)	do { APMVGAB(0x3C9) = (val); break; } while(1)
#define ApmReadDacData()	APMVGAB(0x3C9)

#define STATUS()			(RDXL(0x1FC))
#define STATUS_IOP()			(RDXL_IOP(0x1FC))
#define STATUS_FIFO			(0x0F)
#define STATUS_HOSTBLTBUSY		(1 << 8)
#define STATUS_ENGINEBUSY		(1 << 10)
#define STATUS_SDA			(1 << 16)
#define STATUS_SCL			(1 << 17)

#define SETFOREGROUNDCOLOR(c)		WRXL(0x60,c)
#define SETBACKGROUNDCOLOR(c)		WRXL(0x64,c)

#define SETSOURCEX(x)			WRXW(0x50, x)
#define SETSOURCEY(y)			WRXW(0x52, y)
#define SETSOURCEXY(x,y)		WRXL(0x50, ((y) << 16) | ((x) & 0xFFFF))
#define SETSOURCEOFF(o)			WRXL(0x50, (o))

#define SETDESTX(x)			WRXW(0x54, x)
#define SETDESTY(y)			WRXW(0x56, y)
#define SETDESTXY(x,y)			WRXL(0x54, ((y) << 16) | ((x) & 0xFFFF))
#define SETDESTOFF(o)			WRXL(0x54, (o))

#define SETWIDTH(w)			WRXW(0x58, w)
#define SETHEIGHT(h)			WRXW(0x5A, h)
#define SETWIDTHHEIGHT(w,h)		WRXL(0x58, ((h) << 16) | ((w) & 0xFFFF))

#define SETOFFSET(o)			WRXW(0x5C, (o))
#define SETSOURCEOFFSET(o)		WRXW(0x5E, (o))

#define SETBYTEMASK(mask)		WRXB(0x47, (mask))

#define SETPATTERN(p1, p2)		do {WRXL(0x48, p1); WRXL(0x4C, p2);} while(0)

#define SETDDA_AXIALSTEP(step)		WRXW(0x70, (step))
#define SETDDA_DIAGONALSTEP(step)	WRXW(0x72, (step))
#define SETDDA_ERRORTERM(eterm)		WRXW(0x74, (eterm))
#define SETDDA_ADSTEP(s1,s2)		WRXL(0x70, ((s2) << 16)|((s1) & 0xFFFF))

#define SETCLIP_CTRL(ctrl)		WRXB(0x30, (ctrl))
#define SETCLIP_LEFT(x)			WRXW(0x38, (x))
#define SETCLIP_TOP(y)			WRXW(0x3A, (y))
#define SETCLIP_LEFTTOP(x,y)		WRXL(0x38, ((y) << 16) | ((x) & 0xFFFF))
#define SETCLIP_RIGHT(x)		WRXW(0x3C, (x))
#define SETCLIP_BOT(y)			WRXW(0x3E, (y))
#define SETCLIP_RIGHTBOT(x,y)		WRXL(0x3C, ((y) << 16) | ((x) & 0xFFFF))

/* RASTER OPERATION REGISTER */
/* P = pattern   S = source   D = destination */
#define SETROP(rop)			WRXB(0x46, (rop))
#define ROP_P_and_S_and_D		0x80
#define ROP_S_xor_D			0x66
#define ROP_S				0xCC
#define ROP_P				0xF0
/* Then there are about 252 more operations ... */


/* DRAWING ENGINE CONTROL REGISTER */
#define SETDEC(control)             WRXL(0x40, (control))
#define DEC_OP_VECT_NOENDP          0x0000000D
#define DEC_OP_VECT_ENDP            0x0000000C
#define DEC_OP_HOSTBLT_SCREEN2HOST  0x00000009
#define DEC_OP_HOSTBLT_HOST2SCREEN  0x00000008
#define DEC_OP_STRIP                0x00000004
#define DEC_OP_BLT_STRETCH          0x00000003
#define DEC_OP_RECT                 0x00000002
#define DEC_OP_BLT                  0x00000001
#define DEC_OP_NOOP                 0x00000000
#define DEC_DIR_X_NEG               (1 << 6)
#define DEC_DIR_X_POS               (0 << 6)
#define DEC_DIR_Y_NEG               (1 << 7)
#define DEC_DIR_Y_POS               (0 << 7)
#define DEC_MAJORAXIS_X             (0 << 8) /* Looks like an error in the docs ...*/
#define DEC_MAJORAXIS_Y             (1 << 8)
#define DEC_SOURCE_LINEAR           (1 << 9) 
#define DEC_SOURCE_XY               (0 << 9)
#define DEC_SOURCE_CONTIG           (1 << 11)
#define DEC_SOURCE_RECTANGULAR      (0 << 11)
#define DEC_SOURCE_MONOCHROME       (1 << 12)
#define DEC_SOURCE_COLOR            (0 << 12)
#define DEC_SOURCE_TRANSPARENCY     (1 << 13)
#define DEC_SOURCE_NO_TRANSPARENCY  (0 << 13)
#define DEC_BITDEPTH_MASK	    (7 << 14)
#define DEC_BITDEPTH_24             (4 << 14)
#define DEC_BITDEPTH_32             (3 << 14)
#define DEC_BITDEPTH_16             (2 << 14)
#define DEC_BITDEPTH_8              (1 << 14)
#define DEC_DEST_LINEAR             (1 << 18)
#define DEC_DEST_XY                 (0 << 18)
#define DEC_DEST_CONTIG             (1 << 19)
#define DEC_DEST_RECTANGULAR        (0 << 19)
#define DEC_DEST_TRANSPARENCY       (1 << 20)
#define DEC_DEST_NO_TRANSPARENCY    (0 << 20)
#define DEC_DEST_TRANSP_POLARITY    (1 << 21)
#define DEC_DEST_TRANSP_POLARITYINV (0 << 21)
#define DEC_PATTERN_88_8bCOLOR      (3 << 22)
#define DEC_PATTERN_88_1bMONO       (2 << 22)
#define DEC_PATTERN_44_4bDITHER     (1 << 22)
#define DEC_PATTERN_NONE            (0 << 22)
#define DEC_WIDTH_MASK		    (7 << 24)
#define DEC_WIDTH_1600              (7 << 24)
#define DEC_WIDTH_1280              (6 << 24)
#define DEC_WIDTH_1152              (5 << 24)
#define DEC_WIDTH_1024              (4 << 24)
#define DEC_WIDTH_800               (2 << 24)
#define DEC_WIDTH_640               (1 << 24)
#define DEC_WIDTH_LINEAR            (0 << 24)
#define DEC_DEST_UPD_LASTPIX        (3 << 27)
#define DEC_DEST_UPD_BLCORNER       (2 << 27)
#define DEC_DEST_UPD_TRCORNER       (1 << 27)
#define DEC_DEST_UPD_NONE           (0 << 27)
#define DEC_QUICKSTART_ONDEST       (3 << 29)
#define DEC_QUICKSTART_ONSOURCE     (2 << 29)
#define DEC_QUICKSTART_ONDIMX       (1 << 29)
#define DEC_QUICKSTART_NONE         (0 << 29)
#define DEC_START                   (1 << 31)
#define DEC_START_NO                (0 << 31)

#define AT3D_SST_STATUS		0x1F4
#define		SST_BUSY	1

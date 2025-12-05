#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#include "iopl.h"

int main(void)
{
    int i, HTotal, HDisplay, HSyncStart, HSyncEnd, 
    VTotal, VDisplay, VSyncStart, VSyncEnd;
    unsigned char storeReg, bpp, shift, IOSS = 0, MSS = 0, again = 0;
    unsigned short port;
    int isHiQV = 0;
    int is69030 = 0;

    SET_IOPL();
    
    printf("0x3C6\t0x%X\n",inw(0x3C6));

/* Check to see if the Chip is HiQV */
    outb(0x3D6,0x02);
    storeReg = inb(0x3D7);
    if (storeReg == 0xE0	/* CT65550 */
	|| storeReg == 0xE4	/* CT65554 */
	|| storeReg == 0xE5	/* CT65555 */
	|| storeReg == 0xF4	/* CT68554 */
	|| storeReg == 0xC0)	/* CT69000 */
    {
	isHiQV = 1;
    } else if (storeReg == 0x30) {
      outb(0x3D6,0x03);
      storeReg = inb(0x3D7);
      if (storeReg == 0xC) {
	isHiQV = 1;
	is69030 = 1;
	IOSS=inb(0x3CD);
	MSS=inb(0x3CB);
	outb(0x3CD,((IOSS&0xE0)| 0x11));  /* Select Channel 0 */
	outb(0x3CB,((MSS&0xF0)| 0x8));
	again = 1;
	printf("Pipeline A:\n");
      }
    }
        
 again:
    printf("port 0x3D6 (C&T)\n");
    storeReg = inb(0x3D6);
    shift = 3;
    if (isHiQV==1) {
	outw(0x102,1);	/*global enable, VGA awake*/
	printf("0x%2.2X\n",inb(0x3C3)&0xFF);
	outb(0x3C3,0);	/*disable VGA*/
	outb(0x3C3,1);	/*enable VGA*/
	for(i = 0;i < 0xFF;i++){
	    outb(0x3D6,i);
	    printf("XR 0x%2.2X\t0x%2.2X\n",i,inb(0x3D7)&0xFF);
	}
	outb(0x3D6,0xE2);
	bpp = inb(0x3D7)&0xF0;
    } else {	
	outb(0x3D6, 0x70);
	outw(0x3D6, (inw(0x3D6) | 0x8070));
	outw(0x46E8,0x0016);	/*setup mode*/
	outw(0x102,1);	/*global enable, VGA awake*/
	outw(0x46E8,0x000E);	/*exit from setup mode*/
	printf("0x%2.2X\n",inb(0x3C3)&0xFF);
	outb(0x3C3,0);	/*disable VGA*/
	outw(0x46E8,0x0000);	/*exit from setup mode*/
	outw(0x46E8,0x000E);	/*exit from setup mode*/
	outb(0x3C3,1);	/*enable VGA*/
	outw(0x46E8,0x0000);	/*exit from setup mode*/
	for(i = 0;i < 0x80;i++){
	    outb(0x3D6,i);
	    printf("XR 0x%2.2X\t0x%2.2X\n",i,inb(0x3D7)&0xFF);
	}
	outb(0x3D6,0x2B);
	bpp = inb(0x3D7)&0xF0;
    }
    
    switch(bpp){
      case 0x20:
	bpp = 4;
	break;
      case 0x30:
	bpp = 8;
	break;
      case 0x40:
	bpp = 16;
	shift = 2;
	break;
      case 0x50:
	bpp = 24;
	break;
      default:
	bpp = 0;
    }
    outb(0x3D6,storeReg);

    printf("\nport 0x3D4 (CRTC)\n");
    storeReg = inb(0x3D4);
    if (isHiQV==1) {
	for(i = 0;i < 0x7F;i++){
	    outb(0x3D4,i);
	    printf("CR 0x%2.2X\t0x%2.2X\n",i,inb(0x3D5)&0xFF);
	}
	outb(0x3D4,storeReg);
	printf("\nport 0x3D0 (Flat Panel)\n");
	storeReg = inb(0x3D0);
	for(i = 0;i < 0x7F;i++){
	    outb(0x3D0,i);
	    printf("FR 0x%2.2X\t0x%2.2X\n",i,inb(0x3D1)&0xFF);
	}
	outb(0x3D1,storeReg);
	printf("\nport 0x3D2 (Multimedia)\n");
	storeReg = inb(0x3D2);
	for(i = 0;i < 0x7F;i++){
	    outb(0x3D2,i);
	    printf("MR 0x%2.2X\t0x%2.2X\n",i,inb(0x3D3)&0xFF);
	}
	outb(0x3D3,storeReg);
    } else {	
	for(i = 0;i < 0x40;i++){
	    outb(0x3D4,i);
	    printf("CR 0x%2.2X\t0x%2.2X\n",i,inb(0x3D5)&0xFF);
	}
	outb(0x3D4,storeReg);
    }
    

    printf("port 0x3CE (GC)\n");
    storeReg = inb(0x3CE);
    for(i = 0;i < 0x10;i++){
	outb(0x3CE,i);
	printf("GC 0x%2.2X\t0x%2.2X\n",i,inb(0x3CF)&0xFF);
    }
    outb(0x3CE,storeReg);
    printf("port 0x3C4 (Sequencer)\n");
    storeReg = inb(0x3C4);
    for(i = 0;i < 0x10;i++){
	outb(0x3C4,i);
	printf("SQ 0x%2.2X\t0x%X2.2\n",i,inb(0x3C5)&0xFF);
    }
    outb(0x3C4,storeReg);


    printf("port 0x3C0 (Attribute)\n");
    inb(0x3DA);
    storeReg = inb(0x3C0);
    for(i = 0;i < 0xFF;i++){
	inb(0x3DA);
	outb(0x3C0,i);
	printf("AT 0x%2.2X\t0x%2.2X\n",i,inb(0x3C1)&0xFF);
    }
    inb(0x3DA);
    outb(0x3C0,storeReg);

    printf("0x3CC\t0x%X\n",inb(0x3CC)&0xFF);
    printf("0x3C2\t0x%X\n",inb(0x3C2)&0xFF);
    printf("0x3C3\t0x%X\n",inb(0x3C2)&0xFF);
    printf("0x3CA\t0x%X\n",inb(0x3CA)&0xFF);
    printf("0x3DA\t0x%X\n",inb(0x3DA)&0xFF);

    printf("\nRAMDAC\nport\tvalue\n");
    for(port = 0x83C6; port < 0x83CA;port++){
      printf("0x%4X\t0x%4X\n",port,inw(port));
    }

    if (isHiQV!=1) {
	printf("\nBitBLT\nport\tvalue\n");
	for(port = 0x83D0; port <= 0x9FD0;port+=0x400){
	    printf("0x%4.4X\t0x%4X\n",port,inw(port));
	}

	printf("\nH/W cursor\nport\tvalue\n");
	for(port = 0xA3D0; port <= 0xB3D0;port+=0x400){
	    printf("0x%4.4X\t0x%4X\n",port,inw(port));
	}


	outb(0x3D6, 0x70);
	outw(0x3D6, (inw(0x3D6) | 0x8070));

	printf("0x46E8\t0x%8X\n",inl(0x46E8));
	printf("0x4AE8\t0x%8X\n",inl(0x4AE8));
	printf("0x102\t0x%8X\n",inl(0x102));
	printf("0x103\t0x%8X\n",inl(0x103));

    }    

    storeReg = inb(0x3D4);
    {
	outb(0x3D4,0);
	HTotal = ((inb(0x3D5)&0xFF) + 5) << shift;
	outb(0x3D4,1);
	HDisplay = ((inb(0x3D5)&0xFF) + 1) << shift;
	outb(0x3D4,4);
	HSyncStart = ((inb(0x3D5)&0xFF) + 1) << shift;
	outb(0x3D4,5);
	HSyncEnd = inb(0x3D5)&0x1F;
	outb(0x3D4,5);
	HSyncEnd += HSyncStart >> shift;
	HSyncEnd <<= shift;
	
	outb(0x3D4,6);
	VTotal = inb(0x3D5)&0xFF;
	outb(0x3D4,7);
	VTotal |= (inb(0x3D5)&0x1) << 8;
	VTotal |= (inb(0x3D5)&0x20) << 4;
	VTotal += 2;
	VDisplay = (inb(0x3D5)&0x2) << 7;
	VDisplay |= (inb(0x3D5)&0x40) << 3;
	VSyncStart = (inb(0x3D5)&0x4) << 6;
	VSyncStart |= (inb(0x3D5)&0x80) << 2;
	outb(0x3D4,0x12);
	    VDisplay |= inb(0x3D5)&0xFF;
	VDisplay += 1;
	outb(0x3D4,0x10);
	VSyncStart |= inb(0x3D5)&0xFF;
	
	outb(0x3D4,0x11);
	VSyncEnd = inb(0x3D5)&0xF;
	VSyncEnd += VSyncStart;
	
    }
    outb(0x3D4,storeReg);
    
    printf("\nModeLine with port 0x3D4 (CRTC) %d %d %d %d %d %d %d %d\n",
	   HDisplay, HSyncStart, HSyncEnd, HTotal,
	   VDisplay, VSyncStart, VSyncEnd, VTotal);


    if (is69030==1) {
      if (again==1) {
	again=0;
	printf("\n\nPipeline B:\n");
	outb(0x3CD,((IOSS&0xE0)| 0x1F));  /* Select Channel 1 */
	outb(0x3CB,((MSS&0xF0)| 0xF));
	goto again;
      } else {
	outb(0x3CD,IOSS);
	outb(0x3CB,MSS);
	printf("\n\n0x3CB\t0x%X  (MSS)\n",inb(0x3CB)&0xFF);
	printf("0x3CD\t0x%X  (IOSS)\n",inb(0x3CD)&0xFF);
      }
    }
    RESET_IOPL();
    return 0;
}

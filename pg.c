// primitive graphics for Hello World PSP
#include <pspkernel.h>
#include <pspdisplay.h>
#include <stdlib.h>
#include <string.h>

#include "pg.h"
#include "font.c"

//system call
void pspDisplayWaitVblankStart();
void pspDisplaySetMode(long,long,long);
//void pspDisplaySetFrameBuf(char *topaddr,long linesize,long pixelsize,long);

#define pspDisplaySetFrameBuf sceDisplaySetFrameBuf

//constants

//variables
char *pg_vramtop=(char *)0x04000000;
long pg_screenmode;
long pg_showframe;
long pg_drawframe;



void pgWaitVn(unsigned long count)
{
    for (; count>0; --count) {
	//pspDisplayWaitVblankStart();
	sceDisplayWaitVblankStart();
    }
}


void pgWaitV()
{
    //pspDisplayWaitVblankStart();
    sceDisplayWaitVblankStart();
}


char *pgGetVramAddr(unsigned long x,unsigned long y)
{
    return pg_vramtop+(pg_drawframe?FRAMESIZE:0)+x*PIXELSIZE*2+y*LINESIZE*2+0x40000000;
}


void pgInit()
{
    //pspDisplaySetMode(0,SCREEN_WIDTH,SCREEN_HEIGHT);
    sceDisplaySetMode(0,SCREEN_WIDTH,SCREEN_HEIGHT);
    pgScreenFrame(0,0);
}


void pgPrint(unsigned long x,unsigned long y,unsigned long color,const char *str)
{
	while (*str!=0 && x<CMAX_X && y<CMAX_Y) {
		pgPutChar(x*8,y*8,color,0,*str,1,0,1);
		str++;
		x++;
		if (x>=CMAX_X) {
			x=0;
			y++;
		}
	}
}

/*
void pgPrint2(unsigned long x,unsigned long y,unsigned long color,const char *str)
{
	while (*str!=0 && x<CMAX2_X && y<CMAX2_Y) {
		pgPutChar(x*16,y*16,color,0,*str,1,0,2);
		str++;
		x++;
		if (x>=CMAX2_X) {
			x=0;
			y++;
		}
	}
}


void pgPrint4(unsigned long x,unsigned long y,unsigned long color,const char *str)
{
	while (*str!=0 && x<CMAX4_X && y<CMAX4_Y) {
		pgPutChar(x*32,y*32,color,0,*str,1,0,4);
		str++;
		x++;
		if (x>=CMAX4_X) {
			x=0;
			y++;
		}
	}
}
*/

void pgFillvram(unsigned long color)
{
	unsigned char *vptr0;		//pointer to vram
	unsigned long i;

	vptr0=(unsigned char*)pgGetVramAddr(0,0);
	for (i=0; i<FRAMESIZE/2; i++) {
		*(unsigned short *)vptr0=color;
		vptr0+=PIXELSIZE*2;
	}
}

void pgBitBlt(unsigned long x,unsigned long y,unsigned long w,unsigned long h,unsigned long mag,const unsigned short *d)
{
	unsigned char *vptr0;		//pointer to vram
	unsigned char *vptr;		//pointer to vram
	unsigned long xx,yy,mx,my;
	const unsigned short *dd;
	
	vptr0=(unsigned char *)pgGetVramAddr(x,y);
	for (yy=0; yy<h; yy++) {
		for (my=0; my<mag; my++) {
			vptr=vptr0;
			dd=d;
			for (xx=0; xx<w; xx++) {
				for (mx=0; mx<mag; mx++) {
					*(unsigned short *)vptr=*dd;
					vptr+=PIXELSIZE*2;
				}
				dd++;
			}
			vptr0+=LINESIZE*2;
		}
		d+=w;
	}
	
}

void pgBitBltMask(unsigned long x,unsigned long y,unsigned long w,
		  unsigned long h,unsigned long mag,
		  const unsigned short *d,
		  unsigned short mask)
{
    unsigned char *vptr0;		//pointer to vram
    unsigned char *vptr;		//pointer to vram
    unsigned long xx,yy,mx,my;
    const unsigned short *dd;
	
    vptr0=(unsigned char*)pgGetVramAddr(x,y);
    for (yy=0; yy<h; yy++)
    {
	for (my=0; my<mag; my++)
	{
	    vptr=vptr0;
	    dd=d;
	    for (xx=0; xx<w; xx++)
	    {
		for (mx=0; mx<mag; mx++)
		{
		    if (*dd != mask)
			*(unsigned short *)vptr=*dd;
		    
		    vptr+=PIXELSIZE*2;
		}
		dd++;
	    }
	    vptr0+=LINESIZE*2;
	}
	d+=w;
    }
	
}


void pgPutChar(unsigned long x,unsigned long y,unsigned long color,unsigned long bgcolor,unsigned char ch,char drawfg,char drawbg,char mag)
{
	unsigned char *vptr0;		//pointer to vram
	unsigned char *vptr;		//pointer to vram
	const unsigned char *cfont;		//pointer to font
	unsigned long cx,cy;
	unsigned long b;
	char mx,my;

	cfont=font+ch*8;
	vptr0=(unsigned char*)pgGetVramAddr(x,y);
	for (cy=0; cy<8; cy++) {
		for (my=0; my<mag; my++) {
			vptr=vptr0;
			b=0x80;
			for (cx=0; cx<8; cx++) {
				for (mx=0; mx<mag; mx++) {
					if ((*cfont&b)!=0) {
						if (drawfg) *(unsigned short *)vptr=color;
					} else {
						if (drawbg) *(unsigned short *)vptr=bgcolor;
					}
					vptr+=PIXELSIZE*2;
				}
				b=b>>1;
			}
			vptr0+=LINESIZE*2;
		}
		cfont++;
	}
}


void pgScreenFrame(long mode,long frame)
{
    pg_screenmode=mode;
    frame=(frame?1:0);
    pg_showframe=frame;
    if (mode==0) {
	//screen off
	pg_drawframe=frame;
	pspDisplaySetFrameBuf(0,0,0,1);
    } else if (mode==1) {
	//show/draw same
	pg_drawframe=frame;
	pspDisplaySetFrameBuf(pg_vramtop+(pg_showframe?FRAMESIZE:0),LINESIZE,PIXELSIZE,1);
    } else if (mode==2) {
	//show/draw different
	pg_drawframe=(frame?0:1);
	pspDisplaySetFrameBuf(pg_vramtop+(pg_showframe?FRAMESIZE:0),LINESIZE,PIXELSIZE,1);
    }
}


void pgScreenCopy()
{
    if (pg_showframe)
    {
	memcpy(pg_vramtop, pg_vramtop+FRAMESIZE, FRAMESIZE);
    }
    else
    {
	memcpy(pg_vramtop+FRAMESIZE, pg_vramtop, FRAMESIZE);
    }
}

void pgScreenFlip()
{
	pg_showframe=(pg_showframe?0:1);
	pg_drawframe=(pg_drawframe?0:1);
	pspDisplaySetFrameBuf(pg_vramtop+(pg_showframe?FRAMESIZE:0),LINESIZE,PIXELSIZE,0);
}


void pgScreenFlipV()
{
	pgWaitV();
	pgScreenFlip();
}

void print_hex(unsigned int val)
 {
 	char str[9];
 	int pos;
 	str[8] = 0;
 	pos = 7;
 	while(pos >= 0)
 	{
 		int nibble;
 		nibble = val & 0xF;
 		if(nibble < 0xA)
 		{
 			str[pos--] = nibble + '0';
 		}
 		else
 		{
 			str[pos--] = nibble + 'A' - 0xA;
 		}
 		val >>= 4;
 	}
		pgFillvram(0);
		pgPrint(1,7,0xffff,str);			
		pgScreenFlipV();
 }

void pgDrawFrame(unsigned long x1, unsigned long y1, unsigned long x2,
		 unsigned long y2, unsigned long color)
{
    unsigned char *vptr0;		//pointer to vram
    unsigned long i;
    
    vptr0=(unsigned char*)pgGetVramAddr(0,0);
    for(i=x1; i<=x2; i++)
    {
	((unsigned short *)vptr0)[i*PIXELSIZE + y1*LINESIZE] = color;
	((unsigned short *)vptr0)[i*PIXELSIZE + y2*LINESIZE] = color;
    }
    for(i=y1; i<=y2; i++)
    {
	((unsigned short *)vptr0)[x1*PIXELSIZE + i*LINESIZE] = color;
	((unsigned short *)vptr0)[x2*PIXELSIZE + i*LINESIZE] = color;
    }
}

void pgFillBox(unsigned long x1, unsigned long y1, unsigned long x2,
	       unsigned long y2, unsigned long color)
{
    unsigned char *vptr0;		//pointer to vram
    unsigned long i, j;
    
    vptr0=(unsigned char*)pgGetVramAddr(0,0);
    for(i=y1; i<=y2; i++)
    {
	for(j=x1; j<=x2; j++)
	{
	    ((unsigned short *)vptr0)[j*PIXELSIZE + i*LINESIZE] = color;
	}
    }
}


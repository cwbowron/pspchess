// primitive graphics for Hello World PSP

void pgInit();
void pgWaitV();
void pgWaitVn(unsigned long count);
void pgScreenFrame(long mode,long frame);
void pgScreenFlip();
void pgScreenFlipV();

void pgPrint(unsigned long x,unsigned long y,unsigned long color,const char *str);
/*
void pgPrint2(unsigned long x,unsigned long y,unsigned long color,const char *str);
void pgPrint4(unsigned long x,unsigned long y,unsigned long color,const char *str);
*/
void pgFillvram(unsigned long color);
void pgBitBlt(unsigned long x,unsigned long y,unsigned long w,unsigned long h,unsigned long mag,const unsigned short *d);
void pgPutChar(unsigned long x,unsigned long y,unsigned long color,unsigned long bgcolor,unsigned char ch,char drawfg,char drawbg,char mag);
void print_hex(unsigned int val);

char *pgGetVramAddr(unsigned long x,unsigned long y);

void pgBitBltMask(unsigned long x,unsigned long y,unsigned long w,
		  unsigned long h,unsigned long mag,
		  const unsigned short *d,
		  unsigned short mask);

void pgScreenCopy();

void pgDrawFrame(unsigned long x1, unsigned long y1, unsigned long x2,
		 unsigned long y2, unsigned long color);

void pgFillBox(unsigned long x1, unsigned long y1, unsigned long x2,
	       unsigned long y2, unsigned long color);

#define SCREEN_WIDTH  480
#define SCREEN_HEIGHT 272

#define		PIXELSIZE	1				//in short
#define		LINESIZE	512				//in short
#define		FRAMESIZE	0x44000			//in byte

#define CMAX_X 60
#define CMAX_Y 38
#define CMAX2_X 30
#define CMAX2_Y 19
#define CMAX4_X 15
#define CMAX4_Y 9

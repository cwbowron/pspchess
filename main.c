#include <stdio.h>
#include <pspkernel.h>
#include <pspdebug.h>
#include <pspctrl.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "pg.h"
#include "colors.h"
#include "piece_bitmaps.h"
#include "bce.h"

PSP_MODULE_INFO("pspChess", 0, 1, 1);

#define VERSION			"0.31"

#define SQUARE_SIZE		32
#define MAX_SEARCH_DEPTH	9
#define DEFAULT_SEARCH_DEPTH	4

#define KEY_LOADBOOK		(PSP_CTRL_SQUARE|PSP_CTRL_LEFT)
#define KEY_INC_SEARCH		(PSP_CTRL_SQUARE|PSP_CTRL_UP)
#define KEY_DEC_SEARCH		(PSP_CTRL_SQUARE|PSP_CTRL_DOWN)
#define KEY_TOGGLE_THINKING	(PSP_CTRL_SQUARE|PSP_CTRL_RIGHT)
#define KEY_SHOW_TEXT		(PSP_CTRL_SQUARE|PSP_CTRL_LTRIGGER)
#define KEY_CURSOR_UP		(PSP_CTRL_UP)
#define KEY_CURSOR_DOWN		(PSP_CTRL_DOWN)
#define KEY_CURSOR_LEFT		(PSP_CTRL_LEFT)
#define KEY_CURSOR_RIGHT	(PSP_CTRL_RIGHT)
#define KEY_SELECT		(PSP_CTRL_CROSS)
#define KEY_GO			(PSP_CTRL_RTRIGGER)
#define KEY_BACKUP		(PSP_CTRL_LTRIGGER)
#define KEY_FORCE		(PSP_CTRL_TRIANGLE)
#define KEY_CLEAR		(PSP_CTRL_CIRCLE)
#define KEY_OPTION		(PSP_CTRL_SELECT)
#define KEY_RESET		(PSP_CTRL_START)

#define ANALOG_THRESH		45
#define ANALOG_POS_THRESH	(127+ANALOG_THRESH)
#define ANALOG_NEG_THRESH	(127-ANALOG_THRESH)

chessboard g_board;
chessboard g_board_backup;

chessboard *board = NULL;
chessboard *g_display_board = NULL;

int g_flip_board = 0;
int g_board_flipping = 0;
int g_show_debug_info = 0;
int g_process_extra_keys = 1;
int g_show_options = 0;
int g_background_image = 1;

int g_default_clock = 10;
int g_default_increment = 0;

int g_minimum_computer_delay = 0;

int g_show_captures = 1;
int g_list_moves = 0;
int g_show_checks = 0;
int g_show_available_moves = 0;
int g_show_threats = 0;
int g_default_variant = 0;
chessboard g_initial_board_position;

extern long g_rnd_seed;
extern const unsigned char font[];

int cursor_x = 0;
int cursor_y = 7;

int g_main_thread_id;
int g_ponder_thread_id = -1;

int g_hardcore_debug = 0;

int g_captured_pieces[BKING+1];

int g_last_move;
int g_whose_turn;
int g_external_pieces = 0;

char g_external_pieces_file[MAXPATH] = "";

char g_san_moves[MAXMOVES][SAN_BUFFER_LEN];


int test_keys(int buttons, int keys)
{
    return (buttons & keys) == keys;
}

/* unsigned short rgb2col(unsigned char r,unsigned char g,unsigned char b) */
/* { */
/*     return ((((b>>3) & 0x1F)<<10)+(((g>>3) & 0x1F)<<5)+ */
/* 	    (((r>>3) & 0x1F)<<0)+0x8000); */
/* } */

void flash_display(char *text)
{
    int i;
    
    for (i=0;i<20;i++)
    {
	clear_screen();
	pgPrintCenter(10,i%2?red:green,text);
	pgScreenFlipV();
	pgWaitVn(15);
    }
}

void set_pixel(int x,int y,unsigned short color)
{
    unsigned short *vptr0= (unsigned short *)pgGetVramAddr(x,y);
    *vptr0 = color;
}

unsigned short get_pixel(int x,int y)
{
    unsigned short *vptr0= (unsigned short *)pgGetVramAddr(x,y);
    return *vptr0;
}

// DISPLAY AVAILABLE MOVES
void display_moves()
{
    move_and_score *restore = move_sp;
	      
    genmoves();

    int ox = 35;
    int oy = 1;

    clear_screen();
    draw_board();

    while (move_sp>restore)
    {
	move mv = popmove();
	print_move(ox,oy,mv);

	ox+=5;
	if (ox>55)
	{
	    ox = 35;
	    oy ++;
	}
    }
    pgScreenFlipV();
}



int draw_image(int x, int y, unsigned short *image)
{
    pgBitBlt(x,y,32,32,1,image);
    return 0;
}

int square_color(int x, int y)
{
    return (x+y) % 2 == 0 ? BLACK : WHITE;
}

void pgFancyPutChar(unsigned long x,unsigned long y,
		    unsigned long color,unsigned long bgcolor,
		    unsigned char ch,char drawfg,char drawbg,char mag,
		    int use_r, int use_g, int use_b)
{
    unsigned char *vptr0;		//pointer to vram
    unsigned char *vptr;		//pointer to vram
    const unsigned char *cfont;		//pointer to font
    unsigned long cx,cy;
    unsigned long b;
    char mx,my;

    //if (ch>255) return;
    cfont=font+ch*8;
    vptr0= (unsigned char *)pgGetVramAddr(x,y);
    for (cy=0; cy<8; cy++)
    {
	for (my=0; my<mag; my++)
	{
	    vptr=vptr0;
	    b=0x80;
	    for (cx=0; cx<8; cx++)
	    {
		for (mx=0; mx<mag; mx++)
		{
		    if ((*cfont&b)!=0)
		    {
			int r = 0;
			int g = 0;
			int b = 0;
			
			int f = cy;
			
#define COLOR_GRADIENT  8
#define COLOR_BASE	127
#define COLOR_MAX	255
			
#define RED(x) (((x)&0x1F)<<3)
#define GREEN(x) ((((x)>>5)&0x1F)<<3)
#define BLUE(x) ((((x)>>10)&0x1F)<<3)
			
#define SHADE(x,base,max)	(base+(((x)*(max-base))/COLOR_GRADIENT))

			/*
			int rc = RED(color);
			int gc = GREEN(color);
			int bc = BLUE(color);

			int min_r = max(0,rc-4)
			int min_g = max(0,gc-4);
			int min_b = max(0,bc-4);
			int max_r = min(255,rc+4);
			int max_g = min(255,gc+4);
			int max_b = min(255,bc+4);
			
			if (rc>0) r = SHADE(f,min_r,max_r);
			if (gc>0) g = SHADE(f,min_g,max_g);
			if (bc>0) b = SHADE(f,min_b,max_b);
			*/
			    
			if (use_r) r = SHADE(f,COLOR_BASE,COLOR_MAX);
			if (use_g) g = SHADE(f,COLOR_BASE,COLOR_MAX);
			if (use_b) b = SHADE(f,COLOR_BASE,COLOR_MAX);

			color=rgb2col(r,g,b);
			*(unsigned short *)vptr=color;
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


void pgFancyPrint(unsigned long x,unsigned long y,
		  unsigned long color,const char *str,
		  int use_r, int use_g, int use_b)
{
    while (*str!=0 && x<CMAX_X && y<CMAX_Y)
    {
	pgFancyPutChar(x*8,y*8,color,0,*str,1,0,1,use_r, use_g, use_b);
	str++;
	x++;
	if (x>=CMAX_X)
	{
	    x=0;
	    y++;
	}
    }
}

typedef struct {int piece; unsigned short *image;} piece_image_map;

piece_image_map white_square_map[] = 
{
    { 0, white_emptyData},
    { WPAWN, white_w_pawn_pngData},
    { BPAWN, white_b_pawn_pngData},
    { WKNIGHT, white_w_knight_pngData },
    { BKNIGHT, white_b_knight_pngData},
    { WBISHOP, white_w_bishop_pngData},
    { BBISHOP,white_b_bishop_pngData},
    { WROOK, white_w_rook_pngData},
    { BROOK, white_b_rook_pngData},
    { WQUEEN, white_w_queen_pngData},
    { BQUEEN, white_b_queen_pngData},
    { WKING, white_w_king_pngData},
    { BKING, white_b_king_pngData},
    { -1, NULL },
};

piece_image_map black_square_map[] = 
{
    { 0, black_emptyData},
    { WPAWN, black_w_pawn_pngData},
    { BPAWN, black_b_pawn_pngData},
    { WKNIGHT, black_w_knight_pngData },
    { BKNIGHT, black_b_knight_pngData},
    { WBISHOP, black_w_bishop_pngData},
    { BBISHOP, black_b_bishop_pngData},
    { WROOK, black_w_rook_pngData},
    { BROOK, black_b_rook_pngData},
    { WQUEEN, black_w_queen_pngData},
    { BQUEEN, black_b_queen_pngData},
    { WKING, black_w_king_pngData},
    { BKING, black_b_king_pngData},
    { -1, NULL },
};

unsigned short * piece_map_lookup(int p, piece_image_map *map)
{
    int i;

    for (i=0;;i++)
    {
	if (map[i].piece == -1) return map[i].image;
	if (map[i].piece == p) return map[i].image;
    }
}

unsigned short * get_piece_image_white_square(int p)
{
    return piece_map_lookup(p, white_square_map);
}

unsigned short * get_piece_image_black_square(int p)
{
    return piece_map_lookup(p, black_square_map);
}

int get_absolute_x(int x)
{
    return x * SQUARE_SIZE;
}

int get_absolute_y(int y)
{
    return (7-y) * SQUARE_SIZE;
}

int draw_piece(int x, int y, int p)
{
    int ab_x = get_absolute_x(x);
    int ab_y = get_absolute_y(y);

    if (square_color(x,y) == WHITE)
    {
	draw_image(ab_x,ab_y, get_piece_image_white_square(p));
    }
    else
    {
	draw_image(ab_x,ab_y, get_piece_image_black_square(p));	    
    }
    return 0;
}

int draw_box(int ab_x, int ab_y, int size, unsigned short color)
{
    int a;
    
    for (a=0;a<size;a++)
    {
	set_pixel(ab_x+a,ab_y,color);
	set_pixel(ab_x+a,ab_y+1,color);
	set_pixel(ab_x+a,ab_y+size-1,color);
	set_pixel(ab_x+a,ab_y+size-2,color);

	set_pixel(ab_x,ab_y+a,color);
	set_pixel(ab_x+1,ab_y+a,color);
	set_pixel(ab_x+size-2,ab_y+a,color);
	set_pixel(ab_x+size-1,ab_y+a,color);
    }

    return 0;
}

int draw_cursor(int x, int y, unsigned short color)
{
    int ab_x = get_absolute_x(x);
    int ab_y = get_absolute_y(7-y);

    draw_box(ab_x, ab_y, SQUARE_SIZE, color);
    return 0;
}

int mask_square(square s, unsigned short color)
{
    int x = g_flip_board ? 7-F(s) : F(s);
    int y = g_flip_board ? R(s) : 7-R(s);
    int i,j;
    int bit = 1;
    
    for (i=0;i<SQUARE_SIZE;i++)
    {
	for (j=0;j<SQUARE_SIZE;j++)
	{
	    bit = (i+j) % 2;

	    int ax = get_absolute_x(x) + i;
	    int ay = get_absolute_y(7-y) + j;

	    if (bit)
		set_pixel(ax,ay,color);
	}
    }
    return 0;
}


int hilite_square(square s, unsigned short color)
{
    int x = g_flip_board ? 7-F(s) : F(s);
    int y = g_flip_board ? R(s) : 7-R(s);

    draw_cursor(x,y,color);

    return 0;
}

int hilite_move(move m, unsigned short color_start, unsigned short color_end)
{
    hilite_square(FR(m),color_start);
    hilite_square(TO(m),color_end);
    return 0;
}

void clear_screen()
{
    if (g_background_image)
    {
	static int first_time = 1;
	static unsigned short dark_background[480*272];
    
	if (first_time)
	{
	    int x,y;
	    for (x=0;x<480;x++)
	    {
		for (y=0;y<272;y++)
		{
		    int c = background_image_Data[480*y+x];
		    int r, g, b;

		    r = RED(c)/3;
		    g = GREEN(c)/3;
		    b = BLUE(c)/3;
		    c = rgb2col(r,g,b);
	    
		    dark_background[480*y+x] = c;
		}
	    }

	    first_time = 0;
	}

	pgBitBlt(0,0,480,272,1,dark_background);
    }
    else
    {
	pgFillvram(0);
    }
}


int draw_board()
{
    int i,j;

    clear_screen();
    
    for (i=0;i<8;i++)
    {
	char file_ch = toupper(FL(g_flip_board ? 7-i : i));
	char rank_ch = RK(g_flip_board ? i : 7-i);

/* 	pgPutChar(8*(1+i*4),32*8,brown,black,file_ch,1,0,1); */
/* 	pgPutChar(32*8, (1+i*4)*8,brown,black,rank_ch,1,0,1); */
	msxPutCharAbsolute(i*32+14,32*8,brown,file_ch);
	msxPutCharAbsolute(32*8,i*32+14,brown,rank_ch);
	
	for (j=0;j<8;j++)
	{
	    int p = getpiece_display(i,j);

	    if (g_flip_board)
	    {
		draw_piece(7-i,7-j,p);
	    }
	    else
	    {
		draw_piece(i,j,p);
	    }
	}
    }

    char *by_line = "pspChess " VERSION	\
	" - Christopher Bowron - chess@bowron.us";
    
    pgPrintCenter(33,green,by_line);
    
    return 0;
}

int exit_callback(int arg1, int arg2, void *arg)
{
    shutdown_music();
    sceKernelExitGame(); 
    return 0; 
} 

// Thread to create the callbacks and then begin polling 
int CallbackThread(SceSize args, void *argp)
{ 
    int cbid = sceKernelCreateCallback("Exit Callback", exit_callback, NULL); 
   sceKernelRegisterExitCallback(cbid);
   sceKernelSleepThreadCB();
   return 0;
} 

/* Sets up the callback thread and returns its thread id */ 
int SetupCallbacks(void) 
{ 
   int thid = 0; 

   thid = sceKernelCreateThread("update_thread",
				CallbackThread, 0x11, 0xFA0, 0, 0);

   sceKernelStartThread(thid, 0, 0);
   
   return thid; 
} 

int start_pondering_callback(SceSize args, void *argp)
{
    ponder();
    return 0;
}

void copy_board(chessboard *dst, chessboard *src)
{
    memcpy(dst, src, sizeof(chessboard));
}

int start_pondering()
{
    copy_board(&g_board_backup, &g_board);
    g_display_board = &g_board_backup;

    g_main_thread_id = sceKernelGetThreadId();

    g_ponder_thread_id = sceKernelCreateThread("ponder_thread",
					       start_pondering_callback,
					       0x18,
					       0x10000,
					       0, 0); 

    if (g_ponder_thread_id >= 0)
    {
	sceKernelStartThread(g_ponder_thread_id, 0, 0);
    }

    return g_ponder_thread_id;
}

int stop_pondering()
{
    if (!ponder_mode) return 0;
    
    search_info.stop = 1;

    while (pondering)
    {
	pgWaitV();
    }
    
    g_display_board = board;
    return 0;
}

#define REPEAT_TIME 0x40000
static unsigned long control_bef_ctl  = 0;
static unsigned long control_bef_tick = 0;

unsigned long Read_Key(void)
{
    SceCtrlData ctl;

    sceCtrlPeekBufferPositive(&ctl, 1);

    if (ctl.Lx>ANALOG_POS_THRESH)
	ctl.Buttons |= PSP_CTRL_RIGHT;

    if (ctl.Lx<ANALOG_NEG_THRESH)
	ctl.Buttons |= PSP_CTRL_LEFT;

    if (ctl.Ly>ANALOG_POS_THRESH)
	ctl.Buttons |= PSP_CTRL_DOWN;

    if (ctl.Ly<ANALOG_NEG_THRESH)
	ctl.Buttons |= PSP_CTRL_UP;
	
    if (ctl.Buttons == control_bef_ctl)
    {
	if ((ctl.TimeStamp - control_bef_tick) > REPEAT_TIME)
	{
	    control_bef_tick = ctl.TimeStamp;
	    return control_bef_ctl;
	}
	return 0;
    }

    control_bef_ctl  = ctl.Buttons;
    control_bef_tick = ctl.TimeStamp;

    return control_bef_ctl;
}

int confirm_control(void)
{
    unsigned long key;

    while(1)
    {
	while(1)
	{
	    key = Read_Key();
	    if (key != 0) break;
	    pgWaitV();
	}

	if (key & PSP_CTRL_CROSS)
	{
	    return 1;
	}

	return 0;
    }
}

int user_prompt(char *message)
{
    clear_screen();

    pgPrintCenter(10,lightblue,message);
    pgPrintCenter(14,lightblue,"Press X to Accept");
    pgPrintCenter(16,lightblue,"Any other key to Ignore");
    pgScreenFlipV();

    return confirm_control();
}


void reset_game_confirm()
{
    clear_screen();

    if (user_prompt("Reset Game?"))
    {
	reset_game(1);
    }
}


char* itoa(int val, int base)
{
    static char buf[32] = {0};
    int i;
    int neg = 0;

    if (val<0)
    {
	neg = 1;
	val = -val;
    }

    i = 30;

    if (val==0)
    {
	buf[i] = '0';
	return &buf[i];
    }

    for(; val && i ; --i, val /= base)
	buf[i] = "0123456789abcdef"[val % base];

    if (neg)
    {
	buf[i] = '-';
	return &buf[i];
    }
    return &buf[i+1];
}

void pgPrintInt(unsigned long x,unsigned long y,unsigned long color,int n)
{
    if (color==white)
    {
	pgFancyPrint(x,y,color,itoa(n,10),1,1,1);
    }
    else if (color==red)
    {
	pgFancyPrint(x,y,color,itoa(n,10),1,0,0);
    }
    else
    {
	pgFancyPrint(x,y,color,itoa(n,10),1,0,1);
    }
}

square get_cursor_square(x,y)
{
    return g_flip_board ? SQ(7-x,y) : SQ(x,7-y);
}

void display_debug_info(int x, int y)
{
    int i;

    int eval = evaluate();
    pgFancyPrint(x,y,lightgrey,"Current Eval:",0,0,1);
    pgPrintInt(x+15,y,lightgrey, eval);

    y+= 2;
    
    pgFancyPrint(x,y,lightblue,"PAWNS:", 0,0,1);
    for (i=0;i<10;i++)
    {
	pgPrintInt(x+10+i,y,white,board->pawns[WHITE][i]);
	pgPrintInt(x+10+i,y+1,darkgray,board->pawns[BLACK][i]);
    }

    y+=2;
    pgFancyPrint(x,y,lightblue,"MATERIAL: ", 0,0,1);
    pgPrintInt(x+15,y,white,board->material[WHITE]);
    pgPrintInt(x+15,y+1,darkgray,board->material[BLACK]);

    y+=2;
    pgFancyPrint(x,y,lightblue,"POSITION: ", 0,0,1);
    pgPrintInt(x+15,y,white,board->position[WHITE]);
    pgPrintInt(x+15,y+1,darkgray,board->position[BLACK]);

    y+=2;
    pgFancyPrint(x,y,lightblue,"KING: ", 0,0,1);
    print_square(x+7,y,board->kings[WHITE]);
    print_square(x+7,y+1,board->kings[BLACK]);

    y+=2;
    pgFancyPrint(x,y,lightblue,"PIECES: ", 0,0,1);
    pgPrintInt(x+8,y,white,board->piececount[WHITE]);
    pgPrintInt(x+8,y+1,darkgray,board->piececount[BLACK]);

    pgFancyPrint(14+x,y-1,lightblue,"PNBRQK", 0,0,1);
    for (i=pawn;i<=king;i++)
    {
	pgPrintInt(14+x-1+i,y,white,board->pieces[makepiece(WHITE,i)]);
	pgPrintInt(14+x-1+i,y+1,darkgray,board->pieces[makepiece(BLACK,i)]);
    }

    y+=2;
    pgFancyPrint(x,y,lightblue,"FLAGS: ", 0,0,1);
    pgFancyPrint(x+8,y,lightblue,formatflags(),0,1,1);
}

void display_options(int x, int y)
{
    pgFancyPrint(x,y,lightgrey,"Search Depth: ",0,1,0);
    pgPrintInt(x+15,y,lightgrey, searchdepth);

    pgFancyPrint(x,++y,lightblue,"Show Thoughts: ", 0,1,0);
    pgFancyPrint(x+15,y,white,g_show_thinking?"On":"Off",1,1,1);

    pgFancyPrint(x,++y,lightblue,"Board Flipping: ", 0,1,0);
    pgFancyPrint(x+15,y,white,g_board_flipping?"On":"Off",1,1,1);
	    
    pgFancyPrint(x,++y,lightblue,"Pondering: ", 0,1,0);
    pgFancyPrint(x+15,y,white,ponder_mode?"On":"Off",1,1,1);

    pgFancyPrint(x,++y,lightblue,"Debug Info: ", 0,1,0);
    pgFancyPrint(x+15,y,white,g_show_debug_info?"On":"Off",1,1,1);

    pgFancyPrint(x,++y,lightblue,"Extra Keys: ", 0,1,0);
    pgFancyPrint(x+15,y,white,g_process_extra_keys?"On":"Off",1,1,1);
}

void display_time(int x, int y, unsigned short color, int ms)
{
    int seconds = (ms / 1000) % 60;
    int minutes = (ms / 1000) / 60;

    char *min_string = itoa(minutes,10);
    int offset = strlen(min_string);

    int r,g,b;

    if (color==white)
    {
	r = g = b = 1;
    }
    else
    {
	r = 1;
	g = 0;
	b = 1;
    }
    
    if (ms>0)
    {
 	pgFancyPrint(x,y,color,min_string,r,g,b);
	pgFancyPrint(x+offset,y,color,":",r,g,b);
	if (seconds>9)
	{
	    pgFancyPrint(x+offset+1,y,color,itoa(seconds,10),r,g,b);
	}
	else
	{
	    pgFancyPrint(x+offset+1,y,color,"0",r,g,b);
	    pgFancyPrint(x+offset+2,y,color,itoa(seconds,10),r,g,b);
	}
    }
    else
    {
	pgFancyPrint(x,y,red,"0:00",1,0,0);
    }
}

void display_clocks()
{
    pgPrint(35,25,white,"WHITE:");
    pgPrint(35,26,darkgray,"BLACK:");
    display_time(45,25,white,chessclock[WHITE]);
    display_time(45,26,darkgray,chessclock[BLACK]);
}


// if a key's pressed return it, otherwise sleep the thread
// so we can ponder... 
int get_buttons()
{
    int key;

    while ((key = Read_Key()) == 0)
    {
	// allow thread swapping for pondering...
	if (ponder_mode)
	{
	    //sceKernelSleepThread(0);
	    //sceKernelRotateThreadReadyQueue(0x8);
	}
    }
    
    return key;
}

// hardcore == 1: starting a new game
// hardcore == 0: resetting the current game
void reset_game(int hardcore)
{
    setupboard();

    computer[WHITE] = 0;
    computer[BLACK] = 1;
    chessclock[0]=chessclock[1]=g_default_clock*60*1000;
    
    ply = 0;
    move_sp = move_stack;
    setup_default_weights();

    // variants that have randomness in them must copy
    // from the saved board position if hardcore == 0
    // so that previewing stored games and such do not
    // fuck up the game in progress
    // variants with no randomness can just be setup normally
    // in either case
    switch (g_default_variant)
    {
	case VARIANT_NORMAL:
	    ply = 0;
	    if (hardcore && (g_game_mode != GAMEMODE_CLIENT))
	    {
		loadbook();
	    }
	    setupboard();
	    ply = 0;
	    break;
	case VARIANT_FISCHER_RANDOM:
	    if (hardcore)
		setupboard_fischer_random();
	    else
		memcpy(board, &g_initial_board_position, sizeof(chessboard));
	    break;
	case VARIANT_WILD_5:
	    setupboard_wild_5();
	    break;
	case VARIANT_WILD_8:
	    setupboard_wild_8();
	    break;
	case VARIANT_WILD_8A:
	    setupboard_wild_8a();
	    break;
    }

    memcpy(&g_initial_board_position, board, sizeof(chessboard));
    
    change_gamemode(OPENING,0);
    memset(g_analysis_stack, 0, sizeof(g_analysis_stack));
}

int darken_board()
{
    int i, j, r, g, b;

    for (i=0;i<32*8;i++)
    {
	for (j=0;j<32*8;j++)
	{
	    int c = get_pixel(i,j);
	    
	    r = RED(c)/2;
	    g = GREEN(c)/2;
	    b = BLUE(c)/2;
	    c = rgb2col(r,g,b);

	    set_pixel(i,j,c);
	}
    }

    return 0;
}

void clear_text_area(int x_, int y_, int dx, int dy)
{
    unsigned short *vptr0=(unsigned short *)pgGetVramAddr(0,0);
    int x, y;
    
    int maxx = x_+dx;
    int maxy = y_+dy;
    
    for (x=x_*8;x<maxx*8;x++)
    {
	for (y=y_*8;y<maxy*8;y++)
	{
	    vptr0[512*y+x] = 0;
	}
    }
}

void draw_captured_pieces()
{
    int p;
    int offset;

    int ab_x = 33*8;
    int ab_y = 7*8;

    pgBitBltMask(ab_x,ab_y,206,113,1,image_captured_pieces_bevel,0);

    for (p=pawn,offset=0;p<king;p++,offset++)
    {
	int bp = makepiece(BLACK,p);
	int wp = makepiece(WHITE,p);

	ab_x = 35*8+offset*32+8;
	ab_y = 11*8;
	
	if (g_captured_pieces[bp])
	{
	    draw_image(ab_x,ab_y,get_piece_image_black_square(bp));

	    if (g_captured_pieces[bp]>1)
	    {
		char bf[4];
		sprintf(bf,"%d",g_captured_pieces[bp]);
		
		//pgPrintInt(ab_x/8+3,ab_y/8+3,red,g_captured_pieces[bp]);
		msxPutString(ab_x/8+3,ab_y/8+3,lawngreen,bf);
	    }
	}
	if (g_captured_pieces[wp])
	{
	    draw_image(ab_x,ab_y+32,get_piece_image_black_square(wp));

	    if (g_captured_pieces[wp]>1)
	    {
		char bf[4];
		sprintf(bf,"%d",g_captured_pieces[wp]);

		//pgPrintInt(ab_x/8+3,ab_y/8+4+3,red,g_captured_pieces[wp]);
		msxPutString(ab_x/8+3,ab_y/8+4+3,lawngreen,bf);
	    }
	}
    }
}


// list moves made so far at screen location x any y, show at most
// max plies.
void list_game_moves(int x, int y, int max)
{
    int an_y = y;
    int an_ply;

    an_ply = (ply<max) ? 0 : ply-max;

    // always start with white
    if (an_ply%2==1) an_ply++;
		
    while (1)
    {
	if (an_ply>=ply) break;

	// draw move number
	pgPrintInt(x,an_y,white,(an_ply/2)+1);

	// draw white move
	if (g_list_moves == LIST_MOVES_COORD)
	    print_move(x+3,an_y,gamestack[an_ply]);
	else
	    msxPutString(x+3,an_y,cyan,g_san_moves[an_ply]);

	an_ply++;
	if (an_ply>=ply) break;

	// draw black move
	if (g_list_moves == LIST_MOVES_COORD)
	    print_move(x+10,an_y,gamestack[an_ply]);
	else
	    msxPutString(x+9,an_y,cyan,g_san_moves[an_ply]);

	an_ply++;
	an_y++;
    }
}

// domove wrapper that will potentially save the move on
// the analysis stack and also keeps track of timing info.
void local_domove(move m, int force_analysis_storage)
{
    static long time_of_last_move = 0;
    long end_time;
    int turn;
    
    if (force_analysis_storage || (g_game_mode == GAMEMODE_ANALYSIS))
    {
	g_analysis_stack[ply] = m;
    }

    strcpy(g_san_moves[ply],get_san(m));

    if (g_game_mode == GAMEMODE_CLIENT)
    {
	net_send_line(g_san_moves[ply]);
    }
    else
    {
	turn = tomove();
    
	domove(m);
	g_last_move = m;
	g_whose_turn = tomove();

	end_time = get_ms();
    
	if (time_of_last_move)
	{
	    chessclock[turn] -= (end_time-time_of_last_move);
	    chessclock[turn] += g_default_increment*1000;
	}
    
	time_of_last_move = end_time;
    }
}

int user_select_promo_piece(int color)
{
    int drawn_pieces[] = { queen, knight, rook, bishop };
    
    int cursor = 0;
    int key;
    int max_cursor = sizeof(drawn_pieces)/sizeof(drawn_pieces[0]);
    int i;
    
    while (1)
    {
	clear_screen();
	pgPrintCenter(5,white,"Select Promotion Piece");
	
	for (i=0;i<max_cursor;i++)
	{
	    chesspiece p = makepiece(color, drawn_pieces[i]);
	    
	    int x = 480/2 - max_cursor/2 * 32 + i*32;
	    int y = 70;

	    draw_image(x,y, get_piece_image_black_square(p));

	    if (cursor == i)
	    {
		draw_box(x,y,SQUARE_SIZE,cyan);
	    }
	}
	
	pgScreenFlipV();
	
	while ((key = Read_Key()) == 0) ;

	if (key & PSP_CTRL_LEFT)
	    cursor --;
	if (key & PSP_CTRL_RIGHT)
	    cursor ++;
	if (key & PSP_CTRL_CROSS)
	    break;

	if (cursor>=max_cursor)
	    cursor = 0;
	if (cursor<0)
	    cursor = max_cursor-1;
    }

    return drawn_pieces[cursor];
}

extern u8 msx[];

// modified from pspsdk
void msxPutCharAbsolute(int x, int y, unsigned short color, u8 ch)
{
   int 	i,j, l;
   u8	*font;
   u32  pixel;
   int offset;

   if (!isprint(ch))
       return;

   if (x>=480-8)
       return;
   if (y>=272-8)
       return;
   
   unsigned short *vram_ptr = (unsigned short*)pgGetVramAddr(x,y);
   
   font = &msx[ (int)ch * 8];
   
   for (i=l=0; i < 8; i++, l+= 8, font++)
   {
      for (j=0; j < 8; j++)
      {
	  if ((*font & (128 >> j)))
	  {
	      pixel = color;
	      offset = i*512+j;
	      *(vram_ptr+offset) = pixel;
	  }
      }
   }
}


void msxPutChar( int x, int y, unsigned short color, u8 ch)
{
    msxPutCharAbsolute(x*8,y*8,color,ch);
}

void msxPutString(int x, int y, unsigned short color, char *string)
{
    int i;
    int len = strlen(string);
    
    for (i=0;i<len;i++)
    {
	msxPutCharAbsolute(x*8+i*6,y*8,color,string[i]);
    }
}

void hilite_checks()
{
    int c = tomove();
    //int f = F(board->kings[c]);
    //int r = R(board->kings[c]);

    square king_square = board->kings[c];
    
    move_and_score *restore_sp = move_sp;
    
    //board->flags ^= turn_flag;
    switch_sides();

    genattacks();

    while (move_sp>restore_sp)
    {
	move m = popmove();

	if (TO(m) == king_square)
	{
	    mask_square(FR(m),red);
	}
    }
    
    //board->flags ^= turn_flag;
    switch_sides();
}


void show_cursor_destinations(square start)
{
    move_and_score *restore_sp = move_sp;
    
    genmoves();
    
    while (move_sp>restore_sp)
    {
	move m = popmove();

	if ((FR(m) == start) && (validmove(m) == 1))
	{
	    mask_square(TO(m),green);
	}
    }
}

void show_threats()
{
    move_and_score *restore_sp = move_sp;
    
    switch_sides();

    genattacks();

    while (move_sp>restore_sp)
    {
	move m = popmove();

	if (validmove(m) == 1)
	{
	    mask_square(TO(m),blue);
	}
    }
    
    switch_sides();
}

// if the move is valid as both a castling move and a normal king
// move ask the user which one.  otherwise return the valid move.
// if neither are valid the regular move version will be returned
// and we can print invalid move then...
move possible_king_castle(square start, square end)
{
    int ca = validmove(MV_CA(start,end));
    int nc = validmove(MV(start,end));

    if ((ca == 1)&&(nc == 1))
    {
	char *choices[] = {"No Castling","Castling"};
			
	int selection = user_select_string(choices,2);
	if (selection==0)
	    return MV(start,end);
	else
	    return MV_CA(start,end);
    }
    else if (ca == 1)
    {
	return MV_CA(start,end);
    }
    else
    {
	return MV(start,end);
    }
}


int main(void)
{
    // normally we could use lastmove() macro to get the last move...
    // but if we are pondering it will affect the gamestack, which
    // will affect the lastmove() macro result.  So we will cache
    // the last move to allow pondering

    g_last_move = dummymove;
    
    // Initialize
    pgInit();
    SetupCallbacks();
    pgScreenFrame(2,0);

    sceCtrlSetSamplingCycle(0);
    sceCtrlSetSamplingMode(1);

    int redraw = 1;

    board = &g_board;
    g_display_board = board;

    square move_start = dummysq;
    int show_invalid_move = 0;
    
    searchdepth = DEFAULT_SEARCH_DEPTH;

    int tempy = get_ms();
    g_rnd_seed = tempy;

    compute_randoms();
    
    ponder_mode = 0;

    int start_pondering_dammit = 0;

    // load saved options
    load_options();

    startup_music();

    if (g_external_pieces)
    {
	if (read_png_file(g_external_pieces_file) == 0)
	    process_pieces_file();
    }

    reset_game(1);

    g_whose_turn = tomove();

    if (g_game_mode == GAMEMODE_CLIENT)
    {
	computer[BLACK] = 0;
	computer[WHITE] = 0;
	init_network();
	terminal_mode();
    }
    
    while(1)
    {
	restart_music_if_necessary();
	
	if (g_board_flipping) g_flip_board = (g_whose_turn == BLACK);

	if (redraw) 
	{
	    int y;
	    
	    draw_board();

	    if (move_start != dummysq )
	    {
		hilite_square(move_start, lawngreen);
	    }

	    pgBitBltMask(270,5,64,14,1,image_tomove,0);
	    
	    if (g_whose_turn == WHITE)
		pgFancyPrint(42,1,white,"WHITE",1,1,1);
	    else
		pgFancyPrint(42,1,darkgray,"BLACK",1,0,1);

	    if (!pondering)
	    {
		int game_status = gameoverp(g_whose_turn);
	    
		if (game_status != IN_PROGRESS)
		{
		    computer[WHITE] = 0;
		    computer[BLACK] = 0;
		    
		    switch (game_status)
		    {
			case NON_MATERIAL:
			    pgFancyPrint(35,5,red,"Draw (Material)", 1,0,0);
			    break;
			case REP_DRAW:
			    pgFancyPrint(35,5,red,"Draw (Repetition)", 1,0,0);
			    break;
			case END_CHECKMATE:
			    pgFancyPrint(35,5,red,"Checkmate", 1,0,0);
			    break;
			case END_STALEMATE:
			    pgFancyPrint(35,5,red,"Stalemate", 1,0,0);
			    break;
		    }
		}
		else if (fullincheckp(g_whose_turn))
		{
		    pgFancyPrint(50,1,red,"(In Check)", 1,0,0);
		}
	    }
	    
	    if (ply>0)
	    {
		pgBitBltMask(270,20,81,14,1,image_lastmove,0);
		print_move(44,3,g_last_move);

		//pgFancyPrint(49,3,white,g_san_moves[ply-1],1,1,1);
		msxPutString(49,3,white,g_san_moves[ply-1]);
		
		hilite_move(g_last_move,magenta,purple);
	    }

	    if (g_list_moves)
	    {
		int list_y;
		int last_y;
		
		if (g_show_captures)
		    list_y = 21;
		else
		    list_y = 7;

		if (g_show_options)
		    last_y = 22;
		else if (g_game_mode == GAMEMODE_ANALYSIS)
		    last_y = 30;
		else
		    last_y = 29;

		list_game_moves(35,list_y,(last_y-list_y)*2);
	    }

	    if (g_game_mode == GAMEMODE_ANALYSIS)
	    {
		pgFancyPrint(35,31,white,"Analysis Mode",1,1,1);
	    }
	    else
	    {
		if (g_game_mode == GAMEMODE_NORMAL)
		{
		    pgFancyPrint(35,30,white,
				 g_personality_names[computer[WHITE]],
				 1,1,1);
		    pgFancyPrint(35,31,darkgray,
				 g_personality_names[computer[BLACK]],
				 1,0,1);
		}
		else if (g_game_mode == GAMEMODE_CLIENT)
		{
		    pgFancyPrint(35,30,white,g_server_white,1,1,1);
		    pgFancyPrint(35,31,darkgray,g_server_black,1,0,1);
		}
		
		display_time(50,30,white,chessclock[WHITE]);
		display_time(50,31,darkgray,chessclock[BLACK]);
	    }
	    
	    if (g_show_checks)
		hilite_checks();

	    if (g_show_available_moves)
	    {
		if (move_start == dummysq)
		{
		    show_cursor_destinations(
			get_cursor_square(cursor_x,cursor_y));
		}
		else
		{
		    show_cursor_destinations(move_start);
		}
	    }

	    if (g_show_threats)
	    {
		show_threats();
	    }
	    
	    hilite_square(get_cursor_square(cursor_x,cursor_y), red);

	    if (show_invalid_move)
	    {
		y = 5;
		pgFancyPrint(35,y,red,"Invalid Move!",1,0,0);
		print_move(50,y,show_invalid_move);
		show_invalid_move = 0;
	    }

	    if (computer[g_whose_turn])
	    {
		y = 5;
		pgBitBltMask(440,5,34,32,1,image_thinking,0);
		darken_board();
	    }


	    if (g_game_mode == GAMEMODE_CLIENT)
	    {
		//process_some_network_data_if_necessary();
		display_terminal_screen_board_mode();
	    }
	    else
	    {
		if (g_show_captures)
		    draw_captured_pieces();
		
		y = 7;

		if (g_show_debug_info)
		{
		    if (move_start != dummysq)
		    {
			print_square(35,y,move_start);
			pgFancyPrint(37,y,lightblue,"-",0,1,1);
			print_square(38,y,
				     get_cursor_square(cursor_x,cursor_y));
		    }
		    else
		    {
			print_square(35,y,
				     get_cursor_square(cursor_x,cursor_y));
		    }
		}
	    
		if (g_show_options)
		    display_options(35,23);

		if (g_show_debug_info)
		    display_debug_info(35,9);
	    }
	    
	    pgScreenFlipV();
	    redraw = 0;
	}

	if (!Read_Key() && computer[g_whose_turn])
	{
	    long start_time = get_ms();

	    // if computers are using different evaluation
	    // function clear out the transposition tables
	    if (computer[WHITE] && computer[BLACK] &&
		computer[WHITE] != computer[BLACK] &&
		gamemode != OPENING)
	    {
		clear_hash();
	    }

	    evaluate = g_evaluation_functions[computer[g_whose_turn]];
	    
	    move m = bce();

	    if (validmove(m) != 1)
	    {
		clear_screen();
		draw_board();
		pgFancyPrint(35,5,red,"Crap - Invalid Move",1,0,0);
		print_move(35,6,m);
		print_best_line(35,7,g_best_line,5);
		pgPrintInt(35,3,red,m);
		pgScreenFlipV();

		while (Read_Key () == 0) ;

		m = bce();

		clear_screen();
		print_move(35,7,m);
		pgScreenFlipV();
		
		while (Read_Key () == 0) ;

		computer[WHITE] = 0;
		computer[BLACK] = 0;

		//g_hardcore_debug = 1;
	    }
	    
	    if (g_minimum_computer_delay)
	    {
		for (;;)
		{
		    long now_time = get_ms();

		    long delay = now_time - start_time;

		    if (delay >= g_minimum_computer_delay * 1000)
			break;

		    pgScreenCopy();
		    clear_text_area(35,5,5+strlen("Delaying"),1);
		    pgFancyPrint(35,5,red,"Delaying:",0,1,1);
		    pgPrintInt(45,5,green,g_minimum_computer_delay-(delay/1000));
		    pgScreenFlipV();
		    pgWaitVn(30);
		}
	    }
	    
	    local_domove(m,0);
	    
	    redraw = 1;

	    if (ponder_mode && computer[g_whose_turn] == 0)
	    {
		start_pondering_dammit = 1;
	    }
	}
	else
	{
	    if (start_pondering_dammit)
	    {
		start_pondering();
		start_pondering_dammit = 0;
	    }

	    int buttons = 0;
	    
	    switch (g_game_mode)
	    {
		case GAMEMODE_ANALYSIS:
		case GAMEMODE_NORMAL:
		    buttons = get_buttons();
		    break;
		case GAMEMODE_CLIENT:
		    buttons = Read_Key();
		    while ((buttons == 0) &&
			   !process_some_network_data_if_necessary())
		    {
			pgWaitV();
			buttons = Read_Key();
		    }
		    
		    break;
	    }

	    redraw = 1;

	    if (g_process_extra_keys&&test_keys(buttons, KEY_LOADBOOK))
	    {
		screen_dump();
	    }
	    else
	    if (g_process_extra_keys&&test_keys(buttons, KEY_DEC_SEARCH))
	    {
		searchdepth--;
		if (searchdepth<1)
		    searchdepth = MAX_SEARCH_DEPTH;
	    }
	    else if (g_process_extra_keys&&test_keys(buttons, KEY_INC_SEARCH))
	    {
		searchdepth++;
		if (searchdepth>MAX_SEARCH_DEPTH)
		    searchdepth = 1;
	    }
	    else if (g_process_extra_keys&&test_keys(buttons, KEY_TOGGLE_THINKING))
	    {
		g_show_thinking ^= 1;
	    }
	    else if (test_keys(buttons, PSP_CTRL_SQUARE|PSP_CTRL_SELECT))
	    {
		display_moves();
		while (0==Read_Key());
	    }
	    else if (test_keys(buttons, KEY_SHOW_TEXT))
	    {
		pgn_show_text();
	    }
	    else if (test_keys(buttons, KEY_BACKUP))
	    {
		stop_pondering();

		switch (g_game_mode)
		{
		    case GAMEMODE_NORMAL:
			// fall through
		    case GAMEMODE_ANALYSIS:
			cmd_undo(NULL);
			break;
			
		    case GAMEMODE_CLIENT:
			// do nothing... maybe send takeback?
			break;
		}
		if (ply>0)
		    g_last_move = lastmove();
		else
		    g_last_move = dummymove;

		g_whose_turn = tomove();
	    }
	    else if (test_keys(buttons, KEY_GO))
	    {
		stop_pondering();

		switch (g_game_mode)
		{
		    case GAMEMODE_CLIENT:
			terminal_mode();
			break;
		    case GAMEMODE_NORMAL:
			cmd_go(NULL);
			break;
		    case GAMEMODE_ANALYSIS:
			if (!g_analysis_stack[ply])
			{
			    user_message_wait_key("Past Analysis Stack");
			}
			else if (validmove(g_analysis_stack[ply]) != 1)
			{
			    user_message_wait_key(
				"Invalid Move on Analysis Stack");
			}
			else
			{
			    local_domove(g_analysis_stack[ply],0);
			}
			break;
		}
	    }
	    else if (test_keys(buttons, KEY_CLEAR))
	    {
		move_start = dummysq;
	    }
	    else if (test_keys(buttons, KEY_FORCE))
	    {
		stop_pondering();
		computer[WHITE] = 0;
		computer[BLACK] = 0;
	    }
	    else if (test_keys(buttons, KEY_OPTION))
	    {
		stop_pondering();
		option_menu();

		if (g_game_mode == GAMEMODE_ANALYSIS)
		{
		    computer[WHITE] = 0;
		    computer[BLACK] = 0;
		}
		
		g_last_move = lastmove();
	    }
	    else if (test_keys(buttons, KEY_RESET))
	    {
		switch (g_game_mode)
		{
		    case GAMEMODE_NORMAL:
		    case GAMEMODE_ANALYSIS:
			stop_pondering();
			reset_game_confirm();
			break;
		    case GAMEMODE_CLIENT:
			terminal_mode();
			break;
		}
	    }
	    else if (test_keys(buttons, KEY_SELECT))
	    {
		if (move_start == dummysq)
		{
		    move_start = get_cursor_square(cursor_x, cursor_y);
		}
		else
		{
		    stop_pondering();
			
		    square end = get_cursor_square(cursor_x,cursor_y);

		    int piece = getpiece__(move_start);
		    int piece_value = chesspiecevalue(piece);
		    int piece_color = chesspiececolor(piece);
		    int rank = R(end);

		    move m;
		    
		    if (((piece == WPAWN)&&(rank==7))||
			((piece == BPAWN)&&(rank==0)))
		    {
			int promo = user_select_promo_piece(piece_color);
			m = MV_PR(move_start,end,promo);
		    }
		    else if (piece_value == king)
		    {
			m = possible_king_castle(move_start,end);
		    }
		    else
		    {
			m = MV(move_start,end);
		    }

		    move_start = dummysq;

		    if (validmove(m) == 1)
		    {
			int turn = tomove();

			local_domove(m,0);

			if (fullincheckp(turn))
			{
			    flash_display("poop - i let you move into check");
			}
		    }
		    else
		    {
			show_invalid_move = m;
		    }
		}
	    }
	    else
	    {
		if (test_keys(buttons, KEY_CURSOR_DOWN))
		{
		    cursor_y++;
		}
		if (test_keys(buttons, KEY_CURSOR_UP))
		{
		    cursor_y--;
		}
		if (test_keys(buttons, KEY_CURSOR_LEFT))
		{
		    cursor_x--;
		}
		if (test_keys(buttons, KEY_CURSOR_RIGHT))
		{
		    cursor_x++;
		}
	    }
	    
	    if (cursor_y>7) cursor_y = 0;
	    if (cursor_y<0) cursor_y = 7;
	    if (cursor_x>7) cursor_x = 0;
	    if (cursor_x<0) cursor_x = 7;
	}
    }
    return 0;
}

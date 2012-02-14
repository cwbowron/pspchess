#include <pspkernel.h>
#include <pspctrl.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "pg.h"
#include "bce.h"
#include "colors.h"

char *g_osk_characters =
	"abcdefghijklmnopqrstuvwxyz1234567890,./;'[]\\-=`";
char *g_osk_shift_characters =
	"ABCDEFGHIJKLMNOPQRSTUVWXYZ!@#$%^&*()<>?:\"{}|_+~";

char g_osk_buffer[MAX_STRING];
int g_osk_shift = 0;
int g_osk_cursor = 0;

char get_osk_character(int key, int cursor)
{
    int max = (strlen(g_osk_characters)+3)/4;

    int c = cursor % max;
    
    while (c<0) c += max;

    int s_pos = c*4;

    char *chs = (g_osk_shift) ?
	&g_osk_shift_characters[s_pos] :
	&g_osk_characters[s_pos];

    if (key & PSP_CTRL_TRIANGLE)
	return chs[0];
    else if (key & PSP_CTRL_CIRCLE)
	return chs[3];
    else if (key & PSP_CTRL_CROSS)
	return chs[2];
    else if (key & PSP_CTRL_SQUARE)
	return chs[1];
    return 0;
}

void draw_osk()
{
    int middle_x = 240;

    int y = 230;
    int i;

    msxPutString(5,27,lawngreen,g_osk_buffer);
    msxPutCharAbsolute((strlen(g_osk_buffer))*6+5*8,27*8,cyan,'_');
    
    //pgFillBox(middle_x-16, y-4,middle_x+13, y+26, darkgray);
    pgDrawFrame(middle_x-16, y-4,middle_x+13, y+26, gray);
    pgDrawFrame(30,y-6,480-30,y+28,gray);
    
    for (i=-6;i<=6;i++)
    {
	int color = (i==0) ? cyan : gray;

	int px = middle_x + (i*30);

	msxPutCharAbsolute(
	    px-4,y+1,color,
	    get_osk_character(PSP_CTRL_TRIANGLE,g_osk_cursor+i));
	
	msxPutCharAbsolute(px+3,y+8,color,
			   get_osk_character(PSP_CTRL_CIRCLE,g_osk_cursor+i));
	msxPutCharAbsolute(px-4,y+15,color,
			   get_osk_character(PSP_CTRL_CROSS,g_osk_cursor+i));
	msxPutCharAbsolute(px-11,y+8,color,
			   get_osk_character(PSP_CTRL_SQUARE,g_osk_cursor+i));
    }
}

int quick_string_select()
{
    char *qs_strings[QUICK_COUNT];
    int i;

    for (i=0;i<QUICK_COUNT;i++)
	qs_strings[i] = g_server_quickstring[i];
    
    return user_select_string(qs_strings,QUICK_COUNT);
}

// return  0 on normal processing
// return  1 on "enter" (2x rtrigger)
// return -1 on "abort" (start)
int osk_process_key(int keys)
{
    int ret_val = 0;
    
    char ch;

    if (keys & PSP_CTRL_DOWN)
    {
	if (keys & PSP_CTRL_LTRIGGER)
	{
	    strcpy(g_osk_buffer,"");
	}
    }
    else if (keys & PSP_CTRL_LEFT)
    {
	g_osk_cursor --;
    }
    else if (keys & PSP_CTRL_RIGHT)
    {
	g_osk_cursor ++;
    }
    else if (keys & PSP_CTRL_RTRIGGER)
    {
	int len = strlen(g_osk_buffer);
	if (g_osk_buffer[len-1] == ' ')
	{
	    g_osk_buffer[len-1] = 0;
	    ret_val = 1;
	}
	else
	{
	    g_osk_buffer[len] = ' ';
	    g_osk_buffer[len+1] = 0;
	}
    }
    else if (keys & PSP_CTRL_LTRIGGER)
    {
	g_osk_buffer[strlen(g_osk_buffer)-1] = 0;
    }
    else if (keys & PSP_CTRL_UP)
    {
	g_osk_shift ^= 1;
    }
    else if (keys & PSP_CTRL_SELECT)
    {
	int qs = quick_string_select();
	if (qs>=0)
	{
	    strcat(g_osk_buffer,g_server_quickstring[qs]);
	}
    }
    else if (keys & PSP_CTRL_START)
    {
	ret_val = -1;
    }
    else if ((ch = get_osk_character(keys, g_osk_cursor)))
    {
	int len = strlen(g_osk_buffer);
	g_osk_buffer[len] = ch;
	g_osk_buffer[len+1] = 0;
    }

    return ret_val;
}


void osk_get_string(char *buffer, int max)
{
    strcpy(g_osk_buffer,buffer);
    while (1)
    {
	clear_screen();
	draw_osk();
	pgScreenFlipV();
	int keys = Read_Key();

	int osk_r = osk_process_key(keys);

	if (osk_r)
	{
	    if (osk_r>0)
		strncpy(buffer,g_osk_buffer,max);
	    break;
	}
    }
}

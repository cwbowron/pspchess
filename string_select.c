#include <pspkernel.h>
#include <pspctrl.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <pspiofilemgr_fcntl.h>

#include "pg.h"
#include "colors.h"
#include "bce.h"

void draw_strings(char **strings, int nfiles,
			    int x, int y, int display_start_index,
			    int n_display, int selection)
{
    int i;

    for (i=display_start_index;i<display_start_index+n_display;i++)
    {
	if (i>=nfiles) break;
	
	if (i == selection)
	{
	    msxPutString(x,y,yellow,"->");
	    msxPutString(x+2,y,yellow,strings[i]);
	}
	else
	{
	    msxPutString(x+2,y,cyan,strings[i]);
	}
	y++;
    }

    pgScreenFlipV();
}

// allow user to navigate an array of strings
// return -1 if circle pressed
// return index if cross pressed
int user_select_string(char **strings, int n)
{
    int cursor = 0;
    int display_start = 0;
    int n_display = 20;

    while (1)
    {
	if (cursor>=display_start+n_display)
	    display_start = cursor-n_display+1;
	if (cursor<display_start)
	    display_start = cursor;

	int x = 10;
	int y = 5;
	
	clear_screen();
	clear_text_area(x,y,60-2*x,n_display+4);
	y+=2;
	
	pgPrintCenter(29,red,"X = Select");
	pgPrintCenter(30,red,"O = Abort");
	
	draw_strings(strings, n, x, 7,
		     display_start, n_display, cursor);
	
	int key = get_buttons();

	if (key & PSP_CTRL_DOWN)
	    cursor ++;
	if (key & PSP_CTRL_UP)
	    cursor --;
	if (key & PSP_CTRL_RIGHT)
	    cursor += n_display;
	if (key & PSP_CTRL_LEFT)
	    cursor -= n_display;
	if (key & PSP_CTRL_CIRCLE)
	    return -1;
	if (key & PSP_CTRL_CROSS)
	    return cursor;
	
	if (cursor>=n)
	    cursor = n-1;
	if (cursor<0)
	    cursor = 0;
    }
}

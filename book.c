/*
  This File is part of Boo's Chess Engine
  Copyright 2000 by Christopher Bowron
*/  
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include "bce.h"

#include "colors.h"
#include "pg.h"

int index_count;
int binary_book = 0;

#define BOOK_SIZE 800

extern move book_moves[];

move bookopening()
{
    int i;
    
    int matches = 0;
    int frequency = 0;
    
    move last_match = 0;
    
    for (i=0;i<index_count;i++)
    {
	if (mem.book[i].hash == board->hash)
	{
	    matches ++;
	    frequency += mem.book[i].count;
	    last_match = mem.book[i].m;
	}
    }

    if (matches == 0)
	return dummymove;
    else if (matches == 1)
	return last_match;
    else
    {
	int n = getrandomnumber() % frequency;

	for (i=0;i<index_count;i++)
	{
	    if (mem.book[i].hash == board->hash)
	    {
		n -= mem.book[i].count;

		if (n <= 0)
		    return mem.book[i].m;
	    }
	}
    }
    
    return dummymove;
}

int process_opening(move *move_ptr)
{
    setupboard();
    ply = 0;
    
    while (*move_ptr)
    {
	int i;
	move m = *move_ptr;

	move_ptr++;
	
	for (i=0;i<TABLE_SIZE;i++)
	{
	    if ((mem.book[i].hash == board->hash)&&(mem.book[i].m == m))
	    {
		mem.book[i].count++;
		break;
	    }
	    
	    if (mem.book[i].hash == 0)
	    {
		mem.book[i].hash = board->hash;
		mem.book[i].m = m;
		mem.book[i].count = 1;

		index_count++;
		break;
	    }
	}
	domove(m);
    }
    return 1;
}

int bk_cmp(const void *a, const void *b)
{
    const book_table *bt_a = a;
    const book_table *bt_b = b;
    
    return (bt_a -> hash < bt_b -> hash);
}

void loadbook()
{
    int count = 0;
//    int line_index = 0;

    clear_screen();
    pgPrintCenter(10,lightblue,"Clearing Hash Tables");
    pgScreenFlipV();
    
    clear_hash();
    
    move *move_ptr = book_moves;

    for (;;)
    {
	if (*move_ptr == 0) break;
	
	process_opening(move_ptr);

	// find the 0 at the end
	while (*move_ptr)
	    move_ptr++;

	// advance past it...
	move_ptr++;
	
	//draw_board();
	if (count%(BOOK_SIZE/20) == 0)
	{
	    //pgFillvram(black);
	    draw_board();
	    pgFancyPrint(35,3,lightblue,"Loading Book",0,1,1);
	    pgPrintInt(35,4,lightblue,100*count/BOOK_SIZE);
	    pgFancyPrint(37,4,lightblue,"%",0,1,1);
	    pgScreenFlipV();
	}
	    
	count++;
    }

    setupboard();
}


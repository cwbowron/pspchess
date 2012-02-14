#include <pspkernel.h>
#include <pspctrl.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <pspwlan.h>

#include "pg.h"
#include "bce.h"
#include "colors.h"

FILE *g_network_input_file = 0;

#define TERM_HORIZ 80
#define TERM_VERT 25

#define MATCH_LOGIN	"login:"
#define MATCH_PASSWORD	"password:"
#define FICS_PROMPT	"fics%"

char g_terminal_buffer[TERM_VERT][TERM_HORIZ+1];
int g_terminal_cursor = 0;

char g_server_white[MAX_STRING] = "";
char g_server_black[MAX_STRING] = "";

int init_network()
{
    memset(g_terminal_buffer, 0, sizeof(g_terminal_buffer));
    g_terminal_cursor = 0;

    while (sceWlanGetSwitchState() == 0)
    {
	int r = user_prompt("Turn on WLAN");

	if (!r) 
	{
	    return 0;
	}
    }
    
    g_network_input_file = fopen("freechess.txt","r");

    if (g_network_input_file == 0)
    {
	user_message_wait_key("opening network input file failed");
	return 0;
    }

    return 1;
}

// return 1 if there is input data to be processed
int is_waiting()
{
    return (!feof(g_network_input_file));
}

// read a line of characters from the network, at most max characters
int net_read_line(char *buffer, int max)
{
    if (fgets(buffer,max,g_network_input_file))
	return 1;
    return 0;
}

// send a line to the network, add new line if necessary
int net_send_line(char *buffer)
{
    user_message_wait_key(buffer);
    return 0;
}

int read_network_data(char *buffer, int max)
{
    return fread(buffer, 1, max, g_network_input_file);
}

int style12_piece_lookup(char ch)
{
    switch (ch)
    {
	case 'P': return WPAWN;
	case 'p': return BPAWN;
	case 'N': return WKNIGHT;
	case 'n': return BKNIGHT;
	case 'B': return WBISHOP;
	case 'b': return BBISHOP;
	case 'R': return WROOK;
	case 'r': return BROOK;
	case 'Q': return WQUEEN;
	case 'q': return BQUEEN;
	case 'K': return WKING;
	case 'k': return BKING;
	default: return empty;
    }
}

void parse_style12(char *buffer)
{
    char *separators = " ";
    char local[MAX_STRING];
    strcpy(local,buffer);

    int file, rank;
    int double_push_file;
    
    // <12>
    char *ptr = strtok(local, separators);

    for (rank=7;rank>=0;rank--)
    {
	ptr = strtok(NULL, separators);
	
	for (file=0;file<8;file++)
	{
	    setpiece__(SQ(file,rank), style12_piece_lookup(ptr[file]));
	}
    }

    // TO MOVE
    ptr = strtok(NULL, separators);
    if (strcmp(ptr,"W") == 0)
    {
	if (tomove() == BLACK)
	    switch_sides();
	g_whose_turn = WHITE;
    }
    else
    {
	if (tomove() == WHITE)
	    switch_sides();
	g_whose_turn = BLACK;
    }
    
    // double push
    ptr = strtok(NULL, separators);
    double_push_file = atoi(ptr);
    if (double_push_file>=0)
    {
	 board->flags |= dbl;
    }

    // white short side castle
    ptr = strtok(NULL, separators);
    if (strcmp(ptr,"0") == 0)
	board->flags &= ~wkc;
    else
	board->flags |= wkc;
    
    // white long side castle
    ptr = strtok(NULL, separators);
    if (strcmp(ptr,"0") == 0)
	board->flags &= ~wqc;
    else
	board->flags |= wqc;

    // black short side castle
    ptr = strtok(NULL, separators);
    if (strcmp(ptr,"0") == 0)
	board->flags &= ~bkc;
    else
	board->flags |= bkc;

    // black long side castle
    ptr = strtok(NULL, separators);
    if (strcmp(ptr,"0") == 0)
	board->flags &= ~bqc;
    else
	board->flags |= bqc;
    
    // moves since irreversible
    ptr = strtok(NULL, separators);
    // game number
    ptr = strtok(NULL, separators);
    // white's name
    ptr = strtok(NULL, separators);
    strcpy(g_server_white, ptr);
    
    // black's name
    ptr = strtok(NULL, separators);
    strcpy(g_server_black, ptr);

    // relation to game
    ptr = strtok(NULL, separators);
    // initial time
    ptr = strtok(NULL, separators);
    // time increment
    ptr = strtok(NULL, separators);
    // white material strength
    ptr = strtok(NULL, separators);
    // black material strength
    ptr = strtok(NULL, separators);

    // white remaining time (seconds)
    ptr = strtok(NULL, separators);
    chessclock[WHITE] = atoi(ptr)*1000;
	
    // black remaining time (seconds)
    ptr = strtok(NULL, separators);
    chessclock[BLACK] = atoi(ptr)*1000;

    // move number (to be made)
    ptr = strtok(NULL, separators);
    // last move coordinate
    ptr = strtok(NULL, separators);
    if (strcmp(ptr,"none")==0)
    {
	ply = 0;
    }
    else
    {
	move m;

	ply = 1;

	if (strcmp(ptr,"O-O")==0)
	{
	    if (tomove() == WHITE)
		m = MV_CA(E8,G8);
	    else
		m = MV_CA(E1,G8);
	}
	else if (strcmp(ptr,"O-O-O") == 0)
	{
	    if (tomove() == WHITE)
		m = MV_CA(E8,C8);
	    else
		m = MV_CA(E1,C8);
	}
	else 
	{
	    int sf = ptr[2] - 'a';
	    int sr = ptr[3] - '1';
	    int ef = ptr[5] - 'a';
	    int er = ptr[6] - '1';
	    m = MV(SQ(sf,sr),SQ(ef,er));
	}

	gamestack[ply-1] = m;
	g_last_move = m;

/* 	pgFillvram(0); */
/* 	print_move(0,0,m); */
/* 	pgPrint(0,2,cyan,ptr); */
/* 	pgScreenFlipV(); */
/* 	while (0==Read_Key()); */
    }

    // last move time
    ptr = strtok(NULL, separators);
    // last move san
    ptr = strtok(NULL, separators);
    if (ply)
    {
	strcpy(g_san_moves[ply-1],ptr);
    }
    
    // flip field 
    ptr = strtok(NULL, separators);
}


int process_network_string(char *buffer)
{
    int ret_val = 0;
    
    if (strstr(buffer,"<12>"))
    {
	parse_style12(buffer);
	countmaterial();
	//draw_board();
	//pgScreenFlipV();
	//while (0 == Read_Key());

	ret_val = 1;
    }
    else if (strstr(buffer,MATCH_LOGIN) &&
	     (strcmp(g_server_username,"") != 0))
    {
	net_send_line(g_server_username);
    }
    else if (strstr(buffer,MATCH_PASSWORD) &&
	     (strcmp(g_server_password,"") != 0))
    {
	net_send_line(g_server_password);
    }

    static int init = 0;

    if (!init && strstr(buffer,FICS_PROMPT))
    {
	// at some point
	// send "set interface pspChess"
	// send "set style 12"
	init = 1;
    }
    
    if (g_terminal_cursor < TERM_VERT)
    {
	strncpy(g_terminal_buffer[g_terminal_cursor],buffer,TERM_HORIZ);
	g_terminal_buffer[g_terminal_cursor][TERM_HORIZ] = 0;
	g_terminal_cursor++;
    }
    else
    {
	int i;
	for (i=1;i<TERM_VERT;i++)
	{
	    strcpy(g_terminal_buffer[i-1],
		   g_terminal_buffer[i]);
	}
	strncpy(g_terminal_buffer[TERM_VERT-1],buffer,TERM_HORIZ);
	g_terminal_buffer[TERM_VERT-1][TERM_HORIZ] = 0;
    }
    
    return ret_val;
}

struct {
    char *match_string;
    unsigned short color;
} g_terminal_string_colors[] =
{
    {"<12>", yellow},
    {"tells you", red},
    {"", cyan}	
};

void print_terminal_string(int x, int y, char *str)
{
    int i;
    int max_i =
	sizeof(g_terminal_string_colors)/sizeof(g_terminal_string_colors[0]);
    
    for (i=0;i<max_i;i++)
    {
	if (strstr(str,g_terminal_string_colors[i].match_string))
	{
	    msxPutString(x,y,g_terminal_string_colors[i].color,str);
	    break;
	}
    }
}

#define TERMINAL_STRING_COUNT 5

char *g_network_terminal_strings[TERMINAL_STRING_COUNT] = {
    "\n\r",
    "\n",
    MATCH_LOGIN,
    MATCH_PASSWORD,
    FICS_PROMPT,
};

// find the first occurance of any string in char *terms[] and
// return the pointer to the character AFTER its occurance.
// return NULL if no occurance is found.
char *find_split_point(char *buffer, int max_buffer, char **terms, int n_terms)
{
    int i, j;

    for (i = 0; i<max_buffer; i++)
    {
	char *tmp_ptr = buffer + i;

	for (j = 0; j<TERMINAL_STRING_COUNT;j++)
	{
	    char *term = g_network_terminal_strings[j];
	    int len = strlen(term);

	    if (len + i < max_buffer)
	    {
		if (memcmp(tmp_ptr,term,len) == 0)
		{
		    return tmp_ptr+len;
		}
	    }
	}
    }

    return NULL;
}

// I am not a network guy.
// This is probably horribly ineffecient. 

// return 0 if nothing to process
// return 1 if normal processing
// return -1 if we should go the game screen after processing
int process_some_network_data_if_necessary()
{
    static char big_buffer[4096];
    static char line_buffer[256];
    static int big_buffer_offset = 0;
    
    int ret_val = 0;

    if (is_waiting())
    {
	int n;
	ret_val = 1;

	//net_read_line(line_buffer,256);
	n = read_network_data(line_buffer,256);
	
	if (n)
	{
	    // put new data in the big buffer
	    memcpy(big_buffer+big_buffer_offset,line_buffer,n);
	    big_buffer_offset += n;

	    /*
	      while big_buffer contains line terminals
	      split off one line
	      process it
	      shift big buffer
	    */
	    while (1)
	    {
		char *split_point =
		    find_split_point(big_buffer, big_buffer_offset,
				     g_network_terminal_strings,
				     TERMINAL_STRING_COUNT);
				
		if (split_point == NULL)
		{
		    break;
		}
		else
		{
		    // big_buffer = a*splitmeb*
		    // set tmp = a*splitme\0
		    // set big buffer to b*\0
		    int bytes_to_remove = split_point - big_buffer;
		    
		    memcpy(line_buffer, big_buffer, bytes_to_remove);
		    line_buffer[bytes_to_remove] = 0;

		    big_buffer_offset -= bytes_to_remove;					
		    memmove(big_buffer, split_point, big_buffer_offset);
		    big_buffer[big_buffer_offset] = 0;

		    //printf("%s\n", tmp);
		    if (process_network_string(line_buffer))
			ret_val = -1;
		}
	    }
	}
    }

    return ret_val;
}


void display_terminal_screen()
{
    int y;

    for (y=0;y<g_terminal_cursor;y++)
    {
	print_terminal_string(0,y,g_terminal_buffer[y]);
    }
}

// in game mode, the board takes up the left side of the screen
// but there is no osk so we can use a couple more lines vertically 
void display_terminal_screen_board_mode()
{
    int y;

    for (y=3;y<g_terminal_cursor;y++)
    {
	print_terminal_string(34,y+4,g_terminal_buffer[y]);
    }
}

void terminal_mode()
{
    int key;
    int osk_r;
    
    do
    {
	int breakout = process_some_network_data_if_necessary();

	pgFillvram(0);
	display_terminal_screen();
	draw_osk();
	pgScreenFlipV();

	if (breakout<0)
	    return;
       
	key = Read_Key();
       	osk_r = osk_process_key(key);
	
	if (osk_r>0)
	{
	    net_send_line(g_osk_buffer);
	    memset(g_osk_buffer,0,sizeof(g_osk_buffer));
	}
	
    } while (osk_r>=0);
}

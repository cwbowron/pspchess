#include <pspkernel.h>
#include <pspctrl.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <pspiofilemgr_fcntl.h>
#include <ctype.h>

#include "pg.h"
#include "bce.h"
#include "colors.h"

#define MAX_GAMES 512
#define MAX_LINE 256

FILE *g_imported_file = 0;
long g_import_position = -1;

void strip_quotes(char *str)
{
    if (str[0] == '"')
    {
	char bf[MAX_LINE];
	char *ptr = str;
	
	strcpy(bf,str+1);
	strcpy(str,bf);

	while (*ptr)
	{
	    if (*ptr == '\"') {
		*ptr = 0;
		break;
	    }
	    ptr++;
	}
    }
}

// return -1 on nothing to read
int read_next_header(FILE *f, char *buffer, int buffer_max)
{
    char seps[] = "[] ";
	
    char bf[MAX_LINE];

    char event[MAX_LINE];
    char site[MAX_LINE];
    char date[MAX_LINE];
    char round[MAX_LINE];
    char white_str[MAX_LINE];
    char black_str[MAX_LINE];
    char result[MAX_LINE];

    memset(event,0,sizeof(event));
    memset(site,0,sizeof(site));
    memset(date,0,sizeof(date));
    memset(round,0,sizeof(round));
    memset(white_str,0,sizeof(white_str));
    memset(black_str,0,sizeof(black_str));
    memset(result,0,sizeof(result));

    long header_pos = ftell(f);

    int linecount = 0;
    
    while (1)
    {
	long fpos = ftell(f);

	char *r = fgets(bf,MAX_LINE,f);
	    
	if (r == 0)
	    break;

	// if we've already read some header info then we've reached
	// the end... otherwise we probably need to skip over some
	// game info...
	if (bf[0] != '[')
	{
	    if (linecount>0)
	    {
		fseek(f,fpos,SEEK_SET);
		break;
	    }
	    else
	    {
		header_pos = ftell(f);
	    }
	}
	else
	{
	    linecount++;
	
	    char *tag = strtok(bf,seps);
	    char *val = strtok(NULL, seps);

	    if (stricmp(tag,"round") == 0)
		strcpy(round,val);
	    if (stricmp(tag,"event") == 0)
		strcpy(event,val);
	    if (stricmp(tag,"site") == 0)
		strcpy(site,val);
	    if (stricmp(tag,"date") == 0)
		strcpy(date,val);
	    if (stricmp(tag,"white") == 0)
		strcpy(white_str,val);
	    if (stricmp(tag,"black") == 0)
		strcpy(black_str,val);
	    if (stricmp(tag,"result") == 0)
		strcpy(result,val);
	}
    }

    strip_quotes(round);
    strip_quotes(white_str);
    strip_quotes(black_str);
    strip_quotes(result);
    strip_quotes(event);
    strip_quotes(date);
    strip_quotes(site);
    
    snprintf(buffer,buffer_max,"%s-%s-%s-%s-%s-%s",
	     white_str,black_str,round,event,site,date);

    //return linecount;
    if (linecount == 0)
	return -1;
    return header_pos;
}


int lookup_piece(char ch)
{
    switch (ch)
    {
	case 'K': case 'k':
	    return king;
	case 'Q': case 'q':
	    return queen;
	case 'R': case 'r':
	    return rook;
	case 'B': case 'b':
	    return bishop;
	case 'N': case 'n':
	    return knight;
	case 'P': case 'p':
	    return pawn;
	default:
	    return empty;
    }
}


move decode_san(char *s)
{
    char san[10];

    // local copy so we can mess with it
    strcpy(san,s);

    int promopiece = 0;

    char *eq = strstr(san,"=");
    if (eq)
    {
	*eq = 0;
	eq++;
	promopiece = lookup_piece(*eq);
    }

    char* check = strstr(san,"+");
    if (check)
	*check = 0;

    check = strstr(san,"#");
    if (check)
	*check = 0;

    // at this point we've removed promotions and checks
    if (strcmp(san,"O-O") == 0)
    {
	if (tomove()==WHITE)
	{
	    return MV_CA(g_castling_info[WHITE][1].king_start,
			 g_castling_info[WHITE][1].king_end);
	}
	else
	{
	    return MV_CA(g_castling_info[BLACK][1].king_start,
			 g_castling_info[BLACK][1].king_end);
	}
    }
    if (strcmp(san,"O-O-O") == 0)
    {
	if (tomove() == WHITE)
	{
	    return MV_CA(g_castling_info[WHITE][0].king_start,
			 g_castling_info[WHITE][0].king_end);
	}
	else
	{
	    return MV_CA(g_castling_info[BLACK][0].king_start,
			 g_castling_info[BLACK][0].king_end);
	}
    }
    else
    {
	// remove capture x
	char *cap = strstr(san,"x");
	if (cap)
	{
	    char tmp[10];
	    strcpy(tmp,cap+1);
	    strcpy(cap,tmp);
	}
	    
	int len = strlen(san);
	int file_end = san[len-2] - 'a';
	int rank_end = san[len-1] - '1';
	int file_start = -1;
	int rank_start = -1;
	int piece = -1;
	
	switch (len)
	{
	    case 2:	// e4
		piece = pawn;
		break;
	    case 4:	// e2e4 || Ncf3 || N1f3
		if (isalpha(san[1]))
		{
		    file_start = san[1] - 'a';
		}
		else
		{
		    rank_start = san[1] - '1';
		}
		// FALL THROUGH
	    case 3:	// Nf3 || ed3
		if (isupper(san[0]))
		{
		    piece = lookup_piece(san[0]);
		}
		else
		{
		    file_start = san[0] - 'a';

		    if (len == 3)
			piece = pawn;
		}
		break;
	    case 5:	// Nc1f3
		piece = lookup_piece(san[0]);
		file_start = san[1] - 'a';
		rank_start = san[2] - '1';
		break;
	}

	//char bf[256];
	//sprintf(bf,"%s = %c xx%c%c", s, PC(piece), FL(file_end), RK(rank_end));
	//user_message_wait_key(bf);
	
	move_and_score *restore = move_sp;
	genmoves();
	move san_move = -1;

	int SQ_to = SQ(file_end, rank_end);
	
	while (move_sp>restore)
	{
	    move m = popmove();

	    if (TO(m) == SQ_to)
	    {
		int m_piece = VALUE(getpiece__(FR(m)));

		if ((piece == -1) || (m_piece == piece))
		{
		    if (validmove(m) == 1)
		    {
			int m_start = FR(m);
			
			if ((file_start == -1) ||
			    (file_start == F(m_start)))
			{
			    if ((rank_start == -1) ||
				(rank_start == R(m_start)))
			    {
				if (!is_promo(m) ||
				    (PR(m) == promopiece))
				{
				    san_move = m;
				    move_sp = restore;
				}
			    }
			}
		    }
		}
	    }
	}

	return san_move;
    }
}

int import_game_from_string(char *str)
{
    char *seps = " \n\r\t";
    
    char *tok = strtok(str,seps);

    g_default_variant = VARIANT_NORMAL;

    reset_game(0);
    
    do
    {
	// moves always start with alphabet character
	if (isalpha(tok[0]))
	{
	    move m = decode_san(tok);

	    if (m == -1)
	    {
		char bf[256];
		sprintf(bf,"confusion on %s", tok);
		user_message_wait_key(bf);
		return -1;
	    }
	    else
	    {
		local_domove(m,1);
		draw_board();
		pgScreenFlipV();
		//while (0==Read_Key());
	    }
	}

	tok = strtok(NULL,seps);

    } while (tok);

    return 0;
}

int import_game(FILE *f, int n)
{
    char game_buffer[8192] = "";
    char buffer[256];
    int i;

    g_game_mode = GAMEMODE_ANALYSIS;
    
    fseek(f,0,SEEK_SET);

    for (i=0;i<=n;i++)
    {
	g_import_position = read_next_header(f,buffer,256);
    }

    //user_message_wait_key(buffer);
    
    for (i=0;;i++)
    {
	char *r = fgets(buffer,sizeof(buffer),f);

	if ((!r) ||(buffer[0] == '[')) break;

	//user_message_wait_key(buffer);

	strcat(game_buffer, buffer);
    }

    import_game_from_string(game_buffer);
    return 0;
}


// read all the game names into a buffer and return the number of
// games found... 
int read_pgn_games(FILE *f, char **buffer,int max_buffer)
{
    fseek(f,0,SEEK_SET);

    int game_count = 0;

    char header_string[256];
    
    while (read_next_header(f,header_string,256) >= 0)
    {
	buffer[game_count] = malloc(strlen(header_string)+1);
	strcpy(buffer[game_count],header_string);
	game_count++;
    }

    return game_count;
}

int import_pgn(char *file)
{
    int i;
    char *game_names[MAX_GAMES];

    memset(game_names,0,sizeof(game_names));


    if (g_imported_file)
	fclose(g_imported_file);
    
    FILE *f = fopen(file, "r");

    g_imported_file = f;
    
    if (f == NULL)
    {
	user_message_wait_key("error opening pgn file");
	return -1;
    }
    
    int n_games = read_pgn_games(f,game_names, MAX_GAMES);

    if (n_games == 0)
    {
	user_message_wait_key("no games found");
	return -1;
    }
    else if (n_games == 1)
    {
	import_game(f,0);
    }
    else    
    {
	int game_index = user_select_string(game_names,n_games);
	if (game_index>=0)
	{
	    import_game(f,game_index);
	}
    }

    for (i=0;i<n_games;i++)
    {
	free(game_names[i]);
    }
    
    return 0;
}


void pgn_show_text()
{
    char bf[256];
    
    if (g_imported_file == 0)
	return;

    fseek(g_imported_file,g_import_position,SEEK_SET);

    clear_screen();

    int y = 0;
    int x = 0;
    
    while (1)
    {
	char *fr = fgets(bf,256,g_imported_file);
	if (!fr) break;

	int color = cyan;
	
	if (bf[0] == '[')
	    color = lawngreen;
	
	msxPutString(x,y,color,bf);
	y++;

	if (y>30)
	    break;
    }

    pgPrintCenter(32,red,"SQUARE+LTRIGGER to return to game");

    pgScreenFlipV();

    while (1)
    {
	int k = Read_Key();

	if (test_keys(k,PSP_CTRL_LTRIGGER|PSP_CTRL_SQUARE))
	    break;
    }
}

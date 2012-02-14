#include <pspkernel.h>
#include <pspctrl.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <pspiofilemgr_fcntl.h>

#include "pg.h"
#include "bce.h"
#include "colors.h"
#include "piece_bitmaps.h"

//#define FILE_BASE_DIR		"ms0:/PSP/GAME/PSPCHESS"
#define FILE_BASE_DIR		""
#define MUSIC_BASE_DIR		"ms0:/PSP/MUSIC"
#define FILE_SAVE_NAME		"ms0:/PSP/GAME/CHESS.SAV"
//#define FILE_EXPORT_NAME	"ms0:/PSP/GAME/PSPCHESS/EXPORT.PGN"
#define FILE_EXPORT_NAME	"EXPORT.PGN"
//#define FILE_SAVED_OPTIONS	"ms0:/PSP/GAME/PSPCHESS/PSPCHESS.INI"
#define FILE_SAVED_OPTIONS	"PSPCHESS.INI"
//#define SLOT_NAME_START		"ms0:/PSP/GAME/PSPCHESS/GAME"
#define SLOT_NAME_START		"GAME"
#define SLOT_NAME_END		".CHS"

enum {OPTION_BOOL, OPTION_INT, OPTION_FN,
      OPTION_LIST, OPTION_FILE, OPTION_STRING};
    
#define MAX_STORAGE_SLOT 10

#define MAGIC "CHS0"
#define MAGIC_LEN 4

int g_game_mode = GAMEMODE_NORMAL;

//int g_analysis_mode = 0;
move g_analysis_stack[MAXMOVES];

char *g_list_move_types [] ={ "OFF", "COORDINATE", "SAN", 0};

char *g_variant_names[] = {"NORMAL", "FISCHER RANDOM", "WILD 5", "WILD 8",
			   "WILD 8a", 0};

char *g_music_options[] = {"OFF","FILE","RANDOM", 0};
char *g_game_mode_names[] = {"NORMAL","ANALYSIS","CLIENT",0};

char g_server_password[MAX_STRING] = "password";
char g_server_username[MAX_STRING] = "guest";

char g_server_quickstring[QUICK_COUNT][MAX_STRING] = {
    "match",			
    "accept",			
    "who",				
    "who ra",				
    "seek",		     	
    "sought",	
    "news",				
    "messages u",				
    "clearmessages",				
    "say hi.  i am using pspChess so its hard to type",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "say",
    "whisper",
    "kibitz",
};
	
// these functions return nonzero if we should bust back out to the main
// play loop after they are called...
int do_load_game();
int do_save_game();
int do_export();
int do_save_options();
int do_load_options();
int do_import();
int do_game_options();
int do_sound_options();
int do_display_options();
int do_server_options();
int do_server_connect();

struct option_struct {
    char *text;
    void *var;
    int type;
    void *options;
};

int generic_options_menu(struct option_struct *options, int max);

struct option_struct g_options[] =
{
    {"Game Options", do_game_options, OPTION_FN},
    {"Sound Options", do_sound_options, OPTION_FN},
    {"Display Options", do_display_options, OPTION_FN},
    {"Server Options", do_server_options, OPTION_FN},
    {"Pondering", &ponder_mode, OPTION_BOOL},
    {"Extra Keys", &g_process_extra_keys, OPTION_BOOL},
    {"Game Mode", &g_game_mode, OPTION_LIST, g_game_mode_names},
    {"Search Depth", &searchdepth, OPTION_INT},
    {"Minimum Delay", &g_minimum_computer_delay, OPTION_INT},
    {"Reset Game", reset_game_confirm, OPTION_FN},
    {"Load Game", do_load_game, OPTION_FN},
    {"Save Game", do_save_game, OPTION_FN},
    {"Export PGN", do_export, OPTION_FN},
    {"Import PGN", do_import, OPTION_FN},
    {"Save Options", do_save_options, OPTION_FN},
    {"Load Options", do_load_options, OPTION_FN},
};

char *g_music_extensions[] = {"mp3","mod","stm","s3m","mtm","669",
			      "ult","med","xm","it",0};

char *g_pieces_extensions[] = {"png",0};


struct option_struct g_gameplay_options[] =
{
    {"WHITE", &computer[WHITE], OPTION_LIST, g_personality_names},
    {"BLACK", &computer[BLACK], OPTION_LIST, g_personality_names},    
    {"Variant", &g_default_variant, OPTION_LIST, g_variant_names},    
    {"Clock (Min)", &g_default_clock, OPTION_INT},
    {"Inc (Sec)", &g_default_increment, OPTION_INT},
};

struct option_struct g_sound_options[] =
{
    {"Backgrnd Music", &g_music_on, OPTION_LIST, g_music_options}, 
    {"Music File", g_background_music_file, OPTION_FILE, g_music_extensions},
};

struct option_struct g_display_options[] =
{
    {"Show Thinking", &g_show_thinking, OPTION_BOOL},
    {"Show Captures", &g_show_captures, OPTION_BOOL},
    {"Show Moves", &g_list_moves, OPTION_LIST, g_list_move_types},
    {"Show Options", &g_show_options, OPTION_BOOL},
    {"Hilite Checks", &g_show_checks, OPTION_BOOL},
    {"Show Available", &g_show_available_moves, OPTION_BOOL},
    {"Show Threats", &g_show_threats, OPTION_BOOL},
    {"Debug Info", &g_show_debug_info, OPTION_BOOL},
    {"Board Flipping", &g_board_flipping, OPTION_BOOL},
    {"External Pieces", &g_external_pieces, OPTION_BOOL},
    {"Piece File", &g_external_pieces_file, OPTION_FILE, g_pieces_extensions},
    {"Backgrnd Image", &g_background_image, OPTION_BOOL},
};

struct option_struct g_server_options[] =
{
//    {"connect", do_server_connect, OPTION_FN},
    {"username", g_server_username, OPTION_STRING}, 
    {"password", g_server_password, OPTION_STRING},
    {"quick 1", g_server_quickstring[0], OPTION_STRING},
    {"quick 2", g_server_quickstring[1], OPTION_STRING},
    {"quick 3", g_server_quickstring[2], OPTION_STRING},
    {"quick 4", g_server_quickstring[3], OPTION_STRING},
    {"quick 5", g_server_quickstring[4], OPTION_STRING},
    {"quick 6", g_server_quickstring[5], OPTION_STRING},
    {"quick 7", g_server_quickstring[6], OPTION_STRING},
    {"quick 8", g_server_quickstring[7], OPTION_STRING},
    {"quick 9", g_server_quickstring[8], OPTION_STRING},
    {"quick 10", g_server_quickstring[9], OPTION_STRING},
    {"quick 11", g_server_quickstring[10], OPTION_STRING},
    {"quick 12", g_server_quickstring[11], OPTION_STRING},
    {"quick 13", g_server_quickstring[12], OPTION_STRING},
    {"quick 14", g_server_quickstring[13], OPTION_STRING},
    {"quick 15", g_server_quickstring[14], OPTION_STRING},
    {"quick 16", g_server_quickstring[15], OPTION_STRING},
    {"quick 17", g_server_quickstring[16], OPTION_STRING},
    {"quick 18", g_server_quickstring[17], OPTION_STRING},
    {"quick 19", g_server_quickstring[18], OPTION_STRING},
    {"quick 20", g_server_quickstring[19], OPTION_STRING},
};

struct option_list {
    struct option_struct *options;
    int count;
};

struct option_list g_options_list[] =
{
    { g_options, sizeof(g_options)/sizeof(g_options[0]) },
    { g_gameplay_options,
      sizeof(g_gameplay_options)/sizeof(g_gameplay_options[0]) },
    { g_sound_options,
      sizeof(g_sound_options)/sizeof(g_sound_options[0]) },
    { g_display_options,
      sizeof(g_display_options)/sizeof(g_display_options[0]) },
};


void user_message_wait_key(char *message)
{
    clear_screen();
    pgPrintCenter(12, red, message);
    pgPrintCenter(14, red, "Press any key to continue");
    pgScreenFlipV();
    
    while (Read_Key() == 0) ;
}

int do_import()
{
    char pgn_file[256];
    char *extensions[] = { "pgn", 0 };
    int n_ext = 1;
    
    if (user_select_file(FILE_BASE_DIR,extensions,n_ext,pgn_file))
    {
	import_pgn(pgn_file);
    }
    return 0;
}

// return 0 on failure
// return 1 on success
int load_options()
{
    int i;

    FILE *f = fopen(FILE_SAVED_OPTIONS,"r");

    if (f)
    {
	int option_index;
	int max_option_index=sizeof(g_options_list)/sizeof(g_options_list[0]);
	
	char buffer[256];

	clear_screen();
	
	while (!feof(f))
	{
	    memset(buffer,0,256);
	    fgets(buffer, 256, f);

	    char *bla = buffer;

	    while (*bla)
	    {
		if (*bla == '\n')
		{
		    *bla = 0;
		    break;
		}
		bla++;
	    }
	    
	    for (option_index=0;option_index<max_option_index;option_index++)
	    {
		struct option_struct *options =
		    g_options_list[option_index].options;

		int max_i = g_options_list[option_index].count;
		
		for (i=0;i<max_i;i++)
		{
		    if (strstr(buffer,options[i].text))
		    {
			// foo=bar -> point to bar
			char *value_ptr = buffer+strlen(options[i].text)+1;
			
			switch (options[i].type)
			{
			    case OPTION_STRING:
				// fall through
			    case OPTION_FILE:
				strcpy((char*)options[i].var,value_ptr);
				break;
			    case OPTION_LIST:
			    case OPTION_BOOL:
			    case OPTION_INT:
			    {
				int value = atoi(value_ptr);
				*(int*)(options[i].var) = value;
				break;
			    }
			}
			break;	/* end for loop over variables */
		    }
		}
	    }
	}
	
	fclose(f);

	return 1;
    }

    return 0;
}

int do_load_options()
{
    if (load_options())
    {
	user_message_wait_key("options loaded");
    }
    else
    {
	flash_display("load options failed");
    }
    return 0;
}

int do_save_options()
{
    int i;
    
    FILE *f = fopen(FILE_SAVED_OPTIONS,"w+");

    if (f)
    {
	int option_index;
	int max_option_index =
	    sizeof(g_options_list)/sizeof(g_options_list[0]);
	
	for (option_index = 0; option_index < max_option_index; option_index++)
	{
	    struct option_struct *options =
		g_options_list[option_index].options;
	    
	    int max_i = g_options_list[option_index].count;
	    
	    for (i=0;i<max_i;i++)
	    {
		switch (options[i].type)
		{
		    case OPTION_STRING:
			// fall through
		    case OPTION_FILE:
			fprintf(f,"%s=%s\n",
				options[i].text,
				(char *)options[i].var);
			break;
		    case OPTION_LIST:
		    case OPTION_BOOL:
		    case OPTION_INT:
		    {
			int option_value = *(int*)(options[i].var);
			fprintf(f,"%s=%d\n",options[i].text,option_value);
			break;
		    }
		}
	    }
	}
	
	fclose(f);

	user_message_wait_key("options saved");
    }
    else
    {
	char buffer[80];
	sprintf(buffer,"opening %s failed",FILE_SAVED_OPTIONS);
	flash_display(buffer);
    }

    return 0;
}


// slot 0 is the original save file, slot 1-9 are new name scheme
char *storage_slot_name(int slot)
{
    static char buffer[80];

    if (slot == 0)
    {
	return FILE_SAVE_NAME;
    }
    else
    {
	strcpy(buffer, SLOT_NAME_START);
	strcat(buffer, itoa(slot,10));
	strcat(buffer, SLOT_NAME_END);
	return buffer;
    }
}

int storage_slot_used(int slot)
{
    return file_exists(storage_slot_name(slot));
}

int load_slot(int slot)
{
    FILE *f = fopen(storage_slot_name(slot), "r");
    
    if (f)
    {
	int ply_count;
	int i;

	fread(&ply_count, sizeof(ply_count), 1, f);

	// old save style
	if (ply_count>0)
	{
	    g_default_variant = VARIANT_NORMAL;
	    reset_game(0);
	}
	else
	{
	    char magic[MAGIC_LEN+1];

	    memset(magic,0,sizeof(magic));
	    fread(magic, MAGIC_LEN, 1, f);

	    if (strncmp(magic, MAGIC, MAGIC_LEN) != 0)
		return 0;
	    if (feof(f))
		return 0;

	    fread(board, sizeof(chessboard), 1, f);
	    fread(&ply_count, sizeof(ply_count), 1, f);
	    countmaterial();
	}
	
	move m;
	
	for (i=0;i<ply_count;i++)
	{
	    fread(&m, sizeof(m), 1, f);
	    local_domove(m, 1);
	}

	fclose(f);
	return 1;
    }
    return 0;
}

void preview_slot(int slot)
{
    if (load_slot(slot))
    {
	draw_board();
    }
}

int g_cached_ply;
move g_cached_gamestack[MAXMOVES];

void cache_current_game()
{
    g_cached_ply = ply;
    memcpy(g_cached_gamestack,gamestack,sizeof(gamestack[0])*ply);
}

void restore_cached_game()
{
    int i;
    
    reset_game(0);
    
    for (i=0;i<g_cached_ply;i++)
    {
	local_domove(g_cached_gamestack[i], 1);
    }
}

int is_capture(move m)
{
    return getpiece__(TO(m)) != 0;
}

enum { AMBIG_NONE, AMBIG_FILE, AMBIG_RANK, AMBIG_BOTH };

// returns AMBIG_NONE if no ambiguities
// returns AMBIG_FILE if the file is ambiguous (so use rank)
// returns AMBIG_RANK if the rank is ambiguous (so use file)
// returns AMBIG_BOTH if both rank and file could be ambiguous
int is_ambiguous(move m, int piece)
{
    // keep track of conflicting moves
    move amb[9];
    
    move_and_score *restore_sp = move_sp;

    int count = 0;
    
    genmoves();
    while (move_sp > restore_sp)
    {
	move gm = popmove();

	if ((TO(gm) == TO(m)) && (FR(gm) != FR(m)))
	{
	    if (validmove(gm) == 1)
	    {
		if (getpiece__(FR(gm)) == piece)
		{
		    amb[count] = gm;
		    count++;
		}
	    }
	}
    }
    move_sp = restore_sp;

    if (count == 0)
	return AMBIG_NONE;

    int file_count = 0;
    int rank_count = 0;
    
    int source_file = F(FR(m));
    int source_rank = R(FR(m));

    int i;
    
    for (i=0;i<count;i++)
    {
	int file = F(FR(amb[i]));
	int rank = R(FR(amb[i]));

	if (file == source_file)
	    file_count++;
	if (rank == source_rank)
	    rank_count++;
    }

    if (rank_count>0 && file_count>0)
	return AMBIG_BOTH;
    if (file_count==0)
	return AMBIG_RANK;
    return AMBIG_FILE;
}

char *get_san(move m)
{
    static char san_buffer[SAN_BUFFER_LEN];

    square fr = FR(m);
    square to = TO(m);

    chesspiece p = getpiece__(fr);

    if (is_castle(m))
    {
	int f = F(to);

	if (f<FILEE)
	{
	    return "O-O-O";
	}
	else
	{
	    return "O-O";
	}
    }
    
    int f_fr = F(fr);
    int r_fr = R(fr);
    int f_to = F(to);
    int r_to = R(to);
    
    int piece = chesspiecevalue(p);

    char *ret_val = &san_buffer[SAN_BUFFER_LEN-1];

    *ret_val = 0;
    ret_val--;

    // add +/# to checks/checkmates
    domove(m);
    if (fullincheckp(tomove()))
    {
	if (gameoverp(tomove()) == END_CHECKMATE)
	    *ret_val = '#';
	else
	    *ret_val = '+';
	ret_val --;
    }
    undomove();

    // add =X to promos
    if (is_promo(m))
    {
	*ret_val = PC(PR(m));
	ret_val--;
	*ret_val = '=';
	ret_val--;
    }

    // add target squares
    *ret_val = RK(r_to);
    ret_val--;
    *ret_val = FL(f_to);

    int capture = 0;
    
    // add x to captures
    if (is_capture(m))
    {
	ret_val --;
	*ret_val = 'x';
	capture = 1;
    }

    int amb = is_ambiguous(m, p);
    
    switch (amb)
    {
	case AMBIG_NONE:
	    break;
	case AMBIG_BOTH:
	    ret_val --;
	    *ret_val = RK(r_fr);
	    ret_val --;
	    *ret_val = FL(f_fr);
	    break;
	case AMBIG_RANK:
	    ret_val --;
	    *ret_val = FL(f_fr);
	    break;
	case AMBIG_FILE:
	    ret_val --;
	    *ret_val = RK(r_fr);
	    break;
    }
    
    if (piece == pawn)
    {
	// file required on pawn captures
	if (capture && (amb == AMBIG_NONE))
	{
	    ret_val --;
	    *ret_val = FL(f_fr);
	}
	return ret_val;
    }
    else
    {
	ret_val--;
	*ret_val = PC(piece);
	return ret_val;
    }
}

char *movestring(move m)
{
    static char res[5];
    res[0] = FL(F(FR(m)));
    res[1] = RK(R(FR(m)));
    res[2] = FL(F(TO(m)));
    res[3] = RK(R(TO(m)));
    res[4] = 0;
    return res;
}

char *get_game_status_string(int status, int turn)
{
    switch (status)
    {
	case NON_MATERIAL:
	    return "1/2-1/2";
	case REP_DRAW:
	    return "1/2-1/2";
	case END_CHECKMATE:
	    if (turn == WHITE)
		return "0-1";
	    return "1-0";
	case END_STALEMATE:
	    return "1/2-1/2";
	case IN_PROGRESS:
	    return "*";
	default:
	    return "*";
    }
}

char *get_export_name(struct tm *time_info)
{
    static char export_name_buffer[256];
    
    if (!time_info)
    {
	return FILE_EXPORT_NAME;
    }
    else
    {
	sprintf(export_name_buffer,"%s/%4d_%02d_%02d_%02d_%02d.PGN",
		FILE_BASE_DIR,
		time_info->tm_year+1900,
		time_info->tm_mon+1,
		time_info->tm_mday,
		time_info->tm_hour,
		time_info->tm_min
		);
	return export_name_buffer;
    }
}

int do_export()
{
    int i;

    time_t result;
    result = time(NULL);
    struct tm *time_info = localtime(&result);
	
    char *export_name = get_export_name(time_info);
    
    FILE *f = fopen(export_name,"w+");
    int game_status = gameoverp(tomove());
    char *status_string = get_game_status_string(game_status, tomove());
    
    if (f)
    {
	fprintf(f,"[Event \"pspChess\"]\n");
 	fprintf(f,"[Site \"psp\"]\n");

	if (time_info)
	{
	    fprintf(f,"[Date \"%4d.%02d.%02d\"]\n", time_info->tm_year+1900,
		    time_info->tm_mon+1, time_info->tm_mday);
	}
	else
	{
	    fprintf(f,"[Date \"????.??.??\"]\n");
	}
	
	fprintf(f,"[Round \"1\"]\n");
	fprintf(f,"[White \"%s\"]\n",g_personality_names[computer[WHITE]]);
	fprintf(f,"[Black \"%s\"]\n",g_personality_names[computer[BLACK]]);
	fprintf(f,"[Result \"%s\"]\n", status_string);
	
	cache_current_game();

	reset_game(0);

	clear_screen();
    
	for (i=0;i<g_cached_ply;i++)
	{
	    move m = g_cached_gamestack[i];

	    if (i%2==0)
	    {
		fprintf(f,"\n%d.", i/2 + 1);
	    }
	    
	    fprintf(f," %s", g_san_moves[i]);

	    domove(m);
	}

	fprintf(f," %s\n", status_string);
	
	fclose(f);

	user_message_wait_key("GAME EXPORTED");
    }
    else
    {
	flash_display("opening export file failed");
    }

    return 0;
}

int select_storage_slot()
{
    int i;

    int cursor = 0;
    int ret_val = -1;

    cache_current_game();
    
    while (1)
    {
	int x = 35;
	int y = 6;

	clear_screen();

	if (storage_slot_used(cursor))
	{
	    preview_slot(cursor);
	}
	
	pgFancyPrint(35,30,red,"X TO ACCEPT",1,0,0);
	pgFancyPrint(35,32,red,"TRIANGLE TO CANCEL",1,0,0);
	
	for (i=0;i<MAX_STORAGE_SLOT;i++)
	{
	    y+=2;
	    if (cursor == i)
	    {
		pgFancyPrint(x-2,y,cyan,"->",1,0,1);
		pgFancyPrint(x,y,cyan,"SLOT",1,0,1);
		pgFancyPrint(x+6,y,cyan,itoa(i,10),1,0,1);
	    }
	    else
	    {
		pgFancyPrint(x,y,cyan,"SLOT",0,1,1);
		pgFancyPrint(x+6,y,cyan,itoa(i,10),0,1,1);
	    }

	    if (storage_slot_used(i))
	    {
		pgFancyPrint(x+10,y,cyan,"-USED-",1,0,0);
	    }
	    else
	    {
		pgFancyPrint(x+10,y,cyan,"-EMPTY-",1,1,0);
	    }
	}

	pgScreenFlipV();
	
	int key = get_buttons();

	if (test_keys(key,PSP_CTRL_UP))
	    cursor --;
	if (test_keys(key,PSP_CTRL_DOWN))
	    cursor ++;
	if (test_keys(key,PSP_CTRL_CROSS))
	{
	    ret_val = cursor;
	    break;
	}
	
	if (test_keys(key,PSP_CTRL_TRIANGLE))
	    break;
	
	if (cursor>=MAX_STORAGE_SLOT)
	    cursor = 0;
	if (cursor<0)
	    cursor = MAX_STORAGE_SLOT-1;
    }

    restore_cached_game();
    
    return ret_val;
}

int do_clear_tables()
{
    clear_hash();
    return 0;
}

int do_load_book()
{
    loadbook();
    setupboard();
    ply = 0;
    change_gamemode(OPENING,0);
    return 0;
}

int do_game_options()
{
    return generic_options_menu(
	g_gameplay_options,
	sizeof(g_gameplay_options)/sizeof(g_gameplay_options[0]));
}

int do_sound_options()
{
    return generic_options_menu(
	g_sound_options,
	sizeof(g_sound_options)/sizeof(g_sound_options[0]));
}

int do_display_options()
{
    return generic_options_menu(
	g_display_options,
	sizeof(g_display_options)/sizeof(g_display_options[0]));
}

int do_server_options()
{
    return generic_options_menu(
	g_server_options,
	sizeof(g_server_options)/sizeof(g_server_options[0]));
}

int do_server_connect()
{
    g_game_mode = GAMEMODE_CLIENT;
    computer[BLACK] = 0;
    computer[WHITE] = 0;
    init_network();
    terminal_mode();
    return 1;
}

int do_save_game()
{
    int slot = select_storage_slot();

    if (slot>=0)
    {
	if (storage_slot_used(slot) &&
	    !user_prompt("Do you want to overwrite the slot?"))
	    return 0;
	
	FILE *f = fopen(storage_slot_name(slot),"w+");

	if (f)
	{
	    int bla = 0;
	    char magic[MAGIC_LEN+1] = MAGIC;
	    
	    fwrite(&bla,sizeof(int),1,f);
	    fwrite(magic,MAGIC_LEN,1,f);
	    fwrite(&g_initial_board_position,sizeof(chessboard),1,f);
	    fwrite(&ply, sizeof(ply), 1, f);
	    fwrite(gamestack,sizeof(gamestack[0]), ply, f);
	    fclose(f);
	    
	    user_message_wait_key("GAME SAVED");
	}
	else
	{
	    flash_display("opening file failed");
	}
    }

    return 0;
}

int do_load_game()
{
    int slot = select_storage_slot();

    if (slot>=0)
    {
	if (load_slot(slot))
	{
	    user_message_wait_key("Game Loaded");
	}
	else
	{
	    flash_display("Load Game failed");
	}
    }

    return 0;
}

  
void setup_option_background()
{
    int i;
    int n_pages = 2;
    
    for (i=0;i<n_pages;i++)
    {
	clear_screen();
	pgPrintCenter(1,green,"PSPCHESS BY CHRISTOPHER BOWRON");
	pgPrintCenter(2,green,"CHESS@BOWRON.US");
	pgPrintCenter(3,green,"Graphics by Stadi Thompson");
	pgPrintCenter(4,green,"Distributed under the GNU Public License");
	pgPrintCenter(5,green,"--Share and Enjoy--");

	pgPrintCenter(32,red,"LEFT/RIGHT/X to Change");
	pgPrintCenter(33,red,"TRIANGLE to return To Game");
	pgScreenFlipV();
    }
}

int generic_options_menu(struct option_struct *options, int max)
{
    int i, max_i, key, cursor = 0;

    max_i = max;
    
    setup_option_background();
    
    do
    {
	int y, x2;

	int x = 13;
	
	int start_y = 7;
	
	clear_text_area(x,start_y,60-2*x,max_i+2);
	
	// display options
	for (i=0;i<max_i;i++)
	{
	    y = i+start_y+1;
	    x = 15;
	    x2 = x+18;

	    if (i==cursor)
	    {
		pgFancyPrint(x-2,y,blue,"->", 1,1,0);
		pgFancyPrint(x,y,blue,options[i].text,1,1,0);
	    }
	    else
	    {
		pgFancyPrint(x,y,red,options[i].text,0,1,1);
	    }
	    
	    switch (options[i].type)
	    {
		case OPTION_STRING:
		{
		    char  *value = (char*)(options[i].var);
		    if (strlen(value)>20)
		    {
			char bf[256];
			strcpy(bf,value);
			bf[20] = '.';
			bf[21] = '.';
			bf[22] = '.';
			bf[23] = 0;
			msxPutString(x2-5,y,red,bf);
		    }
		    else
		    {
			msxPutString(x2-5,y,red,value);
		    }
		    
		    break;
		}
		case OPTION_FILE:
		{
		    char  *value = (char*)(options[i].var);
		    if (strlen(value)>20)
		    {
			msxPutString(x2,y,red,strrchr(value,'/')+1);
		    }
		    else
		    {
			msxPutString(x2,y,red,value);
		    }
		}
		break;
		case OPTION_LIST:
		{
		    int option_index = *(int*)(options[i].var);
		    char **option_array = (char**)(options[i].options);
		    pgFancyPrint(x2,y,red,option_array[option_index],1,0,1);
		    break;
		}
 		case OPTION_BOOL:
		    pgFancyPrint(x2,y,red,*(int*)(options[i].var)?
				 "On":"Off",1,0,1);
		    break;
		case OPTION_INT:
		    pgPrintInt(x2,y,red,*(int*)(options[i].var));
		    break;
	    }
	}

	pgScreenFlipV();

	key = Read_Key();

	if (test_keys(key, PSP_CTRL_UP))
	{
	    cursor--;

	    if (cursor<0) cursor = max_i-1;
	}
	else if (test_keys(key, PSP_CTRL_DOWN))
	{
	    cursor ++;

	    if (cursor>=max_i) cursor = 0;
	}
	else if (test_keys(key, PSP_CTRL_LEFT))
	{
	    switch (options[cursor].type)
	    {
		case OPTION_LIST:
		{
		    int *p = options[cursor].var;
		    if (*p) *p = *p-1;
		}
		break;
		case OPTION_BOOL:
		{
		    int *p = options[cursor].var;
		    *p = *p ^= 1;
		}
		break;
		case OPTION_INT:
		{
		    int *p = options[cursor].var;
		    *p = *p - 1;
		}
		break;
	    }
	}
	else if (test_keys(key, PSP_CTRL_RIGHT))
	{
	    switch (options[cursor].type)
	    {
		case OPTION_LIST:
		{
		    int *p = options[cursor].var;
		    int option_index = *p;
		    char **option_array = (char**)(options[cursor].options);

		    if (option_array[option_index+1] != NULL)
			*p = *p + 1;
		}
		break;
		case OPTION_BOOL:
		{
		    int *p = options[cursor].var;
		    *p = *p ^= 1;
		}
		break;
		case OPTION_INT:
		{
		    int *p = options[cursor].var;
		    *p = *p + 1;
		}
		break;
	    }
	}
	else if (test_keys(key, PSP_CTRL_CROSS))
	{
	    switch (options[cursor].type)
	    {
		case OPTION_STRING:
		{
		    char *value = (char*)(options[cursor].var);
		    osk_get_string(value, MAX_STRING);
		    setup_option_background();
		    break;
		}
		
		case OPTION_FILE:
		{
		    char *value = (char*)(options[cursor].var);
		    char **extensions = (char**)(options[cursor].options);
		    
		    if (value == g_background_music_file)
		    {
			user_select_file(MUSIC_BASE_DIR,
					 extensions,MAXPATH,value);
			restart_music_if_necessary();
		    }
		    else if (value == g_external_pieces_file)
		    {
			if (user_select_file(
				FILE_BASE_DIR,extensions,MAXPATH,value))
			{
			    if (g_external_pieces)
			    {
				if (read_png_file(g_external_pieces_file) == 0)
				    process_pieces_file();
			    }
			}
		    }
		    else
		    {
			user_select_file(FILE_BASE_DIR,
					 extensions,MAXPATH,value);
		    }
		    
		    setup_option_background();
		}
		break;
		case OPTION_LIST:
		{
		    int *p = options[cursor].var;
		    int option_index = *p;
		    char **option_array = (char**)(options[cursor].options);

		    if (option_array[option_index+1] != NULL)
			*p = *p + 1;
		    else
			*p = 0;
		}
		break;
		case OPTION_BOOL:
		{
		    int *p = options[cursor].var;
		    *p = *p ^= 1;
		}
		break;
		case OPTION_FN:
		{
		    int (*fn)(void) = options[cursor].var;

		    int ret_val = fn();

		    if (ret_val)
			return ret_val;
		    
		    setup_option_background();
		}
		break;
	    }
	}
    } while (key != PSP_CTRL_TRIANGLE);

    return 0;
}


void option_menu()
{
    generic_options_menu(g_options, sizeof(g_options)/sizeof(g_options[0]));
}

// from weak on ps2dev forums
void screen_dump()
{
    unsigned char bmpHeader24[] = {
	0x42, 0x4d, 0x38, 0xfa, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x36,
	0x00, 0x00, 0x00, 0x28, 0x00, 0x00, 0x00, 0xe0, 0x01, 0x00, 0x00,
	0x10, 0x01, 0x00, 0x00, 0x01, 0x00, 0x18, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x12, 0x0b, 0x00, 0x00, 0x12, 0x0b,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    };
    
    unsigned char  buffer[SCREEN_WIDTH*3];
    unsigned char  r,g,b;
    int bufferIndex = 0;
    unsigned short p;


    static int count = 0;
    char savePath[80];

    sprintf(savePath, "SCREEN%d.bmp", count);

    count++;

    int i, e;
    
    //int file = sceIoOpen(savePath,PSP_O_CREAT|PSP_O_TRUNC|PSP_O_RDWR, 0777);
    FILE *file = fopen(savePath,"w+");
    
// write bmp header
    //sceIoWrite(file,bmpHeader24,54);
    fwrite(bmpHeader24,1,54,file);
   
// write bmp data
    unsigned char *vptr;
    unsigned char *vptr0 = (unsigned char *)pgGetVramAddr(0,271);

    for(i=0; i<272; i++)
    {      
	vptr=vptr0;
	for(e=0; e<480; e++)
	{
	    p = *(unsigned short *)vptr;
	    r = (p<<3)&0xf8;
	    g = (p>>2)&0xf8;
	    b = (p>>7)&0xf8;         
         
	    buffer[bufferIndex] = b;
	    bufferIndex++;         
	    buffer[bufferIndex] = g;
	    bufferIndex++;         
	    buffer[bufferIndex] = r;
	    bufferIndex++;

	    vptr+=PIXELSIZE*2;
	}
	// write chunk
	//sceIoWrite(file,buffer,SCREEN_WIDTH*3);
	fwrite(buffer,1,SCREEN_WIDTH*3,file);
	
	bufferIndex=0;
	vptr0-=LINESIZE*2;
    }

// bmp end
    unsigned char end[] = { 0x00, 0x00 };
    //sceIoWrite(file,end,2);
    fwrite(end,1,2,file);
    
    //sceIoClose(file);
    fclose(file);
}

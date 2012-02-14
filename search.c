/*
  This File is part of Boo's Chess Engine
  Copyright 2001 by Christopher Bowron
*/  
#include <pspkernel.h>
#include <pspctrl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

//#include <sys/timeb.h>
/* #include <setjmp.h> */
//#include <signal.h>
//#include <string.h>

#include "pg.h"
#include "bce.h"

#include "colors.h"

/*  #define NULL_MOVE */

/* #define POLL_FREQ	((1<<13)-1) */
//#define POLL_FREQ	256
#define POLL_FREQ	1024
#define NO_TIMEOUT	INF

#define ITERATIVE_SEARCH	1
#define ASPIRATION_WINDOW	2000

/* #define poll_check() if (((++search_info.nodes)&(POLL_FREQ))==0) poll(); */
#define poll_check() if (((++search_info.nodes)%(POLL_FREQ))==0) poll();

#define stored_inc(d) (d*4)
#define cutoff_inc(d) (d)

/* =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

void sceKernelRotateThreadReadyQueue(int priority);

int gamemode;
int pondering = 0;
int g_current_searchdepth = 0;
int g_start_time = 0;

int history[64*64];		/* history heuristic for move ordering */

int g_show_thinking = 0;

move g_best_line[32];

struct
{
    move m;
    int depth;
} counter[64*64];

search_info_type search_info;
branch_info_type branch_info;

int (*search)(int,int,int,int) = negascout_tables;
//int (*search)(int,int,int,int) = qsearch;
/* int (*search)(int,int,int,int) = negascout_search; */
//int (*search)(int,int,int,int) = horrible_search;
/* =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

long get_ms()
{
    //struct timeb timebuffer;
    //ftime(&timebuffer);
    //return (timebuffer.time * 1000) + timebuffer.millitm;
    return sceKernelLibcTime((void *) 0) * 1000;
}

void print_square(int x, int y, square s)
{
    char str[3] = { FL(F(s)), RK(R(s)), 0 };
    pgFancyPrint(x,y,lightblue, str,0,1,1);
}

void print_move(int x, int y, move m)
{
    msxPutString(x,y,cyan,movestring(m));
}

void print_best_line(int x, int y, move *line, int depth)
{
    int i;
    int localx = x;
    
    pgFancyPrint(x,y,red,"BEST LINE",1,0,0);
    y++;
    for(i=0;i<depth;i++)
    {
	if (line[i] == 0) break;
	
	print_move(localx,y,line[i]);
	localx+=5;

	if (localx>50)
	{
	    y++;
	    localx=x;
	}
    }
}

void display_killers(int x, int y)
{
    int i;
    int j;

    pgFancyPrint(x,y,red,"Killers",1,0,0);

    for (i=0;i<g_current_searchdepth;i++)
    {
	for (j=0;j<2;j++)
	{
	    print_move(x + 5*j, y+1+i, killers[i][j].m);
	}
    }
}

void pgPrintCenter(int y, int color, char *str)
{
    int x = 30 - strlen(str)/2;

    if (color==green)
    {
	pgFancyPrint(x,y,color,str,0,1,0);
    }
    else if (color==red)
    {
	pgFancyPrint(x,y,color,str,1,0,0);
    }
    else
    {
	pgFancyPrint(x,y,color,str,0,1,1);
    }
}

int actualmode()
{
    int i, pieces = 0;

    for (i=0;i<=BKING;i++) pieces += board->pieces[i];
    
    if (pieces<=10)
	return ENDGAME;
    else
	return MIDGAME;
    
}

void change_gamemode(int mode, int clear)
{
    switch(mode)
    {
	case OPENING:
	    gamemode = OPENING;
	    break;
	case MIDGAME:
	    gamemode = MIDGAME;
	    break;
	    
	case ENDGAME:
	    gamemode = ENDGAME;
	    break;
    }

    if (clear)
    {
	clear_hash();
    }
}

/*
  return a quick evaluation of a move for use in move ordering
*/
int quickeval(move m, int p, move hashed)
{
    int result;
    int piece;

    piece = getpiece__(TO(m));
    if (piece)
    {
	int n = weights[chesspiecevalue(piece)];
	int d = weights[chesspiecevalue(getpiece__(FR(m)))];

	if (d==0) d = 100000;	/* king  */
	
	result = 10000 + 3000 * n / d;
    }
    else
	result = 0;

    if (m == counter[lastmove()].m)
	result += 20000;
    else if (m == hashed)
	result += 15000;
    else if (m == killers[p][0].m)
	result += 10000;
    else if (m == killers[p][1].m)
	result += 5000;
    else
	result += history[m];

    return result;
}

/*
  compare two moves for initial move ordering
*/
/* static int movecmp(const void *foo, const void *bar) */
/* { */
/*     return ((move_and_score*)foo)->score-((move_and_score*)bar)->score; */
/* } */

/*
** The h-constants for this version of Shell-sort come from:
** Handbook of Algorithms and Data Structures in Pascal and C
** By Gaston Gonnet, Ricardo Baeza-Yates
** Addison Wesley; ISBN: 0201416077
** These h-increments work better for me than Sedgewick's under
** full optimization.  Your mileage may vary, so I suggest you
** try Sedgewick's suggestions as well.  The h-increment arena
** is a rich environment for exploration.  I suggest attempting
** all possible permutations below 20, since that is where a
** good shell-sort is crucuial.  If you find something wonderul
** you may get your name up in lights.
** -- D. Corbit
*/
int ShellSort(move_and_score start[], move_and_score end[])
{
    int i, j, h;
    size_t n = (end - start);
    move_and_score  key;

    for (h = n; h > 0;)
    {
        for (i = h; i < n; i++)
	{
            j = i;
            key = start[i];
            while ((j >= h) & (key.score < start[j - h].score))
	    {
                start[j] = start[j - h];
                j -= h;
            }
            start[j] = key;
        }
        h = (size_t) ((h > 1) & (h < 5)) ? 1 : 5 * h / 11;
    }

    return 0;
}

void sortmoves(move_and_score *start, move_and_score *end, move hashed)
{
    move_and_score *t;
    
    int p = ply - search_info.startply;

    for (t=start;t<end;t++)
	t->score = quickeval(t->m, p, hashed);
    
    ShellSort(start, end);
}

void clearthoughts()
{
    memset(history, 0, sizeof(history));
    memset(search_info.pv.moves, 0, sizeof(search_info.pv.moves));
    memset(search_info.pv.length, 0, sizeof(search_info.pv.length));
    memset(killers,0,sizeof(killers));
    memset(counter,0,sizeof(counter));

    memset(g_best_line,0,sizeof(g_best_line));
}

/*
void signal_handler(int sig)
{
    search_info.stop = sig;
    signal(SIGINT, signal_handler);
}
*/

/*
  c is the player who is pondering what they are going to do when
  there opponent makes a move.

  this will set up some values in history that are hopefully cutoffs
  for the real search and put stuff in the trasposition tables
*/
void ponder()
{
    int saveddepth;
    int savedtime;
    int j;

    clearthoughts();
    
    if (gamemode == OPENING) return;
    if (ply<=0) return;
    
    //savedhandler = signal(SIGINT, signal_handler);
    search_info.stop = 0;
    
    savedtime = searchtime;
    saveddepth = searchdepth;

    searchdepth = 100;
    searchtime = NO_TIMEOUT;
    pondering = 1;

    //flash_display("ponder");
    
    think();

    //signal(SIGINT, savedhandler);
    searchtime = savedtime;
    searchdepth = saveddepth;

    for (j=1;j<PV_LENGTH;j++)
    {
	search_info.pv.moves[j-1] = search_info.pv.moves[j];
	killers[j-1][0].m = killers[j][0].m;
	killers[j-1][1].m = killers[j][1].m;
	killers[j-1][0].score = killers[j][0].score;
	killers[j-1][1].score = killers[j][1].score;
    }

    pondering = 0;
}


move bookmove()
{
    if (gamemode == OPENING)
    {
	move m = bookopening();
	if (validmove(m)==1)
	{
	    return m;
	}
	else 
	{
	    clear_screen();
	    pgPrintCenter(10,lightblue,"No Book Move Available");
	    pgPrintCenter(12,lightblue,"Switching to Search Mode");
	    pgPrintCenter(16,lightblue,"Clearing hash tables");
	    pgScreenFlipV();
	    change_gamemode(MIDGAME, 1);
	    pgScreenFlipV();
	}
    }
    return dummymove;		/* not in book mode or invalid book move */
}

move think()
{
    long starttime;
    //int duration;
    //int sig;
    int c = tomove();
    //move best_line[PV_LENGTH];
    //int i;
    
    static int best;
    //static int best_depth;
    static int x;
    static int depth_reached;
    static move best_move;
    static int branches;
    //double bf;
    move_and_score *restore_sp;

#ifdef ASPIRATION_WINDOW    
    best = evaluate();
#endif

    /*
    if (actualmode()!=gamemode)
	change_gamemode(actualmode(), 1);
    */

    if (!ponder_mode)
	clearthoughts();
    
    starttime = get_ms();
    search_info.startply = ply;
    restore_sp = move_sp;

    long timeout = 0;
    
    /*
      search for time/30
    */
    if (!pondering)
    {
	int clock_time = chessclock[c];

	if (clock_time<0)
	    clock_time = 500;

	timeout = max(1,clock_time / 30);
	    
	search_info.stoptime = starttime + timeout;
    }

    search_info.nodes = 0;
    search_info.branches = 0;
    tablehits[0] = tablehits[1] = tablehits[2] = 0;
    search_info.pv.hits = 0;
    memset(search_info.move_depth,0,sizeof(search_info.move_depth));
	
    search_info.polling = 0;

#if ITERATIVE_SEARCH 
    g_current_searchdepth = 1;
#else
    g_current_searchdepth = searchdepth;
#endif

    search_info.stop = 0;

    g_start_time = get_ms()/1000;

    while (g_current_searchdepth <= searchdepth && !search_info.stop)
    {
#ifdef ASPIRATION_WINDOW	    
	int alpha = best - ASPIRATION_WINDOW;
	int beta = best + ASPIRATION_WINDOW;
	best = search(alpha, beta, g_current_searchdepth, 1);
	    
	if (best<=alpha)
	{
	    best = search (-INF, beta, g_current_searchdepth, 1);
	}
	else if (best>=beta)
	{
	    best = search (alpha, INF, g_current_searchdepth, 1);
	}
#else
	best = negascout_tables (-INF, INF, g_current_searchdepth,0);
#endif
	
	best_move = g_best_line[0];
	
	branches = search_info.branches;
	    
	g_current_searchdepth++;

	if (best>WIN-50)
	    break;
    }

    depth_reached = x;

    if ((best>WIN-50)||(best<LOSE+50))
	output("mate in %d\n", WIN-abs(best));

    return best_move;
}

move bce()
{
    move temp;
    
    if ((temp=bookmove())!=dummymove)
	return temp;
    else
	return think();
}

int bioskey()
{
    static SceCtrlData control_data;
    sceCtrlPeekBufferPositive(&control_data, 1); 
    return control_data.Buttons;
}

/* check for time up or pondering interrupt... */
void poll()
{
    if (search_info.stop)
    {
    }
    else if (pondering)
    {
	if (bioskey())
	{
	    pgWaitV();
	    //sceKernelRotateThreadReadyQueue(0x11);
	}
    }
    else if (bioskey()&PSP_CTRL_TRIANGLE)
    {
	search_info.stop = 1;
    }
}

void storepv(move m, int p)
{
    search_info.pv.moves[p] = m;
    search_info.pv.length[p] = search_info.pv.length[p+1];
}

void store_retrieved(move m, int p)
{
    search_info.pv.moves[p] = m;
    search_info.pv.moves[p+1] = dummymove;
    search_info.pv.length[p] = p+2;
    search_info.pv.length[p+1] = p+2;
}

int extend_search_p()
{
    if (board->flags&promo_flag)
	return 1;
    return 0;
}

void store_counter(move m, int depth)
{
    if (-depth<=counter[lastmove()].depth)
    {
	counter[lastmove()].m = m;
	counter[lastmove()].depth = depth;
    }
}

#ifdef NULL_MOVE

#define AVOID_NULL_PIECES (7)
#define AVOID_NULL_MAT (10*STATIC_PAWN_VALUE)

int NullOk(int depth, int side_to_move)
{
    int cwp, cbp,
	base_reduction = (depth > 3) ? 0 : 1;
    
    /* If there is a risk of Zugzwang then return base_reduction */
    if (side_to_move == WHITE && board->material[WHITE] < AVOID_NULL_MAT)
        return base_reduction;

    if (side_to_move == BLACK &&  board->material[BLACK] < AVOID_NULL_MAT)
        return base_reduction;

    cwp = board->piececount[WHITE];
    if (side_to_move == WHITE && cwp < AVOID_NULL_PIECES)
        return base_reduction;

    cbp = board->piececount[BLACK];
    if (side_to_move == BLACK && cbp < AVOID_NULL_PIECES)
        return base_reduction;

    return 2 + ((depth) > ((6) + (((cwp < 4 && cbp < 4) ? 2 : 0))) ? 1 : 0);
}
#endif

int qsearch(int a, int b, int d)
{
    move_and_score *restore_sp;
    int incheck = 0;
    int searched = 0;
    int b2;
    int p;
    int c = tomove();
    int current_eval;
    move bestmove = 0;
    int a_original;
    
    p = ply-search_info.startply;
    search_info.pv.length[p]=p;

    if (position_seen_before())
	return 0;
    
    current_eval = evaluate();
     
    if (p>=PV_LENGTH)
	return current_eval;
    else if  (current_eval>=b)
	return current_eval;
    else if (current_eval>a)
	a = current_eval;

    poll_check();
    
    a_original = a;
    b2 = b;

    incheck = incheckp(c);

    restore_sp = move_sp;
    
    genattacks();
    sortmoves(restore_sp, move_sp, 0);
    
    while (move_sp>restore_sp)
    {
	move m;
	int intocheck;
	  
	m = popmove();

	domove(m);

	intocheck = (incheck) ? fullincheckp(c) : intocheckp(m,c);
    
	if (!intocheck)
	{
	    int t = -qsearch(-b2, -a, d+1);
	    
	    searched++;
	    
	    /* re-search */
	    if ((t>a)&&(t<b))
	    {
		if ((searched>1))
		    a = -qsearch(-b,-t, d+1);

		storepv(m, p);
	    }
	    
	    a = max(a,t);
	    if (a >= b)
	    {
		move_sp = restore_sp;
		undomove();

		store_counter(m, p);
		return a;
	    }
	    b2 = a + 1;
	}
	undomove();
    }
  
    move_sp=restore_sp;
  
    if (searched)
    {
	store_counter(bestmove, p);
	return a;		/* had at least one valid move/attack */
    }
    else
    {
	return current_eval;	/* we found no valid attacks */
    }
}

int negascout_tables(int a, int beta, int d, int DoNull)
{
    int searched = 0;
    int incheck;
    move_and_score *restore_sp;
    int b;
    int p;
    int c = tomove();

#ifdef NULL_MOVE
    int nd;
#endif    
    
    int hashscore;
    move bestmove = 0;
    int oldalpha = a;
    int retrieve_type;

    p = ply-search_info.startply;

    /* draw? */
    if (p&&position_seen_before())
	return 0;

    switch (retrieve_type = retrieve_hash(d, &hashscore, &bestmove))
    {
	case EXACT:
	    if ((hashscore>a)&&(hashscore<beta))
		if (validmove(bestmove) == 1)
		{
		    //store_retrieved(bestmove, p);
		    g_best_line[p] = bestmove;
		    return hashscore;
		}
	    break;
	case L_BOUND:
	    if (hashscore>=beta)
		return hashscore;
	    break;
	case U_BOUND:
	    if (hashscore<=a)
		return hashscore;
	    break;
    }
    
    if (d<=0)
	return qsearch(a,beta,0);

    search_info.branches ++;
    poll_check();
    
    search_info.pv.length[p] = p;

    b = beta;

#define CHECKTEST_ALL
#ifdef CHECKTEST_ALL
    incheck = incheckp(c);
#endif
    restore_sp = move_sp;

    genmoves();
    sortmoves(restore_sp, move_sp, bestmove);
    
    while (!search_info.stop && move_sp>restore_sp)
    {
	move m;
	int intocheck;
	
	m = popmove();
	domove(m);

#ifdef CHECKTEST_ALL
	// TODO:
	// try moving into check test so that it tests only
	// prior to storing a position
	intocheck = (incheck) ? fullincheckp(c) : intocheckp(m, c);
	
	if (!intocheck)
#endif
	{
	    int t;
	    searched++;

	    if (d == g_current_searchdepth)
	    {
		if (pondering)
		{
		    char *message = "Pondering";
		    int x = 35;
		    int y = 14;
		    int message_len = strlen(message);
		    
		    pgScreenCopy();

		    // black out text region
		    int aa,bb;
		    for(aa=x*8;aa<(x+message_len+1)*8;aa++)
		    {
			for(bb=y*8;bb<(y+3)*8;bb++)
			{
			    set_pixel(aa,bb,0);
			}
		    }

		    pgFancyPrint(35,y,red,message,0,1,1);
		    print_move(35,y+1,m);
		    pgPrintInt(35,y+2,red,g_current_searchdepth);
		    pgScreenFlipV();
		    
		}
		else if (g_show_thinking || g_hardcore_debug)
		{
		    static int bit = 0;

		    bit = (bit+1) % 2;
		
		    draw_board();
		
		    hilite_move(m,green,green);

		    if (bit)
			pgFancyPrint(35,1,red,"Considering: ",1,0,0);
		    else
			pgFancyPrint(35,1,magenta,"Considering: ",1,0,1);
		
		    print_move(48,1,m);

		    pgPrintInt(35,2,lightgrey,a);
		    pgPrintInt(45,2,lightblue,beta);
		
		    if (bestmove)
		    {
			hilite_move(bestmove,cyan,blue);
			print_best_line(35,4,
					//search_info.pv.moves,
					g_best_line,
					//search_info.pv.length[p]
					100
			    );
		    }

		    int py = 19;
		    pgFancyPrint(35,py,red,"Table Hits",1,0,0);
		    pgFancyPrint(35,++py,lightblue,">",0,1,1);
		    pgPrintInt(37,py,lawngreen,tablehits[0]);
		    pgFancyPrint(35,++py,lightblue,"=",0,1,1);
		    pgPrintInt(37,py,lawngreen,tablehits[1]);
		    pgFancyPrint(35,++py,lightblue,"<",0,1,1);
		    pgPrintInt(37,py,lawngreen,tablehits[2]);

		    py+=2;
		    int current_time = get_ms()/1000;
		    pgFancyPrint(35,py,lightblue,"Elapsed Time",1,0,0);
		    int elapsed = current_time-g_start_time;
		    display_time(35,++py,lawngreen,elapsed*1000);

		    py+=2;
		    pgFancyPrint(35,py,lightblue,"Current Search Depth",1,0,0);
		    pgPrintInt(35,++py,lawngreen,g_current_searchdepth);

		    py+=2;
		    pgFancyPrint(35,py,lightblue,"Branches",1,0,0);
		    pgPrintInt(35,++py,lawngreen,search_info.branches);

		    pgFancyPrint(50,--py,lightblue,"Nodes",1,0,0);
		    pgPrintInt(50,++py,lawngreen,search_info.nodes);

		    display_killers(35,9);
		
		    pgScreenFlipV();

		    if (g_hardcore_debug)
		    {
			while (Read_Key () == 0) ;
		    }
		}
	    }
	    
	    t = -negascout_tables(-b, -a, d-1, 1);

	    int tested_check = 0;
	    
	    if ((t>a)&&(t<beta))
	    {
#ifndef CHECKTEST_ALL
		if (!fullincheckp(c))
#endif 
		{
		    tested_check = 1;
		    
		    /* re-search */
		    if (searched>1)
			a = -negascout_tables(-beta,-t, d-1, 1);
		    history[m] += stored_inc(d);
		    search_info.move_depth[p] = searched;
		    g_best_line[p] = m;
		    bestmove = m;
		}
	    }
	    
	    if (t>a)
	    {
#ifndef CHECKTEST_ALL
		if (tested_check || !fullincheckp(c))
#endif
		{
		    a=t;
		    tested_check = 1;
		}
	    }
		
	    if (a>=beta)
	    {
#ifndef CHECKTEST_ALL
		if (tested_check || !fullincheckp(c))
#endif
		{
		    undomove();
		    move_sp = restore_sp;
		    history[m] += cutoff_inc(d);
		    store_killer(m,p,a,beta);
		    store_hash(d, L_BOUND, a, m);
		    store_counter(m, p);
		    return a;
		}
	    }
	    b = a + 1;
	}
	undomove();
    }
  
    move_sp = restore_sp;

    if (searched)
    {
	if (a>oldalpha)
	{
	    store_hash(d, EXACT, a, bestmove);
	    store_counter(bestmove, p);
	}
	else
	{
	    store_hash(d, U_BOUND, a, bestmove);
	}
	
	return a;		/* at least one valid move */
    }
#ifdef CHECKTEST_ALL
    else if (incheck)
#else
    else if (incheckp(c))
#endif
	return LOSE+p;		/* farther we are from losing the better */
    else
	return STALEMATE;	/* no valid moves and not incheck */
}

int negascout_search(int a, int beta, int d)
{
    int searched = 0;
    int incheck;
    move_and_score *restore_sp;
    int b;
    int p;

    move bestmove = 0;
    int c = tomove();

    p = ply-search_info.startply;
    search_info.pv.length[p] = p;

    if (p&&position_seen_before())
	return 0;

    if (d<=0)
	return qsearch(a,beta,0);

    poll_check();
    search_info.branches++;
    
    b = beta;
    
    incheck = incheckp(c);

    restore_sp = move_sp;
  
    genmoves();
    //sortmoves(restore_sp, move_sp, 0);
    
    while (move_sp>restore_sp)
    {
	move m;
	int intocheck;
	
	m = popmove();
	domove(m);
     
	intocheck = (incheck) ? fullincheckp(c) : intocheckp(m, c);
      
	if (!intocheck)
	{
	    int t;
	    
	    searched++;
	    
	    if (d == g_current_searchdepth)
	    {
		static int bit = 0;

		bit = (bit+1) % 2;
		
		draw_board();
		
		pgPrint(35,1,bit?red:magenta,"Considering...");
		
		hilite_move(bestmove,red,blue);
		
		pgScreenFlipV();
	    }
	    
	    t = -negascout_search(-b, -a, d-1);

	    if ((t>a)&&(t<beta))
	    {
		if (searched>1)
		    a = -negascout_search(-beta,-t, d-1);
		history[m] += stored_inc(d);
		storepv(m, p);
		bestmove = m;
		search_info.move_depth[p] = searched;

		if (d == g_current_searchdepth)
		{
		    draw_board();
		    pgScreenFlipV();
		}
	    }

	    if (t>a) a = t;

	    if (a>=beta)
	    {
		history[m]+=cutoff_inc(d);
		undomove();
		move_sp = restore_sp;
		return a;
	    }
	    b = a + 1;
	}
	undomove();
    }
  
    move_sp = restore_sp;

    if (searched)
	return a;		/* at least one valid move */
    else if (incheck)
	return LOSE+p;		/* the farther we are from losing the better */
    else
	return STALEMATE;	/* no valid moves and not incheck */
}

int horrible_search(int a, int beta, int d)
{
    int searched = 0;
    int incheck;
    move_and_score *restore_sp;
    int b;
    int p;
    move bestmove = 0;

    int c = tomove();

    p = ply-search_info.startply;
    search_info.pv.length[p] = p;

    /* if (p&&position_seen_before()) */
/* 	return 0; */

    if (d<=0)
	return qsearch(a,beta,0);

    poll_check();
    search_info.branches++;
    
    b = beta;
    
    incheck = incheckp(c);
/*      if (incheck) d++; */

    restore_sp = move_sp;
  
    genmoves();
    //sortmoves(restore_sp, move_sp, 0);
    
    while (move_sp>restore_sp)
    {
	move m;
	int intocheck;
	
	m = popmove();
	domove(m);
      
	intocheck = (incheck) ? fullincheckp(c) : intocheckp(m, c);
      
	if (!intocheck)
	{
	    int t;
	    
	    searched++;

	    if (d == g_current_searchdepth)
	    {
		static int bit = 0;

		bit = (bit+1) % 2;
		
		draw_board();
		
		if (bit)
		    pgPrint(35,1,red,"Considering...");
		else
		    pgPrint(35,1,magenta,"Considering...");
		
		hilite_move(bestmove,red,blue);
		
		pgScreenFlipV();
	    }
	    
	    t = -horrible_search(-beta, -a, d-1);

	    
	    if (t>a)
	    {
		history[m] += stored_inc(d);
		//storepv(m, p);
		//search_info.move_depth[p] = searched;

		g_best_line[p] = m;
		a = t;

		bestmove = m;
	    }
	}
	undomove();
    }
  
    move_sp = restore_sp;

    if (searched)
	return a;		/* at least one valid move */
    else if (incheck)
	return LOSE+p;		/* the farther we are from losing the better */
    else
	return STALEMATE;	/* no valid moves and not incheck */
}

/* not currently used */
int MTDF(int estimate, int d)
{
    int g = estimate;
    int upperbound = WIN;
    int lowerbound = LOSE;
    
    while (upperbound>lowerbound)
    {
	int b = (g==lowerbound) ? g + 1 : g;

	g = search(b - 1, b, d, 1);
	
	if (g<b)
	    upperbound = g;
	else
	    lowerbound = g;
    }
    return g;
}

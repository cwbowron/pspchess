#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "bce.h"


int evaluate_bce();
int evaluate_bce_junior();
int evaluate_random();

evaluation_function g_evaluation_functions[] = 
{
    NULL, evaluate_bce, evaluate_bce_junior, evaluate_random
};

char *g_personality_names[] = 
{
    "Human", "BCE", "BCE-jr", "Random", NULL
};

evaluation_function evaluate = evaluate_bce;

int *weights;

int *base_weights;

int weight_mode=1;

int default_weights[]= {
    /* piece values */
    0,				/* no piece */
    1000,			/* pawn */
    3250,			/* knight */
    3500,			/* bishop */
    5000,			/* rook */
    9000,			/* queen */
    0,				/* king */
    /* positional stuff */
    0,				/* king side castle bonus */
    0,				/* queen side castle bonus */
    330,			/* no queen side castling rights penalty*/
    670,			/* no king side castling rights penalty*/
    50,				/* queen tropism */
    30,				/* rook tropism */
    150,			/* doubled rook bonus */
    100,			/* rook on open file */
    100,			/* rook on semi open file */
    200,			/* two bishop bonus */
    20,				/* knight tropism */
    80,				/* isolated pawn */
    150,			/* doubled pawn */
    100,			/* pawn backing up pawn */
    LOSE/4,			/* no mating material */
    200				/* rook on 7th rank */
};

/* passed pawn bonus by rank */
int passed_pawn_bonus[8] = 
{
    0,				
    100,
    200,
    300,
    400,
    500,
    600,
    700,
};


void setup_default_weights()
{
    weights = default_weights;
}

int squarevalue__(int i, chesspiece p)
{
    switch(p)
    {
	case WPAWN:
	    return whitepawnsquares[i];
	case BPAWN:
	    return blackpawnsquares[i];
	case WKNIGHT:
	case BKNIGHT:
	    return knightsquares[i];
	case WBISHOP:
	case BBISHOP:
	    return bishopsquares[i];
	case WROOK:
	case BROOK:
	    return rooksquares[i];
	case WQUEEN:
	case BQUEEN:
	    return queensquares[i];
	case WKING:
	case BKING:
	    return kingsquares[i];
	default:
	    return 0;
    }
}
    
int squarevalue(int f, int r, chesspiece p)
{
    return squarevalue__(SQ(f,r),p);
}


int material_advantage()
{
#ifdef FANCY    
    int d,t,np;
    
    d = board->material[WHITE] - board->material[BLACK];
    t = board->material[WHITE] + board->material[BLACK];
    np = board->pieces[(d>0)?WPAWN:BPAWN];
    
    return d + ((d * np) * (8000 - t)) / (64000 * (np + 1));
#else
    return board->material[WHITE] - board->material[BLACK];
#endif    
}

int mobility_advantage()
{
#ifdef NOT
    int mobility[2];
    move_and_score *restore_sp;

    restore_sp = move_sp;
    genmoves(WHITE);
    mobility[WHITE] = move_sp-restore_sp;
    move_sp = restore_sp;
    genmoves(BLACK);
    mobility[BLACK] = move_sp-restore_sp;
    move_sp = restore_sp;
    
    return (mobility[WHITE] - mobility[BLACK]) * MOBILITYFACTOR;
#else
    return 0;
#endif    
}


int mate_material(int c)
{
    if (board->material[c]<weights[ROOK_VALUE])
    {
	if (board->pieces[PIECE(c,pawn)] == 0)
	    return 0;
    }
    return 1;
}

#define verbose_print(file,rank,bonus,expl) /* do nothing*/ 
#define verbose_printf(foo) /* do nothing*/

inline int min_distance_to_king(int f, int r, int c)
{
    int o = opp(c);
    int df = abs(F(board->kings[o])-f);
    int dr = abs(R(board->kings[o])-r);

    return min(df,dr);
}

inline int manhattan_distance_to_king(int f, int r, int c)
{
    int o = opp(c);
    int df = abs(F(board->kings[o])-f);
    int dr = abs(R(board->kings[o])-r);

    return df+dr;
}

int rooks[2];

// bonus for queen/king tropism
int score_queen(int f, int r, int c)
{
    return (7-min_distance_to_king(f,r,c)) * weights[QUEEN_TROPISM];
}

// bonus for rook/king tropism
// bonus for rook on 7th rank
// bonus for doubled rooks
// bonus for rooks on open files (no pawns)
// bonus for rooks on semi open files (no own pawns)
int score_rook(int f, int r, int c)
{
    int o = opp(c);
    
    int bonus = (7-min_distance_to_king(f,r,c)) * weights[ROOK_TROPISM];
    
    if (c == WHITE)
    {
	if (r == 6)
	{
	    bonus += weights[SEVENTH_RANK_ROOK];
	}
    }
    else if (r == 1)
    {
	bonus += weights[SEVENTH_RANK_ROOK];
    }
    
    if (rooks[c] == -1)
    {
	rooks[c] = SQ(f,r);
    }
    else
    {
	if ((f==F(rooks[c]))||(r==R(rooks[c])))
	{
	    bonus += weights[DOUBLEDROOKS];
	}
    }
    
    if (!board->pawns[c][f+1])
    {
	bonus += weights[SEMIOPEN];
	if (!board->pawns[o][f+1])
	{
	    bonus += weights[OPENFILE];
	}
    }
    
    return bonus;
}

// bonus for knight/king tropism
int score_knight(int f, int r,int c)
{
    return (14-manhattan_distance_to_king(f,r,c)) * weights[KNIGHT_TROPISM];
}

// no bishop bonus
int score_bishop(int c)
{
    return 0;
}

// bonus for passed pawn
// penalty for doubled pawns
// penalty for isolated pawns
// bonus for pawns backing up pawns
int score_pawn(int f, int r, int c)
{
    int bonus = 0;

#define PASSED_PAWN_BONUS
#ifdef PASSED_PAWN_BONUS
    if (c==WHITE)
    {
	int x;
	
	for(x=r+1;x<8;x++)
	{
	    if (getpiece(f,x)==BPAWN)
		goto ppdone;
	    if (getpiece(f-1,x)==BPAWN)
		goto ppdone;
	    if (getpiece(f+1,x)==BPAWN)
		goto ppdone;
	}
	bonus += passed_pawn_bonus[r];
    }
    else
    {
	int x;
	
	for(x=r-1;x>0;x--)
	{
	    if (getpiece(f,x)==WPAWN)
		goto ppdone;
	    if (getpiece(f-1,x)==WPAWN)
		goto ppdone;
	    if (getpiece(f+1,x)==WPAWN)
		goto ppdone;
	}
	bonus += passed_pawn_bonus[7-r];
    }
    ppdone:
    
#endif    
    
    if (board->pawns[c][f+1]>1)
    {
	bonus -= weights[DOUBLED];
    }

    if (!board->pawns[c][f]&&!board->pawns[c][f+2])
    {
	bonus -= weights[ISOLATED];
    }
    else
    {
	int i;
	if (c == WHITE)
	{
	    for (i=r;i>0;i--)
	    {
		if ((f>0)&&(getpiece(f-1,i)==WPAWN))
		    goto Escape;
		if ((f<7)&&(getpiece(f+1,i)==WPAWN))
		    goto Escape;
	    }
	    bonus -= weights[BACKEDUP];
	}
	else
	{
	    for (i=r;i<8;i++)
	    {
		if ((f>0)&&(getpiece(f-1,i)==BPAWN))
		    goto Escape;
		if ((f<7)&&(getpiece(f+1,i)==BPAWN))
		    goto Escape;
	    }
	    bonus -= weights[BACKEDUP];
	}
    }
    
    Escape:
    return bonus;
}


int evaluate_bce()
{
    int adv=0;			/* white's advantage */
    int f,r;
    
    rooks[WHITE] = -1;
    rooks[BLACK] = -1;

    // if neither has mating material == draw
    // otherwise penalty for not having mating material
    if (!mate_material(WHITE))
    {
	if (!mate_material(BLACK))
	{
	    return 0;
	}
	adv += weights[NOMATERIAL];
    }
    else if (!mate_material(BLACK))
    {
	adv += -weights[NOMATERIAL];
    }

    // score individual piece bonuses
    for (f=0;f<8;f++)
    {
	for (r=0;r<8;r++)
	{
	    switch(getpiece(f,r))
	    {
		case WQUEEN:
		    adv += score_queen(f,r,WHITE);
		    break;
		case BQUEEN:
		    adv -= score_queen(f,r,BLACK);
		    break;
		case WROOK:
		    adv += score_rook(f,r,WHITE);
		    break;
		case BROOK:
		    adv -= score_rook(f,r,BLACK);
		    break;
		case WPAWN:
		    adv += score_pawn(f,r,WHITE);
		    break;
		case BPAWN:
		    adv -= score_pawn(f,r,BLACK);
		    break;
		case WKNIGHT:
		    adv += score_knight(f,r,WHITE);
		    break;
		case BKNIGHT:
		    adv -= score_knight(f,r,BLACK);
		    break;
/*  	    case WBISHOP: */
/*  		adv += score_bishop(f,r,WHITE); */
/*  		break; */
/*  	    case BBISHOP: */
/*  		adv -= score_bishop(f,r,BLACK); */
/*  		break; */
	    }
	}
    }

    // bonus for having both bishops (very useful in endgame)
    if (board->pieces[WBISHOP]>=2) {
	adv += weights[TWOBISHOPS];
    }

    if (board->pieces[BBISHOP]>=2) {
	adv -= weights[TWOBISHOPS];
    }

    // bonus for castling king side
    // bonus for castling queen side
    // penalty for losing king side castling rights
    // penatly for losing queen side castling rights
    if (wkcastledp())
    {
	adv+=weights[KCASTLEBONUS];
    }
    else if (wqcastledp())
    {
	adv+=weights[QCASTLEBONUS];
    }
    else
    {
	if (!wkcastlep())
	{
	    adv-=weights[NOCASTLEKING];
	}
	if (!wqcastlep())
	{
	    adv-=weights[NOCASTLEQUEEN];
	}
    }

    if (bkcastledp())
    {
	adv-=weights[KCASTLEBONUS];
    }
    else if (bqcastledp())
    {
	adv-=weights[QCASTLEBONUS];
    }
    else
    {
	if (!bkcastlep())
	{
	    adv+=weights[NOCASTLEKING];
	}
	if (!bqcastlep())
	{
	    adv+=weights[NOCASTLEQUEEN];
	}
    }

    adv += board->position[WHITE]-board->position[BLACK];
    adv += material_advantage();
    adv += mobility_advantage();

    return (tomove() == WHITE) ? adv : -adv;
}


int evaluate_bce_junior()
{
    int adv=0;			/* white's advantage */
    
    rooks[WHITE] = -1;
    rooks[BLACK] = -1;

    // if neither has mating material == draw
    // otherwise penalty for not having mating material
    if (!mate_material(WHITE))
    {
	if (!mate_material(BLACK))
	{
	    return 0;
	}
	adv += weights[NOMATERIAL];
    }
    else if (!mate_material(BLACK))
    {
	adv += -weights[NOMATERIAL];
    }

#if 0
    // score individual piece bonuses
    for (f=0;f<8;f++)
    {
	for (r=0;r<8;r++)
	{
	    switch(getpiece(f,r))
	    {
		case WQUEEN:
		    adv += score_queen(f,r,WHITE);
		    break;
		case BQUEEN:
		    adv -= score_queen(f,r,BLACK);
		    break;
		case WROOK:
		    adv += score_rook(f,r,WHITE);
		    break;
		case BROOK:
		    adv -= score_rook(f,r,BLACK);
		    break;
		case WPAWN:
		    adv += score_pawn(f,r,WHITE);
		    break;
		case BPAWN:
		    adv -= score_pawn(f,r,BLACK);
		    break;
		case WKNIGHT:
		    adv += score_knight(f,r,WHITE);
		    break;
		case BKNIGHT:
		    adv -= score_knight(f,r,BLACK);
		    break;
/*  	    case WBISHOP: */
/*  		adv += score_bishop(f,r,WHITE); */
/*  		break; */
/*  	    case BBISHOP: */
/*  		adv -= score_bishop(f,r,BLACK); */
/*  		break; */
	    }
	}
    }

    // bonus for having both bishops (very useful in endgame)
    if (board->pieces[WBISHOP]>=2) {
	adv += weights[TWOBISHOPS];
    }

    if (board->pieces[BBISHOP]>=2) {
	adv -= weights[TWOBISHOPS];
    }
    
    // bonus for castling king side
    // bonus for castling queen side
    // penalty for losing king side castling rights
    // penatly for losing queen side castling rights
    if (wkcastledp())
    {
	adv+=weights[KCASTLEBONUS];
    }
    else if (wqcastledp())
    {
	adv+=weights[QCASTLEBONUS];
    }
    else
    {
	if (!wkcastlep())
	{
	    adv-=weights[NOCASTLEKING];
	}
	if (!wqcastlep())
	{
	    adv-=weights[NOCASTLEQUEEN];
	}
    }

    if (bkcastledp())
    {
	adv-=weights[KCASTLEBONUS];
    }
    else if (bqcastledp())
    {
	adv-=weights[QCASTLEBONUS];
    }
    else
    {
	if (!bkcastlep())
	{
	    adv+=weights[NOCASTLEKING];
	}
	if (!bqcastlep())
	{
	    adv+=weights[NOCASTLEQUEEN];
	}
    }
#endif

    //adv += board->position[WHITE]-board->position[BLACK];
    adv += material_advantage();
    //adv += mobility_advantage();

    return (tomove() == WHITE) ? adv : -adv;
}

int evaluate_random()
{
    int n = getrandomnumber() % 8000;
    
    return n - 4000;
}


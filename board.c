/*
  This File is part of Boo's Chess Engine
  Copyright 2000 by Christopher Bowron
*/  
#include <stdio.h>
#include <string.h>

#include "bce.h"
#include "pg.h"
#include "colors.h"

struct castling_struct g_castling_info[2][2];

void clearboard()
{
    int f, r;
    for (f=0;f<8;f++)
	for (r=0; r<8; r++)
	    setpiece(f,r,empty);
}

char rep(chesspiece piece)
{
    int v = chesspiecevalue(piece);
    int c = chesspiececolor(piece);
    
    if (c == WHITE)
	return " PNBRQK"[v];
    else if (c==BLACK)
	return " pnbrqk"[v];
    else
	return ' ';
}

void setupboard()
{
    int f;
    int inits[] = { rook, knight, bishop, queen, king, bishop, knight, rook };
     
    clearboard();
    memset(g_captured_pieces, 0, sizeof(g_captured_pieces));
    
    for (f=0; f<8; f++)
    {
      setpiece(f, RANK2, WPAWN);
      setpiece(f, RANK7, BPAWN);

      setpiece(f, RANK1, makepiece(WHITE,inits[f]));
      setpiece(f, RANK8, makepiece(BLACK,inits[f]));
    }

    board->kings[WHITE] = E1;
    board->kings[BLACK] = E8;
    countmaterial();
    board->flags=makevariables();
}

void setupboard_wild_5()
{
    int f;
    int inits[] = { rook, knight, bishop, queen, king, bishop, knight, rook };
     
    clearboard();
    memset(g_captured_pieces, 0, sizeof(g_captured_pieces));
    
    for (f=0; f<8; f++)
    {
      setpiece(f, RANK7, WPAWN);
      setpiece(f, RANK2, BPAWN);

      setpiece(f, RANK8, makepiece(WHITE,inits[f]));
      setpiece(f, RANK1, makepiece(BLACK,inits[f]));
    }

    board->kings[WHITE] = E1;
    board->kings[BLACK] = E8;
    countmaterial();
    board->flags=makevariables();
    //board->flags = 0;
}

void setupboard_wild_8()
{
    int f;
    int inits[] = { rook, knight, bishop, queen, king, bishop, knight, rook };
     
    clearboard();
    memset(g_captured_pieces, 0, sizeof(g_captured_pieces));
    
    for (f=0; f<8; f++)
    {
      setpiece(f, RANK4, WPAWN);
      setpiece(f, RANK5, BPAWN);

      setpiece(f, RANK1, makepiece(WHITE,inits[f]));
      setpiece(f, RANK8, makepiece(BLACK,inits[f]));
    }

    board->kings[WHITE] = E1;
    board->kings[BLACK] = E8;
    countmaterial();
    board->flags=makevariables();
}

void setupboard_wild_8a()
{
    int f;
    int inits[] = { rook, knight, bishop, queen, king, bishop, knight, rook };
     
    clearboard();
    memset(g_captured_pieces, 0, sizeof(g_captured_pieces));
    
    for (f=0; f<8; f++)
    {
      setpiece(f, RANK5, WPAWN);
      setpiece(f, RANK4, BPAWN);

      setpiece(f, RANK1, makepiece(WHITE,inits[f]));
      setpiece(f, RANK8, makepiece(BLACK,inits[f]));
    }

    board->kings[WHITE] = E1;
    board->kings[BLACK] = E8;
    countmaterial();
    board->flags=makevariables();
}

// there may be better ways to do this...
void setupboard_fischer_random()
{
    int f;
    int inits[] = { 0, 0, 0, 0, 0, 0, 0, 0 };
    //int inits[] = { pawn, pawn, pawn, pawn, pawn, pawn, pawn, pawn };

    clearboard();
    memset(g_captured_pieces, 0, sizeof(g_captured_pieces));

    // king_locale = 1-6
    int king_locale = getrandomnumber() % 6 + 1;

    inits[king_locale] = king;

    int rook_1 = getrandomnumber() % king_locale;

    inits[rook_1] = rook;
    
    int rook_2 = getrandomnumber() % (7-king_locale);

    inits[7-rook_2] = rook;

    int b_1 = getrandomnumber() % 5;
    int bishop_locale = -1;
    
    int i;
    for (i=0;i<8;i++)
    {
	if (!inits[i])
	{
	    if (b_1 == 0)
	    {
		inits[i] = bishop;
		bishop_locale = i;
		break;
	    }
	    b_1--;
	}
    }

    int b_2 = getrandomnumber() % 4;

    int start_i = 1 - (bishop_locale % 2);

    for (i=start_i;;i=(i+2) % 8)
    {
	if (!inits[i])
	{
	    if (b_2 == 0)
	    {
		inits[i] = bishop;
		break;
	    }
	    b_2--;
	}
    }
    
    int q = getrandomnumber() % 3;
    for (i=0;;i++)
    {
	if (!inits[i])
	{
	    if (q == 0)
	    {
		inits[i] = queen;
		break;
	    }
	    q--;
	}
    }

    for (i=0;i<8;i++)
    {
	if (!inits[i])
	{
	    inits[i] = knight;
	}
    }
    
    for (f=0; f<8; f++)
    {
	setpiece(f, RANK2, WPAWN);
	setpiece(f, RANK7, BPAWN);

	if (inits[f])
	{
	    setpiece(f, RANK1, makepiece(WHITE,inits[f]));
	    setpiece(f, RANK8, makepiece(BLACK,inits[f]));
	}
    }

    countmaterial();
    board->flags=makevariables();
    //board->flags = 0;
}

void countmaterial()
{
    int i;
    
    memset(board->position, 0, sizeof(board->position));
    memset(board->material, 0, sizeof(board->material));
    memset(board->pawns, 0, sizeof(board->pawns));
    memset(board->pieces,0, sizeof(board->pieces));
    memset(board->pawnbits,0,sizeof(board->pawnbits));

    memset(g_castling_info,0,sizeof(g_castling_info));
    
    board->piececount[0] = 0;
    board->piececount[1] = 0;

    // this is important for reading rook info
    board->kings[WHITE] = 65;
    board->kings[BLACK] = 65;
    
    for (i=0;i<64;i++)
    {
	chesspiece p = getpiece__(i);

	updatematerial__(i,p,1);

	switch (p)
	{
	    case BKING:
	    {
		int rank = R(i);
		board->kings[BLACK] = i;
		g_castling_info[BLACK][0].king_start = i;
		g_castling_info[BLACK][1].king_start = i;

		g_castling_info[BLACK][0].king_end = SQ(FILEC,rank);
		g_castling_info[BLACK][1].king_end = SQ(FILEG,rank);

		g_castling_info[BLACK][0].rook_end = SQ(FILED,rank);
		g_castling_info[BLACK][1].rook_end = SQ(FILEF,rank);
	    }
	    break;
	    case WKING:
		
	    {
		int rank = R(i);
		board->kings[WHITE] = i;
		g_castling_info[WHITE][0].king_start = i;
		g_castling_info[WHITE][1].king_start = i;

		g_castling_info[WHITE][0].king_end = SQ(FILEC,rank);
		g_castling_info[WHITE][1].king_end = SQ(FILEG,rank);

		g_castling_info[WHITE][0].king_end = SQ(FILEC,rank);
		g_castling_info[WHITE][1].king_end = SQ(FILEG,rank);

		g_castling_info[WHITE][0].rook_end = SQ(FILED,rank);
		g_castling_info[WHITE][1].rook_end = SQ(FILEF,rank);
	    }
	    break;
	    case WROOK:
		if (i<board->kings[WHITE])
		    g_castling_info[WHITE][0].rook_start = i;
		else
		    g_castling_info[WHITE][1].rook_start = i;
		break;
	    case BROOK:
		if (i<board->kings[BLACK])
		    g_castling_info[BLACK][0].rook_start = i;
		else
		    g_castling_info[BLACK][1].rook_start = i;
		break;
	}
    }

    g_castling_info[BLACK][0].bittest = bqc;
    g_castling_info[BLACK][1].bittest = bkc;
    g_castling_info[WHITE][0].bittest = wqc;
    g_castling_info[WHITE][1].bittest = wkc;

    g_castling_info[BLACK][0].done_bit = bqcastled;
    g_castling_info[BLACK][1].done_bit = bkcastled;
    g_castling_info[WHITE][0].done_bit = wqcastled;
    g_castling_info[WHITE][1].done_bit = wkcastled;
    
/*     int a; */
/*     int b; */
/*     int y = 0; */

/*     pgFillvram(0); */
/*     for (a=0;a<2;a++) */
/*     { */
/* 	for (b=0;b<2;b++) */
/* 	{ */
/* 	    pgPrintInt(0,y,red,g_castling_info[a][b].bittest); */
/* 	    print_square(10,y,g_castling_info[a][b].king_start); */
/* 	    print_square(15,y,g_castling_info[a][b].king_end); */
/* 	    print_square(20,y,g_castling_info[a][b].rook_start); */
/* 	    y++; */
/* 	} */
/*     } */
/*     pgScreenFlipV(); */
/*     while (0 == Read_Key()); */

    compute_hash();
}


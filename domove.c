/*
  This file is part of Boo's Chess Engine
  Copyright 2000 by Christopher Bowron
*/  

#include <stdlib.h>

#include "bce.h"
#include "colors.h"

int ply;
move gamestack[MAXMOVES];
undo undostack[MAXMOVES];
hashtype positions[MAXMOVES];

/* clear flags that last only one ply */
inline void clear_flags()
{
    board->flags = (board->flags & ~(enp|dbl|cas|promo_flag));
}

void domove(move m)
{
    square start, end;
    chesspiece p, captured;
    int v;
    
    start = FR(m);
    end = TO(m);

    p = getpiece__(start);
    v = VALUE(p);
    
    captured = getpiece__(end);
    
    g_captured_pieces[captured]++;
    
    undostack[ply].m = m;
    undostack[ply].captured = captured;
    undostack[ply].moved = p;
    undostack[ply].flags = board->flags;
    
    gamestack[ply] = m;

    positions[ply] = board->hash;

    board -> hash ^= 1;
    
    /* clear flags that last only one ply */
    clear_flags();

    setpiece__(end, p);
    if (start!=end)
	setpiece__(start, empty);

    updatematerial__(end, captured, 0);
    updatemoved(start,end,p);

    // turn off castling rights on a rook move or rook capture
    // must loop thru both colors because it could be a rook move 
    // or a rook capture
    int i, j;
    for (i=0;i<2;i++)
    {
	for (j=0;j<2;j++)
	{
	    int rs = g_castling_info[i][j].rook_start;
	    
	    if ((end==rs)||(start==rs))
	    {
		board->flags &= ~(g_castling_info[i][j].bittest);
	    }
	}
    }

    if (v == pawn)
    {
	int ef = F(end);
	int er = R(end);
	int sf = F(start);
	int sr = R(start);
	    
	switch (er)
	{
	    // check for double pushes
	    case RANK4:
	    case RANK5:
		if (abs(er-sr)==2)
		    board->flags |= dbl;
		break;

	    // check for en passants
	    case RANK6:
	    case RANK3:
		if ((sf!=ef)&&!captured)
		{
		    board->flags |= enp;
		    updatematerial(ef, sr, p^1, 0);
		    setpiece(ef, sr, empty);
		}
		break;

	    // check for promotions
	    case RANK1:
	    case RANK8:
	    {
		int color = chesspiececolor(p);
		int piece_value = PR(m);
		chesspiece promo = makepiece(color, piece_value);
		setpiece__(end, promo);
		updatematerial__(end,p,0);
		updatematerial__(end,promo,1);
		board->flags |= promo_flag;
	    }
	    break;
	}
    }
    else if (v == king)
    {
	int c = COLOR(p);
	if (is_castle(m))
	{
	    int our_rook = PIECE(c,rook);
	    for (j=0;j<2;j++)
	    {
		if (start == g_castling_info[c][j].king_start &&
		    end == g_castling_info[c][j].king_end)
		{
		    board->flags |= cas;
		    board->flags |= g_castling_info[c][j].done_bit;

		    square s = g_castling_info[c][j].rook_start;
		    square e = g_castling_info[c][j].rook_end;
			    
		    // place rook on new square
		    setpiece__(e,our_rook);

		    // remove rook from old square as long as
		    // its not where we just place the king
		    if (s!=end)
			setpiece__(s,0);

		    updatemoved(s,e,our_rook);
		}
	    }
	}

	board->kings[c] = end;
	board->flags &= (c==WHITE) ? ~(wkc|wqc) : ~(bkc|bqc);
    }

    ply++;

    switch_sides();
}

void undomove()
{
    move m;
    chesspiece p;
    chesspiece captured;
    square start, end;
    int v;
    
    ply--;

    m = undostack[ply].m;
    p = undostack[ply].moved;
    v = VALUE(p);
    captured = undostack[ply].captured;

    g_captured_pieces[captured]--;
    
    start = FR(m);
    end = TO(m);

    if (board->flags & promo_flag)
    {
	/* remove promo */
	updatematerial__(end, getpiece__(end),0);
	/* restore pawn */
	updatematerial__(end, p, 1);		
    }
    else if (board->flags & enp)
    {
	int ef = F(end);
	int sr = R(start);

	// restore the pawn that was captured
	setpiece(ef, sr, p^1);
	updatematerial(ef, sr, p^1, 1);
    }
    else if (v == king)
    {
	int c = COLOR(p);
	
	board->kings[c] = start;
	
	if (is_castle(m))
	{
	    int j;
	    int our_rook = PIECE(c,rook);

	    for (j=0;j<2;j++)
	    {
		int ks = g_castling_info[c][j].king_start;
		int ke = g_castling_info[c][j].king_end;
		
		if ((start == ks)&&(end == ke))
		{
		    square s = g_castling_info[c][j].rook_start;
		    square e = g_castling_info[c][j].rook_end;

		    setpiece__(s, our_rook);
		    setpiece__(e, 0);
		    
		    updatematerial__(s, our_rook, 1);
		    updatematerial__(e, our_rook, 0);
		}
	    }
	}
    }
    /*
    else if (p == BKING)
    {
	board->kings[BLACK] = start;
	if (is_castle(m))
	{
	    int j;
	    for (j=0;j<2;j++)
	    {
		int ks = g_castling_info[BLACK][j].king_start;
		int ke = g_castling_info[BLACK][j].king_end;
		
		if ((start == ks)&&(end == ke))
		{
		    square s = g_castling_info[BLACK][j].rook_start;
		    square e = g_castling_info[BLACK][j].rook_end;

		    setpiece__(s, BROOK);
		    setpiece__(e, 0);

		    updatematerial__(s, BROOK, 1);
		    updatematerial__(e, BROOK, 0);
		}
	    }
	}
    }
    */
    
    setpiece__(start, p);
    if (start!=end)
	setpiece__(end, captured);
    
    updatematerial__(end, captured, 1);
    updatemoved(end, start, p);

    board->flags = undostack[ply].flags;
    board->hash = positions[ply];
}


void updatematerial__(int i, chesspiece p, int add)
{
    if (p)
    {
	int v, c, value, sq;

	v = VALUE(p);
	c = COLOR(p);

	value = default_weights[v];
	
	sq = squarevalue__(i,p);

	board->hash ^= randoms[p][i];

	if (add)
	{
	    board->pieces[p]++;
	    board->material[c] += value;
	    board->position[c] += sq;
	    
	    if (v==pawn)
	    {
		board->pawns[c][F(i)+1] += 1;
	    }
	    else
	    {
		board->piececount[c]++;
	    }
	}
	else
	{
	    board->pieces[p]--;
	    board->material[c] -= value;
	    board->position[c] -= sq;
	    
	    if (v==pawn)
	    {
		board->pawns[c][F(i)+1] -= 1;
	    }
	    else
	    {
		board->piececount[c]--;
	    }
	}
    }
}

void updatematerial(int f, int r, chesspiece p, int add)
{
    updatematerial__(SQ(f,r),p,add);
}

void updatemoved(square start, square end, chesspiece p)
{
    updatematerial__(start, p, 0);
    updatematerial__(end, p, 1);
}

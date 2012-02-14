/*
  This File is part of Boo's Chess Engine
  Copyright 2001 by Christopher Bowron
*/  

#include <stdlib.h>
#include "bce.h"

void getmovese(int f, int r, int c);
void getmovesp(int f, int r, int c);
void getmovesn(int f, int r, int c);
void getmovesb(int f, int r, int c);
void getmovesr(int f, int r, int c);
void getmovesq(int f, int r, int c);
void getmovesk(int f, int r, int c);
void getmovesk_nocastles(int f, int r, int c);

void genattackse(int f, int r, int c);
void genattacksp(int f, int r, int c);
void genattacksn(int f, int r, int c);
void genattacksb(int f, int r, int c);
void genattacksr(int f, int r, int c);
void genattacksq(int f, int r, int c);
void genattacksk(int f, int r, int c);

void gensliderattacks(int f, int r, int df, int dr, int color);
    
typedef void (*generator)(int, int, int);

move_and_score move_stack[8096],*move_sp;

inline void pushmove(move m)
{
    move_sp->m = m;
    move_sp ++;
}

inline move popmove()
{
    move_sp--;
    return move_sp->m;
}

generator attackgens[]=
{
    genattackse,
    genattacksp,
    genattacksn,
    genattacksb,
    genattacksr,
    genattacksq,
    genattacksk
};

generator generators[]=
{
    getmovese,
    getmovesp,
    getmovesn,
    getmovesb,
    getmovesr,
    getmovesq,
    getmovesk
};

void genmoves()
{
    int f,r;

    int col = tomove();
    for (f=0;f<8;f++)
    {
	for (r=0;r<8;r++)
	{
	    chesspiece p = getpiece(f,r);
	    if (p && (chesspiececolor(p) == col))
	    {
		int v = chesspiecevalue(p);
		generators[v](f,r,col);
	    }
	}
    }
}

void genmoves_nocastles()
{
    int f,r;

    int col = tomove();
    for (f=0;f<8;f++)
    {
	for (r=0;r<8;r++)
	{
	    chesspiece p = getpiece(f,r);
	    if (p && (chesspiececolor(p) == col))
	    {
		int v = chesspiecevalue(p);
		if (v==king)
		{
		    getmovesk_nocastles(f,r,col);
		}
		else
		{
		    generators[v](f,r,col);
		}
	    }
	}
    }
}


void genattacks()
{
    int f,r;

    int col = tomove();
    for (f=0;f<8;f++)
    {
	for (r=0;r<8;r++)
	{
	    chesspiece p = getpiece(f,r);
	    
	    if (p && (chesspiececolor(p)==col))
	    {
		int v;
		v = chesspiecevalue(p);
		attackgens[v](f,r,col);
	    }
	}
    }
}

void getmovese(int f, int r, int c)
{
    /* do nothing */
}

void getsliders(int f, int r, int df, int dr, int color)
{
    square s1 = SQ(f,r);
    chesspiece p;
    
    for (;;)
    {
	f += df;
	r += dr;

	if (offboardp(f,r)) return;
	
	p = getpiece(f,r);
	
	if (p == empty)
	    pushmove(MV(s1, SQ(f,r)));
	else if (chesspiececolor(p)==color)
	    return;
	else
	{
	    pushmove(MV(s1, SQ(f,r)));
	    return;
	}
    }
}


inline void push_pawn_move(square start, square end)
{
    if ((R(end) == 7)||(R(end)==0))
    {
	pushmove(MV_PR(start, end, queen));
	pushmove(MV_PR(start, end, knight));
	pushmove(MV_PR(start, end, rook));
	pushmove(MV_PR(start, end, bishop));
    }
    else
    {
	pushmove(MV(start,end));
    }
}

void getmovesp(int f, int r, int c)
{
    int forward = dir(c);
    square loc = SQ(f,r);
    
    r += forward;

    // captures
    if ((f<7) && (oppenentp (f+1, r, c)))
    {
	push_pawn_move(loc, SQ(1+f, r));
    }

    if ((f>0) && (oppenentp (f-1, r, c)))
    {
	push_pawn_move(loc, SQ(f-1, r));
    }
    
    if (emptyp(f,r))
    {
	int startrank = ((c == WHITE) ? 2 : 5);

	push_pawn_move(loc, SQ(f,r));
	
	/* can go two on first move */
	if ((r == startrank)&&(emptyp(f,r+forward)))
	    pushmove(MV(loc, SQ(f,r+forward)));
    }

    // if last move was double push we can possibly do an en passante
    if (doublepushp())
    {
	square start;
	square end;
	move m;

	m = lastmove();
	
	start = FR(m);
	end = TO(m);
	
	r -= forward;		/* restore to original r */
	if (R(end) == r)
	{
	    if (((f+1) == F(start))||
		((f-1) == F(start)))
		pushmove(MV(loc, SQ(F(start),forward+r)));
	}
    }
}

static int df[] = { 1,  1, -1, -1, 2, -2,  2, -2 };
static int dr[] = { 2, -2,  2, -2, 1,  1, -1, -1 };

void getmovesn(int f, int r, int c)
{
    int i, nf, nr;
    int o;
    chesspiece p;
    square s1;

    s1 = SQ(f,r);
    o = opp(c);

    for (i=0;i<8;i++)
    {
	nf = f+df[i];
	nr = r+dr[i];

	if (offboardp(nf,nr))
	    continue;

	p = getpiece(nf, nr);
	if (!p || (c != chesspiececolor(p)))
	{
	    pushmove(MV(s1, SQ(nf, nr)));
	}
    }
}

void getmovesb(int f, int r, int c)
{
    getsliders(f,r,1,1,c);
    getsliders(f,r,1,-1,c);
    getsliders(f,r,-1,1,c);
    getsliders(f,r,-1,-1,c);
}

void getmovesr(int f, int r, int c)
{
    getsliders(f,r, 1, 0,c);
    getsliders(f,r, 0, 1,c);
    getsliders(f,r,-1, 0,c);
    getsliders(f,r, 0,-1,c);
}

void getmovesq(int f, int r, int c)
{
    getmovesr(f,r,c);
    getmovesb(f,r,c);
}

// test whether this castling is allowed
int castling_test(struct castling_struct *info)
{
    // bit test for castling right
    if ((board->flags & info->bittest) == 0)
    {
	return 0;
    }
    
    square ks = info->king_start;
    square rs = info->rook_start;

    int i;
    int increment = abs(B1-A1);

    // test for empty squres in between rook and king
    for (i=min(ks,rs)+increment;i<max(rs,ks);i+=increment)
    {
	if (getpiece__(i)) return 0;
    }

    // end square for rook must be empty or be the king
    chesspiece p = getpiece__(info->rook_end);
    if (p && VALUE(p) != king)
	return 0;
    
    square ke = info->king_end;

    // test for attacks on king from start to end inclusive
    // genmoves_nocastles so that we do not recurse forever
    switch_sides();
    move_and_score *restore_sp = move_sp;
    genmoves_nocastles();

    while (move_sp>restore_sp)
    {
	move m = popmove();

	int i_min = min(ke,ks);
	int i_max = max(ke,ks);

	for (i=i_min;i<=i_max;i+=increment)
	{
	    if (TO(m) == i)
	    {
		move_sp = restore_sp;
		switch_sides();
		return 0;
	    }
	}
    }

    switch_sides();
    return 1;
}

void getmovesk_nocastles(int f, int r, int c)
{
    int df, dr, nf, nr;
    int o;
    chesspiece p;
    
    square s1 = SQ(f,r);
    o = opp(c);
    
    for (df=-1;df<2;df++)
    for (dr=-1;dr<2;dr++)
    {
	if (!df && !dr) continue;
	
	nf = f+df;
	nr = r+dr;
	p = getpiece(nf, nr);
	if (!offboardp(nf, nr) && (!p || (c != chesspiececolor(p))))
	    pushmove(MV(s1, SQ(nf, nr)));
    }
}


void getmovesk(int f, int r, int c)
{
    getmovesk_nocastles(f,r,c);
    
    if (c==WHITE)
    {
	if (castling_test(&g_castling_info[WHITE][0]))
	{
	    pushmove(MV_CA(g_castling_info[WHITE][0].king_start,
			   g_castling_info[WHITE][0].king_end));
	}
	else if (castling_test(&g_castling_info[WHITE][1]))
	{
	    pushmove(MV_CA(g_castling_info[WHITE][1].king_start,
			   g_castling_info[WHITE][1].king_end));
	}
    }
    else
    {
	if (castling_test(&g_castling_info[BLACK][0]))
	{
	    pushmove(MV_CA(g_castling_info[BLACK][0].king_start,
			   g_castling_info[BLACK][0].king_end));
	}
	else if (castling_test(&g_castling_info[BLACK][1]))
	{
	    pushmove(MV_CA(g_castling_info[BLACK][1].king_start,
			   g_castling_info[BLACK][1].king_end));
	}
    }
}

/* generate pawn captures */
void genattacksp(int f, int r, int c)
{
    int forward = dir(c);
    square loc = SQ(f,r);
    
    r += forward;

    if (f<7)
    {
	chesspiece p = getpiece(f+1, r);
	if (p&&chesspiececolor(p)!=c)
	    push_pawn_move(loc, SQ(f+1, r));
    }
    
    if (f>0)
    {
	chesspiece p = getpiece(f-1,r);
	if (p&&chesspiececolor(p)!=c)
	    push_pawn_move(loc, SQ(f-1, r));
    }

    /* get en passante */
    if (doublepushp())
    {
	square start;
	square end;
	move m;

	m = lastmove();
	
	start = FR(m);
	end = TO(m);
	
	r -= forward;		/* restore to original r */
	if (R(end) == r)
	{
	    if (((f+1) == F(start))||
		((f-1) == F(start)))
		pushmove(MV(loc, SQ(F(start),forward+r)));
	}
    }
}

void genattacksn(int f, int r, int c)
{
    int i, nf, nr;
    int o;
    square s1 = SQ(f,r);
    chesspiece p;
    
    o = opp(c);

    for (i=0;i<8;i++)
    {
	nf = f+df[i];
	nr = r+dr[i];

	if (offboardp(nf,nr))
	    continue;
	
	p = getpiece(nf, nr);
	if (p && (c != chesspiececolor(p)))
	{
	    pushmove(MV(s1, SQ(nf, nr)));
	}
    }
}

void genattacksb(int f, int r, int c)
{
    gensliderattacks(f,r,1,1,c);
    gensliderattacks(f,r,1,-1,c);
    gensliderattacks(f,r,-1,1,c);
    gensliderattacks(f,r,-1,-1,c);
}

void genattacksr(int f, int r, int c)
{
    gensliderattacks(f,r, 1, 0,c);
    gensliderattacks(f,r, 0, 1,c);
    gensliderattacks(f,r,-1, 0,c);
    gensliderattacks(f,r, 0,-1,c);
}

void genattacksq(int f, int r, int c)
{
    genattacksb(f,r,c);
    genattacksr(f,r,c);
}

void genattacksk(int f, int r, int c)
{
    int df, dr, nf, nr;
    int o;
    chesspiece p;
   
    square s1 = SQ(f,r);
    o = opp(c);
    
    for (df=-1;df<2;df++)
    for (dr=-1;dr<2;dr++)
    {
	if (!df && !dr) continue;
	
	nf = f+df;
	nr = r+dr;
	
	if (!offboardp(nf, nr)&&
	    (p = getpiece(nf, nr))&&
	    (c != chesspiececolor(p)))
	    pushmove(MV(s1, SQ(nf, nr)));
    }
}

void genattackse(int f, int r, int c)
{
    /* do nothing */
}

void gensliderattacks(int f, int r, int df, int dr, int color)
{
    chesspiece p;
    square s1=SQ(f,r);
    
    for (;;)
    {
	f += df;
	r += dr;

	if (offboardp (f,r)) return;
	
	p = getpiece(f,r);
	
	if (p)
	{
	    if (chesspiececolor(p) != color)
	    {
		pushmove(MV(s1, SQ(f,r)));
	    }
	    return;
	}
    }
}

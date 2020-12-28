//   GreKo chess engine
//   (c) 2002-2020 Vladimir Medvedev <vrm@bk.ru>
//   http://greko.su

#include "moves.h"


////////////////////////////////////////////////////////////////////////////////

void GenAllMoves(const Position& pos, MoveList& mvlist)
{
	mvlist.Clear();

	COLOR side = pos.Side();
	COLOR opp = side ^ 1;
	U64 occ = pos.BitsAll();
	U64 freeOrOpp = ~pos.BitsAll(side);

	PIECE piece, captured;
	U64 x, y;
	FLD from, to;

	//
	//   QUEENS
	//
	
	piece = QUEEN | side;
	x = pos.Bits(piece);
	while (x)
	{
		from = PopLSB(x);
		y = QueenAttacks(from, occ) & freeOrOpp;
		while (y)
		{
			to = PopLSB(y);
			captured = pos[to];
			mvlist.Add(from, to, piece, captured);
		}
	}

	//
	//   ROOKS
	//
	
	piece = ROOK | side;
	x = pos.Bits(piece);
	while (x)
	{
		from = PopLSB(x);
		y = RookAttacks(from, occ) & freeOrOpp;
		while (y)
		{
			to = PopLSB(y);
			captured = pos[to];
			mvlist.Add(from, to, piece, captured);
		}
	}

	//
	//   BISHOPS
	//
	
	piece = BISHOP | side;
	x = pos.Bits(piece);
	while (x)
	{
		from = PopLSB(x);
		y = BishopAttacks(from, occ) & freeOrOpp;
		while (y)
		{
			to = PopLSB(y);
			captured = pos[to];
			mvlist.Add(from, to, piece, captured);
		}
	}

	//
	//   KNIGHTS
	//
	
	piece = KNIGHT | side;
	x = pos.Bits(piece);
	while (x)
	{
		from = PopLSB(x);
		y = BB_KNIGHT_ATTACKS[from] & freeOrOpp;
		while (y)
		{
			to = PopLSB(y);
			captured = pos[to];
			mvlist.Add(from, to, piece, captured);
		}
	}

	//
	//   PAWNS
	//

	int fwd = -8 + 16 * side;
	int second = 6 - 5 * side;
	int seventh = 1 + 5 * side;

	piece = PAWN | side;
	x = pos.Bits(piece);
	while (x)
	{
		from = PopLSB(x);
		int row = Row(from);

		to = from + fwd;
		if (!pos[to])
		{
			if (row == second)
			{
				mvlist.Add(from, to, piece);
				to += fwd;
				if (!pos[to])
					mvlist.Add(from, to, piece);
			}
			else if (row == seventh)
			{
				mvlist.Add(from, to, piece, NOPIECE, QUEEN | side);
				mvlist.Add(from, to, piece, NOPIECE, ROOK | side);
				mvlist.Add(from, to, piece, NOPIECE, BISHOP | side);
				mvlist.Add(from, to, piece, NOPIECE, KNIGHT | side);
			}
			else
				mvlist.Add(from, to, piece);
		}

		y = BB_PAWN_ATTACKS[from][side] & pos.BitsAll(opp);
		while (y)
		{
			to = PopLSB(y);
			captured = pos[to];
			if (row == seventh)
			{
				mvlist.Add(from, to, piece, captured, QUEEN | side);
				mvlist.Add(from, to, piece, captured, ROOK | side);
				mvlist.Add(from, to, piece, captured, BISHOP | side);
				mvlist.Add(from, to, piece, captured, KNIGHT | side);
			}
			else
				mvlist.Add(from, to, piece, captured);
		}
	}

	if (pos.EP() != NF)
	{
		to = pos.EP();
		y = BB_PAWN_ATTACKS[to][opp] & pos.Bits(piece);
		while (y)
		{
			from = PopLSB(y);
			captured = piece ^ 1;
			mvlist.Add(from, to, piece, captured);
		}
	}

	//
	//   KINGS
	//

	piece = KING | side;
	from = pos.King(side);
	y = BB_KING_ATTACKS[from] & freeOrOpp;
	while (y)
	{
		to = PopLSB(y);
		captured = pos[to];
		mvlist.Add(from, to, piece, captured);
	}

	// castlings
	if (pos.CanCastle(side, KINGSIDE))
		mvlist.Add(MOVE_O_O[side]);

	if (pos.CanCastle(side, QUEENSIDE))
		mvlist.Add(MOVE_O_O_O[side]);
}
////////////////////////////////////////////////////////////////////////////////

void GenCapturesAndPromotions(const Position& pos, MoveList& mvlist)
{
	mvlist.Clear();

	COLOR side = pos.Side();
	COLOR opp = side ^ 1;
	U64 occ = pos.BitsAll();
	U64 targets = pos.BitsAll(opp);

	PIECE piece, captured;
	U64 x, y;
	FLD from, to;

	//
	//   QUEENS
	//
	
	piece = QUEEN | side;
	x = pos.Bits(piece);
	while (x)
	{
		from = PopLSB(x);
		y = QueenAttacks(from, occ) & targets;
		while (y)
		{
			to = PopLSB(y);
			captured = pos[to];
			mvlist.Add(from, to, piece, captured);
		}
	}

	//
	//   ROOKS
	//
	
	piece = ROOK | side;
	x = pos.Bits(piece);
	while (x)
	{
		from = PopLSB(x);
		y = RookAttacks(from, occ) & targets;
		while (y)
		{
			to = PopLSB(y);
			captured = pos[to];
			mvlist.Add(from, to, piece, captured);
		}
	}

	//
	//   BISHOPS
	//
	
	piece = BISHOP | side;
	x = pos.Bits(piece);
	while (x)
	{
		from = PopLSB(x);
		y = BishopAttacks(from, occ) & targets;
		while (y)
		{
			to = PopLSB(y);
			captured = pos[to];
			mvlist.Add(from, to, piece, captured);
		}
	}

	//
	//   KNIGHTS
	//
	
	piece = KNIGHT | side;
	x = pos.Bits(piece);
	while (x)
	{
		from = PopLSB(x);
		y = BB_KNIGHT_ATTACKS[from] & targets;
		while (y)
		{
			to = PopLSB(y);
			captured = pos[to];
			mvlist.Add(from, to, piece, captured);
		}
	}

	//
	//   PAWNS
	//

	int fwd = -8 + 16 * side;
	int seventh = 1 + 5 * side;

	piece = PAWN | side;
	x = pos.Bits(piece);
	while (x)
	{
		from = PopLSB(x);
		int row = Row(from);

		to = from + fwd;
		if (!pos[to])
		{
			if (row == seventh)
				mvlist.Add(from, to, piece, NOPIECE, QUEEN | side);
		}

		y = BB_PAWN_ATTACKS[from][side] & pos.BitsAll(opp);
		while (y)
		{
			to = PopLSB(y);
			captured = pos[to];
			if (row == seventh)
				mvlist.Add(from, to, piece, captured, QUEEN | side);
			else
				mvlist.Add(from, to, piece, captured);
		}
	}

	if (pos.EP() != NF)
	{
		to = pos.EP();
		y = BB_PAWN_ATTACKS[to][opp] & pos.Bits(piece);
		while (y)
		{
			from = PopLSB(y);
			captured = piece ^ 1;
			mvlist.Add(from, to, piece, captured);
		}
	}

	//
	//   KINGS
	//

	piece = KING | side;
	from = pos.King(side);
	y = BB_KING_ATTACKS[from] & targets;
	while (y)
	{
		to = PopLSB(y);
		captured = pos[to];
		mvlist.Add(from, to, piece, captured);
	}
}
////////////////////////////////////////////////////////////////////////////////

void AddSimpleChecks(const Position& pos, MoveList& mvlist)
{
	COLOR side = pos.Side();
	COLOR opp = side ^ 1;

	FLD K = pos.King(opp);
	U64 occ = pos.BitsAll();
	U64 free = ~occ;

	PIECE piece;
	U64 x, y;
	FLD from, to;

	U64 zoneN = BB_KNIGHT_ATTACKS[K] & free;
	U64 zoneB = BishopAttacks(K, occ) & free;
	U64 zoneR = RookAttacks(K, occ) & free;
	U64 zoneQ = zoneB | zoneR;

	//
	//   KNIGHTS
	//
	
	piece = KNIGHT | side;
	x = pos.Bits(piece);
	while (x)
	{
		from = PopLSB(x);
		y = BB_KNIGHT_ATTACKS[from] & zoneN;
		while (y)
		{
			to = PopLSB(y);
			mvlist.Add(from, to, piece);
		}
	}

	//
	//   BISHOPS
	//

	piece = BISHOP | side;
	x = pos.Bits(piece);
	while (x)
	{
		from = PopLSB(x);
		y = BishopAttacks(from, occ) & zoneB;
		while (y)
		{
			to = PopLSB(y);
			mvlist.Add(from, to, piece);
		}
	}

	//
	//   ROOKS
	//

	piece = ROOK | side;
	x = pos.Bits(piece);
	while (x)
	{
		from = PopLSB(x);
		y = RookAttacks(from, occ) & zoneR;
		while (y)
		{
			to = PopLSB(y);
			mvlist.Add(from, to, piece);
		}
	}

	//
	//   QUEENS
	//

	piece = QUEEN | side;
	x = pos.Bits(piece);
	while (x)
	{
		from = PopLSB(x);
		y = QueenAttacks(from, occ) & zoneQ;
		while (y)
		{
			to = PopLSB(y);
			mvlist.Add(from, to, piece);
		}
	}
}
////////////////////////////////////////////////////////////////////////////////

U64 GetCheckMask(const Position& pos)
{
	COLOR side = pos.Side();
	COLOR opp = side ^ 1;
	FLD K = pos.King(side);

	U64 x, occ = pos.BitsAll();
	U64 mask = 0;
	FLD from;

	x = BB_PAWN_ATTACKS[K][side] & pos.Bits(PAWN | opp);
	mask |= x;

	x = BB_KNIGHT_ATTACKS[K] & pos.Bits(KNIGHT | opp);
	mask |= x;

	x = BishopAttacks(K, occ) & (pos.Bits(BISHOP | opp) | pos.Bits(QUEEN | opp));
	while (x)
	{
		from = PopLSB(x);
		mask |= BB_SINGLE[from];
		mask |= BB_BETWEEN[K][from];
	}

	x = RookAttacks(K, occ) & (pos.Bits(ROOK | opp) | pos.Bits(QUEEN | opp));
	while (x)
	{
		from = PopLSB(x);
		mask |= BB_SINGLE[from];
		mask |= BB_BETWEEN[K][from];
	}

	return mask;
}
////////////////////////////////////////////////////////////////////////////////

void GenMovesInCheck(const Position& pos, MoveList& mvlist)
{
	mvlist.Clear();

	COLOR side = pos.Side();
	COLOR opp = side ^ 1;
	U64 occ = pos.BitsAll();
	U64 freeOrOpp = ~pos.BitsAll(side);

	PIECE piece, captured;
	U64 x, y;
	FLD from, to;

	U64 checkMask = GetCheckMask(pos);

	//
	//   KINGS
	//

	piece = KING | side;
	from = pos.King(side);
	y = BB_KING_ATTACKS[from] & freeOrOpp;
	while (y)
	{
		to = PopLSB(y);
		captured = pos[to];
		if (captured || !(BB_SINGLE[to] & checkMask))
			mvlist.Add(from, to, piece, captured);
	}

	//
	//   PAWNS
	//

	int fwd = -8 + 16 * side;
	int second = 6 - 5 * side;
	int seventh = 1 + 5 * side;

	piece = PAWN | side;
	x = pos.Bits(piece);
	while (x)
	{
		from = PopLSB(x);
		int row = Row(from);

		to = from + fwd;
		if (!pos[to])
		{
			if (row == second)
			{
				if (BB_SINGLE[to] & checkMask)
					mvlist.Add(from, to, piece);
				to += fwd;
				if (!pos[to])
					if (BB_SINGLE[to] & checkMask)
						mvlist.Add(from, to, piece);
			}
			else if (row == seventh)
			{
				if (BB_SINGLE[to] & checkMask)
				{
					mvlist.Add(from, to, piece, NOPIECE, QUEEN | side);
					mvlist.Add(from, to, piece, NOPIECE, ROOK | side);
					mvlist.Add(from, to, piece, NOPIECE, BISHOP | side);
					mvlist.Add(from, to, piece, NOPIECE, KNIGHT | side);
				}
			}
			else
			{
				if (BB_SINGLE[to] & checkMask)
					mvlist.Add(from, to, piece);
			}
		}

		y = BB_PAWN_ATTACKS[from][side] & pos.BitsAll(opp);
		while (y)
		{
			to = PopLSB(y);
			if (BB_SINGLE[to] & checkMask)
			{
				captured = pos[to];
				if (row == seventh)
				{
					mvlist.Add(from, to, piece, captured, QUEEN | side);
					mvlist.Add(from, to, piece, captured, ROOK | side);
					mvlist.Add(from, to, piece, captured, BISHOP | side);
					mvlist.Add(from, to, piece, captured, KNIGHT | side);
				}
				else
					mvlist.Add(from, to, piece, captured);
			}
		}
	}

	if (pos.EP() != NF)
	{
		to = pos.EP();
		y = BB_PAWN_ATTACKS[to][opp] & pos.Bits(piece);
		while (y)
		{
			from = PopLSB(y);
			captured = piece ^ 1;
			mvlist.Add(from, to, piece, captured);
		}
	}

	//
	//   KNIGHTS
	//
	
	piece = KNIGHT | side;
	x = pos.Bits(piece);
	while (x)
	{
		from = PopLSB(x);
		y = BB_KNIGHT_ATTACKS[from] & freeOrOpp & checkMask;
		while (y)
		{
			to = PopLSB(y);
			captured = pos[to];
			mvlist.Add(from, to, piece, captured);
		}
	}

	//
	//   BISHOPS
	//
	
	piece = BISHOP | side;
	x = pos.Bits(piece);
	while (x)
	{
		from = PopLSB(x);
		y = BishopAttacks(from, occ) & freeOrOpp & checkMask;
		while (y)
		{
			to = PopLSB(y);
			captured = pos[to];
			mvlist.Add(from, to, piece, captured);
		}
	}

	//
	//   ROOKS
	//
	
	piece = ROOK | side;
	x = pos.Bits(piece);
	while (x)
	{
		from = PopLSB(x);
		y = RookAttacks(from, occ) & freeOrOpp & checkMask;
		while (y)
		{
			to = PopLSB(y);
			captured = pos[to];
			mvlist.Add(from, to, piece, captured);
		}
	}

	//
	//   QUEENS
	//
	
	piece = QUEEN | side;
	x = pos.Bits(piece);
	while (x)
	{
		from = PopLSB(x);
		y = QueenAttacks(from, occ) & freeOrOpp & checkMask;
		while (y)
		{
			to = PopLSB(y);
			captured = pos[to];
			mvlist.Add(from, to, piece, captured);
		}
	}
}
////////////////////////////////////////////////////////////////////////////////

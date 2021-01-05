#include "chess.h"
#include "data.h"
/* modified 12/31/15 */
/*
 *******************************************************************************
 *                                                                             *
 *   GenerateCaptures() is used to generate capture and pawn promotion moves   *
 *   from the current position.                                                *
 *                                                                             *
 *   The destination square set is the set of squares occupied by opponent     *
 *   pieces, plus the set of squares on the 8th rank that pawns can advance to *
 *   and promote.                                                              *
 *                                                                             *
 *******************************************************************************
 */
unsigned *GenerateCaptures(TREE * RESTRICT tree, int ply, int side,
    unsigned *move) {
  uint64_t target, piecebd, moves, promotions, pcapturesl, pcapturesr;
  int from, to, temp, common, enemy = Flip(side);

/*
 ************************************************************
 *                                                          *
 *  We produce knight moves by locating the most advanced   *
 *  knight and then using that <from> square as an index    *
 *  into the precomputed knight_attacks data.  We repeat    *
 *  for each knight.                                        *
 *                                                          *
 ************************************************************
 */
  for (piecebd = Knights(side); piecebd; Clear(from, piecebd)) {
    from = MostAdvanced(side, piecebd);
    moves = knight_attacks[from] & Occupied(enemy);
    temp = from + (knight << 12);
    Extract(side, move, moves, temp);
  }
/*
 ************************************************************
 *                                                          *
 *  We produce sliding piece moves by locating each piece   *
 *  type in turn.  We then start with the most advanced     *
 *  piece and generate moves from that square.  This uses   *
 *  "magic move generation" to produce the destination      *
 *  squares.                                                *
 *                                                          *
 ************************************************************
 */
  for (piecebd = Bishops(side); piecebd; Clear(from, piecebd)) {
    from = MostAdvanced(side, piecebd);
    moves = BishopAttacks(from, OccupiedSquares) & Occupied(enemy);
    temp = from + (bishop << 12);
    Extract(side, move, moves, temp);
  }
  for (piecebd = Rooks(side); piecebd; Clear(from, piecebd)) {
    from = MostAdvanced(side, piecebd);
    moves = RookAttacks(from, OccupiedSquares) & Occupied(enemy);
    temp = from + (rook << 12);
    Extract(side, move, moves, temp);
  }
  for (piecebd = Queens(side); piecebd; Clear(from, piecebd)) {
    from = MostAdvanced(side, piecebd);
    moves = QueenAttacks(from, OccupiedSquares) & Occupied(enemy);
    temp = from + (queen << 12);
    Extract(side, move, moves, temp);
  }
/*
 ************************************************************
 *                                                          *
 *  We produce king moves by locating the only king and     *
 *  then using that <from> square as an index into the      *
 *  precomputed king_attacks data.                          *
 *                                                          *
 ************************************************************
 */
  from = KingSQ(side);
  moves = king_attacks[from] & Occupied(enemy);
  temp = from + (king << 12);
  Extract(side, move, moves, temp);
/*
 ************************************************************
 *                                                          *
 *  Now, produce pawn moves.  This is done differently due  *
 *  to inconsistencies in the way a pawn moves when it      *
 *  captures as opposed to normal non-capturing moves.      *
 *  Another exception is capturing enpassant.  The first    *
 *  step is to generate all possible pawn promotions.  We   *
 *  do this by removing all pawns but those on the 7th rank *
 *  and then advancing them if the square in front is empty.*
 *                                                          *
 ************************************************************
 */
  promotions = Pawns(side) & rank_mask[rank7[side]];
  promotions =
      ((side) ? promotions << 8 : promotions >> 8) & ~OccupiedSquares;
  for (; promotions; Clear(to, promotions)) {
    to = LSB(promotions);
    *move++ =
        (to + pawnadv1[side]) | (to << 6) | (pawn << 12) | (queen << 18);
  }
  target = Occupied(enemy) | EnPassantTarget(ply);
  pcapturesl =
      ((side) ? (Pawns(white) & mask_left_edge) << 7 : (Pawns(black) &
          mask_left_edge) >> 9) & target;
  for (; pcapturesl; Clear(to, pcapturesl)) {
    to = MostAdvanced(side, pcapturesl);
    common = (to + capleft[side]) | (to << 6) | (pawn << 12);
    if ((side) ? to < 56 : to > 7)
      *move++ = common | (((PcOnSq(to)) ? Abs(PcOnSq(to)) : pawn) << 15);
    else
      *move++ = common | Abs(PcOnSq(to)) << 15 | queen << 18;
  }
  pcapturesr =
      ((side) ? (Pawns(white) & mask_right_edge) << 9 : (Pawns(black) &
          mask_right_edge) >> 7) & target;
  for (; pcapturesr; Clear(to, pcapturesr)) {
    to = MostAdvanced(side, pcapturesr);
    common = (to + capright[side]) | (to << 6) | (pawn << 12);
    if ((side) ? to < 56 : to > 7)
      *move++ = common | (((PcOnSq(to)) ? Abs(PcOnSq(to)) : pawn) << 15);
    else
      *move++ = common | Abs(PcOnSq(to)) << 15 | queen << 18;
  }
  return move;
}

/* modified 12/31/15 */
/*
 *******************************************************************************
 *                                                                             *
 *   GenerateChecks() is used to generate non-capture moves from the current   *
 *   position.                                                                 *
 *                                                                             *
 *   The first pass produces a bitmap that contains the squares a particular   *
 *   piece type would attack if sitting on the square the enemy king sits on.  *
 *   We then use each of these squares as a source and check to see if the     *
 *   same piece type attacks one of these common targets.  If so, we can move  *
 *   that piece to that square and it will directly attack the king.  We do    *
 *   this for pawns, knights, bishops, rooks and queens to produce the set of  *
 *   "direct checking moves."                                                  *
 *                                                                             *
 *   Then we generate discovered checks in two passes, once for diagonal       *
 *   attacks and once for rank/file attacks (we do it in two passes since a    *
 *   rook can't produce a discovered check along a rank or file since it moves *
 *   in that direction as well.  For diagonals, we first generate the bishop   *
 *   attacks from the enemy king square and mask them with the friendly piece  *
 *   occupied squares bitmap.  This gives us a set of up to 4 "blocking        *
 *   pieces" that could be preventing a check.  We then remove them via the    *
 *   "magic move generation" tricks, and see if we now reach friendly bishops  *
 *   or queens on those diagonals.  If we have a friendly blocker, and a       *
 *   friendly diagonal mover behind that blocker, then moving the blocker is   *
 *   a discovered check (and there could be double-checks included but we do   *
 *   not check for that since a single check is good enough).  We repeat this  *
 *   for the ranks/files and we are done.                                      *
 *                                                                             *
 *   For the present, this code does not produce discovered checks by the      *
 *   king since all king moves are not discovered checks because the king can  *
 *   move in the same direction as the piece it blocks and not uncover the     *
 *   attack.  This might be fixed at some point, but it is rare enough to not  *
 *   be an issue except in far endgames.                                       *
 *                                                                             *
 *******************************************************************************
 */
unsigned *GenerateChecks(TREE * RESTRICT tree, int side, unsigned *move) {
  uint64_t temp_target, target, piecebd, moves;
  uint64_t padvances1, blockers, checkers;
  int from, to, promote, temp, enemy = Flip(side);

/*
 ************************************************************
 *                                                          *
 *  First pass:  produce direct checks.  For each piece     *
 *  type, we pretend that a piece of that type stands on    *
 *  the square of the king and we generate attacks from     *
 *  that square for that piece.  Now, if we can find any    *
 *  piece of that type that attacks one of those squares,   *
 *  then that piece move would deliver a direct check to    *
 *  the enemy king.  Easy, wasn't it?                       *
 *                                                          *
 ************************************************************
 */
  target = ~OccupiedSquares;
/*
 ************************************************************
 *                                                          *
 *  Knight direct checks.                                   *
 *                                                          *
 ************************************************************
 */
  temp_target = target & knight_attacks[KingSQ(enemy)];
  for (piecebd = Knights(side); piecebd; Clear(from, piecebd)) {
    from = MostAdvanced(side, piecebd);
    moves = knight_attacks[from] & temp_target;
    temp = from + (knight << 12);
    Extract(side, move, moves, temp);
  }
/*
 ************************************************************
 *                                                          *
 *  Sliding piece direct checks.                            *
 *                                                          *
 ************************************************************
 */
  temp_target = target & BishopAttacks(KingSQ(enemy), OccupiedSquares);
  for (piecebd = Bishops(side); piecebd; Clear(from, piecebd)) {
    from = MostAdvanced(side, piecebd);
    moves = BishopAttacks(from, OccupiedSquares) & temp_target;
    temp = from + (bishop << 12);
    Extract(side, move, moves, temp);
  }
  temp_target = target & RookAttacks(KingSQ(enemy), OccupiedSquares);
  for (piecebd = Rooks(side); piecebd; Clear(from, piecebd)) {
    from = MostAdvanced(side, piecebd);
    moves = RookAttacks(from, OccupiedSquares) & temp_target;
    temp = from + (rook << 12);
    Extract(side, move, moves, temp);
  }
  temp_target = target & QueenAttacks(KingSQ(enemy), OccupiedSquares);
  for (piecebd = Queens(side); piecebd; Clear(from, piecebd)) {
    from = MostAdvanced(side, piecebd);
    moves = QueenAttacks(from, OccupiedSquares) & temp_target;
    temp = from + (queen << 12);
    Extract(side, move, moves, temp);
  }
/*
 ************************************************************
 *                                                          *
 *   Pawn direct checks.                                    *
 *                                                          *
 ************************************************************
 */
  temp_target = target & pawn_attacks[enemy][KingSQ(enemy)];
  padvances1 = ((side) ? Pawns(white) << 8 : Pawns(black) >> 8) & temp_target;
  for (; padvances1; Clear(to, padvances1)) {
    to = MostAdvanced(side, padvances1);
    *move++ = (to + pawnadv1[side]) | (to << 6) | (pawn << 12);
  }
/*
 ************************************************************
 *                                                          *
 *  Second pass:  produce discovered checks.  Here we do    *
 *  things a bit differently.  We first take diagonal       *
 *  movers.  From the enemy king's position, we generate    *
 *  diagonal moves to see if any of them end at one of our  *
 *  pieces that does not slide diagonally, such as a rook,  *
 *  knight or pawn.  If we find one, we look further down   *
 *  that diagonal to see if we now find a diagonal moves    *
 *  (queen or bishop).  If so, any legal move by the        *
 *  blocking piece (except captures which have already been *
 *  generated) will be a discovered check that needs to be  *
 *  searched.  We do the same for vertical / horizontal     *
 *  rays that are blocked by bishops, knights or pawns that *
 *  would hide a discovered check by a rook or queen.       *
 *                                                          *
 *  First we look for diagonal discovered attacks.  Once we *
 *  know which squares hold pieces that create a discovered *
 *  check when they move, we generate those piece moves     *
 *  piece type by piece type.                               *
 *                                                          *
 ************************************************************
 */
  blockers =
      BishopAttacks(KingSQ(enemy),
      OccupiedSquares) & (Rooks(side) | Knights(side) | Pawns(side));
  if (blockers) {
    checkers =
        BishopAttacks(KingSQ(enemy),
        OccupiedSquares & ~blockers) & (Bishops(side) | Queens(side));
    if (checkers) {
      if (plus7dir[KingSQ(enemy)] & blockers &&
          !(plus7dir[KingSQ(enemy)] & checkers))
        blockers &= ~plus7dir[KingSQ(enemy)];
      if (plus9dir[KingSQ(enemy)] & blockers &&
          !(plus9dir[KingSQ(enemy)] & checkers))
        blockers &= ~plus9dir[KingSQ(enemy)];
      if (minus7dir[KingSQ(enemy)] & blockers &&
          !(minus7dir[KingSQ(enemy)] & checkers))
        blockers &= ~minus7dir[KingSQ(enemy)];
      if (minus9dir[KingSQ(enemy)] & blockers &&
          !(minus9dir[KingSQ(enemy)] & checkers))
        blockers &= ~minus9dir[KingSQ(enemy)];
      target = ~OccupiedSquares;
/*
 ************************************************************
 *                                                          *
 *  Knight discovered checks.                               *
 *                                                          *
 ************************************************************
 */
      temp_target = target & ~knight_attacks[KingSQ(enemy)];
      for (piecebd = Knights(side) & blockers; piecebd; Clear(from, piecebd)) {
        from = MostAdvanced(side, piecebd);
        moves = knight_attacks[from] & temp_target;
        temp = from + (knight << 12);
        Extract(side, move, moves, temp);
      }
/*
 ************************************************************
 *                                                          *
 *  Rook discovered checks.                                 *
 *                                                          *
 ************************************************************
 */
      target = ~OccupiedSquares;
      temp_target = target & ~RookAttacks(KingSQ(enemy), OccupiedSquares);
      for (piecebd = Rooks(side) & blockers; piecebd; Clear(from, piecebd)) {
        from = MostAdvanced(side, piecebd);
        moves = RookAttacks(from, OccupiedSquares) & temp_target;
        temp = from + (rook << 12);
        Extract(side, move, moves, temp);
      }
/*
 ************************************************************
 *                                                          *
 *  Pawn discovered checks.                                 *
 *                                                          *
 ************************************************************
 */
      piecebd =
          Pawns(side) & blockers & ((side) ? ~OccupiedSquares >> 8 :
          ~OccupiedSquares << 8);
      for (; piecebd; Clear(from, piecebd)) {
        from = MostAdvanced(side, piecebd);
        to = from + pawnadv1[enemy];
        if ((side) ? to > 55 : to < 8)
          promote = queen;
        else
          promote = 0;
        *move++ = from | (to << 6) | (pawn << 12) | (promote << 18);
      }
    }
  }
/*
 ************************************************************
 *                                                          *
 *  Next, we look for rank/file discovered attacks.  Once   *
 *  we know which squares hold pieces that create a         *
 *  discovered check when they move, we generate them       *
 *  piece type by piece type.                               *
 *                                                          *
 ************************************************************
 */
  blockers =
      RookAttacks(KingSQ(enemy),
      OccupiedSquares) & (Bishops(side) | Knights(side) | (Pawns(side) &
          rank_mask[Rank(KingSQ(enemy))]));
  if (blockers) {
    checkers =
        RookAttacks(KingSQ(enemy),
        OccupiedSquares & ~blockers) & (Rooks(side) | Queens(side));
    if (checkers) {
      if (plus1dir[KingSQ(enemy)] & blockers &&
          !(plus1dir[KingSQ(enemy)] & checkers))
        blockers &= ~plus1dir[KingSQ(enemy)];
      if (plus8dir[KingSQ(enemy)] & blockers &&
          !(plus8dir[KingSQ(enemy)] & checkers))
        blockers &= ~plus8dir[KingSQ(enemy)];
      if (minus1dir[KingSQ(enemy)] & blockers &&
          !(minus1dir[KingSQ(enemy)] & checkers))
        blockers &= ~minus1dir[KingSQ(enemy)];
      if (minus8dir[KingSQ(enemy)] & blockers &&
          !(minus8dir[KingSQ(enemy)] & checkers))
        blockers &= ~minus8dir[KingSQ(enemy)];
      target = ~OccupiedSquares;
/*
 ************************************************************
 *                                                          *
 *  Knight discovered checks.                               *
 *                                                          *
 ************************************************************
 */
      temp_target = target & ~knight_attacks[KingSQ(enemy)];
      for (piecebd = Knights(side) & blockers; piecebd; Clear(from, piecebd)) {
        from = MostAdvanced(side, piecebd);
        moves = knight_attacks[from] & temp_target;
        temp = from + (knight << 12);
        Extract(side, move, moves, temp);
      }
/*
 ************************************************************
 *                                                          *
 *  Bishop discovered checks.                               *
 *                                                          *
 ************************************************************
 */
      target = ~OccupiedSquares;
      temp_target = target & ~BishopAttacks(KingSQ(enemy), OccupiedSquares);
      for (piecebd = Bishops(side) & blockers; piecebd; Clear(from, piecebd)) {
        from = MostAdvanced(side, piecebd);
        moves = BishopAttacks(from, OccupiedSquares) & temp_target;
        temp = from + (bishop << 12);
        Extract(side, move, moves, temp);
      }
/*
 ************************************************************
 *                                                          *
 *  Pawn discovered checks.                                 *
 *                                                          *
 ************************************************************
 */
      piecebd =
          Pawns(side) & blockers & ((side) ? ~OccupiedSquares >> 8 :
          ~OccupiedSquares << 8);
      for (; piecebd; Clear(from, piecebd)) {
        from = MostAdvanced(side, piecebd);
        to = from + pawnadv1[enemy];
        if ((side) ? to > 55 : to < 8)
          promote = queen;
        else
          promote = 0;
        *move++ = from | (to << 6) | (pawn << 12) | (promote << 18);
      }
    }
  }
  return move;
}

/* modified 12/31/15 */
/*
 *******************************************************************************
 *                                                                             *
 *   GenerateCheckEvasions() is used to generate moves when the king is in     *
 *   check.                                                                    *
 *                                                                             *
 *   Three types of check-evasion moves are generated:                         *
 *                                                                             *
 *   (1) Generate king moves to squares that are not attacked by the           *
 *   opponent's pieces.  This includes capture and non-capture moves.          *
 *                                                                             *
 *   (2) Generate interpositions along the rank/file that the checking attack  *
 *   is coming along (assuming (a) only one piece is checking the king, and    *
 *   (b) the checking piece is a sliding piece [bishop, rook, queen]).         *
 *                                                                             *
 *   (3) Generate capture moves, but only to the square(s) that are giving     *
 *   check.  Captures are a special case.  If there is one checking piece,     *
 *   then capturing it by any piece is tried.  If there are two pieces         *
 *   checking the king, then the only legal capture to try is for the king to  *
 *   capture one of the checking pieces that is on an unattacked square.       *
 *                                                                             *
 *******************************************************************************
 */
unsigned *GenerateCheckEvasions(TREE * RESTRICT tree, int ply, int side,
    unsigned *move) {
  uint64_t target, targetc, targetp, piecebd, moves, empty, checksqs;
  uint64_t padvances1, padvances2, pcapturesl, pcapturesr, padvances1_all;
  int from, to, temp, common, enemy = Flip(side), king_square, checkers;
  int checking_square, check_direction1 = 0, check_direction2 = 0;

/*
 ************************************************************
 *                                                          *
 *  First, determine how many pieces are attacking the king *
 *  and where they are, so we can figure out how to legally *
 *  get out of check.                                       *
 *                                                          *
 ************************************************************
 */
  king_square = KingSQ(side);
  checksqs = AttacksTo(tree, king_square) & Occupied(enemy);
  checkers = PopCnt(checksqs);
  if (checkers == 1) {
    checking_square = LSB(checksqs);
    if (PcOnSq(checking_square) != pieces[enemy][pawn])
      check_direction1 = directions[checking_square][king_square];
    target = InterposeSquares(king_square, checking_square);
    target |= checksqs;
    target |= Kings(enemy);
  } else {
    target = Kings(enemy);
    checking_square = LSB(checksqs);
    if (PcOnSq(checking_square) != pieces[enemy][pawn])
      check_direction1 = directions[checking_square][king_square];
    checking_square = MSB(checksqs);
    if (PcOnSq(checking_square) != pieces[enemy][pawn])
      check_direction2 = directions[checking_square][king_square];
  }
/*
 ************************************************************
 *                                                          *
 *  The next step is to produce the set of valid            *
 *  destination squares.  For king moves, this is simply    *
 *  the set of squares that are not attacked by enemy       *
 *  pieces (if there are any such squares.)                 *
 *                                                          *
 *  Then, if the checking piece is not a knight, we need    *
 *  to know the checking direction so that we can either    *
 *  move the king off of that ray, or else block that ray.  *
 *                                                          *
 *  We produce king moves by locating the only king and     *
 *  then using that <from> square as an index into the      *
 *  precomputed king_attacks data.                          *
 *                                                          *
 ************************************************************
 */
  from = king_square;
  temp = from + (king << 12);
  for (moves = king_attacks[from] & ~Occupied(side); moves; Clear(to, moves)) {
    to = MostAdvanced(side, moves);
    if (!Attacks(tree, enemy, to)
        && directions[from][to] != check_direction1 &&
        directions[from][to] != check_direction2)
      *move++ = temp | (to << 6) | (Abs(PcOnSq(to)) << 15);
  }
/*
 ************************************************************
 *                                                          *
 *  We produce knight moves by locating the most advanced   *
 *  knight and then using that <from> square as an index    *
 *  into the precomputed knight_attacks data.  We repeat    *
 *  for each knight.                                        *
 *                                                          *
 ************************************************************
 */
  if (checkers == 1) {
    for (piecebd = Knights(side); piecebd; Clear(from, piecebd)) {
      from = MostAdvanced(side, piecebd);
      if (!PinnedOnKing(tree, side, from)) {
        moves = knight_attacks[from] & target;
        temp = from + (knight << 12);
        Extract(side, move, moves, temp);
      }
    }
/*
 ************************************************************
 *                                                          *
 *  We produce sliding piece moves by locating each piece   *
 *  type in turn.  We then start with the most advanced     *
 *  piece and generate moves from that square.  This uses   *
 *  "magic move generation" to produce the destination      *
 *  squares.                                                *
 *                                                          *
 ************************************************************
 */
    for (piecebd = Bishops(side); piecebd; Clear(from, piecebd)) {
      from = MostAdvanced(side, piecebd);
      if (!PinnedOnKing(tree, side, from)) {
        moves = BishopAttacks(from, OccupiedSquares) & target;
        temp = from + (bishop << 12);
        Extract(side, move, moves, temp);
      }
    }
    for (piecebd = Rooks(side); piecebd; Clear(from, piecebd)) {
      from = MostAdvanced(side, piecebd);
      if (!PinnedOnKing(tree, side, from)) {
        moves = RookAttacks(from, OccupiedSquares) & target;
        temp = from + (rook << 12);
        Extract(side, move, moves, temp);
      }
    }
    for (piecebd = Queens(side); piecebd; Clear(from, piecebd)) {
      from = MostAdvanced(side, piecebd);
      if (!PinnedOnKing(tree, side, from)) {
        moves = QueenAttacks(from, OccupiedSquares) & target;
        temp = from + (queen << 12);
        Extract(side, move, moves, temp);
      }
    }
/*
 ************************************************************
 *                                                          *
 *  Now, produce pawn moves.  This is done differently due  *
 *  to inconsistencies in the way a pawn moves when it      *
 *  captures as opposed to normal non-capturing moves.      *
 *  Another exception is capturing enpassant.  The first    *
 *  step is to generate all possible pawn moves.  We do     *
 *  this in 2 operations:  (1) shift the pawns forward one  *
 *  rank then AND with empty squares;  (2) shift the pawns  *
 *  forward two ranks and then AND with empty squares.      *
 *                                                          *
 ************************************************************
 */
    empty = ~OccupiedSquares;
    targetp = target & empty;
    if (side) {
      padvances1 = Pawns(white) << 8 & targetp;
      padvances1_all = Pawns(white) << 8 & empty;
      padvances2 = ((padvances1_all & ((uint64_t) 255 << 16)) << 8) & targetp;
    } else {
      padvances1 = Pawns(black) >> 8 & targetp;
      padvances1_all = Pawns(black) >> 8 & empty;
      padvances2 = ((padvances1_all & ((uint64_t) 255 << 40)) >> 8) & targetp;
    }
/*
 ************************************************************
 *                                                          *
 *  Now that we got 'em, we simply enumerate the to squares *
 *  as before, but in four steps since we have four sets of *
 *  potential moves.                                        *
 *                                                          *
 ************************************************************
 */
    for (; padvances2; Clear(to, padvances2)) {
      to = MostAdvanced(side, padvances2);
      if (!PinnedOnKing(tree, side, to + pawnadv2[side]))
        *move++ = (to + pawnadv2[side]) | (to << 6) | (pawn << 12);
    }
    for (; padvances1; Clear(to, padvances1)) {
      to = MostAdvanced(side, padvances1);
      if (!PinnedOnKing(tree, side, to + pawnadv1[side])) {
        common = (to + pawnadv1[side]) | (to << 6) | (pawn << 12);
        if ((side) ? to < 56 : to > 7)
          *move++ = common;
        else {
          *move++ = common | (queen << 18);
          *move++ = common | (knight << 18);
        }
      }
    }
/*
 ************************************************************
 *                                                          *
 *  And then we try to see if the checking piece can be     *
 *  captured by a friendly pawn.                            *
 *                                                          *
 ************************************************************
 */
    targetc = Occupied(enemy) | EnPassantTarget(ply);
    targetc = targetc & target;
    if (Pawns(enemy) & target & ((side) ? EnPassantTarget(ply) >> 8 :
            EnPassantTarget(ply) << 8))
      targetc = targetc | EnPassantTarget(ply);
    if (side) {
      pcapturesl = (Pawns(white) & mask_left_edge) << 7 & targetc;
      pcapturesr = (Pawns(white) & mask_right_edge) << 9 & targetc;
    } else {
      pcapturesl = (Pawns(black) & mask_left_edge) >> 9 & targetc;
      pcapturesr = (Pawns(black) & mask_right_edge) >> 7 & targetc;
    }
    for (; pcapturesl; Clear(to, pcapturesl)) {
      to = MostAdvanced(side, pcapturesl);
      if (!PinnedOnKing(tree, side, to + capleft[side])) {
        common = (to + capleft[side]) | (to << 6) | (pawn << 12);
        if ((side) ? to < 56 : to > 7)
          *move++ = common | (((PcOnSq(to)) ? Abs(PcOnSq(to)) : pawn) << 15);
        else {
          *move++ = common | Abs(PcOnSq(to)) << 15 | queen << 18;
          *move++ = common | Abs(PcOnSq(to)) << 15 | knight << 18;
        }
      }
    }
    for (; pcapturesr; Clear(to, pcapturesr)) {
      to = MostAdvanced(side, pcapturesr);
      if (!PinnedOnKing(tree, side, to + capright[side])) {
        common = (to + capright[side]) | (to << 6) | (pawn << 12);
        if ((side) ? to < 56 : to > 7)
          *move++ = common | (((PcOnSq(to)) ? Abs(PcOnSq(to)) : pawn) << 15);
        else {
          *move++ = common | Abs(PcOnSq(to)) << 15 | queen << 18;
          *move++ = common | Abs(PcOnSq(to)) << 15 | knight << 18;
        }
      }
    }
  }
  return move;
}

/* modified 12/31/15 */
/*
 *******************************************************************************
 *                                                                             *
 *   GenerateNoncaptures() is used to generate non-capture moves from the      *
 *   current position.                                                         *
 *                                                                             *
 *   Once the valid destination squares are known, we have to locate a         *
 *   friendly piece to compute the squares it attacks.                         *
 *                                                                             *
 *   Pawns are handled differently.  Regular pawn moves are produced by        *
 *   shifting the pawn bitmap 8 bits "forward" and anding this with the        *
 *   complement of the occupied squares bitmap double advances are then        *
 *   produced by anding the pawn bitmap with a mask containing 1's on the      *
 *   second rank, shifting this 16 bits "forward" and then anding this with    *
 *   the complement of the occupied squares bitmap as before.  If [to] reaches *
 *   the 8th rank, we produce a set of three moves, promoting the pawn to      *
 *   knight, bishop and rook (queen promotions were generated earlier by       *
 *   GenerateCaptures()).                                                      *
 *                                                                             *
 *******************************************************************************
 */
unsigned *GenerateNoncaptures(TREE * RESTRICT tree, int ply, int side,
    unsigned *move) {
  uint64_t target, piecebd, moves;
  uint64_t padvances1, padvances2, pcapturesl, pcapturesr;
  int from, to, temp, common, enemy = Flip(side);

/*
 ************************************************************
 *                                                          *
 *  First, produce castling moves when they are legal.      *
 *                                                          *
 ************************************************************
 */
  if (Castle(ply, side) > 0) {
    if (Castle(ply, side) & 1 && !(OccupiedSquares & OO[side])
        && !Attacks(tree, enemy, OOsqs[side][0])
        && !Attacks(tree, enemy, OOsqs[side][1])
        && !Attacks(tree, enemy, OOsqs[side][2])) {
      *move++ = (king << 12) + (OOto[side] << 6) + OOfrom[side];
    }
    if (Castle(ply, side) & 2 && !(OccupiedSquares & OOO[side])
        && !Attacks(tree, enemy, OOOsqs[side][0])
        && !Attacks(tree, enemy, OOOsqs[side][1])
        && !Attacks(tree, enemy, OOOsqs[side][2])) {
      *move++ = (king << 12) + (OOOto[side] << 6) + OOfrom[side];
    }
  }
  target = ~OccupiedSquares;
/*
 ************************************************************
 *                                                          *
 *  We produce knight moves by locating the most advanced   *
 *  knight and then using that <from> square as an index    *
 *  into the precomputed knight_attacks data.  We repeat    *
 *  for each knight.                                        *
 *                                                          *
 ************************************************************
 */
  for (piecebd = Knights(side); piecebd; Clear(from, piecebd)) {
    from = MostAdvanced(side, piecebd);
    moves = knight_attacks[from] & target;
    temp = from + (knight << 12);
    Extract(side, move, moves, temp);
  }
/*
 ************************************************************
 *                                                          *
 *  We produce sliding piece moves by locating each piece   *
 *  type in turn.  We then start with the most advanced     *
 *  piece and generate moves from that square.  This uses   *
 *  "magic move generation" to produce the destination      *
 *  squares.                                                *
 *                                                          *
 ************************************************************
 */
  for (piecebd = Bishops(side); piecebd; Clear(from, piecebd)) {
    from = MostAdvanced(side, piecebd);
    moves = BishopAttacks(from, OccupiedSquares) & target;
    temp = from + (bishop << 12);
    Extract(side, move, moves, temp);
  }
  for (piecebd = Rooks(side); piecebd; Clear(from, piecebd)) {
    from = MostAdvanced(side, piecebd);
    moves = RookAttacks(from, OccupiedSquares) & target;
    temp = from + (rook << 12);
    Extract(side, move, moves, temp);
  }
  for (piecebd = Queens(side); piecebd; Clear(from, piecebd)) {
    from = MostAdvanced(side, piecebd);
    moves = QueenAttacks(from, OccupiedSquares) & target;
    temp = from + (queen << 12);
    Extract(side, move, moves, temp);
  }
/*
 ************************************************************
 *                                                          *
 *  We produce king moves by locating the only king and     *
 *  then using that <from> square as an index into the      *
 *  precomputed king_attacks data.                          *
 *                                                          *
 ************************************************************
 */
  from = KingSQ(side);
  moves = king_attacks[from] & target;
  temp = from + (king << 12);
  Extract(side, move, moves, temp);
/*
 ************************************************************
 *                                                          *
 *  Now, produce pawn moves.  This is done differently due  *
 *  to inconsistencies in the way a pawn moves when it      *
 *  captures as opposed to normal non-capturing moves.      *
 *  First we generate all possible pawn moves.  We do this  *
 *  in 4 operations:  (1) shift the pawns forward one rank  *
 *  then and with empty squares;  (2) shift the pawns       *
 *  forward two ranks and then and with empty squares;      *
 *  (3) remove the a-pawn(s) then shift the pawns           *
 *  diagonally left then and with enemy occupied squares;   *
 *  (4) remove the h-pawn(s) then shift the pawns           *
 *  diagonally right then and with enemy occupied squares.  *
 *  note that the only captures produced are under-         *
 *  promotions, because the rest were done in GenCap.       *
 *                                                          *
 ************************************************************
 */
  padvances1 = ((side) ? Pawns(side) << 8 : Pawns(side) >> 8) & target;
  padvances2 =
      ((side) ? (padvances1 & mask_advance_2_w) << 8 : (padvances1 &
          mask_advance_2_b) >> 8) & target;
/*
 ************************************************************
 *                                                          *
 *  Now that we got 'em, we simply enumerate the to         *
 *  squares as before, but in four steps since we have      *
 *  four sets of potential moves.                           *
 *                                                          *
 ************************************************************
 */
  for (; padvances2; Clear(to, padvances2)) {
    to = MostAdvanced(side, padvances2);
    *move++ = (to + pawnadv2[side]) | (to << 6) | (pawn << 12);
  }
  for (; padvances1; Clear(to, padvances1)) {
    to = MostAdvanced(side, padvances1);
    common = (to + pawnadv1[side]) | (to << 6) | (pawn << 12);
    if ((side) ? to < 56 : to > 7)
      *move++ = common;
    else {
      *move++ = common | (rook << 18);
      *move++ = common | (bishop << 18);
      *move++ = common | (knight << 18);
    }
  }
/*
 ************************************************************
 *                                                          *
 *  Generate the rest of the promotions here since          *
 *  GenerateCaptures() only generated captures or           *
 *  promotions to a queen.                                  *
 *                                                          *
 ************************************************************
 */
  target = Occupied(enemy) & rank_mask[rank8[side]];
  pcapturesl =
      ((side) ? (Pawns(white) & mask_left_edge) << 7 : (Pawns(black) &
          mask_left_edge) >> 9) & target;
  for (; pcapturesl; Clear(to, pcapturesl)) {
    to = MostAdvanced(side, pcapturesl);
    common = (to + capleft[side]) | (to << 6) | (pawn << 12);
    *move++ = common | (Abs(PcOnSq(to)) << 15) | (rook << 18);
    *move++ = common | (Abs(PcOnSq(to)) << 15) | (bishop << 18);
    *move++ = common | (Abs(PcOnSq(to)) << 15) | (knight << 18);
  }
  pcapturesr =
      ((side) ? (Pawns(white) & mask_right_edge) << 9 : (Pawns(black) &
          mask_right_edge) >> 7) & target;
  for (; pcapturesr; Clear(to, pcapturesr)) {
    to = MostAdvanced(side, pcapturesr);
    common = (to + capright[side]) | (to << 6) | (pawn << 12);
    *move++ = common | (Abs(PcOnSq(to)) << 15) | (rook << 18);
    *move++ = common | (Abs(PcOnSq(to)) << 15) | (bishop << 18);
    *move++ = common | (Abs(PcOnSq(to)) << 15) | (knight << 18);
  }
  return move;
}

/* modified 12/31/15 */
/*
 *******************************************************************************
 *                                                                             *
 *   PinnedOnKing() is used to determine if the piece on <square> is pinned    *
 *   against the king, so that it's illegal to move it.  This is used to cull  *
 *   potential moves by GenerateCheckEvasions() so that illegal moves are not  *
 *   produced.                                                                 *
 *                                                                             *
 *******************************************************************************
 */
int PinnedOnKing(TREE * RESTRICT tree, int side, int square) {
  int ray, enemy = Flip(side);

/*
 ************************************************************
 *                                                          *
 *  First, determine if the piece being moved is on the     *
 *  same diagonal, rank or file as the king.  If not, then  *
 *  it can't be pinned and we return (0).                   *
 *                                                          *
 ************************************************************
 */
  ray = directions[square][KingSQ(side)];
  if (!ray)
    return 0;
/*
 ************************************************************
 *                                                          *
 *  If they are on the same ray, then determine if the king *
 *  blocks a bishop attack in one direction from this       *
 *  square and a bishop or queen blocks a bishop attack on  *
 *  the same diagonal in the opposite direction (or the     *
 *  same rank/file for rook/queen.)                         *
 *                                                          *
 ************************************************************
 */
  switch (Abs(ray)) {
    case 1:
      if (RankAttacks(square) & Kings(side))
        return (RankAttacks(square) & (Rooks(enemy) | Queens(enemy))) != 0;
      else
        return 0;
    case 7:
      if (Diagh1Attacks(square) & Kings(side))
        return (Diagh1Attacks(square) & (Bishops(enemy) | Queens(enemy))) !=
            0;
      else
        return 0;
    case 8:
      if (FileAttacks(square) & Kings(side))
        return (FileAttacks(square) & (Rooks(enemy) | Queens(enemy))) != 0;
      else
        return 0;
    case 9:
      if (Diaga1Attacks(square) & Kings(side))
        return (Diaga1Attacks(square) & (Bishops(enemy) | Queens(enemy))) !=
            0;
      else
        return 0;
  }
  return 0;
}

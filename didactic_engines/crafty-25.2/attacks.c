#include "chess.h"
#include "data.h"
/* last modified 01/07/14 */
/*
 *******************************************************************************
 *                                                                             *
 *   Attacks() is used to determine if <side> attacks <square>.  The algorithm *
 *   is simple, and is based on the AttacksTo() algorithm, but, rather than    *
 *   returning a bitmap of squares attacking <square> it returns a "1" as soon *
 *   as it finds anything that attacks <square>.                               *
 *                                                                             *
 *******************************************************************************
 */
int Attacks(TREE * RESTRICT tree, int side, int square) {
  if ((rook_attacks[square] & (Rooks(side) | Queens(side)))
      && (RookAttacks(square,
              OccupiedSquares) & (Rooks(side) | Queens(side))))
    return 1;
  if ((bishop_attacks[square] & (Bishops(side) | Queens(side)))
      && (BishopAttacks(square,
              OccupiedSquares) & (Bishops(side) | Queens(side))))
    return 1;
  if (KnightAttacks(square) & Knights(side))
    return 1;
  if (PawnAttacks(Flip(side), square) & Pawns(side))
    return 1;
  if (KingAttacks(square) & Kings(side))
    return 1;
  return 0;
}

/* last modified 01/07/14 */
/*
 *******************************************************************************
 *                                                                             *
 *   AttacksTo() is used to produce a bitboard which is a map of all squares   *
 *   that directly attack this <square>.  The non-sliding pieces are trivial   *
 *   to detect, but for sliding pieces, we use a bitboard trick.  The idea is  *
 *   to compute the squares a queen would attack, if it was standing on        *
 *   <square> and then look at the last square attacked in each direction to   *
 *   determine if it is a sliding piece that moves in the right direction.  To *
 *   finish up, we simply need to Or() all these attackers together.           *
 *                                                                             *
 *******************************************************************************
 */
uint64_t AttacksTo(TREE * RESTRICT tree, int square) {
  uint64_t attacks =
      (PawnAttacks(white, square) & Pawns(black)) | (PawnAttacks(black,
          square) & Pawns(white));
  uint64_t bsliders =
      Bishops(white) | Bishops(black) | Queens(white) | Queens(black);
  uint64_t rsliders =
      Rooks(white) | Rooks(black) | Queens(white) | Queens(black);
  attacks |= KnightAttacks(square) & (Knights(black) | Knights(white));
  if (bishop_attacks[square] & bsliders)
    attacks |= BishopAttacks(square, OccupiedSquares) & bsliders;
  if (rook_attacks[square] & rsliders)
    attacks |= RookAttacks(square, OccupiedSquares) & rsliders;
  attacks |= KingAttacks(square) & (Kings(black) | Kings(white));
  return attacks;
}

/* last modified 01/07/14 */
/*
 *******************************************************************************
 *                                                                             *
 *   AttacksFrom() is used to compute the set of squares the piece on <source> *
 *   attacks.                                                                  *
 *                                                                             *
 *******************************************************************************
 */
uint64_t AttacksFrom(TREE * RESTRICT tree, int side, int source) {

  switch (Abs(PcOnSq(source))) {
    case queen:
      return QueenAttacks(source, OccupiedSquares);
    case rook:
      return RookAttacks(source, OccupiedSquares);
    case bishop:
      return BishopAttacks(source, OccupiedSquares);
    case knight:
      return KnightAttacks(source);
    case pawn:
      return PawnAttacks(side, source);
    case king:
      return KingAttacks(source);
  }
  return 0;
}

/* last modified 01/07/14 */
/*
 *******************************************************************************
 *                                                                             *
 *   Attacked() is used to determine if <square> is attacked.  It returns a    *
 *   two bit value, 01 if <square> is attacked by <side>, 10 if <square> is    *
 *   attacked by <enemy> and 11 if <square> is attacked by both sides.         *
 *                                                                             *
 *******************************************************************************
 */
uint64_t Attacked(TREE * RESTRICT tree, int side, uint64_t squares) {
  uint64_t bsliders, rsliders, set;
  int square;

  bsliders = Bishops(side) | Queens(side);
  rsliders = Rooks(side) | Queens(side);
  for (set = squares; set; set &= set - 1) {
    square = LSB(set);
    do {
      if (KingAttacks(square) & Kings(side))
        break;
      if (KnightAttacks(square) & Knights(side))
        break;
      if (bishop_attacks[square] & bsliders &&
          BishopAttacks(square, OccupiedSquares) & bsliders)
        break;
      if (rook_attacks[square] & rsliders &&
          RookAttacks(square, OccupiedSquares) & rsliders)
        break;
      Clear(square, squares);
    } while (0);
  }
  return squares;
}

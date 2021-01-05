#include "chess.h"
#include "data.h"
/* last modified 01/06/16 */
/*
 *******************************************************************************
 *                                                                             *
 *   UnmakeMove() is responsible for updating the position database whenever a *
 *   move is retracted.  It is the exact inverse of MakeMove(). The hash       *
 *   signature(s) are not updated, they are just restored to their status that *
 *   was saved before the move was made, to save time.                         *
 *                                                                             *
 *******************************************************************************
 */
void UnmakeMove(TREE * RESTRICT tree, int ply, int side, int move) {
  uint64_t bit_move;
  int piece, from, to, captured, promote, enemy = Flip(side);

/*
 ************************************************************
 *                                                          *
 *  First, restore the hash signatures to their state prior *
 *  to this move being made by simply copying the old       *
 *  values.                                                 *
 *                                                          *
 ************************************************************
 */
  HashKey = tree->save_hash_key[ply];
  PawnHashKey = tree->save_pawn_hash_key[ply];
/*
 ************************************************************
 *                                                          *
 *  Now do the things that are common to all pieces, such   *
 *  as updating the bitboards and hash signature.           *
 *                                                          *
 ************************************************************
 */
  piece = Piece(move);
  from = From(move);
  to = To(move);
  captured = Captured(move);
  promote = Promote(move);
  bit_move = SetMask(from) | SetMask(to);
  ClearSet(bit_move, Pieces(side, piece));
  ClearSet(bit_move, Occupied(side));
  PcOnSq(to) = 0;
  PcOnSq(from) = pieces[side][piece];
/*
 ************************************************************
 *                                                          *
 *  Now do the piece-specific things by jumping to the      *
 *  appropriate routine (this only has to deal with pawns   *
 *  and king moves that are castling moves.                 *
 *                                                          *
 ************************************************************
 */
  switch (piece) {
    case pawn:
      if (captured == 1) {
        if (EnPassant(ply) == to) {
          TotalAllPieces++;
          Set(to + epsq[side], Pawns(enemy));
          Set(to + epsq[side], Occupied(enemy));
          PcOnSq(to + epsq[side]) = pieces[enemy][pawn];
          Material -= PieceValues(side, pawn);
          TotalPieces(enemy, pawn)++;
          captured = 0;
        }
      }
      if (promote) {
        TotalPieces(side, pawn)++;
        Clear(to, Pawns(side));
        Clear(to, Occupied(side));
        Clear(to, Pieces(side, promote));
        Material -= PieceValues(side, promote);
        Material += PieceValues(side, pawn);
        TotalPieces(side, occupied) -= p_vals[promote];
        TotalPieces(side, promote)--;
      }
      break;
    case knight:
    case bishop:
    case rook:
    case queen:
      break;
    case king:
      KingSQ(side) = from;
      if (Abs(to - from) == 2) {
        if (to == rook_G[side]) {
          from = rook_H[side];
          to = rook_F[side];
        } else {
          from = rook_A[side];
          to = rook_D[side];
        }
        bit_move = SetMask(from) | SetMask(to);
        ClearSet(bit_move, Rooks(side));
        ClearSet(bit_move, Occupied(side));
        PcOnSq(to) = 0;
        PcOnSq(from) = pieces[side][rook];
      }
      break;
  }
/*
 ************************************************************
 *                                                          *
 *  Next we restore information related to a piece that was *
 *  captured and is now being returned to the board.        *
 *                                                          *
 ************************************************************
 */
  if (captured) {
    TotalAllPieces++;
    Set(to, Pieces(enemy, captured));
    Set(to, Occupied(enemy));
    Material += PieceValues(enemy, captured);
    PcOnSq(to) = pieces[enemy][captured];
    TotalPieces(enemy, captured)++;
    if (captured != pawn)
      TotalPieces(enemy, occupied) += p_vals[captured];
  }
#if defined(DEBUG)
  ValidatePosition(tree, ply, move, "UnmakeMove(1)");
#endif
  return;
}

#include "chess.h"
#include "data.h"
/* last modified 01/06/16 */
/*
 *******************************************************************************
 *                                                                             *
 *   MakeMove() is responsible for updating the position database whenever a   *
 *   piece is moved.  It performs the following operations:  (1) update the    *
 *   board structure itself by moving the piece and removing any captured      *
 *   piece.  (2) update the hash keys.  (3) update material counts.  (4) then  *
 *   update castling status.  (5) and finally update number of moves since     *
 *   last reversible move.                                                     *
 *                                                                             *
 *   There are some special-cases handled here, such as en passant captures    *
 *   where the enemy pawn is not on the <target> square, castling which moves  *
 *   both the king and rook, and then rook moves/captures which give up the    *
 *   castling right to that side when the rook is moved.                       *
 *                                                                             *
 *   note:  side = 1 if white is to move, 0 otherwise.  enemy is the opposite  *
 *   and is 1 if it is not white to move, 0 otherwise.                         *
 *                                                                             *
 *******************************************************************************
 */
void MakeMove(TREE * RESTRICT tree, int ply, int side, int move) {
  uint64_t bit_move;
  int piece, from, to, captured, promote, enemy = Flip(side), cpiece;
#if defined(DEBUG)
  int i;
#endif

/*
 ************************************************************
 *                                                          *
 *  First, some basic information is updated for all moves  *
 *  before we do the piece-specific stuff.  We need to save *
 *  the current position and both hash signatures, and add  *
 *  the current position to the repetition-list for the     *
 *  side on move, before the move is actually made on the   *
 *  board.  We also update the 50 move rule counter which   *
 *  will be reset if a capture or pawn move is made here.   *
 *                                                          *
 *  If the en passant flag was set the previous ply, we     *
 *  have already used it to generate moves at this ply and  *
 *  we need to clear it before continuing.  If it is set,   *
 *  we also need to update the hash signature since the EP  *
 *  opportunity no longer exists after making any move at   *
 *  this ply (one ply deeper than when a pawn was advanced  *
 *  two squares).                                           *
 *                                                          *
 ************************************************************
 */
#if defined(DEBUG)
  ValidatePosition(tree, ply, move, "MakeMove(1)");
#endif
  tree->status[ply + 1] = tree->status[ply];
  tree->save_hash_key[ply] = HashKey;
  tree->save_pawn_hash_key[ply] = PawnHashKey;
  if (EnPassant(ply + 1)) {
    HashEP(EnPassant(ply + 1));
    EnPassant(ply + 1) = 0;
  }
  Reversible(ply + 1)++;
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
  cpiece = PcOnSq(to);
  ClearSet(bit_move, Pieces(side, piece));
  ClearSet(bit_move, Occupied(side));
  Hash(side, piece, from);
  Hash(side, piece, to);
  PcOnSq(from) = 0;
  PcOnSq(to) = pieces[side][piece];
/*
 ************************************************************
 *                                                          *
 *  Now do the piece-specific things by jumping to the      *
 *  appropriate routine.                                    *
 *                                                          *
 ************************************************************
 */
  switch (piece) {
    case pawn:
      HashP(side, from);
      HashP(side, to);
      Reversible(ply + 1) = 0;
      if (captured == 1 && !cpiece) {
        Clear(to + epsq[side], Pawns(enemy));
        Clear(to + epsq[side], Occupied(enemy));
        Hash(enemy, pawn, to + epsq[side]);
        HashP(enemy, to + epsq[side]);
        PcOnSq(to + epsq[side]) = 0;
        Material -= PieceValues(enemy, pawn);
        TotalPieces(enemy, pawn)--;
        TotalAllPieces--;
        captured = 0;
      }
      if (promote) {
        TotalPieces(side, pawn)--;
        Material -= PieceValues(side, pawn);
        Clear(to, Pawns(side));
        Hash(side, pawn, to);
        HashP(side, to);
        Hash(side, promote, to);
        PcOnSq(to) = pieces[side][promote];
        TotalPieces(side, occupied) += p_vals[promote];
        TotalPieces(side, promote)++;
        Material += PieceValues(side, promote);
        Set(to, Pieces(side, promote));
      } else if ((Abs(to - from) == 16) && (mask_eptest[to] & Pawns(enemy))) {
        EnPassant(ply + 1) = to + epsq[side];
        HashEP(to + epsq[side]);
      }
      break;
    case knight:
    case bishop:
    case queen:
      break;
    case rook:
      if (Castle(ply + 1, side) > 0) {
        if ((from == rook_A[side]) && (Castle(ply + 1, side) & 2)) {
          Castle(ply + 1, side) &= 1;
          HashCastle(1, side);
        } else if ((from == rook_H[side]) && (Castle(ply + 1, side) & 1)) {
          Castle(ply + 1, side) &= 2;
          HashCastle(0, side);
        }
      }
      break;
    case king:
      KingSQ(side) = to;
      if (Castle(ply + 1, side) > 0) {
        if (Castle(ply + 1, side) & 2)
          HashCastle(1, side);
        if (Castle(ply + 1, side) & 1)
          HashCastle(0, side);
        if (Abs(to - from) == 2) {
          Castle(ply + 1, side) = -1;
          piece = rook;
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
          Hash(side, rook, from);
          Hash(side, rook, to);
          PcOnSq(from) = 0;
          PcOnSq(to) = pieces[side][rook];
        } else
          Castle(ply + 1, side) = 0;
      }
      break;
  }
/*
 ************************************************************
 *                                                          *
 *  If this is a capture move, we also have to update the   *
 *  information that must change when a piece is removed    *
 *  from the board.                                         *
 *                                                          *
 ************************************************************
 */
  if (captured) {
    Reversible(ply + 1) = 0;
    TotalAllPieces--;
    if (promote)
      piece = promote;
    Hash(enemy, captured, to);
    Clear(to, Pieces(enemy, captured));
    Clear(to, Occupied(enemy));
    Material -= PieceValues(enemy, captured);
    TotalPieces(enemy, captured)--;
    if (captured != pawn)
      TotalPieces(enemy, occupied) -= p_vals[captured];
    switch (captured) {
      case pawn:
        HashP(enemy, to);
        break;
      case knight:
      case bishop:
      case queen:
        break;
      case rook:
        if (Castle(ply + 1, enemy) > 0) {
          if ((to == rook_A[enemy]) && (Castle(ply + 1, enemy) & 2)) {
            Castle(ply + 1, enemy) &= 1;
            HashCastle(1, enemy);
          } else if ((to == rook_H[enemy]) && (Castle(ply + 1, enemy) & 1)) {
            Castle(ply + 1, enemy) &= 2;
            HashCastle(0, enemy);
          }
        }
        break;
      case king:
#if defined(DEBUG)
        Print(2048, "captured a king (Make)\n");
        for (i = 1; i <= ply; i++)
          Print(2048,
              "ply=%2d, phase=%d, piece=%2d,from=%2d,to=%2d,captured=%2d\n",
              i, tree->phase[i], Piece(tree->curmv[i]), From(tree->curmv[i]),
              To(tree->curmv[i]), Captured(tree->curmv[i]));
        Print(2048, "ply=%2d, piece=%2d,from=%2d,to=%2d,captured=%2d\n", i,
            piece, from, to, captured);
        if (log_file)
          DisplayChessBoard(log_file, tree->position);
#endif
        break;
    }
  }
#if defined(DEBUG)
  ValidatePosition(tree, ply + 1, move, "MakeMove(2)");
#endif
  return;
}

/* last modified 01/06/16 */
/*
 *******************************************************************************
 *                                                                             *
 *   MakeMoveRoot() is used to make a move at the root of the game tree,       *
 *   before any searching is done.  It uses MakeMove() to execute the move,    *
 *   but then copies the resulting position back to position[0], the actual    *
 *   board position.  It handles the special-case of the draw-by-repetition    *
 *   rule by clearing the repetition list when a non-reversible move is made,  *
 *   since no repetitions are possible once such a move is played.             *
 *                                                                             *
 *******************************************************************************
 */
void MakeMoveRoot(TREE * RESTRICT tree, int side, int move) {
  int player;

/*
 ************************************************************
 *                                                          *
 *  First, make the move and then reset the repetition      *
 *  index if the 50 move rule counter was reset to zero.    *
 *                                                          *
 ************************************************************
 */
  MakeMove(tree, 0, side, move);
  if (Reversible(1) == 0)
    rep_index = -1;
  tree->rep_list[++rep_index] = HashKey;
/*
 ************************************************************
 *                                                          *
 *  One odd action is to note if the castle status is       *
 *  currently negative, which indicates that that side      *
 *  castled during the previous search.  We simply set the  *
 *  castle status for that side to zero and we are done.    *
 *                                                          *
 *  We then copy this back to ply=0 status (which is the    *
 *  permanent game-board ply).                              *
 *                                                          *
 ************************************************************
 */
  for (player = black; player <= white; player++)
    Castle(1, player) = Max(0, Castle(1, player));
  tree->status[0] = tree->status[1];
}

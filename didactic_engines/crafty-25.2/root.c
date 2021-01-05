#include "chess.h"
#include "data.h"
#include "epdglue.h"
#if defined(SYZYGY)
#  include "tbprobe.h"
#endif
/* last modified 07/11/16 */
/*
 *******************************************************************************
 *                                                                             *
 *   RootMoveList() is used to set up the ply one move list.  It is a  more    *
 *   accurate ordering of the move list than that done for plies deeper than   *
 *   one.  Briefly, Quiesce() is used to obtain the positional score plus the  *
 *   expected gain/loss for pieces that can be captured.                       *
 *                                                                             *
 *******************************************************************************
 */
void RootMoveList(int wtm) {
  TREE *const tree = block[0];
  ROOT_MOVE rtemp;
  unsigned mvp, *lastm, rmoves[256];
  int value, done;
#if defined(SYZYGY)
  int tb_result, tb_root = -9;
#endif

/*
 ************************************************************
 *                                                          *
 *  If the position at the root is a draw, based on EGTB    *
 *  results, we are going to behave differently.  We will   *
 *  extract the root moves that are draws, and toss the     *
 *  losers out.  Then, we will do a normal search on the    *
 *  moves that draw to try and chose the drawing move that  *
 *  gives our opponent the best chance to make an error.    *
 *  This search will be done sans EGTB probes since we al-  *
 *  ready know this is a draw at the root.  We simply find  *
 *  the best move (based on search/eval) that preserves the *
 *  draw.                                                   *
 *                                                          *
 ************************************************************
 */
#if defined(SYZYGY)
  EGTB_draw = 0;
  if (swindle_mode) {
    if (EGTBlimit && TotalAllPieces <= EGTBlimit &&
        Castle(1, white) + Castle(1, black) == 0) {
      tb_result =
          tb_probe_root(Occupied(white), Occupied(black),
          Kings(white) | Kings(black), Queens(white) | Queens(black),
          Rooks(white) | Rooks(black), Bishops(white) | Bishops(black),
          Knights(white) | Knights(black), Pawns(white) | Pawns(black),
          Reversible(1), 0, EnPassant(1), wtm, NULL);
      if (tb_result != TB_RESULT_FAILED) {
        tb_root = TB_GET_WDL(tb_result);
        if ((tb_root == TB_DRAW && MaterialSTM(wtm) > 0) ||
            (tb_root == TB_CURSED_WIN))
          EGTB_draw = 1;
      }
    }
  }
#endif
/*
 ************************************************************
 *                                                          *
 *  First, use GenerateMoves() to generate the set of legal *
 *  moves from the root position.                           *
 *                                                          *
 ************************************************************
 */
  lastm = GenerateCaptures(tree, 1, wtm, rmoves);
  lastm = GenerateNoncaptures(tree, 1, wtm, lastm);
  n_root_moves = lastm - rmoves;
  for (mvp = 0; mvp < n_root_moves; mvp++)
    root_moves[mvp].move = rmoves[mvp];
/*
 ************************************************************
 *                                                          *
 *  Now make each move and use Quiesce() to analyze the     *
 *  potential captures and return a minimax score.          *
 *                                                          *
 *  Special case:  if this is an egtb draw at the root,     *
 *  then this is where we cull the non-drawing moves by     *
 *  doing an EGTB probe for each move.  Any moves that lose *
 *  are left with a very bad sorting score to move them to  *
 *  the end of the root move list.                          *
 *                                                          *
 ************************************************************
 */
  abort_search = 0;
  for (mvp = 0; mvp < n_root_moves; mvp++) {
    value = -4000000;
#if defined(TRACE)
    if (trace_level >= 1) {
      tree->curmv[1] = root_moves[mvp].move;
      Trace(tree, 1, 0, wtm, -MATE, MATE, "RootMoves()", serial, HASH,
          mvp + 1);
    }
#endif
    MakeMove(tree, 1, wtm, root_moves[mvp].move);
    tree->nodes_searched++;
    if (!Check(wtm))
      do {
        tree->curmv[1] = root_moves[mvp].move;
#if defined(SYZYGY)
        if (EGTB_draw && TotalAllPieces <= EGTBlimit &&
            Castle(2, white) + Castle(2, black) == 0) {
          tb_result =
              tb_probe_root(Occupied(white), Occupied(black),
              Kings(white) | Kings(black), Queens(white) | Queens(black),
              Rooks(white) | Rooks(black), Bishops(white) | Bishops(black),
              Knights(white) | Knights(black), Pawns(white) | Pawns(black),
              Reversible(2), 0, EnPassant(2), Flip(wtm), NULL);
          if (tb_result != TB_RESULT_FAILED) {
            tb_result = 4 - TB_GET_WDL(tb_result);
            if (tb_result < tb_root)
              break;
          }
        }
#endif
        value = -Quiesce(tree, 2, Flip(wtm), -MATE, MATE, 0);
/*
 ************************************************************
 *                                                          *
 *  Add in a bonus if this move is part of the previous     *
 *  principal variation.  It was good in the search, we     *
 *  should try it first now.                                *
 *                                                          *
 ************************************************************
 */
        if ((Piece(root_moves[mvp].move) == Piece(last_pv.path[1]))
            && (From(root_moves[mvp].move) == From(last_pv.path[1]))
            && (To(root_moves[mvp].move) == To(last_pv.path[1]))
            && (Captured(root_moves[mvp].move) == Captured(last_pv.path[1]))
            && (Promote(root_moves[mvp].move) == Promote(last_pv.path[1])))
          value += 2000000;
/*
 ************************************************************
 *                                                          *
 *  Fudge the score for promotions so that promotion to a   *
 *  queen is tried first.                                   *
 *                                                          *
 ************************************************************
 */
        if (Promote(root_moves[mvp].move) &&
            (Promote(root_moves[mvp].move) != queen))
          value -= 50;
      } while (0);
    root_moves[mvp].path = tree->pv[1];
    root_moves[mvp].path.pathv = value;
    root_moves[mvp].status = 0;
    root_moves[mvp].bm_age = 0;
    UnmakeMove(tree, 1, wtm, root_moves[mvp].move);
  }
/*
 ************************************************************
 *                                                          *
 *  Sort the moves into order based on the scores returned  *
 *  by Quiesce() which includes evaluation + captures.      *
 *                                                          *
 ************************************************************
 */
  do {
    done = 1;
    for (mvp = 0; mvp < n_root_moves - 1; mvp++) {
      if (root_moves[mvp].path.pathv < root_moves[mvp + 1].path.pathv) {
        rtemp = root_moves[mvp];
        root_moves[mvp] = root_moves[mvp + 1];
        root_moves[mvp + 1] = rtemp;
        done = 0;
      }
    }
  } while (!done);
/*
 ************************************************************
 *                                                          *
 *  Trim the move list to eliminate those moves that hang   *
 *  the king and are illegal.  This also culls any non-     *
 *  drawing moves when we are in the swindle-mode situation *
 *  and want to do a normal search but only on moves that   *
 *  preserve the draw.                                      *
 *                                                          *
 ************************************************************
 */
  for (; n_root_moves; n_root_moves--)
    if (root_moves[n_root_moves - 1].path.pathv > -3000000)
      break;
  if (root_moves[0].path.pathv > 1000000)
    root_moves[0].path.pathv -= 2000000;
/*
 ************************************************************
 *                                                          *
 *  Debugging output to dump root move list and the stuff   *
 *  used to sort them, for testing and debugging.           *
 *                                                          *
 ************************************************************
 */
  if (display_options & 128) {
    Print(128, "%d moves at root\n", n_root_moves);
    Print(128, "     score    move/pv\n");
    for (mvp = 0; mvp < n_root_moves; mvp++)
      Print(128, "%10s    %s\n", DisplayEvaluation(root_moves[mvp].path.pathv,
              wtm), DisplayPath(tree, wtm, &root_moves[mvp].path));
  }
/*
 ************************************************************
 *                                                          *
 *  Finally, before we return the list of moves, we need to *
 *  set the values to an impossible negative value so that  *
 *  as we sort the root move list after fail highs and lows *
 *  the un-searched moves won't pop to the top of the list. *
 *                                                          *
 ************************************************************
 */
  for (mvp = 1; mvp < n_root_moves; mvp++)
    root_moves[mvp].path.pathv = -MATE;
  return;
}

/* last modified 07/11/16 */
/*
 *******************************************************************************
 *                                                                             *
 *   RootMoveEGTB() is used to handle the case where we are using syzygy end-  *
 *   game tablebases and the root position is found in them.  We need to use   *
 *   the DTZ tables to play the best move we can find since the game outcome   *
 *   is known for each possible move at this point.  We return it in a manner  *
 *   similar to Book().                                                        *
 *                                                                             *
 *   Note:  This depends on RootMoveList() being called FIRST since it is the  *
 *   responsible party to note that we are drawn at the root according to EGTB *
 *   and if appropriate, it will let RootMoveEGTB() know this to activate      *
 *   "swindle mode" and play on with a search rather than an instant move.     *
 *                                                                             *
 *******************************************************************************
 */
int RootMoveEGTB(int wtm) {
#if defined(SYZYGY)
  TREE *const tree = block[0];
  int tb_result, result;

/*
 ************************************************************
 *                                                          *
 *  first, we need to find the best TB move.  Simply, this  *
 *  is the move that gives us the best result, even though  *
 *  it might be speculative in the case of choosing a       *
 *  "cursed win" which is still technically a draw if the   *
 *  opponent makes no errors.                               *
 *                                                          *
 ************************************************************
 */
  EGTB_use = EGTBlimit;
  if (EGTB_use <= 0)
    return 0;
  if (EGTB_draw && !puzzling && swindle_mode)
    EGTB_use = 0;
  if (EGTBlimit && !EGTB_use)
    Print(32, "Drawn at root, trying for swindle.\n");
  if (EGTB_use && TotalAllPieces <= EGTBlimit && !Castle(0, white) &&
      !Castle(0, black)) {
    tree->egtb_probes++;
    tb_result =
        tb_probe_root(Occupied(white), Occupied(black),
        Kings(white) | Kings(black), Queens(white) | Queens(black),
        Rooks(white) | Rooks(black), Bishops(white) | Bishops(black),
        Knights(white) | Knights(black), Pawns(white) | Pawns(black),
        Reversible(1), 0, EnPassant(1), wtm, NULL);
    if (tb_result != TB_RESULT_FAILED) {
      int value, piece, captured;
      unsigned cmove, omove;

      if (n_root_moves > 0) {
        tree->egtb_hits++;
        result = TB_GET_WDL(tb_result);
        switch (result) {
          case TB_LOSS:
            value = -TBWIN;
            break;
          case TB_WIN:
            value = TBWIN;
            break;
          case TB_BLESSED_LOSS:
            value = -3;
            break;
          case TB_DRAW:
            value = 0;
            break;
          case TB_CURSED_WIN:
            value = 3;
            break;
          default:
            value = TB_GET_DTZ(tb_result);;
            break;
        }
        if (result != TB_LOSS && result != TB_WIN) {
          if (MaterialSTM(wtm) > 0)
            value += 1;
          else if (MaterialSTM(wtm) < 0)
            value -= 1;
        }
        piece = abs(PcOnSq(TB_GET_FROM(tb_result)));
        captured = abs(PcOnSq(TB_GET_TO(tb_result)));
        cmove =
            TB_GET_FROM(tb_result) | (TB_GET_TO(tb_result) << 6) | (piece <<
            12) | (captured << 15);
        if (TB_GET_PROMOTES(tb_result))
          cmove |= (6 - TB_GET_PROMOTES(tb_result)) << 18;
        end_time = ReadClock();
        tree->pv[0].path[1] = cmove;
        tree->pv[0].pathl = 2;
        tree->pv[0].pathh = 4;
        tree->pv[0].pathd = 0;
        tree->pv[0].pathv = value;
        MakeMove(tree, 1, wtm, cmove);
        result = Mated(tree, 2, Flip(wtm));
        UnmakeMove(tree, 1, wtm, cmove);
        if (result == 1)
          tree->pv[0].pathv = MATE - 2;
        else if (result == 2)
          tree->pv[0].pathv = DrawScore(wtm);
/*
 ************************************************************
 *                                                          *
 *  If we are not mated and did not mate on the move, we    *
 *  flip the side on move and find the best TB move so that *
 *  we can show the expected reply in the PV.               *
 *                                                          *
 ************************************************************
 */
        else {
          MakeMove(tree, 1, wtm, cmove);
          tree->egtb_probes++;
          tb_result =
              tb_probe_root(Occupied(white), Occupied(black),
              Kings(white) | Kings(black), Queens(white) | Queens(black),
              Rooks(white) | Rooks(black), Bishops(white) | Bishops(black),
              Knights(white) | Knights(black), Pawns(white) | Pawns(black),
              Reversible(2), 0, EnPassant(2), Flip(wtm), NULL);
          if (tb_result != TB_RESULT_FAILED) {
            tree->egtb_hits++;
            piece = abs(PcOnSq(TB_GET_FROM(tb_result)));
            captured = abs(PcOnSq(TB_GET_TO(tb_result)));
            omove =
                TB_GET_FROM(tb_result) | (TB_GET_TO(tb_result) << 6) | (piece
                << 12) | (captured << 15);
            if (TB_GET_PROMOTES(tb_result))
              omove |= (6 - TB_GET_PROMOTES(tb_result)) << 18;
            end_time = ReadClock();
            tree->pv[0].path[2] = omove;
            tree->pv[0].pathl = 3;
          }
          UnmakeMove(tree, 1, wtm, cmove);
        }
      }
/*
 ************************************************************
 *                                                          *
 *  We now know the best move to play, and possibly the     *
 *  opponent's best response.  Display this info and then   *
 *  we wait for the next move to pop in.                    *
 *                                                          *
 ************************************************************
 */
      Print(2, "        depth     time       score   variation\n");
      if (n_root_moves == 0) {
        program_end_time = ReadClock();
        tree->pv[0].pathl = 0;
        tree->pv[0].pathd = 0;
        if (Check(wtm))
          value = -(MATE - 1);
        else
          value = DrawScore(wtm);
        Print(2, "                             Mated   (no moves)\n");
        tree->nodes_searched = 1;
        if (!puzzling)
          last_root_value = value;
        return 1;
      }
      DisplayPV(tree, 5, wtm, end_time - start_time, &tree->pv[0], 1);
      return 1;
    }
  }
#endif
  return 0;
}

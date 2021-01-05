#include "chess.h"
#include "data.h"
/* last modified 08/15/15 */
/*
 *******************************************************************************
 *                                                                             *
 *   History() is used to maintain the two killer moves for each ply.  The     *
 *   most recently used killer is always first in the list.                    *
 *                                                                             *
 *   History() also maintains two counter-moves.  These are moves that are     *
 *   directly used to refute a specific move at the previous ply, rather than  *
 *   just a plain killer that has caused cutoffs previously.                   *
 *                                                                             *
 *   History() finally remembers two moves that were played after a specific   *
 *   move at ply-2 (ie the two moves together create some sort of "plan".)     *
 *                                                                             *
 *   History() also maintains the history counters.  Each time a move fails    *
 *   high, it's history count is increased, while all other moves that were    *
 *   searched will have their counts reduced.  The current counter is a        *
 *   saturating counter in the range 0 <= N <= 2047.                           *
 *                                                                             *
 *******************************************************************************
 */
void History(TREE * RESTRICT tree, int ply, int depth, int side, int move,
    int searched[]) {
  int i, index, mindex;
/*
 ************************************************************
 *                                                          *
 *  Now, add this move to the current killer moves if it is *
 *  not already there.  If the move is already first in the *
 *  list, leave it there, otherwise move the first one down *
 *  to slot two and insert this move into slot one.         *
 *                                                          *
 *  If the best move so far is a capture or a promotion,    *
 *  we skip updating the killers (but still update history  *
 *  information) since we search good captures first, which *
 *  happens before killers are tried, making capture moves  *
 *  useless here.                                           *
 *                                                          *
 *  The update value does not depend on depth.  I used a    *
 *  function of depth for years, but as I examined more and *
 *  more trees, that seemed to be lacking because it gives  *
 *  a really large bias towards moves that work near the    *
 *  root, while most of the nodes searched are near the     *
 *  tips.  One unusual characteristic of this counter is    *
 *  that it is a software-based saturating counter.  That   *
 *  is, it can never exceed 2047, nor can it ever drop      *
 *  below zero.                                             *
 *                                                          *
 ************************************************************
 */
  if (!CaptureOrPromote(move)) {
    if (tree->killers[ply].move1 != move) {
      tree->killers[ply].move2 = tree->killers[ply].move1;
      tree->killers[ply].move1 = move;
    }
/*
 ************************************************************
 *                                                          *
 *  Set the counter-move for the move at the previous ply   *
 *  to be this move that caused the fail-high.              *
 *                                                          *
 ************************************************************
 */
    if (tree->counter_move[tree->curmv[ply - 1] & 4095].move1 != move) {
      tree->counter_move[tree->curmv[ply - 1] & 4095].move2 =
          tree->counter_move[tree->curmv[ply - 1] & 4095].move1;
      tree->counter_move[tree->curmv[ply - 1] & 4095].move1 = move;
    }
/*
 ************************************************************
 *                                                          *
 *  Set the move-pair for the move two plies back so that   *
 *  if we play that move again, we will follow up with this *
 *  move to continue the "plan".                            *
 *                                                          *
 ************************************************************
 */
    if (ply > 2) {
      if (tree->move_pair[tree->curmv[ply - 2] & 4095].move1 != move) {
        tree->move_pair[tree->curmv[ply - 2] & 4095].move2 =
            tree->move_pair[tree->curmv[ply - 2] & 4095].move1;
        tree->move_pair[tree->curmv[ply - 2] & 4095].move1 = move;
      }
    }
/*
 ************************************************************
 *                                                          *
 *  Adjust the history counter for the move that caused the *
 *  fail-high, limiting the max value to 2048.              *
 *                                                          *
 ************************************************************
 */
    if (depth > 5) {
      mindex = HistoryIndex(side, move);
      history[mindex] += (2048 - history[mindex]) >> 5;
/*
 ************************************************************
 *                                                          *
 *  Adjust the history counters for the moves that were     *
 *  searched but did not cause a fail-high, limiting the    *
 *  min value to 0.                                         *
 *                                                          *
 ************************************************************
 */
      for (i = 1; i <= searched[0]; i++) {
        index = HistoryIndex(side, searched[i]);
        if (index != mindex)
          history[index] -= history[index] >> 5;
      }
    }
  }
}

#include "chess.h"
#include "data.h"
#if defined(SYZYGY)
#  include "tbprobe.h"
#endif
/* last modified 08/03/16 */
/*
 *******************************************************************************
 *                                                                             *
 *   Search() is the recursive routine used to implement the alpha/beta        *
 *   negamax search (similar to minimax but simpler to code.)  Search() is     *
 *   called whenever there is "depth" remaining so that all moves are subject  *
 *   to searching.  Search() recursively calls itself so long as there is at   *
 *   least one ply of depth left, otherwise it calls Quiesce() instead.        *
 *                                                                             *
 *******************************************************************************
 */
int Search(TREE * RESTRICT tree, int ply, int depth, int wtm, int alpha,
    int beta, int in_check, int do_null) {
  int repeat = 0, value = 0, pv_node = alpha != beta - 1, n_depth;
  int searched[256];

/*
 ************************************************************
 *                                                          *
 *  Timeout.  Check to see if we have searched enough nodes *
 *  that it is time to peek at how much time has been used, *
 *  or if is time to check for operator keyboard input.     *
 *  This is usually enough nodes to force a time/input      *
 *  check about once per second, except when the target     *
 *  time per move is very small, in which case we try to    *
 *  check the time more frequently.                         *
 *                                                          *
 *  Note that we check input or time-out in thread 0.  This *
 *  makes the code simpler and eliminates some problematic  *
 *  race conditions.                                        *
 *                                                          *
 ************************************************************
 */
#if defined(NODES)
  if (search_nodes && --temp_search_nodes <= 0) {
    abort_search = 1;
    return 0;
  }
#endif
  if (tree->thread_id == 0) {
    if (--next_time_check <= 0) {
      next_time_check = nodes_between_time_checks;
      if (TimeCheck(tree, 1)) {
        abort_search = 1;
        return 0;
      }
      if (CheckInput()) {
        Interrupt(ply);
        if (abort_search)
          return 0;
      }
    }
  }
  if (ply >= MAXPLY - 1)
    return beta;
/*
 ************************************************************
 *                                                          *
 *  Draws.  Check for draw by repetition, which includes    *
 *  50 move draws also.  This is the quickest way to get    *
 *  out of further searching, with minimal effort.  This    *
 *  and the next four steps are skipped for moves at the    *
 *  root (ply = 1).                                         *
 *                                                          *
 ************************************************************
 */
  if (ply > 1) {
    if ((repeat = Repeat(tree, ply))) {
      if (repeat == 2 || !in_check) {
        value = DrawScore(wtm);
        if (value < beta)
          SavePV(tree, ply, repeat);
#if defined(TRACE)
        if (ply <= trace_level)
          printf("draw by %s detected, ply=%d.\n",
              (repeat == 3) ? "50-move" : "repetition", ply);
#endif
        return value;
      }
    }
/*
 ************************************************************
 *                                                          *
 *  Mate distance pruning.  If we have found a mate, we can *
 *  stop if we are too deep to find a shorter mate.  This   *
 *  only affects the size of the tree in positions where    *
 *  there are forced mates.  It does not change the outcome *
 *  of the search at all, just the time it takes.           *
 *                                                          *
 ************************************************************
 */
    alpha = Max(alpha, -MATE + ply - 1);
    beta = Min(beta, MATE - ply);
    if (alpha >= beta)
      return alpha;
/*
 ************************************************************
 *                                                          *
 *  Trans/Ref.  Check the transposition/refutation (hash)   *
 *  table to see if we have searched this position          *
 *  previously and still have the results available.  We    *
 *  might get a real score, or a bound, or perhaps only a   *
 *  good move to try first.  The possible results are:      *
 *                                                          *
 *  1. HashProbe() returns "HASH_HIT".  This terminates the *
 *  search instantly and we simply return the value found   *
 *  in the hash table.  This value is simply the value we   *
 *  found when we did a real search in this position        *
 *  previously, and ProbeTransRef() verifies that the value *
 *  is useful based on draft and current bounds.            *
 *                                                          *
 *  2. HashProbe() returns "AVOID_NULL_MOVE" which means    *
 *  the hashed score/bound was no good, but it indicated    *
 *  that trying a null-move in this position would be a     *
 *  waste of time since it will likely fail low, not high.  *
 *                                                          *
 *  3. HashProbe() returns "HASH_MISS" when forces us to do *
 *  a normal search to resolve this node.                   *
 *                                                          *
 ************************************************************
 */
    switch (HashProbe(tree, ply, depth, wtm, alpha, beta, &value)) {
      case HASH_HIT:
        return value;
      case AVOID_NULL_MOVE:
        do_null = 0;
      case HASH_MISS:
        break;
    }
/*
 ************************************************************
 *                                                          *
 *  EGTBs.  Now it's time to try a probe into the endgame   *
 *  tablebase files.  This is done if we notice that there  *
 *  are 6 or fewer pieces left on the board AND the 50 move *
 *  counter is zero which enables probing the WDL EGTBs     *
 *  correctly.  Probing after a capture won't work as it is *
 *  possible that there is a necessary pawn push here and   *
 *  there to reset the 50 move counter, otherwise we could  *
 *  think we were following a winning path but heading to a *
 *  draw.                                                   *
 *                                                          *
 *  This is another way to get out of the search quickly,   *
 *  but not as quickly as the previous steps since this can *
 *  result in an I/O operation.                             *
 *                                                          *
 *  Note that in "swindle mode" this can be turned off by   *
 *  Iterate() setting "EGTB_use = 0" so that we won't probe *
 *  the EGTBs since we are searching only the root moves    *
 *  that lead to a draw and we want to play the move that   *
 *  makes the draw more difficult to reach by the opponent  *
 *  to give him a chance to make a mistake.                 *
 *                                                          *
 *  Another special case is that we slightly fudge the      *
 *  score for draws.  The scores are -0.03 for a "blessed   *
 *  loss", 0.0 for a pure draw, and +0.03 for a "cursed     *
 *  win".  These are then modified by adding 0.01 if the    *
 *  side on move is ahead in material, and subtracting 0.01 *
 *  if the side on move is behind material.  This creates   *
 *  the following inequality:                               *
 *                                                          *
 *     BL- < BL= < BL+ < D- < D= < D+ < CW- < CW= <CW+      *
 *                                                          *
 *  Where BL=blessed loss, D = draw, and CW = cursed win,   *
 *  and - means behind in material, = means equal material, *
 *  and + means ahead in material.                          *
 *                                                          *
 ************************************************************
 */
#if defined(SYZYGY)
    if (depth > EGTB_depth && TotalAllPieces <= EGTB_use &&
        !Castle(ply, white) && !Castle(ply, black) && Reversible(ply) == 0) {
      int tb_result;

      tree->egtb_probes++;
      tb_result =
          tb_probe_wdl(Occupied(white), Occupied(black),
          Kings(white) | Kings(black), Queens(white) | Queens(black),
          Rooks(white) | Rooks(black), Bishops(white) | Bishops(black),
          Knights(white) | Knights(black), Pawns(white) | Pawns(black),
          Reversible(ply), 0, EnPassant(ply), wtm, HashKey);
      if (tb_result != TB_RESULT_FAILED) {
        tree->egtb_hits++;
        switch (tb_result) {
          case TB_LOSS:
            alpha = -TBWIN;
            break;
          case TB_BLESSED_LOSS:
            alpha = -3;
            break;
          case TB_DRAW:
            alpha = 0;
            break;
          case TB_CURSED_WIN:
            alpha = 3;
            break;
          case TB_WIN:
            alpha = TBWIN;
            break;
        }
        if (tb_result != TB_LOSS && tb_result != TB_WIN) {
          if (MaterialSTM(wtm) > 0)
            alpha += 1;
          else if (MaterialSTM(wtm) < 0)
            alpha -= 1;
        }
        if (alpha < beta)
          SavePV(tree, ply, 4);
        return alpha;
      }
    }
#endif
/*
 ************************************************************
 *                                                          *
 *  Null-move.  We now know there is no quick way to get    *
 *  out of here, which leaves one more possibility,         *
 *  although it does require a search, but to a reduced     *
 *  depth.  We try a null move to see if we can get a quick *
 *  cutoff with only a little work.  This operates as       *
 *  follows.  Instead of making a legal move, the side on   *
 *  move passes and does nothing.  The resulting position   *
 *  is searched to a shallower depth than normal (see       *
 *  below).  This will result in a cutoff if our position   *
 *  is very good, but it produces the cutoff much quicker   *
 *  since the search is far shallower than a normal search  *
 *  that would also be likely to fail high.                 *
 *                                                          *
 *  The reduction amount starts off at null_depth (normally *
 *  set to 3 but the user can change this via the crafty    *
 *  personality command) but is then increased as follows:  *
 *                                                          *
 *     R = null_depth + depth / null_divisor                *
 *                                                          *
 *  null_divisor defaults to 6, but this can also be set    *
 *  by the user to try more/less aggressive settings.       *
 *                                                          *
 *  This is skipped for any of the following reasons:       *
 *                                                          *
 *  1. The side on move is in check.  The null move         *
 *     results in an illegal position.                      *
 *  2. No more than one null move can appear in succession  *
 *     as all this does is burn 2x plies of depth.          *
 *  3. The side on move has only pawns left, which makes    *
 *     zugzwang positions more likely.                      *
 *  4. The transposition table probe found an entry that    *
 *     indicates that a null-move search will not fail      *
 *     high, so we avoid the wasted effort.                 *
 *                                                          *
 ************************************************************
 */

    tree->last[ply] = tree->last[ply - 1];
    n_depth = (TotalPieces(wtm, occupied) > 9 || n_root_moves > 17 ||
        depth > 3) ? 1 : 3;
    if (do_null && !pv_node && depth > n_depth && !in_check &&
        TotalPieces(wtm, occupied)) {
      uint64_t save_hash_key;
      int R = null_depth + depth / null_divisor;

      tree->curmv[ply] = 0;
#if defined(TRACE)
      if (ply <= trace_level)
        Trace(tree, ply, depth, wtm, value - 1, value, "SearchNull", serial,
            NULL_MOVE, 0);
#endif
      tree->status[ply + 1] = tree->status[ply];
      Reversible(ply + 1) = 0;
      save_hash_key = HashKey;
      if (EnPassant(ply + 1)) {
        HashEP(EnPassant(ply + 1));
        EnPassant(ply + 1) = 0;
      }
      tree->null_done[Min(R, 15)]++;
      if (depth - R - 1 > 0)
        value =
            -Search(tree, ply + 1, depth - R - 1, Flip(wtm), -beta, -beta + 1,
            0, NO_NULL);
      else
        value = -Quiesce(tree, ply + 1, Flip(wtm), -beta, -beta + 1, 1);
      HashKey = save_hash_key;
      if (abort_search || tree->stop)
        return 0;
      if (value >= beta) {
        HashStore(tree, ply, depth, wtm, LOWER, value, tree->hash_move[ply]);
        return value;
      }
    }
/*
 ************************************************************
 *                                                          *
 *  IID.  This step is rarely executed.  It is used when    *
 *  there is no best move from the hash table, and this is  *
 *  a PV node, since we need a good move to search first.   *
 *  While killers moves are good, they are not quite good   *
 *  enough.  the simplest solution is to try a shallow      *
 *  search (depth-2) to get a move.  note that when we call *
 *  Search() with depth-2, it, too, will not have a hash    *
 *  move, and will therefore recursively continue this      *
 *  process, hence the name "internal iterative deepening." *
 *                                                          *
 ************************************************************
 */
    tree->next_status[ply].phase = HASH;
    if (!tree->hash_move[ply] && depth >= 6 && do_null && ply > 1 && pv_node) {
      tree->curmv[ply] = 0;
      if (depth - 2 > 0)
        value =
            Search(tree, ply, depth - 2, wtm, alpha, beta, in_check, DO_NULL);
      else
        value = Quiesce(tree, ply, wtm, alpha, beta, 1);
      if (abort_search || tree->stop)
        return 0;
      if (value > alpha) {
        if (value < beta) {
          if ((int) tree->pv[ply - 1].pathl > ply)
            tree->hash_move[ply] = tree->pv[ply - 1].path[ply];
        } else
          tree->hash_move[ply] = tree->curmv[ply];
        tree->last[ply] = tree->last[ply - 1];
        tree->next_status[ply].phase = HASH;
      }
    }
  }
/*
 ************************************************************
 *                                                          *
 *  Search moves.  Now we call SearchMoveList() to interate *
 *  through the move list and search each new position.     *
 *  Note that this is done in a separate procedure because  *
 *  this is also the code that is used to do the parallel   *
 *  search.                                                 *
 *                                                          *
 ************************************************************
 */
  searched[0] = 0;
  value =
      SearchMoveList(tree, ply, depth, wtm, alpha, beta, searched, in_check,
      repeat, serial);
  return value;
}

/* last modified 08/03/16 */
/*
 *******************************************************************************
 *                                                                             *
 *   SearchMoveList() is the recursive routine used to implement the main      *
 *   search loop.  This code is responsible for iterating through the move     *
 *   list until it encounters a condition that ends the search at this ply.    *
 *   At that point it simply returns the current negamax value to the caller   *
 *   to handle as necessary.                                                   *
 *                                                                             *
 *   The "mode" flag indicates which of the following conditions apply here    *
 *   which directly controls parts of the search.                              *
 *                                                                             *
 *      mode = serial   ->  this is a serial search.                           *
 *                                                                             *
 *      mode = parallel ->  this is a parallel search, which implies that this *
 *                          is a partial search which means we do NOT want to  *
 *                          do any trans/ref updating and we also need to take *
 *                          care about locking things that are being updated   *
 *                          by more than one thread in parallel.               *
 *                                                                             *
 *   When mode = parallel, this code performs the same function as the old     *
 *   SearchParallel() code, except that it is the main search loop for the     *
 *   program, there is no longer any duplicated code.  This is called by the   *
 *   normal Search() function and by ThreadWait() where idle processes wait    *
 *   for work and then call this procedure to search a subset of the moves at  *
 *   this ply (in parallel with other threads).                                *
 *                                                                             *
 *******************************************************************************
 */
int SearchMoveList(TREE * RESTRICT tree, int ply, int depth, int wtm,
    int alpha, int beta, int searched[], int in_check, int repeat, int mode) {
  TREE *current;
  int extend, reduce, check, original_alpha = alpha, t_beta;
  int i, j, value = 0, pv_node = alpha != beta - 1, search_result, order;
  int moves_done = 0, bestmove, type;

/*
 ************************************************************
 *                                                          *
 *  Basic initialization before we begin the loop through   *
 *  the move list.  If this is a parallel search, we have   *
 *  already searched one move, so we set t_beta to alpha+1  *
 *  to set up for a normal PVS search (for moves 2-n)       *
 *  instead of using alpha,beta for the first move as we do *
 *  in a normal search.  Also, if this is a serial search,  *
 *  we are fixing to search the first move so we set the    *
 *  searched move counter to zero, where in a parallel      *
 *  search this has already been done and we leave it alone *
 *  here.                                                   *
 *                                                          *
 *  We also set <current> to tree for a serial search, and  *
 *  to tree->parent for a parallel search since we need to  *
 *  share the move list at split nodes.                     *
 *                                                          *
 ************************************************************
 */
  tree->next_status[ply].phase = HASH;
  if (mode == parallel) {
    current = tree->parent;
    t_beta = alpha + 1;
  } else {
    current = tree;
    t_beta = beta;
  }
/*
 ************************************************************
 *                                                          *
 *  Iterate.  Now iterate through the move list and search  *
 *  the resulting positions.  Note that Search() culls any  *
 *  move that is not legal by using Check().  The special   *
 *  case is that we must find one legal move to search to   *
 *  confirm that it's not a mate or draw.                   *
 *                                                          *
 *  We call NextMove() which will generate moves in the     *
 *  normal way (captures, killers, etc) or it will use the  *
 *  GenerateEvasions() generator if we are in check.  For   *
 *  the special case of ply=1, we use NextRootMove() since  *
 *  the ply=1 move list has been generated and the order is *
 *  updated as each search iteration is executed.           *
 *                                                          *
 ************************************************************
 */
  while (1) {
    if (ply == 1 && moves_done == 1 && alpha == original_alpha &&
        mode == serial)
      break;
    if (mode == parallel)
      Lock(current->lock);
    order = (ply > 1) ? NextMove(current, ply, depth, wtm, in_check)
        : NextRootMove(current, tree, wtm);
    if (mode == parallel) {
      tree->curmv[ply] = current->curmv[ply];
      Unlock(current->lock);
    }
    if (!order)
      break;
#if defined(TRACE)
    if (ply <= trace_level)
      Trace(tree, ply, depth, wtm, alpha, beta, "SearchMoveList", mode,
          current->phase[ply], order);
#endif
    MakeMove(tree, ply, wtm, tree->curmv[ply]);
    tree->nodes_searched++;
    search_result = ILLEGAL;
    if (in_check || !Check(wtm))
      do {
        searched[0]++;
        moves_done++;
        search_result = LEGAL;
        searched[searched[0]] = tree->curmv[ply];
/*
 ************************************************************
 *                                                          *
 *  Check.  If the move to be made checks the opponent,     *
 *  then we need to remember that he's in check and also    *
 *  extend the depth by one ply for him to get out.         *
 *                                                          *
 *  We do not extend unsafe checking moves (as indicated by *
 *  the SEE algorithm), since these are usually a waste of  *
 *  time and simply blow up the tree search space.          *
 *                                                          *
 *  Note that extending here disables any potential foward  *
 *  pruning or reductions for this move.                    *
 *                                                          *
 ************************************************************
 */
        extend = 0;
        reduce = 0;
        if (Check(Flip(wtm))) {
          check = 1;
          if (SEEO(tree, wtm,
                  tree->curmv[ply]) - pcval[Captured(tree->curmv[ply])] <=
              0) {
            extend = check_depth;
            tree->extensions_done++;
          }
        } else
          check = 0;
/*
 ************************************************************
 *                                                          *
 *  Futility.  First attempt at forward pruning based on    *
 *  the futility idea.                                      *
 *                                                          *
 *  We have an array of pruning margin values that are      *
 *  indexed by depth (remaining plies left until we drop    *
 *  into the quiescence search) and which increase with     *
 *  depth since more depth means a greater chance of        *
 *  bringing the score back up to alpha or beyond.  If the  *
 *  current material + the bonus is less than alpha, we     *
 *  simply avoid searching this move at all, and skip to    *
 *  the next move without expending any more effort.  Note  *
 *  that this is classic forward-pruning and can certainly  *
 *  introduce errors into the search.  However, cluster     *
 *  testing has shown that this improves play in real       *
 *  games.  The current implementation only prunes in the   *
 *  last 6 plies before quiescence, although this can be    *
 *  tuned with the "eval" command changing the "pruning     *
 *  depth" value to something other than 7 (test is for     *
 *  depth < pruning depth, current value is 7 which prunes  *
 *  in last 6 plies only).  Testing shows no benefit in     *
 *  larger values than 7, although this might change in     *
 *  future versions as other things are modified.           *
 *                                                          *
 *  Exception:                                              *
 *                                                          *
 *    We do not prune if we are safely pushing a passed     *
 *    pawn to the 6th rank, where it becomes very dangerous *
 *    since it can promote in two more moves.               *
 *                                                          *
 *  All pruning and reduction code is skipped if any of the *
 *  following are true:                                     *
 *                                                          *
 *  (1) side on move is in check.                           *
 *                                                          *
 *  (2) move has not already been flagged for a search      *
 *      extension.                                          *
 *                                                          *
 *  (3) this is not the first move at this ply.             *
 *                                                          *
 ************************************************************
 */
        if (!in_check && (!extend || !pv_node) && order > 1 &&
            !(PawnPush(wtm, tree->curmv[ply]))) {
          if (depth < FP_depth && !check &&
              MaterialSTM(wtm) + FP_margin[depth] <= alpha && !pv_node) {
            tree->moves_fpruned++;
            break;
          }
/*
 ************************************************************
 *                                                          *
 *  LMP.  Final attempt at forward pruning based on the     *
 *  "late move pruning" idea (a take-off on LMR).           *
 *                                                          *
 *  The basic idea here is that once we have searched a     *
 *  significant number of moves at a ply, it becomes less   *
 *  and less likely that any of the moves left will produce *
 *  a cutoff.  If the move appears to be simple (not a      *
 *  check, etc) then we simply skip it, once the move count *
 *  has been satisfied.  At present, we only do this in the *
 *  last 16 plies although this might be changed in the     *
 *  future.  If you look at the LMP array after it has been *
 *  initialized, you will notice that it is unlikely that   *
 *  LMP can be triggered much beyond depth 8 as you have to *
 *  have a BUNCH of moves to search to reach those limits.  *
 *                                                          *
 ************************************************************
 */
          if (order > LMP[depth] && depth < LMP_depth && !pv_node && !check &&
              alpha > -MATE + 300 && !CaptureOrPromote(tree->curmv[ply])) {
            tree->moves_mpruned++;
            break;
          }
/*
 ************************************************************
 *                                                          *
 *  LMR.  Now it's time to try to reduce the search depth   *
 *  if the move appears to be "poor" because it appears     *
 *  later in the move list.                                 *
 *                                                          *
 *  The reduction is variable and is done via a table look- *
 *  up that uses a function based on remaining depth (more  *
 *  depth remaining, the larger the reduction) and the      *
 *  number of moves searched (more moves searched, the      *
 *  larger the reduction).  The "shape" of this reduction   *
 *  formula is user-settable via the "lmr" command.         *
 *                                                          *
 ************************************************************
 */
          reduce = LMR[Min(depth, 31)][Min(order, 63)];
          if (reduce && (pv_node || extend))
            reduce--;
          tree->LMR_done[reduce]++;
        }
/*
 ************************************************************
 *                                                          *
 *  Now do the PVS search on the current move.              *
 *                                                          *
 *  Note that we do the usual alpha/beta cutoff tests here  *
 *  but we only set an indicator that is used after we have *
 *  called Unmake().  This cleaned up the exit from search  *
 *  and makes it easier to understand when there is only    *
 *  one point where this is done, without needing multiple  *
 *  Unmake() calls when there are different exit points.    *
 *                                                          *
 **************************************************************
 */
        value =
            SearchMove(tree, ply, depth, wtm, alpha, t_beta, beta, extend,
            reduce, check);
        if (value > alpha) {
          search_result = IN_WINDOW;
          if (value >= beta)
            search_result = FAIL_HIGH;
          if (mode == parallel && ply == 1)
            search_result = FAIL_HIGH;
        }
      } while (0);
    UnmakeMove(tree, ply, wtm, tree->curmv[ply]);
    if (abort_search || tree->stop)
      break;
/*
 ************************************************************
 *                                                          *
 *  Test 1.  When we get here, we have made a move,         *
 *  searched it (and re-searched if necessary/appropriate), *
 *  and the move has been unmade so that the board is in a  *
 *  correct state.                                          *
 *                                                          *
 *  If search_result = FAIL_HIGH, the search failed high.   *
 *  The first thing to handle is the case where we are at   *
 *  ply=1, which is a special case.  If we are going to     *
 *  fail high here and terminate the search immediately, we *
 *  need to build the fail-high PV to back up to Iterate()  *
 *  so it will produce the correct output and widen the     *
 *  alpha/beta window.                                      *
 *                                                          *
 *  We then check to see if this is a parallel search.  If  *
 *  so then we are done here, but we need to tell all of    *
 *  the siblings that are helping at this split point that  *
 *  they should immediately stop searching here since we    *
 *  don't need their results.                               *
 *                                                          *
 *  Otherwise we update the killer moves and history        *
 *  counters and store the fail-high information in the     *
 *  trans/ref table for future use if we happen to reach    *
 *  this position again.                                    *
 *                                                          *
 ************************************************************
 */
    if (search_result == FAIL_HIGH) {
      if (ply == 1) {
        if (!tree->stop) {
          tree->pv[1].path[1] = tree->curmv[1];
          tree->pv[1].pathl = 2;
          tree->pv[1].pathh = 0;
          tree->pv[1].pathd = iteration;
          tree->pv[0] = tree->pv[1];
        }
      }
#if (CPUS > 1)
      if (mode == parallel) {
        Lock(lock_smp);
        Lock(tree->parent->lock);
        if (!tree->stop) {
          int proc;

          parallel_aborts++;
          for (proc = 0; proc < smp_max_threads; proc++)
            if (tree->parent->siblings[proc] && proc != tree->thread_id)
              ThreadStop(tree->parent->siblings[proc]);
        }
        Unlock(tree->parent->lock);
        Unlock(lock_smp);
        return beta;
      }
#endif
      tree->fail_highs++;
      if (order == 1)
        tree->fail_high_first_move++;
      HashStore(tree, ply, depth, wtm, LOWER, value, tree->curmv[ply]);
      History(tree, ply, depth, wtm, tree->curmv[ply], searched);
      return beta;
/*
 ************************************************************
 *                                                          *
 *  Test 2.  If search_result = IN_WINDOW, this is a search *
 *  that improved alpha without failing high.  We simply    *
 *  update alpha and continue searching moves.              *
 *                                                          *
 *  Special case:  If ply = 1 in a normal search, we have   *
 *  a best move and score that just changed.  We need to    *
 *  update the root move list by adding the PV and the      *
 *  score, and then we look to make sure this new "best     *
 *  move" is not actually worse than the best we have found *
 *  so far this iteration.  If it is worse, we restore the  *
 *  best move and score from the real best move so our      *
 *  search window won't be out of whack, which would let    *
 *  moves with scores in between this bad move and the best *
 *  move fail high, cause re-searches, and waste time.  We  *
 *  also need to restore the root move list so that the     *
 *  best move (the one we just used to replace the move     *
 *  with a worse score) is first so it is searched first on *
 *  the next iteration.                                     *
 *                                                          *
 *  If this is ply = 1, we display the PV to keep the user  *
 *  informed.                                               *
 *                                                          *
 ************************************************************
 */
    } else if (search_result == IN_WINDOW) {
      alpha = value;
      if (ply == 1 && mode == serial) {
        int best;

       //
       // update path/score for this move
       //
        tree->pv[1].pathv = value;
        tree->pv[0] = tree->pv[1];
        for (best = 0; best < n_root_moves; best++)
          if (root_moves[best].move == tree->pv[1].path[1]) {
            root_moves[best].path = tree->pv[1];
            root_moves[best].path.pathv = alpha;
            break;
          }
       //
       // if this move is not #1 in root list, move it there
       //
        if (best != 0) {
          ROOT_MOVE t;
          t = root_moves[best];
          for (i = best; i > 0; i--)
            root_moves[i] = root_moves[i - 1];
          root_moves[0] = t;
        }
       //
       // if a better score has already been found then move that
       // move to the front of the list and update alpha bound.
       //
        for (i = 0; i < n_root_moves; i++) {
          if (value <= root_moves[i].path.pathv) {
            ROOT_MOVE t;
            value = root_moves[i].path.pathv;
            alpha = value;
            tree->pv[0] = root_moves[i].path;
            tree->pv[1] = tree->pv[0];
            t = root_moves[i];
            for (j = i; j > 0; j--)
              root_moves[j] = root_moves[j - 1];
            root_moves[0] = t;
          }
        }
        Output(tree);
        failhi_delta = 16;
        faillo_delta = 16;
      }
    }
/*
 ************************************************************
 *                                                          *
 *  Test 3.  If search_result = ILLEGAL, this search was    *
 *  given an illegal move and no search was done, we skip   *
 *  any updating and simply select the next move to search. *
 *                                                          *
 ************************************************************
 */
    else if (search_result == ILLEGAL)
      continue;
    t_beta = alpha + 1;
/*
 ************************************************************
 *                                                          *
 *  SMP.  If are doing an SMP search, and we have idle      *
 *  processors, now is the time to get them involved.  We   *
 *  have now satisfied the "young brothers wait" condition  *
 *  since we have searched at least one move.  All that is  *
 *  left is to check the split constraints to see if this   *
 *  is an acceptable split point.                           *
 *                                                          *
 *    (1) We can't split within N plies of the frontier     *
 *        nodes to avoid excessive split overhead.          *
 *                                                          *
 *    (2) We can't split until at least N nodes have been   *
 *        searched since this thread was last split, to     *
 *        avoid splitting too often, mainly in endgames.    *
 *                                                          *
 *    (3) We have to have searched one legal move to avoid  *
 *        splitting at a node where we have no legal moves  *
 *        (the first move tried might have been illegal as  *
 *        in when we encounter a stalemate).                *
 *                                                          *
 *    (4) If we are at ply=1, we can't split unless the     *
 *        smp_split_at_root flag is set to 1, AND the next  *
 *        move in the ply=1 move list is not flagged as     *
 *        "do not search in parallel" which happens when    *
 *        this move was a best move in the last couple of   *
 *        searches and we want all processors on it at once *
 *        to get a score back quicker.                      *
 *                                                          *
 *    (5) if the variable smp_split is != 0, we have idle   *
 *        threads that can help, which means we want to get *
 *        them involved quickly, OR if this node is an      *
 *        acceptable "gratuitous-split" point by being far  *
 *        enough from the tips of the tree to avoid         *
 *        excessive overhead.                               *
 *                                                          *
 *  We use this code recursively to perform a parallel      *
 *  search at this ply.  But when we finish a partial piece *
 *  of the search in parallel, we don't need to update any  *
 *  search data structures, we will defer that until all of *
 *  parallel threads complete and return back into this     *
 *  code after the parallel search has been collapsed back  *
 *  to one instance of search at this ply.                  *
 *                                                          *
 *  Special case:  we do not split if we are at ply=1 and   *
 *  alpha == original_alpha.  That means the first move     *
 *  failed low, and we are going to exit search and return  *
 *  to Iterate() to report this.                            *
 *                                                          *
 *  In Generation II, multiple threads can reach this point *
 *  at the same time.  We allow multiple threads to split   *
 *  at the same time, but then the idle threads will choose *
 *  to join the thread with the most attractive split point *
 *  rather than just taking pot-luck.  The only limitation  *
 *  on a thread adding a split point here is that if the    *
 *  thread already has enough joinable split points that    *
 *  have not been joined yet, we do not incur the overhead  *
 *  of creating another split point until one of the        *
 *  existing split points has been completed or a thread    *
 *  joins at at one of those available split points.        *
 *                                                          *
 *  We do not lock anything here, as the split operation    *
 *  only affects thread-local data.  When the split is done *
 *  then the ThreadJoin() function will acquire the lock    *
 *  needed to avoid race conditions during the join op-     *
 *  eration.                                                *
 *                                                          *
 ************************************************************
 */
#if (CPUS > 1)
    if (mode == serial && moves_done && smp_threads &&
        ThreadSplit(tree, ply, depth, alpha, original_alpha, moves_done))
      do {
        tree->alpha = alpha;
        tree->beta = beta;
        tree->value = alpha;
        tree->wtm = wtm;
        tree->ply = ply;
        tree->depth = depth;
        tree->in_check = in_check;
        tree->searched = searched;
        if (Split(tree)) {
          if (abort_search || tree->stop)
            return 0;
          value = tree->value;
          if (value > alpha) {
            if (ply == 1)
              tree->pv[0] = tree->pv[1];
            if (value >= beta) {
              HashStore(tree, ply, depth, wtm, LOWER, value, tree->cutmove);
              return value;
            }
            alpha = value;
            break;
          }
        }
      } while (0);
#endif
  }
/*
 ************************************************************
 *                                                          *
 *  SMP Cleanup.  If we are doing an SMP search, there are  *
 *  no "end-of-search" things to do.  We have searched all  *
 *  the remaining moves at this ply in parallel, and now    *
 *  return and let the original search that started this    *
 *  sub-tree clean up, do the tests for mate/stalemate,     *
 *  update the hash table, etc.                             *
 *                                                          *
 *  As we return, we end back up in Thread() where we       *
 *  started, which then copies the best score/etc back to   *
 *  the parent thread.                                      *
 *                                                          *
 ************************************************************
 */
  if (abort_search || tree->stop || mode == parallel)
    return alpha;
/*
 ************************************************************
 *                                                          *
 *  Search completed.  All moves have been searched.  If    *
 *  none were legal, return either MATE or DRAW depending   *
 *  on whether the side to move is in check or not.         *
 *                                                          *
 ************************************************************
 */
  if (moves_done == 0) {
    value = (Check(wtm)) ? -(MATE - ply) : DrawScore(wtm);
    if (value >= alpha && value < beta) {
      SavePV(tree, ply, 0);
#if defined(TRACE)
      if (ply <= trace_level)
        printf("Search() no moves!  ply=%d\n", ply);
#endif
    }
    return value;
  } else {
    bestmove =
        (alpha ==
        original_alpha) ? tree->hash_move[ply] : tree->pv[ply].path[ply];
    type = (alpha == original_alpha) ? UPPER : EXACT;
    if (repeat == 2 && alpha != -(MATE - ply - 1)) {
      value = DrawScore(wtm);
      if (value < beta)
        SavePV(tree, ply, 3);
#if defined(TRACE)
      if (ply <= trace_level)
        printf("draw by 50 move rule detected, ply=%d.\n", ply);
#endif
      return value;
    } else if (alpha != original_alpha) {
      tree->pv[ply - 1] = tree->pv[ply];
      tree->pv[ply - 1].path[ply - 1] = tree->curmv[ply - 1];
    }
    HashStore(tree, ply, depth, wtm, type, alpha, bestmove);
    return alpha;
  }
}

/* last modified 08/03/16 */
/*
 *******************************************************************************
 *                                                                             *
 *   SearchMove() implements the PVS search and returns the value.  We do a    *
 *   null-window search with the window (alpha, t_beta) and if that fails high *
 *   we repeat the search with the window {alpha, beta} assuming that beta !=  *
 *   t_beta.                                                                   *
 *                                                                             *
 *******************************************************************************
 */
int SearchMove(TREE * RESTRICT tree, int ply, int depth, int wtm, int alpha,
    int t_beta, int beta, int extend, int reduce, int check) {
  int value;
/*
 ************************************************************
 *                                                          *
 *  PVS search.  We have determined whether the depth is to *
 *  be changed by an extension or a reduction.  If we get   *
 *  to this point, then the move is not being pruned.  So   *
 *  off we go to a recursive search/quiescence call to work *
 *  our way toward a terminal node.                         *
 *                                                          *
 *  There is one special-case to handle.  If the depth was  *
 *  reduced, and Search() returns a value >= beta then      *
 *  accepting that is risky (we reduced the move as we      *
 *  thought it was bad and expected it to fail low) so we   *
 *  repeat the search using the original (non-reduced)      *
 *  depth to see if the fail-high happens again.            *
 *                                                          *
 ************************************************************
 */
  if (depth + extend - reduce - 1 > 0) {
    value =
        -Search(tree, ply + 1, depth + extend - reduce - 1, Flip(wtm),
        -t_beta, -alpha, check, DO_NULL);
    if (value > alpha && reduce) {
      value =
          -Search(tree, ply + 1, depth - 1, Flip(wtm), -t_beta, -alpha, check,
          DO_NULL);
    }
  } else
    value = -Quiesce(tree, ply + 1, Flip(wtm), -t_beta, -alpha, 1);
  if (abort_search || tree->stop)
    return 0;
/*
 ************************************************************
 *                                                          *
 *  PVS re-search.  This is the PVS re-search code.  If we  *
 *  reach this point and value > alpha and value < beta,    *
 *  then this can not be a null-window search.  We have to  *
 *  re-search the position with the original beta value     *
 *  to see if it still fails high before we treat this as a *
 *  real fail-high and back up the value to the previous    *
 *  ply.                                                    *
 *                                                          *
 ************************************************************
 */
  if (value > alpha && value < beta && t_beta < beta) {
    if (ply == 1)
      return beta;
    if (depth + extend - 1 > 0)
      value =
          -Search(tree, ply + 1, depth + extend - 1, Flip(wtm), -beta, -alpha,
          check, DO_NULL);
    else
      value = -Quiesce(tree, ply + 1, Flip(wtm), -beta, -alpha, 1);
    if (abort_search || tree->stop)
      return 0;
  }
  return value;
}

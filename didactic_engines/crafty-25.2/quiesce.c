#include "chess.h"
#include "data.h"
/* last modified 08/03/16 */
/*
 *******************************************************************************
 *                                                                             *
 *   Quiece() is the recursive routine used to implement the quiescence        *
 *   search part of the alpha/beta negamax search.  It performs the following  *
 *   functions:                                                                *
 *                                                                             *
 *   (1) It computes a stand-pat score, which gives the side-on-move the       *
 *   choice of standing pat and not playing any move at all and just accepting *
 *   the current static evaluation, or else it may try captures and/or         *
 *   checking moves to see if it can improve the stand-pat score by making a   *
 *   move that leads to some sort of positional or material gain.              *
 *                                                                             *
 *   (2) The first phase is to generate all possible capture moves and then    *
 *   sort them into descending using the value of the captured piece and the   *
 *   complemented value of the capturing piece.  This is the classic MVV/LVA   *
 *   ordering approach that removes heavy pieces first in an attempt to reduce *
 *   the size of the sub-tree these pieces produce.                            *
 *                                                                             *
 *   (3) When we get ready to actually search each capture, we analyze each    *
 *   move to see if it appears reasonable.  Small pieces can capture larger    *
 *   ones safely, ditto for equal exchanges.  For the rest, we use SEE() to    *
 *   compute the SEE score.  If this is less than zero, we do not search this  *
 *   move at all to avoid wasting time, since a losing capture rarely helps    *
 *   improve the score in the q-search.  The goal here is to find a capture    *
 *   that improves on the stand-pat score and gets us closer to a position     *
 *   that we would describe as "quiet" or "static".                            *
 *                                                                             *
 *   (4) If the parameter "checks" is non-zero, then after searching the       *
 *   captures, we generate checking moves and search those.  This value also   *
 *   slightly changes the way captures are searched, since those that are also *
 *   checks result in calling QuiesceEvasions() which evades checks to see if  *
 *   the check is actually a mate.  This means that we have one layer of full- *
 *   width search to escape checks and do not allow a stand-pat which would    *
 *   hide the effect of the check completely.                                  *
 *                                                                             *
 *******************************************************************************
 */
int Quiesce(TREE * RESTRICT tree, int ply, int wtm, int alpha, int beta,
    int checks) {
  unsigned *next, *movep;
  int original_alpha = alpha, value, repeat;

/*
 ************************************************************
 *                                                          *
 *  Initialize.                                             *
 *                                                          *
 ************************************************************
 */
  if (ply >= MAXPLY - 1)
    return beta;
#if defined(NODES)
  if (search_nodes && --temp_search_nodes <= 0) {
    abort_search = 1;
    return 0;
  }
#endif
  if (tree->thread_id == 0)
    next_time_check--;
/*
 ************************************************************
 *                                                          *
 *  Check for draw by repetition, which includes 50 move    *
 *  draws also.  This is only done at the first ply of the  *
 *  quiescence search since we are following checking moves *
 *  as well.  The parameter "checks" passed in is "1" for   *
 *  that particular case only (when called from Search()).  *
 *  all other calls (from inside Quiesce()) pass a value of *
 *  zero so that additional plies of checks are not tried.  *
 *                                                          *
 ************************************************************
 */
  if (checks) {
    repeat = Repeat(tree, ply);
    if (repeat) {
      value = DrawScore(wtm);
      if (value < beta)
        SavePV(tree, ply, repeat);
#if defined(TRACE)
      if (ply <= trace_level)
        printf("draw by repetition detected, ply=%d.\n", ply);
#endif
      return value;
    }
  }
/*
 ************************************************************
 *                                                          *
 *  Now call Evaluate() to produce the "stand-pat" score    *
 *  that will be returned if no capture is acceptable.  If  *
 *  this score is > alpha and < beta, then we also have to  *
 *  save the path to this node as it is the PV that leads   *
 *  to this score.                                          *
 *                                                          *
 ************************************************************
 */
  value = Evaluate(tree, ply, wtm, alpha, beta);
#if defined(TRACE)
  if (ply <= trace_level)
    Trace(tree, ply, value, wtm, alpha, beta, "Quiesce", serial, EVALUATION,
        0);
#endif
  if (value > alpha) {
    if (value >= beta)
      return value;
    alpha = value;
    tree->pv[ply].pathl = ply;
    tree->pv[ply].pathh = 0;
    tree->pv[ply].pathd = iteration;
  }
/*
 ************************************************************
 *                                                          *
 *  Generate captures and sort them based on simple MVV/LVA *
 *  order.  We simply try to capture the most valuable      *
 *  piece possible, using the least valuable attacker       *
 *  possible, to get rid of heavy pieces quickly and reduce *
 *  the overall size of the tree.                           *
 *                                                          *
 *  Note that later we use the value of the capturing       *
 *  piece, the value of the captured piece, and possibly    *
 *  SEE() to exclude captures that appear to lose material, *
 *  but we delay expending this effort as long as possible, *
 *  since beta cutoffs make it unnecessary to search all of *
 *  these moves anyway.                                     *
 *                                                          *
 ************************************************************
 */
  tree->last[ply] = GenerateCaptures(tree, ply, wtm, tree->last[ply - 1]);
  for (movep = tree->last[ply - 1]; movep < tree->last[ply]; movep++) {
    if (Captured(*movep) == king)
      return beta;
    *movep += MVV_LVA[Captured(*movep)][Piece(*movep)];
  }
  if (!checks && tree->last[ply] == tree->last[ply - 1]) {
    if (alpha != original_alpha) {
      tree->pv[ply - 1] = tree->pv[ply];
      tree->pv[ply - 1].path[ply - 1] = tree->curmv[ply - 1];
    }
    return value;
  }
  NextSort(tree, ply);
/*
 ************************************************************
 *                                                          *
 *  Iterate through the move list and search the resulting  *
 *  positions.  Now that we are ready to actually search    *
 *  the set of capturing moves, we try three quick tests to *
 *  see if the move should be excluded because it appears   *
 *  to lose material.                                       *
 *                                                          *
 *  (1) If the capturing piece is not more valuable than    *
 *  the captured piece, then the move can't lose material   *
 *  and should be searched.                                 *
 *                                                          *
 *  (2) If the capture removes the last opponent piece, we  *
 *  always search this kind of capture since this can be    *
 *  the move the allows a passed pawn to promote when the   *
 *  opponent has no piece to catch it.                      *
 *                                                          *
 *  (3) Otherwise, If the capturing piece is more valuable  *
 *  than the captured piece, we use SEE() to determine if   *
 *  the capture is losing or not so that we don't search    *
 *  hopeless moves.                                         *
 *                                                          *
 ************************************************************
 */
  for (next = tree->last[ply - 1]; next < tree->last[ply]; next++) {
    tree->curmv[ply] = Move(*next);
    if (pcval[Piece(tree->curmv[ply])] > pcval[Captured(tree->curmv[ply])] &&
        TotalPieces(Flip(wtm), occupied)
        - p_vals[Captured(tree->curmv[ply])] > 0 &&
        SEE(tree, wtm, tree->curmv[ply]) < 0)
      continue;
#if defined(TRACE)
    if (ply <= trace_level)
      Trace(tree, ply, 0, wtm, alpha, beta, "Quiesce", serial, CAPTURES,
          next - tree->last[ply - 1] + 1);
#endif
    MakeMove(tree, ply, wtm, tree->curmv[ply]);
    tree->nodes_searched++;
    if (!checks)
      value = -Quiesce(tree, ply + 1, Flip(wtm), -beta, -alpha, 0);
    else if (!Check(wtm)) {
      if (Check(Flip(wtm))) {
        tree->qchecks_done++;
        value = -QuiesceEvasions(tree, ply + 1, Flip(wtm), -beta, -alpha);
      } else
        value = -Quiesce(tree, ply + 1, Flip(wtm), -beta, -alpha, 0);
    }
    UnmakeMove(tree, ply, wtm, tree->curmv[ply]);
    if (abort_search || tree->stop)
      return 0;
    if (value > alpha) {
      if (value >= beta)
        return value;
      alpha = value;
    }
  }
/*
 ************************************************************
 *                                                          *
 *  The next block of code is only executed if the checks   *
 *  parameter is non-zero, otherwise we skip this and exit  *
 *  with no further searching.                              *
 *                                                          *
 *  Generate just the moves (non-captures) that give check  *
 *  and search the ones that SEE() says are safe.  Subtle   *
 *  trick:  we discard the captures left over from the      *
 *  above search since we labeled them "losing moves."      *
 *                                                          *
 ************************************************************
 */
  if (checks) {
    tree->last[ply] = GenerateChecks(tree, wtm, tree->last[ply - 1]);
/*
 ************************************************************
 *                                                          *
 *  Iterate through the move list and search the resulting  *
 *  positions.  We take them in the normal order that       *
 *  GenerateChecks() provides.                              *
 *                                                          *
 ************************************************************
 */
    for (next = tree->last[ply - 1]; next < tree->last[ply]; next++) {
      tree->curmv[ply] = Move(*next);
      if (SEE(tree, wtm, tree->curmv[ply]) >= 0) {
#if defined(TRACE)
        if (ply <= trace_level)
          Trace(tree, ply, 0, wtm, alpha, beta, "Quiesce+checks", serial,
              REMAINING, next - tree->last[ply - 1] + 1);
#endif
        MakeMove(tree, ply, wtm, tree->curmv[ply]);
        tree->nodes_searched++;
        if (!Check(wtm)) {
          tree->qchecks_done++;
          value = -QuiesceEvasions(tree, ply + 1, Flip(wtm), -beta, -alpha);
        }
        UnmakeMove(tree, ply, wtm, tree->curmv[ply]);
        if (abort_search || tree->stop)
          return 0;
        if (value > alpha) {
          if (value >= beta)
            return value;
          alpha = value;
        }
      }
    }
  }
/*
 ************************************************************
 *                                                          *
 *  All moves have been searched.  Return the search result *
 *  that was found.  If the result is not the original      *
 *  alpha score, then we need to back up the PV that is     *
 *  associated with this score.                             *
 *                                                          *
 ************************************************************
 */
  if (alpha != original_alpha) {
    tree->pv[ply - 1] = tree->pv[ply];
    tree->pv[ply - 1].path[ply - 1] = tree->curmv[ply - 1];
  }
  return alpha;
}

/* last modified 08/03/16 */
/*
 *******************************************************************************
 *                                                                             *
 *   QuiesceEvasions() is the recursive routine used to implement the alpha/   *
 *   beta negamax quiescence search.  The primary function here is to escape a *
 *   check that was delivered by Quiesce() at the previous ply.  We do not     *
 *   have the usual "stand pat" option because we have to find a legal move to *
 *   prove we have not been checkmated.                                        *
 *                                                                             *
 *   QuiesceEvasions() uses the legal move generator (GenerateCheckEvasions()) *
 *   to produce only the set of legal moves that escape check.  We try those   *
 *   in the the order they are generated since we are going to try them all    *
 *   unless we get a fail-high.                                                *
 *                                                                             *
 *******************************************************************************
 */
int QuiesceEvasions(TREE * RESTRICT tree, int ply, int wtm, int alpha,
    int beta) {
  int original_alpha, value, moves_searched = 0, order, repeat;

/*
 ************************************************************
 *                                                          *
 *  Initialize.                                             *
 *                                                          *
 ************************************************************
 */
  if (ply >= MAXPLY - 1)
    return beta;
#if defined(NODES)
  if (search_nodes && --temp_search_nodes <= 0) {
    abort_search = 1;
    return 0;
  }
  if (tree->thread_id == 0)
    next_time_check--;
#endif
/*
 ************************************************************
 *                                                          *
 *  Check for draw by repetition, which includes 50 move    *
 *  draws also.                                             *
 *                                                          *
 ************************************************************
 */
  repeat = Repeat(tree, ply);
  if (repeat) {
    value = DrawScore(wtm);
    if (value < beta)
      SavePV(tree, ply, repeat);
#if defined(TRACE)
    if (ply <= trace_level)
      printf("draw by repetition detected, ply=%d.\n", ply);
#endif
    return value;
  }
  original_alpha = alpha;
/*
 ************************************************************
 *                                                          *
 *  Iterate through the move list and search the resulting  *
 *  positions.  These moves are searched in the order that  *
 *  GenerateEvasions() produces them.  No hash move is      *
 *  possible since we don't do probes in Quiesce().  We do  *
 *  clear the hash move before we start selecting moves so  *
 *  that we don't search a bogus move from a different      *
 *  position.                                               *
 *                                                          *
 ************************************************************
 */
  tree->hash_move[ply] = 0;
  tree->next_status[ply].phase = HASH;
  while ((order = NextMove(tree, ply, 0, wtm, 1))) {
#if defined(TRACE)
    if (ply <= trace_level)
      Trace(tree, ply, 0, wtm, alpha, beta, "QuiesceEvasions", serial,
          tree->phase[ply], order);
#endif
    moves_searched++;
    MakeMove(tree, ply, wtm, tree->curmv[ply]);
    tree->nodes_searched++;
    value = -Quiesce(tree, ply + 1, Flip(wtm), -beta, -alpha, 0);
    UnmakeMove(tree, ply, wtm, tree->curmv[ply]);
    if (abort_search || tree->stop)
      return 0;
    if (value > alpha) {
      if (value >= beta)
        return value;
      alpha = value;
    }
  }
/*
 ************************************************************
 *                                                          *
 *  All moves have been searched.  If none were legal, it   *
 *  must be a mate since we have to be in check to reach    *
 *  QuiesceEvasions().                                      *
 *                                                          *
 ************************************************************
 */
  if (moves_searched == 0) {
    value = -(MATE - ply);
    if (value >= alpha && value < beta) {
      SavePV(tree, ply, 0);
#if defined(TRACE)
      if (ply <= trace_level)
        printf("Search() no moves!  ply=%d\n", ply);
#endif
    }
    return value;
  } else if (alpha != original_alpha) {
    tree->pv[ply - 1] = tree->pv[ply];
    tree->pv[ply - 1].path[ply - 1] = tree->curmv[ply - 1];
  }
  return alpha;
}

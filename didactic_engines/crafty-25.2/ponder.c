#include "chess.h"
#include "data.h"
/* last modified 08/03/16 */
/*
 *******************************************************************************
 *                                                                             *
 *   Ponder() is the driver for "pondering" (thinking on the opponent's time.) *
 *   its operation is simple:  Find a predicted move by (a) taking the second  *
 *   move from the principal variation, or (b) call lookup to see if it finds  *
 *   a suggested move from the transposition table.  Then, make this move and  *
 *   do a search from the resulting position.  While pondering, one of three   *
 *   things can happen:  (1) A move is entered, and it matches the predicted   *
 *   move.  We then switch from pondering to thinking and search as normal;    *
 *   (2) A move is entered, but it does not match the predicted move.  We then *
 *   abort the search, unmake the pondered move, and then restart with the     *
 *   move entered.  (3) A command is entered.  If it is a simple command, it   *
 *   can be done without aborting the search or losing time.  If not, we abort *
 *   the search, execute the command, and then attempt to restart pondering if *
 *   the command didn't make that impossible.                                  *
 *                                                                             *
 *******************************************************************************
 */
int Ponder(int wtm) {
  TREE *const tree = block[0];
  int dalpha = -999999, dbeta = 999999, i;
  unsigned *n_ponder_moves, *mv;
  int save_move_number, tlom, value, illegal = 0;

/*
 ************************************************************
 *                                                          *
 *  First, let's check to see if pondering is allowed, or   *
 *  if we should avoid pondering on this move since it is   *
 *  the first move of a game, or if the game is over, or    *
 *  "force" mode is active, or there is input in the queue  *
 *  that needs to be read and processed.                    *
 *                                                          *
 ************************************************************
 */
  if (!ponder || force || over || CheckInput())
    return 0;
  save_move_number = move_number;
/*
 ************************************************************
 *                                                          *
 *  Check the ponder move for legality.  If it is not a     *
 *  legal move, we have to take action to find something to *
 *  ponder.                                                 *
 *                                                          *
 ************************************************************
 */
  strcpy(ponder_text, "none");
  if (ponder_move) {
    if (!VerifyMove(tree, 1, wtm, ponder_move)) {
      ponder_move = 0;
      Print(4095, "ERROR.  ponder_move is illegal (1).\n");
      Print(4095, "ERROR.  PV pathl=%d\n", last_pv.pathl);
      Print(4095, "ERROR.  move=%d  %x\n", ponder_move, ponder_move);
    }
  }
/*
 ************************************************************
 *                                                          *
 *  First attempt, do a hash probe.  However, since a hash  *
 *  collision is remotely possible, we still need to verify *
 *  that the transposition/refutation best move is actually *
 *  legal.                                                  *
 *                                                          *
 ************************************************************
 */
  if (!ponder_move) {
    HashProbe(tree, 0, 0, wtm, dalpha, dbeta, &value);
    if (tree->hash_move[0])
      ponder_move = tree->hash_move[0];
    if (ponder_move) {
      if (!VerifyMove(tree, 1, wtm, ponder_move)) {
        Print(4095, "ERROR.  ponder_move is illegal (2).\n");
        Print(4095, "ERROR.  move=%d  %x\n", ponder_move, ponder_move);
        ponder_move = 0;
      }
    }
  }
/*
 ************************************************************
 *                                                          *
 *  Second attempt.  If that didn't work, then we try what  *
 *  I call a "puzzling" search.  Which is simply a shorter  *
 *  time-limit search for the other side, to find something *
 *  to ponder.                                              *
 *                                                          *
 ************************************************************
 */
  if (!ponder_move) {
    TimeSet(puzzle);
    if (time_limit < 20)
      return 0;
    puzzling = 1;
    Print(32, "              puzzling over a move to ponder.\n");
    last_pv.pathl = 0;
    last_pv.pathd = 0;
    for (i = 0; i < MAXPLY; i++) {
      tree->killers[i].move1 = 0;
      tree->killers[i].move2 = 0;
    }
    Iterate(wtm, puzzle, 0);
    for (i = 0; i < MAXPLY; i++) {
      tree->killers[i].move1 = 0;
      tree->killers[i].move2 = 0;
    }
    puzzling = 0;
    if (tree->pv[0].pathl)
      ponder_move = tree->pv[0].path[1];
    if (!ponder_move)
      return 0;
    for (i = 1; i < (int) tree->pv[0].pathl; i++)
      last_pv.path[i] = tree->pv[0].path[i + 1];
    last_pv.pathl = tree->pv[0].pathl - 1;
    last_pv.pathd = 0;
    if (!VerifyMove(tree, 1, wtm, ponder_move)) {
      ponder_move = 0;
      Print(4095, "ERROR.  ponder_move is illegal (3).\n");
      Print(4095, "ERROR.  PV pathl=%d\n", last_pv.pathl);
      return 0;
    }
  }
/*
 ************************************************************
 *                                                          *
 *  Display the move we are going to "ponder".              *
 *                                                          *
 ************************************************************
 */
  if (wtm)
    Print(32, "White(%d): %s [pondering]\n", move_number, OutputMove(tree, 0,
            wtm, ponder_move));
  else
    Print(32, "Black(%d): %s [pondering]\n", move_number, OutputMove(tree, 0,
            wtm, ponder_move));
  sprintf(ponder_text, "%s", OutputMove(tree, 0, wtm, ponder_move));
  if (post)
    printf("Hint: %s\n", ponder_text);
/*
 ************************************************************
 *                                                          *
 *  Set the ponder move list and eliminate illegal moves.   *
 *  This list is used to test the move entered while we are *
 *  pondering, since we need a move list for the input      *
 *  screening process.                                      *
 *                                                          *
 ************************************************************
 */
  n_ponder_moves = GenerateCaptures(tree, 0, wtm, ponder_moves);
  num_ponder_moves =
      GenerateNoncaptures(tree, 0, wtm, n_ponder_moves) - ponder_moves;
  for (mv = ponder_moves; mv < ponder_moves + num_ponder_moves; mv++) {
    MakeMove(tree, 0, wtm, *mv);
    illegal = Check(wtm);
    UnmakeMove(tree, 0, wtm, *mv);
    if (illegal)
      *mv = 0;
  }
/*
 ************************************************************
 *                                                          *
 *  Now, perform an iterated search, but with the special   *
 *  "pondering" flag set which changes the time controls    *
 *  since there is no need to stop searching until the      *
 *  opponent makes a move.                                  *
 *                                                          *
 ************************************************************
 */
  MakeMove(tree, 0, wtm, ponder_move);
  tree->curmv[0] = ponder_move;
  tree->rep_list[++rep_index] = HashKey;
  tlom = last_opponent_move;
  last_opponent_move = ponder_move;
  if (kibitz)
    strcpy(kibitz_text, "n/a");
  thinking = 0;
  pondering = 1;
  if (!wtm)
    move_number++;
  ponder_value = Iterate(Flip(wtm), think, 0);
  rep_index--;
  move_number = save_move_number;
  pondering = 0;
  thinking = 0;
  last_opponent_move = tlom;
  UnmakeMove(tree, 0, wtm, ponder_move);
/*
 ************************************************************
 *                                                          *
 *  Search completed. the possible return values are:       *
 *                                                          *
 *  (0) No pondering was done, period.                      *
 *                                                          *
 *  (1) Pondering was done, opponent made the predicted     *
 *      move, and we searched until time ran out in a       *
 *      normal manner.                                      *
 *                                                          *
 *  (2) Pondering was done, but the ponder search           *
 *      terminated due to either finding a mate, or the     *
 *      maximum search depth was reached.  The result of    *
 *      this ponder search are valid, but only if the       *
 *      opponent makes the correct (predicted) move.        *
 *                                                          *
 *  (3) Pondering was done, but the opponent either made a  *
 *      different move, or entered a command that has to    *
 *      interrupt the pondering search before the command   *
 *      (or move) can be processed.  This forces Main() to  *
 *      avoid reading in a move/command since one has been  *
 *      read into the command buffer already.               *
 *                                                          *
 ************************************************************
 */
  if (input_status == 1)
    return 1;
  if (input_status == 2)
    return 3;
  return 2;
}

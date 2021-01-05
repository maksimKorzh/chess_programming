#include "chess.h"
#include "data.h"
/* last modified 12/29/15 */
/*
 *******************************************************************************
 *                                                                             *
 *   NextMove() is used to select the next move from the current move list.    *
 *                                                                             *
 *   The "excluded move" code below simply collects any moves that were        *
 *   searched without being generated (hash move and up to 4 killers).  We     *
 *   save them in the NEXT structure and make sure to exclude them when        *
 *   searching after a move generation to avoid the duplicated effort.         *
 *                                                                             *
 *******************************************************************************
 */
int NextMove(TREE * RESTRICT tree, int ply, int depth, int side, int in_check) {
  unsigned *movep, *bestp;
  int hist, bestval, possible;

/*
 ************************************************************
 *                                                          *
 *  The following "big switch" controls the finate state    *
 *  machine that selects moves.  The "phase" value in the   *
 *  next_status[ply] structure is always set after a move   *
 *  is selected, and it defines the next state of the FSM   *
 *  so select the next move in a sequenced order.           *
 *                                                          *
 ************************************************************
 */
  switch (tree->next_status[ply].phase) {
/*
 ************************************************************
 *                                                          *
 *  First, try the transposition table move (which will be  *
 *  the principal variation move as we first move down the  *
 *  tree) or the best move found in this position during a  *
 *  prior search.                                           *
 *                                                          *
 ************************************************************
 */
    case HASH:
      tree->next_status[ply].order = 0;
      tree->next_status[ply].exclude = &tree->next_status[ply].done[0];
      tree->next_status[ply].phase = GENERATE_CAPTURES;
      if (tree->hash_move[ply]) {
        tree->curmv[ply] = tree->hash_move[ply];
        *(tree->next_status[ply].exclude++) = tree->curmv[ply];
        if (ValidMove(tree, ply, side, tree->curmv[ply])) {
          tree->phase[ply] = HASH;
          return ++tree->next_status[ply].order;
        }
#if defined(DEBUG)
        else
          Print(2048, "ERROR:  bad move from hash table, ply=%d\n", ply);
#endif
      }
/*
 ************************************************************
 *                                                          *
 *  Generate captures and sort them based on the simple     *
 *  MVV/LVA ordering where we try to capture the most       *
 *  valuable victim piece possible, using the least         *
 *  valuable attacking piece possible.  Later we will test  *
 *  to see if the capture appears to lose material and we   *
 *  will defer searching it until later.                    *
 *                                                          *
 *  Or, if in check, generate all the legal moves that      *
 *  escape check by using GenerateCheckEvasions().  After   *
 *  we do this, we sort them using MVV/LVA to move captures *
 *  to the front of the list in the correct order.          *
 *                                                          *
 ************************************************************
 */
    case GENERATE_CAPTURES:
      tree->next_status[ply].phase = CAPTURES;
      if (!in_check)
        tree->last[ply] =
            GenerateCaptures(tree, ply, side, tree->last[ply - 1]);
      else
        tree->last[ply] =
            GenerateCheckEvasions(tree, ply, side, tree->last[ply - 1]);
/*
 ************************************************************
 *                                                          *
 *  Now make a pass over the moves to assign the sort value *
 *  for each.  We simply use MVV/LVA move order here.  A    *
 *  simple optimization is to use the pre-computed array    *
 *  MVV_LVA[victim][attacker] which returns a simple value  *
 *  that indicates MVV/LVA order.                           *
 *                                                          *
 ************************************************************
 */
      tree->next_status[ply].remaining = 0;
      for (movep = tree->last[ply - 1]; movep < tree->last[ply]; movep++)
        if (*movep == tree->hash_move[ply]) {
          *movep = 0;
          tree->next_status[ply].exclude = &tree->next_status[ply].done[0];
        } else {
          *movep += MVV_LVA[Captured(*movep)][Piece(*movep)];
          tree->next_status[ply].remaining++;
        }
      NextSort(tree, ply);
      tree->next_status[ply].last = tree->last[ply - 1];
      if (in_check)
        goto remaining_moves;
/*
 ************************************************************
 *                                                          *
 *  Try the captures moves, which are in order based on     *
 *  MVV/LVA ordering.  If a larger-valued piece captures a  *
 *  lesser-valued piece, and SEE() says it loses material,  *
 *  this capture will be deferred until later.              *
 *                                                          *
 *  If we are in check, we jump down to the history moves   *
 *  phase (we don't need to generate any more moves as      *
 *  GenerateCheckEvasions has already generated all legal   *
 *  moves.                                                  *
 *                                                          *
 ************************************************************
 */
    case CAPTURES:
      while (tree->next_status[ply].remaining) {
        tree->curmv[ply] = Move(*(tree->next_status[ply].last++));
        if (!--tree->next_status[ply].remaining)
          tree->next_status[ply].phase = KILLER1;
        if (pcval[Piece(tree->curmv[ply])] <=
            pcval[Captured(tree->curmv[ply])]
            || SEE(tree, side, tree->curmv[ply]) >= 0) {
          *(tree->next_status[ply].last - 1) = 0;
          tree->phase[ply] = CAPTURES;
          return ++tree->next_status[ply].order;
        }
      }
/*
 ************************************************************
 *                                                          *
 *  Now, try the killer moves.  This phase tries the two    *
 *  killers for the current ply without generating moves,   *
 *  which saves time if a cutoff occurs.  After those two   *
 *  killers are searched, we try the killers from two plies *
 *  back since they have greater depth and might produce a  *
 *  cutoff if the current two do not.                       *
 *                                                          *
 ************************************************************
 */
    case KILLER1:
      possible = tree->killers[ply].move1;
      if (!Exclude(tree, ply, possible) &&
          ValidMove(tree, ply, side, possible)) {
        tree->curmv[ply] = possible;
        *(tree->next_status[ply].exclude++) = possible;
        tree->next_status[ply].phase = KILLER2;
        tree->phase[ply] = KILLER1;
        return ++tree->next_status[ply].order;
      }
    case KILLER2:
      possible = tree->killers[ply].move2;
      if (!Exclude(tree, ply, possible) &&
          ValidMove(tree, ply, side, possible)) {
        tree->curmv[ply] = possible;
        *(tree->next_status[ply].exclude++) = possible;
        tree->next_status[ply].phase = (ply < 3) ? COUNTER_MOVE1 : KILLER3;
        tree->phase[ply] = KILLER2;
        return ++tree->next_status[ply].order;
      }
    case KILLER3:
      possible = tree->killers[ply - 2].move1;
      if (!Exclude(tree, ply, possible) &&
          ValidMove(tree, ply, side, possible)) {
        tree->curmv[ply] = possible;
        *(tree->next_status[ply].exclude++) = possible;
        tree->next_status[ply].phase = KILLER4;
        tree->phase[ply] = KILLER3;
        return ++tree->next_status[ply].order;
      }
    case KILLER4:
      possible = tree->killers[ply - 2].move2;
      if (!Exclude(tree, ply, possible) &&
          ValidMove(tree, ply, side, possible)) {
        tree->curmv[ply] = possible;
        *(tree->next_status[ply].exclude++) = possible;
        tree->next_status[ply].phase = COUNTER_MOVE1;
        tree->phase[ply] = KILLER4;
        return ++tree->next_status[ply].order;
      }
/*
 ************************************************************
 *                                                          *
 *  Now, before we give up and generate moves, try the      *
 *  counter-move which was a move that failed high in the   *
 *  past when the move at the previous ply was played.      *
 *                                                          *
 ************************************************************
 */
    case COUNTER_MOVE1:
      possible = tree->counter_move[tree->curmv[ply - 1] & 4095].move1;
      if (!Exclude(tree, ply, possible) &&
          ValidMove(tree, ply, side, possible)) {
        tree->curmv[ply] = possible;
        *(tree->next_status[ply].exclude++) = possible;
        tree->next_status[ply].phase = COUNTER_MOVE2;
        tree->phase[ply] = COUNTER_MOVE1;
        return ++tree->next_status[ply].order;
      }
    case COUNTER_MOVE2:
      possible = tree->counter_move[tree->curmv[ply - 1] & 4095].move2;
      if (!Exclude(tree, ply, possible) &&
          ValidMove(tree, ply, side, possible)) {
        tree->curmv[ply] = possible;
        *(tree->next_status[ply].exclude++) = possible;
        tree->next_status[ply].phase = MOVE_PAIR1;
        tree->phase[ply] = COUNTER_MOVE2;
        return ++tree->next_status[ply].order;
      }
/*
 ************************************************************
 *                                                          *
 *  Finally we try paired moves, which are simply moves     *
 *  that were good when played after the other move in the  *
 *  pair was played two plies back.                         *
 *                                                          *
 ************************************************************
 */
    case MOVE_PAIR1:
      possible = tree->move_pair[tree->curmv[ply - 2] & 4095].move1;
      if (!Exclude(tree, ply, possible) &&
          ValidMove(tree, ply, side, possible)) {
        tree->curmv[ply] = possible;
        *(tree->next_status[ply].exclude++) = possible;
        tree->next_status[ply].phase = MOVE_PAIR2;
        tree->phase[ply] = MOVE_PAIR1;
        return ++tree->next_status[ply].order;
      }
    case MOVE_PAIR2:
      possible = tree->move_pair[tree->curmv[ply - 2] & 4095].move2;
      if (!Exclude(tree, ply, possible) &&
          ValidMove(tree, ply, side, possible)) {
        tree->curmv[ply] = possible;
        *(tree->next_status[ply].exclude++) = possible;
        tree->next_status[ply].phase = GENERATE_QUIET;
        tree->phase[ply] = MOVE_PAIR2;
        return ++tree->next_status[ply].order;
      }
/*
 ************************************************************
 *                                                          *
 *  Now, generate all non-capturing moves, which get added  *
 *  to the move list behind any captures we did not search. *
 *                                                          *
 ************************************************************
 */
    case GENERATE_QUIET:
      if (!in_check)
        tree->last[ply] =
            GenerateNoncaptures(tree, ply, side, tree->last[ply]);
      tree->next_status[ply].last = tree->last[ply - 1];
/*
 ************************************************************
 *                                                          *
 *  Now, try the history moves.  This phase takes the       *
 *  complete move list, and passes over them in a classic   *
 *  selection-sort, choosing the move with the highest      *
 *  history score.  This phase is only done one time, as it *
 *  also purges the hash, killer, counter and paired moves  *
 *  from the list.                                          *
 *                                                          *
 ************************************************************
 */
      tree->next_status[ply].remaining = 0;
      tree->next_status[ply].phase = HISTORY;
      bestval = -99999999;
      bestp = 0;
      for (movep = tree->last[ply - 1]; movep < tree->last[ply]; movep++)
        if (*movep) {
          if (Exclude(tree, ply, *movep))
            *movep = 0;
          else if (depth >= 6) {
            tree->next_status[ply].remaining++;
            hist = history[HistoryIndex(side, *movep)];
            if (hist > bestval) {
              bestval = hist;
              bestp = movep;
            }
          }
        }
      tree->next_status[ply].remaining /= 2;
      if (bestp) {
        tree->curmv[ply] = Move(*bestp);
        *bestp = 0;
        tree->phase[ply] = HISTORY;
        return ++tree->next_status[ply].order;
      }
      goto remaining_moves;
/*
 ************************************************************
 *                                                          *
 *  Now, continue with the history moves, but since one     *
 *  pass has been made over the complete move list, there   *
 *  are no hash/killer moves left in the list, so the tests *
 *  for these can be avoided.                               *
 *                                                          *
 ************************************************************
 */
    case HISTORY:
      if (depth >= 6) {
        bestval = -99999999;
        bestp = 0;
        for (movep = tree->last[ply - 1]; movep < tree->last[ply]; movep++)
          if (*movep) {
            hist = history[HistoryIndex(side, *movep)];
            if (hist > bestval) {
              bestval = hist;
              bestp = movep;
            }
          }
        if (bestp) {
          tree->curmv[ply] = Move(*bestp);
          *bestp = 0;
          if (--(tree->next_status[ply].remaining) <= 0) {
            tree->next_status[ply].phase = REMAINING;
            tree->next_status[ply].last = tree->last[ply - 1];
          }
          tree->phase[ply] = HISTORY;
          return ++tree->next_status[ply].order;
        }
      }
/*
 ************************************************************
 *                                                          *
 *  Then we try the rest of the set of moves, and we do not *
 *  use Exclude() function to skip any moves we have        *
 *  already searched (hash or killers) since the history    *
 *  phase above has already done that.                      *
 *                                                          *
 ************************************************************
 */
    remaining_moves:
      tree->next_status[ply].phase = REMAINING;
      tree->next_status[ply].last = tree->last[ply - 1];
    case REMAINING:
      for (; tree->next_status[ply].last < tree->last[ply];
          tree->next_status[ply].last++)
        if (*tree->next_status[ply].last) {
          tree->curmv[ply] = Move(*tree->next_status[ply].last++);
          tree->phase[ply] = REMAINING;
          return ++tree->next_status[ply].order;
        }
      return NONE;
    default:
      Print(4095, "oops!  next_status.phase is bad! [phase=%d]\n",
          tree->next_status[ply].phase);
  }
  return NONE;
}

/* last modified 07/03/14 */
/*
 *******************************************************************************
 *                                                                             *
 *   NextRootMove() is used to select the next move from the root move list.   *
 *                                                                             *
 *   There is one subtle trick here that must not be broken.  Crafty does LMR  *
 *   at the root, and the reduction amount is dependent on the order in which  *
 *   a specific move is searched.  With the recent changes dealing with this   *
 *   issue in non-root moves, NextRootMove() now simply returns the move's     *
 *   order within the move list.  This might be a problem if the last move in  *
 *   the list fails high, because it would be reduced on the re-search, which  *
 *   is something we definitely don't want.  The solution is found in the code *
 *   inside Iterate().  When a move fails high, it is moved to the top of the  *
 *   move list so that (a) it is searched first on the re-search (more on this *
 *   in a moment) and (b) since its position in the move list is now #1, it    *
 *   will get an order value of 1 which is never reduced.  The only warning is *
 *   that Iterate() MUST re-sort the ply-1 move list after a fail high, even   *
 *   though it seems like a very tiny computational waste.                     *
 *                                                                             *
 *   The other reason for doing the re-sort has to do with the parallel search *
 *   algorithm.  When one thread fails high at the root, it stops the others.  *
 *   they have to carefully undo the "this move has been searched" flag since  *
 *   these incomplete searches need to be re-done after the fail-high move is  *
 *   finished.  But it is possible some of those interrupted moves appear      *
 *   before the fail high move in the move list.  Which would lead Crafty to   *
 *   fail high, then produce a different best move's PV.  By re-sorting, now   *
 *   the fail-high move is always searched first since here we just start at   *
 *   the top of the move list and look for the first "not yet searched" move   *
 *   to return.  It solves several problems, but if that re-sort is not done,  *
 *   things go south quickly.  The voice of experience is all I will say here. *
 *                                                                             *
 *******************************************************************************
 */
int NextRootMove(TREE * RESTRICT tree, TREE * RESTRICT mytree, int side) {
  uint64_t total_nodes;
  int which, i, t;

/*
 ************************************************************
 *                                                          *
 *  First, we check to see if we only have one legal move.  *
 *  If so, and we are not pondering, we stop after a short  *
 *  search, saving time, but making sure we have something  *
 *  to ponder.                                              *
 *                                                          *
 ************************************************************
 */
  if (!annotate_mode && !pondering && !booking && n_root_moves == 1 &&
      iteration > 10) {
    abort_search = 1;
    return NONE;
  }
/*
 ************************************************************
 *                                                          *
 *  For the moves at the root of the tree, the list has     *
 *  already been generated and sorted.                      *
 *                                                          *
 *  We simply have to find the first move that has a zero   *
 *  "already searched" flag and choose that one.  We do set *
 *  the "already searched" flag for this move before we     *
 *  return so that it won't be searched again in another    *
 *  thread.                                                 *
 *                                                          *
 ************************************************************
 */
  for (which = 0; which < n_root_moves; which++) {
    if (!(root_moves[which].status & 8)) {
      if (search_move) {
        if (root_moves[which].move != search_move) {
          root_moves[which].status |= 8;
          continue;
        }
      }
      tree->curmv[1] = root_moves[which].move;
      root_moves[which].status |= 8;
/*
 ************************************************************
 *                                                          *
 *  We have found a move to search.  If appropriate, we     *
 *  display this move, along with the time and information  *
 *  such as which move this is in the list and how many     *
 *  are left to search before this iteration is done, and   *
 *  a "status" character that shows the state of the        *
 *  current search ("?" means we are pondering, waiting on  *
 *  a move to be entered, "*" means we are searching and    *
 *  our clock is running).  We also display the NPS for     *
 *  the search, simply for information about how fast the   *
 *  machine is running.                                     *
 *                                                          *
 ************************************************************
 */
      if (ReadClock() - start_time > noise_level && display_options & 16) {
        sprintf(mytree->remaining_moves_text, "%d/%d", which + 1,
            n_root_moves);
        end_time = ReadClock();
        Lock(lock_io);
        if (pondering)
          printf("         %2i   %s%7s?  ", iteration,
              Display2Times(end_time - start_time),
              mytree->remaining_moves_text);
        else
          printf("         %2i   %s%7s*  ", iteration,
              Display2Times(end_time - start_time),
              mytree->remaining_moves_text);
        printf("%d. ", move_number);
        if (Flip(side))
          printf("... ");
        strcpy(mytree->root_move_text, OutputMove(tree, 1, side,
                tree->curmv[1]));
        total_nodes = block[0]->nodes_searched;
        for (t = 0; t < smp_max_threads; t++)
          for (i = 0; i < 64; i++)
            if (!(thread[t].blocks & SetMask(i)))
              total_nodes += block[t * 64 + 1 + i]->nodes_searched;
        nodes_per_second = total_nodes * 100 / Max(end_time - start_time, 1);
        i = strlen(mytree->root_move_text);
        i = (i < 8) ? i : 8;
        strncat(mytree->root_move_text, "          ", 8 - i);
        printf("%s", mytree->root_move_text);
        printf("(%snps)             \r", DisplayKMB(nodes_per_second, 0));
        fflush(stdout);
        Unlock(lock_io);
      }
/*
 ************************************************************
 *                                                          *
 *  Bit of a tricky exit.  If the move is not flagged as    *
 *  "OK to search in parallel or reduce" then we return     *
 *  "DO_NOT_REDUCE" which will prevent Search() from        *
 *  reducing the move (LMR).  Otherwise we return the more  *
 *  common "REMAINING" value which allows LMR to be used on *
 *  those root moves.                                       *
 *                                                          *
 ************************************************************
 */
      if (root_moves[which].status & 4)
        tree->phase[1] = DO_NOT_REDUCE;
      else
        tree->phase[1] = REMAINING;
      return which + 1;
    }
  }
  return NONE;
}

/* last modified 11/13/14 */
/*
 *******************************************************************************
 *                                                                             *
 *   NextRootMoveParallel() is used to determine if the next root move can be  *
 *   searched in parallel.  If it appears to Iterate() that one of the moves   *
 *   following the first move might become the best move, the 'no parallel'    *
 *   flag is set to speed up finding the new best move.  This flag is set if   *
 *   this root move has an "age" value > 0 which indicates this move was the   *
 *   "best move" within the previous 3 search iterations.  We want to search   *
 *   such moves as quickly as possible, prior to starting a parallel search at *
 *   the root, in case this move once again becomes the best move and provides *
 *   a better alpha bound.                                                     *
 *                                                                             *
 *******************************************************************************
 */
int NextRootMoveParallel(void) {
  int which;

/*
 ************************************************************
 *                                                          *
 *  Here we simply check the root_move status flag that is  *
 *  set in Iterate() after each iteration is completed.  A  *
 *  value of "1" indicates this move has to be searched by  *
 *  all processors together, splitting at the root must     *
 *  wait until we have searched all moves that have been    *
 *  "best" during the previous three plies.                 *
 *                                                          *
 *  The root move list has a flag, bit 3, used to indicate  *
 *  that this move has been best recently.  If this bit is  *
 *  set, we are forced to use all processors to search this *
 *  move so that it is completed quickly rather than being  *
 *  searched by just one processor, and taking much longer  *
 *  to get a score back.  We do this to give the search the *
 *  best opportunity to fail high on this move before we    *
 *  run out of time.                                        *
 *                                                          *
 ************************************************************
 */
  for (which = 0; which < n_root_moves; which++)
    if (!(root_moves[which].status & 8))
      break;
  if (which < n_root_moves && !(root_moves[which].status & 4))
    return 1;
  return 0;
}

/* last modified 09/11/15 */
/*
 *******************************************************************************
 *                                                                             *
 *   Exclude() searches the list of moves searched prior to generating a move  *
 *   list to exclude those that were searched via a hash table best move or    *
 *   through the killer moves for the current ply and two plies back.          *
 *                                                                             *
 *   The variable next_status[].excluded is the total number of non-generated  *
 *   moves we searched.  next_status[].remaining is initially set to excluded, *
 *   but each time an excluded move is found, the counter is decremented.      *
 *   Once all excluded moves have been found, we avoid running through the     *
 *   list of excluded moves on each call and simply return.                    *
 *                                                                             *
 *******************************************************************************
 */
int Exclude(TREE * RESTRICT tree, int ply, int move) {
  unsigned *i;

  if (tree->next_status[ply].exclude > &tree->next_status[ply].done[0])
    for (i = &tree->next_status[ply].done[0];
        i < tree->next_status[ply].exclude; i++)
      if (move == *i)
        return 1;
  return 0;
}

/* last modified 05/20/15 */
/*
 *******************************************************************************
 *                                                                             *
 *   NextSort() is used to sort the move list.  This is a list of 32 bit       *
 *   values where the rightmost 21 bits is the compressed move, and the left-  *
 *   most 11 bits are the sort key (MVV/LVA values).                           *
 *                                                                             *
 *******************************************************************************
 */
void NextSort(TREE * RESTRICT tree, int ply) {
  unsigned temp, *movep, *tmovep;

/*
 ************************************************************
 *                                                          *
 *  This is a simple insertion sort algorithm.              *
 *                                                          *
 ************************************************************
 */
  if (tree->last[ply] > tree->last[ply - 1] + 1) {
    for (movep = tree->last[ply - 1] + 1; movep < tree->last[ply]; movep++) {
      temp = *movep;
      tmovep = movep - 1;
      while (tmovep >= tree->last[ply - 1] && SortV(*tmovep) < SortV(temp)) {
        *(tmovep + 1) = *tmovep;
        tmovep--;
      }
      *(tmovep + 1) = temp;
    }
  }
}

#include <math.h>
#include "chess.h"
#include "data.h"
/* last modified 02/23/14 */
/*
 *******************************************************************************
 *                                                                             *
 *   TimeAdjust() is called to adjust timing variables after each program move *
 *   is made.  It simply increments the number of moves made, decrements the   *
 *   amount of time used, and then makes any necessary adjustments based on    *
 *   the time controls.                                                        *
 *                                                                             *
 *******************************************************************************
 */
void TimeAdjust(int side, int time_used) {
/*
 ************************************************************
 *                                                          *
 *  Decrement the number of moves remaining to the next     *
 *  time control.  Then subtract the time the program took  *
 *  to choose its move from the time remaining.             *
 *                                                          *
 ************************************************************
 */
  tc_moves_remaining[side]--;
  tc_time_remaining[side] -=
      (tc_time_remaining[side] >
      time_used) ? time_used : tc_time_remaining[side];
  if (!tc_moves_remaining[side]) {
    if (tc_sudden_death == 2)
      tc_sudden_death = 1;
    tc_moves_remaining[side] += tc_secondary_moves;
    tc_time_remaining[side] += tc_secondary_time;
    Print(4095, "time control reached (%s)\n", (side) ? "white" : "black");
  }
  if (tc_increment)
    tc_time_remaining[side] += tc_increment;
}

/* last modified 10/01/14 */
/*
 *******************************************************************************
 *                                                                             *
 *   TimeCheck() is used to determine when the search should stop.  It uses    *
 *   several conditions to make this determination:  (1) The search time has   *
 *   exceeded the time per move limit;  (2) The value at the root of the tree  *
 *   has not dropped too low.                                                  *
 *                                                                             *
 *   We use one additional trick here to avoid stopping the search just before *
 *   we change to a better move.  We simply do our best to complete the        *
 *   iteration which means we have searched every move to this same depth.     *
 *                                                                             *
 *   This is implemented by having Search() call TimeCheck() passing it a      *
 *   value of one (1) for the parameter busy.  TimeCheck() will only end the   *
 *   search if we have exceeded the max time limit.  Otherwise, we continue.   *
 *   Iterate() calls TimeCheck() passing it a value of "0" for busy, which     *
 *   simply says "now, if we have used the target time limit (which can be     *
 *   modified by the "difficulty value), we will stop and not try another      *
 *   iteration."                                                               *
 *                                                                             *
 *   The "difficulty" value is used to implement the concept of an "easy move" *
 *   or a "hard move".  With an easy move, we want to spend less time since    *
 *   the easy move is obvious.  The opposite idea is a hard move, where we     *
 *   actually want to spend more time to be sure we don't make a mistake by`   *
 *   moving too quickly.                                                       *
 *                                                                             *
 *   The basic methodology revolves around how many times we change our mind   *
 *   on the best move at the root of the tree.                                 *
 *                                                                             *
 *   The "difficulty" variable is initially set to 100, which represents a     *
 *   percentage of the actual target time we should spend on this move.  If    *
 *   we end an iteration without having changed our mind at all, difficulty    *
 *   is reduced by multiplying by .9, with a lower bound of 60%.               *
 *                                                                             *
 *   If we change our mind during an iteration, there are two cases.  (1) If   *
 *   difficulty is < 100%, we set it back to 100% +20% for each time we        *
 *   changed the best move.  (2) if difficulty is already at 100% or higher,   *
 *   we multiply difficulty by .80, then add 20% for each root move change.    *
 *   For example, suppose we are at difficulty=60%, and we suddenly change our *
 *   mind twice this iteration (3 different best moves).                       *
 *                                                                             *
 *   difficulty = 100 + 3*20 = 160% of the actual target time will be used.    *
 *                                                                             *
 *   Suppose we change back and forth between two best moves multiple times,   *
 *   with difficulty currently at 100%.  The first time:                       *
 *                                                                             *
 *   difficulty = .80 * 100 + 2*20 = 120%                                      *
 *                                                                             *
 *   The next iteration:                                                       *
 *                                                                             *
 *   difficulty = .80 * 120 + 2 * 20 = 96% _ 40% = 136%                        *
 *                                                                             *
 *   The next iteration:                                                       *
 *                                                                             *
 *   difficulty = .80 * 136% + 40% = 149%                                      *
 *                                                                             *
 *   If we stop changing our mind, then difficulty starts on a downward trend. *
 *   The basic idea is that if we are locked in on a move, we can make it a    *
 *   bit quicker, but if we are changing back and forth, we are going to spend *
 *   more time to try to choose the best move.                                 *
 *                                                                             *
 *******************************************************************************
 */
int TimeCheck(TREE * RESTRICT tree, int busy) {
  int time_used;
  int i, ndone;

/*
 ************************************************************
 *                                                          *
 *  Check to see if we need to "burp" the time to let the   *
 *  operator know the search is progressing and how much    *
 *  time has been used so far.                              *
 *                                                          *
 ************************************************************
 */
  time_used = (ReadClock() - start_time);
  if (time_used >= noise_level && display_options & 16 && time_used > burp) {
    Lock(lock_io);
    if (pondering)
      printf("         %2i   %s%7s?  ", iteration, Display2Times(time_used),
          tree->remaining_moves_text);
    else
      printf("         %2i   %s%7s*  ", iteration, Display2Times(time_used),
          tree->remaining_moves_text);
    if (display_options & 16)
      printf("%d. ", move_number);
    if ((display_options & 16) && Flip(root_wtm))
      printf("... ");
    printf("%s(%snps)             \r", tree->root_move_text,
        DisplayKMB(nodes_per_second, 0));
    burp = (time_used / 1500) * 1500 + 1500;
    fflush(stdout);
    Unlock(lock_io);
  }
/*
 ************************************************************
 *                                                          *
 *  First, check to see if there is only one root move.  If *
 *  so, and we are not pondering, searching a book move or  *
 *  or annotating a game, we can return and make this move  *
 *  instantly.  We do need to finish iteration 1 so that we *
 *  actually back up a move to play.                        *
 *                                                          *
 ************************************************************
 */
  if (n_root_moves == 1 && !booking && !annotate_mode && !pondering &&
      iteration > 1 && (time_used > time_limit || time_used > 100))
    return 1;
  if (iteration <= 2)
    return 0;
/*
 ************************************************************
 *                                                          *
 *  If we are pondering or in analyze mode, we do not       *
 *  terminate on time since there is no time limit placed   *
 *  on these searches.  If we have reached the absolute     *
 *  time limit, we stop the search instantly.               *
 *                                                          *
 ************************************************************
 */
  if (pondering || analyze_mode)
    return 0;
  if (time_used > absolute_time_limit)
    return 1;
/*
 ************************************************************
 *                                                          *
 *  If the operator has specified a specific time limit, we *
 *  stop when we hit that regardless of any other tests     *
 *  used during normal timeing.                             *
 *                                                          *
 ************************************************************
 */
  if (search_time_limit) {
    if (time_used < time_limit)
      return 0;
    else
      return 1;
  }
/*
 ************************************************************
 *                                                          *
 *  If we are under the time limit already set, we do not   *
 *  terminate the search.  Once we reach that limit, we     *
 *  abort the search if we are fixing to start another      *
 *  iteration, otherwise we keep searching to try to        *
 *  complete the current iteration.                         *
 *                                                          *
 ************************************************************
 */
  if (time_used < (difficulty * time_limit) / 100)
    return 0;
  if (!busy)
    return 1;
/*
 ************************************************************
 *                                                          *
 *  We have reached the target time limit.  If we are in    *
 *  the middle of an iteration, we keep going unless we are *
 *  stuck on the first move, where there is no benefit to   *
 *  continuing and this will just burn clock time away.     *
 *                                                          *
 *  This is a bit tricky, because if we are on the first    *
 *  move AND we have failed low, we want to continue the    *
 *  search to find something better, if we have not failed  *
 *  low, we will abort the search in the test that follows  *
 *  this one.                                               *
 *                                                          *
 ************************************************************
 */
  ndone = 0;
  for (i = 0; i < n_root_moves; i++)
    if (root_moves[i].status & 8)
      ndone++;
  if (ndone == 1 && !(root_moves[0].status & 1))
    return 1;
/*
 ************************************************************
 *                                                          *
 *  We are in the middle of an iteration, we have used the  *
 *  allocated time limit, but we have more moves left to    *
 *  search.  We forge on until we complete the iteration    *
 *  which will terminate the search, or until we reach the  *
 *  "absolute_time_limit" where we terminate the search no  *
 *  matter what is going on.                                *
 *                                                          *
 ************************************************************
 */
  if (time_used + 300 > tc_time_remaining[root_wtm])
    return 1;
  return 0;
}

/* last modified 09/30/14 */
/*
 *******************************************************************************
 *                                                                             *
 *   TimeSet() is called to set the two variables "time_limit" and             *
 *   "absolute_time_limit" which controls the amount of time taken by the      *
 *   iterated search.  It simply takes the timing controls as set by the user  *
 *   and uses these values to calculate how much time should be spent on the   *
 *   next search.                                                              *
 *                                                                             *
 *******************************************************************************
 */
void TimeSet(int search_type) {
  int mult = 0, extra = 0;
  int surplus, average;
  int simple_average;

  surplus = 0;
  average = 0;
/*
 ************************************************************
 *                                                          *
 *  Check to see if we are in a sudden-death type of time   *
 *  control.  If so, we have a fixed amount of time         *
 *  remaining.  Set the search time accordingly and exit.   *
 *                                                          *
 *  The basic algorithm is to divide the remaining time     *
 *  left on the clock by a constant (that is larger for     *
 *  ponder=off games since we don't get to ponder and save  *
 *  time as the game progresses) and add the increment.     *
 *                                                          *
 *  Set our MAX search time to the smaller of 5 * the time  *
 *  limit or 1/2 of the time left on the clock.             *
 *                                                          *
 ************************************************************
 */
  if (tc_sudden_death == 1) {
    time_limit =
        (tc_time_remaining[root_wtm] -
        tc_safety_margin) / (ponder ? 20 : 26) + tc_increment;
    absolute_time_limit =
        Min(time_limit * 5, tc_time_remaining[root_wtm] / 2);
  }
/*
 ************************************************************
 *                                                          *
 *  We are not in a sudden_death situation.  We now have    *
 *  two choices:  If the program has saved enough time to   *
 *  meet the surplus requirement, then we simply divide     *
 *  the time left evenly among the moves left.  If we       *
 *  haven't yet saved up a cushion so that "hard-moves"     *
 *  can be searched more thoroughly, we simply take the     *
 *  number of moves divided into the total time as the      *
 *  target.                                                 *
 *                                                          *
 ************************************************************
 */
  else {
    if (move_number <= tc_moves)
      simple_average = (tc_time - tc_safety_margin) / tc_moves;
    else
      simple_average =
          (tc_secondary_time - tc_safety_margin) / tc_secondary_moves;
    surplus =
        Max(tc_time_remaining[root_wtm] - tc_safety_margin -
        simple_average * tc_moves_remaining[root_wtm], 0);
    average =
        (tc_time_remaining[root_wtm] - tc_safety_margin +
        tc_moves_remaining[root_wtm] * tc_increment)
        / tc_moves_remaining[root_wtm];
    if (surplus < tc_safety_margin)
      time_limit = (average < simple_average) ? average : simple_average;
    else
      time_limit =
          (average < 2.0 * simple_average) ? average : 2.0 * simple_average;
  }
  if (tc_increment > 200 && moves_out_of_book < 2)
    time_limit *= 1.2;
  if (time_limit <= 0)
    time_limit = 5;
  absolute_time_limit =
      time_limit + surplus / 2 + (tc_time_remaining[root_wtm] -
      tc_safety_margin) / 4;
  if (absolute_time_limit > 5 * time_limit)
    absolute_time_limit = 5 * time_limit;
  if (absolute_time_limit > tc_time_remaining[root_wtm] / 2)
    absolute_time_limit = tc_time_remaining[root_wtm] / 2;
/*
 ************************************************************
 *                                                          *
 *  The "usage" option can be used to force the time limit  *
 *  higher or lower than normal.  The new "timebook"        *
 *  command can also modify the target time making the      *
 *  program use more time early in the game as it exits the *
 *  book, knowing it will save time later on by ponder hits *
 *  and instant moves.                                      *
 *                                                          *
 ************************************************************
 */
  if (usage_level)
    time_limit *= 1.0 + usage_level / 100.0;
  if (first_nonbook_factor && moves_out_of_book < first_nonbook_span) {
    mult =
        (first_nonbook_span - moves_out_of_book + 1) * first_nonbook_factor;
    extra = time_limit * mult / first_nonbook_span / 100;
    time_limit += extra;
  }
/*
 ************************************************************
 *                                                          *
 *  If the operator has set an absolute search time limit   *
 *  already, then we simply copy this value and return.     *
 *                                                          *
 ************************************************************
 */
  if (search_time_limit) {
    time_limit = search_time_limit;
    absolute_time_limit = time_limit;
  }
  if (search_type == puzzle || search_type == booking) {
    time_limit /= 10;
    absolute_time_limit = time_limit * 3;
  }
  if (!tc_sudden_death && !search_time_limit &&
      time_limit > 3 * tc_time / tc_moves)
    time_limit = 3 * tc_time / tc_moves;
  time_limit = Min(time_limit, absolute_time_limit);
  if (search_type != puzzle) {
    if (!tc_sudden_death)
      Print(32, "        time surplus %s  ", DisplayTime(surplus));
    else
      Print(32, "        ");
    Print(32, "time limit %s", DisplayTimeKibitz(time_limit));
    Print(32, " (%s)\n", DisplayTimeKibitz(absolute_time_limit));
  }
  if (time_limit <= 1) {
    time_limit = 1;
    usage_level = 0;
  }
}

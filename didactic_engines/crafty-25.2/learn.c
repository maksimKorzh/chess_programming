#include <math.h>
#include <time.h>
#include "chess.h"
#include "data.h"
#if defined(UNIX)
#  include <unistd.h>
#endif

/* last modified 02/24/14 */
/*
 *******************************************************************************
 *                                                                             *
 *   LearnAdjust() us used to scale the learn value, which can be used to      *
 *   limit the aggressiveness of the learning algorithm.  All we do here is    *
 *   divide the learn value passed in by "learning / 10".                      *
 *                                                                             *
 *******************************************************************************
 */
int LearnAdjust(int value) {

  if (learning / 10 > 0)
    return value / (learning / 10);
  else
    return 0;
}

/* last modified 02/24/14 */
/*
 *******************************************************************************
 *                                                                             *
 *   LearnBook() is used to update the book database when a game ends for any  *
 *   reason.  It uses the global "learn_value" variable and updates the book   *
 *   based on the moves played and the value that was "learned".               *
 *                                                                             *
 *   The global learn_value has two possible sources.  If a game ends with a   *
 *   real result (win, lose or draw) then the learrn_value will be set to a    *
 *   number in the interval {-300, 300} depending on the result.  If there is  *
 *   no result (the operator exits the program prior to reaching a conclusion  *
 *   (quit, end, ^C) then we will use the values from the first few searches   *
 *   after leaving book to compute a learrn_value (see LearnValue() comments   *
 *   later in this file).                                                      *
 *                                                                             *
 *******************************************************************************
 */
void LearnBook() {
  float book_learn[64], t_learn_value;
  int nplies = 0, thisply = 0, i, j, v, cluster;
  unsigned char buf32[4];

/*
 ************************************************************
 *                                                          *
 *  If we have not been "out of book" for N moves, all we   *
 *  we need to do is take the search evaluation for the     *
 *  search just completed and tuck it away in the book      *
 *  learning array (book_learn_eval[]) for use later.       *
 *                                                          *
 ************************************************************
 */
  if (!book_file)
    return;
  if (!learn)
    return;
  if (Abs(learn_value) != learning)
    learn_value = LearnAdjust(learn_value);
  learn = 0;
  Print(32, "LearnBook() updating book database\n");
/*
 ************************************************************
 *                                                          *
 *  Now we build a vector of book learning results.  We     *
 *  give every book move below the last point where there   *
 *  were alternatives 100% of the learned score.  We give   *
 *  the book move played at that point 100% of the learned  *
 *  score as well.  Then we divide the learned score by the *
 *  number of alternatives, and propagate this score back   *
 *  until there was another alternative, where we do this   *
 *  again and again until we reach the top of the book      *
 *  tree.                                                   *
 *                                                          *
 ************************************************************
 */
  t_learn_value = ((float) learn_value) / 100.0;
  for (i = 0; i < 64; i++)
    if (learn_nmoves[i] > 1)
      nplies++;
  nplies = Max(nplies, 1);
  for (i = 0; i < 64; i++) {
    if (learn_nmoves[i] > 1)
      thisply++;
    book_learn[i] = t_learn_value * (float) thisply / (float) nplies;
  }
/*
 ************************************************************
 *                                                          *
 *  Now find the appropriate cluster, find the key we were  *
 *  passed, and update the resulting learn value.           *
 *                                                          *
 ************************************************************
 */
  for (i = 0; i < 64 && learn_seekto[i]; i++) {
    if (learn_seekto[i] > 0) {
      fseek(book_file, learn_seekto[i], SEEK_SET);
      v = fread(buf32, 4, 1, book_file);
      if (v <= 0)
        perror("Learn() fread error: ");
      cluster = BookIn32(buf32);
      if (cluster)
        BookClusterIn(book_file, cluster, book_buffer);
      for (j = 0; j < cluster; j++)
        if (!(learn_key[i] ^ book_buffer[j].position))
          break;
      if (j >= cluster)
        return;
      if (fabs(book_buffer[j].learn) < 0.0001)
        book_buffer[j].learn = book_learn[i];
      else
        book_buffer[j].learn = (book_buffer[j].learn + book_learn[i]) / 2.0;
      fseek(book_file, learn_seekto[i] + 4, SEEK_SET);
      if (cluster)
        BookClusterOut(book_file, cluster, book_buffer);
      fflush(book_file);
    }
  }
}

/* last modified 02/24/14 */
/*
 *******************************************************************************
 *                                                                             *
 *   LearnFunction() is called to compute the adjustment value added to the    *
 *   learn counter in the opening book.  It takes three pieces of information  *
 *   into consideration to do this:  the search value, the search depth that   *
 *   produced this value, and the rating difference (Crafty-opponent) so that  *
 *   + numbers means Crafty is expected to win, - numbers mean Crafty is ex-   *
 *   pected to lose.                                                           *
 *                                                                             *
 *******************************************************************************
 */
int LearnFunction(int sv, int search_depth, int rating_difference,
    int trusted_value) {
  float multiplier;
  static const float rating_mult_t[11] = { .00625, .0125, .025, .05, .075, .1,
    0.15, 0.2, 0.25, 0.3, 0.35
  };
  static const float rating_mult_ut[11] = { .25, .2, .15, .1, .05, .025, .012,
    .006, .003, .001
  };
  int sd, rd, value;

  sd = Max(Min(search_depth - 10, 19), 0);
  rd = Max(Min(rating_difference / 200, 5), -5) + 5;
  if (trusted_value)
    multiplier = rating_mult_t[rd] * sd;
  else
    multiplier = rating_mult_ut[rd] * sd;
  sv = Max(Min(sv, 5 * learning), -5 * learning);
  value = (int) (sv * multiplier);
  return value;
}

/* last modified 02/24/14 */
/*
 *******************************************************************************
 *                                                                             *
 *   LearnValue() is used to monitor the scores over the first N moves out of  *
 *   book.  After these moves have been played, the evaluations are then used  *
 *   to decide whether the last book move played was a reasonable choice or    *
 *   not.  (N is set by the #define LEARN_INTERVAL definition.)                *
 *                                                                             *
 *   This procedure does not directly update the book.  Rather, it sets the    *
 *   global learn_value variable to represent the goodness or badness of the   *
 *   position where we left the opening book.  This will be used later to      *
 *   update the book in the event the game ends without any sort of actual     *
 *   result.  In a normal situation, we will base our learning on the result   *
 *   of the game, win-lose-draw.  But it is possible that the game ends before *
 *   the final result is known.  In this case, we will use the score from the  *
 *   learn_value we compute here so that we learn _something_ from playing a   *
 *   game fragment.                                                            *
 *                                                                             *
 *   There are three cases to be handled.  (1) If the evaluation is bad right  *
 *   out of book, or it drops enough to be considered a bad line, then the     *
 *   book move will have its "learn" value reduced to discourage playing this  *
 *   move again.  (2) If the evaluation is even after N moves, then the learn  *
 *   value will be increased, but by a relatively modest amount, so that a few *
 *   even results will offset one bad result.  (3) If the evaluation is very   *
 *   good after N moves, the learn value will be increased by a large amount   *
 *   so that this move will be favored the next time the game is played.       *
 *                                                                             *
 *******************************************************************************
 */
void LearnValue(int search_value, int search_depth) {
  int i, interval;
  int best_eval = -999999, best_eval_p = 0;
  int worst_eval = 999999, worst_eval_p = 0;
  int best_after_worst_eval = -999999, worst_after_best_eval = 999999;

/*
 ************************************************************
 *                                                          *
 *  If we have not been "out of book" for N moves, all we   *
 *  need to do is take the search evaluation for the search *
 *  just completed and tuck it away in the book learning    *
 *  array (book_learn_eval[]) for use later.                *
 *                                                          *
 ************************************************************
 */
  if (!book_file)
    return;
  if (!learn || learn_value != 0)
    return;
  if (moves_out_of_book <= LEARN_INTERVAL) {
    if (moves_out_of_book) {
      book_learn_eval[moves_out_of_book - 1] = search_value;
      book_learn_depth[moves_out_of_book - 1] = search_depth;
    }
  }
/*
 ************************************************************
 *                                                          *
 *  Check the evaluations we've seen so far.  If they are   *
 *  within reason (+/- 1/3 of a pawn or so) we simply keep  *
 *  playing and leave the book alone.  If the eval is much  *
 *  better or worse, we need to update the learning data.   *
 *                                                          *
 ************************************************************
 */
  else if (moves_out_of_book == LEARN_INTERVAL + 1) {
    if (moves_out_of_book < 1)
      return;
    interval = Min(LEARN_INTERVAL, moves_out_of_book);
    if (interval < 2)
      return;
    for (i = 0; i < interval; i++) {
      if (book_learn_eval[i] > best_eval) {
        best_eval = book_learn_eval[i];
        best_eval_p = i;
      }
      if (book_learn_eval[i] < worst_eval) {
        worst_eval = book_learn_eval[i];
        worst_eval_p = i;
      }
    }
    if (best_eval_p < interval - 1) {
      for (i = best_eval_p; i < interval; i++)
        if (book_learn_eval[i] < worst_after_best_eval)
          worst_after_best_eval = book_learn_eval[i];
    } else
      worst_after_best_eval = book_learn_eval[interval - 1];
    if (worst_eval_p < interval - 1) {
      for (i = worst_eval_p; i < interval; i++)
        if (book_learn_eval[i] > best_after_worst_eval)
          best_after_worst_eval = book_learn_eval[i];
    } else
      best_after_worst_eval = book_learn_eval[interval - 1];
/*
 ************************************************************
 *                                                          *
 *  We now have the best eval for the first N moves out of  *
 *  book, the worst eval for the first N moves out of book, *
 *  and the worst eval that follows the best eval.  This    *
 *  will be used to recognize the following cases of        *
 *  results that follow a book move:                        *
 *                                                          *
 ************************************************************
 */
/*
 ************************************************************
 *                                                          *
 *  (1) The best score is very good, and it doesn't drop    *
 *  after following the game further.  This case detects    *
 *  those moves in book that are "good" and should be       *
 *  played whenever possible, while avoiding the sound      *
 *  gambits that leave us ahead in material for a short     *
 *  while until the score starts to drop as the gambit      *
 *  begins to show its effect.                              *
 *                                                          *
 ************************************************************
 */
    if (best_eval == best_after_worst_eval) {
      learn_value = best_eval;
      for (i = 0; i < interval; i++)
        if (learn_value == book_learn_eval[i])
          search_depth = Max(search_depth, book_learn_depth[i]);
    }
/*
 ************************************************************
 *                                                          *
 *  (2) The worst score is bad, and doesn't improve any     *
 *  after the worst point, indicating that the book move    *
 *  chosen was "bad" and should be avoided in the future.   *
 *                                                          *
 ************************************************************
 */
    else if (worst_eval == worst_after_best_eval) {
      learn_value = worst_eval;
      for (i = 0; i < interval; i++)
        if (learn_value == book_learn_eval[i])
          search_depth = Max(search_depth, book_learn_depth[i]);
    }
/*
 ************************************************************
 *                                                          *
 *  (3) Things seem even out of book and remain that way    *
 *  for N moves.  We will just average the 10 scores and    *
 *  use that as an approximation.                           *
 *                                                          *
 ************************************************************
 */
    else {
      learn_value = 0;
      search_depth = 0;
      for (i = 0; i < interval; i++) {
        learn_value += book_learn_eval[i];
        search_depth += book_learn_depth[i];
      }
      learn_value /= interval;
      search_depth /= interval;
    }
    learn_value =
        LearnFunction(learn_value, search_depth,
        crafty_rating - opponent_rating, learn_value < 0);
  }
}

#include "chess.h"
#include "data.h"
/* last modified 08/03/16 */
/*
 *******************************************************************************
 *                                                                             *
 *   Repeat() is used to detect a draw by repetition.  The repetition list is  *
 *   a simple 1d array that contains the Zobrist signatures for each position  *
 *   reached in the game since the last irreversable moves at the root.  New   *
 *   positions are only added to this list here in Repeat().                   *
 *                                                                             *
 *   Repeat() scans the list to determine if this position has occurred at     *
 *   once previously (two-fold repetition.)  If so, this will be treated as a  *
 *   draw by Search()/Quiesce().                                               *
 *                                                                             *
 *   Repeat() also handles 50-move draws.  The position[] structure contains   *
 *   the count of moves since the last capture or pawn push.  When this value  *
 *   reaches 100 (plies, which is 50 moves) the score is set to DRAW.          *
 *                                                                             *
 *   Repeat() has one tricky issue.  MakeMoveRoot() has to insert moves into   *
 *   the repetition list so that things will work well.  We end up with the    *
 *   list having entries from 0 through "rep_index" where the last entry is    *
 *   the entry stored after the last actual move in the game.  Search() will   *
 *   call Repeat() at ply=1, which stores the SAME position at position        *
 *   rep_index + 1.  That gets the list out of sync.  To solve this, Iterate() *
 *   decrements rep_index immediately prior to calling Search(), and it then   *
 *   increments it immediately after Search() returns.  This causes Search()   *
 *   to overwrite the entry with the same value for ply=1, but now the list is *
 *   in a sane state for the rest of the search.  Do NOT remove those to lines *
 *   in Iterate() or repetition detection will be broken.                      *
 *                                                                             *
 *******************************************************************************
 */
int Repeat(TREE * RESTRICT tree, int ply) {
  int where, count;

/*
 ************************************************************
 *                                                          *
 *  If the 50-move rule has been reached, then adjust the   *
 *  score to reflect the impending draw.  If we have not    *
 *  made 2 moves for each side (or more) since the last     *
 *  irreversible move, there is no way to repeat a prior    *
 *  position.                                               *
 *                                                          *
 ************************************************************
 */
  tree->rep_list[rep_index + ply] = HashKey;
  if (Reversible(ply) < 4)
    return 0;
  if (Reversible(ply) > 99)
    return 3;
/*
 ************************************************************
 *                                                          *
 *  Now we scan the right part of the repetition list,      *
 *  which is to search backward from the entry for 2 plies  *
 *  back (no point in checking current position, we KNOW    *
 *  that is a match but it is not a repetition, it is just  *
 *  the current position's entry which we don't want to     *
 *  check).  We can use the reversible move counter as a    *
 *  limit since there is no point in scanning the list back *
 *  beyond the last capture/pawn move since there can not   *
 *  possibly be a repeat beyond that point.                 *
 *                                                          *
 ************************************************************
 */
  count = Reversible(ply) / 2 - 1;
  for (where = rep_index + ply - 4; count; where -= 2, count--) {
    if (HashKey == tree->rep_list[where])
      return 2;
  }
  return 0;
}

/* last modified 08/03/16 */
/*
 *******************************************************************************
 *                                                                             *
 *   Repeat3x() is used to detect a real draw by repetition.  This routine is  *
 *   only called from Main() and simply scans the complete list searching for  *
 *   exactly three repetitions (two additional repetitions of the current      *
 *   position.)  This is used to actually claim a draw by repetition or by the *
 *   50 move rule.                                                             *
 *                                                                             *
 *******************************************************************************
 */
int Repeat3x(TREE * RESTRICT tree) {
  int reps = 0, where;

/*
 ************************************************************
 *                                                          *
 *  If the 50-move rule has been reached, then return an    *
 *  indicator that a draw can be claimed if wanted.         *
 *                                                          *
 ************************************************************
 */
  if (Reversible(0) > 99)
    return 2;
/*
 ************************************************************
 *                                                          *
 *  Scan the repetition list to determine if this position  *
 *  has occurred two times previously, making this an       *
 *  actual 3-fold repetition.  Note we only check every     *
 *  other entry since the position has to be repeated 3x    *
 *  with the SAME side on move.                             *
 *                                                          *
 ************************************************************
 */
  for (where = rep_index % 2; where < rep_index; where += 2)
    if (HashKey == tree->rep_list[where])
      reps++;
  return reps == 2;
}

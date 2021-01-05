#include <math.h>
#include "chess.h"
#include "data.h"
#if defined(UNIX)
#  include <unistd.h>
#endif
/* last modified 05/08/14 */
/*
 *******************************************************************************
 *                                                                             *
 *   Book() is used to determine if the current position is in the book data-  *
 *   base.  It simply takes the set of moves produced by root_moves() and then *
 *   tries each position's hash key to see if it can be found in the data-     *
 *   base.  If so, such a move represents a "book move."  The set of flags is  *
 *   used to decide on a sub-set of moves to be used as the "book move pool"   *
 *   from which a move is chosen randomly.                                     *
 *                                                                             *
 *   The format of a book position is as follows:                              *
 *                                                                             *
 *   64 bits:  hash key for this position.                                     *
 *                                                                             *
 *    8 bits:  flag bits defined as  follows:                                  *
 *                                                                             *
 *        0000 0001  ?? flagged move                         (0x01)            *
 *        0000 0010   ? flagged move                         (0x02)            *
 *        0000 0100   = flagged move                         (0x04)            *
 *        0000 1000   ! flagged move                         (0x08)            *
 *        0001 0000  !! flagged move                         (0x10)            *
 *        0010 0000     black won at least 1 game            (0x20)            *
 *        0100 0000     at least one game was drawn          (0x40)            *
 *        1000 0000     white won at least 1 game            (0x80)            *
 *                                                                             *
 *   24 bits:  number of games this move was played.                           *
 *                                                                             *
 *   32 bits:  learned value (floating point).                                 *
 *                                                                             *
 *     (Note:  counts are normalized to a max of 255.                          *
 *                                                                             *
 *******************************************************************************
 */
#define BAD_MOVE  0x02
#define GOOD_MOVE 0x08
int Book(TREE * RESTRICT tree, int wtm) {
  static int book_moves[200];
  static BOOK_POSITION start_moves[200];
  static uint64_t selected_key[200];
  static int selected[200];
  static int selected_order_played[200], selected_value[200];
  static int selected_status[200], selected_percent[200],
      book_development[200];
  static int bs_played[200], bs_percent[200];
  static int book_status[200], evaluations[200], bs_learn[200];
  static float bs_value[200], total_value;
  static uint64_t book_key[200], bs_key[200];
  int m1_status, forced = 0, total_percent, play_percentage = 0;
  float tempr;
  int done, i, j, last_move, temp, which, minlv = 999999, maxlv = -999999;
  int maxp = -999999, minev = 999999, maxev = -999999;
  int nflagged, im, value, np, book_ponder_move;
  int cluster, scluster, test, v;
  unsigned char buf32[4];
  uint64_t temp_hash_key, common, tempk;
  int key, nmoves, num_selected, st;
  int percent_played, total_played, total_moves, smoves;
  int distribution;
  int initial_development;
  char *kibitz_p;

/*
 ************************************************************
 *                                                          *
 *  If we have been out of book for several moves, return   *
 *  and start the normal tree search.                       *
 *                                                          *
 ************************************************************
 */
  if (moves_out_of_book > 6)
    return 0;
/*
 ************************************************************
 *                                                          *
 *  Position is known, read the start book file and save    *
 *  each move found.  These will be used later to augment   *
 *  the flags in the normal book to offer better control.   *
 *                                                          *
 ************************************************************
 */
  test = HashKey >> 49;
  smoves = 0;
  if (books_file) {
    fseek(books_file, test * sizeof(int), SEEK_SET);
    v = fread(buf32, 4, 1, books_file);
    if (v <= 0)
      perror("Book() fread error: ");
    key = BookIn32(buf32);
    if (key > 0) {
      fseek(books_file, key, SEEK_SET);
      v = fread(buf32, 4, 1, books_file);
      if (v <= 0)
        perror("Book() fread error: ");
      scluster = BookIn32(buf32);
      if (scluster)
        BookClusterIn(books_file, scluster, book_buffer);
      for (im = 0; im < n_root_moves; im++) {
        common = HashKey & ((uint64_t) 65535 << 48);
        MakeMove(tree, 1, wtm, root_moves[im].move);
        if (Repeat(tree, 2)) {
          UnmakeMove(tree, 1, wtm, root_moves[im].move);
          return 0;
        }
        temp_hash_key = (wtm) ? HashKey : ~HashKey;
        temp_hash_key = (temp_hash_key & ~((uint64_t) 65535 << 48)) | common;
        for (i = 0; i < scluster; i++)
          if (!(temp_hash_key ^ book_buffer[i].position)) {
            start_moves[smoves++] = book_buffer[i];
            break;
          }
        UnmakeMove(tree, 1, wtm, root_moves[im].move);
      }
    }
  }
/*
 ************************************************************
 *                                                          *
 *  Position is known, read in the appropriate cluster.     *
 *  Note that this cluster will have all possible book      *
 *  moves from current position in it (as well as others of *
 *  course.)                                                *
 *                                                          *
 ************************************************************
 */
  test = HashKey >> 49;
  if (book_file) {
    fseek(book_file, test * sizeof(int), SEEK_SET);
    v = fread(buf32, 4, 1, book_file);
    if (v <= 0)
      perror("Book() fread error: ");
    key = BookIn32(buf32);
    if (key > 0) {
      book_learn_seekto = key;
      fseek(book_file, key, SEEK_SET);
      v = fread(buf32, 4, 1, book_file);
      if (v <= 0)
        perror("Book() fread error: ");
      cluster = BookIn32(buf32);
      if (cluster)
        BookClusterIn(book_file, cluster, book_buffer);
    } else
      cluster = 0;
    if (!cluster && !smoves)
      return 0;
/*
 ************************************************************
 *                                                          *
 *  Now add any moves from books.bin to the end of the      *
 *  cluster so that they will be played even if not in the  *
 *  regular database of moves.                              *
 *                                                          *
 ************************************************************
 */
    for (i = 0; i < smoves; i++) {
      for (j = 0; j < cluster; j++)
        if (!(book_buffer[j].position ^ start_moves[i].position))
          break;
      if (j >= cluster) {
        book_buffer[cluster] = start_moves[i];
        book_buffer[cluster].status_played =
            book_buffer[cluster].status_played & 037700000000;
        cluster++;
      }
    }
/*
 ************************************************************
 *                                                          *
 *  First cycle through the root move list, make each move, *
 *  and see if the resulting hash key is in the book        *
 *  database.                                               *
 *                                                          *
 ************************************************************
 */
    initial_development = tree->score_mg;
    EvaluateCastling(tree, 1, wtm);
    initial_development = tree->score_mg - initial_development;
    total_moves = 0;
    nmoves = 0;
    for (im = 0; im < n_root_moves; im++) {
      common = HashKey & ((uint64_t) 65535 << 48);
      MakeMove(tree, 1, wtm, root_moves[im].move);
      if (Repeat(tree, 2)) {
        UnmakeMove(tree, 1, wtm, root_moves[im].move);
        return 0;
      }
      temp_hash_key = (wtm) ? HashKey : ~HashKey;
      temp_hash_key = (temp_hash_key & ~((uint64_t) 65535 << 48)) | common;
      for (i = 0; i < cluster; i++) {
        if (!(temp_hash_key ^ book_buffer[i].position)) {
          book_status[nmoves] = book_buffer[i].status_played >> 24;
          bs_played[nmoves] = book_buffer[i].status_played & 077777777;
          bs_learn[nmoves] = (int) (book_buffer[i].learn * 100.0);
          if (puzzling)
            bs_played[nmoves] += 1;
          tree->curmv[1] = root_moves[im].move;
          if (!Captured(root_moves[im].move)) {
            book_development[nmoves] = tree->score_mg;
            EvaluateCastling(tree, 2, wtm);
            book_development[nmoves] =
                tree->score_mg - book_development[nmoves];
          } else
            book_development[nmoves] = 0;
          total_moves += bs_played[nmoves];
          evaluations[nmoves] = Evaluate(tree, 2, wtm, -99999, 99999);
          evaluations[nmoves] -= MaterialSTM(wtm);
          bs_percent[nmoves] = 0;
          for (j = 0; j < smoves; j++) {
            if (!(book_buffer[i].position ^ start_moves[j].position)) {
              book_status[nmoves] |= start_moves[j].status_played >> 24;
              bs_percent[nmoves] = start_moves[j].status_played & 077777777;
              break;
            }
          }
          book_moves[nmoves] = root_moves[im].move;
          book_key[nmoves] = temp_hash_key;
          nmoves++;
          break;
        }
      }
      UnmakeMove(tree, 1, wtm, root_moves[im].move);
    }
    if (!nmoves)
      return 0;
    book_learn_nmoves = nmoves;
/*
 ************************************************************
 *                                                          *
 *  If any moves have a very bad or a very good learn       *
 *  value, set the appropriate ? or ! flag so the move be   *
 *  played or avoided as appropriate.                       *
 *                                                          *
 ************************************************************
 */
    for (i = 0; i < nmoves; i++)
      if (!(book_status[i] & BAD_MOVE))
        maxp = Max(maxp, bs_played[i]);
    for (i = 0; i < nmoves; i++) {
      if (bs_learn[i] <= LEARN_COUNTER_BAD && !bs_percent[i]
          && !(book_status[i] & 0x18))
        book_status[i] |= BAD_MOVE;
      if (wtm && !(book_status[i] & 0x80) && !bs_percent[i]
          && !(book_status[i] & 0x18))
        book_status[i] |= BAD_MOVE;
      if (!wtm && !(book_status[i] & 0x20) && !bs_percent[i]
          && !(book_status[i] & 0x18))
        book_status[i] |= BAD_MOVE;
      if (bs_played[i] < maxp / 10 && !bs_percent[i] && book_random &&
          !(book_status[i] & 0x18))
        book_status[i] |= BAD_MOVE;
      if (bs_learn[i] >= LEARN_COUNTER_GOOD && !(book_status[i] & 0x03))
        book_status[i] |= GOOD_MOVE;
      if (bs_percent[i])
        book_status[i] |= GOOD_MOVE;
    }
/*
 ************************************************************
 *                                                          *
 *  We have the book moves, now it's time to decide how     *
 *  they are supposed to be sorted and compute the sort     *
 *  index.                                                  *
 *                                                          *
 ************************************************************
 */
    for (i = 0; i < nmoves; i++) {
      if (!(book_status[i] & BAD_MOVE)) {
        minlv = Min(minlv, bs_learn[i]);
        maxlv = Max(maxlv, bs_learn[i]);
        minev = Min(minev, evaluations[i]);
        maxev = Max(maxev, evaluations[i]);
        maxp = Max(maxp, bs_played[i]);
      }
    }
    maxp++;
    for (i = 0; i < nmoves; i++) {
      bs_value[i] = 1;
      bs_value[i] += bs_played[i] / (float) maxp *1000.0 * book_weight_freq;

      if (minlv < maxlv)
        bs_value[i] +=
            (bs_learn[i] - minlv) / (float) (maxlv -
            minlv) * 1000.0 * book_weight_learn;
      if (minev < maxev)
        bs_value[i] +=
            (evaluations[i] - minev) / (float) (Max(maxev - minev,
                50)) * 1000.0 * book_weight_eval;
    }
    total_played = total_moves;
/*
 ************************************************************
 *                                                          *
 *  If there are any ! moves, make their popularity count   *
 *  huge since they have to be considered.                  *
 *                                                          *
 ************************************************************
 */
    for (i = 0; i < nmoves; i++)
      if (book_status[i] & 0x18)
        break;
    if (i < nmoves) {
      for (i = 0; i < nmoves; i++) {
        if (book_status[i] & 0x18)
          bs_value[i] += 8000.0;
        if (!(book_status[i] & 0x03))
          bs_value[i] += 4000.0;
      }
    }
/*
 ************************************************************
 *                                                          *
 *  Now sort the moves based on the complete sort value.    *
 *                                                          *
 ************************************************************
 */
    if (nmoves)
      do {
        done = 1;
        for (i = 0; i < nmoves - 1; i++) {
          if (bs_percent[i] < bs_percent[i + 1]
              || (bs_percent[i] == bs_percent[i + 1]
                  && bs_value[i] < bs_value[i + 1])) {
            tempr = bs_played[i];
            bs_played[i] = bs_played[i + 1];
            bs_played[i + 1] = tempr;
            tempr = bs_value[i];
            bs_value[i] = bs_value[i + 1];
            bs_value[i + 1] = tempr;
            temp = evaluations[i];
            evaluations[i] = evaluations[i + 1];
            evaluations[i + 1] = temp;
            temp = bs_learn[i];
            bs_learn[i] = bs_learn[i + 1];
            bs_learn[i + 1] = temp;
            temp = book_development[i];
            book_development[i] = book_development[i + 1];
            book_development[i + 1] = temp;
            temp = book_moves[i];
            book_moves[i] = book_moves[i + 1];
            book_moves[i + 1] = temp;
            temp = book_status[i];
            book_status[i] = book_status[i + 1];
            book_status[i + 1] = temp;
            temp = bs_percent[i];
            bs_percent[i] = bs_percent[i + 1];
            bs_percent[i + 1] = temp;
            tempk = book_key[i];
            book_key[i] = book_key[i + 1];
            book_key[i + 1] = tempk;
            done = 0;
          }
        }
      } while (!done);
/*
 ************************************************************
 *                                                          *
 *  Display the book moves, and total counts, etc. if the   *
 *  operator has requested it.                              *
 *                                                          *
 ************************************************************
 */
    if (show_book) {
      Print(32, "  after screening, the following moves can be played\n");
      Print(32,
          "  move     played    %%  score    learn    " "sortv   P%%  P\n");
      for (i = 0; i < nmoves; i++) {
        Print(32, "%6s", OutputMove(tree, 1, wtm, book_moves[i]));
        st = book_status[i];
        if (st & 0x1f) {
          if (st & 0x01)
            Print(32, "??");
          else if (st & 0x02)
            Print(32, "? ");
          else if (st & 0x04)
            Print(32, "= ");
          else if (st & 0x08)
            Print(32, "! ");
          else if (st & 0x10)
            Print(32, "!!");
        } else
          Print(32, "  ");
        Print(32, "   %6d", bs_played[i]);
        Print(32, "  %3d", 100 * bs_played[i] / Max(total_moves, 1));
        Print(32, "%s", DisplayEvaluation(evaluations[i], wtm));
        Print(32, "%9.2f", (float) bs_learn[i] / 100.0);
        Print(32, " %9.1f", bs_value[i]);
        Print(32, " %3d", bs_percent[i]);
        if ((book_status[i] & book_accept_mask &&
                !(book_status[i] & book_reject_mask))
            || (!(book_status[i] & book_reject_mask) && (bs_percent[i]
                    || book_status[i] & 0x18 || (wtm && book_status[i] & 0x80)
                    || (!wtm && book_status[i] & 0x20))))
          Print(32, "  Y");
        else
          Print(32, "  N");
        Print(32, "\n");
      }
    }
/*
 ************************************************************
 *                                                          *
 *  Check for book moves with the play % value set.  if     *
 *  there are any such moves, then exclude all moves that   *
 *  do not have a play % or a !/!! flag set.                *
 *                                                          *
 ************************************************************
 */
    for (i = 0; i < nmoves; i++)
      if (bs_percent[i])
        play_percentage = 1;
/*
 ************************************************************
 *                                                          *
 *  Delete ? and ?? moves first, which includes those moves *
 *  with bad learned results.  Here is where we also        *
 *  exclude moves with no play % if we find at least one    *
 *  with a non-zero value.                                  *
 *                                                          *
 ************************************************************
 */
    num_selected = 0;
    if (!play_percentage) {
      for (i = 0; i < nmoves; i++)
        if (!(book_status[i] & 0x03) || bs_percent[i]) {
          selected_status[num_selected] = book_status[i];
          selected_order_played[num_selected] = bs_played[i];
          selected_value[num_selected] = bs_value[i];
          selected_percent[num_selected] = bs_percent[i];
          selected_key[num_selected] = book_key[i];
          selected[num_selected++] = book_moves[i];
        }
    } else {
      for (i = 0; i < nmoves; i++)
        if (bs_percent[i]) {
          selected_status[num_selected] = book_status[i];
          selected_order_played[num_selected] = bs_played[i];
          selected_value[num_selected] = bs_value[i];
          selected_percent[num_selected] = bs_percent[i];
          selected_key[num_selected] = book_key[i];
          selected[num_selected++] = book_moves[i];
        }
    }
    for (i = 0; i < num_selected; i++) {
      book_status[i] = selected_status[i];
      bs_played[i] = selected_order_played[i];
      bs_value[i] = selected_value[i];
      bs_percent[i] = selected_percent[i];
      book_moves[i] = selected[i];
    }
    nmoves = num_selected;
/*
 ************************************************************
 *                                                          *
 *  If this is a real search (not a puzzling search to      *
 *  find a move by the opponent to ponder) then we need to  *
 *  set up the whisper info for later.                      *
 *                                                          *
 ************************************************************
 */
    if (!puzzling)
      do {
        kibitz_text[0] = '\0';
        if (!nmoves)
          break;
        sprintf(kibitz_text, "book moves (");
        kibitz_p = kibitz_text + strlen(kibitz_text);
        for (i = 0; i < nmoves; i++) {
          sprintf(kibitz_p, "%s %d%%", OutputMove(tree, 1, wtm,
                  book_moves[i]), 100 * bs_played[i] / Max(total_played, 1));
          kibitz_p = kibitz_text + strlen(kibitz_text);
          if (i < nmoves - 1) {
            sprintf(kibitz_p, ", ");
            kibitz_p = kibitz_text + strlen(kibitz_text);
          }
        }
        sprintf(kibitz_p, ")\n");
      } while (0);
/*
 ************************************************************
 *                                                          *
 *  Now select a move from the set of moves just found. Do  *
 *  this in three distinct passes:  (1) look for !! moves;  *
 *  (2) look for ! moves;  (3) look for any other moves.    *
 *  Note: book_accept_mask *should* have a bit set for any  *
 *  move that is selected, including !! and ! type moves so *
 *  that they *can* be excluded if desired.  Note also that *
 *  book_reject_mask should have ?? and ? set (at a         *
 *  minimum) to exclude these types of moves.               *
 *                                                          *
 ************************************************************
 */
    num_selected = 0;
    if (!num_selected && !puzzling)
      if (book_accept_mask & 16)
        for (i = 0; i < nmoves; i++)
          if (book_status[i] & 16) {
            forced = 1;
            selected_status[num_selected] = book_status[i];
            selected_order_played[num_selected] = bs_played[i];
            selected_value[num_selected] = bs_value[i];
            selected_key[num_selected] = book_key[i];
            selected[num_selected++] = book_moves[i];
          }
    if (!num_selected && !puzzling)
      if (book_accept_mask & 8)
        for (i = 0; i < nmoves; i++)
          if (book_status[i] & 8) {
            forced = 1;
            selected_status[num_selected] = book_status[i];
            selected_order_played[num_selected] = bs_played[i];
            selected_value[num_selected] = bs_value[i];
            selected_key[num_selected] = book_key[i];
            selected[num_selected++] = book_moves[i];
          }
    if (!num_selected && !puzzling)
      if (book_accept_mask & 4)
        for (i = 0; i < nmoves; i++)
          if (book_status[i] & 4) {
            selected_status[num_selected] = book_status[i];
            selected_order_played[num_selected] = bs_played[i];
            selected_value[num_selected] = bs_value[i];
            selected_key[num_selected] = book_key[i];
            selected[num_selected++] = book_moves[i];
          }
    if (!num_selected && !puzzling)
      for (i = 0; i < nmoves; i++)
        if (book_status[i] & book_accept_mask) {
          selected_status[num_selected] = book_status[i];
          selected_order_played[num_selected] = bs_played[i];
          selected_value[num_selected] = bs_value[i];
          selected_key[num_selected] = book_key[i];
          selected[num_selected++] = book_moves[i];
        }
    if (!num_selected)
      for (i = 0; i < nmoves; i++) {
        selected_status[num_selected] = book_status[i];
        selected_order_played[num_selected] = bs_played[i];
        selected_value[num_selected] = bs_value[i];
        selected_key[num_selected] = book_key[i];
        selected[num_selected++] = book_moves[i];
      }
    if (!num_selected)
      return 0;
    for (i = 0; i < num_selected; i++) {
      book_status[i] = selected_status[i];
      book_moves[i] = selected[i];
      bs_played[i] = selected_order_played[i];
      bs_value[i] = selected_value[i];
      bs_key[i] = selected_key[i];
    }
    nmoves = num_selected;
    if (nmoves == 0)
      return 0;
    Print(32, "               book moves {");
    for (i = 0; i < nmoves; i++) {
      Print(32, "%s", OutputMove(tree, 1, wtm, book_moves[i]));
      if (i < nmoves - 1)
        Print(32, ", ");
    }
    Print(32, "}\n");
    nflagged = 0;
    for (i = 0; i < nmoves; i++)
      if (book_status[i] & 8)
        nflagged++;
    nmoves = Max(Min(nmoves, book_selection_width), nflagged);
    if (show_book) {
      Print(32, "               moves considered {");
      for (i = 0; i < nmoves; i++) {
        Print(32, "%s", OutputMove(tree, 1, wtm, book_moves[i]));
        if (i < nmoves - 1)
          Print(32, ", ");
      }
      Print(32, "}\n");
    }
/*
 ************************************************************
 *                                                          *
 *  We have the book moves, if any have specified percents  *
 *  for play, then adjust the bs_value[] to reflect this    *
 *  percentage.                                             *
 *                                                          *
 ************************************************************
 */
    total_value = 0.0;
    total_percent = 0;
    for (i = 0; i < nmoves; i++) {
      if (!bs_percent[i])
        total_value += bs_value[i];
      total_percent += bs_percent[i];
    }
    if (fabs(total_value) < 0.0001)
      total_value = 1000.0;
    total_percent = (total_percent > 99) ? 99 : total_percent;
    for (i = 0; i < nmoves; i++)
      if (bs_percent[i])
        bs_value[i] =
            total_value / (1.0 -
            (float) total_percent / 100.0) * (float) bs_percent[i] / 100.0;
/*
 ************************************************************
 *                                                          *
 *  Display the book moves, and total counts, etc. if the   *
 *  operator has requested it.                              *
 *                                                          *
 ************************************************************
 */
    if (show_book) {
      Print(32, "  move     played    %%  score     sortv  P%%  P\n");
      for (i = 0; i < nmoves; i++) {
        Print(32, "%6s", OutputMove(tree, 1, wtm, book_moves[i]));
        st = book_status[i];
        if (st & 0x1f) {
          if (st & 0x01)
            Print(32, "??");
          else if (st & 0x02)
            Print(32, "? ");
          else if (st & 0x04)
            Print(32, "= ");
          else if (st & 0x08)
            Print(32, "! ");
          else if (st & 0x10)
            Print(32, "!!");
        } else
          Print(32, "  ");
        Print(32, "   %6d", bs_played[i]);
        Print(32, "  %3d", 100 * bs_played[i] / Max(total_moves, 1));
        Print(32, "%s", DisplayEvaluation(evaluations[i], wtm));
        Print(32, " %9.1f", bs_value[i]);
        Print(32, " %3d", bs_percent[i]);
        if ((book_status[i] & book_accept_mask &&
                !(book_status[i] & book_reject_mask))
            || (!(book_status[i] & book_reject_mask) && ((wtm &&
                        book_status[i] & 0x80) || (!wtm &&
                        book_status[i] & 0x20))))
          Print(32, "  Y");
        else
          Print(32, "  N");
        Print(32, "\n");
      }
    }
/*
 ************************************************************
 *                                                          *
 *  If random=0, then we search the set of legal book moves *
 *  with the normal search engine (but with a short time    *
 *  limit) to choose among them.                            *
 *                                                          *
 ************************************************************
 */
    if (nmoves && (!puzzling || mode != tournament_mode)) {
      np = bs_played[nmoves - 1];
      if (!puzzling && (!book_random || (mode == tournament_mode &&
                  np < book_search_trigger))) {
        if (!forced) {
          n_root_moves = nmoves;
          for (i = 0; i < n_root_moves; i++) {
            root_moves[i].move = book_moves[i];
            root_moves[i].status = 0;
          }
          last_pv.pathd = 0;
          booking = 1;
          value = Iterate(wtm, booking, 1);
          booking = 0;
          if (value < -50) {
            last_pv.pathd = 0;
            return 0;
          }
        } else {
          tree->pv[0].path[1] = book_moves[0];
          tree->pv[0].pathl = 2;
          tree->pv[0].pathd = 0;
          tree->pv[0].pathv = 0;
        }
        return 1;
      }
    }
/*
 ************************************************************
 *                                                          *
 *  If puzzling, in tournament mode we try to find the best *
 *  non-book move, because a book move will produce a quick *
 *  move anyway.  We therefore would rather search for a    *
 *  non-book move, just in case the opponent goes out of    *
 *  book here.                                              *
 *                                                          *
 ************************************************************
 */
    else if (mode == tournament_mode && puzzling) {
      RootMoveList(wtm);
      for (i = 0; i < n_root_moves; i++)
        for (j = 0; j < nmoves; j++)
          if (root_moves[i].move == book_moves[j])
            root_moves[i].move = 0;
      for (i = 0, j = 0; i < n_root_moves; i++)
        if (root_moves[i].move != 0)
          root_moves[j++] = root_moves[i];
      n_root_moves = j;
      Print(32, "               moves considered {only non-book moves}\n");
      nmoves = j;
      if (nmoves > 1) {
        last_pv.pathd = 0;
        booking = 1;
        Iterate(wtm, booking, 1);
        booking = 0;
      } else {
        tree->pv[0].path[1] = book_moves[0];
        tree->pv[0].pathl = 2;
        tree->pv[0].pathd = 0;
      }
      return 1;
    }
    last_move = nmoves;
/*
 ************************************************************
 *                                                          *
 *  Compute a random value and use this to generate a book  *
 *  move based on a probability distribution of the number  *
 *  of games won by each book move.                         *
 *                                                          *
 ************************************************************
 */
    which = Random32();
    j = ReadClock() / 100 % 13;
    for (i = 0; i < j; i++)
      which = Random32();
    total_moves = 0;
    for (i = 0; i < last_move; i++) {
      if (bs_percent[0])
        total_moves += bs_value[i];
      else
        total_moves += bs_value[i] * bs_value[i];
    }
    distribution = Abs(which) % Max(total_moves, 1);
    for (which = 0; which < last_move; which++) {
      if (bs_percent[0])
        distribution -= bs_value[which];
      else
        distribution -= bs_value[which] * bs_value[which];
      if (distribution < 0)
        break;
    }
    which = Min(which, last_move - 1);
    tree->pv[0].path[1] = book_moves[which];
    percent_played = 100 * bs_played[which] / Max(total_played, 1);
    total_played = bs_played[which];
    m1_status = book_status[which];
    tree->pv[0].pathl = 2;
    tree->pv[0].pathd = 0;
    if (mode != tournament_mode) {
      MakeMove(tree, 1, wtm, book_moves[which]);
      if ((book_ponder_move = BookPonderMove(tree, Flip(wtm)))) {
        tree->pv[0].path[2] = book_ponder_move;
        tree->pv[0].pathl = 3;
      }
      UnmakeMove(tree, 1, wtm, book_moves[which]);
    }
    book_learn_key = bs_key[which];
    Print(32, "               book   0.0s    %3d%%   ", percent_played);
    Print(32, " %s", OutputMove(tree, 1, wtm, tree->pv[0].path[1]));
    st = m1_status & book_accept_mask & (~224);
    if (st) {
      if (st & 1)
        Print(32, "??");
      else if (st & 2)
        Print(32, "?");
      else if (st & 4)
        Print(32, "=");
      else if (st & 8)
        Print(32, "!");
      else if (st & 16)
        Print(32, "!!");
    }
    MakeMove(tree, 1, wtm, tree->pv[0].path[1]);
    if (tree->pv[0].pathl > 2)
      Print(32, " %s", OutputMove(tree, 2, Flip(wtm), tree->pv[0].path[2]));
    UnmakeMove(tree, 1, wtm, tree->pv[0].path[1]);
    Print(32, "\n");
    return 1;
  }
  return 0;
}

/* last modified 02/23/14 */
/*
 *******************************************************************************
 *                                                                             *
 *   BookPonderMove() is used to find a move to ponder, to avoid the overhead  *
 *   of a "puzzling" search early in the game (unless there are no book moves  *
 *   found, of course.)  The algorithm is much simpler than the normal book    *
 *   move code...  just find the move with the largest frequency counter and   *
 *   assume that will be played.                                               *
 *                                                                             *
 *******************************************************************************
 */
int BookPonderMove(TREE * RESTRICT tree, int wtm) {
  uint64_t temp_hash_key, common;
  static unsigned book_moves[200];
  int i, v, key, cluster, n_moves, im, played, tplayed;
  unsigned *lastm;
  int book_ponder_move = 0, test;
  unsigned char buf32[4];

/*
 ************************************************************
 *                                                          *
 *  position is known, read in the appropriate cluster.     *
 *  note that this cluster will have all possible book      *
 *  moves from current position in it (as well as others of *
 *  course.)                                                *
 *                                                          *
 ************************************************************
 */
  if (book_file) {
    test = HashKey >> 49;
    fseek(book_file, test * sizeof(int), SEEK_SET);
    v = fread(buf32, 4, 1, book_file);
    if (v <= 0)
      perror("Book() fread error: ");
    key = BookIn32(buf32);
    if (key > 0) {
      fseek(book_file, key, SEEK_SET);
      v = fread(buf32, 4, 1, book_file);
      if (v <= 0)
        perror("Book() fread error: ");
      cluster = BookIn32(buf32);
      if (cluster)
        BookClusterIn(book_file, cluster, book_buffer);
    } else
      cluster = 0;
    if (!cluster)
      return 0;
    lastm = GenerateCaptures(tree, 2, wtm, book_moves);
    lastm = GenerateNoncaptures(tree, 2, wtm, lastm);
    n_moves = lastm - book_moves;
/*
 ************************************************************
 *                                                          *
 *  First cycle through the root move list, make each move, *
 *  and see if the resulting hash key is in the book        *
 *  database.                                               *
 *                                                          *
 ************************************************************
 */
    played = -1;
    for (im = 0; im < n_moves; im++) {
      common = HashKey & ((uint64_t) 65535 << 48);
      MakeMove(tree, 2, wtm, book_moves[im]);
      temp_hash_key = (wtm) ? HashKey : ~HashKey;
      temp_hash_key = (temp_hash_key & ~((uint64_t) 65535 << 48)) | common;
      for (i = 0; i < cluster; i++) {
        if (!(temp_hash_key ^ book_buffer[i].position)) {
          tplayed = book_buffer[i].status_played & 077777777;
          if (tplayed > played) {
            played = tplayed;
            book_ponder_move = book_moves[im];
          }
          break;
        }
      }
      UnmakeMove(tree, 2, wtm, book_moves[im]);
    }
  }
  return book_ponder_move;
}

/* last modified 05/08/14 */
/*
 *******************************************************************************
 *                                                                             *
 *   Bookup() is used to create/add to the opening book file.  typing "<file>  *
 *   create" will erase the old book file and start from scratch,              *
 *                                                                             *
 *   The format of the input data is a left bracket ("[") followed by any      *
 *   title information desired, followed by a right bracket ("]") followed by  *
 *   a sequence of moves.  The sequence of moves is assumed to start at ply=1, *
 *   with white-to-move (normal opening position) and can contain as many      *
 *   moves as desired (no limit on the depth of each variation.)  The file     *
 *   *must* be terminated with a line that begins with "end", since handling   *
 *   the EOF condition makes portable code difficult.                          *
 *                                                                             *
 *   Book moves can either be typed in by hand, directly into book_add(), by   *
 *   using the "book create/add" command.  Using the command "book add/create  *
 *   filename" will cause book_add() to read its opening text moves from       *
 *   filename rather than from the keyboard                                    *
 *                                                                             *
 *   In addition to the normal text for a move (reduced or full algebraic is   *
 *   accepted, ie, e4, ed, exd4, e3d4, etc. are all acceptable) some special   *
 *   characters can be appended to a move.                                     *
 *                                                                             *
 *        ?? ->  Never play this move.  since the same book is used for both   *
 *               black and white, you can enter moves in that white might      *
 *               play, but prevent the program from choosing them on its own.  *
 *        ?  ->  Avoid this move except for non-important games.  These        *
 *               openings are historically those that the program doesn't play *
 *               very well, but which aren't outright losing.                  *
 *        =  ->  Drawish move, only play this move if drawish moves are        *
 *               allowed by the operator.  This is used to encourage the       *
 *               program to play drawish openings (Petrov's comes to mind)     *
 *               when the program needs to draw or is facing a formidable      *
 *               opponent (deep thought comes to mind.)                        *
 *        !  ->  Always play this move, if there isn't a move with the !! flag *
 *               set also.  This is a strong move, but not as strong as a !!   *
 *               move.                                                         *
 *        !! ->  Always play this move.  This can be used to make the program  *
 *               favor particular lines, or to mark a strong move for certain  *
 *               opening traps.                                                *
 *                                                                             *
 *  {Play nn%} is used to force this specific book move to be played a         *
 *  specific percentage of the time, and override the frequency of play that   *
 *               comes from the large pgn database.                            *
 *                                                                             *
 *******************************************************************************
 */
void Bookup(TREE * RESTRICT tree, int nargs, char **args) {
  BB_POSITION *bbuffer;
  uint64_t temp_hash_key, common;
  FILE *book_input;
  char fname[128], start, *ch, output_filename[128];
  static char schar[2] = { "." };
  int result = 0, played, i, mask_word, total_moves;
  int move, move_num, wtm, book_positions, major, minor;
  int cluster, max_cluster, ignored = 0, ignored_mp = 0, ignored_lose =
      0;
  int errors, data_read;
  int start_elapsed_time, ply, max_ply = 256;
  int stat, files = 0, buffered = 0, min_played = 0, games_parsed = 0;
  int wins, losses;
  BOOK_POSITION current, next;
  BB_POSITION temp;
  int last, cluster_seek, next_cluster;
  int counter, *index, max_search_depth;
  double wl_percent = 0.0;

/*
 ************************************************************
 *                                                          *
 *  Open the correct book file for writing/reading          *
 *                                                          *
 ************************************************************
 */
#if defined(POSITIONS)
  unsigned int output_pos, output_wtm;
  FILE *pout = fopen("positions", "w");
#endif
  if (!strcmp(args[1], "create")) {
    if (nargs < 4) {
      Print(4095, "usage:  <binfile> create <pgn-filename> ");
      Print(4095, "maxply [minplay] [win/lose %%]\n");
      return;
    }
    max_ply = atoi(args[3]);
    if (nargs >= 5) {
      min_played = atoi(args[4]);
    }
    if (nargs > 5) {
      wl_percent = atof(args[5]) / 100.0;
    }
    strcpy(output_filename, args[0]);
    if (!strstr(output_filename, ".bin")) {
      strcat(output_filename, ".bin");
    }
  } else if (!strcmp(args[1], "off")) {
    if (book_file)
      fclose(book_file);
    if (books_file)
      fclose(normal_bs_file);
    if (computer_bs_file)
      fclose(computer_bs_file);
    book_file = 0;
    books_file = 0;
    computer_bs_file = 0;
    normal_bs_file = 0;
    Print(4095, "book file disabled.\n");
    return;
  } else if (!strcmp(args[1], "on")) {
    if (!book_file) {
      sprintf(fname, "%s/book.bin", book_path);
      book_file = fopen(fname, "rb+");
      sprintf(fname, "%s/books.bin", book_path);
      books_file = fopen(fname, "rb+");
      Print(4095, "book file enabled.\n");
    }
    return;
  } else if (!strcmp(args[1], "mask")) {
    if (nargs < 4) {
      Print(4095, "usage:  book mask accept|reject value\n");
      return;
    } else if (!strcmp(args[2], "accept")) {
      book_accept_mask = BookMask(args[3]);
      book_reject_mask = book_reject_mask & ~book_accept_mask;
      return;
    } else if (!strcmp(args[2], "reject")) {
      book_reject_mask = BookMask(args[3]);
      book_accept_mask = book_accept_mask & ~book_reject_mask;
      return;
    }
  } else if (!strcmp(args[1], "random")) {
    if (nargs < 3) {
      Print(4095, "usage:  book random <n>\n");
      return;
    }
    book_random = atoi(args[2]);
    switch (book_random) {
      case 0:
        Print(4095, "play best book line after search.\n");
        Print(4095, "  ..book selection width set to 99.\n");
        book_selection_width = 99;
        break;
      case 1:
        Print(4095, "choose from book moves randomly (using weights.)\n");
        break;
      default:
        Print(4095, "valid options are 0-1.\n");
        break;
    }
    return;
  } else if (!strcmp(args[1], "trigger")) {
    if (nargs < 3) {
      Print(4095, "usage:  book trigger <n>\n");
      return;
    }
    book_search_trigger = atoi(args[2]);
    Print(4095, "search book moves if the most popular was not played\n");
    Print(4095, "at least %d times.\n", book_search_trigger);
    return;
  } else if (!strcmp(args[1], "width")) {
    if (nargs < 3) {
      Print(4095, "usage:  book width <n>\n");
      return;
    }
    book_selection_width = atoi(args[2]);
    book_random = 1;
    Print(4095, "choose from %d best moves.\n", book_selection_width);
    Print(4095, "  ..book random set to 1.\n");
    return;
  } else {
    Print(4095, "usage:  book [option] [filename] [maxply] [minplay]\n");
    return;
  }
  if (!(book_input = fopen(args[2], "r"))) {
    printf("file %s does not exist.\n", args[2]);
    return;
  }
  ReadPGN(0, 0);
  if (book_file)
    fclose(book_file);
  book_file = fopen(output_filename, "wb+");
  bbuffer = (BB_POSITION *) malloc(sizeof(BB_POSITION) * SORT_BLOCK);
  if (!bbuffer) {
    Print(4095, "Unable to malloc() sort buffer, aborting\n");
    CraftyExit(1);
  }
  fseek(book_file, 0, SEEK_SET);
/*
 ************************************************************
 *                                                          *
 *  Now, read in a series of moves (terminated by the "["   *
 *  of the next title or by "end" for end of the file) and  *
 *  make them.  After each MakeMove(), we can grab the hash *
 *  key, and use it to access the book data file to add     *
 *  this position.  Note that we have to check the last     *
 *  character of a move for the special flags and set the   *
 *  correct bit in the status for this position.  When we   *
 *  reach the end of a book line, we back up to the         *
 *  starting position and start over.                       *
 *                                                          *
 ************************************************************
 */
  major = atoi(version);
  minor = atoi(strchr(version, '.') + 1);
  major = (major << 16) + minor;
  start = !strstr(output_filename, "book.bin");
  printf("parsing pgn move file (100k moves/dot)\n");
  start_elapsed_time = ReadClock();
  if (book_file) {
    total_moves = 0;
    max_search_depth = 0;
    errors = 0;
    do {
      data_read = ReadPGN(book_input, 0);
      if (data_read == -1)
        Print(4095, "end-of-file reached\n");
    } while (data_read == 0);
    do {
      if (data_read < 0) {
        Print(4095, "end-of-file reached\n");
        break;
      }
      if (data_read == 1) {
        if (strstr(buffer, "Site")) {
          games_parsed++;
          result = 3;
        } else if (strstr(buffer, "esult")) {
          if (strstr(buffer, "1-0"))
            result = 2;
          else if (strstr(buffer, "0-1"))
            result = 1;
          else if (strstr(buffer, "1/2-1/2"))
            result = 0;
          else if (strstr(buffer, "*"))
            result = 3;
        }
        data_read = ReadPGN(book_input, 0);
      } else
        do {
          wtm = 1;
          InitializeChessBoard(tree);
          tree->status[1] = tree->status[0];
          move_num = 1;
          tree->status[2] = tree->status[1];
          ply = 0;
          data_read = 0;
#if defined(POSITIONS)
          output_pos = Random32();
          output_pos = (output_pos | (output_pos >> 16)) & 65535;
          output_pos = output_pos % 20 + 8;
          output_wtm = Random32() & 1;
#endif
          while (data_read == 0) {
            mask_word = 0;
            if ((ch = strpbrk(buffer, "?!"))) {
              mask_word = BookMask(ch);
              *ch = 0;
            }
            if (!strchr(buffer, '$') && !strchr(buffer, '*')) {
              if (ply < max_ply)
                move = ReadNextMove(tree, buffer, 2, wtm);
              else {
                move = 0;
                ignored++;
              }
              if (move) {
                ply++;
                max_search_depth = Max(max_search_depth, ply);
                total_moves++;
                common = HashKey & ((uint64_t) 65535 << 48);
                MakeMove(tree, 2, wtm, move);
                tree->status[2] = tree->status[3];
                if (ply <= max_ply) {
                  temp_hash_key = (wtm) ? HashKey : ~HashKey;
                  temp_hash_key =
                      (temp_hash_key & ~((uint64_t) 65535 << 48)) | common;
                  memcpy(bbuffer[buffered].position, (char *) &temp_hash_key,
                      8);
                  if (result & 1)
                    mask_word |= 0x20;
                  if (result == 0)
                    mask_word |= 0x40;
                  if (result & 2)
                    mask_word |= 0x80;
                  bbuffer[buffered].status = mask_word;
                  bbuffer[buffered++].percent_play =
                      pgn_suggested_percent + (wtm << 7);
                  if (buffered >= SORT_BLOCK) {
                    BookSort(bbuffer, buffered, ++files);
                    buffered = 0;
                    strcpy(schar, "S");
                  }
                }
                if (!(total_moves % 100000)) {
                  printf("%s", schar);
                  strcpy(schar, ".");
                  if (!(total_moves % 6000000))
                    printf(" (%dk)\n", total_moves / 1000);
                  fflush(stdout);
                }
                wtm = Flip(wtm);
                if (wtm)
                  move_num++;
#if defined(POSITIONS)
                if (wtm == output_wtm && move_num == output_pos) {
                  SEARCH_POSITION temp_pos;
                  int twtm;
                  char t_initial_position[256];

                  strcpy(t_initial_position, initial_position);
                  temp_pos = tree->status[0];
                  tree->status[0] = tree->status[3];
                  if (Castle(0, white) < 0)
                    Castle(0, white) = 0;
                  if (Castle(0, black) < 0)
                    Castle(0, black) = 0;
                  strcpy(buffer, "savepos *");
                  twtm = game_wtm;
                  game_wtm = wtm;
                  Option(tree);
                  game_wtm = twtm;
                  fprintf(pout, "%s\n", initial_position);
                  strcpy(initial_position, t_initial_position);
                  tree->status[0] = temp_pos;
                }
#endif
              } else if (strspn(buffer, "0123456789/-.*") != strlen(buffer)
                  && ply < max_ply) {
                errors++;
                Print(4095, "ERROR!  move %d: %s is illegal (line %d)\n",
                    move_num, buffer, ReadPGN(book_input, -2));
                ReadPGN(book_input, -1);
                DisplayChessBoard(stdout, tree->position);
                do {
                  data_read = ReadPGN(book_input, 0);
                  if (data_read == -1)
                    Print(4095, "end-of-file reached\n");
                } while (data_read == 0);
                break;
              }
            }
            data_read = ReadPGN(book_input, 0);
          }
          strcpy(initial_position, "");
        } while (0);
    } while (strcmp(buffer, "end") && data_read != -1);
    if (book_input != stdin)
      fclose(book_input);
    if (buffered)
      BookSort(bbuffer, buffered, ++files);
    free(bbuffer);
    printf("S  <done>\n");
    if (total_moves == 0) {
      Print(4095, "ERROR - empty input PGN file\n");
      return;
    }
/*
 ************************************************************
 *                                                          *
 *  Now merge these "chunks" into book.bin, keeping all of  *
 *  the "flags" as well as counting the number of times     *
 *  that each move was played.                              *
 *                                                          *
 ************************************************************
 */
    printf("merging sorted files (%d) (100k/dot)\n", files);
    counter = 0;
    index = (int *) malloc(32768 * sizeof(int));
    if (!index) {
      Print(4095, "Unable to malloc() index block, aborting\n");
      CraftyExit(1);
    }
    for (i = 0; i < 32768; i++)
      index[i] = -1;
    temp = BookupNextPosition(files, 1);
    memcpy((char *) &current.position, temp.position, 8);
    current.status_played = temp.status << 24;
    if (start)
      current.status_played += temp.percent_play & 127;
    current.learn = 0.0;
    played = 1;
    fclose(book_file);
    book_file = fopen(output_filename, "wb+");
    fseek(book_file, sizeof(int) * 32768, SEEK_SET);
    last = current.position >> 49;
    index[last] = ftell(book_file);
    book_positions = 0;
    cluster = 0;
    cluster_seek = sizeof(int) * 32768;
    fseek(book_file, cluster_seek + sizeof(int), SEEK_SET);
    max_cluster = 0;
    wins = 0;
    losses = 0;
    if (temp.status & 128 && temp.percent_play & 128)
      wins++;
    if (temp.status & 128 && !(temp.percent_play & 128))
      losses++;
    if (temp.status & 32 && !(temp.percent_play & 128))
      wins++;
    if (temp.status & 32 && temp.percent_play & 128)
      losses++;
    while (FOREVER) {
      temp = BookupNextPosition(files, 0);
      memcpy((char *) &next.position, temp.position, 8);
      next.status_played = temp.status << 24;
      if (start)
        next.status_played += temp.percent_play & 127;
      next.learn = 0.0;
      counter++;
      if (counter % 100000 == 0) {
        printf(".");
        if (counter % 6000000 == 0)
          printf(" (%dk)\n", counter / 1000);
        fflush(stdout);
      }
      if (current.position == next.position) {
        current.status_played = current.status_played | next.status_played;
        played++;
        if (temp.status & 128 && temp.percent_play & 128)
          wins++;
        if (temp.status & 128 && !(temp.percent_play & 128))
          losses++;
        if (temp.status & 32 && !(temp.percent_play & 128))
          wins++;
        if (temp.status & 32 && temp.percent_play & 128)
          losses++;
      } else {
        if (played >= min_played && wins >= (losses * wl_percent)) {
          book_positions++;
          cluster++;
          max_cluster = Max(max_cluster, cluster);
          if (!start)
            current.status_played += played;
          current.learn = 0.0;
          memcpy((void *) &book_buffer_char[0].position,
              (void *) BookOut64(current.position), 8);
          memcpy((void *) &book_buffer_char[0].status_played,
              (void *) BookOut32(current.status_played), 4);
          memcpy((void *) &book_buffer_char[0].learn,
              (void *) BookOut32(current.learn), 4);
          stat =
              fwrite(book_buffer_char, sizeof(BOOK_POSITION), 1, book_file);
          if (stat != 1)
            Print(4095, "ERROR!  write failed, disk probably full.\n");
        } else if (played < min_played)
          ignored_mp++;
        else
          ignored_lose++;
        if (last != (int) (next.position >> 49)) {
          next_cluster = ftell(book_file);
          fseek(book_file, cluster_seek, SEEK_SET);
          memcpy((void *) &cluster, BookOut32(cluster), 4);
          stat = fwrite(&cluster, sizeof(int), 1, book_file);
          if (stat != 1)
            Print(4095, "ERROR!  write failed, disk probably full.\n");
          if (next.position == 0)
            break;
          fseek(book_file, next_cluster + sizeof(int), SEEK_SET);
          cluster_seek = next_cluster;
          last = next.position >> 49;
          index[last] = next_cluster;
          cluster = 0;
        }
        wins = 0;
        losses = 0;
        if (temp.status & 128 && temp.percent_play & 128)
          wins++;
        if (temp.status & 128 && !(temp.percent_play & 128))
          losses++;
        if (temp.status & 32 && !(temp.percent_play & 128))
          wins++;
        if (temp.status & 32 && temp.percent_play & 128)
          losses++;
        current = next;
        played = 1;
        if (next.position == 0)
          break;
      }
    }
    fseek(book_file, 0, SEEK_SET);
    for (i = 0; i < 32768; i++) {
      memcpy((void *) &cluster, (void *) BookOut32(index[i]), 4);
      fwrite(&cluster, 4, 1, book_file);
    }
    fseek(book_file, 0, SEEK_END);
    memcpy((void *) &cluster, (void *) BookOut32(major), 4);
    fwrite(&cluster, 4, 1, book_file);
/*
 ************************************************************
 *                                                          *
 *  Now clean up, remove the sort.n files, and print the    *
 *  statistics for building the book.                       *
 *                                                          *
 ************************************************************
 */
    for (i = 1; i <= files; i++) {
      sprintf(fname, "sort.%d", i);
      remove(fname);
    }
    free(index);
    start_elapsed_time = ReadClock() - start_elapsed_time;
    Print(4095, "\n\nparsed %d moves (%d games).\n", total_moves,
        games_parsed);
    Print(4095, "found %d errors during parsing.\n", errors);
    Print(4095, "ignored %d moves (maxply=%d).\n", ignored, max_ply);
    Print(4095, "ignored %d moves (minplayed=%d).\n", ignored_mp,
        min_played);
    Print(4095, "ignored %d moves (win/lose=%.1f%%).\n", ignored_lose,
        wl_percent * 100);
    Print(4095, "book contains %d unique positions.\n", book_positions);
    Print(4095, "deepest book line was %d plies.\n", max_search_depth);
    Print(4095, "longest cluster of moves was %d.\n", max_cluster);
    Print(4095, "time used:  %s elapsed.\n", DisplayTime(start_elapsed_time));
  }
  strcpy(initial_position, "");
  InitializeChessBoard(tree);
}

/* last modified 02/23/14 */
/*
 *******************************************************************************
 *                                                                             *
 *   BookMask() is used to convert the flags for a book move into an 8 bit     *
 *   mask that is either kept in the file, or is set by the operator to select *
 *   which opening(s) the program is allowed to play.                          *
 *                                                                             *
 *******************************************************************************
 */
int BookMask(char *flags) {
  int i, mask;

  mask = 0;
  for (i = 0; i < (int) strlen(flags); i++) {
    if (flags[i] == '?') {
      if (flags[i + 1] == '?') {
        mask = mask | 1;
        i++;
      } else
        mask = mask | 2;
    } else if (flags[i] == '=') {
      mask = mask | 4;
    } else if (flags[i] == '!') {
      if (flags[i + 1] == '!') {
        mask = mask | 16;
        i++;
      } else
        mask = mask | 8;
    }
  }
  return mask;
}

/* last modified 02/23/14 */
/*
 *******************************************************************************
 *                                                                             *
 *   BookSort() is called to sort a block of moves after they have been parsed *
 *   and converted to hash keys.                                               *
 *                                                                             *
 *******************************************************************************
 */
void BookSort(BB_POSITION * buffer, int number, int fileno) {
  char fname[16];
  FILE *output_file;
  int stat;

  qsort((char *) buffer, number, sizeof(BB_POSITION), BookupCompare);
  sprintf(fname, "sort.%d", fileno);
  if (!(output_file = fopen(fname, "wb+")))
    printf("ERROR.  unable to open sort output file\n");
  stat = fwrite(buffer, sizeof(BB_POSITION), number, output_file);
  if (stat != number)
    Print(4095, "ERROR!  write failed, disk probably full.\n");
  fclose(output_file);
}

/* last modified 02/23/14 */
/*
 *******************************************************************************
 *                                                                             *
 *   BookupNextPosition() is the heart of the "merge" operation that is done   *
 *   after the chunks of the parsed/hashed move file are sorted.  This code    *
 *   opens the sort.n files, and returns the least (lexically) position key to *
 *   counted/merged into the main book database.                               *
 *                                                                             *
 *******************************************************************************
 */
BB_POSITION BookupNextPosition(int files, int init) {
  char fname[20];
  static FILE *input_file[100];
  static BB_POSITION *buffer[100];
  static int data_read[100], next[100];
  int i, used;
  BB_POSITION least;

  if (init) {
    for (i = 1; i <= files; i++) {
      sprintf(fname, "sort.%d", i);
      if (!(input_file[i] = fopen(fname, "rb"))) {
        printf("unable to open sort.%d file, may be too many files open.\n",
            i);
        CraftyExit(1);
      }
      buffer[i] = (BB_POSITION *) malloc(sizeof(BB_POSITION) * MERGE_BLOCK);
      if (!buffer[i]) {
        printf("out of memory.  aborting. \n");
        CraftyExit(1);
      }
      fseek(input_file[i], 0, SEEK_SET);
      data_read[i] =
          fread(buffer[i], sizeof(BB_POSITION), MERGE_BLOCK, input_file[i]);
      next[i] = 0;
    }
  }
  for (i = 0; i < 8; i++)
    least.position[i] = 0;
  least.status = 0;
  least.percent_play = 0;
  used = -1;
  for (i = 1; i <= files; i++)
    if (data_read[i]) {
      least = buffer[i][next[i]];
      used = i;
      break;
    }
  if (i > files) {
    for (i = 1; i <= files; i++)
      fclose(input_file[i]);
    return least;
  }
  for (i++; i <= files; i++) {
    if (data_read[i]) {
      uint64_t p1, p2;

      memcpy((char *) &p1, least.position, 8);
      memcpy((char *) &p2, buffer[i][next[i]].position, 8);
      if (p1 > p2) {
        least = buffer[i][next[i]];
        used = i;
      }
    }
  }
  if (--data_read[used] == 0) {
    data_read[used] =
        fread(buffer[used], sizeof(BB_POSITION), MERGE_BLOCK,
        input_file[used]);
    next[used] = 0;
  } else
    next[used]++;
  return least;
}

int BookupCompare(const void *pos1, const void *pos2) {
  static uint64_t p1, p2;

  memcpy((char *) &p1, ((BB_POSITION *) pos1)->position, 8);
  memcpy((char *) &p2, ((BB_POSITION *) pos2)->position, 8);
  if (p1 < p2)
    return -1;
  if (p1 > p2)
    return +1;
  return 0;
}

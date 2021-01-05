#include "chess.h"
#include "data.h"
/* last modified 02/24/14 */
/*
 *******************************************************************************
 *                                                                             *
 *   OutputMove() is responsible for converting the internal move format to a  *
 *   string that can be displayed.  First, it simply converts the from/to      *
 *   squares to fully-qualified algebraic (which includes O-O and O-O-O for    *
 *   castling moves).  Next, we try several "shortcut" forms and call          *
 *   input_move(silent=1) to let it silently check the move for uniqueness.    *
 *   as soon as we get a non-ambiguous move, we return that text string.       *
 *                                                                             *
 *******************************************************************************
 */
char *OutputMove(TREE * RESTRICT tree, int ply, int wtm, int move) {
  static char text_move[10], new_text[10];
  unsigned *mvp;
  char *text = text_move;
  static const char piece_names[7] = { ' ', 'P', 'N', 'B', 'R', 'Q', 'K' };
/*
 ************************************************************
 *                                                          *
 *  Special case for null-move which will only be used in a *
 *  search trace which dumps the entire tree.               *
 *                                                          *
 ************************************************************
 */
  if (move == 0) {
    strcpy(text, "null");
    return text;
  }
  do {
/*
 ************************************************************
 *                                                          *
 *  Check for castling moves first.                         *
 *                                                          *
 ************************************************************
 */
    if ((Piece(move) == king) && (Abs(From(move) - To(move)) == 2)) {
      if (wtm) {
        if (To(move) == 2)
          strcpy(text_move, "O-O-O");
        else
          strcpy(text_move, "O-O");
      } else {
        if (To(move) == 58)
          strcpy(text_move, "O-O-O");
        else
          strcpy(text_move, "O-O");
      }
      break;
    }
/*
 ************************************************************
 *                                                          *
 *  Not a castling move.  Convert the move to a fully-      *
 *  qualified algebraic move as a starting point.           *
 *                                                          *
 ************************************************************
 */
    text = new_text;
    if ((int) Piece(move) > pawn)
      *text++ = piece_names[Piece(move)];
    *text++ = File(From(move)) + 'a';
    *text++ = Rank(From(move)) + '1';
    if (Captured(move))
      *text++ = 'x';
    *text++ = File(To(move)) + 'a';
    *text++ = Rank(To(move)) + '1';
    if (Promote(move)) {
      *text++ = '=';
      *text++ = piece_names[Promote(move)];
    }
    *text = '\0';
    strcpy(text_move, new_text);
    if (output_format > 0)
      break;
/*
 ************************************************************
 *                                                          *
 *  Now we try some short forms.  If this is a pawn move    *
 *  (first character is "P") and the move is not a capture  *
 *  move, we can try just the destination square (Pe2e4     *
 *  becomes e4).                                            *
 *                                                          *
 ************************************************************
 */
    if (Piece(move) == pawn) {
      if (!Captured(move)) {
        strcpy(text_move, new_text + 2);
        if (InputMove(tree, ply, wtm, 1, 0, text_move))
          break;
      }
/*
 ************************************************************
 *                                                          *
 *  If this is a pawn and it is capturing something, try    *
 *  the usual pawn capture format (Pe4xd5 becomes exd5).    *
 *                                                          *
 ************************************************************
 */
      text_move[0] = new_text[0];
      strcpy(text_move + 1, new_text + 2);
      if (InputMove(tree, ply, wtm, 1, 0, text_move))
        break;
/*
 ************************************************************
 *                                                          *
 *  It is a pawn move and we can't find a shorter form, so  *
 *  leave it as a fully-qualified move and go with it as    *
 *  is.  (this will not normally happen).                   *
 *                                                          *
 ************************************************************
 */
      strcpy(text_move, new_text);
      break;
    }
/*
 ************************************************************
 *                                                          *
 *  If the move is a normal piece move, and does not        *
 *  capture anything, we try the piece + destination format *
 *  first (Ng1f3 becomes Nf3).                              *
 *                                                          *
 ************************************************************
 */
    if (!Captured(move)) {
      text_move[0] = new_text[0];
      strcpy(text_move + 1, new_text + 3);
      if (InputMove(tree, ply, wtm, 1, 0, text_move))
        break;
/*
 ************************************************************
 *                                                          *
 *  If that is ambiguous, we will try two alternatives:     *
 *  (1) add in the origin file;  (2) add in the origin rank *
 *  (Ng1f3 becomes Ngf3 or N1f3).                           *
 *                                                          *
 ************************************************************
 */
      text_move[0] = new_text[0];
      text_move[1] = new_text[1];
      strcpy(text_move + 2, new_text + 3);
      if (InputMove(tree, ply, wtm, 1, 0, text_move))
        break;
      text_move[0] = new_text[0];
      strcpy(text_move + 1, new_text + 2);
      if (InputMove(tree, ply, wtm, 1, 0, text_move))
        break;
/*
 ************************************************************
 *                                                          *
 *  Nothing worked, so we go with the fully-qualified move. *
 *                                                          *
 ************************************************************
 */
      strcpy(text_move, new_text);
      break;
    } else {
/*
 ************************************************************
 *                                                          *
 *  If this is a capture, we try the short form of a        *
 *  capture move (Ng1xf3 becomes Nxf3)                      *
 *                                                          *
 ************************************************************
 */
      text_move[0] = new_text[0];
      strcpy(text_move + 1, new_text + 3);
      if (InputMove(tree, ply, wtm, 1, 0, text_move))
        break;
/*
 ************************************************************
 *                                                          *
 *  If that didn't work, we try adding in the origin file   *
 *  or the origin rank (Ng1xf3 becomes Ngxf3 or N1xf3).     *
 *                                                          *
 ************************************************************
 */
      text_move[0] = new_text[0];
      text_move[1] = new_text[1];
      strcpy(text_move + 2, new_text + 3);
      if (InputMove(tree, ply, wtm, 1, 0, text_move))
        break;
      text_move[0] = new_text[0];
      strcpy(text_move + 1, new_text + 2);
      if (InputMove(tree, ply, wtm, 1, 0, text_move))
        break;
/*
 ************************************************************
 *                                                          *
 *  Nothing worked, return the fully-qualified move.        *
 *                                                          *
 ************************************************************
 */
      strcpy(text_move, new_text);
      break;
    }
  } while (0);
/*
 ************************************************************
 *                                                          *
 *  If the move is a check, or mate, append either "+" or   *
 *  "#" as appropriate.                                     *
 *                                                          *
 ************************************************************
 */
  if (output_format == 0) {
    text = text_move + strlen(text_move);
    tree->status[MAXPLY] = tree->status[ply];
    MakeMove(tree, MAXPLY, wtm, move);
    if (Check(Flip(wtm))) {
      mvp =
          GenerateCheckEvasions(tree, MAXPLY + 1, Flip(wtm),
          tree->move_list + 4800);
      if (mvp == (tree->move_list + 4800))
        *text++ = '#';
      else
        *text++ = '+';
    }
    UnmakeMove(tree, MAXPLY, wtm, move);
    *text = 0;
  }
  return text_move;
}

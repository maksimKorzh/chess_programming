#include "chess.h"
#include "data.h"
/* last modified 02/24/14 */
/*
 *******************************************************************************
 *                                                                             *
 *   InputMove() is responsible for converting a move from a text string to    *
 *   the internal move format.  It allows the so-called "reduced algebraic     *
 *   move format" which makes the origin square optional unless required for   *
 *   clarity.  It also accepts as little as required to remove ambiguity from  *
 *   the move, by using GenerateMoves() to produce a set of legal moves that   *
 *   the text can be applied against to eliminate those moves not intended.    *
 *   Hopefully, only one move will remain after the elimination and legality   *
 *   checks.                                                                   *
 *                                                                             *
 *******************************************************************************
 */
int InputMove(TREE * RESTRICT tree, int ply, int wtm, int silent,
    int ponder_list, char *text) {
  unsigned moves[220], *mv, *mvp, *goodmove = 0;
  int piece = -1, capture, promote, give_check;
  int ffile, frank, tfile, trank;
  int current, i, nleft;
  char *goodchar, *tc;
  char movetext[128];
  static const char pieces[15] =
      { ' ', ' ', 'P', 'p', 'N', 'n', 'B', 'b', 'R', 'r',
    'Q', 'q', 'K', 'k', '\0'
  };
  static const char pro_pieces[15] =
      { ' ', ' ', 'P', 'p', 'N', 'n', 'B', 'b', 'R', 'r', 'Q', 'q',
    'K', 'k', '\0'
  };
/*
 ************************************************************
 *                                                          *
 *  First, we need to strip off the special characters for  *
 *  check, mate, bad move, good move, and such that might   *
 *  come from a PGN input file.                             *
 *                                                          *
 ************************************************************
 */
  if ((tc = strchr(text, '!')))
    *tc = 0;
  if ((tc = strchr(text, '?')))
    *tc = 0;
/*
 ************************************************************
 *                                                          *
 *  Check for full coordinate input (f1e1) and handle that  *
 *  if needed.                                              *
 *                                                          *
 ************************************************************
 */
  if (strlen(text) == 0)
    return 0;
  if ((text[0] >= 'a') && (text[0] <= 'h') && (text[1] >= '1')
      && (text[1] <= '8') && (text[2] >= 'a') && (text[2] <= 'h')
      && (text[3] >= '1') && (text[3] <= '8'))
    return InputMoveICS(tree, ply, wtm, silent, ponder_list, text);
/*
 ************************************************************
 *                                                          *
 *  Initialize move structure.  If we discover a parsing    *
 *  error, this will cause us to return a move of "0" to    *
 *  indicate some sort of error was detected.               *
 *                                                          *
 ************************************************************
 */
  tree->status[MAXPLY] = tree->status[ply];
  strcpy(movetext, text);
  moves[0] = 0;
  piece = 0;
  capture = 0;
  promote = 0;
  give_check = 0;
  frank = -1;
  ffile = -1;
  trank = -1;
  tfile = -1;
  goodchar = strchr(movetext, '#');
  if (goodchar)
    *goodchar = 0;
/*
 ************************************************************
 *                                                          *
 *  First we look for castling moves which are a special    *
 *  case with an unusual syntax compared to normal moves.   *
 *                                                          *
 ************************************************************
 */
  if (!strcmp(movetext, "o-o") || !strcmp(movetext, "o-o+")
      || !strcmp(movetext, "O-O") || !strcmp(movetext, "O-O+")
      || !strcmp(movetext, "0-0") || !strcmp(movetext, "0-0+")) {
    piece = king;
    if (wtm) {
      ffile = 4;
      frank = 0;
      tfile = 6;
      trank = 0;
    } else {
      ffile = 4;
      frank = 7;
      tfile = 6;
      trank = 7;
    }
  } else if (!strcmp(movetext, "o-o-o") || !strcmp(movetext, "o-o-o+")
      || !strcmp(movetext, "O-O-O") || !strcmp(movetext, "O-O-O+")
      || !strcmp(movetext, "0-0-0") || !strcmp(movetext, "0-0-0+")) {
    piece = king;
    if (wtm) {
      ffile = 4;
      frank = 0;
      tfile = 2;
      trank = 0;
    } else {
      ffile = 4;
      frank = 7;
      tfile = 2;
      trank = 7;
    }
  } else {
/*
 ************************************************************
 *                                                          *
 *  OK, it is not a castling move.  Check for two "b"       *
 *  characters which might be a piece (bishop) and a file   *
 *  (b-file).  The first "b" should be "B" but we allow     *
 *  this to make typing input simpler.                      *
 *                                                          *
 ************************************************************
 */
    if ((movetext[0] == 'b') && (movetext[1] == 'b'))
      movetext[0] = 'B';
/*
 ************************************************************
 *                                                          *
 *  Check to see if there is a "+" character which means    *
 *  that this move is a check.  We can use this to later    *
 *  eliminate all non-checking moves as possibilities.      *
 *                                                          *
 ************************************************************
 */
    if (strchr(movetext, '+')) {
      *strchr(movetext, '+') = 0;
      give_check = 1;
    }
/*
 ************************************************************
 *                                                          *
 *  If this is a promotion, indicated by an "=" in the      *
 *  text, we can pick up the promote-to piece and save it   *
 *  to use later when eliminating moves.                    *
 *                                                          *
 ************************************************************
 */
    if (strchr(movetext, '=')) {
      goodchar = strchr(movetext, '=');
      goodchar++;
      promote = (strchr(pro_pieces, *goodchar) - pro_pieces) >> 1;
      *strchr(movetext, '=') = 0;
    }
/*
 ************************************************************
 *                                                          *
 *  Now for a kludge.  ChessBase (and others) can't follow  *
 *  the PGN standard of bxc8=Q for promotion, and instead   *
 *  will produce "bxc8Q" omitting the PGN-standard "="      *
 *  character.  We handle that here so that we can read     *
 *  their non-standard moves.                               *
 *                                                          *
 ************************************************************
 */
    else {
      char *prom = strchr(pro_pieces, movetext[strlen(movetext) - 1]);

      if (prom) {
        promote = (prom - pro_pieces) >> 1;
        movetext[strlen(movetext) - 1] = 0;
      }
    }
/*
 ************************************************************
 *                                                          *
 *  Next we extract the last rank/file designators from the *
 *  text, since the destination is required for all valid   *
 *  non-castling moves.  Note that we might not have both a *
 *  rank and file but we must have at least one.            *
 *                                                          *
 ************************************************************
 */
    current = strlen(movetext) - 1;
    trank = movetext[current] - '1';
    if ((trank >= 0) && (trank <= 7))
      movetext[current] = 0;
    else
      trank = -1;
    current = strlen(movetext) - 1;
    tfile = movetext[current] - 'a';
    if ((tfile >= 0) && (tfile <= 7))
      movetext[current] = 0;
    else
      tfile = -1;
    if (strlen(movetext)) {
/*
 ************************************************************
 *                                                          *
 *  The first character is the moving piece, unless it is a *
 *  pawn.  In this case, the moving piece is omitted and we *
 *  know what it has to be.                                 *
 *                                                          *
 ************************************************************
 */
      if (strchr("  PpNnBBRrQqKk", *movetext)) {
        piece = (strchr(pieces, movetext[0]) - pieces) >> 1;
        for (i = 0; i < (int) strlen(movetext); i++)
          movetext[i] = movetext[i + 1];
      }
/*
 ************************************************************
 *                                                          *
 *  It is also possible that this move is a capture, which  *
 *  is indicated by a "x" between either the source and     *
 *  destination squares, or between the moving piece and    *
 *  the destination.                                        *
 *                                                          *
 ************************************************************
 */
      if ((strlen(movetext)) && (movetext[strlen(movetext) - 1] == 'x')) {
        capture = 1;
        movetext[strlen(movetext) - 1] = 0;
      } else
        capture = 0;
/*
 ************************************************************
 *                                                          *
 *  It is possible to have no source square, but we could   *
 *  have a complete algebraic square designation, or just   *
 *  rank or file, needed to disambiguate the move.          *
 *                                                          *
 ************************************************************
 */
      if (strlen(movetext)) {
        ffile = movetext[0] - 'a';
        if ((ffile < 0) || (ffile > 7)) {
          ffile = -1;
          frank = movetext[0] - '1';
          if ((frank < 0) || (frank > 7))
            piece = -1;
        } else {
          if (strlen(movetext) == 2) {
            frank = movetext[1] - '1';
            if ((frank < 0) || (frank > 7))
              piece = -1;
          }
        }
      }
    }
  }
/*
 ************************************************************
 *                                                          *
 *  Now for the easy part.  We first generate all moves if  *
 *  not pondering, or else use a pre-computed list of moves *
 *  (if pondering) since the board position is not correct  *
 *  for move input analysis.  We then loop through the list *
 *  of moves, using the information we extracted previously *
 *  , and eliminate all moves that are (a) the wrong piece  *
 *  type;  (b) wrong source or destination square;          *
 *  (c) wrong promotion type;  (d) should be a capture,     *
 *  check or promotion but is not, or vice-versa.           *
 *                                                          *
 ************************************************************
 */
  if (!piece)
    piece = 1;
  if (!ponder_list) {
    mvp = GenerateCaptures(tree, MAXPLY, wtm, moves);
    mvp = GenerateNoncaptures(tree, MAXPLY, wtm, mvp);
  } else {
    for (i = 0; i < num_ponder_moves; i++)
      moves[i] = ponder_moves[i];
    mvp = moves + num_ponder_moves;
  }
  for (mv = &moves[0]; mv < mvp; mv++) {
    if (piece && (Piece(*mv) != piece))
      *mv = 0;
    if ((ffile >= 0) && (File(From(*mv)) != ffile))
      *mv = 0;
    if (capture && (!Captured(*mv)))
      *mv = 0;
    if (promote && (Promote(*mv) != promote))
      *mv = 0;
    if ((frank >= 0) && (Rank(From(*mv)) != frank))
      *mv = 0;
    if ((tfile >= 0) && (File(To(*mv)) != tfile))
      *mv = 0;
    if ((trank >= 0) && (Rank(To(*mv)) != trank))
      *mv = 0;
    if (!ponder_list && *mv) {
      MakeMove(tree, MAXPLY, wtm, *mv);
      if (Check(wtm) || (give_check && !Check(Flip(wtm)))) {
        UnmakeMove(tree, MAXPLY, wtm, *mv);
        *mv = 0;
      } else
        UnmakeMove(tree, MAXPLY, wtm, *mv);
    }
  }
/*
 ************************************************************
 *                                                          *
 *  Once we have completed eliminating incorrect moves, we  *
 *  hope to have exactly one move left.  If none or left,   *
 *  the entered move is illegal.  If more than one is left, *
 *  the move entered is ambiguous.  If appropriate, we      *
 *  output some sort of diagnostic message and then return. *
 *                                                          *
 ************************************************************
 */
  nleft = 0;
  for (mv = &moves[0]; mv < mvp; mv++) {
    if (*mv) {
      nleft++;
      goodmove = mv;
    }
  }
  if (nleft == 1)
    return *goodmove;
  if (!silent) {
    if (nleft == 0)
      Print(4095, "Illegal move: %s\n", text);
    else if (piece < 0)
      Print(4095, "Illegal move (unrecognizable): %s\n", text);
    else
      Print(4095, "Illegal move (ambiguous): %s\n", text);
  }
  return 0;
}

/* last modified 02/24/14 */
/*
 *******************************************************************************
 *                                                                             *
 *   InputMoveICS() is responsible for converting a move from the ics format   *
 *   [from][to][promote] to the program's internal format.                     *
 *                                                                             *
 *******************************************************************************
 */
int InputMoveICS(TREE * RESTRICT tree, int ply, int wtm, int silent,
    int ponder_list, char *text) {
  unsigned moves[220], *mv, *mvp, *goodmove = 0;
  int piece = -1, promote;
  int ffile, frank, tfile, trank;
  int i, nleft;
  char movetext[128];
  static const char pieces[15] =
      { ' ', ' ', 'P', 'p', 'N', 'n', 'B', 'b', 'R', 'r',
    'Q', 'q', 'K', 'k', '\0'
  };
/*
 ************************************************************
 *                                                          *
 *  Initialize move structure.  If we discover a parsing    *
 *  error, this will cause us to return a move of "0" to    *
 *  indicate some sort of error was detected.               *
 *                                                          *
 ************************************************************
 */
  if (strlen(text) == 0)
    return 0;
  tree->status[MAXPLY] = tree->status[ply];
  strcpy(movetext, text);
  moves[0] = 0;
  promote = 0;
/*
 ************************************************************
 *                                                          *
 *  First we look for castling moves which are a special    *
 *  case with an unusual syntax compared to normal moves.   *
 *                                                          *
 ************************************************************
 */
  if (!strcmp(movetext, "o-o") || !strcmp(movetext, "O-O")
      || !strcmp(movetext, "0-0")) {
    piece = king;
    if (wtm) {
      ffile = 4;
      frank = 0;
      tfile = 6;
      trank = 0;
    } else {
      ffile = 4;
      frank = 7;
      tfile = 6;
      trank = 7;
    }
  } else if (!strcmp(movetext, "o-o-o") || !strcmp(movetext, "O-O-O")
      || !strcmp(movetext, "0-0-0")) {
    piece = king;
    if (wtm) {
      ffile = 4;
      frank = 0;
      tfile = 2;
      trank = 0;
    } else {
      ffile = 4;
      frank = 7;
      tfile = 2;
      trank = 7;
    }
  } else {
/*
 ************************************************************
 *                                                          *
 *  Next we extract both rank/file designators from the     *
 *  text, since the destination is required for all valid   *
 *  non-castling moves.                                     *
 *                                                          *
 ************************************************************
 */
    ffile = movetext[0] - 'a';
    frank = movetext[1] - '1';
    tfile = movetext[2] - 'a';
    trank = movetext[3] - '1';
/*
 ************************************************************
 *                                                          *
 *  If this is a promotion, indicated by an "=" in the      *
 *  text, we can pick up the promote-to piece and save it   *
 *  to use later when eliminating moves.                    *
 *                                                          *
 ************************************************************
 */
    if (movetext[4] == '=')
      promote = (strchr(pieces, movetext[5]) - pieces) >> 1;
    else if ((movetext[4] != 0) && (movetext[4] != ' '))
      promote = (strchr(pieces, movetext[4]) - pieces) >> 1;
  }
/*
 ************************************************************
 *                                                          *
 *  Now for the easy part.  We first generate all moves if  *
 *  not pondering, or else use a pre-computed list of moves *
 *  (if pondering) since the board position is not correct  *
 *  for move input analysis.  We then loop through the list *
 *  of moves, using the information we extracted previously *
 *  and eliminate all moves that are (a) the wrong piece    *
 *  type;  (b) wrong source or destination square;          *
 *  (c) wrong promotion type;  (d) should be a capture,     *
 *  check or promotion but is not or vice-versa.            *
 *                                                          *
 ************************************************************
 */
  if (!ponder_list) {
    mvp = GenerateCaptures(tree, MAXPLY, wtm, moves);
    mvp = GenerateNoncaptures(tree, MAXPLY, wtm, mvp);
  } else {
    for (i = 0; i < num_ponder_moves; i++)
      moves[i] = ponder_moves[i];
    mvp = moves + num_ponder_moves;
  }
  for (mv = &moves[0]; mv < mvp; mv++) {
    if (Promote(*mv) != promote)
      *mv = 0;
    if (Rank(From(*mv)) != frank)
      *mv = 0;
    if (File(From(*mv)) != ffile)
      *mv = 0;
    if (Rank(To(*mv)) != trank)
      *mv = 0;
    if (File(To(*mv)) != tfile)
      *mv = 0;
    if (!ponder_list && *mv) {
      MakeMove(tree, MAXPLY, wtm, *mv);
      if (Check(wtm)) {
        UnmakeMove(tree, MAXPLY, wtm, *mv);
        *mv = 0;
      } else
        UnmakeMove(tree, MAXPLY, wtm, *mv);
    }
  }
/*
 ************************************************************
 *                                                          *
 *  Once we have completed eliminating incorrect moves, we  *
 *  hope to have exactly one move left.  If none or left,   *
 *  the entered move is illegal.  If more than one is left, *
 *  the move entered is ambiguous.  If appropriate, we      *
 *  output some sort of diagnostic message and then return. *
 *                                                          *
 ************************************************************
 */
  nleft = 0;
  for (mv = &moves[0]; mv < mvp; mv++) {
    if (*mv) {
      nleft++;
      goodmove = mv;
    }
  }
  if (nleft == 1)
    return *goodmove;
  if (!silent) {
    if (nleft == 0)
      Print(4095, "Illegal move: %s\n", text);
    else if (piece < 0)
      Print(4095, "Illegal move (unrecognizable): %s\n", text);
    else
      Print(4095, "Illegal move (ambiguous): %s\n", text);
  }
  return 0;
}

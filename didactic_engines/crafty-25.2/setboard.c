#include "chess.h"
#include "data.h"
/* last modified 05/08/14 */
/*
 *******************************************************************************
 *                                                                             *
 *   SetBoard() is used to set up the board in any position desired.  It uses  *
 *   a forsythe-like string of characters to describe the board position.      *
 *                                                                             *
 *   The standard piece codes p,n,b,r,q,k are used to denote the type of piece *
 *   on a square, upper/lower case are used to indicate the side (program/     *
 *   opponent) of the piece.                                                   *
 *                                                                             *
 *   The pieces are entered with square a8 first, then b8, ... until the full  *
 *   8th rank is completed.  A "/" terminates that rank.  This is repeated for *
 *   each of the 8 ranks, with the last (1st) rank not needing a terminating   *
 *   "/".  For empty squares, a number between 1 and 8 can be used to indicate *
 *   the number of adjacent empty squares.                                     *
 *                                                                             *
 *   That board description must be followed by a "b" or "w" to indicate which *
 *   side is on move.                                                          *
 *                                                                             *
 *   Next, up to 4 characters are used to indicate which side can castle and   *
 *   to which side.  An uppercase K means white can castle kingside, while a   *
 *   lowercase q means black can castle queenside.                             *
 *                                                                             *
 *   Finally, if there is an enpassant capture possible (the last move was a   *
 *   double pawn move and there was an enemy pawn that could capture it.  The  *
 *   square is the square the capturing pawn ends up on.                       *
 *                                                                             *
 *           K2R/PPP////q/5ppp/7k/ b - -                                       *
 *                                                                             *
 *   this assumes that k represents a white king and -q represents a black     *
 *   queen.                                                                    *
 *                                                                             *
 *                            k  *  *  r  *  *  *  *                           *
 *                            p  p  p  *  *  *  *  *                           *
 *                            *  *  *  *  *  *  *  *                           *
 *                            *  *  *  *  *  *  *  *                           *
 *                            *  *  *  *  *  *  *  *                           *
 *                           -q  *  *  *  *  *  *  *                           *
 *                            *  *  *  *  * -p -p -p                           *
 *                            *  *  *  *  *  *  * -k                           *
 *                                                                             *
 *******************************************************************************
 */
void SetBoard(TREE * tree, int nargs, char *args[], int special) {
  int twtm, i, match, num, pos, square, tboard[64];
  int bcastle, ep, wcastle, error = 0, whichsq;
  char input[80];
  static const char bdinfo[] =
      { 'k', 'q', 'r', 'b', 'n', 'p', '*', 'P', 'N', 'B',
    'R', 'Q', 'K', '*', '1', '2', '3', '4',
    '5', '6', '7', '8', '/'
  };
  static const char status[13] =
      { 'K', 'Q', 'k', 'q', 'a', 'b', 'c', 'd', 'e', 'f', 'g',
    'h', ' '
  };
  static const int firstsq[8] = { 56, 48, 40, 32, 24, 16, 8, 0 };

  if (special)
    strcpy(input, initial_position);
  else
    strcpy(input, args[0]);
  for (i = 0; i < 64; i++)
    tboard[i] = 0;
/*
 ************************************************************
 *                                                          *
 *  Scan the input string searching for pieces, numbers     *
 *  [empty squares], slashes [end-of-rank] and a blank      *
 *  [end of board, start of castle status].                 *
 *                                                          *
 ************************************************************
 */
  whichsq = 0;
  square = firstsq[whichsq];
  num = 0;
  for (pos = 0; pos < (int) strlen(args[0]); pos++) {
    for (match = 0; match < 23 && args[0][pos] != bdinfo[match]; match++);
    if (match > 22)
      break;
/*
 "/" -> end of this rank.
 */
    else if (match == 22) {
      num = 0;
      if (whichsq > 6)
        break;
      square = firstsq[++whichsq];
    }
/*
 "1-8" -> empty squares.
 */
    else if (match >= 14) {
      num += match - 13;
      square += match - 13;
      if (num > 8) {
        printf("more than 8 squares on one rank\n");
        error = 1;
        break;
      }
      continue;
    }
/*
 piece codes.
 */
    else {
      if (++num > 8) {
        printf("more than 8 squares on one rank\n");
        error = 1;
        break;
      }
      tboard[square++] = match - 6;
    }
  }
/*
 ************************************************************
 *                                                          *
 *  Now extract (a) side to move [w/b], (b) castle status   *
 *  [KkQq for white/black king-side ok, white/black queen-  *
 *  side ok], (c) enpassant target square.                  *
 *                                                          *
 ************************************************************
 */
  twtm = 0;
  ep = 0;
  wcastle = 0;
  bcastle = 0;
/*
 ************************************************************
 *                                                          *
 *  Side to move.                                           *
 *                                                          *
 ************************************************************
 */
  if (args[1][0] == 'w')
    twtm = 1;
  else if (args[1][0] == 'b')
    twtm = 0;
  else {
    printf("side to move is bad\n");
    error = 1;
  }
/*
 ************************************************************
 *                                                          *
 *  Castling/enpassant status.                              *
 *                                                          *
 ************************************************************
 */
  if (nargs > 2 && strlen(args[2])) {
    if (strcmp(args[2], "-")) {
      for (pos = 0; pos < (int) strlen(args[2]); pos++) {
        for (match = 0; (match < 13) && (args[2][pos] != status[match]);
            match++);
        if (match == 0)
          wcastle += 1;
        else if (match == 1)
          wcastle += 2;
        else if (match == 2)
          bcastle += 1;
        else if (match == 3)
          bcastle += 2;
        else if (args[2][0] != '-') {
          printf("castling status is bad.\n");
          error = 1;
        }
      }
    }
  }
  if (nargs > 3 && strlen(args[3])) {
    if (strcmp(args[3], "-")) {
      if (args[3][0] >= 'a' && args[3][0] <= 'h' && args[3][1] > '0' &&
          args[3][1] < '9') {
        ep = (args[3][1] - '1') * 8 + args[3][0] - 'a';
      } else if (args[3][0] != '-') {
        printf("enpassant status is bad.\n");
        error = 1;
      }
    }
  }
  for (i = 0; i < 64; i++)
    PcOnSq(i) = tboard[i];
  Castle(0, white) = wcastle;
  Castle(0, black) = bcastle;
  EnPassant(0) = 0;
  if (ep) {
    do {
      if (twtm && Rank(ep) == RANK6 && PcOnSq(ep - 8) == -pawn) {
        if (File(ep) != 7 && PcOnSq(ep - 7) == pawn)
          break;
        if (File(ep) != 0 && PcOnSq(ep - 9) == pawn)
          break;
      } else if (!twtm && Rank(ep) == RANK3 && PcOnSq(ep + 8) == pawn) {
        if (File(ep) != 0 && PcOnSq(ep + 7) == -pawn)
          break;
        if (File(ep) != 7 && PcOnSq(ep + 9) == -pawn)
          break;
      } else {
        ep = 0;
      }
      if (!ep) {
        printf("enpassant status is bad.\n");
        ep = 0;
        error = 1;
      }
    } while (0);
    EnPassant(0) = ep;
  }
/*
 ************************************************************
 *                                                          *
 *  Now check the castling status status to make sure that  *
 *  the board is in a state that matches.                   *
 *                                                          *
 ************************************************************
 */
  if (((Castle(0, white) & 2) && (PcOnSq(A1) != rook))
      || ((Castle(0, white) & 1) && (PcOnSq(H1) != rook))
      || ((Castle(0, black) & 2) && (PcOnSq(A8) != -rook))
      || ((Castle(0, black) & 1) && (PcOnSq(H8) != -rook))) {
    printf("ERROR-- castling status does not match board position\n");
    error = 1;
  }
/*
 ************************************************************
 *                                                          *
 *  Now set the bitboards so that error tests can be done.  *
 *                                                          *
 ************************************************************
 */
  SetChessBitBoards(tree);
/*
 ************************************************************
 *                                                          *
 *  Now check the position for a sane position, which means *
 *  no more than 8 pawns, no more than 10 knights, bishops  *
 *  or rooks, no more than 9 queens, no pawns on 1st or 8th *
 *  rank, etc.                                              *
 *                                                          *
 ************************************************************
 */
  game_wtm = twtm;
  error += InvalidPosition(tree);
  if (!error) {
    if (log_file)
      DisplayChessBoard(log_file, tree->position);
    rep_index = 0;
    tree->rep_list[0] = HashKey;
    Reversible(0) = 0;
    if (!special) {
      last_mate_score = 0;
      InitializeKillers();
      last_pv.pathd = 0;
      last_pv.pathl = 0;
      tree->pv[0].pathd = 0;
      tree->pv[0].pathl = 0;
      moves_out_of_book = 0;
    }
  } else {
    if (special)
      Print(4095, "bad string = \"%s\"\n", initial_position);
    else
      Print(4095, "bad string = \"%s\"\n", args[0]);
    InitializeChessBoard(tree);
    Print(4095, "Illegal position, using normal initial chess position\n");
  }
}

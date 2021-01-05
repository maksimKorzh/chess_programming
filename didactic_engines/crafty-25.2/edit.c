#include "chess.h"
#include "data.h"
/* last modified 05/08/14 */
/*
 *******************************************************************************
 *                                                                             *
 *   Edit() is used to edit (alter) the current board position.  It clears the *
 *   board and then allows the operator to enter a position using the syntax   *
 *   defined by the xboard/winboard "edit" command.                            *
 *                                                                             *
 *   White sets the color of pieces added to white.  This color will stay in   *
 *   effect until specifically changed.                                        *
 *                                                                             *
 *   Black sets the color of pieces added to black.  This color will stay in   *
 *   effect until specifically changed.                                        *
 *                                                                             *
 *   # clears the chessboard completely.                                       *
 *                                                                             *
 *   C changes (toggles) the color of pieces being placed on the board.        *
 *                                                                             *
 *   End (or . for ICS/Xboard) terminates Edit().                              *
 *                                                                             *
 *   Pieces are placed on the board by three character "commands" of the form  *
 *   [piece][square] where piece is a member of the normal set of pieces       *
 *   {P,N,B,R,Q,K} and [square] is algebraic square notation (a1-h8).  Ex: Ke8 *
 *   puts a king (of the current "color") on square e8.                        *
 *                                                                             *
 *******************************************************************************
 */
void Edit(void) {
  int athome = 1, i, piece, readstat, square, tfile, trank, wtm = 1, error =
      0;
  static const char pieces[] =
      { 'x', 'X', 'P', 'p', 'N', 'n', 'B', 'b', 'R', 'r',
    'Q', 'q', 'K', 'k', '\0'
  };
  TREE *const tree = block[0];

/*
 ************************************************************
 *                                                          *
 *  Process the commands to set the board[n] form of the    *
 *  chess position.                                         *
 *                                                          *
 ************************************************************
 */
  while (FOREVER) {
    if ((input_stream == stdin) && !xboard) {
      if (wtm)
        printf("edit(white): ");
      else
        printf("edit(black): ");
    }
    fflush(stdout);
    readstat = Read(1, buffer);
    if (readstat < 0)
      return;
    nargs = ReadParse(buffer, args, " \t;");
    if (xboard)
      Print(32, "edit.command:%s\n", args[0]);
    if (!strcmp(args[0], "white"))
      wtm = 1;
    else if (!strcmp(args[0], "black"))
      wtm = 0;
    if (!strcmp(args[0], "#"))
      for (i = 0; i < 64; i++)
        PcOnSq(i) = 0;
    else if (!strcmp(args[0], "c"))
      wtm = Flip(wtm);
    else if (!strcmp(args[0], "end") || (!strcmp(args[0], ".")))
      break;
    else if (!strcmp(args[0], "d"))
      DisplayChessBoard(stdout, tree->position);
    else if (strlen(args[0]) == 3) {
      if (strchr(pieces, args[0][0])) {
        piece = (strchr(pieces, args[0][0]) - pieces) >> 1;
        tfile = args[0][1] - 'a';
        trank = args[0][2] - '1';
        square = (trank << 3) + tfile;
        if ((square < 0) || (square > 63))
          printf("unrecognized square %s\n", args[0]);
        if (wtm)
          PcOnSq(square) = piece;
        else
          PcOnSq(square) = -piece;
      }
    } else if (strlen(args[0]) == 2) {
      piece = pawn;
      tfile = args[0][0] - 'a';
      trank = args[0][1] - '1';
      square = (trank << 3) + tfile;
      if ((square < 0) || (square > 63))
        printf("unrecognized square %s\n", args[0]);
      if (wtm)
        PcOnSq(square) = piece;
      else
        PcOnSq(square) = -piece;
    } else
      printf("unrecognized piece %s\n", args[0]);
  }
/*
 ************************************************************
 *                                                          *
 *  Now, if a king is on its original square, check the     *
 *  rooks to see if they are and set the castle status      *
 *  accordingly.  Note that this checks for pieces on the   *
 *  original rank, but not their original squares (ICS      *
 *  "wild" games) and doesn't set castling if true.         *
 *                                                          *
 *  The winboard/xboard "edit" command does not give us a   *
 *  way of setting castling status, so we have to guess.    *
 *                                                          *
 ************************************************************
 */
  Castle(0, white) = 0;
  Castle(0, black) = 0;
  EnPassant(0) = 0;
  for (i = 0; i < 16; i++)
    if (PcOnSq(i) == 0 || PcOnSq(i + 48) == 0)
      athome = 0;
  if (!athome || (PcOnSq(A1) == rook && PcOnSq(B1) == knight &&
          PcOnSq(C1) == bishop && PcOnSq(D1) == queen && PcOnSq(E1) == king &&
          PcOnSq(F1) == bishop && PcOnSq(G1) == knight && PcOnSq(H1) == rook
          && PcOnSq(A8) == -rook && PcOnSq(B8) == -knight &&
          PcOnSq(C8) == -bishop && PcOnSq(D8) == -queen && PcOnSq(E8) == -king
          && PcOnSq(F8) == -bishop && PcOnSq(G8) == -knight &&
          PcOnSq(H8) == -rook)) {
    if (PcOnSq(E1) == king) {
      if (PcOnSq(A1) == rook)
        Castle(0, white) = Castle(0, white) | 2;
      if (PcOnSq(H1) == rook)
        Castle(0, white) = Castle(0, white) | 1;
    }
    if (PcOnSq(E8) == -king) {
      if (PcOnSq(A8) == -rook)
        Castle(0, black) = Castle(0, black) | 2;
      if (PcOnSq(H8) == -rook)
        Castle(0, black) = Castle(0, black) | 1;
    }
  }
/*
 ************************************************************
 *                                                          *
 *  Basic board is now set.  Now it's time to set the bit   *
 *  board representation correctly.                         *
 *                                                          *
 ************************************************************
 */
  SetChessBitBoards(tree);
  error += InvalidPosition(tree);
  if (!error) {
    if (log_file)
      DisplayChessBoard(log_file, tree->position);
    wtm = 1;
    move_number = 1;
    rep_index = 0;
    tree->rep_list[0] = HashKey;
    Reversible(0) = 0;
    moves_out_of_book = 0;
  } else {
    InitializeChessBoard(tree);
    Print(4095, "Illegal position, using normal initial chess position\n");
  }
}

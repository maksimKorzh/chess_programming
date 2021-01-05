#include "chess.h"
#include "data.h"
/* last modified 02/26/14 */
/*
 *******************************************************************************
 *                                                                             *
 *   EVTest() is used to test the program against a suite of test positions to *
 *   measure its performance on a particular machine, or to evaluate its skill *
 *   after modifying it in some way.                                           *
 *                                                                             *
 *   The test is initiated by using the "evtest <filename>" command to read in *
 *   the suite of problems from file <filename>.  The format of this file is   *
 *   as follows:                                                               *
 *                                                                             *
 *   <forsythe-string>  This sets the board position using the usual Forsythe  *
 *   notation (see module SetBoard() for a full explanation explanation of the *
 *   syntax).                                                                  *
 *                                                                             *
 *   After reading this position, the program then calls Evaluate() to produce *
 *   a positional evaluation, along with any debug output from Evaluate(), and *
 *   then goes on to the next position.                                        *
 *                                                                             *
 *   A common use (with code included below) is to take a position, and then   *
 *   "flip" it (reverse the board rank by rank changing the color of each      *
 *   piece as this is done, giving us two positions.  We then take each of     *
 *   these and mirror them, which reverses them file by file, which now gives  *
 *   four positions in total.  We run these thru Evaluate() to make sure that  *
 *   all produce exactly the same score, except for the color change for the   *
 *   two that changed the piece colors.  This is used to make certain that the *
 *   evaluation is not asymmetric with respect to the colors, or to the side   *
 *   of the board, which catches errors that are difficult to spot otherwise.  *
 *                                                                             *
 *******************************************************************************
 */
void EVTest(char *filename) {
  FILE *test_input;
  char *eof;
  char buff[4096];
  TREE *const tree = block[0];

/*
 ************************************************************
 *                                                          *
 *  Read in the position                                    *
 *                                                          *
 ************************************************************
 */
  if (!(test_input = fopen(filename, "r"))) {
    printf("file %s does not exist.\n", filename);
    return;
  }
  while (FOREVER) {
    eof = fgets(buffer, 4096, test_input);
    if (eof) {
      char *delim;

      delim = strchr(buffer, '\n');
      if (delim)
        *delim = 0;
      delim = strchr(buffer, '\r');
      if (delim)
        *delim = ' ';
    } else
      break;
    strcpy(buff, buffer);
    nargs = ReadParse(buffer, args, " \t;");
    if (!strcmp(args[0], "end"))
      break;
/*
 ************************************************************
 *                                                          *
 *  Now for the asymmetry tests.  We use the built-in       *
 *  commands "flip" and "flop" to create the four           *
 *  positions, and call Evaluate() for each.                *
 *                                                          *
 ************************************************************
 */
    else {
      int s1, s2, s3, s4, id;

      for (id = 2; id < nargs; id++)
        if (!strcmp(args[id], "id"))
          break;
      if (id >= nargs)
        id = 0;
      SetBoard(tree, nargs, args, 0);
      Castle(0, white) = 0;
      Castle(1, white) = 0;
      Castle(0, black) = 0;
      Castle(1, black) = 0;
      tree->pawn_score.key = 0;
      root_wtm = game_wtm;
      s1 = Evaluate(tree, 0, game_wtm, -99999, 99999);
      strcpy(buffer, "flop");
      Option(tree);
      tree->pawn_score.key = 0;
      root_wtm = game_wtm;
      s2 = Evaluate(tree, 0, game_wtm, -99999, 99999);
      strcpy(buffer, "flip");
      Option(tree);
      tree->pawn_score.key = 0;
      root_wtm = game_wtm;
      s3 = Evaluate(tree, 0, game_wtm, -99999, 99999);
      strcpy(buffer, "flop");
      Option(tree);
      tree->pawn_score.key = 0;
      root_wtm = game_wtm;
      s4 = Evaluate(tree, 0, game_wtm, -99999, 99999);
/*
 ************************************************************
 *                                                          *
 *  If the evaluations (with sign corrected if piece color  *
 *  was changed) are different, we display the four values  *
 *  and then display the original board position so that we *
 *  can debug the cause of the asymmetry.                   *
 *                                                          *
 ************************************************************
 */
      if (s1 != s2 || s1 != s3 || s1 != s4 || s2 != s3 || s2 != s4 ||
          s3 != s4) {
        strcpy(buffer, "flip");
        Option(tree);
        printf("FEN = %s\n", buff);
        DisplayChessBoard(stdout, tree->position);
        if (id)
          Print(4095, "id=%s  ", args[id + 1]);
        Print(4095, "wtm=%d  score=%d  %d (flop)  %d (flip)  %d (flop)\n",
            game_wtm, s1, s2, s3, s4);
      }
    }
  }
  input_stream = stdin;
}

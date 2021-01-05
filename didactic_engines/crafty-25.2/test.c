#include "chess.h"
#include "data.h"
/* last modified 01/09/15 */
/*
 *******************************************************************************
 *                                                                             *
 *   Test() is used to test the program against a suite of test positions to   *
 *   measure its performance on a particular machine, or to evaluate its skill *
 *   after modifying it in some way.                                           *
 *                                                                             *
 *   The test is initiated by using the "test <filename>" command to read in   *
 *   the suite of problems from file <filename>.  The format of this file is   *
 *   as follows:                                                               *
 *                                                                             *
 *   Setboard <forsythe-string>:  This sets the board position using the usual *
 *   forsythe notation (see module SetBoard() in setc for a full ex-           *
 *   planation of the syntax).                                                 *
 *                                                                             *
 *   Solution <move1> <move2> ... <moven>:  this provides a solution move (or  *
 *   set of solution moves if more than one is correct).  If the search finds  *
 *   one of these moves, then the prblem is counted as correct, otherwise it   *
 *   is counted wrong.                                                         *
 *                                                                             *
 *   After reading these two lines, the program then searches to whatever time *
 *   or depth limit has been set, when it reaches the end-of-file condition or *
 *   when it reads a record containing the string "end" it then displays the   *
 *   number correct and the number missed.                                     *
 *                                                                             *
 *   There are two test modules here.  Test() handles the specific Crafty test *
 *   data format (dates back to Cray Blitz days) while TestEPD() handles the   *
 *   EPD-style test positions which is more concise.  Other than the parsing   *
 *   differences, these are identical modules.                                 *
 *                                                                             *
 *******************************************************************************
 */
void Test(char *filename, FILE * unsolved, int screen, int margin) {
  TREE *const tree = block[0];
  FILE *test_input;
  uint64_t nodes = 0;
  int i, move, right = 0, wrong = 0, correct, time = 0, len, nfailed = 0;
  float avg_depth = 0.0;
  char failed[8][4096], *eof, *delim;

/*
 ************************************************************
 *                                                          *
 *  Read in the position and then the solutions.  After     *
 *  executing a search to find the best move (according to  *
 *  the program, anyway) compare it against the list of     *
 *  solutions and count it right or wrong.                  *
 *                                                          *
 ************************************************************
 */
  if (!(test_input = fopen(filename, "r"))) {
    printf("file %s does not exist.\n", filename);
    return;
  }
  Print(4095, "\n");
  eof = fgets(buffer, 4096, test_input);
  if (!strstr(buffer, "title")) {
    fclose(test_input);
    TestEPD(filename, unsolved, screen, margin);
    return;
  }
  if (book_file) {
    fclose(book_file);
    book_file = 0;
  }
  if (books_file) {
    fclose(books_file);
    books_file = 0;
  }
  fclose(test_input);
  test_input = fopen(filename, "r");
  while (FOREVER) {
    eof = fgets(buffer, 4096, test_input);
    strcpy(failed[nfailed++], buffer);
    if (eof) {
      delim = strchr(buffer, '\n');
      if (delim)
        *delim = 0;
      delim = strchr(buffer, '\r');
      if (delim)
        *delim = ' ';
    } else
      break;
    nargs = ReadParse(buffer, args, " \t;");
    if (!strcmp(args[0], "end"))
      break;
    else if (!strcmp(args[0], "title")) {
      Print(4095,
          "=============================================="
          "========================\n");
      Print(4095, "! ");
      len = 0;
      for (i = 1; i < nargs; i++) {
        Print(4095, "%s ", args[i]);
        len += strlen(args[i]) + 1;
        if (len > 65)
          break;
      }
      for (i = len; i < 67; i++)
        printf(" ");
      Print(4095, "!\n");
      Print(4095,
          "=============================================="
          "========================\n");
    } else if (strcmp(args[0], "solution")) {
      Option(tree);
    } else {
      number_of_solutions = 0;
      solution_type = 0;
      Print(4095, "solution ");
      for (i = 1; i < nargs; i++) {
        if (args[i][strlen(args[i]) - 1] == '?') {
          solution_type = 1;
          args[i][strlen(args[i]) - 1] = '\0';
        } else if (*(args + i)[strlen(args[i]) - 1] == '!') {
          solution_type = 0;
          args[i][strlen(args[i]) - 1] = '\0';
        }
        move = InputMove(tree, 0, game_wtm, 0, 0, args[i]);
        if (move) {
          solutions[number_of_solutions] = move;
          Print(4095, "%d. %s", (number_of_solutions++) + 1, OutputMove(tree,
                  0, game_wtm, move));
          if (solution_type == 1)
            Print(4095, "? ");
          else
            Print(4095, "  ");
        } else
          DisplayChessBoard(stdout, tree->position);
      }
      Print(4095, "\n");
      InitializeHashTables(0);
      last_pv.pathd = 0;
      thinking = 1;
      tree->status[1] = tree->status[0];
      Iterate(game_wtm, think, 0);
      thinking = 0;
      nodes += tree->nodes_searched;
      avg_depth += (float) iteration;
      time += (end_time - start_time);
      correct = solution_type;
      for (i = 0; i < number_of_solutions; i++) {
        if (!solution_type) {
          if (solutions[i] == (tree->pv[1].path[1] & 0x001fffff))
            correct = 1;
        } else if (solutions[i] == (tree->pv[1].path[1] & 0x001fffff))
          correct = 0;
      }
      if (correct) {
        right++;
        Print(4095, "----------------------> solution correct (%d/%d).\n",
            right, right + wrong);
      } else {
        wrong++;
        Print(4095, "----------------------> solution incorrect (%d/%d).\n",
            right, right + wrong);
        if (unsolved)
          for (i = 0; i < nfailed; i++)
            fputs(failed[i], unsolved);
      }
      nfailed = 0;
    }
  }
/*
 ************************************************************
 *                                                          *
 *  Now print the results.                                  *
 *                                                          *
 ************************************************************
 */
  if (right + wrong) {
    Print(4095, "\n\n\n");
    Print(4095, "test results summary:\n\n");
    Print(4095, "total positions searched..........%12d\n", right + wrong);
    Print(4095, "number right......................%12d\n", right);
    Print(4095, "number wrong......................%12d\n", wrong);
    Print(4095, "percentage right..................%12d\n",
        right * 100 / (right + wrong));
    Print(4095, "percentage wrong..................%12d\n",
        wrong * 100 / (right + wrong));
    Print(4095, "total nodes searched..............%12" PRIu64 "\n", nodes);
    Print(4095, "average search depth..............%12.1f\n",
        avg_depth / (right + wrong));
    Print(4095, "nodes per second..................%12" PRIu64 "\n",
        nodes * 100 / Max(time, 1));
    Print(4095, "total time........................%12s\n",
        DisplayTime(time));
  }
  input_stream = stdin;
  early_exit = 99;
  fclose(test_input);
}

/* last modified 06/26/15 */
/*
 *******************************************************************************
 *                                                                             *
 *   TestEPD() is used to test the program against a suite of test positions   *
 *   to measure its performance on a particular machine, or to evaluate its    *
 *   skill after modifying it in some way.                                     *
 *                                                                             *
 *   The test is initiated by using the "test <filename>" command to read in   *
 *   the suite of problems from file <filename>.  The format of this file is   *
 *   as follows:                                                               *
 *                                                                             *
 *   <forsythe-string>  am/bm move1 move2 etc; title "xxx"                     *
 *                                                                             *
 *   Am means "avoid move" and bm means "best move".  Each test position may   *
 *   have multiple moves to avoid or that are best, but both am and bm may not *
 *   appear on one position.                                                   *
 *                                                                             *
 *   The title is just a comment that is given in the program output to make   *
 *   it easier to match output to specific positions.                          *
 *                                                                             *
 *   One new addition is the ability to take a set of EPD records and run a    *
 *   search on each one.  If the final evaluation is within some window, then  *
 *   the input record is written out to a second file.  This is used to screen *
 *   cluster-testing starting positions to weed out those that are so badly    *
 *   unbalanced that one side always wins.                                     *
 *                                                                             *
 *******************************************************************************
 */
void TestEPD(char *filename, FILE * unsolved, int screen, int margin) {
  TREE *const tree = block[0];
  FILE *test_input, *test_output = 0;
  uint64_t nodes = 0;
  int i, move, right = 0, wrong = 0, correct, time = 0, len, culled = 0, r =
      0;
  float avg_depth = 0.0;
  char *eof, *mvs, *title, tbuffer[512], failed[4096];

/*
 ************************************************************
 *                                                          *
 *  Read in the position and then the solutions.  After     *
 *  executing a search to find the best move (according to  *
 *  the program, anyway) compare it against the list of     *
 *  solutions and count it right or wrong.                  *
 *                                                          *
 ************************************************************
 */
  if (!(test_input = fopen(filename, "r"))) {
    printf("file %s does not exist.\n", filename);
    return;
  }
  if (screen) {
    char outfile[256];

    strcpy(outfile, filename);
    strcat(outfile, ".screened");
    if (!(test_output = fopen(outfile, "w"))) {
      printf("file %s cannot be opened for write.\n", filename);
      return;
    }
  }
  if (book_file) {
    fclose(book_file);
    book_file = 0;
  }
  if (books_file) {
    fclose(books_file);
    books_file = 0;
  }
  while (FOREVER) {
    eof = fgets(buffer, 4096, test_input);
    strcpy(failed, buffer);
    Print(4095, "%s\n", buffer);
    strcpy(tbuffer, buffer);
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
    r++;
    mvs = strstr(buffer, " sd ");
    if (mvs) {
      search_depth = atoi(mvs + 3);
      *(mvs - 1) = 0;
      Print(4095, "search depth %d\n", search_depth);
    }
    mvs = strstr(buffer, " bm ");
    if (!mvs)
      mvs = strstr(buffer, " am ");
    if (!mvs && !screen)
      Print(4095, "Warning. am/bm field missing, input string follows\n%s\n",
          buffer);
    if (mvs)
      mvs++;
    title = strstr(buffer, "id");
    if (mvs)
      *(mvs - 1) = 0;
    if (title)
      *(title - 1) = 0;
    if (title) {
      title = strchr(title, '\"') + 1;
      if (title) {
        if (strchr(title, '\"')) {
          *strchr(title, '\"') = 0;
        }
      }
      Print(4095,
          "=============================================="
          "========================\n");
      Print(4095, "! ");
      Print(4095, "%s ", title);
      len = 66 - strlen(title);
      for (i = 0; i < len; i++)
        printf(" ");
      Print(4095, "!\n");
      Print(4095,
          "=============================================="
          "========================\n");
    }
    Option(tree);
    if (mvs) {
      nargs = ReadParse(mvs, args, " \t;");
      number_of_solutions = 0;
      solution_type = 0;
      if (!strcmp(args[0], "am"))
        solution_type = 1;
      Print(4095, "solution ");
      for (i = 1; i < nargs; i++) {
        if (!strcmp(args[i], "c0"))
          break;
        move = InputMove(tree, 0, game_wtm, 0, 0, args[i]);
        if (move) {
          solutions[number_of_solutions] = move;
          Print(4095, "%d. %s", (number_of_solutions++) + 1, OutputMove(tree,
                  0, game_wtm, move));
          if (solution_type == 1)
            Print(4095, "? ");
          else
            Print(4095, "  ");
        } else
          DisplayChessBoard(stdout, tree->position);
      }
    }
    Print(4095, "\n");
    InitializeHashTables(0);
    last_pv.pathd = 0;
    thinking = 1;
    tree->status[1] = tree->status[0];
    Iterate(game_wtm, think, 0);
    if (screen) {
      if (Abs(last_root_value) < margin)
        fwrite(tbuffer, 1, strlen(tbuffer), test_output);
      else
        culled++;
      printf("record #%d,  culled %d, score=%s          \r", r, culled,
          DisplayEvaluation(last_root_value, game_wtm));
      fflush(stdout);
    }
    thinking = 0;
    nodes += tree->nodes_searched;
    avg_depth += (float) iteration;
    time += (end_time - start_time);
    if (!screen) {
      correct = solution_type;
      for (i = 0; i < number_of_solutions; i++) {
        if (!solution_type) {
          if (solutions[i] == (tree->pv[1].path[1] & 0x001fffff))
            correct = 1;
        } else if (solutions[i] == (tree->pv[1].path[1] & 0x001fffff))
          correct = 0;
      }
      if (correct) {
        right++;
        Print(4095, "----------------------> solution correct (%d/%d).\n",
            right, right + wrong);
      } else {
        wrong++;
        Print(4095, "----------------------> solution incorrect (%d/%d).\n",
            right, right + wrong);
        if (unsolved)
          fputs(failed, unsolved);
      }
    }
  }
/*
 ************************************************************
 *                                                          *
 *  Now print the results.                                  *
 *                                                          *
 ************************************************************
 */
  if (r) {
    Print(4095, "\n\n\n");
    Print(4095, "test results summary:\n\n");
    Print(4095, "total positions searched..........%12d\n", r);
    if (!screen) {
      Print(4095, "number right......................%12d\n", right);
      Print(4095, "number wrong......................%12d\n", wrong);
      Print(4095, "percentage right..................%12d\n",
          right * 100 / (right + wrong));
      Print(4095, "percentage wrong..................%12d\n",
          wrong * 100 / (right + wrong));
    } else
      Print(4095, "records excluded..................%12d\n", culled);

    Print(4095, "total nodes searched..............%12" PRIu64 "\n", nodes);
    Print(4095, "average search depth..............%12.1f\n", avg_depth / r);
    Print(4095, "nodes per second..................%12" PRIu64 "\n",
        nodes * 100 / Max(1, time));
    Print(4095, "total time........................%12s\n",
        DisplayTime(time));
  }
  input_stream = stdin;
  early_exit = 99;
  fclose(test_input);
}

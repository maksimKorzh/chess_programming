#include "chess.h"
#include "data.h"
/* last modified 01/18/09 */
/*
 *******************************************************************************
 *                                                                             *
 *   Analyze() is used to handle the "analyze" command.  This mode basically   *
 *   puts Crafty into a "permanent pondering" state, where it reads a move     *
 *   from the input stream, and then "ponders" for the opposite side.          *
 *   Whenever a move is entered, Crafty reads this move, updates the game      *
 *   board, and then starts "pondering" for the other side.                    *
 *                                                                             *
 *   The purpose of this mode is to force Crafty to follow along in a game,    *
 *   providing analysis continually for the side on move until a move is       *
 *   entered, advancing the game to the next position.                         *
 *                                                                             *
 *******************************************************************************
 */
void Analyze() {
  int i, v, move, back_number, readstat = 1;
  TREE *const tree = block[0];

/*
 ************************************************************
 *                                                          *
 *  Initialize.                                             *
 *                                                          *
 ************************************************************
 */
  int save_swindle_mode = swindle_mode;

  swindle_mode = 0;
  ponder_move = 0;
  analyze_mode = 1;
  if (!xboard)
    display_options |= 1 + 2 + 4;
  printf("Analyze Mode: type \"exit\" to terminate.\n");
/*
 ************************************************************
 *                                                          *
 *  Now loop waiting on input, searching the current        *
 *  position continually until a move comes in.             *
 *                                                          *
 ************************************************************
 */
  while (FOREVER) {
    do {
      last_pv.pathd = 0;
      last_pv.pathl = 0;
      input_status = 0;
      pondering = 1;
      tree->status[1] = tree->status[0];
      Iterate(game_wtm, think, 0);
      pondering = 0;
      if (book_move)
        moves_out_of_book = 0;
      if (!xboard) {
        if (game_wtm)
          printf("analyze.White(%d): ", move_number);
        else
          printf("analyze.Black(%d): ", move_number);
        fflush(stdout);
      }
/*
 ************************************************************
 *                                                          *
 *  If we get back to here, something has been typed in and *
 *  is in the command buffer normally, unless the search    *
 *  terminated naturally due to finding a mate or reaching  *
 *  the max depth allowable.                                *
 *                                                          *
 ************************************************************
 */
      if (!input_status)
        do {
          readstat = Read(1, buffer);
          if (readstat < 0)
            break;
          nargs = ReadParse(buffer, args, " \t;");
          Print(32, "%s\n", buffer);
          if (strstr(args[0], "timeleft") && !xboard) {
            if (game_wtm)
              printf("analyze.White(%d): ", move_number);
            else
              printf("analyze.Black(%d): ", move_number);
            fflush(stdout);
          }
        } while (strstr(args[0], "timeleft"));
      else
        nargs = ReadParse(buffer, args, " \t;");
      if (readstat < 0)
        break;
      move = 0;
      if (!strcmp(args[0], "exit"))
        break;
/*
 ************************************************************
 *                                                          *
 *  First, check for the special analyze command "back n"   *
 *  and handle it if present, otherwise try Option() to see *
 *  if it recognizes the input as a command.                *
 *                                                          *
 ************************************************************
 */
      if (OptionMatch("back", args[0])) {
        if (nargs > 1)
          back_number = atoi(args[1]);
        else
          back_number = 1;
        for (i = 0; i < back_number; i++) {
          game_wtm = Flip(game_wtm);
          if (Flip(game_wtm))
            move_number--;
        }
        if (move_number == 0) {
          move_number = 1;
          game_wtm = 1;
        }
        sprintf(buffer, "reset %d", move_number);
        Option(tree);
        display = tree->position;
      } else if (Option(tree)) {
        display = tree->position;
      }
/*
 ************************************************************
 *                                                          *
 *  If InputMove() can recognize this as a move, make it,   *
 *  swap sides, and return to the top of the loop to call   *
 *  search from this new position.                          *
 *                                                          *
 ************************************************************
 */
      else if ((move = InputMove(tree, 0, game_wtm, 1, 0, buffer))) {
        char *outmove = OutputMove(tree, 0, game_wtm, move);

        if (history_file) {
          fseek(history_file, ((move_number - 1) * 2 + 1 - game_wtm) * 10,
              SEEK_SET);
          fprintf(history_file, "%9s\n", outmove);
        }
        if (game_wtm)
          Print(32, "White(%d): ", move_number);
        else
          Print(32, "Black(%d): ", move_number);
        Print(32, "%s\n", outmove);
        if (speech) {
          char announce[64];

          strcpy(announce, "./speak ");
          strcat(announce, outmove);
          strcat(announce, " &");
          v = system(announce);
          if (v != 0)
            perror("Analyze() system() error: ");
        }
        MakeMoveRoot(tree, game_wtm, move);
        display = tree->position;
        last_mate_score = 0;
        if (log_file)
          DisplayChessBoard(log_file, tree->position);
      }
/*
 ************************************************************
 *                                                          *
 *  If Option() didn't handle the input, then it is illegal *
 *  and should be reported to the user.                     *
 *                                                          *
 ************************************************************
 */
      else {
        pondering = 0;
        if (Option(tree) == 0)
          printf("illegal move: %s\n", buffer);
        pondering = 1;
        display = tree->position;
      }
    } while (!move);
    if (readstat < 0 || !strcmp(args[0], "exit"))
      break;
    game_wtm = Flip(game_wtm);
    if (game_wtm)
      move_number++;
  }
  analyze_mode = 0;
  printf("analyze complete.\n");
  pondering = 0;
  swindle_mode = save_swindle_mode;
}

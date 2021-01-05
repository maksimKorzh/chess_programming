#include <signal.h>
#include "chess.h"
#include "data.h"
/* last modified 01/17/09 */
/*
 *******************************************************************************
 *                                                                             *
 *   Interrupt() is used to read in a move when the operator types something   *
 *   while a search is in progress (during pondering as one example.)  This    *
 *   routine reads in a command (move) and then makes two attempts to use this *
 *   input:  (1) call Option() to see if the command can be executed;  (2) try *
 *   InputMove() to see if this input is a legal move; If so, and we are       *
 *   pondering see if it matches the move we are pondering.                    *
 *                                                                             *
 *******************************************************************************
 */
void Interrupt(int ply) {
  TREE *const tree = block[0];
  int temp, i, left = 0, readstat, result, time_used, save_move_number;

/*
 ************************************************************
 *                                                          *
 *  If trying to find a move to ponder, and the operator    *
 *  types a command, exit a.s.a.p.                          *
 *                                                          *
 ************************************************************
 */
  if (puzzling)
    abort_search = 2;
/*
 ************************************************************
 *                                                          *
 *  First check to see if this is a command by calling      *
 *  Option().  Option() will return a 0 if it didn't        *
 *  recognize the command; otherwise it returns a 1 if the  *
 *  command was executed, or a 2 if we need to abort the    *
 *  search to execute the command.                          *
 *                                                          *
 ************************************************************
 */
  else
    while (FOREVER) {
      readstat = Read(0, buffer);
      if (readstat <= 0)
        break;
      nargs = ReadParse(buffer, args, " \t;");
      if (nargs == 0) {
        Print(32, "ok.\n");
        break;
      }
      if (strcmp(args[0], ".")) {
        save_move_number = move_number;
        if (!game_wtm)
          move_number--;
        if (root_wtm)
          Print(32, "Black(%d): %s\n", move_number, buffer);
        else
          Print(32, "White(%d): %s\n", move_number, buffer);
        move_number = save_move_number;
      }
/*
 ************************************************************
 *                                                          *
 *  "." command displays status of current search (this is  *
 *  part of winboard protocol.)                             *
 *                                                          *
 ************************************************************
 */
      if (!strcmp(args[0], ".")) {
        if (xboard) {
          end_time = ReadClock();
          time_used = (end_time - start_time);
          printf("stat01: %d ", time_used);
          printf("%" PRIu64 " ", tree->nodes_searched);
          printf("%d ", iteration);
          for (i = 0; i < n_root_moves; i++)
            if (!(root_moves[i].status & 8))
              left++;
          printf("%d %d\n", left, n_root_moves);
          fflush(stdout);
          break;
        } else {
          end_time = ReadClock();
          time_used = (end_time - start_time);
          printf("time:%s ", DisplayTime(time_used));
          printf("nodes:%" PRIu64 "\n", tree->nodes_searched);
#if (CPUS > 1)
          ThreadTrace(block[0], 0, 1);
#endif
        }
      }
/*
 ************************************************************
 *                                                          *
 *  "?" command says "move now!"                            *
 *                                                          *
 ************************************************************
 */
      else if (!strcmp(args[0], "?")) {
        if (thinking) {
          abort_search = 1;
        }
      }
/*
 ************************************************************
 *                                                          *
 *  "@" command says "assume ponder move was played."       *
 *                                                          *
 ************************************************************
 */
      else if (!strcmp(args[0], "@")) {
        if (pondering) {
          predicted++;
          input_status = 1;
          pondering = 0;
          thinking = 1;
          opponent_end_time = ReadClock();
          program_start_time = ReadClock();
          Print(32, "predicted move made.\n");
        }
      }
/*
 ************************************************************
 *                                                          *
 *  Next see if Option() recognizes this as a command. Note *
 *  some commands can't be executed in the middle of a      *
 *  search.  Option() returns a value >= 2 in such cases.   *
 *  If we are pondering, we can back out of the search and  *
 *  execute the command, otherwise we produce an error and  *
 *  continue searching for our move.                        *
 *                                                          *
 ************************************************************
 */
      else {
        save_move_number = move_number;
        if (!analyze_mode && !game_wtm)
          move_number--;
        result = Option(tree);
        move_number = save_move_number;
        if (result >= 2) {
          if (thinking && result != 3)
            Print(32, "command not legal now.\n");
          else {
            abort_search = 2;
            input_status = 2;
            break;
          }
        } else if ((result != 1) && analyze_mode) {
          abort_search = 1;
          input_status = 2;
          break;
        }
/*
 ************************************************************
 *                                                          *
 *  Option() didn't recognize the input as a command so now *
 *  we check to see if the operator typed a move.  If so,   *
 *  and it matched the predicted move, switch from          *
 *  pondering to thinking to start the timer.  If this is a *
 *  move, but not the predicted move, abort the search and  *
 *  start over with the right move.                         *
 *                                                          *
 ************************************************************
 */
        else if (!result) {
          if (pondering) {
            nargs = ReadParse(buffer, args, " \t;");
            temp = InputMove(tree, 0, Flip(root_wtm), 1, 1, args[0]);
            if (temp) {
              if ((From(temp) == From(ponder_move))
                  && (To(temp) == To(ponder_move))
                  && (Piece(temp) == Piece(ponder_move))
                  && (Captured(temp) == Captured(ponder_move))
                  && (Promote(temp) == Promote(ponder_move))) {
                predicted++;
                input_status = 1;
                pondering = 0;
                thinking = 1;
                opponent_end_time = ReadClock();
                program_start_time = ReadClock();
                Print(32, "predicted move made.\n");
              } else {
                input_status = 2;
                abort_search = 2;
                break;
              }
            } else if (!strcmp(args[0], "go") || !strcmp(args[0], "move")
                || !strcmp(args[0], "SP")) {
              abort_search = 2;
              break;
            } else
              Print(4095, "Illegal move: %s\n", args[0]);
          } else
            Print(4095, "unrecognized/illegal command: %s\n", args[0]);
        }
      }
    }
  if (log_file)
    fflush(log_file);
}

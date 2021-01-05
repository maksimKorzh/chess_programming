#include <ctype.h>
#include <signal.h>
#include "chess.h"
#include "data.h"
#if defined(UNIX)
#  include <unistd.h>
#  include <signal.h>
#endif
#include "epdglue.h"
#include "tbprobe.h"
/* last modified 08/03/16 */
/*
 *******************************************************************************
 *                                                                             *
 *   Option() is used to handle user input necessary to control/customize the  *
 *   program.  It performs all functions excepting chess move input which is   *
 *   handled by main().                                                        *
 *                                                                             *
 *******************************************************************************
 */
int Option(TREE * RESTRICT tree) {
  int v;

/*
 ************************************************************
 *                                                          *
 *  parse the input.  If it looks like a FEN string, don't  *
 *  parse using "/" as a separator, otherwise do.           *
 *                                                          *
 ************************************************************
 */
  if (StrCnt(buffer, '/') >= 7)
    nargs = ReadParse(buffer, args, " \t;=");
  else
    nargs = ReadParse(buffer, args, " \t;=/");
  if (!nargs)
    return 1;
  if (args[0][0] == '#')
    return 1;
/*
 ************************************************************
 *                                                          *
 *  EPD implementation interface code.  EPD commands can    *
 *  not be handled if the program is actually searching in  *
 *  a real game, and if Crafty is "pondering" this has to   *
 *  be stopped.                                             *
 *                                                          *
 ************************************************************
 */
#if defined(EPD)
  if (initialized) {
    if (EGCommandCheck(buffer)) {
      if (thinking || pondering)
        return 2;
      else {
        EGCommand(buffer);
        return 1;
      }
    }
  }
#endif
/*
 ************************************************************
 *                                                          *
 *  "!" character is a 'shell escape' that passes the rest  *
 *  of the command to a shell for execution.  Note that it  *
 *  is ignored if in xboard mode because one could use your *
 *  zippy2password to execute commands on your local        *
 *  machine, probably something that is not wanted.         *
 *                                                          *
 ************************************************************
 */
  if (buffer[0] == '!') {
    if (!xboard) {
      v = system(strchr(buffer, '!') + 1);
      if (v != 0)
        perror("Option() system() error: ");
    }
  }
/*
 ************************************************************
 *                                                          *
 *  "." ignores "." if it happens to get to this point, if  *
 *  xboard is running.                                      *
 *                                                          *
 ************************************************************
 */
  else if (OptionMatch(".", *args)) {
    if (xboard) {
      printf("stat01: 0 0 0 0 0\n");
      fflush(stdout);
      return 1;
    } else
      return 0;
  }
/*
 ************************************************************
 *                                                          *
 *  "accepted" handles the new xboard protocol version 2    *
 *  accepted command.                                       *
 *                                                          *
 ************************************************************
 */
  else if (OptionMatch("accepted", *args)) {
  }
/*
 ************************************************************
 *                                                          *
 *  "adaptive" sets the new adaptive hash algorithm         *
 *  parameters.  It requires five parameters.  The first is *
 *  an estimated NPS, the second is the minimum hash size,  *
 *  and the third is the maximum hash size.  The adaptive   *
 *  algorithm will look at the time control, and try to     *
 *  adjust the hash sizes to an optimal value without       *
 *  dropping below the minimum or exceeding the maximum     *
 *  memory size given.  The min/max sizes can be given      *
 *  using the same syntax as the hash= command, ie xxx,     *
 *  xxxK or xxxM will all work. The fourth and fifth        *
 *  parameters are used to limit hashp in the same way.     *
 *                                                          *
 ************************************************************
 */
  else if (OptionMatch("adaptive", *args)) {
    if (nargs != 6) {
      printf("usage:  adaptive NPS hmin hmax pmin pmax\n");
      return 1;
    }
    if (nargs > 1) {
      adaptive_hash = atoiKMB(args[1]);
      adaptive_hash_min = atoiKMB(args[2]);
      adaptive_hash_max = atoiKMB(args[3]);
      adaptive_hashp_min = atoiKMB(args[4]);
      adaptive_hashp_max = atoiKMB(args[5]);
    }
    Print(32, "adaptive estimated NPS =  %s\n", DisplayKMB(adaptive_hash, 1));
    Print(32, "adaptive minimum hsize =  %s\n", DisplayKMB(adaptive_hash_min,
            1));
    Print(32, "adaptive maximum hsize =  %s\n", DisplayKMB(adaptive_hash_max,
            1));
    Print(32, "adaptive minimum psize =  %s\n", DisplayKMB(adaptive_hashp_min,
            1));
    Print(32, "adaptive maximum psize =  %s\n", DisplayKMB(adaptive_hashp_max,
            1));
  }
/*
 ************************************************************
 *                                                          *
 *  "alarm" command turns audible move warning on/off.      *
 *                                                          *
 ************************************************************
 */
  else if (OptionMatch("alarm", *args)) {
    if (!strcmp(args[1], "on"))
      audible_alarm = 0x07;
    else if (!strcmp(args[1], "off"))
      audible_alarm = 0x00;
    else
      printf("usage:  alarm on|off\n");
  }
/*
 ************************************************************
 *                                                          *
 *  "analyze" puts Crafty in analyze mode, where it reads   *
 *  moves in and between moves, computes as though it is    *
 *  trying to find the best move to make.  When another     *
 *  move is entered, it switches sides and continues.  It   *
 *  will never make a move on its own, rather, it will      *
 *  continue to analyze until an "exit" command is given.   *
 *                                                          *
 ************************************************************
 */
  else if (OptionMatch("analyze", *args)) {
    if (thinking || pondering)
      return 2;
    Analyze();
  }
/*
 ************************************************************
 *                                                          *
 *  "annotate" command is used to read a series of moves    *
 *  and analyze the resulting game, producing comments as   *
 *  requested by the user.  This also handles the annotateh *
 *  (html) and annotatet (LaTex) output forms of the        *
 *  command.                                                *
 *                                                          *
 ************************************************************
 */
  else if (OptionMatch("annotate", *args) || OptionMatch("annotateh", *args)
      || OptionMatch("annotatet", *args)) {
    if (thinking || pondering)
      return 2;
    Annotate();
  }
/*
 ************************************************************
 *                                                          *
 *  "autotune" command is used to automatically tune the    *
 *  SMP search parameters that affect search efficiency.    *
 *                                                          *
 ************************************************************
 */
  else if (OptionMatch("autotune", *args)) {
    if (thinking || pondering)
      return 2;
    AutoTune(nargs, args);
  }
/*
 ************************************************************
 *                                                          *
 *  "batch" command disables asynchronous I/O so that a     *
 *  stream of commands can be put into a file and they are  *
 *  not executed instantly.                                 *
 *                                                          *
 ************************************************************
 */
  else if (OptionMatch("batch", *args)) {
    if (!strcmp(args[1], "on"))
      batch_mode = 1;
    else if (!strcmp(args[1], "off"))
      batch_mode = 0;
    else
      printf("usage:  batch on|off\n");
  }
/*
 ************************************************************
 *                                                          *
 *  "beep" command is ignored. [xboard compatibility]       *
 *                                                          *
 ************************************************************
 */
  else if (OptionMatch("beep", *args)) {
    return xboard;
  }
/*
 ************************************************************
 *                                                          *
 *  "bench" runs internal performance benchmark.  An        *
 *  optional second argument can increase or decrease the   *
 *  time it takes.  "bench 1" increases the default depth   *
 *  by one ply, and "bench -1" reduces the depth to speed   *
 *  it up.                                                  *
 *                                                          *
 ************************************************************
 */
  else if (OptionMatch("bench", *args)) {
    int mod = 0;

    if (nargs > 1)
      mod = atoi(args[1]);
    (void) Bench(mod, 0);
  }
/*
 ************************************************************
 *                                                          *
 *  "bk"  book command from xboard sends the suggested book *
 *  moves.                                                  *
 *                                                          *
 ************************************************************
 */
  else if (OptionMatch("bk", *args)) {
    printf("\t%s\n\n", book_hint);
    fflush(stdout);
    return xboard;
  }
/*
 ************************************************************
 *                                                          *
 *  "black" command sets black to move (Flip(wtm)).         *
 *                                                          *
 ************************************************************
 */
  else if (OptionMatch("white", *args)) {
    if (thinking || pondering)
      return 2;
    game_wtm = 1;
    ponder_move = 0;
    last_pv.pathd = 0;
    last_pv.pathl = 0;
    if (!game_wtm)
      Pass();
    force = 0;
  } else if (OptionMatch("black", *args)) {
    if (thinking || pondering)
      return 2;
    game_wtm = 0;
    ponder_move = 0;
    last_pv.pathd = 0;
    last_pv.pathl = 0;
    if (game_wtm)
      Pass();
    force = 0;
  }
/*
 ************************************************************
 *                                                          *
 *  "bogus" command is ignored. [xboard compatibility]      *
 *                                                          *
 ************************************************************
 */
  else if (OptionMatch("bogus", *args)) {
    return xboard;
  }
/*
 ************************************************************
 *                                                          *
 *  "book" command updates/creates the opening book file.   *
 *                                                          *
 ************************************************************
 */
  else if (OptionMatch("book", *args)) {
    nargs = ReadParse(buffer, args, " \t;");
    Bookup(tree, nargs, args);
  } else if (!strcmp("create", *(args + 1))) {
    nargs = ReadParse(buffer, args, " \t;");
    Bookup(tree, nargs, args);
  }
/*
 ************************************************************
 *                                                          *
 *  "bookw" command updates the book selection weights.     *
 *                                                          *
 ************************************************************
 */
  else if (OptionMatch("bookw", *args)) {
    if (nargs > 1) {
      if (OptionMatch("frequency", args[1]))
        book_weight_freq = atof(args[2]);
      else if (OptionMatch("evaluation", args[1]))
        book_weight_eval = atof(args[2]);
      else if (OptionMatch("learning", args[1]))
        book_weight_learn = atof(args[2]);
    } else {
      Print(32, "frequency (freq)..............%4.2f\n", book_weight_freq);
      Print(32, "static evaluation (eval)......%4.2f\n", book_weight_eval);
      Print(32, "learning (learn)..............%4.2f\n", book_weight_learn);
    }
  }
/*
 ************************************************************
 *                                                          *
 *  "clock" command displays chess clock.                   *
 *                                                          *
 ************************************************************
 */
  else if (OptionMatch("clock", *args)) {
    int side;

    for (side = white; side >= black; side--) {
      Print(32, "time remaining (%s): %s", (side) ? "white" : "black",
          DisplayHHMMSS(tc_time_remaining[side]));
      if (tc_sudden_death != 1)
        Print(32, "  (%d more moves)", tc_moves_remaining[side]);
      Print(32, "\n");
    }
    Print(32, "\n");
    if (tc_sudden_death == 1)
      Print(32, "Sudden-death time control in effect\n");
  }
/*
 ************************************************************
 *                                                          *
 *  "computer" lets Crafty know it is playing a computer.   *
 *                                                          *
 ************************************************************
 */
  else if (OptionMatch("computer", *args)) {
    Print(32, "playing a computer!\n");
    accept_draws = 1;
    if (resign)
      resign = 10;
    resign_count = 4;
    usage_level = 0;
    books_file = (computer_bs_file) ? computer_bs_file : normal_bs_file;
  }
/*
 ************************************************************
 *                                                          *
 *  "display" command displays the chess board.             *
 *                                                          *
 *  "display" command sets specific display options which   *
 *  control how "chatty" the program is.  In the variable   *
 *  display_options, the following bits are set/cleared     *
 *  based on the option chosen.                             *
 *                                                          *
 *    1 -> display move/time/results/etc.                   *
 *    2 -> display PV.                                      *
 *    4 -> display fail high / fail low moves               *
 *    8 -> display search statistics.                       *
 *   16 -> display root moves as they are searched.         *
 *   32 -> display general informational messages.          *
 *   64 -> display ply-1 move list / flags after each       *
 *         iteration.                                       *
 *  128 -> display root moves and scores before search      *
 *         begins.                                          *
 * 2048 -> error messages (can not be disabled).            *
 *                                                          *
 ************************************************************
 */
  else if (OptionMatch("display", *args)) {
    int i, set, old_display_options = display_options;
    char *doptions[8] = { "moveinfo", "pv", "fail", "stats", "moves", "info",
      "ply1", "movelist"
    };
    char *descriptions[8] = { "display move time/results/etc",
      "principal variation", "fail highs/lows", "search statistics",
      "root moves as they are searched", "general information",
      "ply1 move list after each iteration",
      "root move list and scores prior to search"
    };

    if (nargs > 1) {
      if (!strcmp(args[1], "all"))
        old_display_options = ~display_options;
      for (i = 0; i < 8; i++) {
        if (strstr(args[1], doptions[i])) {
          if (strstr(args[1], "no"))
            set = 0;
          else
            set = 1;
          display_options &= ~(1 << i);
          display_options |= set << i;
          break;
        }
      }
      for (i = 0; i < 8; i++) {
        if ((old_display_options & (1 << i)) != (display_options & (1 << i))) {
          Print(32, "display ");
          if (!(display_options & (1 << i)))
            Print(32, "no");
          Print(32, "%s (%s)\n", doptions[i], descriptions[i]);
        }
      }
    } else
      DisplayChessBoard(stdout, display);
  }
/*
 ************************************************************
 *                                                          *
 *  "debug" handles the new debug command that is often     *
 *  modified to test some modified code function.           *
 *                                                          *
 ************************************************************
 */
  else if (OptionMatch("debug", *args)) {
    Print(32, "ERROR:  no debug code included\n");
  }
/*
 ************************************************************
 *                                                          *
 *  "depth" command sets a specific search depth to         *
 *  control the tree search depth. [xboard compatibility].  *
 *                                                          *
 ************************************************************
 */
  else if (OptionMatch("depth", *args)) {
    if (nargs < 2) {
      printf("usage:  depth <n>\n");
      return 1;
    }
    search_depth = atoi(args[1]);
    Print(32, "search depth set to %d.\n", search_depth);
  }
/*
 ************************************************************
 *                                                          *
 *  "draw" is used to offer Crafty a draw, or to control    *
 *  whether Crafty will offer and/or accept draw offers.    *
 *                                                          *
 ************************************************************
 */
  else if (OptionMatch("draw", *args)) {
    if (nargs == 1) {
      draw_offer_pending = 1;
      if (draw_offered) {
        Print(4095, "1/2-1/2 {Draw agreed}\n");
        strcpy(pgn_result, "1/2-1/2");
      }
    } else {
      if (!strcmp(args[1], "accept")) {
        accept_draws = 1;
        Print(32, "accept draw offers\n");
      } else if (!strcmp(args[1], "decline")) {
        accept_draws = 0;
        Print(32, "decline draw offers\n");
      } else if (!strcmp(args[1], "dynamic")) {
        if (nargs > 2)
          dynamic_draw_score = atoi(args[2]);
        Print(32, "dynamic draw scores %s\n",
            (dynamic_draw_score) ? "enabled" : "disabled");
      } else if (!strcmp(args[1], "offer")) {
        offer_draws = 1;
        Print(32, "offer draws\n");
      } else if (!strcmp(args[1], "nooffer")) {
        offer_draws = 0;
        Print(32, "do not offer draws\n");
      } else
        Print(32, "usage: draw accept|decline|offer|nooffer\n");
    }
  }
/*
 ************************************************************
 *                                                          *
 *  "easy" command disables thinking on opponent's time.    *
 *                                                          *
 ************************************************************
 */
  else if (OptionMatch("easy", *args)) {
    if (thinking || pondering)
      return 2;
    ponder = 0;
    Print(32, "pondering disabled.\n");
  }
/*
 ************************************************************
 *                                                          *
 *  "echo" command displays messages from command file.     *
 *                                                          *
 ************************************************************
 */
  else if (OptionMatch("echo", *args) || OptionMatch("title", *args)) {
  }
/*
 ************************************************************
 *                                                          *
 *  "edit" command modifies the board position.             *
 *                                                          *
 ************************************************************
 */
  else if (OptionMatch("edit", *args)) {
    if (thinking || pondering)
      return 2;
    Edit();
    move_number = 1; /* discard history */
    if (!game_wtm) {
      game_wtm = 1;
      Pass();
    }
    ponder_move = 0;
    last_pv.pathd = 0;
    last_pv.pathl = 0;
    strcpy(buffer, "savepos *");
    Option(tree);
  }
/*
 ************************************************************
 *                                                          *
 *  "egtb" command enables syzygy endgame database tables.  *
 *                                                          *
 ************************************************************
 */
  else if (OptionMatch("egtb", *args)) {
#if defined(SYZYGY)
    if (!strcmp(args[1], "off"))
      EGTBlimit = 0;
    else {
      if (!EGTB_setup) {
        tb_init(tb_path);
        EGTB_setup = 1;
      }
      EGTBlimit = TB_LARGEST;
    }
    if (EGTBlimit)
      Print(32, "SYZYGY EGTB access enabled, %d piece TBs found\n",
          TB_LARGEST);
    else
      Print(32, "SYZYGY EGTB access disabled.\n");
#else
    Print(32, "SYZYGY support not included (no -DSYZYGY)\n");
#endif
  }
/*
 ************************************************************
 *                                                          *
 *  "egtbd" command sets the probe depth limit.  If the     *
 *  remaining depth is < this limit, probes are not done to *
 *  avoid slowing the search unnecessarily.                 *
 *                                                          *
 ************************************************************
 */
#if defined(SYZYGY)
  else if (OptionMatch("egtbd", *args)) {
    if (nargs > 1)
      EGTB_depth = atoi(args[1]);
    Print(32, "EGTB probe depth set to %d\n", EGTB_depth);
  }
#endif
/*
 ************************************************************
 *                                                          *
 *  "end" (or "quit") command terminates the program.       *
 *                                                          *
 ************************************************************
 */
  else if (OptionMatch("end", *args) || OptionMatch("quit", *args)) {
    abort_search = 1;
    quit = 1;
    last_search_value =
        (crafty_is_white) ? last_search_value : -last_search_value;
    if (moves_out_of_book)
      LearnBook();
    if (thinking || pondering)
      return 1;
    CraftyExit(0);
  }
/*
 ************************************************************
 *                                                          *
 *  "evtest" command runs a test suite of problems and      *
 *  prints evaluations only.                                *
 *                                                          *
 ************************************************************
 */
  else if (OptionMatch("evtest", *args)) {
    if (thinking || pondering)
      return 2;
    if (nargs < 2) {
      printf("usage:  evtest <filename>\n");
      return 1;
    }
    EVTest(args[1]);
    ponder_move = 0;
    last_pv.pathd = 0;
    last_pv.pathl = 0;
  }
/*
 ************************************************************
 *                                                          *
 *  "exit" command resets input device to STDIN.            *
 *                                                          *
 ************************************************************
 */
  else if (OptionMatch("exit", *args)) {
    if (analyze_mode)
      return 0;
    if (input_stream != stdin)
      fclose(input_stream);
    input_stream = stdin;
    ReadClear();
    Print(32, "\n");
  }
/*
 ************************************************************
 *                                                          *
 *  "flag" command controls whether Crafty will call the    *
 *  flag in xboard/winboard games (to end the game.)        *
 *                                                          *
 ************************************************************
 */
  else if (OptionMatch("flag", *args)) {
    if (nargs < 2) {
      printf("usage:  flag on|off\n");
      return 1;
    }
    if (!strcmp(args[1], "on"))
      call_flag = 1;
    else if (!strcmp(args[1], "off"))
      call_flag = 0;
    if (call_flag)
      Print(32, "end game on time forfeits\n");
    else
      Print(32, "ignore time forfeits\n");
  }
/*
 ************************************************************
 *                                                          *
 *  "flip" command flips the board, interchanging each      *
 *  rank with the corresponding rank on the other half of   *
 *  the board, and also reverses the color of all pieces.   *
 *                                                          *
 ************************************************************
 */
  else if (OptionMatch("flip", *args)) {
    int file, rank, piece, temp;

    if (thinking || pondering)
      return 2;
    for (rank = 0; rank < 4; rank++) {
      for (file = 0; file < 8; file++) {
        piece = -PcOnSq((rank << 3) + file);
        PcOnSq((rank << 3) + file) = -PcOnSq(((7 - rank) << 3) + file);
        PcOnSq(((7 - rank) << 3) + file) = piece;
      }
    }
    game_wtm = Flip(game_wtm);
    temp = Castle(0, white);
    Castle(0, white) = Castle(0, black);
    Castle(0, black) = temp;
    SetChessBitBoards(tree);
#if defined(DEBUG)
    ValidatePosition(tree, 0, game_wtm, "Option().flip");
#endif
  }
/*
 ************************************************************
 *                                                          *
 *  "flop" command flops the board, interchanging each      *
 *  file with the corresponding file on the other half of   *
 *  the board.                                              *
 *                                                          *
 ************************************************************
 */
  else if (OptionMatch("flop", *args)) {
    int file, rank, piece;

    if (thinking || pondering)
      return 2;
    for (rank = 0; rank < 8; rank++) {
      for (file = 0; file < 4; file++) {
        piece = PcOnSq((rank << 3) + file);
        PcOnSq((rank << 3) + file) = PcOnSq((rank << 3) + 7 - file);
        PcOnSq((rank << 3) + 7 - file) = piece;
      }
    }
    SetChessBitBoards(tree);
#if defined(DEBUG)
    ValidatePosition(tree, 0, game_wtm, "Option().flop");
#endif
  }
/*
 ************************************************************
 *                                                          *
 *  "force" command forces the program to make a specific   *
 *  move instead of its last chosen move.                   *
 *                                                          *
 ************************************************************
 */
  else if (OptionMatch("force", *args)) {
    int move, movenum, save_move_number;
    char text[16];

    if (thinking || pondering)
      return 3;
    if (xboard) {
      force = 1;
      return 3;
    }
    if (nargs < 2) {
      printf("usage:  force <move>\n");
      return 1;
    }
    ponder_move = 0;
    presult = 0;
    last_pv.pathd = 0;
    last_pv.pathl = 0;
    save_move_number = move_number;
    movenum = move_number;
    if (game_wtm)
      movenum--;
    strcpy(text, args[1]);
    sprintf(buffer, "reset %d", movenum);
    game_wtm = Flip(game_wtm);
    Option(tree);
    move = InputMove(tree, 0, game_wtm, 0, 0, text);
    if (move) {
      if (input_stream != stdin)
        printf("%s\n", OutputMove(tree, 0, game_wtm, move));
      if (history_file) {
        fseek(history_file, ((movenum - 1) * 2 + 1 - game_wtm) * 10,
            SEEK_SET);
        fprintf(history_file, "%9s\n", OutputMove(tree, 0, game_wtm, move));
      }
      MakeMoveRoot(tree, game_wtm, move);
      last_pv.pathd = 0;
      last_pv.pathl = 0;
    } else if (input_stream == stdin)
      printf("illegal move.\n");
    game_wtm = Flip(game_wtm);
    move_number = save_move_number;
    strcpy(ponder_text, "none");
  }
/*
 ************************************************************
 *                                                          *
 *  "go" command does nothing, except force main() to start *
 *  a search.  ("move" is an alias for go).                 *
 *                                                          *
 ************************************************************
 */
  else if (OptionMatch("go", *args) || OptionMatch("move", *args)) {
    int t;
    char temp[128];

    if (thinking || pondering)
      return 2;
    if (game_wtm) {
      if (strncmp(pgn_white, "Crafty", 6)) {
        strcpy(temp, pgn_white);
        strcpy(pgn_white, pgn_black);
        strcpy(pgn_black, temp);
      }
    } else {
      if (strncmp(pgn_black, "Crafty", 6)) {
        strcpy(temp, pgn_white);
        strcpy(pgn_white, pgn_black);
        strcpy(pgn_black, temp);
      }
    }
    t = tc_time_remaining[white];
    tc_time_remaining[white] = tc_time_remaining[black];
    tc_time_remaining[black] = t;
    t = tc_moves_remaining[white];
    tc_moves_remaining[white] = tc_moves_remaining[black];
    tc_moves_remaining[black] = t;
    force = 0;
    return -1;
  }
/*
 ************************************************************
 *                                                          *
 *  "history" command displays game history (moves).        *
 *                                                          *
 ************************************************************
 */
  else if (OptionMatch("history", *args)) {
    int i;
    char buffer[128];

    if (history_file) {
      printf("    white       black\n");
      for (i = 0; i < (move_number - 1) * 2 - game_wtm + 1; i++) {
        fseek(history_file, i * 10, SEEK_SET);
        v = fscanf(history_file, "%s", buffer);
        if (v <= 0)
          perror("Option() fscanf error: ");
        if (!(i % 2))
          printf("%3d", i / 2 + 1);
        printf("  %-10s", buffer);
        if (i % 2 == 1)
          printf("\n");
      }
      if (Flip(game_wtm))
        printf("  ...\n");
    }
  }
/*
 ************************************************************
 *                                                          *
 *  "hard" command enables thinking on opponent's time.     *
 *                                                          *
 ************************************************************
 */
  else if (OptionMatch("hard", *args)) {
    ponder = 1;
    Print(32, "pondering enabled.\n");
  }
/*
 ************************************************************
 *                                                          *
 *  "hash" command controls the transposition table size.   *
 *  The size can be entered in one of four ways:            *
 *                                                          *
 *     hash=nnn  where nnn is in bytes.                     *
 *     hash=nnnK where nnn is in K bytes.                   *
 *     hash=nnnM where nnn is in M bytes.                   *
 *     hash=nnnG where nnn is in G bytes.                   *
 *                                                          *
 *  the only restriction is that the hash table is computed *
 *  as a perfect power of 2.  Any value that is not a       *
 *  perfect power of 2 is rounded down so that it is, in    *
 *  order to avoid breaking the addressing scheme.          *
 *                                                          *
 ************************************************************
 */
  else if (OptionMatch("hash", *args)) {
    size_t old_hash_size = hash_table_size, new_hash_size;

    if (thinking || pondering)
      return 2;
    if (nargs > 1) {
      allow_memory = 0;
      if (xboard)
        Print(4095, "Warning--  xboard 'memory' option disabled\n");
      new_hash_size = atoiKMB(args[1]);
      if (new_hash_size < 64 * 1024) {
        printf("ERROR.  Minimum hash table size is 64K bytes.\n");
        return 1;
      }
      hash_table_size = ((1ull) << MSB(new_hash_size)) / 16;
      AlignedRemalloc((void *) &hash_table, 64,
          hash_table_size * sizeof(HASH_ENTRY));
      if (!hash_table) {
        printf("AlignedRemalloc() failed, not enough memory.\n");
        exit(1);
      }
      hash_mask = (hash_table_size - 1) & ~3;
    }
    Print(32, "hash table memory = %s bytes",
        DisplayKMB(hash_table_size * sizeof(HASH_ENTRY), 1));
    Print(32, " (%s entries).\n", DisplayKMB(hash_table_size, 1));
    InitializeHashTables(old_hash_size != hash_table_size);
  }
/*
 ************************************************************
 *                                                          *
 *  "phash" command controls the path hash table size. The  *
 *  size can be entered in one of four ways:                *
 *                                                          *
 *     phash=nnn  where nnn is in bytes.                    *
 *     phash=nnnK where nnn is in K bytes.                  *
 *     phash=nnnM where nnn is in M bytes.                  *
 *     phash=nnnG where nnn is in G bytes.                  *
 *                                                          *
 *  the only restriction is that the path hash table must   *
 *  have a perfect power of 2 entries.  The value entered   *
 *  will be rounded down to meet that requirement.          *
 *                                                          *
 ************************************************************
 */
  else if (OptionMatch("phash", *args)) {
    size_t old_hash_size = hash_path_size, new_hash_size;

    if (thinking || pondering)
      return 2;
    if (nargs > 1) {
      new_hash_size = atoiKMB(args[1]);
      if (new_hash_size < 64 * 1024) {
        printf("ERROR.  Minimum phash table size is 64K bytes.\n");
        return 1;
      }
      hash_path_size = ((1ull) << MSB(new_hash_size / sizeof(HPATH_ENTRY)));
      AlignedRemalloc((void *) &hash_path, 64,
          sizeof(HPATH_ENTRY) * hash_path_size);
      if (!hash_path) {
        printf("AlignedRemalloc() failed, not enough memory.\n");
        hash_path_size = 0;
        hash_path = 0;
      }
      hash_path_mask = (hash_path_size - 1) & ~15;
    }
    Print(32, "hash path table memory = %s bytes",
        DisplayKMB(hash_path_size * sizeof(HPATH_ENTRY), 1));
    Print(32, " (%s entries).\n", DisplayKMB(hash_path_size, 1));
    InitializeHashTables(old_hash_size != hash_path_size);
  }
/*
 ************************************************************
 *                                                          *
 *  "hashp" command controls the pawn hash table size.      *
 *                                                          *
 ************************************************************
 */
  else if (OptionMatch("hashp", *args)) {
    size_t old_hash_size = pawn_hash_table_size, new_hash_size;

    if (thinking || pondering)
      return 2;
    if (nargs > 1) {
      allow_memory = 0;
      if (xboard)
        Print(4095, "Warning--  xboard 'memory' option disabled\n");
      new_hash_size = atoiKMB(args[1]);
      if (new_hash_size < 16 * 1024) {
        printf("ERROR.  Minimum pawn hash table size is 16K bytes.\n");
        return 1;
      }
      pawn_hash_table_size =
          1ull << MSB(new_hash_size / sizeof(PAWN_HASH_ENTRY));
      AlignedRemalloc((void *) &pawn_hash_table, 64,
          sizeof(PAWN_HASH_ENTRY) * pawn_hash_table_size);
      if (!pawn_hash_table) {
        printf("AlignedRemalloc() failed, not enough memory.\n");
        exit(1);
      }
      pawn_hash_mask = pawn_hash_table_size - 1;
    }
    Print(32, "pawn hash table memory = %s bytes",
        DisplayKMB(pawn_hash_table_size * sizeof(PAWN_HASH_ENTRY), 1));
    Print(32, " (%s entries).\n", DisplayKMB(pawn_hash_table_size, 1));
    InitializeHashTables(old_hash_size != pawn_hash_table_size);
  }
/*
 ************************************************************
 *                                                          *
 *  "help" command lists commands/options.                  *
 *                                                          *
 ************************************************************
 */
  else if (OptionMatch("help", *args)) {
    FILE *helpfile;
    char *readstat = (char *) -1;
    int lines = 0;

    helpfile = fopen("crafty.hlp", "r");
    if (!helpfile) {
      printf("ERROR.  Unable to open \"crafty.hlp\" -- help unavailable\n");
      return 1;
    }
    if (nargs > 1) {
      while (1) {
        readstat = fgets(buffer, 128, helpfile);
        if (!readstat) {
          printf("Sorry, no help available for \"%s\"\n", args[1]);
          fclose(helpfile);
          return 1;
        }
        if (buffer[0] == '<') {
          if (strstr(buffer, args[1]))
            break;
        }
      }
    }
    while (1) {
      readstat = fgets(buffer, 128, helpfile);
      if (!readstat)
        break;
      if (strchr(buffer, '\n'))
        *strchr(buffer, '\n') = 0;
      if (!strcmp(buffer, "<end>"))
        break;
      printf("%s\n", buffer);
      lines++;
      if (lines > 22) {
        lines = 0;
        printf("<return> for more...");
        fflush(stdout);
        Read(1, buffer);
      }
    }
    fclose(helpfile);
  }
/*
 ************************************************************
 *                                                          *
 *  "hint" displays the expected move based on the last     *
 *  search done. [xboard compatibility]                     *
 *                                                          *
 ************************************************************
 */
  else if (OptionMatch("hint", *args)) {
    if (strlen(ponder_text)) {
      printf("Hint: %s\n", ponder_text);
      fflush(stdout);
    }
  }
/*
 ************************************************************
 *                                                          *
 *  "input" command directs the program to read input from  *
 *  a file until eof is reached or an "exit" command is     *
 *  encountered while reading the file.                     *
 *                                                          *
 ************************************************************
 */
  else if (OptionMatch("input", *args)) {
    if (thinking || pondering)
      return 2;
    nargs = ReadParse(buffer, args, " \t=");
    if (nargs < 2) {
      printf("usage:  input <filename>\n");
      return 1;
    }
    if (!(input_stream = fopen(args[1], "r"))) {
      printf("file does not exist.\n");
      input_stream = stdin;
    }
  }
/*
 ************************************************************
 *                                                          *
 *  "info" command gives some information about Crafty.     *
 *                                                          *
 ************************************************************
 */
  else if (OptionMatch("info", *args)) {
    Print(32, "Crafty version %s\n", version);
    Print(32, "number of threads =         %2d\n", smp_max_threads);
    Print(32, "hash table memory = %s bytes", DisplayKMB(hash_table_size * 64,
            1));
    Print(32, " (%s entries).\n", DisplayKMB(hash_table_size * 5, 0));
    Print(32, "pawn hash table memory = %5s\n",
        DisplayKMB(pawn_hash_table_size * sizeof(PAWN_HASH_ENTRY), 1));
    if (!tc_sudden_death) {
      Print(32, "%d moves/%d minutes %d seconds primary time control\n",
          tc_moves, tc_time / 6000, (tc_time / 100) % 60);
      Print(32, "%d moves/%d minutes %d seconds secondary time control\n",
          tc_secondary_moves, tc_secondary_time / 6000,
          (tc_secondary_time / 100) % 60);
      if (tc_increment)
        Print(32, "increment %d seconds.\n", tc_increment / 100);
    } else if (tc_sudden_death == 1) {
      Print(32, " game/%d minutes primary time control\n", tc_time / 6000);
      if (tc_increment)
        Print(32, "increment %d seconds.\n", (tc_increment / 100) % 60);
    } else if (tc_sudden_death == 2) {
      Print(32, "%d moves/%d minutes primary time control\n", tc_moves,
          tc_time / 6000);
      Print(32, "game/%d minutes secondary time control\n",
          tc_secondary_time / 6000);
      if (tc_increment)
        Print(32, "increment %d seconds.\n", tc_increment / 100);
    }
    Print(32, "book frequency (freq)..............%4.2f\n", book_weight_freq);
    Print(32, "book static evaluation (eval)......%4.2f\n", book_weight_eval);
    Print(32, "book learning (learn)..............%4.2f\n",
        book_weight_learn);
  }
/*
 ************************************************************
 *                                                          *
 *  "kibitz" command sets kibitz mode for ICS.  =1 will     *
 *  kibitz mate announcements, =2 will kibitz scores and    *
 *  other info, =3 will kibitz scores and PV, =4 adds the   *
 *  list of book moves, =5 displays the PV after each       *
 *  iteration completes.                                    *
 *                                                          *
 ************************************************************
 */
  else if (OptionMatch("kibitz", *args)) {
    if (nargs < 2) {
      printf("usage:  kibitz <level>\n");
      return 1;
    }
    kibitz = Min(5, atoi(args[1]));
  }
/*
 ************************************************************
 *                                                          *
 *  "learn" command enables/disables the learning           *
 *  algorithm used in Crafty.  this is controlled by a      *
 *  single variable that is either 0 or 1 (disabled or      *
 *  enabled).                                               *
 *                                                          *
 *  "learn clear" clears all learning data in the opening   *
 *  book, returning it to a state identical to when the     *
 *  book was originally created.                            *
 *                                                          *
 ************************************************************
 */
  else if (OptionMatch("learn", *args)) {
    if (nargs == 2) {
      if (OptionMatch("clear", *(args + 1))) {
        int index[32768], i, j, cluster;
        unsigned char buf32[4];

        fseek(book_file, 0, SEEK_SET);
        for (i = 0; i < 32768; i++) {
          v = fread(buf32, 4, 1, book_file);
          if (v <= 0)
            perror("Option() fread error: ");
          index[i] = BookIn32(buf32);
        }
        for (i = 0; i < 32768; i++)
          if (index[i] > 0) {
            fseek(book_file, index[i], SEEK_SET);
            v = fread(buf32, 4, 1, book_file);
            if (v <= 0)
              perror("Option() fread error: ");
            cluster = BookIn32(buf32);
            if (cluster)
              BookClusterIn(book_file, cluster, book_buffer);
            for (j = 0; j < cluster; j++)
              book_buffer[j].learn = 0.0;
            fseek(book_file, index[i] + sizeof(int), SEEK_SET);
            if (cluster)
              BookClusterOut(book_file, cluster, book_buffer);
          }
      } else {
        learning = atoi(args[1]);
        learn = (learning > 0) ? 1 : 0;
        if (learning)
          Print(32, "book learning enabled {-%d,+%d}\n", learning, learning);
        else
          Print(32, "book learning disabled\n");
      }
    }
  }
/*
 ************************************************************
 *                                                          *
 *  "level" command sets time controls [xboard compati-     *
 *  bility.]                                                *
 *                                                          *
 ************************************************************
 */
  else if (OptionMatch("level", *args)) {
    if (nargs < 4) {
      printf("usage:  level <nmoves> <stime> <inc>\n");
      return 1;
    }
    tc_moves = atoi(args[1]);
    tc_time = atoi(args[2]) * 60;
    if (strchr(args[2], ':'))
      tc_time = tc_time + atoi(strchr(args[2], ':') + 1);
    tc_time *= 100;
    tc_increment = atoi(args[3]) * 100;
    tc_time_remaining[white] = tc_time;
    tc_time_remaining[black] = tc_time;
    if (!tc_moves) {
      tc_sudden_death = 1;
      tc_moves = 1000;
      tc_moves_remaining[white] = 1000;
      tc_moves_remaining[black] = 1000;
    } else
      tc_sudden_death = 0;
    if (tc_moves) {
      tc_secondary_moves = tc_moves;
      tc_secondary_time = tc_time;
      tc_moves_remaining[white] = tc_moves;
      tc_moves_remaining[black] = tc_moves;
    }
    if (!tc_sudden_death) {
      Print(32, "%d moves/%d seconds primary time control\n", tc_moves,
          tc_time / 100);
      Print(32, "%d moves/%d seconds secondary time control\n",
          tc_secondary_moves, tc_secondary_time / 100);
      if (tc_increment)
        Print(32, "increment %d seconds.\n", tc_increment / 100);
    } else if (tc_sudden_death == 1) {
      Print(32, " game/%d seconds primary time control\n", tc_time / 100);
      if (tc_increment)
        Print(32, "increment %d seconds.\n", tc_increment / 100);
    }
    if (adaptive_hash) {
      uint64_t positions_per_move;
      float percent;
      int optimal_hash_size;

      TimeSet(think);
      time_limit /= 100;
      positions_per_move = time_limit * adaptive_hash / 16;
      optimal_hash_size = positions_per_move * 16;
      printf("optimal=%d\n", optimal_hash_size);
      optimal_hash_size = Max(optimal_hash_size, adaptive_hash_min);
      optimal_hash_size = Min(optimal_hash_size, adaptive_hash_max);
      sprintf(buffer, "hash=%d\n", optimal_hash_size);
      Option(tree);
      percent =
          (float) (optimal_hash_size -
          adaptive_hash_min) / (float) (adaptive_hash_max -
          adaptive_hash_min);
      optimal_hash_size =
          adaptive_hashp_min + percent * (adaptive_hashp_max -
          adaptive_hashp_min);
      optimal_hash_size = Max(optimal_hash_size, adaptive_hashp_min);
      sprintf(buffer, "hashp=%d\n", optimal_hash_size);
      Option(tree);
    }
  }
/*
 ************************************************************
 *                                                          *
 *  "linelength" sets the default line length to something  *
 *  other than 80, if desired.  Setting this to a huge      *
 *  number makes a PV print on one line for easier parsing  *
 *  by automated scripts.                                   *
 *                                                          *
 ************************************************************
 */
  else if (OptionMatch("linelength", *args)) {
    if (nargs > 2) {
      printf("usage:  linelength <n>\n");
      return 1;
    }
    if (nargs == 2)
      line_length = atoi(args[1]);
    printf("line length set to %d.\n", line_length);
  }
/*
 ************************************************************
 *                                                          *
 *  "list" command allows the operator to add or remove     *
 *  names from the various lists Crafty uses to recognize   *
 *  and adapt to particular opponents.                      *
 *                                                          *
 *  list <listname> <player>                                *
 *                                                          *
 *  <listname> is one of AK, B, C, GM, IM, SP.              *
 *                                                          *
 *  The final parameter is a name to add  or remove.  If    *
 *  you prepend a + to the name, that asks that the name be *
 *  added to the list.  If you prepend a - to the name,     *
 *  that asks that the name be removed from the list.  If   *
 *  no name is given, the list is displayed.                *
 *                                                          *
 *  AK is the "auto-kibitz" list.  Crafty will kibitz info  *
 *  on a chess server when playing any opponent in this     *
 *  list.  This should only have computer names as humans   *
 *  don't approve of kibitzes while they are playing.       *
 *                                                          *
 *  B identifies "blocker" players, those that try to       *
 *  block the position and go for easy draws.  This makes   *
 *  Crafty try much harder to prevent this from happening,  *
 *  even at the expense of positional compensation.         *
 *                                                          *
 *  GM and IM identify titled players.  This affects how    *
 *  and when Crafty resigns or offers/accepts draws.  For   *
 *  GM players it will do so fairly early after the right   *
 *  circumstances have been seen, for IM it delays a bit    *
 *  longer as they are more prone to making a small error   *
 *  that avoids the loss or draw.                           *
 *                                                          *
 *  SP is the "special player" option.  This is an extended *
 *  version of the "list" command that allows you to        *
 *  specify a special "start book" for a particular         *
 *  opponent to make Crafty play specific openings against  *
 *  that opponent, as well as allowing you to specify a     *
 *  personality file to use against that specific opponent  *
 *  when he is identified by the correct "name" command.    *
 *                                                          *
 *  For the SP list, the command is extended to use         *
 *                                                          *
 *  "list SP +player book=filename  personality=filename"   *
 *                                                          *
 *  For the SP list, the files specified must exist in the  *
 *  current directory unless the bookpath and perspath      *
 *  commands direct Crafty to look elsewhere.               *
 *                                                          *
 ************************************************************
 */
  else if (OptionMatch("list", *args)) {
    char **targs;
    char listname[5][3] = { "AK", "B", "GM", "IM", "SP" };
    char **listaddr[] = { AK_list, B_list, GM_list,
      IM_list, SP_list
    };
    int i, list, lastent = -1;

    targs = args;
    for (list = 0; list < 5; list++) {
      if (!strcmp(listname[list], args[1]))
        break;
    }
    if (list > 4) {
      printf("usage:  list AK|B|GM|IM|P|SP +name1 -name2 etc\n");
      return 1;
    }
    nargs -= 2;
    targs += 2;
    if (nargs) {
      while (nargs) {
        if (targs[0][0] == '-') {
          for (i = 0; i < 128; i++)
            if (listaddr[list][i]) {
              if (!strcmp(listaddr[list][i], targs[0] + 1)) {
                free(listaddr[list][i]);
                listaddr[list][i] = NULL;
                Print(32, "%s removed from %s list.\n", targs[0] + 1,
                    listname[list]);
                break;
              }
            }
        } else if (targs[0][0] == '+') {
          for (i = 0; i < 128; i++)
            if (listaddr[list][i]) {
              if (!strcmp(listaddr[list][i], targs[0] + 1)) {
                Print(32, "Warning: %s is already in %s list.\n",
                    targs[0] + 1, listname[list]);
                break;
              }
            }
          for (i = 0; i < 128; i++)
            if (listaddr[list][i] == NULL)
              break;
          if (i >= 128)
            Print(32, "ERROR!  %s list is full at 128 entries\n",
                listname[list]);
          else {
            listaddr[list][i] = malloc(strlen(targs[0]));
            strcpy(listaddr[list][i], targs[0] + 1);
            Print(32, "%s added to %s list.\n", targs[0] + 1, listname[list]);
            if (list == 5)
              lastent = i;
          }
        } else if (!strcmp(targs[0], "clear")) {
          for (i = 0; i < 128; i++) {
            free(listaddr[list][i]);
            listaddr[list][i] = NULL;
          }
        } else if (!strcmp(targs[0], "book") && lastent != -1) {
          char filename[256];
          FILE *file;

          strcpy(filename, book_path);
          strcat(filename, "/");
          strcat(filename, targs[1]);
          if (!strstr(args[2], ".bin"))
            strcat(filename, ".bin");
          file = fopen(filename, "r");
          if (!file) {
            Print(4095, "ERROR  book file %s can not be opened\n", filename);
            break;
          }
          fclose(file);
          SP_opening_filename[lastent] = malloc(strlen(filename) + 1);
          strcpy(SP_opening_filename[lastent], filename);
          nargs--;
          targs++;
        } else if (!strcmp(targs[0], "personality") && lastent != -1) {
          char filename[256];
          FILE *file;

          strcat(filename, targs[1]);
          if (!strstr(args[2], ".cpf"))
            strcat(filename, ".cpf");
          file = fopen(filename, "r");
          if (!file) {
            Print(4095, "ERROR  personality file %s can not be opened\n",
                filename);
            break;
          }
          fclose(file);
          SP_personality_filename[lastent] = malloc(strlen(filename) + 1);
          strcpy(SP_personality_filename[lastent], filename);
          nargs--;
          targs++;
        } else
          printf("error, name must be preceeded by +/- flag.\n");
        nargs--;
        targs++;
      }
    } else {
      Print(32, "%s List:\n", listname[list]);
      for (i = 0; i < 128; i++) {
        if (listaddr[list][i]) {
          Print(32, "%s", listaddr[list][i]);
          if (list == 5) {
            if (SP_opening_filename[i])
              Print(32, "  book=%s", SP_opening_filename[i]);
            if (SP_personality_filename[i])
              Print(32, "  personality=%s", SP_personality_filename[i]);
          }
          Print(32, "\n");
        }
      }
    }
  }
/*
 ************************************************************
 *                                                          *
 *  "lmp" command sets the formla parameters that produce   *
 *  LMP pruning bounds array.                               *
 *                                                          *
 *     lmp <maxdepth> <base> <scale>                        *
 *                                                          *
 *  <maxdepth> is the max depth at which LMP is done.       *
 *                                                          *
 *  <base> is the base pruning move count.  The function is *
 *  an exponential of the form x = base + f(y).  The        *
 *  default is currently 3.                                 *
 *                                                          *
 *  <scale> is the exponent of the exponential function.    *
 *  larger numbers produce more conservative (larger) move  *
 *  counts.  Smaller values are more aggressive.  The       *
 *  default is currently 1.9.                               *
 *                                                          *
 ************************************************************
 */
  else if (OptionMatch("lmp", *args)) {
    int i;

    if ((nargs > 1 && nargs < 4) || nargs > 4) {
      printf("usage:  lmp <maxdepth> <base> <scale>\n");
      return 1;
    }
    if (nargs > 1) {
      LMP_depth = atoi(args[1]);
      LMP_base = atoi(args[2]);
      LMP_scale = atof(args[3]);
      InitializeLMP();
    }
    Print(32, "LMP depth=%d  base=%d  scale=%f\n", LMP_depth, LMP_base,
        LMP_scale);
    Print(32, "depth:  ");
    for (i = 1; i < 16; i++)
      Print(32, "%4d", i);
    Print(32, "\n");
    Print(32, "movcnt: ");
    for (i = 1; i < 16; i++)
      Print(32, "%4d", LMP[i]);
    Print(32, "\n");
  }
/*
 ************************************************************
 *                                                          *
 *  "lmr" command sets the formla parameters that produce   *
 *  LMR reduction matrix.  The format is:                   *
 *                                                          *
 *     lmr <min> <max> <depth bias> <moves bias> <scale>    *
 *                                                          *
 *  <min> is the minimum LMR reduction.  This probably      *
 *  should not be changed from 1, the default.              *
 *                                                          *
 *  <max> is the maximum LMR reduction.  If you adjust the  *
 *  following values, you might need to increase this as it *
 *  is an absolute clamp and no value can exceed this no    *
 *  matter what the formula produces.                       *
 *                                                          *
 *  <depth_bias> is simply a multiplier that causes depth   *
 *  to influence the reduction amount more or less (as the  *
 *  value drops below the value used for <moves bias> below *
 *  or as it is increased above <moves bias>.  The default  *
 *  is 2.0.                                                 *
 *                                                          *
 *  <moves bias> is simply a multiplier that causes the     *
 *  number of moves already searched to become more or less *
 *  important than the remaining depth as above.  The       *
 *  default is 1.0.                                         *
 *                                                          *
 *  <scale> is used to scale the formula back since it uses *
 *  a logarithmic expression.  The basic idea is to adjust  *
 *  the above two values to produce the desired "shape" of  *
 *  the reduction matrix, then adjust this to change the    *
 *  reduction amounts overall.  The default is 2.9.         *
 *                                                          *
 ************************************************************
 */
  else if (OptionMatch("lmr", *args)) {
    int i, j;

    if ((nargs > 1 && nargs < 6) || nargs > 7) {
      printf("usage:  lmr <min> <max> <depth bias> <move bias> <scale>\n");
      return 1;
    }
    if (nargs > 1) {
      LMR_min = atoi(args[1]);
      LMR_max = Min(atoi(args[2]), 15);
      LMR_db = atof(args[3]);
      LMR_mb = atof(args[4]);
      LMR_s = atof(args[5]);
      InitializeLMR();
    }
    if (nargs > 6) {
      char *axis = "|||||||||||depth left|||||||||||";

      Print(32,
          "LMR values:  %d(min) %d(max) %.2f(depth) %.2f(moves) %.2f(scale).\n",
          LMR_min, LMR_max, LMR_db, LMR_mb, LMR_s);
      Print(32, "\n                 LMR reductions[depth][moves]\n");
      Print(32, "  ----------------------moves searched-----------------\n");
      Print(32, " |      ");
      for (i = 0; i < 64; i += 1)
        Print(32, "%3d", i);
      Print(32, "\n");
      for (i = 0; i < 32; i += 1) {
        Print(32, " %c %3d: ", axis[i], i);
        for (j = 0; j < 64; j += 1)
          Print(32, " %2d", LMR[i][j]);
        Print(32, "\n");
      }
    } else {
      char *axis = "||depth left|||";

      Print(32,
          "LMR values:  %d(min) %d(max) %.2f(depth) %.2f(moves) %.2f(scale).\n",
          LMR_min, LMR_max, LMR_db, LMR_mb, LMR_s);
      Print(32, "\n                 LMR reductions[depth][moves]\n");
      Print(32, "  ----------------------moves searched------------------\n");
      Print(32, " |      ");
      for (i = 2; i < 64; i += 4)
        Print(32, "%3d", i);
      Print(32, "\n");
      for (i = 3; i < 32; i += 2) {
        Print(32, " %c %3d: ", axis[(i - 3) / 2], i);
        for (j = 2; j < 64; j += 4)
          Print(32, " %2d", LMR[i][j]);
        Print(32, "\n");
      }
      Print(32, "    note:  table is shown compressed, each index is in\n");
      Print(32, "    units of 1, all rows/columns are not shown above\n");
    }
  }
/*
 ************************************************************
 *                                                          *
 *   "load" command directs the program to read input from  *
 *   a file until a "setboard" command is found  this       *
 *   command is then executed, setting up the position for  *
 *   a search.                                              *
 *                                                          *
 ************************************************************
 */
  else if (OptionMatch("load", *args)) {
    char title[64];
    char *readstat;
    FILE *prob_file;

    if (thinking || pondering)
      return 2;
    nargs = ReadParse(buffer, args, " \t=");
    if (nargs < 3) {
      printf("usage:  input <filename> title\n");
      return 1;
    }
    if (!(prob_file = fopen(args[1], "r"))) {
      printf("file does not exist.\n");
      return 1;
    }
    strcpy(title, args[2]);
    while (!feof(prob_file)) {
      readstat = fgets(buffer, 128, prob_file);
      if (readstat) {
        char *delim;

        delim = strchr(buffer, '\n');
        if (delim)
          *delim = 0;
        delim = strchr(buffer, '\r');
        if (delim)
          *delim = ' ';
      }
      if (readstat == NULL)
        break;
      nargs = ReadParse(buffer, args, " \t;\n");
      if (!strcmp(args[0], "title") && strstr(buffer, title))
        break;
    }
    while (!feof(prob_file)) {
      readstat = fgets(buffer, 128, prob_file);
      if (readstat) {
        char *delim;

        delim = strchr(buffer, '\n');
        if (delim)
          *delim = 0;
        delim = strchr(buffer, '\r');
        if (delim)
          *delim = ' ';
      }
      if (readstat == NULL)
        break;
      nargs = ReadParse(buffer, args, " \t;\n");
      if (!strcmp(args[0], "setboard")) {
        Option(tree);
        break;
      }
    }
    fclose(prob_file);
  }
/*
 ************************************************************
 *                                                          *
 *   "log" command turns log on/off, and also lets you view *
 *   the end of the log or copy it to disk as needed.  To   *
 *   view the end, simply type "log <n>" where n is the #   *
 *   of lines you'd like to see (the last <n> lines).  You  *
 *   can add a filename to the end and the output will go   *
 *   to this file instead.                                  *
 *                                                          *
 ************************************************************
 */
  else if (OptionMatch("log", *args)) {
    char filename[64];

    if (nargs < 2) {
      printf("usage:  log on|off|n [filename]\n");
      return 1;
    }
    if (!strcmp(args[1], "on")) {
      int id;

      id = InitializeGetLogID();
      sprintf(log_filename, "%s/log.%03d", log_path, id);
      sprintf(history_filename, "%s/game.%03d", log_path, id);
      log_file = fopen(log_filename, "w");
      history_file = fopen(history_filename, "w+");
    } else if (!strcmp(args[1], "off")) {
      if (log_file)
        fclose(log_file);
      log_file = 0;
      sprintf(filename, "%s/log.%03d", log_path, log_id - 1);
      remove(filename);
      sprintf(filename, "%s/game.%03d", log_path, log_id - 1);
      remove(filename);
    } else if (args[1][0] >= '0' && args[1][0] <= '9')
      log_id = atoi(args[1]);
  }
/*
 ************************************************************
 *                                                          *
 *   "memory" command is used to set the max memory to use  *
 *   for hash and hashp combined.  This is an xboard        *
 *   compatibility command, not normally used by players.   *
 *                                                          *
 ************************************************************
 */
  else if (OptionMatch("memory", *args)) {
    uint64_t size;
    size_t hmemory, pmemory;
    if (nargs < 2) {
      printf("usage:  memory <size>\n");
      return 1;
    }
    if (allow_memory) {
      size = (uint64_t) atoi(args[1]);
      if (size == 0) {
        Print(4095, "ERROR - memory size can not be zero\n");
        return 1;
      }
      hmemory = (1ull) << MSB(size);
      size &= ~hmemory;
      pmemory = (1ull) << MSB(size);
      sprintf(buffer, "hash %" PRIu64 "M\n", (uint64_t) hmemory);
      Option(tree);
      if (pmemory) {
        sprintf(buffer, "hashp %" PRIu64 "M\n", (uint64_t) pmemory);
        Option(tree);
      }
    } else
      Print(4095, "WARNING - memory command ignored.\n");
  }
/*
 ************************************************************
 *                                                          *
 *   "mode" command sets tournament mode or normal mode.    *
 *   Tournament mode is used when Crafty is in a "real"     *
 *   tournament.  It forces draw_score to 0, and makes      *
 *   Crafty display the chess clock after each move.        *
 *                                                          *
 ************************************************************
 */
  else if (OptionMatch("mode", *args)) {
    if (nargs > 1) {
      if (!strcmp(args[1], "tournament")) {
        mode = tournament_mode;
        printf("use 'settc' command if a game is restarted after Crafty\n");
        printf("has been terminated for any reason.\n");
      } else if (!strcmp(args[1], "normal")) {
        mode = normal_mode;
        book_weight_learn = 1.0;
        book_weight_freq = 1.0;
        book_weight_eval = 0.5;
      } else if (!strcmp(args[1], "match")) {
        mode = normal_mode;
        book_weight_learn = 1.0;
        book_weight_freq = 0.2;
        book_weight_eval = 0.1;
      } else {
        printf("usage:  mode normal|tournament|match\n");
        mode = normal_mode;
        book_weight_learn = 1.0;
        book_weight_freq = 1.0;
        book_weight_eval = 0.5;
      }
    }
    if (mode == tournament_mode)
      printf("tournament mode.\n");
    else if (mode == normal_mode)
      printf("normal mode.\n");
  }
/*
 ************************************************************
 *                                                          *
 *   "name" command saves opponents name and writes it into *
 *   logfile along with the date/time.  It also scans the   *
 *   list of known computers and adjusts its opening book   *
 *   to play less "risky" if it matches.  If the opponent   *
 *   is in the GM list, it tunes the resignation controls   *
 *   to resign earlier.  Ditto for other lists that are     *
 *   used to recognize specific opponents and adjust things *
 *   accordingly.                                           *
 *                                                          *
 ************************************************************
 */
  else if (OptionMatch("name", *args)) {
    char *next;
    int i;

    if (nargs < 2) {
      printf("usage:  name <name>\n");
      return 1;
    }
    if (game_wtm) {
      strcpy(pgn_white, args[1]);
      sprintf(pgn_black, "Crafty %s", version);
    } else {
      strcpy(pgn_black, args[1]);
      sprintf(pgn_white, "Crafty %s", version);
    }
    Print(32, "Crafty %s vs %s\n", version, args[1]);
    next = args[1];
    while (*next) {
      *next = tolower(*next);
      next++;
    }
    if (mode != tournament_mode) {
      for (i = 0; i < 128; i++)
        if (AK_list[i] && !strcmp(AK_list[i], args[1])) {
          kibitz = 4;
          break;
        }
      for (i = 0; i < 128; i++)
        if (GM_list[i] && !strcmp(GM_list[i], args[1])) {
          Print(32, "playing a GM!\n");
          book_selection_width = 3;
          resign = Min(6, resign);
          resign_count = 4;
          draw_count = 4;
          accept_draws = 1;
          kibitz = 0;
          break;
        }
      for (i = 0; i < 128; i++)
        if (IM_list[i] && !strcmp(IM_list[i], args[1])) {
          Print(32, "playing an IM!\n");
          book_selection_width = 4;
          resign = Min(9, resign);
          resign_count = 5;
          draw_count = 4;
          accept_draws = 1;
          kibitz = 0;
          break;
        }
      for (i = 0; i < 128; i++)
        if (SP_list[i] && !strcmp(SP_list[i], args[1])) {
          FILE *normal_bs_file = books_file;

          Print(32, "playing a special player!\n");
          if (SP_opening_filename[i]) {
            books_file = fopen(SP_opening_filename[i], "rb");
            if (!books_file) {
              Print(4095, "Error!  unable to open %s for player %s.\n",
                  SP_opening_filename[i], SP_list[i]);
              books_file = normal_bs_file;
            }
          }
          if (SP_personality_filename[i]) {
            sprintf(buffer, "personality load %s\n",
                SP_personality_filename[i]);
            Option(tree);
          }
          break;
        }
    }
    printf("tellicsnoalias kibitz Hello from Crafty v%s! (%d cpus)\n",
        version, Max(1, smp_max_threads));
  }
/*
 ************************************************************
 *                                                          *
 *  "new" command initializes for a new game.               *
 *                                                          *
 ************************************************************
 */
  else if (OptionMatch("new", *args)) {
    Print(4095, "NOTICE:  ""new"" command not implemented, please exit and\n");
    Print(4095, "restart crafty to re-initialize everything for a new game\n");
    return 1;
  }
/*
 ************************************************************
 *                                                          *
 *  "noise" command sets a minimum limit on time searched   *
 *  before we start to display the normal search output.    *
 *  With today's hardware and deep searches, it is easy to  *
 *  get "swamped" with output.  Using "noise" you can say   *
 *  "hold the output until you have searched for <x> time   *
 *  (where time can be x.xx seconds or just x seconds.)     *
 *                                                          *
 ************************************************************
 */
  else if (OptionMatch("noise", *args)) {
    if (nargs < 2) {
      printf("usage:  noise <n>\n");
      return 1;
    }
    noise_level = atof(args[1]) * 100;
    Print(32, "noise level set to %.2f seconds.\n",
        (float) noise_level / 100.0);
  }
/*
 ************************************************************
 *                                                          *
 *  "null" command sets the minimum null-move reduction and *
 *  a value that is used to compute the max reduction.      *
 *                                                          *
 *     null <min> <divisor>                                 *
 *                                                          *
 *  <min> is the minimum null move reduction.  The default  *
 *  is 3 which is pretty reliable.                          *
 *                                                          *
 *  <divisor> increases the null move by the following      *
 *  simple formula:                                         *
 *                                                          *
 *    null_reduction = min + depth / divisor                *
 *                                                          *
 *  The default value is currently 10, which will increase  *
 *  R (null-move reduction) by one at any position where    *
 *  depth >= 10 and < 20.  Or by two when depth > 20.  Etc. *
 *                                                          *
 ************************************************************
 */
  else if (OptionMatch("null", *args)) {

    if (nargs > 3) {
      printf("usage:  null <min> <divisor>\n");
      return 1;
    }
    if (nargs > 1) {
      null_depth = atoi(args[1]);
      null_divisor = atoi(args[2]);
    }
    Print(32, "null move:  R = %d + depth / %d\n", null_depth, null_divisor);
  }
/*
 ************************************************************
 *                                                          *
 *  "otim" command sets the opponent's time remaining.      *
 *  This is used to determine if the opponent is in time    *
 *  trouble, and is factored into the draw score if he is.  *
 *                                                          *
 ************************************************************
 */
  else if (OptionMatch("otim", *args)) {
    if (nargs < 2) {
      printf("usage:  otime <time(unit=.01 secs))>\n");
      return 1;
    }
    tc_time_remaining[game_wtm] = atoi(args[1]);
    if (log_file && time_limit > 99)
      fprintf(log_file, "time remaining: %s (opponent).\n",
          DisplayTime(tc_time_remaining[game_wtm]));
    if (call_flag && xboard && tc_time_remaining[game_wtm] < 1) {
      if (crafty_is_white)
        Print(32, "1-0 {Black ran out of time}\n");
      else
        Print(32, "0-1 {White ran out of time}\n");
    }
  }
/*
 ************************************************************
 *                                                          *
 *  "output" command sets long or short algebraic output.   *
 *  Long is Ng1f3, while short is simply Nf3.               *
 *                                                          *
 ************************************************************
 */
  else if (OptionMatch("output", *args)) {
    if (nargs < 2) {
      printf("usage:  output long|short\n");
      return 1;
    }
    if (!strcmp(args[1], "long"))
      output_format = 1;
    else if (!strcmp(args[1], "short"))
      output_format = 0;
    else
      printf("usage:  output long|short\n");
    if (output_format == 1)
      Print(32, "output moves in long algebraic format\n");
    else if (output_format == 0)
      Print(32, "output moves in short algebraic format\n");
  }
/*
 ************************************************************
 *                                                          *
 *  "personality" command is used to adjust the eval terms  *
 *  and search options to modify the way Crafty plays.      *
 *                                                          *
 ************************************************************
 */
  else if (OptionMatch("personality", *args)) {
    int i, j, param, index, value;

/*
 ************************************************************
 *                                                          *
 *  Handle the "personality list" command and dump every-   *
 *  thing the user can modify.                              *
 *                                                          *
 ************************************************************
 */
    if (nargs == 2 && !strcmp(args[1], "list")) {
      printf("\n");
      for (i = 0; i < 256; i++) {
        if (!personality_packet[i].description)
          continue;
        if (personality_packet[i].value) {
          switch (personality_packet[i].type) {
            case 1:
              printf("%3d  %s %7d\n", i, personality_packet[i].description,
                  *(int *) personality_packet[i].value);
              break;
            case 2:
              printf("%3d  %s %7d (mg) %7d (eg)\n", i,
                  personality_packet[i].description,
                  ((int *) personality_packet[i].value)[mg],
                  ((int *) personality_packet[i].value)[eg]);
              break;
            case 3:
              printf("%3d  %s %7.2f\n", i, personality_packet[i].description,
                  *(double *) personality_packet[i].value);
              break;
            case 4:
              printf("%3d  %s    ", i, personality_packet[i].description);
              for (j = 0; j < personality_packet[i].size; j++)
                printf("%4d", ((int *) personality_packet[i].value)[j]);
              printf("\n");
              break;
          }
        } else {
          printf("==================================================\n");
          printf("=         %s  =\n", personality_packet[i].description);
          printf("==================================================\n");
        }
      }
      printf("\n");
      return 1;
    }
/*
 ************************************************************
 *                                                          *
 *  Handle the "personality load" command and read in the   *
 *  specified *.cpf file.                                   *
 *                                                          *
 ************************************************************
 */
    if (!strcmp(args[1], "load")) {
      FILE *file;
      char filename[256];

      strcpy(filename, args[2]);
      if (!strstr(filename, ".cpf"))
        strcat(filename, ".cpf");
      Print(32, "Loading personality file %s\n", filename);
      if ((file = fopen(filename, "r+"))) {
        while (fgets(buffer, 4096, file)) {
          char *delim;

          delim = strchr(buffer, '\n');
          if (delim)
            *delim = 0;
          delim = strstr(buffer, "->");
          if (delim)
            *delim = 0;
          delim = strchr(buffer, '\r');
          if (delim)
            *delim = ' ';
          Option(tree);
        }
        fclose(file);
      }
      return 1;
    }
/*
 ************************************************************
 *                                                          *
 *  Handle the "personality save" command and dump every-   *
 *  thing that can be modified to a file.                   *
 *                                                          *
 ************************************************************
 */
    if (nargs == 3 && !strcmp(args[1], "save")) {
      char filename[256];
      FILE *file;

      strcpy(filename, args[2]);
      if (!strstr(filename, ".cpf"))
        strcat(filename, ".cpf");
      file = fopen(filename, "w");
      if (!file) {
        printf("ERROR.  Unable to open %s for writing\n", args[2]);
        return 1;
      }
      printf("saving to file \"%s\"\n", filename);
      fprintf(file, "# Crafty v%s personality file\n", version);
      for (i = 0; i < 256; i++) {
        if (!personality_packet[i].description)
          continue;
        if (personality_packet[i].value) {
          if (personality_packet[i].size <= 1)
            fprintf(file, "personality %3d %7d\n", i,
                *((int *) personality_packet[i].value));
          else if (personality_packet[i].size > 1) {
            fprintf(file, "personality %3d ", i);
            for (j = 0; j < personality_packet[i].size; j++)
              fprintf(file, "%d ", ((int *) personality_packet[i].value)[j]);
            fprintf(file, "\n");
          }
        }
      }
      fprintf(file, "exit\n");
      fclose(file);
      return 1;
    }
/*
 ************************************************************
 *                                                          *
 *  Handle the "personality index val" command that changes *
 *  only those personality terms that are scalars.          *
 *                                                          *
 ************************************************************
 */
    param = atoi(args[1]);
    value = atoi(args[2]);
    if (!personality_packet[param].value) {
      Print(4095, "ERROR.  evaluation term %d is not defined\n", param);
      return 1;
    }
    if (personality_packet[param].size == 0) {
      if (nargs > 3) {
        printf("this eval term requires exactly 1 value.\n");
        return 1;
      }
      *(int *) personality_packet[param].value = value;
    }
/*
 ************************************************************
 *                                                          *
 *  Handle the "personality index v1 v2 .. vn" command that *
 *  changes eval terms that are vectors.                    *
 *                                                          *
 ************************************************************
 */
    else {
      index = nargs - 2;
      if (index != personality_packet[param].size) {
        printf
            ("this eval term (%s [%d]) requires exactly %d values, found %d.\n",
            personality_packet[param].description, param,
            Abs(personality_packet[param].size), index);
        return 1;
      }
      for (i = 0; i < index; i++)
        ((int *) personality_packet[param].value)[i] = atoi(args[i + 2]);
    }
    InitializeKingSafety();
  }
/*
 ************************************************************
 *                                                          *
 *  "bookpath", "logpath" and "tbpath" set the default      *
 *  paths to locate or save these files.                    *
 *                                                          *
 ************************************************************
 */
  else if (OptionMatch("logpath", *args) || OptionMatch("bookpath", *args)
      || OptionMatch("tbpath", *args)) {
    if (OptionMatch("logpath", *args) || OptionMatch("bookpath", *args)) {
      if (log_file)
        Print(4095, "ERROR -- this must be used on command line only\n");
    }
    nargs = ReadParse(buffer, args, " \t=");
    if (nargs < 2) {
      printf("usage:  bookpath|perspath|logpath|tbpath <path>\n");
      return 1;
    }
    if (!strchr(args[1], '(')) {
      if (strstr(args[0], "bookpath"))
        strcpy(book_path, args[1]);
      else if (strstr(args[0], "logpath"))
        strcpy(log_path, args[1]);
#if defined(SYZYGY)
      else if (strstr(args[0], "tbpath"))
        strcpy(tb_path, args[1]);
#endif

    } else {
      if (strchr(args[1], ')')) {
        *strchr(args[1], ')') = 0;
        if (strstr(args[0], "bookpath"))
          strcpy(book_path, args[1] + 1);
        else if (strstr(args[0], "logpath"))
          strcpy(log_path, args[1] + 1);
#if defined(SYZYGY)
        else if (strstr(args[0], "tbpath"))
          strcpy(tb_path, args[1] + 1);
#endif
      } else
        Print(4095, "ERROR multiple paths must be enclosed in ( and )\n");
    }
  }
/*
 ************************************************************
 *                                                          *
 *  "perf" command turns times move generator/make_move.    *
 *                                                          *
 ************************************************************
 */
#define PERF_CYCLES 4000000
  else if (OptionMatch("perf", *args)) {
    int i, clock_before, clock_after;
    unsigned *mv;
    float time_used;

    if (thinking || pondering)
      return 2;
    clock_before = clock();
    while (clock() == clock_before);
    clock_before = clock();
    for (i = 0; i < PERF_CYCLES; i++) {
      tree->last[1] = GenerateCaptures(tree, 0, game_wtm, tree->last[0]);
      tree->last[1] = GenerateNoncaptures(tree, 0, game_wtm, tree->last[1]);
    }
    clock_after = clock();
    time_used =
        ((float) clock_after - (float) clock_before) / (float) CLOCKS_PER_SEC;
    printf("generated %d moves, time=%.2f seconds\n",
        (int) (tree->last[1] - tree->last[0]) * PERF_CYCLES, time_used);
    printf("generated %d moves per second\n",
        (int) (((float) (PERF_CYCLES * (tree->last[1] -
                        tree->last[0]))) / time_used));
    clock_before = clock();
    while (clock() == clock_before);
    clock_before = clock();
    for (i = 0; i < PERF_CYCLES; i++) {
      tree->last[1] = GenerateCaptures(tree, 0, game_wtm, tree->last[0]);
      tree->last[1] = GenerateNoncaptures(tree, 0, game_wtm, tree->last[1]);
      for (mv = tree->last[0]; mv < tree->last[1]; mv++) {
        MakeMove(tree, 0, game_wtm, *mv);
        UnmakeMove(tree, 0, game_wtm, *mv);
      }
    }
    clock_after = clock();
    time_used =
        ((float) clock_after - (float) clock_before) / (float) CLOCKS_PER_SEC;
    printf("generated/made/unmade %d moves, time=%.2f seconds\n",
        (int) (tree->last[1] - tree->last[0]) * PERF_CYCLES, time_used);
    printf("generated/made/unmade %d moves per second\n",
        (int) (((float) (PERF_CYCLES * (tree->last[1] -
                        tree->last[0]))) / time_used));
  }
/*
 ************************************************************
 *                                                          *
 *  "perft" command turns tests move generator/make_move.   *
 *                                                          *
 ************************************************************
 */
  else if (OptionMatch("perft", *args)) {
    float time_used;
    int i, clock_before, clock_after;

    if (thinking || pondering)
      return 2;
    clock_before = clock();
    while (clock() == clock_before);
    clock_before = clock();
    if (nargs < 2) {
      printf("usage:  perft <depth>\n");
      return 1;
    }
    tree->status[1] = tree->status[0];
    tree->last[0] = tree->move_list;
    i = atoi(args[1]);
    if (i <= 0) {
      Print(32, "usage:  perft <maxply>\n");
      return 1;
    }
    total_moves = 0;
    OptionPerft(tree, 1, i, game_wtm);
    clock_after = clock();
    time_used =
        ((float) clock_after - (float) clock_before) / (float) CLOCKS_PER_SEC;
    printf("total moves=%" PRIu64 "  time=%.2f\n", total_moves, time_used);
  }
/*
 ************************************************************
 *                                                          *
 *  "pgn" command sets the various PGN header files.        *
 *                                                          *
 ************************************************************
 */
  else if (OptionMatch("pgn", *args)) {
    int i;

    if (nargs < 3) {
      printf("usage:  pgn <tag> <value>\n");
      return 1;
    }
    if (!strcmp(args[1], "Event")) {
      pgn_event[0] = 0;
      for (i = 2; i < nargs; i++) {
        strcpy(pgn_event + strlen(pgn_event), args[i]);
        strcpy(pgn_event + strlen(pgn_event), " ");
      }
    } else if (!strcmp(args[1], "Site")) {
      pgn_site[0] = 0;
      for (i = 2; i < nargs; i++) {
        strcpy(pgn_site + strlen(pgn_site), args[i]);
        strcpy(pgn_site + strlen(pgn_site), " ");
      }
    } else if (!strcmp(args[1], "Round")) {
      pgn_round[0] = 0;
      strcpy(pgn_round, args[2]);
    } else if (!strcmp(args[1], "White")) {
      pgn_white[0] = 0;
      for (i = 2; i < nargs; i++) {
        strcpy(pgn_white + strlen(pgn_white), args[i]);
        strcpy(pgn_white + strlen(pgn_white), " ");
      }
    } else if (!strcmp(args[1], "WhiteElo")) {
      pgn_white_elo[0] = 0;
      strcpy(pgn_white_elo, args[2]);
    } else if (!strcmp(args[1], "Black")) {
      pgn_black[0] = 0;
      for (i = 2; i < nargs; i++) {
        strcpy(pgn_black + strlen(pgn_black), args[i]);
        strcpy(pgn_black + strlen(pgn_black), " ");
      }
    } else if (!strcmp(args[1], "BlackElo")) {
      pgn_black_elo[0] = 0;
      strcpy(pgn_black_elo, args[2]);
    }
  }
/*
 ************************************************************
 *                                                          *
 *  "ping" command simply echos the argument back to xboard *
 *  to let it know all previous commands have been executed *
 *  and we are ready for whatever is next.                  *
 *                                                          *
 ************************************************************
 */
  else if (OptionMatch("ping", *args)) {
    if (pondering)
      Print(-1, "pong %s\n", args[1]);
    else
      pong = atoi(args[1]);
  }
/*
 ************************************************************
 *                                                          *
 *  "playother" command says "position is set up, we are    *
 *  waiting on the opponent to move, ponder if you want to  *
 *  do so.                                                  *
 *                                                          *
 ************************************************************
 */
  else if (OptionMatch("playother", *args)) {
    force = 0;
  }
/*
 ************************************************************
 *                                                          *
 *  "ponder" command toggles pondering off/on or sets a     *
 *  move to ponder.                                         *
 *                                                          *
 ************************************************************
 */
  else if (OptionMatch("ponder", *args)) {
    if (thinking || pondering)
      return 2;
    if (nargs < 2) {
      printf("usage:  ponder off|on|<move>\n");
      return 1;
    }
    if (!strcmp(args[1], "on")) {
      ponder = 1;
      Print(32, "pondering enabled.\n");
    } else if (!strcmp(args[1], "off")) {
      ponder = 0;
      Print(32, "pondering disabled.\n");
    } else {
      ponder_move = InputMove(tree, 0, game_wtm, 0, 0, args[1]);
      last_pv.pathd = 0;
      last_pv.pathl = 0;
    }
  }
/*
 ************************************************************
 *                                                          *
 *  "post/nopost" command sets/resets "show thinking" mode  *
 *  for xboard compatibility.                               *
 *                                                          *
 ************************************************************
 */
  else if (OptionMatch("post", *args)) {
    post = 1;
  } else if (OptionMatch("nopost", *args)) {
    post = 0;
  }
/*
 ************************************************************
 *                                                          *
 *  "protover" command is sent by xboard to identify the    *
 *  xboard protocol version and discover what the engine    *
 *  can handle.                                             *
 *                                                          *
 ************************************************************
 */
  else if (OptionMatch("protover", *args)) {
    int pversion = atoi(args[1]);

    if (pversion >= 1 && pversion <= 3) {
      if (pversion >= 2) {
        Print(-1, "feature ping=1 setboard=1 san=1 time=1 draw=1\n");
        Print(-1, "feature sigint=0 sigterm=0 reuse=0 analyze=1\n");
        Print(-1, "feature myname=\"Crafty-%s\" name=1\n", version);
        Print(-1, "feature playother=1 colors=0 memory=%d\n", allow_memory);
#if (CPUS > 1)
        Print(-1, "feature smp=%d\n", allow_cores);
#endif
        Print(-1, "feature variants=\"normal,nocastle\"\n");
        Print(-1, "feature done=1\n");
        xboard_done = 1;
      }
    } else
      Print(4095, "ERROR, bogus xboard protocol version received.\n");
  }
/*
 ************************************************************
 *                                                          *
 *  "random" command is ignored. [xboard compatibility]     *
 *                                                          *
 ************************************************************
 */
  else if (OptionMatch("random", *args)) {
    return xboard;
  }
/*
 ************************************************************
 *                                                          *
 *  "rating" is used by xboard to set Crafty's rating and   *
 *  the opponent's rating, which is used by the learning    *
 *  functions.                                              *
 *                                                          *
 ************************************************************
 */
  else if (OptionMatch("rating", *args)) {
    int rd;

    if (nargs < 3) {
      printf("usage:  rating <Crafty> <opponent>\n");
      return 1;
    }
    crafty_rating = atoi(args[1]);
    opponent_rating = atoi(args[2]);
    if (crafty_rating == 0 && opponent_rating == 0) {
      crafty_rating = 2500;
      opponent_rating = 2300;
    }
    if (dynamic_draw_score) {
      rd = opponent_rating - crafty_rating;
      rd = Max(Min(rd, 300), -300);
      abs_draw_score = rd / 8;
      if (log_file) {
        fprintf(log_file, "Crafty's rating: %d.\n", crafty_rating);
        fprintf(log_file, "opponent's rating: %d.\n", opponent_rating);
        fprintf(log_file, "draw score: %d.\n", abs_draw_score);
      }
    }
  }
/*
 ************************************************************
 *                                                          *
 *  "remove" command backs up the game one whole move,      *
 *  leaving the opponent still on move.  It's intended for  *
 *  xboard compatibility, but works in any mode.            *
 *                                                          *
 ************************************************************
 */
  else if (OptionMatch("remove", *args)) {
    if (thinking || pondering)
      return 2;
    move_number--;
    sprintf(buffer, "reset %d", move_number);
    Option(tree);
  }
/*
 ************************************************************
 *                                                          *
 *  "read" reads game moves in and makes them.  This can    *
 *  be used in two ways:  (1) type "read" and then start    *
 *  entering moves;  type "exit" when done;  (2) type       *
 *  "read <filename>" to read moves in from <filename>.     *
 *  Note that read will attempt to skip over "non-move"     *
 *  text and try to extract moves if it can.                *
 *                                                          *
 *  Note that "reada" appends to the existing position,     *
 *  while "read" resets the board to the start position     *
 *  before reading moves.                                   *
 *                                                          *
 ************************************************************
 */
  else if (OptionMatch("read", *args) || OptionMatch("reada", *args)) {
    FILE *read_input = 0;
    int append, move, readstat;

    if (thinking || pondering)
      return 2;
    nargs = ReadParse(buffer, args, " \t=");
    if (!strcmp("reada", *args))
      append = 1;
    else
      append = 0;
    ponder_move = 0;
    last_pv.pathd = 0;
    last_pv.pathl = 0;
    if (nargs > 1) {
      if (!(read_input = fopen(args[1], "r"))) {
        printf("file %s does not exist.\n", args[1]);
        return 1;
      }
    } else {
      printf("type \"exit\" to terminate.\n");
      read_input = stdin;
    }
    if (!append) {
      InitializeChessBoard(tree);
      game_wtm = 1;
      move_number = 1;
      tc_moves_remaining[white] = tc_moves;
      tc_moves_remaining[black] = tc_moves;
    }
/*
 step 1:  read in the PGN tags.
 */
    readstat = ReadPGN(0, 0);
    do {
      if (read_input == stdin) {
        if (game_wtm)
          printf("read.White(%d): ", move_number);
        else
          printf("read.Black(%d): ", move_number);
        fflush(stdout);
      }
      readstat = ReadPGN(read_input, 0);
    } while (readstat == 1);
    if (readstat < 0)
      return 1;
/*
 step 2:  read in the moves.
 */
    do {
      move = 0;
      move = ReadNextMove(tree, buffer, 0, game_wtm);
      if (move) {
        if (read_input != stdin) {
          printf("%s ", OutputMove(tree, 0, game_wtm, move));
          if (!(move_number % 8) && Flip(game_wtm))
            printf("\n");
        }
        fseek(history_file, ((move_number - 1) * 2 + 1 - game_wtm) * 10,
            SEEK_SET);
        fprintf(history_file, "%9s\n", OutputMove(tree, 0, game_wtm, move));
        MakeMoveRoot(tree, game_wtm, move);
        TimeAdjust(game_wtm, 0);
#if defined(DEBUG)
        ValidatePosition(tree, 1, move, "Option()");
#endif
      } else if (!read_input)
        printf("illegal move.\n");
      if (move) {
        game_wtm = Flip(game_wtm);
        if (game_wtm)
          move_number++;
      }
      if (read_input == stdin) {
        if (game_wtm)
          printf("read.White(%d): ", move_number);
        else
          printf("read.Black(%d): ", move_number);
        fflush(stdout);
      }
      readstat = ReadPGN(read_input, 0);
      if (readstat < 0)
        break;
      if (!strcmp(buffer, "exit"))
        break;
    } while (1);
    moves_out_of_book = 0;
    printf("NOTICE: %d moves to next time control\n",
        tc_moves_remaining[root_wtm]);
    root_wtm = !game_wtm;
    if (read_input != stdin) {
      printf("\n");
      fclose(read_input);
    }
  }
/*
 ************************************************************
 *                                                          *
 *  "rejected" handles the new xboard protocol version 2    *
 *  rejected command.                                       *
 *                                                          *
 ************************************************************
 */
  else if (OptionMatch("rejected", *args)) {
    Print(4095, "ERROR.  feature %s rejected by xboard\n", args[1]);
  }
/*
 ************************************************************
 *                                                          *
 *  "reset" restores (backs up) a game to a prior position  *
 *  with the same side on move.  Reset 17 would reset the   *
 *  position to what it was at move 17 with the current     *
 *  still on move (you can use white/black commands to      *
 *  change the side to move first, if needed.)              *
 *                                                          *
 ************************************************************
 */
  else if (OptionMatch("reset", *args)) {
    int i, move, nmoves;

    if (!history_file)
      return 1;
    if (thinking || pondering)
      return 2;
    if (nargs < 2) {
      printf("usage:  reset <movenumber>\n");
      return 1;
    }
    ponder_move = 0;
    last_mate_score = 0;
    last_pv.pathd = 0;
    last_pv.pathl = 0;
    if (thinking || pondering)
      return 2;
    over = 0;
    move_number = atoi(args[1]);
    if (!move_number) {
      move_number = 1;
      return 1;
    }
    nmoves = (move_number - 1) * 2 + 1 - game_wtm;
    root_wtm = Flip(game_wtm);
    InitializeChessBoard(tree);
    game_wtm = 1;
    move_number = 1;
    tc_moves_remaining[white] = tc_moves;
    tc_moves_remaining[black] = tc_moves;
    for (i = 0; i < nmoves; i++) {
      fseek(history_file, i * 10, SEEK_SET);
      v = fscanf(history_file, "%s", buffer);
      if (v <= 0)
        perror("Option() fscanf error: ");
/*
 If the move is "pass", that means that the side on move passed.
 This includes the case where the game started from a black-to-move
 position; then white's first move is recorded as a pass.
 */
      if (strcmp(buffer, "pass") == 0) {
        game_wtm = Flip(game_wtm);
        if (game_wtm)
          move_number++;
        continue;
      }
      move = InputMove(tree, 0, game_wtm, 0, 0, buffer);
      if (move)
        MakeMoveRoot(tree, game_wtm, move);
      else {
        printf("ERROR!  move %s is illegal\n", buffer);
        break;
      }
      TimeAdjust(game_wtm, 0);
      game_wtm = Flip(game_wtm);
      if (game_wtm)
        move_number++;
    }
    moves_out_of_book = 0;
    printf("NOTICE: %d moves to next time control\n",
        tc_moves_remaining[root_wtm]);
  }
/*
 ************************************************************
 *                                                          *
 *  "resign" command sets the resignation threshold to the  *
 *  number of pawns the program must be behind before       *
 *  resigning (0 -> disable resignations).  Resign with no  *
 *  arguments will mark the pgn result as lost by the       *
 *  opponent.                                               *
 *                                                          *
 ************************************************************
 */
  else if (OptionMatch("resign", *args)) {
    if (nargs < 2) {
      if (crafty_is_white) {
        Print(4095, "result 1-0\n");
        strcpy(pgn_result, "1-0");
      } else {
        Print(4095, "result 0-1\n");
        strcpy(pgn_result, "0-1");
      }
      learn_value = 300;
      LearnBook();
      return 1;
    }
    resign = atoi(args[1]);
    if (nargs == 3)
      resign_count = atoi(args[2]);
    if (resign)
      Print(32, "resign after %d consecutive moves with score < %d.\n",
          resign_count, -resign);
    else
      Print(32, "disabled resignations.\n");
  }
/*
 ************************************************************
 *                                                          *
 *  "result" command comes from xboard/winboard and gives   *
 *  the result of the current game.  If learning routines   *
 *  have not yet been activated, this will do it.           *
 *                                                          *
 ************************************************************
 */
  else if (OptionMatch("result", *args)) {
    if (nargs > 1) {
      if (!strcmp(args[1], "1-0")) {
        strcpy(pgn_result, "1-0");
        if (crafty_is_white)
          learn_value = 300;
        else
          learn_value = -300;
      } else if (!strcmp(args[1], "0-1")) {
        strcpy(pgn_result, "0-1");
        if (crafty_is_white)
          learn_value = -300;
        else
          learn_value = 300;
      } else if (!strcmp(args[1], "1/2-1/2")) {
        strcpy(pgn_result, "1/2-1/2");
        learn_value = 1;
      }
      LearnBook();
      return 1;
    }
  }
/*
 ************************************************************
 *                                                          *
 *  "safety" command sets a specific time safety margin     *
 *  target for normal timed games.  This can generally be   *
 *  left at the default value unless Crafty is being        *
 *  manually operated.                                      *
 *                                                          *
 ************************************************************
 */
  else if (OptionMatch("safety", *args)) {
    if (nargs == 2)
      tc_safety_margin = atoi(args[1]) * 100;
    Print(32, "safety margin set to %s.\n", DisplayTime(tc_safety_margin));
  }
/*
 ************************************************************
 *                                                          *
 *  "savegame" command saves the game in a file in PGN      *
 *  format.  Command has an optional filename.              *
 *                                                          *
 ************************************************************
 */
  else if (OptionMatch("savegame", *args)) {
    struct tm *timestruct;
    FILE *output_file;
    time_t secs;
    int i, more, swtm;
    char input[128], text[128], *next;

    output_file = stdout;
    secs = time(0);
    timestruct = localtime((time_t *) & secs);
    if (nargs > 1) {
      if (!(output_file = fopen(args[1], "w"))) {
        printf("unable to open %s for write.\n", args[1]);
        return 1;
      }
    }
    fprintf(output_file, "[Event \"%s\"]\n", pgn_event);
    fprintf(output_file, "[Site \"%s\"]\n", pgn_site);
    fprintf(output_file, "[Date \"%4d.%02d.%02d\"]\n",
        timestruct->tm_year + 1900, timestruct->tm_mon + 1,
        timestruct->tm_mday);
    fprintf(output_file, "[Round \"%s\"]\n", pgn_round);
    fprintf(output_file, "[White \"%s\"]\n", pgn_white);
    fprintf(output_file, "[WhiteElo \"%s\"]\n", pgn_white_elo);
    fprintf(output_file, "[Black \"%s\"]\n", pgn_black);
    fprintf(output_file, "[BlackElo \"%s\"]\n", pgn_black_elo);
    fprintf(output_file, "[Result \"%s\"]\n", pgn_result);
/* Handle setup positions and initial pass by white */
    swtm = 1;
    if (move_number > 1 || !game_wtm) {
      fseek(history_file, 0, SEEK_SET);
      if (fscanf(history_file, "%s", input) == 1 &&
          strcmp(input, "pass") == 0)
        swtm = 0;
    }
    if (initial_position[0])
      fprintf(output_file, "[FEN \"%s\"]\n[SetUp \"1\"]\n", initial_position);
    else if (!swtm) {
      fprintf(output_file,
          "[FEN \"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR b KQkq - 0 1\"\n"
          "[SetUp \"1\"]\n");
    }
    fprintf(output_file, "\n");
    next = text;
    if (!swtm) {
      strcpy(next, "1... ");
      next = text + strlen(text);
    }
/* Output the moves */
    more = 0;
    for (i = (swtm ? 0 : 1); i < (move_number - 1) * 2 - game_wtm + 1; i++) {
      fseek(history_file, i * 10, SEEK_SET);
      v = fscanf(history_file, "%s", input);
      if (v <= 0)
        perror("Option() fscanf error: ");
      if (!(i % 2)) {
        sprintf(next, "%d. ", i / 2 + 1);
        next = text + strlen(text);
      }
      sprintf(next, "%s ", input);
      next = text + strlen(text);
      more = 1;
      if (next - text >= 60) {
        fprintf(output_file, "%s\n", text);
        more = 0;
        next = text;
      }
    }
    if (more)
      fprintf(output_file, "%s", text);
    fprintf(output_file, "%s\n", pgn_result);
    if (output_file != stdout)
      fclose(output_file);
    printf("PGN save complete.\n");
  }
/*
 ************************************************************
 *                                                          *
 *  "savepos" command saves the current position in a FEN   *
 *  (Forsythe-Edwards Notation) string that can be later    *
 *  used to recreate this exact position.                   *
 *                                                          *
 ************************************************************
 */
  else if (OptionMatch("savepos", *args)) {
    FILE *output_file;
    int rank, file, nempty;

    output_file = stdout;
    if (nargs > 1) {
      if (!strcmp(args[1], "*")) {
        output_file = 0;
        strcpy(initial_position, "");
      } else if (!(output_file = fopen(args[1], "w"))) {
        printf("unable to open %s for write.\n", args[1]);
        return 1;
      }
    }
    if (output_file)
      fprintf(output_file, "setboard ");
    for (rank = RANK8; rank >= RANK1; rank--) {
      nempty = 0;
      for (file = FILEA; file <= FILEH; file++) {
        if (PcOnSq((rank << 3) + file)) {
          if (nempty) {
            if (output_file)
              fprintf(output_file, "%c", empty_sqs[nempty]);
            else
              sprintf(initial_position + strlen(initial_position), "%c",
                  empty_sqs[nempty]);
            nempty = 0;
          }
          if (output_file)
            fprintf(output_file, "%c",
                translate[PcOnSq((rank << 3) + file) + 6]);
          else
            sprintf(initial_position + strlen(initial_position), "%c",
                translate[PcOnSq((rank << 3) + file) + 6]);
        } else
          nempty++;
      }
      if (empty_sqs[nempty]) {
        if (output_file)
          fprintf(output_file, "%c", empty_sqs[nempty]);
        else
          sprintf(initial_position + strlen(initial_position), "%c",
              empty_sqs[nempty]);
      }
      if (rank != RANK1) {
        if (output_file)
          fprintf(output_file, "/");
        else
          sprintf(initial_position + strlen(initial_position), "/");
      }
    }
    if (output_file)
      fprintf(output_file, " %c ", (game_wtm) ? 'w' : 'b');
    else
      sprintf(initial_position + strlen(initial_position), " %c ",
          (game_wtm) ? 'w' : 'b');
    if (Castle(0, white) & 1) {
      if (output_file)
        fprintf(output_file, "K");
      else
        sprintf(initial_position + strlen(initial_position), "K");
    }
    if (Castle(0, white) & 2) {
      if (output_file)
        fprintf(output_file, "Q");
      else
        sprintf(initial_position + strlen(initial_position), "Q");
    }
    if (Castle(0, black) & 1) {
      if (output_file)
        fprintf(output_file, "k");
      else
        sprintf(initial_position + strlen(initial_position), "k");
    }
    if (Castle(0, black) & 2) {
      if (output_file)
        fprintf(output_file, "q");
      else
        sprintf(initial_position + strlen(initial_position), "q");
    }
    if (!Castle(0, white) && !Castle(0, black)) {
      if (output_file)
        fprintf(output_file, " -");
      else
        sprintf(initial_position + strlen(initial_position), " -");
    }
    if (EnPassant(0)) {
      if (output_file)
        fprintf(output_file, " %c%c", File(EnPassant(0)) + 'a',
            Rank(EnPassant(0)) + '1');
      else
        sprintf(initial_position + strlen(initial_position), " %c%c",
            File(EnPassant(0)) + 'a', Rank(EnPassant(0)) + '1');
    } else {
      if (output_file)
        fprintf(output_file, " -");
      else
        sprintf(initial_position + strlen(initial_position), " -");
    }
    if (output_file)
      fprintf(output_file, "\n");
    if (output_file && output_file != stdout) {
      fprintf(output_file, "exit\n");
      fclose(output_file);
    }
    if (output_file)
      printf("FEN save complete.\n");
  }
/*
 ************************************************************
 *                                                          *
 *  "scale" command is used for tuning.  We modify this to  *
 *  scale some scoring value(s) by a percentage that can    *
 *  be either positive or negative.                         *
 *                                                          *
 ************************************************************
 */
  else if (!strcmp("scale", *args)) {
    scale = atoi(args[1]);
  }
/*
 ************************************************************
 *                                                          *
 *  "score" command displays static evaluation of the       *
 *  current board position.                                 *
 *                                                          *
 ************************************************************
 */
  else if (OptionMatch("score", *args)) {
    int phase, s, tw, tb, mgb, mgw, egb, egw, trop[2];

    if (thinking || pondering)
      return 2;
    memset((void *) &(tree->pawn_score), 0, sizeof(tree->pawn_score));
    Print(32, "note: scores are for the white side\n");
    Print(32, "                       ");
    Print(32, " +-----------white----------+");
    Print(32, "-----------black----------+\n");
    tree->score_mg = 0;
    tree->score_eg = 0;
    mgb = tree->score_mg;
    EvaluateMaterial(tree, game_wtm);
    mgb = tree->score_mg - mgb;
    Print(32, "material.......%s", DisplayEvaluation(mgb, 1));
    Print(32, "  |    comp     mg      eg   |");
    Print(32, "    comp     mg      eg   |\n");
    root_wtm = Flip(game_wtm);
    tree->status[1] = tree->status[0];
    s = Evaluate(tree, 1, game_wtm, -99999, 99999);
    trop[black] = tree->tropism[black];
    trop[white] = tree->tropism[white];
    if (!game_wtm)
      s = -s;
    tree->score_mg = 0;
    tree->score_eg = 0;
    phase =
        Min(62, TotalPieces(white, occupied) + TotalPieces(black, occupied));
    tree->pawn_score.score_mg = 0;
    tree->pawn_score.score_eg = 0;
    mgb = tree->pawn_score.score_mg;
    egb = tree->pawn_score.score_eg;
    EvaluatePawns(tree, black);
    mgb = tree->pawn_score.score_mg - mgb;
    egb = tree->pawn_score.score_eg - egb;
    mgw = tree->pawn_score.score_mg;
    egw = tree->pawn_score.score_eg;
    EvaluatePawns(tree, white);
    mgw = tree->pawn_score.score_mg - mgw;
    egw = tree->pawn_score.score_eg - egw;
    tb = (mgb * phase + egb * (62 - phase)) / 62;
    tw = (mgw * phase + egw * (62 - phase)) / 62;
    Print(32, "pawns..........%s  |", DisplayEvaluation(tb + tw, 1));
    Print(32, " %s", DisplayEvaluation(tw, 1));
    Print(32, " %s", DisplayEvaluation(mgw, 1));
    Print(32, " %s  |", DisplayEvaluation(egw, 1));
    Print(32, " %s", DisplayEvaluation(tb, 1));
    Print(32, " %s", DisplayEvaluation(mgb, 1));
    Print(32, " %s  |\n", DisplayEvaluation(egb, 1));
    mgb = tree->score_mg;
    egb = tree->score_eg;
    EvaluatePassedPawns(tree, black, game_wtm);
    mgb = tree->score_mg - mgb;
    egb = tree->score_eg - egb;
    mgw = tree->score_mg;
    egw = tree->score_eg;
    EvaluatePassedPawns(tree, white, game_wtm);
    mgw = tree->score_mg - mgw;
    egw = tree->score_eg - egw;
    tb = (mgb * phase + egb * (62 - phase)) / 62;
    tw = (mgw * phase + egw * (62 - phase)) / 62;
    Print(32, "passed pawns...%s  |", DisplayEvaluation(tb + tw, 1));
    Print(32, " %s", DisplayEvaluation(tw, 1));
    Print(32, " %s", DisplayEvaluation(mgw, 1));
    Print(32, " %s  |", DisplayEvaluation(egw, 1));
    Print(32, " %s", DisplayEvaluation(tb, 1));
    Print(32, " %s", DisplayEvaluation(mgb, 1));
    Print(32, " %s  |\n", DisplayEvaluation(egb, 1));
    mgb = tree->score_mg;
    egb = tree->score_eg;
    EvaluateKnights(tree, black);
    mgb = tree->score_mg - mgb;
    egb = tree->score_eg - egb;
    mgw = tree->score_mg;
    egw = tree->score_eg;
    EvaluateKnights(tree, white);
    mgw = tree->score_mg - mgw;
    egw = tree->score_eg - egw;
    tb = (mgb * phase + egb * (62 - phase)) / 62;
    tw = (mgw * phase + egw * (62 - phase)) / 62;
    Print(32, "knights........%s  |", DisplayEvaluation(tb + tw, 1));
    Print(32, " %s", DisplayEvaluation(tw, 1));
    Print(32, " %s", DisplayEvaluation(mgw, 1));
    Print(32, " %s  |", DisplayEvaluation(egw, 1));
    Print(32, " %s", DisplayEvaluation(tb, 1));
    Print(32, " %s", DisplayEvaluation(mgb, 1));
    Print(32, " %s  |\n", DisplayEvaluation(egb, 1));
    mgb = tree->score_mg;
    egb = tree->score_eg;
    EvaluateBishops(tree, black);
    mgb = tree->score_mg - mgb;
    egb = tree->score_eg - egb;
    mgw = tree->score_mg;
    egw = tree->score_eg;
    EvaluateBishops(tree, white);
    mgw = tree->score_mg - mgw;
    egw = tree->score_eg - egw;
    tb = (mgb * phase + egb * (62 - phase)) / 62;
    tw = (mgw * phase + egw * (62 - phase)) / 62;
    Print(32, "bishops........%s  |", DisplayEvaluation(tb + tw, 1));
    Print(32, " %s", DisplayEvaluation(tw, 1));
    Print(32, " %s", DisplayEvaluation(mgw, 1));
    Print(32, " %s  |", DisplayEvaluation(egw, 1));
    Print(32, " %s", DisplayEvaluation(tb, 1));
    Print(32, " %s", DisplayEvaluation(mgb, 1));
    Print(32, " %s  |\n", DisplayEvaluation(egb, 1));
    mgb = tree->score_mg;
    egb = tree->score_eg;
    EvaluateRooks(tree, black);
    mgb = tree->score_mg - mgb;
    egb = tree->score_eg - egb;
    mgw = tree->score_mg;
    egw = tree->score_eg;
    EvaluateRooks(tree, white);
    mgw = tree->score_mg - mgw;
    egw = tree->score_eg - egw;
    tb = (mgb * phase + egb * (62 - phase)) / 62;
    tw = (mgw * phase + egw * (62 - phase)) / 62;
    Print(32, "rooks..........%s  |", DisplayEvaluation(tb + tw, 1));
    Print(32, " %s", DisplayEvaluation(tw, 1));
    Print(32, " %s", DisplayEvaluation(mgw, 1));
    Print(32, " %s  |", DisplayEvaluation(egw, 1));
    Print(32, " %s", DisplayEvaluation(tb, 1));
    Print(32, " %s", DisplayEvaluation(mgb, 1));
    Print(32, " %s  |\n", DisplayEvaluation(egb, 1));
    mgb = tree->score_mg;
    egb = tree->score_eg;
    EvaluateQueens(tree, black);
    mgb = tree->score_mg - mgb;
    egb = tree->score_eg - egb;
    mgw = tree->score_mg;
    egw = tree->score_eg;
    EvaluateQueens(tree, white);
    mgw = tree->score_mg - mgw;
    egw = tree->score_eg - egw;
    tb = (mgb * phase + egb * (62 - phase)) / 62;
    tw = (mgw * phase + egw * (62 - phase)) / 62;
    Print(32, "queens.........%s  |", DisplayEvaluation(tb + tw, 1));
    Print(32, " %s", DisplayEvaluation(tw, 1));
    Print(32, " %s", DisplayEvaluation(mgw, 1));
    Print(32, " %s  |", DisplayEvaluation(egw, 1));
    Print(32, " %s", DisplayEvaluation(tb, 1));
    Print(32, " %s", DisplayEvaluation(mgb, 1));
    Print(32, " %s  |\n", DisplayEvaluation(egb, 1));
    tree->tropism[black] = trop[black];
    tree->tropism[white] = trop[white];
    mgb = tree->score_mg;
    egb = tree->score_eg;
    EvaluateKing(tree, 1, black);
    mgb = tree->score_mg - mgb;
    egb = tree->score_eg - egb;
    mgw = tree->score_mg;
    egw = tree->score_eg;
    EvaluateKing(tree, 1, white);
    mgw = tree->score_mg - mgw;
    egw = tree->score_eg - egw;
    tb = (mgb * phase + egb * (62 - phase)) / 62;
    tw = (mgw * phase + egw * (62 - phase)) / 62;
    Print(32, "kings..........%s  |", DisplayEvaluation(tb + tw, 1));
    Print(32, " %s", DisplayEvaluation(tw, 1));
    Print(32, " %s", DisplayEvaluation(mgw, 1));
    Print(32, " %s  |", DisplayEvaluation(egw, 1));
    Print(32, " %s", DisplayEvaluation(tb, 1));
    Print(32, " %s", DisplayEvaluation(mgb, 1));
    Print(32, " %s  |\n", DisplayEvaluation(egb, 1));
    egb = tree->score_eg;
    if ((TotalPieces(white, occupied) == 0 && tree->pawn_score.passed[black])
        || (TotalPieces(black, occupied) == 0 &&
            tree->pawn_score.passed[white]))
      EvaluatePassedPawnRaces(tree, game_wtm);
    egb = tree->score_eg - egb;
    Print(32, "pawn races.....%s", DisplayEvaluation(egb, 1));
    Print(32, "  +--------------------------+--------------------------+\n");
    Print(32, "total..........%s\n", DisplayEvaluation(s, 1));
  }
/*
 ************************************************************
 *                                                          *
 *  "screen" command runs runs through a test suite of      *
 *  positions and culls any where a search returns a value  *
 *  outside the margin given to the screen command.         *
 *                                                          *
 ************************************************************
 */
  else if (OptionMatch("screen", *args)) {
    int margin = 9999999, save_noise, save_display;

    nargs = ReadParse(buffer, args, " \t;=");
    if (thinking || pondering)
      return 2;
    if (nargs < 3) {
      printf("usage:  screen <filename> score-margin\n");
      return 1;
    }
    save_noise = noise_level;
    save_display = display_options;
    early_exit = 99;
    margin = atoi(args[2]);
    noise_level = 99999999;
    display_options = 2048;
    Test(args[1], 0, 1, margin);
    noise_level = save_noise;
    display_options = save_display;
    ponder_move = 0;
    last_pv.pathd = 0;
    last_pv.pathl = 0;
  }
/*
 ************************************************************
 *                                                          *
 *  "sd" command sets a specific search depth to control    *
 *  the tree search depth.                                  *
 *                                                          *
 ************************************************************
 */
  else if (OptionMatch("sd", *args)) {
    if (nargs < 2) {
      printf("usage:  sd <depth>\n");
      return 1;
    }
    search_depth = atoi(args[1]);
    Print(32, "search depth set to %d.\n", search_depth);
  }
/*
 ************************************************************
 *                                                          *
 *  "search" command sets a specific move for the search    *
 *  to analyze, ignoring all others completely.             *
 *                                                          *
 ************************************************************
 */
  else if (OptionMatch("search", *args)) {
    if (thinking || pondering)
      return 2;
    if (nargs < 2) {
      printf("usage:  search <move>\n");
      return 1;
    }
    search_move = InputMove(tree, 0, game_wtm, 0, 0, args[1]);
    if (!search_move)
      search_move = InputMove(tree, 0, Flip(game_wtm), 0, 0, args[1]);
    if (!search_move)
      printf("illegal move.\n");
  }
/*
 ************************************************************
 *                                                          *
 *  "setboard" command sets the board to a specific         *
 *  position for analysis by the program.                   *
 *                                                          *
 ************************************************************
 */
  else if (OptionMatch("setboard", *args)) {
    if (thinking || pondering)
      return 2;
    nargs = ReadParse(buffer, args, " \t;=");
    if (nargs < 3) {
      printf("usage:  setboard <fen>\n");
      return 1;
    }
    SetBoard(tree, nargs - 1, args + 1, 0);
    move_number = 1;
    if (!game_wtm) {
      game_wtm = 1;
      Pass();
    }
    ponder_move = 0;
    last_pv.pathd = 0;
    last_pv.pathl = 0;
    over = 0;
    strcpy(buffer, "savepos *");
    Option(tree);
  } else if (StrCnt(*args, '/') > 3) {
    if (thinking || pondering)
      return 2;
    nargs = ReadParse(buffer, args, " \t;=");
    SetBoard(tree, nargs, args, 0);
    move_number = 1;
    if (!game_wtm) {
      game_wtm = 1;
      Pass();
    }
    ponder_move = 0;
    last_pv.pathd = 0;
    last_pv.pathl = 0;
    over = 0;
    strcpy(buffer, "savepos *");
    Option(tree);
  }
/*
 ************************************************************
 *                                                          *
 *  "settc" command is used to reset the time controls      *
 *  after a complete restart.                               *
 *                                                          *
 ************************************************************
 */
  else if (OptionMatch("settc", *args)) {
    if (thinking || pondering)
      return 2;
    if (nargs < 4) {
      printf("usage:  settc <wmoves> <wtime> <bmoves> <btime>\n");
      return 1;
    }
    tc_moves_remaining[white] = atoi(args[1]);
    tc_time_remaining[white] = ParseTime(args[2]) * 6000;
    tc_moves_remaining[black] = atoi(args[3]);
    tc_time_remaining[black] = ParseTime(args[4]) * 6000;
    Print(32, "time remaining: %s (white).\n",
        DisplayTime(tc_time_remaining[white]));
    Print(32, "time remaining: %s (black).\n",
        DisplayTime(tc_time_remaining[black]));
    if (tc_sudden_death != 1) {
      Print(32, "%d moves to next time control (white)\n",
          tc_moves_remaining[white]);
      Print(32, "%d moves to next time control (black)\n",
          tc_moves_remaining[black]);
    } else
      Print(32, "Sudden-death time control in effect\n");
    TimeSet(999);
  }
/*
 ************************************************************
 *                                                          *
 *  "show" command enables/disables whether or not we want  *
 *  show book information as the game is played.            *
 *                                                          *
 ************************************************************
 */
  else if (OptionMatch("show", *args)) {
    if (nargs < 2) {
      printf("usage:  show book\n");
      return 1;
    }
    if (OptionMatch("book", args[1])) {
      show_book = !show_book;
      if (show_book)
        Print(32, "show book statistics\n");
      else
        Print(32, "don't show book statistics\n");
    }
  }
/*
 ************************************************************
 *                                                          *
 *  "skill" command sets a value from 1-100 that affects    *
 *  Crafty's playing skill level.  100 => max skill, 1 =>   *
 *  minimal skill.  This is used to slow the search speed   *
 *  (and depth) significantly.                              *
 *                                                          *
 ************************************************************
 */
#if defined(SKILL)
  else if (OptionMatch("skill", *args)) {
    if (nargs < 2) {
      printf("usage:  skill <1-100>\n");
      return 1;
    }
    if (skill != 100)
      printf("ERROR:  skill can only be changed one time in a game\n");
    else {
      skill = atoi(args[1]);
      if (skill < 1 || skill > 100) {
        printf("ERROR: skill range is 1-100 only\n");
        skill = 100;
      }
      Print(32, "skill level set to %d%%\n", skill);
    }
  }
#endif
/*
 ************************************************************
 *                                                          *
 *   "smp" command is used to tune the various SMP search   *
 *   parameters.                                            *
 *                                                          *
 *   "smpaffinity" command is used to enable (>= 0) and to  *
 *   disable smp processor affinity (off).  If you try to   *
 *   run two instances of Crafty on the same machine, ONE   *
 *   them (if not both) need to have processor affinity     *
 *   disabled or else you can use the smpaffinity=<n> to    *
 *   prevent processor conflicts.  If you use a 32 core     *
 *   machine, and you want to run two instances of Crafty,  *
 *   use smpaffinity=0 on one, and smpaffinity=16 on the    *
 *   other.  The first will bind to processors 0-15, and    *
 *   the second will bind to processors 16-31.              *
 *                                                          *
 *   "smpgroup" command is used to control how many threads *
 *   may work together at any point in the tree.  The       *
 *   usual default is 6, but this might be reduced on a     *
 *   machine with a large number of processors.  It should  *
 *   be tested, of course.                                  *
 *                                                          *
 *   "smpmin" sets the minimum depth the search can split   *
 *   at to keep it from splitting too near the leaves.      *
 *                                                          *
 *   "smpmt" command is used to set the maximum number of   *
 *   parallel threads to use, assuming that Crafty was      *
 *   compiled with -DSMP.  This value can not be set        *
 *   larger than the compiled-in -DCPUS=n value.            *
 *                                                          *
 *   "smpnice" command turns on "nice" mode where idle      *
 *   processors are terminated between searches to avoid    *
 *   burning CPU time in the idle loop.                     *
 *                                                          *
 *   "smpnuma" command enables NUMA mode which distributes  *
 *   hash tables across all NUMA nodes evenly.  If your     *
 *   machine is not NUMA, or only has one socket (node) you *
 *   should set this to zero as it will be slightly more    *
 *   efficient when you change hash sizes.                  *
 *                                                          *
 *   "smproot" command is used to enable (1) or disable (0) *
 *   splitting the tree at the root (ply=1).  Splitting at  *
 *   the root is more efficient, but might slow finding the *
 *   move in some test positions.                           *
 *                                                          *
 *   "smpgsd" sets the minimum depth remaining at which a   *
 *   gratuitous split can be done.                          *
 *                                                          *
 *   "smpgsl" sets the maximum number of gratuitous splits  *
 *   per thread.  This only counts splits that have not yet *
 *   been joined.                                           *
 *                                                          *
 ************************************************************
 */
  else if (OptionMatch("smpaffinity", *args)) {
    if (nargs < 2) {
      printf("usage:  smpaffinity <0/1>\n");
      return 1;
    }
    if (!strcmp(args[1], "off"))
      smp_affinity = -1;
    else
      smp_affinity = atoi(args[1]);
    if (smp_affinity >= 0)
      Print(32, "smp processor affinity enabled.\n");
    else
      Print(32, "smp processor affinity disabled.\n");
  } else if (OptionMatch("smpmin", *args)) {
    if (nargs < 2) {
      printf("usage:  smpmin <depth>\n");
      return 1;
    }
    smp_min_split_depth = atoi(args[1]);
    Print(32, "minimum thread depth set to %d.\n", smp_min_split_depth);
  } else if (OptionMatch("smpgroup", *args)) {
    if (nargs < 2) {
      printf("usage:  smpgroup <threads>\n");
      return 1;
    }
    smp_split_group = atoi(args[1]);
    Print(32, "maximum thread group size set to %d.\n", smp_split_group);
  } else if (OptionMatch("smpmt", *args) || OptionMatch("mt", *args)
      || OptionMatch("cores", *args)) {
    int proc;

    if (nargs < 2) {
      printf("usage:  smpmt=<threads>\n");
      return 1;
    }
    if (thinking || pondering)
      return 3;
    allow_cores = 0;
    if (xboard)
      Print(4095, "Warning--  xboard 'cores' option disabled\n");
    smp_max_threads = atoi(args[1]);
    if (smp_max_threads > hardware_processors) {
      Print(4095, "ERROR - machine has %d processors.\n",
          hardware_processors);
      Print(4095, "ERROR - max threads can not exceed this limit.\n");
      smp_max_threads = hardware_processors;
    }
    if (smp_max_threads > CPUS) {
      Print(4095, "ERROR - Crafty was compiled with CPUS=%d.", CPUS);
      Print(4095, "  mt can not exceed this value.\n");
      smp_max_threads = CPUS;
    }
    if (smp_max_threads == 1) {
      Print(4095, "ERROR - max threads can be set to zero (0) to");
      Print(4095, " disable parallel search, otherwise it must be > 1.\n");
      smp_max_threads = 0;
    }
    if (smp_max_threads)
      Print(32, "max threads set to %d.\n", smp_max_threads);
    else
      Print(32, "parallel threads disabled.\n");
    for (proc = 1; proc < CPUS; proc++)
      if (proc >= smp_max_threads)
        thread[proc].terminate = 1;
  } else if (OptionMatch("smpnice", *args)) {
    if (nargs < 2) {
      printf("usage:  smpnice 0|1\n");
      return 1;
    }
    smp_nice = atoi(args[1]);
    if (smp_nice)
      Print(32, "SMP terminate extra threads when idle.\n");
    else
      Print(32, "SMP keep extra threads spinning when idle.\n");
  } else if (OptionMatch("smpnuma", *args)) {
    if (nargs < 2) {
      printf("usage:  smpnuma 0|1\n");
      return 1;
    }
    smp_numa = atoi(args[1]);
    if (smp_numa)
      Print(32, "SMP NUMA mode enabled.\n");
    else
      Print(32, "SMP NUMA mode disabled.\n");
  } else if (OptionMatch("smproot", *args)) {
    if (nargs < 2) {
      printf("usage:  smproot 0|1\n");
      return 1;
    }
    smp_split_at_root = atoi(args[1]);
    if (smp_split_at_root)
      Print(32, "SMP search split at ply >= 1.\n");
    else
      Print(32, "SMP search split at ply > 1.\n");
  } else if (OptionMatch("smpgsl", *args)) {
    if (nargs < 2) {
      printf("usage:  smpgsl <n>\n");
      return 1;
    }
    smp_gratuitous_limit = atoi(args[1]);
    Print(32, "maximum gratuitous splits allowed %d.\n",
        smp_gratuitous_limit);
  } else if (OptionMatch("smpgsd", *args)) {
    if (nargs < 2) {
      printf("usage:  smpgsd <nodes>\n");
      return 1;
    }
    smp_gratuitous_depth = atoi(args[1]);
    Print(32, "gratuitous split min depth %d.\n", smp_gratuitous_depth);
  }
/*
 ************************************************************
 *                                                          *
 *  "sn" command sets a specific number of nodes to search  *
 *  before stopping.  Note:  this requires -DNODES as an    *
 *  option when building Crafty.                            *
 *                                                          *
 ************************************************************
 */
  else if (OptionMatch("sn", *args)) {
    if (nargs < 2) {
      printf("usage:  sn <nodes>\n");
      return 1;
    }
    search_nodes = atoi(args[1]);
    Print(32, "search nodes set to %" PRIu64 ".\n", search_nodes);
    ponder = 0;
  }
/*
 ************************************************************
 *                                                          *
 *  "speech" command turns speech on/off.                   *
 *                                                          *
 ************************************************************
 */
  else if (OptionMatch("speech", *args)) {
    if (nargs < 2) {
      printf("usage:  speech on|off\n");
      return 1;
    }
    if (!strcmp(args[1], "on"))
      speech = 1;
    else if (!strcmp(args[1], "off"))
      speech = 0;
    if (speech)
      Print(4095, "Audio output enabled\n");
    else
      Print(4095, "Audio output disabled\n");
  }
/*
 ************************************************************
 *                                                          *
 *  "st" command sets a specific search time to control the *
 *  tree search time.                                       *
 *                                                          *
 ************************************************************
 */
  else if (OptionMatch("st", *args)) {
    if (nargs < 2) {
      printf("usage:  st <time>\n");
      return 1;
    }
    search_time_limit = atof(args[1]) * 100;
    Print(32, "search time set to %.2f.\n",
        (float) search_time_limit / 100.0);
  }
/*
 ************************************************************
 *                                                          *
 *  "swindle" command turns swindle mode off/on.            *
 *                                                          *
 ************************************************************
 */
  else if (OptionMatch("swindle", *args)) {
    if (!strcmp(args[1], "on"))
      swindle_mode = 1;
    else if (!strcmp(args[1], "off"))
      swindle_mode = 0;
    else
      printf("usage:  swindle on|off\n");
  }
/*
 ************************************************************
 *                                                          *
 *  "tags" command lists the current PGN header tags.       *
 *                                                          *
 ************************************************************
 */
  else if (OptionMatch("tags", *args)) {
    struct tm *timestruct;
    uint64_t secs;

    secs = time(0);
    timestruct = localtime((time_t *) & secs);
    printf("[Event \"%s\"]\n", pgn_event);
    printf("[Site \"%s\"]\n", pgn_site);
    printf("[Date \"%4d.%02d.%02d\"]\n", timestruct->tm_year + 1900,
        timestruct->tm_mon + 1, timestruct->tm_mday);
    printf("[Round \"%s\"]\n", pgn_round);
    printf("[White \"%s\"]\n", pgn_white);
    printf("[WhiteElo \"%s\"]\n", pgn_white_elo);
    printf("[Black \"%s\"]\n", pgn_black);
    printf("[BlackElo \"%s\"]\n", pgn_black_elo);
    printf("[Result \"%s\"]\n", pgn_result);
  }
/*
 ************************************************************
 *                                                          *
 *  "test" command runs a test suite of problems and        *
 *  displays results.                                       *
 *                                                          *
 ************************************************************
 */
  else if (OptionMatch("test", *args)) {
    FILE *unsolved = NULL;
    int save_noise, save_display;

    if (thinking || pondering)
      return 2;
    nargs = ReadParse(buffer, args, " \t;=");
    if (nargs < 2) {
      printf("usage:  test <filename> [exitcnt]\n");
      return 1;
    }
    save_noise = noise_level;
    save_display = display_options;
    if (nargs > 2)
      early_exit = atoi(args[2]);
    if (nargs > 3)
      unsolved = fopen(args[3], "w+");
    Test(args[1], unsolved, 0, 0);
    noise_level = save_noise;
    display_options = save_display;
    ponder_move = 0;
    last_pv.pathd = 0;
    last_pv.pathl = 0;
    if (unsolved)
      fclose(unsolved);
  }
/*
 ************************************************************
 *                                                          *
 *  "time" is used to set the basic search timing controls. *
 *  The general form of the command is as follows:          *
 *                                                          *
 *    time nmoves/ntime/[nmoves/ntime]/[increment]          *
 *                                                          *
 *  nmoves/ntime represents a traditional first time        *
 *  control when nmoves is an integer representing the      *
 *  number of moves and ntime is the total time allowed for *
 *  these moves.  The [optional] nmoves/ntime is a          *
 *  traditional secondary time control.  Increment is a     *
 *  feature related to ics play and emulates the fischer    *
 *  clock where "increment" is added to the time left after *
 *  each move is made.                                      *
 *                                                          *
 *  As an alternative, nmoves can be "sd" which represents  *
 *  a "sudden death" time control of the remainder of the   *
 *  game played in ntime.  The optional secondary time      *
 *  control can be a sudden-death time control, as in the   *
 *  following example:                                      *
 *                                                          *
 *    time 60/30/sd/30                                      *
 *                                                          *
 *  This sets 60 moves in 30 minutes, then game in 30       *
 *  additional minutes.  An increment can be added if       *
 *  desired.                                                *
 *                                                          *
 ************************************************************
 */
  else if (OptionMatch("time", *args)) {
    if (xboard) {
      tc_time_remaining[Flip(game_wtm)] = atoi(args[1]);
      if (log_file && time_limit > 99)
        fprintf(log_file, "time remaining: %s (Crafty).\n",
            DisplayTime(tc_time_remaining[Flip(game_wtm)]));
    } else {
      if (thinking || pondering)
        return 2;
      tc_moves = 60;
      tc_time = 180000;
      tc_moves_remaining[white] = 60;
      tc_moves_remaining[black] = 60;
      tc_time_remaining[white] = 180000;
      tc_time_remaining[black] = 180000;
      tc_secondary_moves = 60;
      tc_secondary_time = 180000;
      tc_increment = 0;
      tc_sudden_death = 0;
/*
 first let's pick off the basic time control (moves/minutes)
 */
      if (nargs > 1)
        if (!strcmp(args[1], "sd")) {
          tc_sudden_death = 1;
          tc_moves = 1000;
        }
      if (nargs > 2) {
        tc_moves = atoi(args[1]);
        tc_time = atoi(args[2]) * 100;
      }
/*
 now let's pick off the secondary time control (moves/minutes)
 */
      tc_secondary_time = tc_time;
      tc_secondary_moves = tc_moves;
      if (nargs > 4) {
        if (!strcmp(args[3], "sd")) {
          tc_sudden_death = 2;
          tc_secondary_moves = 1000;
        } else
          tc_secondary_moves = atoi(args[3]);
        tc_secondary_time = atoi(args[4]) * 100;
      }
      if (nargs > 5)
        tc_increment = atoi(args[5]) * 100;
      tc_time_remaining[white] = tc_time;
      tc_time_remaining[black] = tc_time;
      tc_moves_remaining[white] = tc_moves;
      tc_moves_remaining[black] = tc_moves;
      if (!tc_sudden_death) {
        Print(32, "%d moves/%d minutes primary time control\n", tc_moves,
            tc_time / 100);
        Print(32, "%d moves/%d minutes secondary time control\n",
            tc_secondary_moves, tc_secondary_time / 100);
        if (tc_increment)
          Print(32, "increment %d seconds.\n", tc_increment / 100);
      } else if (tc_sudden_death == 1) {
        Print(32, " game/%d minutes primary time control\n", tc_time / 100);
        if (tc_increment)
          Print(32, "increment %d seconds.\n", tc_increment / 100);
      } else if (tc_sudden_death == 2) {
        Print(32, "%d moves/%d minutes primary time control\n", tc_moves,
            tc_time / 100);
        Print(32, "game/%d minutes secondary time control\n",
            tc_secondary_time / 100);
        if (tc_increment)
          Print(32, "increment %d seconds.\n", tc_increment / 100);
      }
      tc_time *= 60;
      tc_time_remaining[white] *= 60;
      tc_time_remaining[black] *= 60;
      tc_secondary_time *= 60;
      tc_safety_margin = tc_time / 6;
    }
  }
/*
 ************************************************************
 *                                                          *
 *  "timebook" command is used to adjust Crafty's time      *
 *  usage after it leaves the opening book.  The first      *
 *  value specifies the multiplier for the time added to    *
 *  the first move out of book expressed as a percentage    *
 *  (100 is 100% for example).  The second value specifies  *
 *  the "span" (number of moves) that this multiplier       *
 *  decays over.  For example, "timebook 100 10" says to    *
 *  add 100% of the normal search time for the first move   *
 *  out of book, then 90% for the next, until after 10      *
 *  non-book moves have been played, the percentage has     *
 *  dropped back to 0 where it will stay for the rest of    *
 *  the game.                                               *
 *                                                          *
 ************************************************************
 */
  else if (OptionMatch("timebook", *args)) {
    if (nargs < 3) {
      printf("usage:  timebook <percentage> <move span>\n");
      return 1;
    }
    first_nonbook_factor = atoi(args[1]);
    first_nonbook_span = atoi(args[2]);
    if (first_nonbook_factor < 0 || first_nonbook_factor > 500) {
      Print(4095, "ERROR, factor must be >= 0 and <= 500\n");
      first_nonbook_factor = 0;
    }
    if (first_nonbook_span < 0 || first_nonbook_span > 30) {
      Print(4095, "ERROR, span must be >= 0 and <= 30\n");
      first_nonbook_span = 0;
    }
  }
/*
 ************************************************************
 *                                                          *
 *  "trace" command sets the search trace level which will  *
 *  dump the tree as it is searched.                        *
 *                                                          *
 ************************************************************
 */
  else if (OptionMatch("trace", *args)) {
#if !defined(TRACE)
    printf
        ("Sorry, but I can't display traces unless compiled with -DTRACE\n");
#endif
    if (nargs < 2) {
      printf("usage:  trace <depth>\n");
      return 1;
    }
    trace_level = atoi(args[1]);
    printf("trace=%d\n", trace_level);
  }
/*
 ************************************************************
 *                                                          *
 *  "undo" command backs up 1/2 move, which leaves the      *
 *  opposite side on move. [xboard compatibility]           *
 *                                                          *
 ************************************************************
 */
  else if (OptionMatch("undo", *args)) {
    if (thinking || pondering)
      return 2;
    if (!game_wtm || move_number != 1) {
      game_wtm = Flip(game_wtm);
      if (Flip(game_wtm))
        move_number--;
      sprintf(buffer, "reset %d", move_number);
      Option(tree);
    }
  }
/*
 ************************************************************
 *                                                          *
 *  "usage" command controls the time usage multiplier      *
 *  factors used in the game  - percentage increase or      *
 *  decrease in time used up front.  Enter a number between *
 *  1 to 100 for the % decrease (negative value) or to      *
 *  increase (positive value) although other time           *
 *  limitation controls may kick in.  This more commonly    *
 *  used in the .craftyrc/crafty.rc file.                   *
 *                                                          *
 ************************************************************
 */
  else if (OptionMatch("usage", *args)) {
    if (nargs < 2) {
      printf("usage:  usage <percentage>\n");
      return 1;
    }
    usage_level = atoi(args[1]);
    if (usage_level > 50)
      usage_level = 50;
    else if (usage_level < -50)
      usage_level = -50;
    Print(32, "time usage up front set to %d percent increase/(-)decrease.\n",
        usage_level);
  }
/*
 ************************************************************
 *                                                          *
 *  "variant" command sets the wild variant being played    *
 *  on a chess server.  [xboard compatibility].             *
 *                                                          *
 ************************************************************
 */
  else if (OptionMatch("variant", *args)) {
    if (thinking || pondering)
      return 2;
    printf("command=[%s]\n", buffer);
    return -1;
  }
/*
 ************************************************************
 *                                                          *
 *  "whisper" command sets whisper mode for ICS.  =1 will   *
 *  whisper mate announcements, =2 will whisper scores and  *
 *  other info, =3 will whisper scores and PV, =4 adds the  *
 *  list of book moves, =5 displays the PV after each       *
 *  iteration completes, and =6 displays the PV each time   *
 *  it changes in an iteration.                             *
 *                                                          *
 ************************************************************
 */
  else if (OptionMatch("whisper", *args)) {
    if (nargs < 2) {
      printf("usage:  whisper <level>\n");
      return 1;
    }
    kibitz = 16 + Min(0, atoi(args[1]));
  }
/*
 ************************************************************
 *                                                          *
 *  "white" command sets white to move (wtm).               *
 *                                                          *
 ************************************************************
 */
  else if (OptionMatch("white", *args)) {
    if (thinking || pondering)
      return 2;
    ponder_move = 0;
    last_pv.pathd = 0;
    last_pv.pathl = 0;
    if (!game_wtm)
      Pass();
    force = 0;
  }
/*
 ************************************************************
 *                                                          *
 *  "wild" command sets up an ICS wild position (only 7 at  *
 *  present, but any can be added easily, except for those  *
 *  that Crafty simply can't play (two kings, invisible     *
 *  pieces, etc.)                                           *
 *                                                          *
 ************************************************************
 */
  else if (OptionMatch("wild", *args)) {
    int i;

    if (nargs < 2) {
      printf("usage:  wild <value>\n");
      return 1;
    }
    i = atoi(args[1]);
    switch (i) {
      case 7:
        strcpy(buffer, "setboard 4k/5ppp/////PPP/3K/ w");
        Option(tree);
        break;
      default:
        printf("sorry, only wild7 implemented at present\n");
        break;
    }
  }
/*
 ************************************************************
 *                                                          *
 *  "xboard" command is normally invoked from main() via    *
 *  the xboard command-line option.  It sets proper         *
 *  defaults for Xboard interface requirements.             *
 *                                                          *
 ************************************************************
 */
  else if (OptionMatch("xboard", *args) || OptionMatch("winboard", *args)) {
    if (!xboard) {
      signal(SIGINT, SIG_IGN);
      xboard = 1;
      display_options = 2048;
      Print(-1, "\n");
      Print(-1, "tellicsnoalias set 1 Crafty v%s (%d cpus)\n", version, Max(1,
              smp_max_threads));
      Print(-1, "tellicsnoalias kibitz Hello from Crafty v%s! (%d cpus)\n",
          version, Max(1, smp_max_threads));
    }
  }
/*
 ************************************************************
 *                                                          *
 *  "?" command does nothing, but since this is the "move   *
 *  now" keystroke, if Crafty is not searching, this will   *
 *  simply "wave it off" rather than produce an error.      *
 *                                                          *
 ************************************************************
 */
  else if (OptionMatch("?", *args)) {
  }
/*
 ************************************************************
 *                                                          *
 *  unknown command, it must be a move.                     *
 *                                                          *
 ************************************************************
 */
  else
    return 0;
/*
 ************************************************************
 *                                                          *
 *  command executed, return for another.                   *
 *                                                          *
 ************************************************************
 */
  return 1;
}

/*
 *******************************************************************************
 *                                                                             *
 *   OptionMatch() is used to recognize user commands.  It requires that the   *
 *   command (text input which is the *2nd parameter* conform to the following *
 *   simple rules:                                                             *
 *                                                                             *
 *     1.  The input must match the command, starting at the left-most         *
 *         character.                                                          *
 *     2.  If the command starts with a sequence of characters that could      *
 *         be interpreted as a chess move as well (re for reset and/or rook    *
 *         to the e-file) then the input must match enough of the command      *
 *         to get past the ambiguity (res would be minimum we will accept      *
 *         for the reset command.)                                             *
 *                                                                             *
 *******************************************************************************
 */
int OptionMatch(char *command, char *input) {
/*
 ************************************************************
 *                                                          *
 *  check for the obvious exact match first.                *
 *                                                          *
 ************************************************************
 */
  if (!strcmp(command, input))
    return 1;
/*
 ************************************************************
 *                                                          *
 *  now use strstr() to see if "input" is in "command." the *
 *  first requirement is that input matches command         *
 *  starting at the very left-most character.               *
 *                                                          *
 ************************************************************
 */
  if (strstr(command, input) == command)
    return 1;
  return 0;
}

void OptionPerft(TREE * RESTRICT tree, int ply, int depth, int wtm) {
  unsigned *mv;
#if defined(TRACE)
  static char line[256];
  static char move[16], *p[64];
#endif

  tree->last[ply] = GenerateCaptures(tree, ply, wtm, tree->last[ply - 1]);
  for (mv = tree->last[ply - 1]; mv < tree->last[ply]; mv++)
    if (Captured(*mv) == king)
      return;
  tree->last[ply] = GenerateNoncaptures(tree, ply, wtm, tree->last[ply]);
#if defined(TRACE)
  p[1] = line;
#endif
  for (mv = tree->last[ply - 1]; mv < tree->last[ply]; mv++) {
#if defined(TRACE)
    strcpy(move, OutputMove(tree, ply, wtm, *mv));
#endif
    MakeMove(tree, ply, wtm, *mv);
#if defined(TRACE)
    if (ply <= trace_level) {
      strcpy(p[ply], move);
      strcpy(line + strlen(line), " ");
      p[ply + 1] = line + strlen(line);
      if (ply == trace_level)
        printf("%s\n", line);
    }
#endif
    if (depth - 1)
      OptionPerft(tree, ply + 1, depth - 1, Flip(wtm));
    else if (!Check(wtm))
      total_moves++;
    UnmakeMove(tree, ply, wtm, *mv);
  }
}

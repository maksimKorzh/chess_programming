/**************************************************
 *   Faile version 0.6                            *
 *   Author: Adrien Regimbald                     *
 *   E-mail: adrien@gpu.srv.ualberta.ca           *
 *   Web Page: http://www.ualberta.ca/~adrien/    *
 *                                                *
 *   File: Utils.                                 *
 *   Purpose: Misc. functions for:                *
 *   Printing moves, input/output, debugging,     *
 *   as well as anything I needed to use, but     *
 *   wasn't in the ANSI C standard.               *
 **************************************************/

#include "faile.h"
#include "protos.h"
#include "extvars.h"

void print_tree(int depth, int indent) {

   /* get a local copy of arrays for print_tree: */
   move_s move1[200];
   int numb_moves;
   int a;

   numb_moves = 0;

   if (depth == (Maxdepth + 1)) {
      return;  /*if we are at wanted depth, bail out of print_tree */
   }

   generate_move(&move1[0], &numb_moves);


   /*loop through the moves at given ply */

   for (a = 0; a < numb_moves; a++) {

      int b;/* indent a line */

      for (b = 0; b < indent; b++) {
         putchar (' ');
      }
      print_move (&move1[0], a);

      make_move(&move1[0], a);

      /* for shorter depths, it may be usefull to print out diagrams, rather than
         just the move tree: */
      /* print_pos(); */

      /* now, we call print_tree recursively.  The depth increases,
      and for increased depth, the indent also increases, so that we
      get our "tree" */

      if (check_legal(&move1[0], a))
         print_tree (depth + 1, indent + 6);

      /*unmake the move, and go on to the next one: */

      unmake_move(&move1[0], a);

   }

   /* at this point, the tree is finished printing */

}


void print_move (move_s move[], int d) {

   /* print out in long algerbraic, a move */


   printf("%c%d%c%d\n\n", file (move[d].from) + 96, rank (move[d].from),
      file (move[d].target) + 96, rank (move[d].target));


}

void print_move2 (int from, int target, int promoted_to) {

   /* a version of pieces for this function: */

   char pieces2[17] = {' ', '!', '!', 'N', 'n', '!', '!', 'R', 'r', 'Q', 'q',
      'B', 'b', ' ', '!', '!', '!'};

   /* print out in long algerbraic, a move */

   /* check for that silly black to move after set-up problem: */
   if (!from && !target && !promoted_to) fprintf(stream, "...  ");

   else fprintf(stream, "%c%d%c%d%c", file (from) + 96, rank (from),
      file (target) + 96, rank (target), pieces2[promoted_to]);

}


void print_pos(void) {

   /* print a text based board of the current position */

   int a, b, c;
   char piecesa[17] = {'@', '*', '+', 'N', 'n', 'K', 'k', 'R', 'r', 'Q', 'q',
      'B', 'b', '-', '!', '-', '-'};

   if (comp_color == 2) {
      for (a = 1; a <= 8; a++) {
         fprintf(stream, "%d  ", 9 - a);

         for (b = 0; b <= 11; b++) {
            c = 120 - a*12 + b;

            if (board[c] != 0) {
               fprintf(stream, "%c ", piecesa[board[c]]);
            }
         }

         fprintf(stream, "\n");

      }

      fprintf(stream, "\n   a b c d e f g h\n\n");
   }

   else {
      for (a = 1; a <= 8; a++) {
         fprintf(stream, "%d  ", a);

         for (b = 0; b <= 11; b++) {
            c = 24 + a*12 - b;

            if (board[c] != 0) {
               fprintf(stream, "%c ", piecesa[board[c]]);
            }
         }

         fprintf(stream, "\n");

      }

      fprintf(stream, "\n   h g f e d c b a\n\n");
   }


}


int play_move(move_s move_to_play) {

   int from, target, en_passant, castle, promote_to, i;
   char coord_move[5];

   from = move_to_play.from;
   target = move_to_play.target;
   en_passant = move_to_play.en_passant;
   castle = move_to_play.castle;
   promote_to = move_to_play.promoted;

   /* remove remnant ep_pieces left by think(), or even when checking for
      legal move */
   for (i = 50; i <= 57; i++) {
      if (board[i] == wep_piece) {
         board[i] = npiece;
      }
   }

   for (i = 86; i <= 93; i++) {
      if (board[i] == bep_piece) {
         board[i] = npiece;
      }
   }

   /* update fifty move info: */
   if (board[from] == wpawn || board[from] == bpawn || squares[target])
      fifty = 0;
   else
      fifty++;

   if (book_ply < 20) {
      if (!book_ply) {
         comp_to_coord(move_to_play, coord_move);
         strcpy(opening_history, coord_move);
      }
      else {
         comp_to_coord(move_to_play, coord_move);
         strcat(opening_history, coord_move);
      }
   }

   book_ply++;

   if (!castle) {
      /* determine type of castling, if any: */
      if (from == 30 && target == 32 && board[from] == wking)
         castle = wck;
      else if (from == 30 && target == 28 && board[from] == wking)
         castle = wcq;
      else if (from == 114 && target == 116 && board[from] == bking)
         castle = bck;
      else if (from == 114 && target == 112 && board[from] == bking)
         castle = bcq;
      else
         castle = 0;
   }

   /* check for white kingside castling move: */
   if (castle == wck) {
      board[target] = board[from];
      board[from] = npiece;
      board[33] = npiece;
      board[31] = wrook;
      moved[target]++;
      moved[from]++;
      moved[33]++;
      moved[31]++;
      white_castled = TRUE;
      wking_loc = target;

      init_piece_squares();
      memcpy(rep_kludge[rep_ply++], board, sizeof(board));
      return 1;
   }

   /* check for white queenside castling move: */
   if (castle == wcq) {
      board[target] = board[from];
      board[from] = npiece;
      board[26] = npiece;
      board[29] = wrook;
      moved[target]++;
      moved[from]++;
      moved[26]++;
      moved[29]++;
      white_castled = TRUE;
      wking_loc = target;

      init_piece_squares();
      memcpy(rep_kludge[rep_ply++], board, sizeof(board));
      return 1;
   }

   /* check for black kingside castling move: */
   if (castle == bck) {
      board[target] = board[from];
      board[from] = npiece;
      board[117] = npiece;
      board[115] = brook;
      moved[target]++;
      moved[from]++;
      moved[117]++;
      moved[115]++;
      black_castled = TRUE;
      bking_loc = target;

      init_piece_squares();
      memcpy(rep_kludge[rep_ply++], board, sizeof(board));
      return 1;
   }

   /* check for black queenside castling move: */
   if (castle == bcq) {
      board[target] = board[from];
      board[from] = npiece;
      board[110] = npiece;
      board[113] = brook;
      moved[target]++;
      moved[from]++;
      moved[110]++;
      moved[113]++;
      black_castled = TRUE;
      bking_loc = target;

      init_piece_squares();
      memcpy(rep_kludge[rep_ply++], board, sizeof(board));
      return 1;
   }

   /* check to see if white (computer) made an en passant move: */
   if (en_passant == 1 && (white_to_move %2) == 1) {
     board[target] = board[from];
     board[from] = npiece;
     board[target - 12] = npiece;
     moved[target]++;
     moved[from]++;
     moved[target-12]++;

     init_piece_squares();
     memcpy(rep_kludge[rep_ply++], board, sizeof(board));
     return 1;
   }

   /* check to see if black (computer) made an en passant move: */
   if (en_passant == 1) {
     board[target] = board[from];
     board[from] = npiece;
     board[target + 12] = npiece;
     moved[target]++;
     moved[from]++;
     moved[target+12]++;

     init_piece_squares();
     memcpy(rep_kludge[rep_ply++], board, sizeof(board));
     return 1;
   }

   /* check to see if black moved his pawn up 2 squares allowing ep: */
   if (board[from] == bpawn && rank (from) == 7 && target == from - 24) {
      board[target] = board[from];
      board[from] = npiece;

      /* put in for rep comparison before adding ep_piece */
      memcpy(rep_kludge[rep_ply++], board, sizeof(board));

      board[from - 12] = bep_piece;
      moved[target]++;
      moved[from]++;

      init_piece_squares();
      return 1;
   }

   /* check to see if black (human) made an en passant move: */
   if (board[from] == bpawn && board[target] == wep_piece) {
      board[target] = board[from];
      board[from] = npiece;
      board[target + 12] = npiece;
      moved[target]++;
      moved[from]++;
      moved[target + 12]++;

      init_piece_squares();
      memcpy(rep_kludge[rep_ply++], board, sizeof(board));
      return 1;
   }

   /* check to see if white moved his pawn up 2 squares allowing ep: */
   if (board[from] == wpawn && rank (from) == 2 && target == from + 24) {
      board[target] = board[from];
      board[from] = npiece;

      /* put in for rep comparison before adding ep_piece */
      memcpy(rep_kludge[rep_ply++], board, sizeof(board));

      board[from + 12] = wep_piece;
      moved[target]++;
      moved[from]++;

      init_piece_squares();
      return 1;
   }

   /* check to see if white (human) made an en passant move: */
   if (board[from] == wpawn && board[target] == bep_piece) {
      board[target] = board[from];
      board[from] = npiece;
      board[target - 12] = npiece;
      moved[target]++;
      moved[from]++;
      moved[target - 12]++;

      init_piece_squares();
      memcpy(rep_kludge[rep_ply++], board, sizeof(board));
      return 1;
   }

   /* check for a promotion move:
      (human promotion is handled by the letter cases, a comp. promotion is
      handled by the default case.) */
   if (promote_to != '\0' && promote_to != 0) {
      switch (promote_to) {
         case ('q'):
            board[target] = bqueen;
            break;
         case ('Q'):
            board[target] = wqueen;
            break;
         case ('r'):
            board[target] = brook;
            break;
         case ('R'):
            board[target] = wrook;
            break;
         case ('n'):
            board[target] = bknight;
            break;
         case ('N'):
            board[target] = wknight;
            break;
         case ('b'):
            board[target] = bbishop;
            break;
         case ('B'):
            board[target] = wbishop;
            break;
         default:
            board[target] = promote_to;
            break;
      }

      board[from] = npiece;
      moved[target]++;
      moved[from]++;

      init_piece_squares();
      memcpy(rep_kludge[rep_ply++], board, sizeof(board));
      return 1;
   }

   if (board[from] == wking) wking_loc = target;
   else if (board[from] == bking) bking_loc = target;
   board[target] = board[from];
   board[from] = npiece;
   moved[target]++;
   moved[from]++;

   init_piece_squares();
   memcpy(rep_kludge[rep_ply++], board, sizeof(board));
   return 1;
}


void print_history (int start_num) {

   /* print a move list of the moves made: */

   int i;
   char end_s[7];

   if (result == white_won) strcpy(end_s, "1-0");
   else if (result == black_won) strcpy(end_s, "0-1");
   else strcpy(end_s, "1/2-1/2");

   if (start_num <= 0)
      start_num = 1;

   if (result == no_mate) {
      for (i = start_num; i <= move_num; i++) {
         fprintf(stream, "%-3d ", i);
         if (i == move_num) {
            if ((white_to_move %2) == 1) {
               fprintf(stream, "???\n");
            }
            else {
               fprintf(stream, "%s   ???\n", history[i].white);
            }
         }
         else fprintf(stream,"%s   %s\n", history[i].white, history[i].black);
      }
   }

   else {
      for (i = start_num; i <= move_num; i++) {
         fprintf(stream, "%-3d ", i);
         if (i == move_num) {
            if ((white_to_move %2) == 0) fprintf(stream, "%s   %s\n",
               history[i].white, end_s);
            else fprintf(stream, "%s\n", end_s);
         }
         else fprintf(stream,"%s   %s\n", history[i].white, history[i].black);
      }
   }
      
}

Bool parse_epd(void) {

   /* parse epd input to set up the board: */

   char input[72], *s;

   /* epd_in is used to index the 12x12 board, in the order in which the
      epd will be entered: */
   int epd_in[64] = {
      110, 111, 112, 113, 114, 115, 116, 117,
       98,  99, 100, 101, 102, 103, 104, 105,
       86,  87,  88,  89,  90,  91,  92,  93,
       74,  75,  76,  77,  78,  79,  80,  81,
       62,  63,  64,  65,  66,  67,  68,  69,
       50,  51,  52,  53,  54,  55,  56,  57,
       38,  39,  40,  41,  42,  43,  44,  45,
       26,  27,  28,  29,  30,  31,  32,  33};
   int num_squares = 0, i;
   Bool done_epd = FALSE;

   /* make sure we don't try to use the book, if we've set up a position: */
   book_ply = 21;

   /* initialize all board squares to npiece: */
   for (i = 0; i <= 63; i++) {board[epd_in[i]] = npiece;}

   /* initially don't allow any castling, to be changed later if
      user specifies it should be, also initially assume that all minors
      and the queen have moved, for the purpose of simplifying eval() of
      development before queen movement: */

   memset (moved, 1, sizeof(moved));

   rinput(input,80);

   /* loop through the input: */
   for (s=input, i=0; *s; s++, i++) {
      /* assess the board part of the epd string: */
      if (num_squares <= 63) {
         switch (input[i]) {
            case ('P'):
               board[epd_in[num_squares]] = wpawn;
               num_squares++;
               break;
            case ('p'):
               board[epd_in[num_squares]] = bpawn;
               num_squares++;
               break;
            case ('N'):
               board[epd_in[num_squares]] = wknight;
               num_squares++;
               break;
            case ('n'):
               board[epd_in[num_squares]] = bknight;
               num_squares++;
               break;
            case ('K'):
               board[epd_in[num_squares]] = wking;
               wking_loc = input[num_squares];
               num_squares++;
               break;
            case ('k'):
               board[epd_in[num_squares]] = bking;
               bking_loc = input[num_squares];
               num_squares++;
               break;
            case ('R'):
               board[epd_in[num_squares]] = wrook;
               num_squares++;
               break;
            case ('r'):
               board[epd_in[num_squares]] = brook;
               num_squares++;
               break;
            case ('Q'):
               board[epd_in[num_squares]] = wqueen;
               num_squares++;
               break;
            case ('q'):
               board[epd_in[num_squares]] = bqueen;
               num_squares++;
               break;
            case ('B'):
               board[epd_in[num_squares]] = wbishop;
               num_squares++;
               break;
            case ('b'):
               board[epd_in[num_squares]] = bbishop;
               num_squares++;
               break;
            case ('\n'):
               break;
            case ('/'):
               break;
            default:
               num_squares += input[i]-48;
               break;
         }
      }

      /* go through the info part of the epd string: */
      else if (!done_epd) {
         switch(input[i]) {
            case ('w'):
               white_to_move = 1;
               break;
            case ('b'):
               white_to_move = 2;
               break;
            case ('K'):
               moved[33] = 0;
               break;
            case ('Q'):
               moved[26] = 0;
               break;
            case ('k'):
               moved[117] = 0;
               break;
            case ('q'):
               moved[110] = 0;
               break;
            case ('-'):
               break;
            case ('/'):
               break;
            case ('\n'):
               break;
            default:
               if (white_to_move == 1)
                  board[(input[i]-96+25) + (input[i+1]-49)*12] = bep_piece;
               else
                  board[(input[i]-96+25) + (input[i+1]-49)*12] = wep_piece;
               done_epd = TRUE;
               break;
         }
      }

   }

   init_piece_squares();

   return 1; /* do different returns, if I do more checking within the
                function for a legal setup. */

}

Bool verify_board(void) {

   /* simply check to see that the board is set up right: */

   char verify[2], settings[10];
   int i = 0, j;

   if (white_to_move == 1) {
      settings[i++] = 'w';
      settings[i++] = ' ';
   }

   else {
      settings[i++] = 'b';
      settings[i++] = ' ';
   }

   if (!moved[33]) settings[i++] = 'K';
   if (!moved[26]) settings[i++] = 'Q';
   if (!moved[117]) settings[i++] = 'k';
   if (!moved[110]) settings[i++] = 'q';

   for (j = 26; j < 118; j++) {
      if (board[j] == wep_piece) {
         settings[i++] = ' ';
         settings[i++] = file (j) + 96;
         settings[i++] = rank (j) + 49;
      }
      if (board[j] == bep_piece) {
         settings[i++] = ' ';
         settings[i++] = file (j) + 96;
         settings[i++] = rank (j) + 48;
      }
   }

   settings[i] = '\0';

   rclrscr();
   printf("Please verify that board is set up right!\n");
   print_pos();
   printf("Game state: %s\n\n",settings);
   printf("\nIs the board set up right? (y/n):  ");
   rinput(verify, 1);
   if (verify[0] == 'y')
      return TRUE;
   else
      return FALSE;

}


void rinput (char str[], int n) {

   /* my input function!  ignores whitespace, ends at a linefeed,
      and adds a null character at end of string */

   char ch;
   int i = 0;

   while ((ch = getchar()) != '\n') {
      if (i < n) {
         if (xboard_mode)
            str[i++] = ch;
         else {
            if (ch != ' ')
               str[i++] = ch;
         }
      }
   }

   str[i] = '\0';

}

void rdelay (int time_in_s) {

   /* my delay function.  Will cause a delay of "time_in_s" seconds */

   time_t time1, time2, timer = 0;

   time(&time1);

   while (timer < time_in_s) {
      time(&time2);
      timer = difftime(time2, time1);
   }

}

void rclrscr (void) {

   /* my clear screen function.  Uses ANSI escape characters to clear
      the screen.  I was told this doesn't work on some systems, but I am
      using ANSI C compliant techniques, and I can't do much about non
      ANSI C compliant systems.  If it doesn't work on your system, try to
      get an ANSI handler, and if that's not possible, you can always just
      put in 50 or so "\n"s, but that's pretty ugly ;) */

   printf("\033[H\033[J");

}

void file_output(int move_num) {

   /* appends movelist and diagram of current position to output.txt */

   time_t current = time(NULL);
   FILE *output = fopen ("Failelog.txt", "a");

   stream = output;

   /* print date and time */
   fputs(ctime(&current), output);
   fprintf(output, "\n");

   /*print move list thus far: */
   print_history(1);

   fprintf(output, "\n");

   print_pos();

   fprintf(output, "\n\n");

   fclose (output);

   stream = stdout;

}

void get_user_input(int move_num, char input[]) {

   /* runs until user either makes a move, or quits, ie. allows the user to
      enter multiple commands before moving or quiting */

   int done_input = 0;
   char *s;

   /* flagging xboard_mode will make rinput not avoid spaces: */
   xboard_mode = TRUE;

   while (done_input != 1) {
      printf("Your move:  ");
      rinput(input, 255);
      /* put everything in lower case, to make string comparison easier: */
      for (s=input; *s; s++) {*s = tolower (*s);}
      done_input = 1;
      if ((!strcmp(input, "quit")) || (!strcmp(input, "exit"))) exit(0);
      else if (!strcmp(input, "file")) {
         file_output(move_num);
         refresh_screen(move_num);
         printf("Game file has been appended to Failelog.txt\n\n");
         done_input = 0;
      }
      else if (!strcmp(input, "nps")) {
         refresh_screen(move_num);
         if (search_time == 0) {
            printf("NPS: n/a \n\n");
         }
         else printf("NPS: %li (%0.2f%% qnodes)\n\n",
            (long int)(nodes/clock_elapsed), qnodes_perc);
         done_input = 0;
      }
      else if (!strcmp(input, "nodes")) {
         refresh_screen(move_num);
         printf("Number of nodes: %li\n\n", nodes);
         done_input = 0;
      }
      else if (!strcmp(input, "new")) {
         new_game = TRUE;
      }
      else if (!strcmp(input, "resign")) {
         if (comp_color == 1) result = white_won;
         else result = black_won;
         refresh_screen(move_num);
      }
      else if (!strncmp(input, "level", 5)) {
         /* get time controls: */
         sscanf(input + 6, "%ld %ld %lf", &moves_to_tc,
            &minutes_per_game, &increment);
         time_left = (double) minutes_per_game * 60.0;
         done_input = 0;
         refresh_screen(move_num);
      }
      else if (!strcmp(input, "refresh")) {
         rclrscr();
         refresh_screen(move_num);
         done_input = 0;
      }
      /* future commands here */
   }

   xboard_mode = FALSE;

}

void refresh_screen(int move_num) {

   /* just a function to refresh the screen after executing a command */

   print_history(move_num - 1);
   printf("\n");
   print_pos();
   printf("\a");

}


void init_game(void) {

   /* set up a new game: */

   int i, j;
   int boardi[144] = {
      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
      0,  0,  7,  3, 11,  9,  5, 11,  3,  7,  0,  0,
      0,  0,  1,  1,  1,  1,  1,  1,  1,  1,  0,  0,
      0,  0, 13, 13, 13, 13, 13, 13, 13, 13,  0,  0,
      0,  0, 13, 13, 13, 13, 13, 13, 13, 13,  0,  0,
      0,  0, 13, 13, 13, 13, 13, 13, 13, 13,  0,  0,
      0,  0, 13, 13, 13, 13, 13, 13, 13, 13,  0,  0,
      0,  0,  2,  2,  2,  2,  2,  2,  2,  2,  0,  0,
      0,  0,  8,  4, 12, 10,  6, 12,  4,  8,  0,  0,
      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0
   };

   memcpy(board, boardi, sizeof(boardi));

   for (i = 0; i <= 143; i++) {
      moved[i] = 0;
   }

   /* rather than asses when to print out "...", just init them all to "..."
      to solve the problem ;) */
   for (i = 0; i < 500; i++) {
      strcpy(history[i].white, "...");
      strcpy(history[i].black, "...");
   }

   /* make a dummy pv, which is empty, to put over the pv after a search, so
      that the last pv won't be used on a new game. */

   for (i = 0; i < 100; i++) {
      for (j = 0; j < 100; j++) {
         pv_empty[i][j].from = 0;
         pv_empty[i][j].target = 0;
         pv_empty[i][j].captured = 0;
         pv_empty[i][j].en_passant = 0;
         pv_empty[i][j].promoted = 0;
         pv_empty[i][j].castle = 0;
      }
   }

   init_piece_squares();

   if (!use_book)
      book_ply = 21;
   else book_ply = 0;

   fifty = 0;

   rep_ply = 0;
   memset(rep_kludge, 0, sizeof(rep_kludge));

   new_game = FALSE;
   move_num = 1;
   result = no_mate;
   game_end = FALSE;
   white_to_move = 1;
   wking_loc = 30;
   bking_loc = 114;
   white_castled = FALSE;
   black_castled = FALSE;
   post = TRUE;
   moves_to_tc = 0;
   increment = 0;
   time_left = 300;
   opp_time = 300;
   time_cushion = 0;
   if (!xboard_mode)
      rclrscr();

}

void parse_user_input(void) {

   /* rather than putting this stuff in main(), evaluate it here to make
      main() more readable */

   char input[11], coord_move[6];
   move_s player_move;
   move_s moves[200];
   Bool legal_user_input;
   int num_moves, i;

   legal_user_input = FALSE;
   num_moves = 0;
   generate_move(&moves[0], &num_moves);

   while (!legal_user_input) {
      /* get user's input, and put it in the move_s player_move: */
      get_user_input(move_num, &input[0]);

      if (result || game_end || new_game) break;

      player_move.from = ((input[0]-96+25) + (input[1]-49)*12);
      player_move.target = ((input[2]-96+25) + (input[3]-49)*12);
      player_move.promoted = input[4];
      player_move.castle = 0;
      player_move.en_passant = 0;

      /* compare the user input to legal moves: */

      for (i = 0; i < num_moves; i++) {
         comp_to_coord(moves[i], coord_move);
         if (!strcmp(input, coord_move)) {
            make_move(&moves[0], i);
            if (check_legal(&moves[0], i)) {
               legal_user_input = TRUE;
               /* must change en_passant in user move, to alert
                  play_move to an en_passant move! */
               if (moves[i].en_passant == 1)
                  player_move.en_passant = moves[i].en_passant;
            }
            unmake_move(&moves[0], i);
         }

      }

      if (!legal_user_input) {
         printf("Illegal move!\a\n");
      }
   }

   if (!result && !game_end && !new_game) {
      /* account for get_user_input making everything lower-case: */
      if (comp_color == 2) {
         player_move.promoted = toupper(player_move.promoted);
      }
      play_move(player_move);
      if ((white_to_move %2) == 1) strcpy(history[move_num].white, input);
      else strcpy(history[move_num].black, input);
   }

}


void comp_to_coord (move_s comp_move, char new_move[]) {

   /* convert a computer move_s format of move to the coord. notation: */


   if (!comp_move.promoted) {
      sprintf(new_move, "%c%d%c%d",
         file (comp_move.from) + 96, rank (comp_move.from),
         file (comp_move.target) + 96, rank (comp_move.target));
   }

   /* this is for a move with a promotion: */
   else {
      if (comp_move.promoted == wknight || comp_move.promoted == bknight)
         sprintf(new_move, "%c%d%c%dn",
            file (comp_move.from) + 96, rank (comp_move.from),
            file (comp_move.target) + 96, rank (comp_move.target));
      else if(comp_move.promoted == wrook || comp_move.promoted == brook)
         sprintf(new_move, "%c%d%c%dr",
            file (comp_move.from) + 96, rank (comp_move.from),
            file (comp_move.target) + 96, rank (comp_move.target));
      else if(comp_move.promoted == wbishop || comp_move.promoted == bbishop)
         sprintf(new_move, "%c%d%c%db",
            file (comp_move.from) + 96, rank (comp_move.from),
            file (comp_move.target) + 96, rank (comp_move.target));
      else
         sprintf(new_move, "%c%d%c%dq",
            file (comp_move.from) + 96, rank (comp_move.from),
            file (comp_move.target) + 96, rank (comp_move.target));
   }


}

Bool in_check(void) {

   /* checks if the side to move is in check: */

   if ((white_to_move %2) == 1) {
      if (is_attacked(wking_loc, 2) == 1)
         return TRUE;
      else return FALSE;
   }
   else {
      if (is_attacked(bking_loc, 1) == 1)
         return TRUE;
      else return FALSE;
   }

}


void qsort_moves(move_s moves[], int values[], int start, int end) {

   /* Quick Sort, used to sort my moves.  It's given two array arguments,
      since it's easier to sort an array of values, but we also have to
      change the order of the moves: */

   int partition, i, j, tmpvalue;
   move_s tmpmove;

   if (end > start) {
      i = start;
      j = end;
      partition = values[(i+j)/2];
      do {
         while (values[i] > partition) i++;
         while (values[j] < partition) j--;
         if (i <= j) {
            tmpvalue = values[i];
            values[i] = values[j];
            values[j] = tmpvalue;
            tmpmove = moves[i];
            moves[i] = moves[j];
            moves[j] = tmpmove;
            i++;
            j--;
         }
      } while (i <= j);
      qsort_moves(&moves[0], &values[0], i, end);
      qsort_moves(&moves[0], &values[0], start, j);
   }
}

void init_piece_squares (void) {

   /* reset the piece / square tables: */

   int i;

   num_pieces = 0;

   pieces[0] = 0;

   for (i = 26; i < 118; i++)
      if (board[i] && (board[i] < npiece)) {
         num_pieces += 1;
         pieces[num_pieces] = i;
         squares[i] = num_pieces;
      }
      else
         squares[i] = 0;

}

double allocate_time(void) {

   /* calculate how much time the program can use for this search. */

   double allocated_time = 0;
   double move_speed = 30.0;

   /* calculate sudden death time allocation: */
   if (!moves_to_tc) {
      /* the basic principle is to take time_left, and allways aim to
         make 30 or more moves in the remaining time, so allocated_time
         will decrease as the game goes on.  Under certain circumstances,
         it may be necessary to allocate less time though. */

      /* Consider if we're behind our opponent on time.  If we are
         behind by more than 10%, increase move_speed to 40.  If we are behind
         by more than 5%, increase move_speed to 35: Also, pay opponent's
         time no heed when we're not in xboard_mode, since it's not actually
         updated then. */
      if ((opp_time - time_left) > (opp_time / 10.0) && xboard_mode)
         move_speed = 40.0;
      else if ((opp_time - time_left) > (opp_time / 20.0) && xboard_mode)
         move_speed = 35.0;

      /* if we have > 1 min left && adding an increment won't bring our time
         near flagging, add the increment to allocated_time: */
      if (time_left > 60) {
         allocated_time = time_left / move_speed;
         if ((time_left - allocated_time - increment) > 1 && increment)
            allocated_time += increment;
      }
      /* otherwise, if we have <= 1 min left, allocate 1 second per move,
         plus the increment, if it's safe to do so: */
      else {
         allocated_time = 1.0;
         if ((time_left - allocated_time - increment) > 1 && increment)
            allocated_time += increment;
      }
   }

   /* calculate conventional clock time allocation: */
   else {
      allocated_time = (double) minutes_per_game / moves_to_tc * 60 - 1;
      /* if adding the time_cushion will add >= 1 second(s), add it: */
      if (time_cushion >= 1.0) {
         allocated_time += time_cushion * 2.0 / 3.0;
         time_cushion -= time_cushion * 2.0 / 3.0;
      }
   }

   return allocated_time;

}

Bool is_rep_kludge (void) {

   /* check for draw by repetition.  This involves: 1) checking to see if
      our move will create a 3 fold repetition and 2) checking to see if
      our move allows our opponent to create a 3 fold repetition on the
      next move. */

   int i, matches, rep_check[144], rep_opp_check[144], num_moves = 0, j;
   move_s moves[200];
   Bool is_draw = FALSE;

   /* store the 3 rep draw check for our move: */
   memcpy(rep_check, rep_kludge[rep_ply + 1],
      sizeof(rep_kludge[rep_ply + 1]));
   /* game state (ie. ep_pieces) don't count for 3 rep, so: */
   for (i = 26; i < 118; i++)
      if (rep_check[i] == wep_piece || rep_check[i] == bep_piece)
         rep_check[i] = npiece;

   /* here is where we do a draw by rep kludge: */
   matches = 0;
   for (i = 0; i < rep_ply; i++)
      if (!memcmp(rep_check, rep_kludge[i], sizeof(rep_check)))
         matches++;

   if (matches >= 2)
      return TRUE;

   /* look at our opponent's chance to do a 3 fold repetition: */

   generate_move(&moves[0], &num_moves);

   for (i = 0; i < num_moves; i++) {

      make_move(&moves[0], i);

      if (check_legal(&moves[0], i)) {

         /* store the 3 rep draw check for our opponent's move: */
         memcpy(rep_opp_check, board, sizeof(board));
         /* game state (ie. ep_pieces) don't count for 3 rep, so: */
         for (j = 26; j < 118; j++)
            if (rep_opp_check[j] == wep_piece ||
                rep_opp_check[j] == bep_piece)
               rep_opp_check[j] = npiece;

         /* here is where we do a draw by rep kludge for opp: */
         matches = 0;
         for (j = 0; j < rep_ply; j++)
            if (!memcmp(rep_opp_check, rep_kludge[j], sizeof(rep_opp_check)))
               matches++;

         if (matches >= 2)
            is_draw = TRUE;
      }

      unmake_move(&moves[0], i);
   }

   return is_draw;

}

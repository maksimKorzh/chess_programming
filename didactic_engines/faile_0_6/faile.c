/**************************************************
 *   Faile version 0.6                            *
 *   Author: Adrien Regimbald                     *
 *   E-mail: adrien@gpu.srv.ualberta.ca           *
 *   Web Page: http://www.ualberta.ca/~adrien/    *
 *                                                *
 *   File: Faile.c                                *
 *   Purpose: Main program.                       *
 **************************************************/

#include "faile.h"
#include "protos.h"

#ifdef lines
int line[20][2];
int line_depth;
FILE *lines_out;
#endif

int white_to_move = 1;
Bool game_end = FALSE;
int castle_flag = 0;
long int nodes,qnodes;
double search_time;
double clock_elapsed;
Bool white_castled, black_castled;
int wking_loc = 30 , bking_loc = 114;
int comp_color;
End_of_game result;
Bool new_game;
int move_num;
int ply;
white_black history[500];
Bool captures = FALSE;
Bool time_failure, time_exit;
double time_for_move;
time_t time0;
time_t search_start;
clock_t clock0;
double qnodes_perc;
int pv_length[100];
move_s pv[100][100];
Bool searching_pv;
int history_h[144][144];
int iterative_depth;
move_s pv_empty[100][100];
Bool xboard_mode;
int num_pieces;
int pieces[33];
int squares[144];
int current_score;
Bool post;
double time_left, increment;
long int moves_to_tc, minutes_per_game;
double time_cushion;
double opp_time;
char book[5000][81];
int num_book_lines;
int book_ply;
Bool use_book;
char opening_history[81];
int rep_kludge[500][144];
int rep_ply;
int fifty_move[500];
int fifty;
move_s junk_move = {0, 0, 0, 0, 0, 0, 0};
move_s killer1[100];
move_s killer2[100];
int killer_scores[100];
Bool is_endgame;

FILE *stream;

int board[144] = {
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

int moved[144] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    };

int main(int argc, char *argv[]) {

   move_s comp_move;
   char input_reset_board[2], input[256], human_color[2];
   Bool setup_return;
   char color[2];
   char coord_move[6];

   use_book = init_book();
   if (!use_book)
      printf("Error opening book file, faile.opn\n");

   stream = stdout;

   if (argc == 2 && !strcmp(argv[1], "-xboard"))
      xboard();

   printf("Faile version 0.6\nby Adrien (Reggie) Regimbald\n\n");
   rdelay(2);

   new_game = TRUE;

   while(new_game) {

      init_game();

      printf("What color would you like to play? (w/b):  ");
      rinput(human_color, 1);
      if (tolower(human_color[0]) == 'w') comp_color = 2;
      else comp_color = 1;

      printf("Would you like to set up the board? (y/n):  ");
      rinput(input_reset_board, 1);
      setup_return = FALSE;
      if (tolower(input_reset_board[0]) == 'y') {
         while (setup_return == FALSE) {
            printf("Please enter an EPD string:\n");
            parse_epd();
            setup_return = verify_board();
         }
      }

      if (white_to_move == 1) color[0] = 'w';
      else color[0] = 'b';

      /* print a copy of board so something can be seen while comp. is
         thinking on the first move: */
      rclrscr();
      print_history(1);
      printf("\n");
      print_pos();

      while (!game_end) {

         /* computer's move: */
         if ((comp_color %2) == (white_to_move %2)) {

            nodes = 0;
            ply = 0;

            clock0 = clock();
            time0 = time(0);

            #ifdef lines
            lines_out = fopen("lines.txt", "a");
            #endif

            comp_move = think();

            clock_elapsed = (clock()-clock0)/(double)CLOCKS_PER_SEC;
            search_time = difftime(time(0), time0);

            time_left -= search_time;
            time_left += increment;

            #ifdef lines
            fclose(lines_out);
            #endif

            if (((comp_color == 1 && result != black_won) ||
                (comp_color == 2 && result != white_won)) &&
                (result != stalemate)) {
               play_move(comp_move);

               comp_to_coord (comp_move, coord_move);

               if (comp_color == 1) {
                  strcpy(history[move_num].white, coord_move);
               }

               else {
                  strcpy(history[move_num].black, coord_move);
               }

               white_to_move += 1;
               if (comp_color == 2) move_num++;

            }

            printf("\n");
            print_history(move_num - 1);
            printf("\n");
            print_pos();
            printf("\a"); /* beep after computer moves */

         }

         if (result) {
            game_end = TRUE;
         }

         else {

            /* get user input: */
            parse_user_input();

            if (!result && !game_end) {
               white_to_move += 1;
               if (comp_color == 1) move_num++;

               print_history(move_num);
               printf("\n");
               print_pos();
            }

            if (result || new_game) break;
         }
      }

      /* check to see if we want a new game: */
      if (new_game) continue;
      if (game_end || result) get_user_input(move_num, &input[0]);

   }

   return 0;

}

move_s think(void) {

   /* think() is simply a function to do iterative deepening, as well as
      some time management. */

   double time_used = 0;
   double elapsed = 0, time_given = 0;
   time_t start;
   move_s comp_move, last_move;
   int i_max, j = 0, i;
   int boardt[144];
   char coord_move[6];

   current_score = -100000;

   /* before we do anything, check to see if we can make a move from book! */
   if (book_ply < 20) {
      comp_move = choose_book_move();
      /* if choose_book_move() didn't return a junk move indicating that
         no book move was found, play the book move! :) */
      if (comp_move.from != 0) {
         printf("0 0 0 0 (Book Move)\n");
         return comp_move;
      }
   }

   nodes = 0;
   qnodes = 0;

   start = time(0);

   time_for_move = allocate_time();
   time_given = time_for_move;

   /* if we have <= 10 seconds left, and allocate_time() didn't see fit
      to add any time via increment, we better start moving fast! :) */
   if (time_left <= 10 && time_for_move == 1.0) i_max = lightning_depth;
   else i_max = Maxdepth;

   memcpy (boardt, board, sizeof(boardt));

   memset (history_h, 0, sizeof(history_h));

   memcpy (pv, pv_empty, sizeof(pv_empty));

   time_failure = FALSE;

   init_eval();

   for (i = 0; i < 100; i++) {
      killer1[i] = junk_move;
      killer2[i] = junk_move;
      killer_scores[i] = -100000;
   }

   /* go deeper, iteratively: */

   for (iterative_depth = 1; iterative_depth <= i_max; iterative_depth++) {
      /* if we've used more than 2/3 of time_given, don't go deeper: */
      elapsed = difftime(time(0), start);
      time_used += (elapsed - time_used);

      if (time_used > (2.0/3.0 * time_given)  &&
          iterative_depth > lightning_depth) break;
      if (time_used > time_given && iterative_depth > 1) break;
      time_for_move = time_given - time_used;

      searching_pv = TRUE;

      last_move = search_root (-100000, 100000, iterative_depth);

      /* If the search was stabalized in time, comp_move is set: */
      if (!time_failure || iterative_depth == 1) {
         comp_move = last_move;
         if (iterative_depth >= lightning_depth && post) {
            /* post search output: */
            elapsed = difftime(time(0), start);
            printf("%d ", iterative_depth);
            printf("%d ", current_score);
            printf("%0.0f ", elapsed * 100);
            printf("%li ", nodes);
            for (j = 1; j < pv_length[1]; j++) {
               comp_to_coord(pv[1][j], coord_move);
               printf("%s ", coord_move);
            }
            printf("\n");
         }
      }

      /* I must account for en_passant pieces which are left over if made
         on the last node, since make_move was not called, as well as putting
         back an en_passant piece that may have been present before search,
         but was removed by a call of make_move: */

      memcpy (board, boardt, sizeof(boardt));

      /* reset the killer scores, so that we can determine if we can get
         better killers, but leave the killer moves themselves intact to
         use them for move ordering */

      for (i = 0; i < 100; i++)
         killer_scores[i] = -100000;
   }

   qnodes_perc = (double)qnodes / (double)nodes * 100;

   elapsed = difftime(time(0), start);
   time_cushion += time_for_move - elapsed;

   /* let allocate_time use increment for games with a moves_to_tc, via
      the time cushion, but this way, we ensure it's added only after
      we've moved: */
   if (moves_to_tc > 0 && increment)
     time_cushion += increment;

   return comp_move;

}


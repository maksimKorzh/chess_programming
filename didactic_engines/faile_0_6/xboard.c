/**************************************************
 *   Faile version 0.6                            *
 *   Author: Adrien Regimbald                     *
 *   E-mail: adrien@gpu.srv.ualberta.ca           *
 *   Web Page: http://www.ualberta.ca/~adrien/    *
 *                                                *
 *   File: Xboard.c                               *
 *   Purpose: xboard/winboard module              *
 **************************************************/

#include "faile.h"
#include "protos.h"
#include "extvars.h"


void xboard (void) {

   Bool force_mode, legal_user_input;
   move_s comp_move, player_move, moves[200];
   char input[256], coord_move[6];
   int num_moves, i, comp_color;

   /* init some things, turn off buffer, and ignore signals: */

   xboard_mode = TRUE;
   init_game();
   setbuf(stdout, NULL);
   setbuf(stdin, NULL);
   signal(SIGINT, SIG_IGN);
   force_mode = FALSE;
   comp_color = 0;

   num_moves = 0;

   printf("\n");

   /* go into an infinite loop of getting input and responding to it: */

   while (TRUE) {

      if ((comp_color %2) == (white_to_move %2) && !force_mode) {
         nodes = 0;
         ply = 0;

         clock0 = clock();
         time0 = time(0);

         comp_move = think();

         clock_elapsed = (clock()-clock0)/(double)CLOCKS_PER_SEC;
         search_time = difftime(time(0), time0);

         time_left -= search_time;
         time_left += increment;

         /* check to see if we've been mated: */
         if (((comp_color %2) == 1 && result != black_won) ||
            ((comp_color %2) == 0 && result != white_won)) {

            comp_to_coord (comp_move, coord_move);

            if (result == no_mate) {
               play_move(comp_move);
               white_to_move += 1;
               printf("move %s\n", coord_move);
            }

            else {
               printf("move %s\n", coord_move);
               if (result == black_won)
                  printf ("0-1 {Black Mates}\n");
               else if (result == white_won)
                  printf ("1-0 {White Mates}\n");
               else {
                  if (fifty >= 100)
                     printf("1/2-1/2 {50 Move Rule}\n");
                  else
                     printf ("1/2-1/2 {Draw}\n");
               }
            }
         }

         else {
            if (result == black_won)
               printf ("0-1 {Black Mates}\n");
            else if (result == white_won)
               printf ("1-0 {White Mates}\n");
            else
               printf ("1/2-1/2 {Draw}\n");
         }
            

      }

      /* get our input to check: */
      rinput(input, 255);

      /* check to see if we've got a move, and check for legality: */
      if (isalpha((int)input[0]) && isdigit((int)input[1])
         && isalpha((int)input[2]) && isdigit((int)input[3])) {

         num_moves = 0;

         legal_user_input = FALSE;

         generate_move(&moves[0], &num_moves);

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
            printf("Illegal move: %s\n", input);
         }

         else {
            if ((white_to_move %2) == 1) {
               player_move.promoted = toupper(player_move.promoted);
            }
            play_move(player_move);
            white_to_move += 1;
         }
      }

      /* now, for commands other than moves: */

      if (!strcmp(input, "xboard")) {
         continue;
      }
      if (!strcmp(input, "random"))
         continue;
      if (!strcmp(input, "hard"))
         continue;
      if (!strcmp(input, "easy"))
         continue;
      if (!strcmp(input, "?"))
         continue;
      if (!strcmp(input, "new")) {
         init_game();
         comp_color = 0;
         force_mode = FALSE;
      }
      else if (!strcmp(input, "white")) {
         white_to_move = 1;
         comp_color = 0;
         /* switch clocks here, if I ever keep track of opponent time */
      }
      else if (!strcmp(input, "black")) {
         white_to_move = 0;
         comp_color = 1;
         /* once again, switch clocks here in future */
      }
      else if (!strcmp(input, "force")) {
         force_mode = TRUE;
      }
      else if (!strcmp(input, "go")) {
         comp_color = white_to_move;
         force_mode = FALSE;
      }
      else if (!strncmp(input, "time", 4)) {
         sscanf(input + 5, "%lf", &time_left);
         time_left /= 100;
      }
      else if (!strncmp(input, "otim", 4)) {
         sscanf(input + 5, "%lf", &opp_time);
         opp_time /= 100;
      }
      else if (!strncmp(input, "level", 5)) {
         /* get time controls: */
         sscanf(input + 6, "%ld %ld %lf", &moves_to_tc,
            &minutes_per_game, &increment);
         time_left = (double) minutes_per_game * 60.0;
         opp_time = time_left;
      }
      else if (!strcmp(input, "post")) {
         post = TRUE;
      }
      else if (!strcmp(input, "nopost")) {
         post = FALSE;
      }
      else if (!strcmp(input, "quit")) {
         exit (0);
      }
   }
}




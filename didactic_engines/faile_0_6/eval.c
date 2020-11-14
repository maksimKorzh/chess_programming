/**************************************************
 *   Faile version 0.6                            *
 *   Author: Adrien Regimbald                     *
 *   E-mail: adrien@gpu.srv.ualberta.ca           *
 *   Web Page: http://www.ualberta.ca/~adrien/    *
 *                                                *
 *   File: Eval.c                                 *
 *   Purpose: Functions for evaluation, including *
 *   eval() and in the future, qsearch().         *
 **************************************************/

#include "faile.h"
#include "protos.h"
#include "extvars.h"

/* Bishop and Knight bonus tables will not change during the game, so they
   will be defined here: */

int bishop_b[144] = {
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0, -5, -5, -5, -5, -5, -5, -5, -5,  0,  0,
   0,  0, -5, 10,  0, 10, 10,  0, 10, -5,  0,  0,
   0,  0, -5,  0,  6, 10, 10,  6,  0, -5,  0,  0,
   0,  0, -5,  3, 10,  3,  3, 10,  3, -5,  0,  0,
   0,  0, -5,  3, 10,  3,  3, 10,  3, -5,  0,  0,
   0,  0, -5,  0,  6, 10, 10,  6,  0, -5,  0,  0,
   0,  0, -5, 10,  0, 10, 10,  0, 10, -5,  0,  0,
   0,  0, -5, -5, -5, -5, -5, -5, -5, -5,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0};

int knight_b[144] = {
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,-20,-10,-10,-10,-10,-10,-10,-20,  0,  0,
   0,  0,-10,  0,  0,  0,  0,  0,  0,-10,  0,  0,
   0,  0,-10,  0,  5,  5,  5,  5,  0,-10,  0,  0,
   0,  0,-10,  0,  5, 10, 10,  5,  0,-10,  0,  0,
   0,  0,-10,  0,  5, 10, 10,  5,  0,-10,  0,  0,
   0,  0,-10,  0,  5,  5,  5,  5,  0,-10,  0,  0,
   0,  0,-10,  0,  0,  0,  0,  0,  0,-10,  0,  0,
   0,  0,-20,-10,-10,-10,-10,-10,-10,-20,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0};

/* the bonus tables for pawns and kings will change as we go to the endgame,
   so I will only declare them here: */

int wp_b[144], bp_b[144], wk_b[144], bk_b[144];
Bool king_safety;


void init_eval (void) {

   /* this function basically initializes the bonus tables which will change
      during the endgame */

   int wp[144] = {
      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
      0,  0,  1,  2,  3,  4,  4,  3,  2,  1,  0,  0,
      0,  0,  2,  4,  6,  8,  8,  6,  4,  2,  0,  0,
      0,  0,  3,  6,  9, 12, 12,  9,  6,  3,  0,  0,
      0,  0,  4,  8, 12, 16, 16, 12,  8,  4,  0,  0,
      0,  0,  5, 10, 15, 20, 20, 15, 10,  5,  0,  0,
      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0};


   int bp[144] = {
      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
      0,  0,  5, 10, 15, 20, 20, 15, 10,  5,  0,  0,
      0,  0,  4,  8, 12, 16, 16, 12,  8,  4,  0,  0,
      0,  0,  3,  6,  9, 12, 12,  9,  6,  3,  0,  0,
      0,  0,  2,  4,  6,  8,  8,  6,  4,  2,  0,  0,
      0,  0,  1,  2,  3,  4,  4,  3,  2,  1,  0,  0,
      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0};

   int wk[144] = {
      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
      0,  0, -5, 10, 10,  0,  0,  5, 10, -5,  0,  0,
      0,  0,-10,-10,-10,-10,-10,-10,-10,-10,  0,  0,
      0,  0,-20,-20,-20,-20,-20,-20,-20,-20,  0,  0,
      0,  0,-30,-30,-30,-30,-30,-30,-30,-30,  0,  0,
      0,  0,-40,-40,-40,-40,-40,-40,-40,-40,  0,  0,
      0,  0,-50,-50,-50,-50,-50,-50,-50,-50,  0,  0,
      0,  0,-60,-60,-60,-60,-60,-60,-60,-60,  0,  0,
      0,  0,-70,-70,-70,-70,-70,-70,-70,-70,  0,  0,
      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0};

   int bk[144] = {
      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
      0,  0,-70,-70,-70,-70,-70,-70,-70,-70,  0,  0,
      0,  0,-60,-60,-60,-60,-60,-60,-60,-60,  0,  0,
      0,  0,-50,-50,-50,-50,-50,-50,-50,-50,  0,  0,
      0,  0,-40,-40,-40,-40,-40,-40,-40,-40,  0,  0,
      0,  0,-30,-30,-30,-30,-30,-30,-30,-30,  0,  0,
      0,  0,-20,-20,-20,-20,-20,-20,-20,-20,  0,  0,
      0,  0,-10,-10,-10,-10,-10,-10,-10,-10,  0,  0,
      0,  0, -5, 10, 10,  0,  0,  5, 10, -5,  0,  0,
      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0};

   int ke[144] = {
      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
      0,  0, -5, -3, -1,  0,  0, -1, -3, -5,  0,  0,
      0,  0, -3,  5,  5,  5,  5,  5,  5, -3,  0,  0,
      0,  0, -1,  5, 10, 10, 10, 10,  5, -1,  0,  0,
      0,  0,  0,  5, 10, 15, 15, 10,  5,  0,  0,  0,
      0,  0,  0,  5, 10, 15, 15, 10,  5,  0,  0,  0,
      0,  0, -1,  5, 10, 10, 10, 10,  5, -1,  0,  0,
      0,  0, -3,  5,  5,  5,  5,  5,  5, -3,  0,  0,
      0,  0, -5, -3, -1,  0,  0, -1, -3, -5,  0,  0,
      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0};

   int i, non_pawn, piece;
   Bool endgame;

   endgame = FALSE;
   non_pawn = 0;
   piece = 0;

   for (i = 1; i <= num_pieces; i++) {
      if (squares[pieces[i]])
         piece = board[pieces[i]];
      if (piece != wpawn && piece != bpawn)
         non_pawn += 1;
   }

   /* if we have no more than 4 pieces (and 2 kings), use endgame bonuses */
   if (non_pawn <= 6)
      endgame = TRUE;

   /* alert rest of program to end-game status */
   if (endgame)
      is_endgame = TRUE;
   else
      is_endgame = FALSE;

   if (!endgame) {
      memcpy(wp_b, wp, sizeof(wp));
      memcpy(bp_b, bp, sizeof(bp));

      /* encourage the king to not advance: */
      memcpy(wk_b, wk, sizeof(wk));
      memcpy(bk_b, bk, sizeof(bk));

      /* pay more attention to king safety: */
      king_safety = TRUE;
   }

   else {
      /* give pawn bonuses more bite, to further encourage passing pawns in
         the endgame: */

      for (i = 0; i < 144; i++)
         wp_b[i] = 2 * wp[i];
      for (i = 0; i < 144; i++)
         bp_b[i] = 2 * bp[i];

      /* encourage king centralization: */
      memcpy(wk_b, ke, sizeof(ke));
      memcpy(bk_b, ke, sizeof(ke));

      /* pay less attention to king safety: */
      king_safety = FALSE;
   }

}


int eval(void) {

   /* find a score at the current position for the side to move:
    (initially assume that side to move is white, if false, we will
    change it later with the if statement on the return) */

   int a, score, square, pawn_file, wking_file, bking_file;
   int pawns[2][10];
   Bool isolated;

   score = 0;

   memset (pawns, 0, sizeof(pawns));

   #ifdef lines

   for (a = 1; a < line_depth; a++) {
      if (a != line_depth - 1)
         fprintf(lines_out, "%c%d%c%d ", file(line[a][0])+96,
            rank(line[a][0]), file(line[a][1])+96, rank(line[a][1]));
      else
         fprintf(lines_out, "%c%d%c%d\n", file(line[a][0])+96,
            rank(line[a][0]), file(line[a][1])+96, rank(line[a][1]));
   }
   #endif

   /* loop through the board, setting up our pawns array: (note, the files
      are offset by one, to allow an easy check with imaginary files to left
      of a-file, and right of h-file, rather than using if's in the pawn
      evaluation!)*/

   for (a = 1; a <= num_pieces; a++) {
      square = pieces[a];
      if (board[square] == wpawn)
         pawns[1][file(square) + 1] += 1;
      else if (board[square] == bpawn)
         pawns[0][file(square) + 1] += 1;
   }

   /* now, loop through the board, and add up all piece values & bonuses: */

   for (a = 1; a <= num_pieces; a++) {
      square = pieces[a];
      pawn_file = file(square) + 1;
      switch (board[square]) {
         case (wpawn):
            isolated = FALSE;
            score += 100;
            score += wp_b[square];

            /* check for isolated pawns: */
            if (!pawns[1][pawn_file + 1] && !pawns[1][pawn_file - 1]) {
               score -= 10;
               isolated = TRUE;
               /* check to see if it's exposed as well: */
               if (!pawns[0][pawn_file])
                  score -= 20;
            }

            /* check to see if the pawn is doubled, or tripled, or...: */
            if (pawns[1][pawn_file] > 1)
               score -= 3 * (pawns[1][pawn_file] - 1);

            /* check to see if the pawn is passed: */
            if (!pawns[0][pawn_file] && !pawns[0][pawn_file + 1] &&
               !pawns[0][pawn_file - 1]) {
               score += 15;
               score += 3 * wp_b[square];
               /* check to see if the pawn is connected as well: */
               if (!isolated)
                  score += 30;
            }

            break;

         case (bpawn):
            isolated = FALSE;
            score -= 100;
            score -= bp_b[square];

            /* check for isolated pawns: */
            if (!pawns[0][pawn_file + 1] && !pawns[0][pawn_file - 1]) {
               score += 10;
               isolated = TRUE;
               /* check to see if it's exposed as well: */
               if (!pawns[1][pawn_file])
                  score += 20;
            }

            /* check to see if the pawn is doubled, or tripled, or...: */
            if (pawns[0][pawn_file] > 1)
               score += 3 * (pawns[0][pawn_file] - 1);

            /* check to see if the pawn is passed: */
            if (!pawns[1][pawn_file] && !pawns[1][pawn_file + 1] &&
               !pawns[1][pawn_file - 1]) {
               score -= 15;
               score -= 3 * bp_b[square];
               /* check to see if the pawn is connected as well: */
               if (!isolated)
                  score -= 30;
            }

            break;

         case (wrook):
            score += 500;

            /* give a bonus for being on 7th rank: */
            if (rank(square) == 7)
               score += 10;

            /* check to see how open the rook's file is: */
            if (!pawns[1][pawn_file]) {
               score += 8;
               /* we now know it's at least half open, see if maybe it's
                  completely open: */
               if (!pawns[0][pawn_file])
                  score += 3;
            }

            break;

         case (brook):
            score -= 500;

            /* give a bonus for being on the 2nd rank: */
            if (rank(square) == 2)
               score -= 10;

            /* check to see how open the rook's file is: */
            if (!pawns[0][pawn_file]) {
               score -= 8;
               /* we now know it's at least half open, see if maybe it's
                  completely open: */
               if (!pawns[1][pawn_file])
                  score -= 3;
            }

            break;

         /* minor pieces simply require a piece value, plus a simple bonus */

         case (wbishop):
            score += 325;
            score += bishop_b[square];
            break;

         case (bbishop):
            score -= 325;
            score -= bishop_b[square];
            break;

         case (wknight):
            score += 310;
            score += knight_b[square];
            break;

         case (bknight):
            score -= 310;
            score -= knight_b[square];
            break;

         case (wqueen):
            score += 900;

            /* penalty for the queen moving before the other minors: */
            if (square != 29)
               if (!moved[28] || !moved[27] || !moved[31] || !moved[32])
                  score -= 7;
            break;

         case (bqueen):
            score -= 900;

            /* penalty for the queen moving before the other minors: */
            if (square != 113)
               if (!moved[112] || !moved[111] || !moved[115] || !moved[116])
                  score += 7;
            break;

         case (wking):
            score += wk_b[square];
            if (king_safety) {

               /* encourage castling: */
               if (white_castled) {
                  score += 30;

                  /* encourage keeping a good pawn cover before endgame: */
                  if (moved[square + 11]) {
                     score -= 3;
                     if (board[square + 23] != wpawn)
                        score -= 6;
                  }
                  if (moved[square + 12]) {
                     score -= 4;
                     if (board[square + 24] != wpawn)
                        score -= 8;
                  }
                  if (moved[square + 13]) {
                     score -= 3;
                     if (board[square + 25] != wpawn)
                        score -= 6;
                  }
               }

               /* as a general rule, check for pawn cover near king: */
               if (!pawns[1][pawn_file + 1])
                  score -= 7;
               if (!pawns[1][pawn_file])
                  score -= 7;
               if (!pawns[1][pawn_file - 1])
                  score -= 7;
            }
            break;

         case (bking):
            score -= bk_b[square];
            if (king_safety) {

               /* encourage castling: */
               if (black_castled) {
                  score -= 30;

                  /* encourage keeping a good pawn cover before endgame: */
                  if (moved[square - 11]) {
                     score += 3;
                     if (board[square - 23] != bpawn)
                        score += 6;
                  }
                  if (moved[square - 12]) {
                     score += 4;
                     if (board[square - 24] != bpawn)
                        score += 8;
                  }
                  if (moved[square - 13]) {
                     score += 3;
                     if (board[square - 25] != bpawn)
                        score += 6;
                  }
               }

               /* as a general rule, check for pawn cover near king: */
               if (!pawns[0][pawn_file + 1])
                  score += 7;
               if (!pawns[0][pawn_file])
                  score += 7;
               if (!pawns[0][pawn_file - 1])
                  score += 7;
            }
            break;
      }
   }

   /* if there is > 2 files between the white and black king, we need to
      pay attention to pawn storms and open files: */

   if (king_safety) {
      wking_file = file (wking_loc);
      bking_file = file (bking_loc);
      if (((wking_file - bking_file) > 2) ||
          ((wking_file - bking_file) < 2)) {
         /* discourage black pawn storms! */
         if (rank (wking_loc) == 1) {
            if (moved[wking_loc + 71]) {
               if (board[wking_loc + 35] == bpawn)
                  score -= 8;
               if (board[wking_loc + 23] == bpawn)
                  score -= 11;
            }
            if (moved[wking_loc + 72]) {
               if (board[wking_loc + 36] == bpawn)
                  score -= 8;
               if (board[wking_loc + 24] == bpawn)
                  score -= 11;
            }
            if (moved[wking_loc + 73]) {
               if (board[wking_loc + 37] == bpawn)
                  score -= 8;
               if (board[wking_loc + 25] == bpawn)
                  score -= 11;
            }
         }
         /* otherwise, you're on opposite sides, and your king isn't on the
            first rank!! not good! :) */
         else
            score -= 8;

         /* discourage white pawn storms (for black)! */
         if (rank (bking_loc) == 8) {
            if (moved[bking_loc - 71]) {
               if (board[bking_loc - 35] == wpawn)
                  score += 8;
               if (board[bking_loc - 23] == wpawn)
                  score += 11;
            }
            if (moved[bking_loc - 72]) {
               if (board[bking_loc - 36] == wpawn)
                  score += 8;
               if (board[bking_loc - 24] == wpawn)
                  score += 11;
            }
            if (moved[bking_loc - 73]) {
               if (board[bking_loc - 37] == wpawn)
                  score += 8;
               if (board[bking_loc - 25] == wpawn)
                  score += 11;
            }
         }
         /* otherwise, you're on opposite sides, and your king isn't on the
            eighth rank!! not good! :) */
         else
            score += 8;

         /* if the kings are on opposite wings, they are free to open up
            lines on the opposite wing, so we must give penalties to a
            player who's king is on one of these lines: */

         /* from white's point of view: */
         if (!pawns[0][wking_file + 2])
            score -= 4;
         if (!pawns[0][wking_file + 1])
            score -= 5;
         if (!pawns[0][wking_file])
            score -= 4;

         /* from black's point of view: */
         if (!pawns[1][bking_file + 2])
            score += 4;
         if (!pawns[1][bking_file + 1])
            score += 5;
         if (!pawns[1][bking_file])
            score += 4;
      }
   }

   /* make sure that the e & d pawns aren't blocked before they
      can move! */

   if (!moved[41] && moved[53])
      score -= 8;
   if (!moved[42] && moved[54])
      score -= 8;
   if (!moved[101] && moved[89])
      score += 8;
   if (!moved[102] && moved[90])
      score += 8;

   if (white_to_move % 2 != 1) return -score;
   return score;

}

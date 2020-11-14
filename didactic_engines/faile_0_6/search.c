/**************************************************
 *   Faile version 0.6                            *
 *   Author: Adrien Regimbald                     *
 *   E-mail: adrien@gpu.srv.ualberta.ca           *
 *   Web Page: http://www.ualberta.ca/~adrien/    *
 *                                                *
 *   File: Search.c                               *
 *   Purpose: Functions for tree search:          *
 *   root search, search, as well as pruning      *
 **************************************************/

#include "faile.h"
#include "protos.h"
#include "extvars.h"

move_s search_root(int alpha, int beta, int depth) {
   /* get a local copy of arrays for search_root: */
   move_s move1[200];

   int numb_moves;
   int a, root_score = 0, j;
   Bool no_moves, legal_move, check_best;
   char coord_move[6];
   double elapsed = 0;

   move_s best_move = junk_move;

   search_start = time(0);

   numb_moves = 0;
   no_moves = TRUE;
   captures = FALSE;
   time_exit = FALSE;
   ply = 1;
   time_failure = TRUE;

   pv_length[ply] = ply;

   if (in_check() ) depth++;

   generate_move(&move1[0], &numb_moves);

   order_moves(&move1[0], numb_moves);

   /*loop through the moves at a given ply */

   for (a = 0; a < numb_moves; a++) {

      #ifdef lines
      line_depth = 1;
      #endif

      make_move(&move1[0], a);
      ply++;

      legal_move = FALSE;
      check_best = FALSE;

      /* assert (check_legal(&move1, a) != -1); */

      /* only go to next depth if the move was legal: */
      if (check_legal(&move1[0], a)) {
         memcpy(rep_kludge[rep_ply + 1], board, sizeof(board));
         nodes += 1;
         legal_move = TRUE;
         root_score = -search(-beta, -alpha, depth-1);
         if (!time_exit)
            time_failure = FALSE;
         /* dictate whether we should check this move against the best move.
            do this in 2 cases: 1) We don't have a time_exit, or 2) It is
            our first found move. */
         if (!time_exit || no_moves)
            check_best = TRUE;

         no_moves = FALSE;

         /* check to see if we're facing a draw by 3 rep: */
         if (is_rep_kludge())
            /* assume that if we are doing well, that it will be a draw,
               and that if we are doing poorly, that it will not be a draw.
               This is just a temporary kludge, after all ;) */
            if (root_score)
               root_score = 0;
      }

      ply--;
      unmake_move(&move1[0], a);

      /* only consider legal moves to return : */
      if (root_score > alpha && legal_move && check_best) {
         /* we have a cutoff, so increase this move's history value, so
            that we can order it higher in order_moves(), the idea being
            that we may get an earlier cutoff */
         history_h[move1[a].from][move1[a].target] += depth;

         /* since we have a cutoff, we should also make this better cutoff
            killer1, and make the previous killer1 be killer2. */
         if (root_score > killer_scores[ply] && !time_exit) {
            killer_scores[ply] = root_score;
            killer2[ply] = killer1[ply];
            killer1[ply] = move1[a];
         }

         best_move = move1[a];
         alpha = root_score;
         current_score = alpha;

         /* set the principle variation (pv): */
         pv[ply][ply] = move1[a];
         for (j = ply + 1; j < pv_length[ply + 1]; j++)
            pv[ply][j] = pv[ply + 1][j];
         pv_length[ply] = pv_length[ply + 1];

         if (iterative_depth >= lightning_depth && post &&!time_exit) {
            /* post search output: */
            elapsed = difftime(time(0), time0);
            printf("%d ", iterative_depth);
            printf("%d ", alpha);
            printf("%0.0f ", elapsed * 100);
            printf("%li ", nodes);
            for (j = 1; j < pv_length[1]; j++) {
               comp_to_coord(pv[1][j], coord_move);
               printf("%s ", coord_move);
            }
            printf("\n");
         }
      }

      /* check to see if we have to exit the search, due to a time_exit: */
      if (time_exit) {
         /* if we have found a best_move before time_exit, we can use that */
         if (!no_moves)
            return best_move;
         /* otherwise, we have to keep searching for a legal move to return */
      }

   }

   /* if no_moves == 1, we have to check for mate/stalemate: */

   if (no_moves == 1) {
      if ((white_to_move %2) == 1) {
         if (is_attacked(wking_loc, 2)) {
            result = black_won;
         }
         else {
            result = stalemate;
         }
      }
      else if ((white_to_move %2) == 0) {
         if (is_attacked(bking_loc, 1)) {
            result = white_won;
         }
         else {
            result = stalemate;
         }
      }
   }

   if (root_score == 99998) {
      if ((white_to_move %2) == 1) result = white_won;
      else result = black_won;
   }

   return best_move;
}



int search (int alpha, int beta, int depth) {
   /* get a local copy of arrays for search: */
   move_s move1[200];
   int numb_moves;
   int a, score = 0, j;
   Bool no_moves, legal_move;

   numb_moves = 0;
   no_moves = TRUE;
   captures = FALSE;

   /* check to see if we should go deeper based on time left: */
   if ((iterative_depth > lightning_depth) && !(nodes & 1000)) {
      if (time_for_move - difftime(time(0), search_start) <= 0) {
         time_exit = TRUE;
         return 0;
      }
   }

   if (ply < Maxdepth + 1 && in_check() ) depth++;

   if (!depth) {

      #ifdef lines
      line_depth = ply;
      #endif

      /* try to find a stable position to evaluate with qsearch: */
      score = qsearch(alpha, beta, (Maxdepth * 3));
      return score;
   }

   else {

      pv_length[ply] = ply;

      generate_move(&move1[0], &numb_moves);

      order_moves(&move1[0], numb_moves);

      /*loop through the moves at a given ply */

      for (a = 0; a < numb_moves; a++) {

         #ifdef lines
         line_depth = ply;
         #endif

         make_move(&move1[0], a);
         ply++;

         /* assert (check_legal(&move1, a) != -1); */

         legal_move = FALSE;

         /* only go to next depth if the move was legal: */
         if (check_legal(&move1[0], a)) {
            nodes += 1;
            no_moves = FALSE;
            legal_move = TRUE;
            score = -search(-beta, -alpha, depth-1);
         }

         ply--;
         unmake_move(&move1[0], a);

         /* check to see if we ran out of time going deeper: */
         if (time_exit) return 0;

         if ((score > alpha) && legal_move) {
            /* we have a cutoff, so increase this move's history value, so
               that we can order it higher in order_moves(), the idea being
               that we may get an earlier cutoff */
            history_h[move1[a].from][move1[a].target] += depth;

            /* since we have a cutoff, we should also make this better cutoff
               killer1, and make the previous killer1 be killer2. */
            if (score > killer_scores[ply]) {
               killer_scores[ply] = score;
               killer2[ply] = killer1[ply];
               killer1[ply] = move1[a];
            }

            if (score >= beta)
               return beta;
            alpha = score;

            /* set the pv: */
            pv[ply][ply] = move1[a];
            for (j = ply + 1; j < pv_length[ply + 1]; j++)
               pv[ply][j] = pv[ply + 1][j];
            pv_length[ply] = pv_length[ply + 1];
         }

      }

      /* if no_moves == 1, we have to check for mate/stalemate: */

      if ((no_moves == 1)) {
         if (in_check())
            return (-100000 + ply);
         else return 0;
      }

   }

   /* check for draw by 50 move rule: (we have to wait for fifty > 100, b/c
      it is possible that one side mated on it's 50th move, in which case
      it is not a draw.  Therefore we must go one ply deeper to verify
      that there is no mate) */
   if (fifty > 100)
      return 0;

   return alpha;

}

void order_moves(move_s move[], int num_moves) {

   /* order the moves based on (material gained - capturing piece), to make
      move ordering better, to hopefully make alpha-beta find a cutoff
      earlier */

   int values[200], i, from, target, promoted, captured;
   int c_val[17] = {0, 100, 100, 310, 310, 4500, 4500, 515, 515,
      955, 955, 325, 325, 0, 100, 100}; /* ep_pieces were given a value,
      since captured shows the ep_piece, not the pawn */

   /* fill the values array: */

   /* if we're still searching the pv, give the pv move a bonus, so it will
      be searched first: */

   if (searching_pv && !captures) {
      searching_pv = FALSE;
      for (i = 0; i < num_moves; i++) {
         from = move[i].from;
         target = move[i].target;
         promoted = move[i].promoted;
         captured = move[i].captured;
         /* look at captures first, and look at captures with the highest
            likely material gain first: */
         if (captured != npiece)
            values[i] = c_val[captured] - c_val[board[from]];
         else values[i] = -10000;
         /* pv bonus: */
         if (from == pv[1][ply].from &&
             target == pv[1][ply].target &&
             promoted == pv[1][ply].promoted) {
            searching_pv = TRUE;
            values[i] += 100000;
         }
         /* no need to add anything more to the pv... it will be ordered
            first already */
         else {
            /* history heuristic bonus: */
            values[i] += history_h[from][target];

            /* killer move bonus: */
            if (from == killer1[ply].from &&
                target == killer1[ply].target &&
                promoted == killer1[ply].promoted)
               values[i] += 1000;
            else if (from == killer2[ply].from &&
                target == killer2[ply].target &&
                promoted == killer2[ply].promoted)
               values[i] += 500;
         }
      }
   }

   /* special handling while in qsearch(), since we don't update history_h
      (due to the problem of depth increasing on first call of qsearch()): */
   else if (searching_pv) {
      searching_pv = FALSE;
      for (i = 0; i < num_moves; i++) {
         from = move[i].from;
         target = move[i].target;
         promoted = move[i].promoted;
         captured = move[i].captured;
         /* look at captures first, and look at captures with the highest
            likely material gain first: */
         values[i] = c_val[captured] - c_val[board[from]];
         /* pv bonus: */
         if (from == pv[1][ply].from &&
             target == pv[1][ply].target &&
             promoted == pv[1][ply].promoted) {
            searching_pv = TRUE;
            values[i] += 100000;
         }
         else {
            /* killer move bonus: */
            if (from == killer1[ply].from &&
                target == killer1[ply].target &&
                promoted == killer1[ply].promoted)
               values[i] += 1000;
            else if (from == killer2[ply].from &&
                target == killer2[ply].target &&
                promoted == killer2[ply].promoted)
               values[i] += 500;
         }
      }
   }

   /* more special consideration for qsearch, but when we're not searching
      the pv. */
   else if (!searching_pv && captures) {
      searching_pv = FALSE;
      for (i = 0; i < num_moves; i++) {
         from = move[i].from;
         target = move[i].target;
         promoted = move[i].promoted;
         captured = move[i].captured;
         /* look at captures first, and look at captures with the highest
            likely material gain first: */
         values[i] = c_val[captured] - c_val[board[from]];

         /* killer move bonus : */
         if (from == killer1[ply].from &&
             target == killer1[ply].target &&
             promoted == killer1[ply].promoted)
            values[i] += 1000;
         else if (from == killer2[ply].from &&
             target == killer2[ply].target &&
             promoted == killer2[ply].promoted)
            values[i] += 500;
      }
   }


   else {
      for (i = 0; i < num_moves; i++) {
         from = move[i].from;
         target = move[i].target;
         promoted = move[i].promoted;
         captured = move[i].captured;
         /* look at captures first, and look at captures with the highest
            likely material gain first: */
         if (captured != npiece)
            values[i] = c_val[captured] - c_val[board[from]];
         else values[i] = -10000;

         /* history heuristic bonus: */
         values[i] += history_h[from][target];

         /* killer move bonus: */
         if (from == killer1[ply].from &&
             target == killer1[ply].target &&
             promoted == killer1[ply].promoted)
            values[i] += 1000;
         else if (from == killer2[ply].from &&
             target == killer2[ply].target &&
             promoted == killer2[ply].promoted)
            values[i] += 500;
      }
   }

   /* sort the move list: */
   qsort_moves(&move[0], &values[0], 0, num_moves-1);

   /* for (i = 0; i < num_moves-1; i++)
      assert(values[i] >= values[i+1]); */
}


int qsearch (int alpha, int beta, int depth) {
   /* get a local copy of arrays for search: */
   move_s move1[200];
   int numb_moves;
   int a, score = 0, standpat, j;
   Bool legal_move;

   numb_moves = 0;

   if (!depth) {

      #ifdef lines
      line_depth = ply;
      #endif

      score = eval(); /* we can't look at captures TOO long, so we must end
                         it sometime! */
      return score;
   }

   else {

      /* see if we can exit qsearch early with a good standpat score: */
      standpat = eval();
      if (standpat >= beta) return standpat;
      else if (standpat > alpha) alpha = standpat;

      /* generate capture moves only: */
      captures = TRUE;
      generate_move(&move1[0], &numb_moves);

      pv_length[ply] = ply;

      order_moves(&move1[0], numb_moves);
      captures = FALSE; /* here as a cheap way of exiting ordering faster */

      /*loop through the moves at a given ply */

      for (a = 0; a < numb_moves; a++) {

         #ifdef lines
         line_depth = ply;
         #endif

         make_move(&move1[0], a);
         ply++;

         /* assert (check_legal(&move1, a) != -1); */

         legal_move = FALSE;

         /* only go to next depth if the move was legal: */
         if (check_legal(&move1[0], a)) {
            nodes += 1;
            qnodes += 1;
            legal_move = TRUE;
            score = -qsearch(-beta, -alpha, depth-1);
         }

         ply--;
         unmake_move(&move1[0], a);

         if ((score > alpha) && legal_move) {
            /* since we have a cutoff, we should also make this better cutoff
               killer1, and make the previous killer1 be killer2. */
            if (score > killer_scores[ply]) {
               killer_scores[ply] = score;
               killer2[ply] = killer1[ply];
               killer1[ply] = move1[a];
            }
            if (score >= beta)
               return beta;

            alpha = score;

            /* set the pv: */
            pv[ply][ply] = move1[a];
            for (j = ply + 1; j < pv_length[ply + 1]; j++)
               pv[ply][j] = pv[ply + 1][j];
            pv_length[ply] = pv_length[ply + 1];
         }

      }

      /* since we are not looking at non-capture moves, we can't tell if we
         are mated/stalemated, thus there is no need to check to see if we
         have any moves or not. */

   }

   return alpha;

}



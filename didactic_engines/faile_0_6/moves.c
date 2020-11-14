/**************************************************
 *   Faile version 0.6                            *
 *   Author: Adrien Regimbald                     *
 *   E-mail: adrien@gpu.srv.ualberta.ca           *
 *   Web Page: http://www.ualberta.ca/~adrien/    *
 *                                                *
 *   File: Moves.c                                *
 *   Purpose: Functions for move handling:        *
 *   Generation, making, unmaking, and legality   *
 **************************************************/

#include "faile.h"
#include "protos.h"
#include "extvars.h"

int generate_move(move_s move2[], int *n_moves) {


   int from, target, i;

   castle_flag = 0;

   /* check to see that we only consider moves for the side to move! */

   if (white_to_move %2 == 1) {
      /* for (from = (26) ; from < (118) ; from++) {
         switch(board[from]) { */
      for (i = 1; i <= num_pieces; i++) {
         from = pieces[i];
         switch (board[from]) {
            case (wpawn):
               if ((rank (from) == 2) && (board[from + 24] == 13)
                  && (board[from +12] == 13))  TryOffset(24);
               if (((board[from + 13] %2) == 0) && (board[from + 13] != 0))
                  TryOffset(13);
               if (((board[from + 11] %2) == 0) && (board[from + 11] != 0))
                  TryOffset(11);
               if (board[from + 12] == 13) TryOffset(12);
               break;
            case (wknight):
               TryOffset(10);
               TryOffset(- 10);
               TryOffset(14);
               TryOffset(- 14);
               TryOffset(23);
               TryOffset(- 23);
               TryOffset(25);
               TryOffset(- 25);
               break;
            case (wbishop):
               TrySlide(13);
               TrySlide(-13);
               TrySlide(11);
               TrySlide(-11);
               break;
            case (wrook):
               TrySlide(12);
               TrySlide(-12);
               TrySlide(1);
               TrySlide(-1);
               break;
            case (wking):
               TryOffset(1);
               TryOffset(- 1);
               TryOffset(11);
               TryOffset(- 11);
               TryOffset(12);
               TryOffset(- 12);
               TryOffset(13);
               TryOffset(- 13);
               /* try white castling: */
               if (from == 30) {
                  if (moved[30] == 0) {
                     if ((moved[33] == 0) && (board[33] == wrook)) {
                        if ((board[31] == npiece) && (board[32] == npiece)) {
                           castle_flag = wck;
                           TryOffset(2);
                        }
                     }
                     if ((moved[26] == 0) && (board[26] == wrook)) {
                        if ((board[27] == npiece) && (board[28] == npiece)
                           && (board[29] == npiece)) {
                           castle_flag = wcq;
                           TryOffset(-2);
                        }
                     }
                  }
               }
               break;
            case (wqueen):
               TrySlide(12);
               TrySlide(-12);
               TrySlide(-1);
               TrySlide(1);
               TrySlide(13);
               TrySlide(-13);
               TrySlide(11);
               TrySlide(-11);
               break;
            default:
               break;
         }
      }
   }

   else {
      /* for (from = (26) ; from < (118) ; from++) {
         switch(board[from]) { */
      for (i = 1; i <= num_pieces; i++) {
         from = pieces[i];
         switch (board[from]) {
            case (bpawn):
               if ((rank (from) == 7) && (board[from - 24] == 13)
                  && (board[from - 12] == 13)) TryOffset(- 24);
               if (((board[from - 13] %2) == 1) && (board[from - 13] != 13))
                  TryOffset(- 13);
               if (((board[from - 11] %2) == 1) && (board[from - 11] != 13))
                  TryOffset(- 11);
    	         if (board[from - 12] == 13) TryOffset(- 12);
               break;
            case (bknight):
               TryOffset(10);
               TryOffset(- 10);
               TryOffset(14);
               TryOffset(- 14);
               TryOffset(23);
               TryOffset(- 23);
               TryOffset(25);
               TryOffset(- 25);
               break;
            case (bbishop):
               TrySlide(13);
               TrySlide(-13);
               TrySlide(11);
               TrySlide(-11);
               break;
            case (brook):
               TrySlide(12);
               TrySlide(-12);
               TrySlide(1);
               TrySlide(-1);
               break;
            case (bking):
               TryOffset(1);
               TryOffset(- 1);
               TryOffset(11);
               TryOffset(- 11);
               TryOffset(12);
               TryOffset(- 12);
               TryOffset(13);
               TryOffset(- 13);
               /* try black castling: */
               if (from == 114) {
                  if (moved[114] == 0) {
                     if ((moved[117] == 0) && (board[117] == brook)) {
                        if ((board[115]==npiece) && (board[116]==npiece)) {
                           castle_flag = bck;
                           TryOffset(2);
                        }
                     }
                     if ((moved[110] == 0) && (board[110] == brook)) {
                        if ((board[111]==npiece) && (board[112]==npiece)
                           && (board[113] == npiece)) {
                           castle_flag = bcq;
                           TryOffset(-2);
                        }
                     }
                  }
               }
               break;
            case (bqueen):
               TrySlide(12);
               TrySlide(-12);
               TrySlide(-1);
               TrySlide(1);
               TrySlide(13);
               TrySlide(-13);
               TrySlide(11);
               TrySlide(-11);
               break;
            default:
               break;
         }
      }
   }

   return 1;

}

int try_move(move_s move[], int *num_moves, int from, int target) {

   if (board[target] == frame) return 0;

   if (castle_flag && captures) return 0;

   /* add white kingside castling moves to move arrays */
   if (castle_flag == wck) {
      move[*num_moves].from = from;
      move[*num_moves].target = target;
      move[*num_moves].captured = board[target];
      move[*num_moves].en_passant = 0;
      move[*num_moves].castle = wck;
      move[*num_moves].promoted = 0;
      (*num_moves)++;
      castle_flag = 0;

      return 1;
   }

   /* add white queenside castling moves to move arrays */
   if (castle_flag == wcq) {
      move[*num_moves].from = from;
      move[*num_moves].target = target;
      move[*num_moves].captured = board[target];
      move[*num_moves].en_passant = 0;
      move[*num_moves].castle = wcq;
      move[*num_moves].promoted = 0;
      (*num_moves)++;
      castle_flag = 0;

      return 1;
   }

   /* add black kingside castling moves to move arrays */
   if (castle_flag == bck) {
      move[*num_moves].from = from;
      move[*num_moves].target = target;
      move[*num_moves].captured = board[target];
      move[*num_moves].en_passant = 0;
      move[*num_moves].castle = bck;
      move[*num_moves].promoted = 0;
      (*num_moves)++;
      castle_flag = 0;

      return 1;
   }

   /* add black queenside castling moves to move arrays */
   if (castle_flag == bcq) {
      move[*num_moves].from = from;
      move[*num_moves].target = target;
      move[*num_moves].captured = board[target];
      move[*num_moves].en_passant = 0;
      move[*num_moves].castle = bcq;
      move[*num_moves].promoted = 0;
      (*num_moves)++;
      castle_flag = 0;

      return 1;
   }

   /* add white en passant moves to move arrays.  en_passanta value of 1 will
      be used in make_move and unmake_move */
   if ((board[from] == wpawn) && (board[target] == bep_piece)) {
      move[*num_moves].from = from;
      move[*num_moves].target = target;
      move[*num_moves].captured = board[target];
      move[*num_moves].en_passant = 1;
      move[*num_moves].castle = 0;
      move[*num_moves].promoted = 0;
      (*num_moves)++;

      return 1;
   }

   /* add black en passant moves to move arrays.  en_passanta value of 1 will
      be used in make_move and unmake_move */
   if ((board[from] == bpawn) && (board[target] == wep_piece)) {
      move[*num_moves].from = from;
      move[*num_moves].target = target;
      move[*num_moves].captured = board[target];
      move[*num_moves].en_passant = 1;
      move[*num_moves].castle = 0;
      move[*num_moves].promoted = 0;
      (*num_moves)++;

      return 1;
   }

   /* look at capture moves: (taking an "ep_piece" doesn't count as capture)*/
   if ((board[target] != npiece) && (board[target] != wep_piece)
      && (board[target] != bep_piece)) {

      /* the if statement stops a piece from capturing another piece of the
      same color */

      if ((board[from] %2) == (board[target] % 2))
         return 0;

      /* look at white promotion moves done by capturing: */

      else if ((board[from] == wpawn) && (rank(from) == 7)) {

         /* promotion to queen: */
         move[*num_moves].from = from;
         move[*num_moves].target = target;
         move[*num_moves].captured = board[target];
         move[*num_moves].en_passant = 0;
         move[*num_moves].castle = 0;
         move[*num_moves].promoted = wqueen;

         (*num_moves)++;

         /* promotion to rook: */
         move[*num_moves].from = from;
         move[*num_moves].target = target;
         move[*num_moves].captured = board[target];
         move[*num_moves].en_passant = 0;
         move[*num_moves].castle = 0;
         move[*num_moves].promoted = wrook;

         (*num_moves)++;

         /* promotion to knight: */
         move[*num_moves].from = from;
         move[*num_moves].target = target;
         move[*num_moves].captured = board[target];
         move[*num_moves].en_passant = 0;
         move[*num_moves].castle = 0;
         move[*num_moves].promoted = wknight;

         (*num_moves)++;

         /* promotion to bishop: */
         move[*num_moves].from = from;
         move[*num_moves].target = target;
         move[*num_moves].captured = board[target];
         move[*num_moves].en_passant = 0;
         move[*num_moves].castle = 0;
         move[*num_moves].promoted = wbishop;

         (*num_moves)++;

         return 0;
      }

      /* look at black promotion moves done by capturing: */

      else if ((board[from] == bpawn) && (rank(from) == 2)) {

         /* promotion to queen: */
         move[*num_moves].from = from;
         move[*num_moves].target = target;
         move[*num_moves].captured = board[target];
         move[*num_moves].en_passant = 0;
         move[*num_moves].castle = 0;
         move[*num_moves].promoted = bqueen;

         (*num_moves)++;

         /* promotion to rook: */
         move[*num_moves].from = from;
         move[*num_moves].target = target;
         move[*num_moves].captured = board[target];
         move[*num_moves].en_passant = 0;
         move[*num_moves].castle = 0;
         move[*num_moves].promoted = brook;

         (*num_moves)++;

         /* promotion to knight: */
         move[*num_moves].from = from;
         move[*num_moves].target = target;
         move[*num_moves].captured = board[target];
         move[*num_moves].en_passant = 0;
         move[*num_moves].castle = 0;
         move[*num_moves].promoted = bknight;

         (*num_moves)++;

         /* promotion to bishop: */
         move[*num_moves].from = from;
         move[*num_moves].target = target;
         move[*num_moves].captured = board[target];
         move[*num_moves].en_passant = 0;
         move[*num_moves].castle = 0;
         move[*num_moves].promoted = bbishop;

         (*num_moves)++;

         return 0;
      }


      /* the stuff in the else statement simply makes sure that a sliding
      piece won't go further than the first piece it slides into */

      else {
         move[*num_moves].from = from;
         move[*num_moves].target = target;
         move[*num_moves].captured = board[target];
         move[*num_moves].en_passant = 0;
         move[*num_moves].castle = 0;
         move[*num_moves].promoted = 0;

         (*num_moves)++;

         return 0;
      }
   }

   /* look at white promotion moves done by moving up one: */

   if ((board[from] == wpawn) && (rank(from) == 7)) {

      /* promotion to queen: */
      move[*num_moves].from = from;
      move[*num_moves].target = target;
      move[*num_moves].captured = board[target];
      move[*num_moves].en_passant = 0;
      move[*num_moves].castle = 0;
      move[*num_moves].promoted = wqueen;

      (*num_moves)++;

      /* promotion to rook: */
      move[*num_moves].from = from;
      move[*num_moves].target = target;
      move[*num_moves].captured = board[target];
      move[*num_moves].en_passant = 0;
      move[*num_moves].castle = 0;
      move[*num_moves].promoted = wrook;

      (*num_moves)++;

      /* promotion to knight: */
      move[*num_moves].from = from;
      move[*num_moves].target = target;
      move[*num_moves].captured = board[target];
      move[*num_moves].en_passant = 0;
      move[*num_moves].castle = 0;
      move[*num_moves].promoted = wknight;

      (*num_moves)++;

      /* promotion to bishop: */
      move[*num_moves].from = from;
      move[*num_moves].target = target;
      move[*num_moves].captured = board[target];
      move[*num_moves].en_passant = 0;
      move[*num_moves].castle = 0;
      move[*num_moves].promoted = wbishop;

      (*num_moves)++;

      return 1;
   }

   /* look at black promotion moves done by moving down one: */

   else if ((board[from] == bpawn) && (rank(from) == 2)) {

      /* promotion to queen: */
      move[*num_moves].from = from;
      move[*num_moves].target = target;
      move[*num_moves].captured = board[target];
      move[*num_moves].en_passant = 0;
      move[*num_moves].castle = 0;
      move[*num_moves].promoted = bqueen;

      (*num_moves)++;

      /* promotion to rook: */
      move[*num_moves].from = from;
      move[*num_moves].target = target;
      move[*num_moves].captured = board[target];
      move[*num_moves].en_passant = 0;
      move[*num_moves].castle = 0;
      move[*num_moves].promoted = brook;

      (*num_moves)++;

      /* promotion to knight: */
      move[*num_moves].from = from;
      move[*num_moves].target = target;
      move[*num_moves].captured = board[target];
      move[*num_moves].en_passant = 0;
      move[*num_moves].castle = 0;
      move[*num_moves].promoted = bknight;

      (*num_moves)++;

      /* promotion to bishop: */
      move[*num_moves].from = from;
      move[*num_moves].target = target;
      move[*num_moves].captured = board[target];
      move[*num_moves].en_passant = 0;
      move[*num_moves].castle = 0;
      move[*num_moves].promoted = bbishop;

      (*num_moves)++;

      return 1;

   }

   /* now, look at a "normal" non-capture move: */
   else if (!captures) {
      move[*num_moves].from = from;
      move[*num_moves].target = target;
      move[*num_moves].captured = board[target];
      move[*num_moves].en_passant = 0;
      move[*num_moves].castle = 0;
      move[*num_moves].promoted = 0;

      (*num_moves)++;

      return 1;
   }

   return 1;  /* even though we don't want to look at non-capture moves, we
                 still want to see if a sliding piece will be able to make a
                 capture.  Even so, the non-capture parts of the slide are
                 not added to the move arrays. */

}

int make_move(move_s move[], int e) {

   int a, b;
   int from, target, captured, en_passant, promoted, castle;

   from = move[e].from;
   target = move[e].target;
   captured = move[e].captured;
   en_passant = move[e].en_passant;
   promoted = move[e].promoted;
   castle = move[e].castle;

   /* modify general piece / square information.  Special moves such as
      castling and en_passant will be modified a bit later in make_move() */

   move[e].captured_number = squares[target];
   pieces[squares[from]] = target;
   pieces[squares[target]] = 0;
   squares[target] = squares[from];
   squares[from] = 0;

   /* update fifty move info: */
   fifty_move[ply] = fifty;
   if (board[from] == wpawn || board[from] == bpawn || squares[target])
      fifty = 0;
   else
      fifty++;

   /* remove any "ep pieces" that are still on the board. en_passanta
      covers ability to do ep moves! */
   for (a = 50; a <= 57; a++) {
      if (board[a] == wep_piece) {
         board[a] = npiece;
      }
   }


   for (b = 86; b <= 93; b++) {
      if (board[b] == bep_piece) {
         board[b] = npiece;
      }
   }

   /* now, if the move made moves a pawn up two, this is where we place
         the "en passant piece" : */

   if ((board[from] == wpawn) && (target == from + 24)) {
      board[from + 12] = wep_piece;
   }

   else if ((board[from] == bpawn) && (target == from - 24)) {
      board[from - 12] = bep_piece;
   }

   /* check to see if the move is a white kingside castling move: */
   if (castle == wck) {
      /* we only have to move rook, king move is handled by the last
         section of this function */
      board[33] = npiece;
      board[31] = wrook;
      moved[33]++;
      moved[31]++;
      white_castled = TRUE;
      pieces[squares[33]] = 31;
      squares[31] = squares[33];
      squares[33] = 0;
   }

   /* check to see if the move is a white queenside castling move: */
   else if (castle == wcq) {
      /* we only have to move rook, king move is handled by the last
         section of this function */
      board[26] = npiece;
      board[29] = wrook;
      moved[26]++;
      moved[29]++;
      white_castled = TRUE;
      pieces[squares[26]] = 29;
      squares[29] = squares[26];
      squares[26] = 0;
   }

   /* check to see if the move is a black kingside castling move: */
   else if (castle == bck) {
      /* we only have to move rook, king move is handled by the last
         section of this function */
      board[117] = npiece;
      board[115] = brook;
      moved[117]++;
      moved[115]++;
      black_castled = TRUE;
      pieces[squares[117]] = 115;
      squares[115] = squares[117];
      squares[117] = 0;
   }

   /* check to see if the move is a black queenside castling move: */
   else if (castle == bcq) {
      /* we only have to move rook, king move is handled by the last
         section of this function */
      board[110] = npiece;
      board[113] = brook;
      moved[110]++;
      moved[113]++;
      black_castled = TRUE;
      pieces[squares[110]] = 113;
      squares[113] = squares[110];
      squares[110] = 0;
   }

   /* check to see if the move is an en passant move: */
   else if (en_passant == 1) {
      if (board[from] == wpawn) {
         board[target] = board[from];
         board[from] = npiece;
         board[target - 12] = npiece;
         moved[target]++;
         moved[from]++;
         moved[target - 12]++;
         move[e].captured_number = squares[target - 12];
         pieces[squares[target - 12]] = 0;
         squares[target - 12] = 0;
      }

      else if (board[from] == bpawn) {
         board[target] = board[from];
         board[from] = npiece;
         board[target + 12] = npiece;
         moved[target]++;
         moved[from]++;
         moved[target + 12]++;
         move[e].captured_number = squares[target + 12];
         pieces[squares[target + 12]] = 0;
         squares[target + 12] = 0;
      }

      #ifdef lines
      line[line_depth][0] = from;
      line[line_depth][1] = target;
      #endif

      white_to_move += 1;

      return 1;
   }

   /* check for a promotion move: */

   if (promoted != 0) {
      board[target] = promoted;
      board[from] = npiece;
      moved[target]++;
      moved[from]++;
   }

   else {

      /* now, the "normal" part of special moves, the king move in
         castling, and normal moves! ;) */

      if (board[from] == wking) wking_loc = target;
      else if (board[from] == bking) bking_loc = target;
      board[target] = board[from];
      board[from] = npiece;
      moved[target]++;
      moved[from]++;

   }

   #ifdef lines
   line[line_depth][0] = from;
   line[line_depth][1] = target;
   #endif

   white_to_move += 1;

   return 1;
}


int unmake_move(move_s move[], int e) {

   int from, target, captured, en_passant, promoted, castle, captured_number;

   from = move[e].from;
   target = move[e].target;
   captured = move[e].captured;
   en_passant = move[e].en_passant;
   promoted = move[e].promoted;
   castle = move[e].castle;
   captured_number = move[e].captured_number;

   /* undo the changes made to piece / square tables by make_move() for
      general moves: */

   squares[from] = squares[target];
   squares[target] = captured_number; /* rectify this for ep in ep section */
   pieces[squares[target]] = target;
   pieces[squares[from]] = from;

   /* update the fifty move rule information: */
   fifty = fifty_move[ply];

   /* check to see if the move is a white kingside castling move: */
   if (castle == wck) {
      /* we only have to move rook, king move is handled by the last
         else statement */
      board[31] = npiece;
      board[33] = wrook;
      moved[31]--;
      moved[33]--;
      white_castled = FALSE;
      squares[33] = squares[31];
      squares[31] = 0;
      pieces[squares[33]] = 33;
   }

   /* check to see if the move is a white queenside castling move: */
   else if (castle == wcq) {
      /* we only have to move rook, king move is handled by the last
         else statement */
      board[29] = npiece;
      board[26] = wrook;
      moved[29]--;
      moved[26]--;
      white_castled = FALSE;
      squares[26] = squares[29];
      squares[29] = 0;
      pieces[squares[26]] = 26;
   }

   /* check to see if the move is a black kingside castling move: */
   else if (castle == bck) {
      /* we only have to move rook, king move is handled by the last
         else statement */
      board[115] = npiece;
      board[117] = brook;
      moved[115]--;
      moved[117]--;
      black_castled = FALSE;
      squares[117] = squares[115];
      squares[115] = 0;
      pieces[squares[117]] = 117;
   }

   /* check to see if the move is a black queenside castling move: */
   else if (castle == bcq) {
      /* we only have to move rook, king move is handled by the last
         else statement */
      board[113] = npiece;
      board[110] = brook;
      moved[113]--;
      moved[110]--;
      black_castled = FALSE;
      squares[110] = squares[113];
      squares[113] = 0;
      pieces[squares[110]] = 110;
   }

   /* check to see if the move we have to undo was an en passant move: */
   if (en_passant == 1) {
      if (board[target] == wpawn) {
         board[from] = board[target];
         board[target] = npiece; /* don't need to replace "ep pieces"
                                    due to en_passant! */
         board[target - 12] = bpawn; /* replace the pawn taken */
         moved[from]--;
         moved[target]--;
         moved[target - 12]--;
         squares[target - 12] = captured_number;
         pieces[captured_number] = target - 12;
         squares[target] = 0;
      }

      if (board[target] == bpawn) {
         board[from] = board[target];
         board[target] = npiece; /* don't need to replace "ep pieces"
                                    due to en_passanta! */
         board[target + 12] = wpawn; /* replace the pawn taken ep */
         moved[from]--;
         moved[target]--;
         moved[target + 12]--;
         squares[target + 12] = captured_number;
         pieces[captured_number] = target + 12;
         squares[target] = 0;
      }
   }

   /* check to see if we have to undo a white promotion: */

   else if ((promoted %2) == 1) {
      board[from] = wpawn;
      board[target] = captured;
      moved[from]--;
      moved[target]--;
   }

   /* check to see if we have to undo a black promotion: */

   else if (((promoted %2) == 0) && (promoted != 0)) {
      board[from] = bpawn;
      board[target] = captured;
      moved[from]--;
      moved[target]--;
   }

   else {
      if (board[target] == wking) wking_loc = from;
      else if (board[target] == bking) bking_loc = from;
      board[from] = board[target];
      board[target] = captured;
      moved[from]--;
      moved[target]--;
   }

   white_to_move -= 1;

   return 1;
}

Bool is_attacked(int location, int color) {

   /* this function will determine if square location is attacked by side
      designated by color.  Returns a 1 if the square is under attack, and
      0 if it is not. */

   int queen_offsets[8] = {12, -12, -1, 1, 13, -13, 11, -11};
   int knight_offsets[8] = {10, -10, 14, -14, 23, -23, 25, -25};
   int a, target;

   /* loops for queen */

   /* rook style moves: */
   for (a = 0; a < 4; a++) {
      if ((color %2) == 0) {
         target = location + queen_offsets[a];
         if (board[target] == bking) return 1;
         while (board[target] != frame) {
            if ((board[target]==brook) || (board[target]==bqueen)) return 1;
            else if ((board[target] != npiece) && (board[target] != 15) &&
                    (board[target] != 16)) break;
            target += queen_offsets[a];
         }
      }
      else if ((color %2) == 1) {
         target = location + queen_offsets[a];
         if (board[target] == wking) return 1;
         while (board[target] != frame) {
            if ((board[target]==wrook) || (board[target]==wqueen)) return 1;
            else if ((board[target] != npiece) && (board[target] != 15) &&
                    (board[target] != 16)) break;
            target += queen_offsets[a];
         }
      }
   }

   /* bishop style moves: */
   for (a = 4; a < 8; a++) {
      if ((color %2) == 0) {
         target = location + queen_offsets[a];
         if ((board[target] == bpawn) && ((a == 4) || (a == 6))) return 1;
         if (board[target] == bking) return 1;
         while (board[target] != frame) {
            if ((board[target]==bbishop) || (board[target]==bqueen)) return 1;
            else if ((board[target] != npiece) && (board[target] != 15) &&
                    (board[target] != 16)) break;
            target += queen_offsets[a];
         }
      }
      else if ((color %2) == 1) {
         target = location + queen_offsets[a];
         if ((board[target] == wpawn) && ((a == 5) || (a == 7))) return 1;
         if (board[target] == wking) return 1;
         while (board[target] != frame) {
            if ((board[target]==wbishop) || (board[target]==wqueen)) return 1;
            else if ((board[target] != npiece) && (board[target] != 15) &&
                    (board[target] != 16)) break;
            target += queen_offsets[a];
         }
      }

   }

   /* loop for knight */

   for (a = 0; a < 8; a++) {
      target = location + knight_offsets[a];
      if ((color %2) == 0) {
         if (board[target] == bknight) return 1;
      }
      else {
         if (board[target] == wknight) return 1;
      }
   }

   /* if we have not had a return 1 by now, there is no attacker of side
      color aimed at square location */

   return 0;

}

int check_legal(move_s move[], int a) {

   /* function to be used after make_move to determine if a move which was
      made was legal (ie. the player who last moved is not still in check,
      or has not castled through check).  Will return 0 if move is illegal,
      1 if move was legal.
      Incidentally, if the player who was to move has no moves, then if he is
      in check, he is mated, if he's not in check, he's stalemated.  Whether
      he's in check or not for the purposes of finding mate/stalemate will be
      made after unmake_move via is_attacked() */

   /* check about white kingside castling legality: */
   if (move[a].castle == wck) {
      if (is_attacked(30, 2)) return 0;
      else if (is_attacked(31, 2)) return 0;
      else if (is_attacked(32, 2)) return 0;
      else return 1;
   }

   /* check about white queenside castling legality: */
   else if (move[a].castle == wcq) {
      if (is_attacked(30, 2)) return 0;
      else if (is_attacked(29, 2)) return 0;
      else if (is_attacked(28, 2)) return 0;
      else return 1;
   }

   /* check about black kingside castling legality: */
   else if (move[a].castle == bck) {
      if (is_attacked(114, 1)) return 0;
      else if (is_attacked(115, 1)) return 0;
      else if (is_attacked(116, 1)) return 0;
      else return 1;
   }

   /* check about black queenside castling legality: */
   else if (move[a].castle == bcq) {
      if (is_attacked(114, 1)) return 0;
      else if (is_attacked(113, 1)) return 0;
      else if (is_attacked(112, 1)) return 0;
      else return 1;
   }

   /* check to see if white is in check, if he just made a move: */
   else if ((white_to_move %2) == 0) {
      if (is_attacked(wking_loc, 2)) return 0;
      else return 1;
   }

   /* check to see if black is in check, if he just made a move: */
   else if ((white_to_move %2) == 1) {
      if (is_attacked(bking_loc, 1)) return 0;
      else return 1;
   }

   /* if anything wasn't covered by those cases, we have an error: */
   return -1;
}

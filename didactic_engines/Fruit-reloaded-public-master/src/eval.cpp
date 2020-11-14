/* Fruit reloaded, a UCI chess playing engine derived from Fruit 2.1
 *
 * Copyright (C) 2004-2005 Fabien Letouzey
 * Copyright (C) 2012-2014 Daniel Mehrmann
 * Copyright (C) 2013-2014 Ryan Benitez
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

// eval.cpp

// includes

#include "attack.h"
#include "bit.h"
#include "board.h"
#include "colour.h"
#include "eval.h"
#include "list.h"
#include "material.h"
#include "move.h"
#include "move_gen.h"
#include "option.h"
#include "pawn.h"
#include "piece.h"
#include "pst.h"
#include "see.h"
#include "smp.h"
#include "util.h"
#include "value.h"
#include "vector.h"

// macros

#define THROUGH(piece)  ((piece)==Empty)

#define NO_DEFENDER(to,pawn) ((Bit[SQUARE_TO_64((to))] & (pawn))==0)
#define NO_ATTACKER(to,pawn) ((Bit[SQUARE_TO_64((to))] & (pawn))==0)

// constants and variables

static int PieceActivityWeight = 256; // 100%
static int KingSafetyWeight = 256;
static int PawnActivityWeight = 256;

// mobility options

static const bool UseMobLinear = true;   // Linear or table based mobility
static const bool UseMobXRay = true;     // X-Ray attack through mobility
static const bool UseMobPawnSafe = true; // Pawn safe mobility

static const int KnightUnit = 3; // was 4
static const int BishopUnit = 4; // was 6
static const int RookUnit = 5;   // was 7
static const int QueenUnit = 9;  // was 13

static const int MobMove = 1;
static const int MobAttack = 1;
static const int MobDefense = 0;

static const bool UseMobAttack = true;
static const int MobAttackPawn = 2;
static const int MobAttackKnight = 5;
static const int MobAttackBishop = 5;
static const int MobAttackRook = 10;
static const int MobAttackQueen = 20;
static const int MobAttackKing = 40; // obsolete

static const int KnightMobOpening = 4;
static const int KnightMobEndgame = 4;
static const int BishopMobOpening = 5;
static const int BishopMobEndgame = 5;
static const int RookMobOpening = 2;
static const int RookMobEndgame = 4;
static const int QueenMobOpening = 1;
static const int QueenMobEndgame = 2;
static const int KingMobOpening = 0;
static const int KingMobEndgame = 0;

// rook options

static const bool UseOpenFile = true;
static const int RookSemiOpenFileOpening = 10;
static const int RookSemiOpenFileEndgame = 10;
static const int RookOpenFileOpening = 20;
static const int RookOpenFileEndgame = 20;
static const int RookSemiKingFileOpening = 10;
static const int RookKingFileOpening = 20;

// king attack options

static const bool UseKingAttack = true;
static const int KingAttackOpening = 20;

// pawn options

static const bool UsePawnAttack = true;
static const bool UsePawnMobility = false;

// shelter options

static const bool UseShelter = true;
static const int ShelterOpening = 256; // 100%
static const bool UseStorm = true;
static const int StormOpening = 10;

// deep rank options

static const int Rook7thOpening = 20;
static const int Rook7thEndgame = 40;
static const int Queen7thOpening = 10;
static const int Queen7thEndgame = 20;

// eval values

static const int TrappedBishop = 100;

static const int BlockedBishop = 50;
static const int BlockedRook = 50;

static const int SideToMoveOpening = 10;
static const int SideToMoveEndgame = 10;

static const int PassedOpeningMin = 10;
static const int PassedOpeningMax = 70;
static const int PassedEndgameMin = 20;
static const int PassedEndgameMax = 140;

static const int UnstoppablePasser = 800;
static const int FreePasser = 60;

static const int AttackerDistance = 5;
static const int DefenderDistance = 20;

// "constants"

static const int ValueDraw = 0;

static const uint64 PawnAttWhiteL = 0xFFFF7F7F7F7F7F7FULL;
static const uint64 PawnAttWhiteR = 0x00FEFEFEFEFEFEFEULL;
static const uint64 PawnAttBlackL = 0x3FBFBFBFBFBFBF80ULL;
static const uint64 PawnAttBlackR = 0x7F7F7F7F7F7F7F00ULL;

static const int MobKnight[StageNb][16] = { // only 8 are needed
   {-16, -8, -4, +0, +4, +8,+11,+13,+14,+14,+14,+14,+14,+14,+14,+14 },
   {-14, -7, -3, +0, +3, +6, +8, +9,+10,+10,+10,+14,+14,+14,+14,+14 },
};

static const int MobBishop[StageNb][16] = { // only 13 are needed
   {-20,-15,-10, -5, +0, +5, +9,+12,+14,+15,+16,+17,+18,+19,+19,+19 },
   {-22,-17,-12, -7, -2, +3, +7,+10,+12,+13,+14,+15,+16,+17,+18,+19 },
};

static const int MobRook[StageNb][16] = { // only 14 are needed
   {-10, -8, -6, -4, -2, +0, +2, +4, +5, +6, +7, +7, +7, +7, +7, +7 },
   {-20,-16,-12, -8, -4, +0, +4, +8,+12,+15,+17,+18,+19,+20,+21,+22 },
};

static const int MobQueen[StageNb][32] = { // only 27 are needed
   { -6, -5, -4, -3, -2, -1, +0, +1, +2, +3, +4, +4, +5, +5, +6, +6,
     +7, +7, +7, +7, +7, +7, +7, +7, +7, +7, +7, +7, +7, +7, +7, +7 },

   {-12, -10, -8 -6, -4, -2, +0, +2, +4, +6, +7, +8, +9,+10,+11,+11,
    +12,+12,+12,+12,+12,+12,+12,+12,+12,+12,+12,+12,+12,+12,+12,+12 },
};

static const int KingAttackWeight[16] = {
   0, 0, 128, 192, 224, 240, 248, 252, 254, 255, 256, 256 ,256, 256, 256, 256,
};

static const int MajLine[8] = { // major
   +25, +25, +15, +5, +5, +15, +25, +25, // TODO: tune me!
};

static const int MinLine[8] = { // minor
   +35, +25, +15, +5, +5, +15, +25, +35, // TODO: tune me!
};

static const inc_t KnightVector[8] = {
   -33, -31, -18, -14, +14, +18, +31, +33,
};

static const inc_t BishopVector[4] = {
   -17, -15, +15, +17,
};

static const inc_t RookVector[4] = {
   -16,  -1,  +1, +16,
};

static const inc_t QueenVector[8] = {
   -17, -16, -15,  -1,  +1, +15, +16, +17,
};

// variables

static int MobUnit[ColourNb][PieceNb];

static int KingAttackUnit[PieceNb];

static int MajPst[64]; // major
static int MinPst[64]; // minor
static int NBPst[64];  // knight/bishop

uint64 Bit[64]; // HACK: declaration bit.h TODO: move me ?

// prototypes

static void eval_draw          (const board_t * board, const material_info_t * mat_info, const pawn_info_t * pawn_info, int mul[2]);

static bool eval_win           (const board_t * board, const material_info_t * mat_info, int * opening, int * endgame);

static void eval_piece         (const board_t * board, const material_info_t * mat_info, const pawn_info_t * pawn_info, const uint64 pawn_attacks[], int * opening, int * endgame);
static void eval_king          (const board_t * board, const material_info_t * mat_info, int * opening, int * endgame);
static void eval_pawn          (const board_t * board, const pawn_info_t * pawn_info, const uint64 pawn_attacks[], int * opening, int * endgame);
static void eval_pattern       (const board_t * board, int * opening, int * endgame);
static void eval_promote       (const board_t * board, int * opening, int * endgame);
static void eval_tempo         (const board_t * board, int * opening, int * endgame);

static bool unstoppable_passer (const board_t * board, int pawn, int colour);
static bool king_passer        (const board_t * board, int pawn, int colour);
static bool free_passer        (const board_t * board, int pawn, int colour);

static int  pawn_att_dist      (int pawn, int king, int colour);
static int  pawn_def_dist      (int pawn, int king, int colour);

static void draw_init_list     (int list[], const board_t * board, int pawn_colour);

static bool draw_kpkq          (const int list[], int turn);
static bool draw_kpkr          (const int list[], int turn);
static bool draw_kpkb          (const int list[], int turn);
static bool draw_kpkn          (const int list[], int turn);

static bool draw_knpk          (const int list[], int turn);

static bool draw_krpkr         (const int list[], int turn);
static bool draw_kbpkb         (const int list[], int turn);

static int  win_kxk            (const board_t * board, int colour);
static int  win_kxkx           (const board_t * board, int colour);
static int  win_knbk           (const board_t * board, int colour);
static int  win_kbbk           (const board_t * board, int colour);

static int  shelter_square     (const board_t * board, int square, int colour);
static int  shelter_file       (const board_t * board, int file, int rank, int colour);

static int  storm_file         (const board_t * board, int file, int colour);

static bool bishop_can_attack  (const board_t * board, int to, int colour);

// functions

// eval_init()

void eval_init() {

   int colour;
   int piece;
   int sq;
   int file, rank;

   // UCI options

   eval_init_options();

   // Bit[] TODO: move me ?

   for (sq = 0; sq < 64; sq++) {
      Bit[sq] = U64(1) << sq;
   }

   // mobility table

   for (colour = 0; colour < ColourNb; colour++) {
      for (piece = 0; piece < PieceNb; piece++) {
         MobUnit[colour][piece] = 0;
      }
   }

   MobUnit[White][Empty] = MobMove;

   MobUnit[White][BP] = MobAttackPawn;
   MobUnit[White][BN] = MobAttackKnight;
   MobUnit[White][BB] = MobAttackBishop;
   MobUnit[White][BR] = MobAttackRook;
   MobUnit[White][BQ] = MobAttackQueen;
   MobUnit[White][BK] = MobAttackKing;

   MobUnit[White][WP] = MobDefense;
   MobUnit[White][WN] = MobDefense;
   MobUnit[White][WB] = MobDefense;
   MobUnit[White][WR] = MobDefense;
   MobUnit[White][WQ] = MobDefense;
   MobUnit[White][WK] = MobDefense;

   MobUnit[Black][Empty] = MobMove;

   MobUnit[Black][WP] = MobAttackPawn;
   MobUnit[Black][WN] = MobAttackKnight;
   MobUnit[Black][WB] = MobAttackBishop;
   MobUnit[Black][WR] = MobAttackRook;
   MobUnit[Black][WQ] = MobAttackQueen;
   MobUnit[Black][WK] = MobAttackKing;

   MobUnit[Black][BP] = MobDefense;
   MobUnit[Black][BN] = MobDefense;
   MobUnit[Black][BB] = MobDefense;
   MobUnit[Black][BR] = MobDefense;
   MobUnit[Black][BQ] = MobDefense;
   MobUnit[Black][BK] = MobDefense;

   // KingAttackUnit[]

   for (piece = 0; piece < PieceNb; piece++) {
      KingAttackUnit[piece] = 0;
   }

   KingAttackUnit[WN] = 1;
   KingAttackUnit[WB] = 1;
   KingAttackUnit[WR] = 2;
   KingAttackUnit[WQ] = 4;

   KingAttackUnit[BN] = 1;
   KingAttackUnit[BB] = 1;
   KingAttackUnit[BR] = 2;
   KingAttackUnit[BQ] = 4;

   // MajPst[]

   for (sq = 0; sq < 64; sq++) {
      MajPst[sq] = 0;
   }

   for (sq = 0; sq < 64; sq++) {
      MajPst[sq] += MajLine[(sq & 7)];  // file
      MajPst[sq] += MajLine[(sq >> 3)]; // rank
   }

   // MinPst[]

   for (sq = 0; sq < 64; sq++) {
      MinPst[sq] = 0;
   }

   for (sq = 0; sq < 64; sq++) {
      MinPst[sq] += MinLine[(sq & 7)];  // file
      MinPst[sq] += MinLine[(sq >> 3)]; // rank
   }

   // NBPst[]

   for (sq = 0; sq < 64; sq++) {
      NBPst[sq] = 0;
   }

   for (sq = 0; sq < 64; sq++) {

      file = (sq & 7);
      rank = (sq >> 3);

      if ((((file|rank) ^ 7) & 4) != 0) { // highest vector

         NBPst[sq] += MinLine[file];
         NBPst[sq] += MinLine[rank];

         // vector mirror

         NBPst[(sq ^ 0x07)] += NBPst[sq] / 2; // TODO: tune me !
         NBPst[(sq ^ 0x38)] += NBPst[sq] / 2; // TODO: tune me !
         NBPst[(sq ^ 0x3F)] += NBPst[sq];
      }
   }
}

// eval_init_option()

void eval_init_options() {

   // UCI options

   PieceActivityWeight = (option_get_int("Piece Activity") * 256 + 50) / 100;
   KingSafetyWeight    = (option_get_int("King Safety")    * 256 + 50) / 100;
   PawnActivityWeight  = (option_get_int("Pawn Activity")  * 256 + 50) / 100;
}

// eval()

int eval(const board_t * board, int thread) {

   int opening, endgame;
   uint64 pawn_attacks[ColourNb];
   material_info_t mat_info[1];
   pawn_info_t pawn_info[1];
   int mul[ColourNb];
   int phase;
   int eval;
   int wb, bb;

   ASSERT(board!=NULL);
   ASSERT(thread_is_ok(thread));

   ASSERT(board_is_legal(board));
   ASSERT(!board_is_check(board)); // exceptions are extremely rare

   // init

   opening = 0;
   endgame = 0;

   // pawn attack "bitboard"

   pawn_attacks[White] = (((board->pawn_bitboard[White] & PawnAttWhiteL) << 2) |
                           (board->pawn_bitboard[White] & PawnAttWhiteR)) << 7;
   pawn_attacks[Black] = (((board->pawn_bitboard[Black] >> 2) & PawnAttBlackL) |
                           (board->pawn_bitboard[Black] & PawnAttBlackR)) >> 7;

   // material

   material_get_info(mat_info,board,thread);

   opening += mat_info->opening;
   endgame += mat_info->endgame;

   mul[White] = mat_info->mul[White];
   mul[Black] = mat_info->mul[Black];

   // pawns

   pawn_get_info(pawn_info,board,thread);

   opening += pawn_info->opening;
   endgame += pawn_info->endgame;

   // draw

   eval_draw(board,mat_info,pawn_info,mul);

   if (mat_info->mul[White] < mul[White]) mul[White] = mat_info->mul[White];
   if (mat_info->mul[Black] < mul[Black]) mul[Black] = mat_info->mul[Black];

   if (mul[White] == 0 && mul[Black] == 0) return ValueDraw;

   // win

   if (eval_win(board,mat_info,&opening,&endgame)) goto phase;

   // pst

   opening += board->opening;
   endgame += board->endgame;

   // eval

   eval_piece(board,mat_info,pawn_info,pawn_attacks,&opening,&endgame);
   eval_king(board,mat_info,&opening,&endgame);
   eval_pawn(board,pawn_info,pawn_attacks,&opening,&endgame);
   eval_pattern(board,&opening,&endgame);
   eval_promote(board,&opening,&endgame);
   eval_tempo(board,&opening,&endgame);

phase:

   // phase mix

   phase = mat_info->phase;
   eval = ((opening * (256 - phase)) + (endgame * phase)) / 256;

   // drawish bishop endgames

   if ((mat_info->flags & DrawBishopFlag) != 0) {

      wb = board->piece[White][1];
      ASSERT(PIECE_IS_BISHOP(board->square[wb]));

      bb = board->piece[Black][1];
      ASSERT(PIECE_IS_BISHOP(board->square[bb]));

      if (SQUARE_COLOUR(wb) != SQUARE_COLOUR(bb)) {
         if (mul[White] == 16) mul[White] = 8; // 1/2
         if (mul[Black] == 16) mul[Black] = 8; // 1/2
      }
   }

   // draw bound

   if (eval > ValueDraw) {
      eval = (eval * mul[White]) / 16;
   } else if (eval < ValueDraw) {
      eval = (eval * mul[Black]) / 16;
   }

   // value range

   if (eval < -ValueEvalInf) eval = -ValueEvalInf;
   if (eval > +ValueEvalInf) eval = +ValueEvalInf;

   ASSERT(eval>=-ValueEvalInf&&eval<=+ValueEvalInf);

   // turn

   if (COLOUR_IS_BLACK(board->turn)) eval = -eval;

   ASSERT(!value_is_mate(eval));

   return eval;
}

// eval_draw()

static void eval_draw(const board_t * board, const material_info_t * mat_info, const pawn_info_t * pawn_info, int mul[2]) {

   int colour;
   int me, opp;
   int pawn, king;
   int pawn_file;
   int prom;
   int list[7+1];

   ASSERT(board!=NULL);
   ASSERT(mat_info!=NULL);
   ASSERT(pawn_info!=NULL);
   ASSERT(mul!=NULL);

   // draw patterns

   for (colour = 0; colour < ColourNb; colour++) {

      me = colour;
      opp = COLOUR_OPP(me);

      // KB*P+K* draw

      if ((mat_info->cflags[me] & MatRookPawnFlag) != 0) {

         pawn = pawn_info->single_file[me];

         if (pawn != SquareNone) { // all pawns on one file

            pawn_file = SQUARE_FILE(pawn);

            if (pawn_file == FileA || pawn_file == FileH) {

               king = KING_POS(board,opp);
               prom = PAWN_PROMOTE(pawn,me);

               if (DISTANCE(king,prom) <= 1 && !bishop_can_attack(board,prom,me)) {
                  mul[me] = 0;
               }
            }
         }
      }

      // K(B)P+K+ draw

      if ((mat_info->cflags[me] & MatBishopFlag) != 0) {

         pawn = pawn_info->single_file[me];

         if (pawn != SquareNone) { // all pawns on one file

            king = KING_POS(board,opp);

            if (SQUARE_FILE(king)  == SQUARE_FILE(pawn)
             && PAWN_RANK(king,me) >  PAWN_RANK(pawn,me)
             && !bishop_can_attack(board,king,me)) {
               mul[me] = 1; // 1/16
            }
         }
      }

      // KNPK* draw

      if ((mat_info->cflags[me] & MatKnightFlag) != 0) {

         pawn = board->pawn[me][0];
         king = KING_POS(board,opp);

         if (SQUARE_FILE(king)  == SQUARE_FILE(pawn)
          && PAWN_RANK(king,me) >  PAWN_RANK(pawn,me)
          && PAWN_RANK(pawn,me) <= Rank6) {
            mul[me] = 1; // 1/16
         }
      }
   }

   // recognisers, only heuristic draws here!

   if (false) {

   } else if (mat_info->recog == MAT_KPKQ) {

      // KPKQ (white)

      draw_init_list(list,board,White);

      if (draw_kpkq(list,board->turn)) {
         mul[White] = 1; // 1/16;
         mul[Black] = 1; // 1/16;
      }

   } else if (mat_info->recog == MAT_KQKP) {

      // KPKQ (black)

      draw_init_list(list,board,Black);

      if (draw_kpkq(list,COLOUR_OPP(board->turn))) {
         mul[White] = 1; // 1/16;
         mul[Black] = 1; // 1/16;
      }

   } else if (mat_info->recog == MAT_KPKR) {

      // KPKR (white)

      draw_init_list(list,board,White);

      if (draw_kpkr(list,board->turn)) {
         mul[White] = 1; // 1/16;
         mul[Black] = 1; // 1/16;
      }

   } else if (mat_info->recog == MAT_KRKP) {

      // KPKR (black)

      draw_init_list(list,board,Black);

      if (draw_kpkr(list,COLOUR_OPP(board->turn))) {
         mul[White] = 1; // 1/16;
         mul[Black] = 1; // 1/16;
      }

   } else if (mat_info->recog == MAT_KPKB) {

      // KPKB (white)

      draw_init_list(list,board,White);

      if (draw_kpkb(list,board->turn)) {
         mul[White] = 1; // 1/16;
         mul[Black] = 1; // 1/16;
      }

   } else if (mat_info->recog == MAT_KBKP) {

      // KPKB (black)

      draw_init_list(list,board,Black);

      if (draw_kpkb(list,COLOUR_OPP(board->turn))) {
         mul[White] = 1; // 1/16;
         mul[Black] = 1; // 1/16;
      }

   } else if (mat_info->recog == MAT_KPKN) {

      // KPKN (white)

      draw_init_list(list,board,White);

      if (draw_kpkn(list,board->turn)) {
         mul[White] = 1; // 1/16;
         mul[Black] = 1; // 1/16;
      }

   } else if (mat_info->recog == MAT_KNKP) {

      // KPKN (black)

      draw_init_list(list,board,Black);

      if (draw_kpkn(list,COLOUR_OPP(board->turn))) {
         mul[White] = 1; // 1/16;
         mul[Black] = 1; // 1/16;
      }


   } else if (mat_info->recog == MAT_KNPK) {

      // KNPK (white)

      draw_init_list(list,board,White);

      if (draw_knpk(list,board->turn)) {
         mul[White] = 1; // 1/16;
         mul[Black] = 1; // 1/16;
      }

   } else if (mat_info->recog == MAT_KKNP) {

      // KNPK (black)

      draw_init_list(list,board,Black);

      if (draw_knpk(list,COLOUR_OPP(board->turn))) {
         mul[White] = 1; // 1/16;
         mul[Black] = 1; // 1/16;
      }

   } else if (mat_info->recog == MAT_KRPKR) {

      // KRPKR (white)

      draw_init_list(list,board,White);

      if (draw_krpkr(list,board->turn)) {
         mul[White] = 1; // 1/16;
         mul[Black] = 1; // 1/16;
      }

   } else if (mat_info->recog == MAT_KRKRP) {

      // KRPKR (black)

      draw_init_list(list,board,Black);

      if (draw_krpkr(list,COLOUR_OPP(board->turn))) {
         mul[White] = 1; // 1/16;
         mul[Black] = 1; // 1/16;
      }

   } else if (mat_info->recog == MAT_KBPKB) {

      // KBPKB (white)

      draw_init_list(list,board,White);

      if (draw_kbpkb(list,board->turn)) {
         mul[White] = 1; // 1/16;
         mul[Black] = 1; // 1/16;
      }

   } else if (mat_info->recog == MAT_KBKBP) {

      // KBPKB (black)

      draw_init_list(list,board,Black);

      if (draw_kbpkb(list,COLOUR_OPP(board->turn))) {
         mul[White] = 1; // 1/16;
         mul[Black] = 1; // 1/16;
      }
   }
}

static bool eval_win(const board_t * board, const material_info_t * mat_info, int * opening, int * endgame) {

   int value;

   ASSERT(board!=NULL);
   ASSERT(mat_info!=NULL);
   ASSERT(opening!=NULL);
   ASSERT(endgame!=NULL);

   // init

   value = 0;

   // recognisers, only heuristic win here!

   if (false) {

   } else if (mat_info->recog == MAT_KRK) {

      // KRK (white)

      value = win_kxk(board,White);
      ASSERT(value>ValueWin&&value<ValueEvalInf);

   } else if (mat_info->recog == MAT_KKR) {

      // KRK (black)

      value = win_kxk(board,Black);
      ASSERT(value<-ValueWin&&value>-ValueEvalInf);

   } else if (mat_info->recog == MAT_KQK) {

      // KQK (white)

      value = win_kxk(board,White);
      ASSERT(value>ValueWin&&value<ValueEvalInf);

   } else if (mat_info->recog == MAT_KKQ) {

      // KQK (black)

      value = win_kxk(board,Black);
      ASSERT(value<-ValueWin&&value>-ValueEvalInf);

   } else if (mat_info->recog == MAT_KQKR) {

      // KQKR (white)

      value = win_kxkx(board,White);
      ASSERT(value>0&&value<200);

   } else if (mat_info->recog == MAT_KRKQ) {

      // KQKR (black)

      value = win_kxkx(board,Black);
      ASSERT(value>-200&&value<0);

   } else if (mat_info->recog == MAT_KRKB) {

      // KRKB (white)

      value = win_kxkx(board,White);
      ASSERT(value>0&&value<200);

   } else if (mat_info->recog == MAT_KBKR) {

      // KRKB (black)

      value = win_kxkx(board,Black);
      ASSERT(value>-200&&value<0);

   } else if (mat_info->recog == MAT_KRKN) {

      // KRKN (white)

      value = win_kxkx(board,White);
      ASSERT(value>0&&value<200);

   } else if (mat_info->recog == MAT_KNKR) {

      // KRKN (black)

      value = win_kxkx(board,Black);
      ASSERT(value>-200&&value<0);

   } else if (mat_info->recog == MAT_KNBK) {

      // KNBK(white)

      value = win_knbk(board,White);
      ASSERT(value>ValueWin&&value<ValueEvalInf);

   } else if (mat_info->recog == MAT_KKNB) {

      // KNBK(black)

      value = win_knbk(board,Black);
      ASSERT(value<-ValueWin&&value>-ValueEvalInf);

   } else if (mat_info->recog == MAT_KBBK) {

      // KBBK(white)

      value = win_kbbk(board,White);
      ASSERT(value==0||(value>ValueWin&&value<ValueEvalInf));

   } else if (mat_info->recog == MAT_KKBB) {

      // KBBK(black)

      value = win_kbbk(board,Black);
      ASSERT(value==0||(value<-ValueWin&&value>-ValueEvalInf));
   }

   // update

   ASSERT(!value_is_mate(value));

   *opening += value;
   *endgame += value;

   return (value != 0);
}

// eval_piece()

static void eval_piece(const board_t * board, const material_info_t * mat_info, const pawn_info_t * pawn_info, const uint64 pawn_attacks[], int * opening, int * endgame) {

   int colour;
   int op[ColourNb], eg[ColourNb];
   int me, opp;
   const sq_t * ptr;
   int i;
   int from, to;
   int piece;
   int mob;
   int capture;
   const int * unit;
   int rook_file, king_file;
   int king;
   int delta;
   int att;
   inc_t inc;

   ASSERT(board!=NULL);
   ASSERT(mat_info!=NULL);
   ASSERT(pawn_info!=NULL);
   ASSERT(pawn_attacks!=NULL);
   ASSERT(opening!=NULL);
   ASSERT(endgame!=NULL);

   // init

   for (colour = 0; colour < ColourNb; colour++) {
      op[colour] = 0;
      eg[colour] = 0;
   }

   // eval

   for (colour = 0; colour < ColourNb; colour++) {

      me = colour;
      opp = COLOUR_OPP(me);
      unit = MobUnit[me];

      const uint64 pawn_safe = pawn_attacks[opp];

      // piece loop

      for (ptr = &board->piece[me][1]; (from=*ptr) != SquareNone; ptr++) { // HACK: no king

         piece = board->square[from];
         att = 0;
         mob = 0;

         switch (PIECE_TYPE(piece)) {

         case Knight64:

            // mobility

            for (i = 0; i < 8; i++) {

               to = from + KnightVector[i];

               if ((capture = unit[board->square[to]]) > MobDefense) {
                  if (!UseMobPawnSafe || NO_ATTACKER(to,pawn_safe)) mob++;
                  if (UseMobAttack && capture > MobAttack) {
                     if (NO_DEFENDER(to,pawn_safe)) att += capture;
                  }
               }
            }

            if (UseMobLinear) {
               op[me] += (mob - KnightUnit) * KnightMobOpening;
               eg[me] += (mob - KnightUnit) * KnightMobEndgame;
            } else {
               op[me] += MobKnight[Opening][mob];
               eg[me] += MobKnight[Endgame][mob];
            }

            if (UseMobAttack) {
               op[me] += att - MobAttackPawn;
               eg[me] += (att * 2) - MobAttackPawn;
            }

            break;

         case Bishop64:

            // mobility

            for (i = 0; i < 4; i++) {

               inc = BishopVector[i];

               for (to = from+inc; capture=board->square[to], THROUGH(capture); to += inc) {
                  if (!UseMobPawnSafe || NO_ATTACKER(to,pawn_safe)) mob += MobMove;
               }

               if ((piece = unit[capture]) > MobDefense) { // MobAttack
                  mob += MobAttack;
                  if (UseMobAttack && NO_DEFENDER(to,pawn_safe)) att += piece;

               } else if (UseMobXRay && (capture & BishopFlag) != 0) { // MobXRay
                  do {
                     ASSERT(COLOUR_IS(capture,me));
                     for (to += inc; capture=board->square[to], THROUGH(capture); to += inc) {
                        if (!UseMobPawnSafe || NO_ATTACKER(to,pawn_safe)) mob += MobMove;
                     }
                  } while ((capture & BishopFlag) != 0 && PIECE_COLOUR(capture) == me);
               }
            }

            if (UseMobLinear) {
               op[me] += (mob - BishopUnit) * BishopMobOpening;
               eg[me] += (mob - BishopUnit) * BishopMobEndgame;
            } else {
               op[me] += MobBishop[Opening][mob];
               eg[me] += MobBishop[Endgame][mob];
            }

            if (UseMobAttack) {
               op[me] += att - MobAttackPawn;
               eg[me] += (att * 2) - MobAttackPawn;
            }

            break;

         case Rook64:

            // mobility

            for (i = 0; i < 4; i++) {

               inc = RookVector[i];

               for (to = from+inc; capture=board->square[to], THROUGH(capture); to += inc) {
                  if (!UseMobPawnSafe || NO_ATTACKER(to,pawn_safe)) mob += MobMove;
               }

               if ((piece = unit[capture]) > MobDefense) { // MobAttack
                  mob += MobAttack;
                  if (UseMobAttack && NO_DEFENDER(to,pawn_safe)) att += piece;

               } else if (UseMobXRay && (capture & RookFlag) != 0) { // MobXRay
                  do {
                     ASSERT(COLOUR_IS(capture,me));
                     for (to += inc; capture=board->square[to], THROUGH(capture); to += inc) {
                        if (!UseMobPawnSafe || NO_ATTACKER(to,pawn_safe)) mob += MobMove;
                     }
                  } while ((capture & RookFlag) != 0 && PIECE_COLOUR(capture) == me);
               }
            }

            if (UseMobLinear) {
               op[me] += (mob - RookUnit) * RookMobOpening;
               eg[me] += (mob - RookUnit) * RookMobEndgame;
            } else {
               op[me] += MobRook[Opening][mob];
               eg[me] += MobRook[Endgame][mob];
            }

            if (UseMobAttack) {
               op[me] += att - MobAttackPawn;
               eg[me] += (att * 2) - MobAttackPawn;
            }

            // open file

            if (UseOpenFile) {

               op[me] -= RookOpenFileOpening / 2;
               eg[me] -= RookOpenFileEndgame / 2;

               rook_file = SQUARE_FILE(from);

               if (board->pawn_file[me][rook_file] == 0) { // no friendly pawn

                  op[me] += RookSemiOpenFileOpening;
                  eg[me] += RookSemiOpenFileEndgame;

                  if (board->pawn_file[opp][rook_file] == 0) { // no enemy pawn
                     op[me] += RookOpenFileOpening - RookSemiOpenFileOpening;
                     eg[me] += RookOpenFileEndgame - RookSemiOpenFileEndgame;
                  }

                  if ((mat_info->cflags[me] & MatKingFlag) != 0) {

                     king = KING_POS(board,opp);
                     king_file = SQUARE_FILE(king);

                     delta = ABS(rook_file-king_file); // file distance

                     if (delta <= 1) {
                        op[me] += RookSemiKingFileOpening;
                        if (delta == 0) op[me] += RookKingFileOpening - RookSemiKingFileOpening;
                     }
                  }
               }
            }

            // 7th rank

            if (PAWN_RANK(from,me) == Rank7) {
               if ((pawn_info->flags[opp] & BackRankFlag) != 0 // opponent pawn on 7th rank
                || PAWN_RANK(KING_POS(board,opp),me) == Rank8) {
                  op[me] += Rook7thOpening;
                  eg[me] += Rook7thEndgame;
               }
            }

            break;

         case Queen64:

            // mobility

            for (int i = 0; i < 8; i++) {

               inc = QueenVector[i];

               for (to = from+inc; capture=board->square[to], THROUGH(capture); to += inc) {
                  if (!UseMobPawnSafe || NO_ATTACKER(to,pawn_safe)) mob += MobMove;
               }

               if ((piece = unit[capture]) > MobDefense) { // MobAttack
                  mob += MobAttack;
                  if (UseMobAttack && NO_DEFENDER(to,pawn_safe)) att += piece;
               }
            }

            if (UseMobLinear) {
               op[me] += (mob - QueenUnit) * QueenMobOpening;
               eg[me] += (mob - QueenUnit) * QueenMobEndgame;
            } else {
               op[me] += MobQueen[Opening][mob];
               eg[me] += MobQueen[Endgame][mob];
            }

            if (UseMobAttack) {
               op[me] += (att / 2) - MobAttackPawn;
               eg[me] += att - MobAttackPawn;
            }

            // 7th rank

            if (PAWN_RANK(from,me) == Rank7) {
               if ((pawn_info->flags[opp] & BackRankFlag) != 0 // opponent pawn on 7th rank
                || PAWN_RANK(KING_POS(board,opp),me) == Rank8) {
                  op[me] += Queen7thOpening;
                  eg[me] += Queen7thEndgame;
               }
            }

            break;
         }
      }
   }

   // update

   *opening += ((op[White] - op[Black]) * PieceActivityWeight) / 256;
   *endgame += ((eg[White] - eg[Black]) * PieceActivityWeight) / 256;
}

// eval_king()

static void eval_king(const board_t * board, const material_info_t * mat_info, int * opening, int * endgame) {

   int colour;
   int op[ColourNb], eg[ColourNb];
   int me, opp;
   int from;
   int penalty_1, penalty_2;
   int tmp;
   int penalty;
   int king;
   const sq_t * ptr;
   int piece;
   int attack_tot;
   int piece_nb;

   ASSERT(board!=NULL);
   ASSERT(mat_info!=NULL);
   ASSERT(opening!=NULL);
   ASSERT(endgame!=NULL);

   // init

   for (colour = 0; colour < ColourNb; colour++) {
      op[colour] = 0;
      eg[colour] = 0;
   }

   // king attacks

   if (UseKingAttack) {

      for (colour = 0; colour < ColourNb; colour++) {

         if ((mat_info->cflags[colour] & MatKingFlag) != 0) {

            me = colour;
            opp = COLOUR_OPP(me);

            king = KING_POS(board,me);

            // piece attacks

            attack_tot = 0;
            piece_nb = 0;

            for (ptr = &board->piece[opp][1]; (from=*ptr) != SquareNone; ptr++) { // HACK: no king

               piece = board->square[from];

               if (piece_attack_king(board,piece,from,king)) {
                  piece_nb++;
                  attack_tot += KingAttackUnit[piece];
               }
            }

            // scoring

            ASSERT(piece_nb>=0&&piece_nb<16);
            op[colour] -= (attack_tot * KingAttackOpening * KingAttackWeight[piece_nb]) / 256;
         }
      }
   }

   // white pawn shelter

   if (UseShelter && (mat_info->cflags[White] & MatKingFlag) != 0) {

      me = White;

      // king

      penalty_1 = shelter_square(board,KING_POS(board,me),me);

      // castling

      penalty_2 = penalty_1;

      if ((board->flags & FlagsWhiteKingCastle) != 0) {
         tmp = shelter_square(board,G1,me);
         if (tmp < penalty_2) penalty_2 = tmp;
      }

      if ((board->flags & FlagsWhiteQueenCastle) != 0) {
         tmp = shelter_square(board,B1,me);
         if (tmp < penalty_2) penalty_2 = tmp;
      }

      ASSERT(penalty_2>=0&&penalty_2<=penalty_1);

      // penalty

      penalty = (penalty_1 + penalty_2) / 2;
      ASSERT(penalty>=0);

      op[me] -= (penalty * ShelterOpening) / 256;
   }

   // black pawn shelter

   if (UseShelter && (mat_info->cflags[Black] & MatKingFlag) != 0) {

      me = Black;

      // king

      penalty_1 = shelter_square(board,KING_POS(board,me),me);

      // castling

      penalty_2 = penalty_1;

      if ((board->flags & FlagsBlackKingCastle) != 0) {
         tmp = shelter_square(board,G8,me);
         if (tmp < penalty_2) penalty_2 = tmp;
      }

      if ((board->flags & FlagsBlackQueenCastle) != 0) {
         tmp = shelter_square(board,B8,me);
         if (tmp < penalty_2) penalty_2 = tmp;
      }

      ASSERT(penalty_2>=0&&penalty_2<=penalty_1);

      // penalty

      penalty = (penalty_1 + penalty_2) / 2;
      ASSERT(penalty>=0);

      op[me] -= (penalty * ShelterOpening) / 256;
   }

   // update

   *opening += ((op[White] - op[Black]) * KingSafetyWeight) / 256;
   *endgame += ((eg[White] - eg[Black]) * KingSafetyWeight) / 256;
}

// eval_pawn()

static void eval_pawn(const board_t * board, const pawn_info_t * pawn_info, const uint64 pawn_attacks[], int * opening, int * endgame) {

   int colour;
   int op[ColourNb], eg[ColourNb];
   int me, opp;
   int inc;
   int capture;
   int mob;
   int value;
   const sq_t * ptr;
   int att, def;
   int bits;
   int file, rank;
   int from, sq;
   int min, max;
   int delta;

   ASSERT(board!=NULL);
   ASSERT(pawn_info!=NULL);
   ASSERT(pawn_attacks!=NULL);
   ASSERT(opening!=NULL);
   ASSERT(endgame!=NULL);

   // init

   for (colour = 0; colour < ColourNb; colour++) {
      op[colour] = 0;
      eg[colour] = 0;
   }

   // pawn activity

   for (colour = 0; colour < ColourNb; colour++) {

      mob = 0;
      value = 0;
      me = colour;
      opp = COLOUR_OPP(me);
      inc = PAWN_MOVE_INC(me);

      for (ptr = &board->pawn[me][0]; (from=*ptr) != SquareNone; ptr++) { // TODO: Speed me!

         if (UsePawnAttack) {

            // capture

            sq = from + inc;

            capture = board->square[sq+1];

            if (PIECE_COLOUR(capture) == opp) {
               value += VALUE_PIECE(capture) - 50; // TODO: Tune me!

               if (is_pinned(board,sq+1,opp)) {
                  value += VALUE_PIECE(capture) - 50; // TODO: Tune me!
               }
            }

            capture = board->square[sq-1];

            if (PIECE_COLOUR(capture) == opp) {
               value += VALUE_PIECE(capture) - 50; // TODO: Tune me!

               if (is_pinned(board,sq-1,opp)) {
                  value += VALUE_PIECE(capture) - 50; // TODO: Tune me!
               }
            }
         }

         if (UsePawnMobility && board->square[sq] != Empty) mob++;
      }

      // update

      if (UsePawnMobility) {
         op[me] -= mob * 2; // TODO: Tune me!
         eg[me] -= mob * 2; // TODO: Tune me!
      }

      if (UsePawnAttack) {
         op[me] += value / 10; // TODO: Tune me!
         eg[me] += value / 10; // TODO: Tune me!
      }
   }

   // pawn passed

   for (colour = 0; colour < ColourNb; colour++) {

      att = colour;
      def = COLOUR_OPP(att);

      for (bits = pawn_info->passed_bits[att]; bits != 0; bits &= bits-1) {

         file = BIT_FIRST(bits);
         ASSERT(file>=FileA&&file<=FileH);

         rank = BIT_LAST(board->pawn_file[att][file]);
         ASSERT(rank>=Rank2&&rank<=Rank7);

         sq = SQUARE_MAKE(file,rank);
         if (COLOUR_IS_BLACK(att)) sq = SQUARE_RANK_MIRROR(sq);

         ASSERT(PIECE_IS_PAWN(board->square[sq]));
         ASSERT(COLOUR_IS(board->square[sq],att));

         // opening scoring

         op[att] += quad(PassedOpeningMin,PassedOpeningMax,rank);

         // endgame scoring init

         min = PassedEndgameMin;
         max = PassedEndgameMax;

         delta = max - min;
         ASSERT(delta>0);

         // "dangerous" bonus

         if (board->piece_size[def] <= 1 // defender has no piece
          && (unstoppable_passer(board,sq,att) || king_passer(board,sq,att))) {
            delta += UnstoppablePasser;
         } else if (free_passer(board,sq,att)) {
            delta += FreePasser;
         }

         // king-distance bonus

         delta -= pawn_att_dist(sq,KING_POS(board,att),att) * AttackerDistance;
         delta += pawn_def_dist(sq,KING_POS(board,def),att) * DefenderDistance;

         // endgame scoring

         eg[att] += min;
         if (delta > 0) eg[att] += quad(0,delta,rank);
      }
   }

   // update

   *opening += ((op[White] - op[Black]) * PawnActivityWeight) / 256;
   *endgame += ((eg[White] - eg[Black]) * PawnActivityWeight) / 256;
}

// eval_pattern()

static void eval_pattern(const board_t * board, int * opening, int * endgame) {

   ASSERT(board!=NULL);
   ASSERT(opening!=NULL);
   ASSERT(endgame!=NULL);

   // trapped bishop (7th rank)

   if ((board->square[A7] == WB && board->square[B6] == BP)
    || (board->square[B8] == WB && board->square[C7] == BP)) {
      *opening -= TrappedBishop;
      *endgame -= TrappedBishop;
   }

   if ((board->square[H7] == WB && board->square[G6] == BP)
    || (board->square[G8] == WB && board->square[F7] == BP)) {
      *opening -= TrappedBishop;
      *endgame -= TrappedBishop;
   }

   if ((board->square[A2] == BB && board->square[B3] == WP)
    || (board->square[B1] == BB && board->square[C2] == WP)) {
      *opening += TrappedBishop;
      *endgame += TrappedBishop;
   }

   if ((board->square[H2] == BB && board->square[G3] == WP)
    || (board->square[G1] == BB && board->square[F2] == WP)) {
      *opening += TrappedBishop;
      *endgame += TrappedBishop;
   }

   // trapped bishop (6th rank)

   if (board->square[A6] == WB && board->square[B5] == BP) {
      *opening -= TrappedBishop / 2;
      *endgame -= TrappedBishop / 2;
   }

   if (board->square[H6] == WB && board->square[G5] == BP) {
      *opening -= TrappedBishop / 2;
      *endgame -= TrappedBishop / 2;
   }

   if (board->square[A3] == BB && board->square[B4] == WP) {
      *opening += TrappedBishop / 2;
      *endgame += TrappedBishop / 2;
   }

   if (board->square[H3] == BB && board->square[G4] == WP) {
      *opening += TrappedBishop / 2;
      *endgame += TrappedBishop / 2;
   }

   // blocked bishop

   if (board->square[D2] == WP && board->square[D3] != Empty && board->square[C1] == WB) {
      *opening -= BlockedBishop;
   }

   if (board->square[E2] == WP && board->square[E3] != Empty && board->square[F1] == WB) {
      *opening -= BlockedBishop;
   }

   if (board->square[D7] == BP && board->square[D6] != Empty && board->square[C8] == BB) {
      *opening += BlockedBishop;
   }

   if (board->square[E7] == BP && board->square[E6] != Empty && board->square[F8] == BB) {
      *opening += BlockedBishop;
   }

   // blocked rook

   if ((board->square[C1] == WK || board->square[B1] == WK)
    && (board->square[A1] == WR || board->square[A2] == WR || board->square[B1] == WR)) {
      *opening -= BlockedRook;
   }

   if ((board->square[F1] == WK || board->square[G1] == WK)
    && (board->square[H1] == WR || board->square[H2] == WR || board->square[G1] == WR)) {
      *opening -= BlockedRook;
   }

   if ((board->square[C8] == BK || board->square[B8] == BK)
    && (board->square[A8] == BR || board->square[A7] == BR || board->square[B8] == BR)) {
      *opening += BlockedRook;
   }

   if ((board->square[F8] == BK || board->square[G8] == BK)
    && (board->square[H8] == BR || board->square[H7] == BR || board->square[G8] == BR)) {
      *opening += BlockedRook;
   }
}

// eval_promote()

static void eval_promote(const board_t * board, int * opening, int * endgame) {

   int colour;
   int i;
   int move;
   int best_value, value;
   list_t list[1];

   ASSERT(board!=NULL);
   ASSERT(opening!=NULL);
   ASSERT(endgame!=NULL);

   for (colour = 0; colour < ColourNb; colour++) {

      best_value = 0;
      gen_promotes(list,board,colour);

      for (i = 0; i < LIST_SIZE(list); i++) {

         move = LIST_MOVE(list,i);

         ASSERT(move_is_ok(move));
         ASSERT(MOVE_IS_PROMOTE(move));

         if ((move & MoveAllFlags) != MovePromoteQueen) continue;

         value = see_move(move,board,0);
         if (value > 0) best_value += value;
      }

      // update

      best_value /= 10;
      ASSERT(value_is_ok(best_value));

      if (COLOUR_IS_BLACK(colour)) best_value = -best_value;

      *opening += best_value;
      *endgame += best_value;
   }
}

// eval_tempo()

static void eval_tempo(const board_t * board, int * opening, int * endgame) {

   ASSERT(board!=NULL);
   ASSERT(opening!=NULL);
   ASSERT(endgame!=NULL);

   if (COLOUR_IS_WHITE(board->turn)) {
     *opening += SideToMoveOpening;
     *endgame += SideToMoveEndgame;
   } else {
     *opening -= SideToMoveOpening;
     *endgame -= SideToMoveEndgame;
   }
}

// unstoppable_passer()

static bool unstoppable_passer(const board_t * board, int pawn, int colour) {

   int me, opp;
   int file, rank;
   int king;
   int prom;
   const sq_t * ptr;
   int sq;
   int dist;

   ASSERT(board!=NULL);
   ASSERT(SQUARE_IS_OK(pawn));
   ASSERT(COLOUR_IS_OK(colour));

   me = colour;
   opp = COLOUR_OPP(me);

   file = SQUARE_FILE(pawn);
   rank = PAWN_RANK(pawn,me);

   king = KING_POS(board,opp);

   // clear promotion path?

   for (ptr = &board->piece[me][0]; (sq=*ptr) != SquareNone; ptr++) {
      if (SQUARE_FILE(sq) == file && PAWN_RANK(sq,me) > rank) {
         return false; // "friendly" blocker
      }
   }

   // init

   if (rank == Rank2) {
      pawn += PAWN_MOVE_INC(me);
      rank++;
      ASSERT(rank==PAWN_RANK(pawn,me));
   }

   ASSERT(rank>=Rank3&&rank<=Rank7);

   prom = PAWN_PROMOTE(pawn,me);

   dist = DISTANCE(pawn,prom);
   ASSERT(dist==Rank8-rank);
   if (board->turn == opp) dist++;

   if (DISTANCE(king,prom) > dist) return true; // not in the square

   return false;
}

// king_passer()

static bool king_passer(const board_t * board, int pawn, int colour) {

   int me;
   int king;
   int file;
   int prom;

   ASSERT(board!=NULL);
   ASSERT(SQUARE_IS_OK(pawn));
   ASSERT(COLOUR_IS_OK(colour));

   me = colour;

   king = KING_POS(board,me);
   file = SQUARE_FILE(pawn);
   prom = PAWN_PROMOTE(pawn,me);

   if (DISTANCE(king,prom) <= 1
    && DISTANCE(king,pawn) <= 1
    && (SQUARE_FILE(king) != file
     || (file != FileA && file != FileH))) {
      return true;
   }

   return false;
}

// free_passer()

static bool free_passer(const board_t * board, int pawn, int colour) {

   int me;
   int inc;
   int sq;
   int move;

   ASSERT(board!=NULL);
   ASSERT(SQUARE_IS_OK(pawn));
   ASSERT(COLOUR_IS_OK(colour));

   me = colour;

   inc = PAWN_MOVE_INC(me);
   sq = pawn + inc;
   ASSERT(SQUARE_IS_OK(sq));

   if (board->square[sq] != Empty) return false;

   move = MOVE_MAKE(pawn,sq);
   if (see_move(move,board,0) < 0) return false;

   return true;
}

// pawn_att_dist()

static int pawn_att_dist(int pawn, int king, int colour) {

   int me;
   int inc;
   int target;

   ASSERT(SQUARE_IS_OK(pawn));
   ASSERT(SQUARE_IS_OK(king));
   ASSERT(COLOUR_IS_OK(colour));

   me = colour;
   inc = PAWN_MOVE_INC(me);

   target = pawn + inc;

   return DISTANCE(king,target);
}

// pawn_def_dist()

static int pawn_def_dist(int pawn, int king, int colour) {

   int me;
   int inc;
   int target;

   ASSERT(SQUARE_IS_OK(pawn));
   ASSERT(SQUARE_IS_OK(king));
   ASSERT(COLOUR_IS_OK(colour));

   me = colour;
   inc = PAWN_MOVE_INC(me);

   target = pawn + inc;

   return DISTANCE(king,target);
}

// draw_init_list()

static void draw_init_list(int list[], const board_t * board, int pawn_colour) {

   int pos;
   int att, def;
   const sq_t * ptr;
   int sq;
   int pawn;
   int i;

   ASSERT(list!=NULL);
   ASSERT(board!=NULL);
   ASSERT(COLOUR_IS_OK(pawn_colour));

   // init

   pos = 0;

   att = pawn_colour;
   def = COLOUR_OPP(att);

   ASSERT(board->pawn_size[att]==1);
   ASSERT(board->pawn_size[def]==0);

   // att

   for (ptr = &board->piece[att][0]; (sq=*ptr) != SquareNone; ptr++) {
      list[pos++] = sq;
   }

   for (ptr = &board->pawn[att][0]; (sq=*ptr) != SquareNone; ptr++) {
      list[pos++] = sq;
   }

   // def

   for (ptr = &board->piece[def][0]; (sq=*ptr) != SquareNone; ptr++) {
      list[pos++] = sq;
   }

   for (ptr = &board->pawn[def][0]; (sq=*ptr) != SquareNone; ptr++) {
      list[pos++] = sq;
   }

   // end marker

   ASSERT(pos==board->piece_nb);

   list[pos] = SquareNone;

   // file flip?

   pawn = board->pawn[att][0];

   if (SQUARE_FILE(pawn) >= FileE) {
      for (i = 0; i < pos; i++) {
         list[i] = SQUARE_FILE_MIRROR(list[i]);
      }
   }

   // rank flip?

   if (COLOUR_IS_BLACK(pawn_colour)) {
      for (i = 0; i < pos; i++) {
         list[i] = SQUARE_RANK_MIRROR(list[i]);
      }
   }
}

// draw_kpkq()

static bool draw_kpkq(const int list[], int turn) {

   int wk, wp, bk, bq;
   int prom;
   int dist;
   // int wp_file, wp_rank;

   ASSERT(list!=NULL);
   ASSERT(COLOUR_IS_OK(turn));

   // load

   wk = *list++;
   ASSERT(SQUARE_IS_OK(wk));

   wp = *list++;
   ASSERT(SQUARE_IS_OK(wp));
   ASSERT(SQUARE_FILE(wp)<=FileD);

   bk = *list++;
   ASSERT(SQUARE_IS_OK(bk));

   bq = *list++;
   ASSERT(SQUARE_IS_OK(bq));

   ASSERT(*list==SquareNone);

   // test

   if (false) {

   } else if (wp == A7) {

      prom = A8;
      dist = 4;

      if (wk == B7 || wk == B8) { // best case
         if (COLOUR_IS_WHITE(turn)) dist--;
      } else if (wk == A8 || ((wk == C7 || wk == C8) && bq != A8)) { // white loses a tempo
         if (COLOUR_IS_BLACK(turn) && SQUARE_FILE(bq) != FileB) return false;
      } else {
         return false;
      }

      ASSERT(bq!=prom);
      if (DISTANCE(bk,prom) > dist) return true;

   } else if (wp == C7) {

      prom = C8;
      dist = 4;

      if (false) {

      } else if (wk == C8) { // dist = 0

         dist++; // self-blocking penalty
         if (COLOUR_IS_WHITE(turn)) dist--; // right-to-move bonus

      } else if (wk == B7 || wk == B8) { // dist = 1, right side

         dist--; // right-side bonus
         if (DELTA_INC_LINE(wp-bq) == wk-wp) dist++; // pinned-pawn penalty
         if (COLOUR_IS_WHITE(turn)) dist--; // right-to-move bonus

      } else if (wk == D7 || wk == D8) { // dist = 1, wrong side

         if (DELTA_INC_LINE(wp-bq) == wk-wp) dist++; // pinned-pawn penalty
         if (COLOUR_IS_WHITE(turn)) dist--; // right-to-move bonus

      } else if ((wk == A7 || wk == A8) && bq != C8) { // dist = 2, right side

         if (COLOUR_IS_BLACK(turn) && SQUARE_FILE(bq) != FileB) return false;

         dist--; // right-side bonus

      } else if ((wk == E7 || wk == E8) && bq != C8) { // dist = 2, wrong side

         if (COLOUR_IS_BLACK(turn) && SQUARE_FILE(bq) != FileD) return false;

      } else {

         return false;
      }

      ASSERT(bq!=prom);
      if (DISTANCE(bk,prom) > dist) return true;
   }

   return false;
}

// draw_kpkr()

static bool draw_kpkr(const int list[], int turn) {

   int wk, wp, bk, br;
   int wk_file, wk_rank;
   int wp_file, wp_rank;
   int br_file, br_rank;
   int inc;
   int prom;
   int dist;

   ASSERT(list!=NULL);
   ASSERT(COLOUR_IS_OK(turn));

   // load

   wk = *list++;
   ASSERT(SQUARE_IS_OK(wk));

   wp = *list++;
   ASSERT(SQUARE_IS_OK(wp));
   ASSERT(SQUARE_FILE(wp)<=FileD);

   bk = *list++;
   ASSERT(SQUARE_IS_OK(bk));

   br = *list++;
   ASSERT(SQUARE_IS_OK(br));

   ASSERT(*list==SquareNone);

   // init

   wk_file = SQUARE_FILE(wk);
   wk_rank = SQUARE_RANK(wk);

   wp_file = SQUARE_FILE(wp);
   wp_rank = SQUARE_RANK(wp);

   br_file = SQUARE_FILE(br);
   br_rank = SQUARE_RANK(br);

   inc = PAWN_MOVE_INC(White);
   prom = PAWN_PROMOTE(wp,White);

   // conditions

   if (false) {

   } else if (DISTANCE(wk,wp) == 1) {

      ASSERT(ABS(wk_file-wp_file)<=1);
      ASSERT(ABS(wk_rank-wp_rank)<=1);

      // no-op

   } else if (DISTANCE(wk,wp) == 2 && ABS(wk_rank-wp_rank) <= 1) {

      ASSERT(ABS(wk_file-wp_file)==2);
      ASSERT(ABS(wk_rank-wp_rank)<=1);

      if (COLOUR_IS_BLACK(turn) && br_file != (wk_file + wp_file) / 2) return false;

   } else {

      return false;
   }

   // white features

   dist = DISTANCE(wk,prom) + DISTANCE(wp,prom);
   if (wk == prom) dist++;

   if (wk == wp+inc) { // king on pawn's "front square"
      if (wp_file == FileA) return false;
      dist++; // self-blocking penalty
   }

   // black features

   if (br_file != wp_file && br_rank != Rank8) {
      dist--; // misplaced-rook bonus
   }

   // test

   if (COLOUR_IS_WHITE(turn)) dist--; // right-to-move bonus

   if (DISTANCE(bk,prom) > dist) return true;

   return false;
}

// draw_kpkb()

static bool draw_kpkb(const int list[], int turn) {

   int wk, wp, bk, bb;
   int inc;
   int end, to;
   int delta, inc_2;
   int sq;

   ASSERT(list!=NULL);
   ASSERT(COLOUR_IS_OK(turn));

   // load

   wk = *list++;
   ASSERT(SQUARE_IS_OK(wk));

   wp = *list++;
   ASSERT(SQUARE_IS_OK(wp));
   ASSERT(SQUARE_FILE(wp)<=FileD);

   bk = *list++;
   ASSERT(SQUARE_IS_OK(bk));

   bb = *list++;
   ASSERT(SQUARE_IS_OK(bb));

   ASSERT(*list==SquareNone);

   // blocked pawn?

   inc = PAWN_MOVE_INC(White);
   end = PAWN_PROMOTE(wp,White) + inc;

   for (to = wp+inc; to != end; to += inc) {

      ASSERT(SQUARE_IS_OK(to));

      if (to == bb) return true; // direct blockade

      delta = to - bb;
      ASSERT(delta_is_ok(delta));

      if (PSEUDO_ATTACK(BB,delta)) {

         inc_2 = DELTA_INC_ALL(delta);
         ASSERT(inc_2!=IncNone);

         sq = bb;
         do {
            sq += inc_2;
            ASSERT(SQUARE_IS_OK(sq));
            ASSERT(sq!=wk);
            ASSERT(sq!=wp);
            ASSERT(sq!=bb);
            if (sq == to) return true; // indirect blockade
         } while (sq != bk);
      }
   }

   return false;
}

// draw_kpkn()

static bool draw_kpkn(const int list[], int turn) {

   int wk, wp, bk, bn;
   int inc;
   int end;
   int file;
   int sq;

   ASSERT(list!=NULL);
   ASSERT(COLOUR_IS_OK(turn));

   // load

   wk = *list++;
   ASSERT(SQUARE_IS_OK(wk));

   wp = *list++;
   ASSERT(SQUARE_IS_OK(wp));
   ASSERT(SQUARE_FILE(wp)<=FileD);

   bk = *list++;
   ASSERT(SQUARE_IS_OK(bk));

   bn = *list++;
   ASSERT(SQUARE_IS_OK(bn));

   ASSERT(*list==SquareNone);

   // blocked pawn?

   inc = PAWN_MOVE_INC(White);
   end = PAWN_PROMOTE(wp,White) + inc;

   file = SQUARE_FILE(wp);
   if (file == FileA || file == FileH) end -= inc;

   for (sq = wp+inc; sq != end; sq += inc) {

      ASSERT(SQUARE_IS_OK(sq));

      if (sq == bn || PSEUDO_ATTACK(BN,sq-bn)) return true; // blockade
   }

   return false;
}

// draw_knpk()

static bool draw_knpk(const int list[], int turn) {

   int wk, wn, wp, bk;

   ASSERT(list!=NULL);
   ASSERT(COLOUR_IS_OK(turn));

   // load

   wk = *list++;
   ASSERT(SQUARE_IS_OK(wk));

   wn = *list++;
   ASSERT(SQUARE_IS_OK(wn));

   wp = *list++;
   ASSERT(SQUARE_IS_OK(wp));
   ASSERT(SQUARE_FILE(wp)<=FileD);

   bk = *list++;
   ASSERT(SQUARE_IS_OK(bk));

   ASSERT(*list==SquareNone);

   // test

   if (wp == A7 && DISTANCE(bk,A8) <= 1) return true;

   return false;
}

// draw_krpkr()

static bool draw_krpkr(const int list[], int turn) {

   int wk, wr, wp, bk, br;
   int wp_file, wp_rank;
   int bk_file, bk_rank;
   int br_file, br_rank;
   int prom;

   ASSERT(list!=NULL);
   ASSERT(COLOUR_IS_OK(turn));

   // load

   wk = *list++;
   ASSERT(SQUARE_IS_OK(wk));

   wr = *list++;
   ASSERT(SQUARE_IS_OK(wr));

   wp = *list++;
   ASSERT(SQUARE_IS_OK(wp));
   ASSERT(SQUARE_FILE(wp)<=FileD);

   bk = *list++;
   ASSERT(SQUARE_IS_OK(bk));

   br = *list++;
   ASSERT(SQUARE_IS_OK(br));

   ASSERT(*list==SquareNone);

   // test

   wp_file = SQUARE_FILE(wp);
   wp_rank = SQUARE_RANK(wp);

   bk_file = SQUARE_FILE(bk);
   bk_rank = SQUARE_RANK(bk);

   br_file = SQUARE_FILE(br);
   br_rank = SQUARE_RANK(br);

   prom = PAWN_PROMOTE(wp,White);

   if (false) {

   } else if (bk == prom) {

      // TODO: rook near Rank1 if wp_rank == Rank6?

      if (br_file > wp_file) return true;

   } else if (bk_file == wp_file && bk_rank > wp_rank) {

      return true;

   } else if (wr == prom && wp_rank == Rank7 && (bk == G7 || bk == H7) && br_file == wp_file) {

      if (br_rank <= Rank3) {
         if (DISTANCE(wk,wp) > 1) return true;
      } else { // br_rank >= Rank4
         if (DISTANCE(wk,wp) > 2) return true;
      }
   }

   return false;
}

// draw_kbpkb()

static bool draw_kbpkb(const int list[], int turn) {

   int wk, wb, wp, bk, bb;
   int inc;
   int end, to;
   int delta, inc_2;
   int sq;

   ASSERT(list!=NULL);
   ASSERT(COLOUR_IS_OK(turn));

   // load

   wk = *list++;
   ASSERT(SQUARE_IS_OK(wk));

   wb = *list++;
   ASSERT(SQUARE_IS_OK(wb));

   wp = *list++;
   ASSERT(SQUARE_IS_OK(wp));
   ASSERT(SQUARE_FILE(wp)<=FileD);

   bk = *list++;
   ASSERT(SQUARE_IS_OK(bk));

   bb = *list++;
   ASSERT(SQUARE_IS_OK(bb));

   ASSERT(*list==SquareNone);

   // opposit colour?

   if (SQUARE_COLOUR(wb) == SQUARE_COLOUR(bb)) return false; // TODO

   // blocked pawn?

   inc = PAWN_MOVE_INC(White);
   end = PAWN_PROMOTE(wp,White) + inc;

   for (to = wp+inc; to != end; to += inc) {

      ASSERT(SQUARE_IS_OK(to));

      if (to == bb) return true; // direct blockade

      delta = to - bb;
      ASSERT(delta_is_ok(delta));

      if (PSEUDO_ATTACK(BB,delta)) {

         inc_2 = DELTA_INC_ALL(delta);
         ASSERT(inc_2!=IncNone);

         sq = bb;
         do {
            sq += inc_2;
            ASSERT(SQUARE_IS_OK(sq));
            ASSERT(sq!=wk);
            ASSERT(sq!=wb);
            ASSERT(sq!=wp);
            ASSERT(sq!=bb);
            if (sq == to) return true; // indirect blockade
         } while (sq != bk);
      }
   }

   return false;
}

// win_kxk()

static int win_kxk(const board_t * board, int colour) {

   int me, opp;
   int att, def;
   int dist;
   int value;

   ASSERT(board!=NULL);
   ASSERT(colour==White||colour==Black);

   ASSERT(board->piece_nb==3);
   ASSERT(board->pawn_size[White]==0);
   ASSERT(board->pawn_size[Black]==0);
   ASSERT((board->square[board->piece[colour][1]]&RookFlag)!=0); // HACK: queen included

   // init

   me = colour;
   opp = COLOUR_OPP(me);
   att = KING_POS(board,me);
   def = KING_POS(board,opp);
   dist = DISTANCE(att,def);
   value = ValueWin;

   // pst

   value += MajPst[SQUARE_TO_64(def)];

   // king-king distance

   value += 80 - (dist * 10); // TODO: tune me!

   if (COLOUR_IS_BLACK(me)) value = -value;

   ASSERT(value!=0);
   ASSERT(!value_is_mate(value));

   return value;
}

// win_kxkx()

static int win_kxkx(const board_t * board, int colour) {

   int me, opp;
   int att, def;
   int dist;
   int value;

   ASSERT(board!=NULL);
   ASSERT(colour==White||colour==Black);

   ASSERT(board->piece_nb==4);
   ASSERT(board->pawn_size[White]==0);
   ASSERT(board->pawn_size[Black]==0);

   // init

   me = colour;
   opp = COLOUR_OPP(me);
   att = KING_POS(board,me);
   def = KING_POS(board,opp);
   dist = DISTANCE(att,def);
   value = 0;

   // pst

   value += MajPst[SQUARE_TO_64(def)];

   // king-king distance

   value += 80 - (dist * 10); // TODO: tune me!

   if (COLOUR_IS_BLACK(me)) value = -value;

   ASSERT(value!=0);
   ASSERT(value>-200&&value<200);

   return value;
}

// win_knbk()

static int win_knbk(const board_t * board, int colour) {

   int me, opp;
   int att, def;
   int dist;
   int value;
   int sq_64;

   ASSERT(board!=NULL);
   ASSERT(colour==White||colour==Black);

   ASSERT(board->piece_nb==4);
   ASSERT(board->pawn_size[White]==0);
   ASSERT(board->pawn_size[Black]==0);
   ASSERT(board->square[board->piece[colour][1]]==(BishopFlag|COLOUR_FLAG(colour)));
   ASSERT(board->square[board->piece[colour][2]]==(KnightFlag|COLOUR_FLAG(colour)));

   // init

   me = colour;
   opp = COLOUR_OPP(me);
   att = KING_POS(board,me);
   def = KING_POS(board,opp);
   dist = DISTANCE(att,def);
   value = ValueWin;

   // bishop color

   if (SQUARE_COLOUR(board->piece[colour][1]) == Black) {
      sq_64 = (SQUARE_TO_64(def) ^ 07);
   } else {
      sq_64 = SQUARE_TO_64(def);
   }

   // pst

   value += NBPst[sq_64];

   // king-king distance

   value += 50 - (dist * 5); // TODO: tune me!

   if (COLOUR_IS_BLACK(me)) value = -value;

   ASSERT(value!=0);
   ASSERT(!value_is_mate(value));

   return value;
}

// win_kbbk()

static int win_kbbk(const board_t * board, int colour) {

   int me, opp;
   int att, def;
   int dist;
   int value;

   ASSERT(board!=NULL);
   ASSERT(colour==White||colour==Black);

   ASSERT(board->piece_nb==4);
   ASSERT(board->pawn_size[White]==0);
   ASSERT(board->pawn_size[Black]==0);
   ASSERT(board->square[board->piece[colour][1]]==(BishopFlag|COLOUR_FLAG(colour)));
   ASSERT(board->square[board->piece[colour][2]]==(BishopFlag|COLOUR_FLAG(colour)));

   // init

   me = colour;
   opp = COLOUR_OPP(me);
   att = KING_POS(board,me);
   def = KING_POS(board,opp);
   dist = DISTANCE(att,def);
   value = ValueWin;

   // conditions

   if (SQUARE_COLOUR(board->piece[colour][1]) == SQUARE_COLOUR(board->piece[colour][2])) return 0;

   // pst

   value += MinPst[SQUARE_TO_64(def)];

   // king-king distance

   value += 80 - (dist * 10); // TODO: tune me!

   if (COLOUR_IS_BLACK(me)) value = -value;

   ASSERT(value!=0);
   ASSERT(!value_is_mate(value));

   return value;
}

// shelter_square()

static int shelter_square(const board_t * board, int square, int colour) {

   int penalty;
   int file, rank;

   ASSERT(board!=NULL);
   ASSERT(SQUARE_IS_OK(square));
   ASSERT(COLOUR_IS_OK(colour));

   penalty = 0;

   file = SQUARE_FILE(square);
   rank = PAWN_RANK(square,colour);

   penalty += shelter_file(board,file,rank,colour) * 2;
   if (file != FileA) penalty += shelter_file(board,file-1,rank,colour);
   if (file != FileH) penalty += shelter_file(board,file+1,rank,colour);

   if (penalty == 0) penalty = 11; // weak back rank

   if (UseStorm) {
      penalty += storm_file(board,file,colour);
      if (file != FileA) penalty += storm_file(board,file-1,colour);
      if (file != FileH) penalty += storm_file(board,file+1,colour);
   }

   return penalty;
}

// shelter_file()

static int shelter_file(const board_t * board, int file, int rank, int colour) {

   int dist;
   int penalty;

   ASSERT(board!=NULL);
   ASSERT(file>=FileA&&file<=FileH);
   ASSERT(rank>=Rank1&&rank<=Rank8);
   ASSERT(COLOUR_IS_OK(colour));

   dist = BIT_FIRST(board->pawn_file[colour][file]&BitGE[rank]);
   ASSERT(dist>=Rank2&&dist<=Rank8);

   dist = Rank8 - dist;
   ASSERT(dist>=0&&dist<=6);

   penalty = 36 - dist * dist;
   ASSERT(penalty>=0&&penalty<=36);

   return penalty;
}

// storm_file()

static int storm_file(const board_t * board, int file, int colour) {

   int dist;
   int penalty;

   ASSERT(board!=NULL);
   ASSERT(file>=FileA&&file<=FileH);
   ASSERT(COLOUR_IS_OK(colour));

   dist = BIT_LAST(board->pawn_file[COLOUR_OPP(colour)][file]);
   ASSERT(dist>=Rank1&&dist<=Rank7);

   penalty = 0;

   switch (dist) {
   case Rank4:
      penalty = StormOpening * 1;
      break;
   case Rank5:
      penalty = StormOpening * 3;
      break;
   case Rank6:
      penalty = StormOpening * 6;
      break;
   }

   return penalty;
}

// bishop_can_attack()

static bool bishop_can_attack(const board_t * board, int to, int colour) {

   const sq_t * ptr;
   int from;
   int piece;

   ASSERT(board!=NULL);
   ASSERT(SQUARE_IS_OK(to));
   ASSERT(COLOUR_IS_OK(colour));

   for (ptr = &board->piece[colour][1]; (from=*ptr) != SquareNone; ptr++) { // HACK: no king

      piece = board->square[from];

      if (PIECE_IS_BISHOP(piece) && SQUARE_COLOUR(from) == SQUARE_COLOUR(to)) {
         return true;
      }
   }

   return false;
}

// end of eval.cpp

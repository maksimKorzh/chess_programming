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

// material.cpp

// includes

#include <cstring>

#include "board.h"
#include "colour.h"
#include "hash.h"
#include "material.h"
#include "option.h"
#include "piece.h"
#include "protocol.h"
#include "smp.h"
#include "square.h"
#include "util.h"

// constants

static const bool UseStats = false;

static const bool UseTable = true;
static const uint32 TableSize = 256; // 4kB * ThreadMax

static const int PawnPhase   = 0;
static const int KnightPhase = 1;
static const int BishopPhase = 1;
static const int RookPhase   = 2;
static const int QueenPhase  = 4;

static const int TotalPhase = PawnPhase * 16 + KnightPhase * 4 + BishopPhase * 4 + RookPhase * 4 + QueenPhase * 2;

// constants and variables

static int MaterialWeight = 256; // 100%

static int PawnWeight = 256;
static int KnightWeight = 256;
static int BishopWeight = 256;
static int RookWeight = 256;
static int QueenWeight = 256;

static int BishopPairWeight = 256;

static const int PawnOpening   = 80; // was 100
static const int PawnEndgame   = 90; // was 100
static const int KnightOpening = 320;
static const int KnightEndgame = 330;
static const int BishopOpening = 325;
static const int BishopEndgame = 335;
static const int RookOpening   = 500;
static const int RookEndgame   = 500;
static const int QueenOpening  = 975;
static const int QueenEndgame  = 1000;

static const int BishopPairOpening = 50;
static const int BishopPairEndgame = 50;

// types

typedef material_info_t entry_t;

struct material_t {
   entry_t table[ThreadMax][TableSize];
   uint32 size;
   uint32 mask;
   uint32 used;            // TODO: thread based
   sint64 read_nb;         // TODO: thread based
   sint64 read_hit;        // TODO: thread based
   sint64 write_nb;        // TODO: thread based
   sint64 write_collision; // TODO: thread based
};

// variables

static material_t Material[1];

// prototypes

static void material_comp_info (material_info_t * info, const board_t * board);

// functions

// material_init()

void material_init() {

   // UCI options

   MaterialWeight = (option_get_int("Material") * 256 + 50) / 100;

   PawnWeight =   (option_get_int("Pawn")   * 256 + 50) / 100;
   KnightWeight = (option_get_int("Knight") * 256 + 50) / 100;
   BishopWeight = (option_get_int("Bishop") * 256 + 50) / 100;
   RookWeight =   (option_get_int("Rook")   * 256 + 50) / 100;
   QueenWeight =  (option_get_int("Queen")  * 256 + 50) / 100;

   // material table

   if (UseTable) {

      ASSERT(sizeof(entry_t)==16);

      Material->size = TableSize;
      Material->mask = TableSize - 1;

      material_clear();

   } else {

      Material->size = 0;
      Material->mask = 0;
   }
}

// material_clear()

void material_clear() {

   memset(Material->table,0,sizeof(Material->table));

   Material->used = 0;
   Material->read_nb = 0;
   Material->read_hit = 0;
   Material->write_nb = 0;
   Material->write_collision = 0;
}

// material_set_option()

void material_set_option(int weight) {

   ASSERT(weight>=0&&weight<=400); // option.cpp
   MaterialWeight = (weight * 256 + 50) / 100;

   material_clear();
}

void piece_set_option(int weight, int piece) {

   ASSERT(weight>=0&&weight<=400); // option.cpp
   ASSERT(piece>=MAT_PAWN&&piece<=MAT_BISHOP_PAIR);

   switch (piece) {

   case MAT_PAWN:

      PawnWeight = (weight * 256 + 50) / 100;
      break;

   case MAT_KNIGHT:

      KnightWeight = (weight * 256 + 50) / 100;
      break;

   case MAT_BISHOP:

      BishopWeight = (weight * 256 + 50) / 100;
      break;

   case MAT_ROOK:

      RookWeight = (weight * 256 + 50) / 100;
      break;

   case MAT_QUEEN:

      QueenWeight = (weight * 256 + 50) / 100;
      break;

   case MAT_BISHOP_PAIR:

      BishopPairWeight = (weight * 256 + 50) / 100;
      break;

   default:

      ASSERT(false);
      break;
   }

   material_clear();
}

// material_get_info()

void material_get_info(material_info_t * info, const board_t * board, int thread) {

   uint64 key;
   entry_t * entry;

   ASSERT(info!=NULL);
   ASSERT(board!=NULL);
   ASSERT(thread_is_ok(thread));

   // probe

   if (UseTable) {

      if (UseStats) Material->read_nb++;

      key = board->material_key;
      entry = &Material->table[thread][KEY_INDEX(key)&Material->mask];

      if (entry->lock == KEY_LOCK(key)) {

         // found

         if (UseStats) Material->read_hit++;

         *info = *entry;

         return;
      }
   }

   // calculation

   material_comp_info(info,board);

   // store

   if (UseTable) {

      if (UseStats) {
         Material->write_nb++;

         if (entry->lock == 0) { // HACK: assume free entry
            Material->used++;
         } else {
            Material->write_collision++;
         }
      }

      *entry = *info;
      entry->lock = KEY_LOCK(key);
   }
}

// material_comp_info()

static void material_comp_info(material_info_t * info, const board_t * board) {

   int wp, wn, wb, wr, wq;
   int bp, bn, bb, br, bq;
   int wt, bt;
   int wm, bm;
   int colour;
   int recog;
   int flags;
   int cflags[ColourNb];
   int mul[ColourNb];
   int phase;
   int opening, endgame;

   ASSERT(info!=NULL);
   ASSERT(board!=NULL);

   // init

   wp = board->number[WhitePawn12];
   wn = board->number[WhiteKnight12];
   wb = board->number[WhiteBishop12];
   wr = board->number[WhiteRook12];
   wq = board->number[WhiteQueen12];

   bp = board->number[BlackPawn12];
   bn = board->number[BlackKnight12];
   bb = board->number[BlackBishop12];
   br = board->number[BlackRook12];
   bq = board->number[BlackQueen12];

   wt = wq + wr + wb + wn + wp; // no king
   bt = bq + br + bb + bn + bp; // no king

   wm = wb + wn;
   bm = bb + bn;

   // recogniser

   recog = MAT_NONE;

   if (false) {

   } else if (wt == 0 && bt == 0) {

      recog = MAT_KK;

   } else if (wt == 1 && bt == 0) {

      if (wb == 1) recog = MAT_KBK;
      if (wn == 1) recog = MAT_KNK;
      if (wp == 1) recog = MAT_KPK;
      if (wr == 1) recog = MAT_KRK;
      if (wq == 1) recog = MAT_KQK;

   } else if (wt == 0 && bt == 1) {

      if (bb == 1) recog = MAT_KKB;
      if (bn == 1) recog = MAT_KKN;
      if (bp == 1) recog = MAT_KKP;
      if (br == 1) recog = MAT_KKR;
      if (bq == 1) recog = MAT_KKQ;

   } else if (wt == 1 && bt == 1) {

      if (wq == 1 && bq == 1) recog = MAT_KQKQ;
      if (wq == 1 && br == 1) recog = MAT_KQKR;
      if (wr == 1 && bq == 1) recog = MAT_KRKQ;
      if (wq == 1 && bp == 1) recog = MAT_KQKP;
      if (wp == 1 && bq == 1) recog = MAT_KPKQ;

      if (wr == 1 && br == 1) recog = MAT_KRKR;
      if (wr == 1 && bp == 1) recog = MAT_KRKP;
      if (wp == 1 && br == 1) recog = MAT_KPKR;

      if (wb == 1 && bb == 1) recog = MAT_KBKB;
      if (wb == 1 && bp == 1) recog = MAT_KBKP;
      if (wp == 1 && bb == 1) recog = MAT_KPKB;

      if (wn == 1 && bn == 1) recog = MAT_KNKN;
      if (wn == 1 && bp == 1) recog = MAT_KNKP;
      if (wp == 1 && bn == 1) recog = MAT_KPKN;

   } else if (wt == 2 && bt == 0) {

      if (wb == 2)            recog = MAT_KBBK;
      if (wb == 1 && wp == 1) recog = MAT_KBPK;
      if (wb == 1 && wn == 1) recog = MAT_KNBK;
      if (wn == 1 && wp == 1) recog = MAT_KNPK;

   } else if (wt == 0 && bt == 2) {

      if (bb == 2)            recog = MAT_KKBB;
      if (bb == 1 && bp == 1) recog = MAT_KKBP;
      if (bn == 1 && bb == 1) recog = MAT_KKNB;
      if (bn == 1 && bp == 1) recog = MAT_KKNP;

   } else if (wt == 2 && bt == 1) {

      if (wr == 1 && wp == 1 && br == 1) recog = MAT_KRPKR;
      if (wb == 1 && wp == 1 && bb == 1) recog = MAT_KBPKB;

   } else if (wt == 1 && bt == 2) {

      if (wr == 1 && br == 1 && bp == 1) recog = MAT_KRKRP;
      if (wb == 1 && bb == 1 && bp == 1) recog = MAT_KBKBP;
   }

   // draw node (exact-draw recogniser)

   flags = 0; // TODO: MOVE ME
   for (colour = 0; colour < ColourNb; colour++) cflags[colour] = 0;

   if (wq+wr+wp == 0 && bq+br+bp == 0) { // no major piece or pawn
      if (wm + bm <= 1 // at most one minor => KK, KBK or KNK
       || recog == MAT_KBKB) {
         flags |= DrawNodeFlag;
      }
   } else if (recog == MAT_KPK  || recog == MAT_KKP
           || recog == MAT_KBPK || recog == MAT_KKBP) {
      flags |= DrawNodeFlag;
   }

   // bishop endgame

   if (wq+wr+wn == 0 && bq+br+bn == 0) { // only bishops
      if (wb == 1 && bb == 1) {
         if (wp-bp >= -2 && wp-bp <= +2) { // pawn diff <= 2
            flags |= DrawBishopFlag;
         }
      }
   }

   // multipliers

   for (colour = 0; colour < ColourNb; colour++) mul[colour] = 16; // 1

   // white multiplier

   if (wp == 0) { // white has no pawns

      int w_maj = wq * 2 + wr;
      int w_min = wb + wn;
      int w_tot = w_maj * 2 + w_min;

      int b_maj = bq * 2 + br;
      int b_min = bb + bn;
      int b_tot = b_maj * 2 + b_min;

      if (false) {

      } else if (w_tot == 1) {

         ASSERT(w_maj==0);
         ASSERT(w_min==1);

         // KBK* or KNK*, always insufficient

         mul[White] = 0;

      } else if (w_tot == 2 && wn == 2) {

         ASSERT(w_maj==0);
         ASSERT(w_min==2);

         // KNNK*, usually insufficient

         if (b_tot != 0 || bp == 0) {
            mul[White] = 0;
         } else { // KNNKP+, might not be draw
            mul[White] = 1; // 1/16
         }

      } else if (w_tot == 2 && wb == 2 && b_tot == 1 && bn == 1) {

         ASSERT(w_maj==0);
         ASSERT(w_min==2);
         ASSERT(b_maj==0);
         ASSERT(b_min==1);

         // KBBKN*, barely drawish (not at all?)

         mul[White] = 8; // 1/2

      } else if (w_tot-b_tot <= 1 && w_maj <= 2) {

         // no more than 1 minor up, drawish

         mul[White] = 2; // 1/8
      }

   } else if (wp == 1) { // white has one pawn

      int w_maj = wq * 2 + wr;
      int w_min = wb + wn;
      int w_tot = w_maj * 2 + w_min;

      int b_maj = bq * 2 + br;
      int b_min = bb + bn;
      int b_tot = b_maj * 2 + b_min;

      if (false) {

      } else if (b_min != 0) {

         // assume black sacrifices a minor against the lone pawn

         b_min--;
         b_tot--;

         if (false) {

         } else if (w_tot == 1) {

            ASSERT(w_maj==0);
            ASSERT(w_min==1);

            // KBK* or KNK*, always insufficient

            mul[White] = 4; // 1/4

         } else if (w_tot == 2 && wn == 2) {

            ASSERT(w_maj==0);
            ASSERT(w_min==2);

            // KNNK*, usually insufficient

            mul[White] = 4; // 1/4

         } else if (w_tot-b_tot <= 1 && w_maj <= 2) {

            // no more than 1 minor up, drawish

            mul[White] = 8; // 1/2
         }

      } else if (br != 0) {

         // assume black sacrifices a rook against the lone pawn

         b_maj--;
         b_tot -= 2;

         if (false) {

         } else if (w_tot == 1) {

            ASSERT(w_maj==0);
            ASSERT(w_min==1);

            // KBK* or KNK*, always insufficient

            mul[White] = 4; // 1/4

         } else if (w_tot == 2 && wn == 2) {

            ASSERT(w_maj==0);
            ASSERT(w_min==2);

            // KNNK*, usually insufficient

            mul[White] = 4; // 1/4

         } else if (w_tot-b_tot <= 1 && w_maj <= 2) {

            // no more than 1 minor up, drawish

            mul[White] = 8; // 1/2
         }
      }
   }

   // black multiplier

   if (bp == 0) { // black has no pawns

      int w_maj = wq * 2 + wr;
      int w_min = wb + wn;
      int w_tot = w_maj * 2 + w_min;

      int b_maj = bq * 2 + br;
      int b_min = bb + bn;
      int b_tot = b_maj * 2 + b_min;

      if (false) {

      } else if (b_tot == 1) {

         ASSERT(b_maj==0);
         ASSERT(b_min==1);

         // KBK* or KNK*, always insufficient

         mul[Black] = 0;

      } else if (b_tot == 2 && bn == 2) {

         ASSERT(b_maj==0);
         ASSERT(b_min==2);

         // KNNK*, usually insufficient

         if (w_tot != 0 || wp == 0) {
            mul[Black] = 0;
         } else { // KNNKP+, might not be draw
            mul[Black] = 1; // 1/16
         }

      } else if (b_tot == 2 && bb == 2 && w_tot == 1 && wn == 1) {

         ASSERT(b_maj==0);
         ASSERT(b_min==2);
         ASSERT(w_maj==0);
         ASSERT(w_min==1);

         // KBBKN*, barely drawish (not at all?)

         mul[Black] = 8; // 1/2

      } else if (b_tot-w_tot <= 1 && b_maj <= 2) {

         // no more than 1 minor up, drawish

         mul[Black] = 2; // 1/8
      }

   } else if (bp == 1) { // black has one pawn

      int w_maj = wq * 2 + wr;
      int w_min = wb + wn;
      int w_tot = w_maj * 2 + w_min;

      int b_maj = bq * 2 + br;
      int b_min = bb + bn;
      int b_tot = b_maj * 2 + b_min;

      if (false) {

      } else if (w_min != 0) {

         // assume white sacrifices a minor against the lone pawn

         w_min--;
         w_tot--;

         if (false) {

         } else if (b_tot == 1) {

            ASSERT(b_maj==0);
            ASSERT(b_min==1);

            // KBK* or KNK*, always insufficient

            mul[Black] = 4; // 1/4

         } else if (b_tot == 2 && bn == 2) {

            ASSERT(b_maj==0);
            ASSERT(b_min==2);

            // KNNK*, usually insufficient

            mul[Black] = 4; // 1/4

         } else if (b_tot-w_tot <= 1 && b_maj <= 2) {

            // no more than 1 minor up, drawish

            mul[Black] = 8; // 1/2
         }

      } else if (wr != 0) {

         // assume white sacrifices a rook against the lone pawn

         w_maj--;
         w_tot -= 2;

         if (false) {

         } else if (b_tot == 1) {

            ASSERT(b_maj==0);
            ASSERT(b_min==1);

            // KBK* or KNK*, always insufficient

            mul[Black] = 4; // 1/4

         } else if (b_tot == 2 && bn == 2) {

            ASSERT(b_maj==0);
            ASSERT(b_min==2);

            // KNNK*, usually insufficient

            mul[Black] = 4; // 1/4

         } else if (b_tot-w_tot <= 1 && b_maj <= 2) {

            // no more than 1 minor up, drawish

            mul[Black] = 8; // 1/2
         }
      }
   }

   // potential draw for white

   if (wt == wb+wp && wp >= 1) cflags[White] |= MatRookPawnFlag;
   if (wt == wb+wp && wb <= 1 && wp >= 1 && bt > bp) cflags[White] |= MatBishopFlag;

   if (wt == 2 && wn == 1 && wp == 1 && bt > bp) cflags[White] |= MatKnightFlag;

   // potential draw for black

   if (bt == bb+bp && bp >= 1) cflags[Black] |= MatRookPawnFlag;
   if (bt == bb+bp && bb <= 1 && bp >= 1 && wt > wp) cflags[Black] |= MatBishopFlag;

   if (bt == 2 && bn == 1 && bp == 1 && wt > wp) cflags[Black] |= MatKnightFlag;

   // draw leaf (likely draw)

   if (recog == MAT_KQKQ || recog == MAT_KRKR) {
      mul[White] = 0;
      mul[Black] = 0;
   }

   // king safety

   if (bq >= 1 && bq+br+bb+bn >= 2) cflags[White] |= MatKingFlag;
   if (wq >= 1 && wq+wr+wb+wn >= 2) cflags[Black] |= MatKingFlag;

   // phase (0: opening -> 256: endgame)

   phase = TotalPhase;

   phase -= wp * PawnPhase;
   phase -= wn * KnightPhase;
   phase -= wb * BishopPhase;
   phase -= wr * RookPhase;
   phase -= wq * QueenPhase;

   phase -= bp * PawnPhase;
   phase -= bn * KnightPhase;
   phase -= bb * BishopPhase;
   phase -= br * RookPhase;
   phase -= bq * QueenPhase;

   if (phase < 0) phase = 0;

   ASSERT(phase>=0&&phase<=TotalPhase);
   phase = (phase * 256 + (TotalPhase / 2)) / TotalPhase;

   ASSERT(phase>=0&&phase<=256);

   // material

   opening = 0;
   endgame = 0;

   opening += (wp * PawnOpening    * PawnWeight)   / 256;
   opening += (wn * KnightOpening  * KnightWeight) / 256;
   opening += (wb * BishopOpening  * BishopWeight) / 256;
   opening += (wr * RookOpening    * RookWeight)   / 256;
   opening += (wq * QueenOpening   * QueenWeight)  / 256;

   opening -= (bp * PawnOpening    * PawnWeight)   / 256;
   opening -= (bn * KnightOpening  * KnightWeight) / 256;
   opening -= (bb * BishopOpening  * BishopWeight) / 256;
   opening -= (br * RookOpening    * RookWeight)   / 256;
   opening -= (bq * QueenOpening   * QueenWeight)  / 256;

   endgame += (wp * PawnEndgame    * PawnWeight)   / 256;
   endgame += (wn * KnightEndgame  * KnightWeight) / 256;
   endgame += (wb * BishopEndgame  * BishopWeight) / 256;
   endgame += (wr * RookEndgame    * RookWeight)   / 256;
   endgame += (wq * QueenEndgame   * QueenWeight)  / 256;

   endgame -= (bp * PawnEndgame    * PawnWeight)   / 256;
   endgame -= (bn * KnightEndgame  * KnightWeight) / 256;
   endgame -= (bb * BishopEndgame  * BishopWeight) / 256;
   endgame -= (br * RookEndgame    * RookWeight)   / 256;
   endgame -= (bq * QueenEndgame   * QueenWeight)  / 256;

   // bishop pair

   if (wb >= 2) { // HACK: assumes different colours
      opening += (BishopPairOpening * BishopPairWeight) / 256;
      endgame += (BishopPairEndgame * BishopPairWeight) / 256;
   }

   if (bb >= 2) { // HACK: assumes different colours
      opening -= (BishopPairOpening * BishopPairWeight) / 256;
      endgame -= (BishopPairEndgame * BishopPairWeight) / 256;
   }

   // store info

   info->recog = recog;
   info->flags = flags;
   for (colour = 0; colour < ColourNb; colour++) info->cflags[colour] = cflags[colour];
   for (colour = 0; colour < ColourNb; colour++) info->mul[colour] = mul[colour];
   info->phase = phase;
   info->opening = (opening * MaterialWeight) / 256;
   info->endgame = (endgame * MaterialWeight) / 256;
}

// end of material.cpp

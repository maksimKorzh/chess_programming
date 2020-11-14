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

// pawn.cpp

// includes

#include <cstring>

#include "board.h"
#include "bit.h"
#include "colour.h"
#include "hash.h"
#include "option.h"
#include "pawn.h"
#include "piece.h"
#include "protocol.h"
#include "smp.h"
#include "square.h"
#include "util.h"

// constants

static const bool UseStats = false;

static const bool UseGroup = true;
static const bool UseAlone = true;

static const bool UseTable = true;
static const uint32 TableSize = 16384; // 256kB * ThreadMax

// types

typedef pawn_info_t entry_t;

struct pawn_t {
   entry_t table[ThreadMax][TableSize];
   uint32 size;
   uint32 mask;
   uint32 used;            // TODO: thread based
   sint64 read_nb;         // TODO: thread based
   sint64 read_hit;        // TODO: thread based
   sint64 write_nb;        // TODO: thread based
   sint64 write_collision; // TODO: thread based
};

// constants and variables

static int PawnStructureWeight = 256; // 100%

static const int DoubledOpening = 10;
static const int DoubledEndgame = 20;

static const int IsolatedOpening = 10;
static const int IsolatedOpeningOpen = 20;
static const int IsolatedEndgame = 20;

static const int BackwardOpening = 8;
static const int BackwardOpeningOpen = 16;
static const int BackwardEndgame = 10;

static const int AloneOpening = 5;      // TODO: tune me!
static const int AloneOpeningOpen = 10; // TODO: tune me!
static const int AloneEndgame = 10;     // TODO: tune me!

static const int GroupOpening = 2;
static const int GroupEndgame = 0;

static const int ProtectedOpeningMin = 1;
static const int ProtectedOpeningMax = 11;
static const int ProtectedEndgameMin = 2;
static const int ProtectedEndgameMax = 22;

static const int CandidateOpeningMin = 5;
static const int CandidateOpeningMax = 55;
static const int CandidateEndgameMin = 10;
static const int CandidateEndgameMax = 110;

// variables

int BitEQ[16];
int BitLT[16];
int BitLE[16];
int BitGT[16];
int BitGE[16];

int BitFirst[0x100];
int BitLast[0x100];
int BitCount[0x100];
int BitRev[0x100];

static pawn_t Pawn[1];
static int Bonus[RankNb];

static int BitForwardRank1[RankNb];
static int BitForwardRank2[RankNb];
static int BitForwardRank3[RankNb];

static int BitBackwardRank1[RankNb];
static int BitBackwardRank2[RankNb];
static int BitBackwardRank3[RankNb];

// prototypes

static void pawn_comp_info    (pawn_info_t * info, const board_t * board);

static bool pawn_is_safe      (const board_t * board, int colour, int file, int rank);
static bool pawn_is_candidate (const board_t * board, int colour, int file, int rank);

// functions

// pawn_init_bit()

void pawn_init_bit() {

   int rank;
   int first, last, count;
   int b, rev;

   // rank-indexed Bit*[]

   for (rank = 0; rank < RankNb; rank++) {

      BitEQ[rank] = 0;
      BitLT[rank] = 0;
      BitLE[rank] = 0;
      BitGT[rank] = 0;
      BitGE[rank] = 0;

      BitForwardRank1[rank] = 0;
      BitForwardRank2[rank] = 0;
      BitForwardRank3[rank] = 0;

      BitBackwardRank1[rank] = 0;
      BitBackwardRank2[rank] = 0;
      BitBackwardRank3[rank] = 0;
   }

   for (rank = Rank1; rank <= Rank8; rank++) {
      BitEQ[rank] = 1 << (rank - Rank1);
      BitLT[rank] = BitEQ[rank] - 1;
      BitLE[rank] = BitLT[rank] | BitEQ[rank];
      BitGT[rank] = BitLE[rank] ^ 0xFF;
      BitGE[rank] = BitGT[rank] | BitEQ[rank];
   }

   for (rank = Rank1; rank <= Rank8; rank++) {
      BitForwardRank1[rank] = BitEQ[rank+1];
      BitForwardRank2[rank] = BitEQ[rank+1] | BitEQ[rank+2];
      BitForwardRank3[rank] = BitEQ[rank+1] | BitEQ[rank+2] | BitEQ[rank+3];

      BitBackwardRank1[rank] = BitEQ[rank-1];
      BitBackwardRank2[rank] = BitEQ[rank-1] | BitEQ[rank-2];
      BitBackwardRank3[rank] = BitEQ[rank-1] | BitEQ[rank-2] | BitEQ[rank-3];
   }

   // bit-indexed Bit*[]

   for (b = 0; b < 0x100; b++) {

      first = Rank8; // HACK for pawn shelter
      last = Rank1; // HACK
      count = 0;
      rev = 0;

      for (rank = Rank1; rank <= Rank8; rank++) {
         if ((b & BitEQ[rank]) != 0) {
            if (rank < first) first = rank;
            if (rank > last) last = rank;
            count++;
            rev |= BitEQ[RANK_OPP(rank)];
         }
      }

      BitFirst[b] = first;
      BitLast[b] = last;
      BitCount[b] = count;
      BitRev[b] = rev;
   }
}

// pawn_init()

void pawn_init() {

   int rank;

   // UCI options

   PawnStructureWeight = (option_get_int("Pawn Structure") * 256 + 50) / 100;

   // bonus

   for (rank = 0; rank < RankNb; rank++) Bonus[rank] = 0;

   Bonus[Rank4] = 26;
   Bonus[Rank5] = 77;
   Bonus[Rank6] = 154;
   Bonus[Rank7] = 256;

   // pawn hash-table

   if (UseTable) {

      ASSERT(sizeof(entry_t)==16);

      Pawn->size = TableSize;
      Pawn->mask = TableSize - 1;

      pawn_clear();

   } else {

      Pawn->size = 0;
      Pawn->mask = 0;
   }
}

// pawn_clear()

void pawn_clear() {

   memset(Pawn->table,0,sizeof(Pawn->table));

   Pawn->used = 0;
   Pawn->read_nb = 0;
   Pawn->read_hit = 0;
   Pawn->write_nb = 0;
   Pawn->write_collision = 0;
}

// pawn_structure_option()

void pawn_structure_set_option(int weight) {

   ASSERT(weight>=0&&weight<=400); // option.cpp
   PawnStructureWeight = (weight * 256 + 50) / 100;

   pawn_clear();
}

// pawn_get_info()

void pawn_get_info(pawn_info_t * info, const board_t * board, int thread) {

   uint64 key;
   entry_t * entry;

   ASSERT(info!=NULL);
   ASSERT(board!=NULL);
   ASSERT(thread_is_ok(thread));

   // probe

   if (UseTable) {

      if (UseStats) Pawn->read_nb++;

      key = board->pawn_key;
      entry = &Pawn->table[thread][KEY_INDEX(key)&Pawn->mask];

      if (entry->lock == KEY_LOCK(key)) {

         // found

         if (UseStats) Pawn->read_hit++;

         *info = *entry;

         return;
      }
   }

   // calculation

   pawn_comp_info(info,board);

   // store

   if (UseTable) {

      if (UseStats) {
         Pawn->write_nb++;

         if (entry->lock == 0) { // HACK: assume free entry
            Pawn->used++;
         } else {
            Pawn->write_collision++;
         }
      }

      *entry = *info;
      entry->lock = KEY_LOCK(key);
   }
}

// pawn_comp_info()

static void pawn_comp_info(pawn_info_t * info, const board_t * board) {

   int colour;
   int file, rank;
   int me, opp;
   const sq_t * ptr;
   int sq;
   bool alone, backward, candidate, doubled, group, isolated, open, passed;
   int t1, t2, t3;
   bool support;
   int bits;
   int opening[ColourNb], endgame[ColourNb];
   int flags[ColourNb];
   int file_bits[ColourNb];
   int passed_bits[ColourNb];
   int single_file[ColourNb];

   ASSERT(info!=NULL);
   ASSERT(board!=NULL);

   // pawn_file[]

#if DEBUG
   for (colour = 0; colour < ColourNb; colour++) {

      int pawn_file[FileNb];

      me = colour;

      for (file = 0; file < FileNb; file++) {
         pawn_file[file] = 0;
      }

      for (ptr = &board->pawn[me][0]; (sq=*ptr) != SquareNone; ptr++) {

         file = SQUARE_FILE(sq);
         rank = PAWN_RANK(sq,me);

         ASSERT(file>=FileA&&file<=FileH);
         ASSERT(rank>=Rank2&&rank<=Rank7);

         pawn_file[file] |= BIT(rank);
      }

      for (file = 0; file < FileNb; file++) {
         if (board->pawn_file[colour][file] != pawn_file[file]) my_fatal("board->pawn_file[][]\n");
      }
   }
#endif

   // init

   for (colour = 0; colour < ColourNb; colour++) {

      opening[colour] = 0;
      endgame[colour] = 0;

      flags[colour] = 0;
      file_bits[colour] = 0;
      passed_bits[colour] = 0;
      single_file[colour] = SquareNone;
   }

   // features and scoring

   for (colour = 0; colour < ColourNb; colour++) {

      me = colour;
      opp = COLOUR_OPP(me);

      for (ptr = &board->pawn[me][0]; (sq=*ptr) != SquareNone; ptr++) {

         // init

         file = SQUARE_FILE(sq);
         rank = PAWN_RANK(sq,me);

         ASSERT(file>=FileA&&file<=FileH);
         ASSERT(rank>=Rank2&&rank<=Rank7);

         // flags

         file_bits[me] |= BIT(file);

         if (rank == Rank2) {
            flags[me] |= BackRankFlag;
         } else if (rank == Rank7) {
            flags[me] |= SevenRankFlag;
         }

         // features

         alone = false;
         backward = false;
         candidate = false;
         doubled = false;
         group = false;
         isolated = false;
         open = false;
         passed = false;

         t1 = board->pawn_file[me][file-1] | board->pawn_file[me][file+1];
         t2 = board->pawn_file[me][file] | BitRev[board->pawn_file[opp][file]];

         support = (t1 & BitEQ[rank]) != 0 || (t1 & BitEQ[rank-1]) != 0;

         // doubled

         if ((board->pawn_file[me][file] & BitLT[rank]) != 0) {
            doubled = true;
         }

         // isolated, backward and alone

         if (t1 == 0) {

            isolated = true;

         } else if (!support) {

            alone = true;

            // really alone/backward forward ?

            if ((t1 & BitForwardRank1[rank]) != 0) {

               ASSERT(rank+2<=Rank8);

               if ((t2 & BitForwardRank1[rank]) == 0 && pawn_is_safe(board,me,file,rank+1)) {
                  alone = false;
               }

            } else if (rank == Rank2 && ((t1 & BitEQ[rank+2]) != 0)) {

               ASSERT(rank+3<=Rank8);

               if ((t2 & BitForwardRank2[rank]) == 0) {
                  if (pawn_is_safe(board,me,file,rank+1) && pawn_is_safe(board,me,file,rank+2)) {
                     alone = false;
                  }
               }
            }

            // backward

            if (alone && (t1 & BitLE[rank]) == 0) {

               backward = true;

            // really alone backward ?

            } else if (UseAlone && alone && rank >= Rank4) {

               ASSERT(rank-2>=Rank2);

               if (file > FileA) {

                  t3 = board->pawn_file[me][file-1] | BitRev[board->pawn_file[opp][file-1]]; // HACK

                  if ((t1 & BitEQ[rank-2]) != 0) {
                     if ((t3 & BitBackwardRank1[rank]) == 0) {
                        if (pawn_is_safe(board,me,file-1,rank-1)) {
                           alone = false;
                        }
                     }

                  } else if (rank == Rank5 && (t1 & BitEQ[rank-3]) != 0) {

                     ASSERT(rank-3==Rank2);

                     if ((t3 & BitBackwardRank2[rank]) == 0) {
                        if (pawn_is_safe(board,me,file-1,rank-1) && pawn_is_safe(board,me,file-1,rank-2)) {
                           alone = false;
                        }
                     }
                  }
               }

               if (file < FileH && alone) {

                  t3 = board->pawn_file[me][file+1] | BitRev[board->pawn_file[opp][file+1]]; // HACK

                  if ((t1 & BitEQ[rank-2]) != 0) {
                     if ((t3 & BitBackwardRank1[rank]) == 0) {
                        if (pawn_is_safe(board,me,file+1,rank-1)) {
                           alone = false;
                        }
                     }

                  } else if (rank == Rank5 && (t1 & BitEQ[rank-3]) != 0) {

                     ASSERT(rank-3==Rank2);

                     if ((t3 & BitBackwardRank2[rank]) == 0) {
                        if (pawn_is_safe(board,me,file+1,rank-1) && pawn_is_safe(board,me,file+1,rank-2)) {
                           alone = false;
                        }
                     }
                  }
               }
            }

         } else if (support) {

            if ((t1 & BitEQ[rank]) != 0 && pawn_is_safe(board,me,file,rank)) group = true;

         } else {

            ASSERT(false);
         }

         // open, candidate and passed

         if ((t2 & BitGT[rank]) == 0) {

            open = true;

            if (((BitRev[board->pawn_file[opp][file-1]] | BitRev[board->pawn_file[opp][file+1]]) & BitGT[rank]) == 0) {

               passed = true;
               passed_bits[me] |= BIT(file);

            } else {

               // candidate?

               if (pawn_is_candidate(board,colour,file,rank)) candidate = true;
            }
         }

         // score

         if (doubled) {
            opening[me] -= DoubledOpening;
            endgame[me] -= DoubledEndgame;
         }

         if (isolated) {
            if (open) {
               opening[me] -= IsolatedOpeningOpen;
               endgame[me] -= IsolatedEndgame;
            } else {
               opening[me] -= IsolatedOpening;
               endgame[me] -= IsolatedEndgame;
            }
         }

         if (backward) {
            if (open) {
               opening[me] -= BackwardOpeningOpen;
               endgame[me] -= BackwardEndgame;
            } else {
               opening[me] -= BackwardOpening;
               endgame[me] -= BackwardEndgame;
            }
         } else if (UseAlone && alone) {
            if (open) {
               opening[me] -= AloneOpeningOpen;
               endgame[me] -= AloneEndgame;
            } else {
               opening[me] -= AloneOpening;
               endgame[me] -= AloneEndgame;
            }
         }

         if (support) {
            if (UseGroup) {
               if (group) {
                  opening[me] += GroupOpening;
                  endgame[me] += GroupEndgame;
               } else {
                  opening[me] += GroupOpening / 2;
                  endgame[me] += GroupEndgame / 2;
               }
            }

            if (passed) { // HACK: should be in eval_passer()
               opening[me] += quad(ProtectedOpeningMin,ProtectedOpeningMax,rank);
               endgame[me] += quad(ProtectedEndgameMin,ProtectedEndgameMax,rank);
            }
         }

         if (candidate) {
            opening[me] += quad(CandidateOpeningMin,CandidateOpeningMax,rank);
            endgame[me] += quad(CandidateEndgameMin,CandidateEndgameMax,rank);
         }
      }
   }

   // store info

   info->opening = ((opening[White] - opening[Black]) * PawnStructureWeight) / 256;
   info->endgame = ((endgame[White] - endgame[Black]) * PawnStructureWeight) / 256;

   for (colour = 0; colour < ColourNb; colour++) {

      me = colour;
      opp = COLOUR_OPP(me);

      // draw flags

      bits = file_bits[me];

      if (bits != 0 && (bits & (bits-1)) == 0) { // one set bit

         file = BIT_FIRST(bits);
         rank = BIT_FIRST(board->pawn_file[me][file]);
         ASSERT(rank>=Rank2);

         if (((BitRev[board->pawn_file[opp][file-1]] | BitRev[board->pawn_file[opp][file+1]]) & BitGT[rank]) == 0) {
            rank = BIT_LAST(board->pawn_file[me][file]);
            single_file[me] = SQUARE_MAKE(file,rank);
         }
      }

      info->flags[colour] = flags[colour];
      info->passed_bits[colour] = passed_bits[colour];
      info->single_file[colour] = single_file[colour];
   }
}

// pawn_is_safe()

static bool pawn_is_safe(const board_t * board, int colour, int file, int rank) {

   int me, opp;
   int n;

   ASSERT(board!=NULL);
   ASSERT(COLOUR_IS_OK(colour));
   ASSERT(file>=FileA&&file<=FileH);
   ASSERT(rank>=Rank2&&rank<=Rank8);

   // init

   me = colour;
   opp = COLOUR_OPP(me);
   n = 0;

   // count

   n += BIT_COUNT(board->pawn_file[me][file-1]&BitEQ[rank-1]);
   n += BIT_COUNT(board->pawn_file[me][file+1]&BitEQ[rank-1]);

   n -= BIT_COUNT(BitRev[board->pawn_file[opp][file-1]]&BitEQ[rank+1]);
   n -= BIT_COUNT(BitRev[board->pawn_file[opp][file+1]]&BitEQ[rank+1]);

   return n >= 0;
}

// pawn_is_candidate()

static bool pawn_is_candidate(const board_t * board, int colour, int file, int rank) {

   int me, opp;
   int n;

   ASSERT(board!=NULL);
   ASSERT(COLOUR_IS_OK(colour));
   ASSERT(file>=FileA&&file<=FileH);
   ASSERT(rank>=Rank2&&rank<=Rank8);

   // init

   me = colour;
   opp = COLOUR_OPP(me);
   n = 0;

   // count

   n += BIT_COUNT(board->pawn_file[me][file-1]&BitLE[rank]);
   n += BIT_COUNT(board->pawn_file[me][file+1]&BitLE[rank]);

   n -= BIT_COUNT(BitRev[board->pawn_file[opp][file-1]]&BitGT[rank]);
   n -= BIT_COUNT(BitRev[board->pawn_file[opp][file+1]]&BitGT[rank]);

   if (n >= 0) return pawn_is_safe(board,me,file,rank); // safe ?

   return false;
}

// quad()

int quad(int y_min, int y_max, int x) {

   int y;

   ASSERT(y_min>=0&&y_min<=y_max&&y_max<=+32767);
   ASSERT(x>=Rank2&&x<=Rank7);

   y = y_min + ((y_max - y_min) * Bonus[x] + 128) / 256;
   ASSERT(y>=y_min&&y<=y_max);

   return y;
}

// end of pawn.cpp


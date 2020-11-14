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

// sort.cpp

// includes

#include "attack.h"
#include "board.h"
#include "colour.h"
#include "list.h"
#include "move.h"
#include "move_check.h"
#include "move_evasion.h"
#include "move_gen.h"
#include "move_legal.h"
#include "piece.h"
#include "search.h"
#include "see.h"
#include "sort.h"
#include "smp.h"
#include "util.h"
#include "value.h"

// constants

static const bool UseMoveEval = true;

static const int KillerNb = 2;

static const int HistorySize = 12 * 64;
static const int HistoryMax = 16384;
static const int HistoryValue = 8192 * 4;

static const int HistoryEvalMax = 2048;
static const int HistoryEvalValue = 0;

static const int TransScore   = +32766;
static const int GoodScore    = +30000;
static const int KillerScore  = +28000;
static const int HistoryScore =      0;
static const int BadScore     = -28000;

static const int CODE_SIZE = 256;

// macros

#define HISTORY_INC(depth)  ((depth)*(depth))
#define HISTORY_EVAL(value) ((value)==(HistoryEvalValue)?(0):(value))

// types

enum gen_t {
   GEN_ERROR,
   GEN_LEGAL_EVASION,
   GEN_TRANS,
   GEN_CAPTURE,
   GEN_KILLER,
   GEN_QUIET,
   GEN_BAD,
   GEN_EVASION_QS,
   GEN_CAPTURE_QS,
   GEN_CHECK_QS,
   GEN_END
};

enum test_t {
   TEST_ERROR,
   TEST_NONE,
   TEST_LEGAL,
   TEST_TRANS_KILLER,
   TEST_CAPTURE,
   TEST_KILLER,
   TEST_QUIET,
   TEST_BAD,
   TEST_CAPTURE_QS,
   TEST_CHECK_QS
};

// variables

static int PosLegalEvasion;
static int PosSEE;

static int PosEvasionQS;
static int PosCheckQS;
static int PosCaptureQS;

static int Code[CODE_SIZE];

static uint16 Killer[ThreadMax][HeightMax][KillerNb];

static sint32 History[HistorySize];
static uint16 HistHit[HistorySize];
static uint16 HistTot[HistorySize];
static sint16 HistEval[HistorySize];

// prototypes

static void note_quiet_moves  (list_t * list, const board_t * board);
static void note_moves_simple (list_t * list, const board_t * board, int trans_killer);
static void note_mvv_lva      (list_t * list, const board_t * board);

static int  move_value        (int move, const board_t * board, int height, int trans_killer, int thread);
static int  capture_value     (int move, const board_t * board);
static int  quiet_move_value  (int move, const board_t * board);
static int  move_value_simple (int move, const board_t * board, int trans_killer);

static int  history_prob      (int move, const board_t * board);

static bool capture_is_good   (int move, const board_t * board);

static int  mvv_lva           (int move, const board_t * board);

static int  history_index     (int move, const board_t * board);

// functions

// sort_init()

void sort_init() {

   int thread;
   int i, height;
   int pos;

   // killer

   for (thread = 0; thread < ThreadMax; thread++) {
      for (height = 0; height < HeightMax; height++) {
         for (i = 0; i < KillerNb; i++) Killer[thread][height][i] = MoveNone;
      }
   }

   // move eval

   for (i = 0; i < HistorySize; i++) HistEval[i] = HistoryEvalValue;

   // history

   for (i = 0; i < HistorySize; i++) History[i] = 0;

   for (i = 0; i < HistorySize; i++) {
      HistHit[i] = 1;
      HistTot[i] = 1;
   }

   // Code[]

   for (pos = 0; pos < CODE_SIZE; pos++) Code[pos] = GEN_ERROR;

   pos = 0;

   // main search

   PosLegalEvasion = pos;
   Code[pos++] = GEN_LEGAL_EVASION;

   for (thread = 0; thread < SlaveMax; thread++) Code[pos++] = GEN_END; // for lock less move generation

   PosSEE = pos;
   Code[pos++] = GEN_TRANS;
   Code[pos++] = GEN_CAPTURE;
   Code[pos++] = GEN_KILLER;
   Code[pos++] = GEN_QUIET;
   Code[pos++] = GEN_BAD;

   for (thread = 0; thread < SlaveMax; thread++) Code[pos++] = GEN_END; // for lock less move generation

   // quiescence search

   PosEvasionQS = pos;
   Code[pos++] = GEN_EVASION_QS;

   for (thread = 0; thread < SlaveMax; thread++) Code[pos++] = GEN_END; // for lock less move generation

   PosCheckQS = pos;
   Code[pos++] = GEN_TRANS;
   Code[pos++] = GEN_CAPTURE_QS;
   Code[pos++] = GEN_CHECK_QS;

   for (thread = 0; thread < SlaveMax; thread++) Code[pos++] = GEN_END; // for lock less move generation

   PosCaptureQS = pos;
   Code[pos++] = GEN_TRANS;
   Code[pos++] = GEN_CAPTURE_QS;

   for (thread = 0; thread < SlaveMax; thread++) Code[pos++] = GEN_END;// for lock less move generation

   ASSERT(pos<CODE_SIZE);
}

// move_eval()

void move_eval(const board_t * board, int eval) {

   int index;
   int best_value, value;
   int move;

   ASSERT(board!=NULL);
   ASSERT(value_is_ok(eval));

   move = board->move;
   ASSERT(move_is_ok(move));

   // exceptions

   if ((move & (1 << 15)) != 0)        return;
   if (MOVE_TO(move) == board->cap_sq) return;
   if (MOVE_IS_CASTLE(move))           return;

   ASSERT(!MOVE_IS_PROMOTE(move));
   ASSERT(!MOVE_IS_EN_PASSANT(move));
   ASSERT(board->cap_sq==Empty);
   ASSERT(!board_is_check(board));

   index = PIECE_TO_12(board->square[MOVE_TO(move)]) * 64 + SQUARE_TO_64(MOVE_TO(move));
   ASSERT(index>=0&&index<HistorySize);

   if (eval > ValueQueen) {
      value = ValueQueen;
   } else if (eval < -ValueQueen) {
      value = -ValueQueen;
   } else {
      value = eval;
   }

   best_value = HistEval[index];

   ASSERT(best_value<=HistoryEvalMax);
   ASSERT(best_value>=-HistoryEvalMax);

   if (value >= best_value) {

      if (value == HistoryEvalValue) value++;
      HistEval[index] = value;

   } else {

      if (HistEval[index] > -ValueQueen) {

         if (best_value == HistoryEvalValue+1) {
            HistEval[index] -= 2;
         } else {
            HistEval[index]--;
         }
      }
   }

   ASSERT(HistEval[index]!=HistoryEvalValue);
   ASSERT(HistEval[index]<=HistoryEvalMax);
   ASSERT(HistEval[index]>=-HistoryEvalMax);
}

// sort_init()

void sort_init(sort_t * sort, board_t * board, const attack_t * attack, int depth, int height, int trans_killer, int thread) {

   ASSERT(sort!=NULL);
   ASSERT(board!=NULL);
   ASSERT(attack!=NULL);
   ASSERT(depth_is_ok(depth));
   ASSERT(height_is_ok(height));
   ASSERT(trans_killer==MoveNone||move_is_ok(trans_killer));
   ASSERT(thread_is_ok(thread));

   sort->board = board;
   sort->attack = attack;

   sort->depth = depth;
   sort->height = height;

   sort->trans_killer = trans_killer;
   sort->killer_1 = Killer[thread][sort->height][0];
   sort->killer_2 = Killer[thread][sort->height][1];

   if (ATTACK_IN_CHECK(sort->attack)) {

      gen_legal_evasions(sort->list,sort->board,sort->attack);
      note_moves(sort->list,sort->board,sort->height,sort->trans_killer,thread);
      list_sort(sort->list);

      sort->gen = PosLegalEvasion + 1;
      sort->test = TEST_NONE;

   } else { // not in check

      LIST_CLEAR(sort->list);
      sort->gen = PosSEE;
   }

   sort->pos = 0;
}

// sort_next()

int sort_next(sort_t * sort) {

   int move;
   int gen;

   ASSERT(sort!=NULL);

   while (true) {

      while (sort->pos < LIST_SIZE(sort->list)) {

         // next move

         move = LIST_MOVE(sort->list,sort->pos);
         sort->value = 65536; // default score
         sort->pos++;

         ASSERT(move!=MoveNone);

         // test

         if (false) {

         } else if (sort->test == TEST_NONE) {

            // Evasion (no-op)

         } else if (sort->test == TEST_TRANS_KILLER) {

            if (!move_is_pseudo(move,sort->board)) continue;
            if (!pseudo_is_legal(move,sort->board)) continue;

         } else if (sort->test == TEST_CAPTURE) {

            ASSERT(MOVE_IS_TACTICAL(move,sort->board));

            if (move == sort->trans_killer) continue;

            if (!capture_is_good(move,sort->board)) {
               LIST_ADD(sort->bad,move);
               continue;
            }

            if (!pseudo_is_legal(move,sort->board)) continue;

         } else if (sort->test == TEST_KILLER) {

            if (move == sort->trans_killer) continue;
            if (!quiet_is_pseudo(move,sort->board)) continue;
            if (!pseudo_is_legal(move,sort->board)) continue;

            ASSERT(!MOVE_IS_TACTICAL(move,sort->board));

            sort->value = 32768;

         } else if (sort->test == TEST_QUIET) {

            ASSERT(!MOVE_IS_TACTICAL(move,sort->board));

            if (move == sort->trans_killer) continue;
            if (move == sort->killer_1) continue;
            if (move == sort->killer_2) continue;
            if (!pseudo_is_legal(move,sort->board)) continue;

            sort->value = history_prob(move,sort->board);

         } else if (sort->test == TEST_BAD) {

            ASSERT(MOVE_IS_TACTICAL(move,sort->board));
            ASSERT(!capture_is_good(move,sort->board));

            ASSERT(move!=sort->trans_killer);
            if (!pseudo_is_legal(move,sort->board)) continue;

         } else {

            ASSERT(false);

            return MoveNone;
         }

         ASSERT(pseudo_is_legal(move,sort->board));

         return move;
      }

      // next stage

      gen = Code[sort->gen++];

      if (false) {

      } else if (gen == GEN_TRANS) {

         LIST_CLEAR(sort->list);
         if (sort->trans_killer != MoveNone) LIST_ADD(sort->list,sort->trans_killer);

         sort->test = TEST_TRANS_KILLER;

      } else if (gen == GEN_CAPTURE) {

         gen_captures(sort->list,sort->board);
         note_mvv_lva(sort->list,sort->board);
         list_sort(sort->list);

         LIST_CLEAR(sort->bad);

         sort->test = TEST_CAPTURE;

      } else if (gen == GEN_KILLER) {

         LIST_CLEAR(sort->list);
         if (sort->killer_1 != MoveNone) LIST_ADD(sort->list,sort->killer_1);
         if (sort->killer_2 != MoveNone) LIST_ADD(sort->list,sort->killer_2);

         sort->test = TEST_KILLER;

      } else if (gen == GEN_QUIET) {

         gen_quiet_moves(sort->list,sort->board);
         note_quiet_moves(sort->list,sort->board);
         list_sort(sort->list);

         sort->test = TEST_QUIET;

      } else if (gen == GEN_BAD) {

         list_copy(sort->list,sort->bad);

         sort->test = TEST_BAD;

      } else {

         ASSERT(gen==GEN_END);

         return MoveNone;
      }

      sort->pos = 0;
   }
}

// sort_init_qs()

void sort_init_qs(sort_t * sort, board_t * board, const attack_t * attack, int depth, int node_type, int trans_killer) {

   ASSERT(sort!=NULL);
   ASSERT(board!=NULL);
   ASSERT(attack!=NULL);
   ASSERT(depth_is_ok(depth));
   ASSERT(node_type==NodePV||node_type==NodeCut||node_type==NodeAll);
   ASSERT(trans_killer==MoveNone||move_is_ok(trans_killer));

   sort->board = board;
   sort->attack = attack;

   sort->depth = depth;
   sort->value = node_type; // HACK
   sort->trans_killer = trans_killer;

   if (ATTACK_IN_CHECK(sort->attack)) {
      sort->gen = PosEvasionQS;
   } else if (depth >= 0) {
      sort->gen = PosCheckQS;
   } else {
      sort->gen = PosCaptureQS;
   }

   LIST_CLEAR(sort->list);
   sort->pos = 0;
}

// sort_next_qs()

int sort_next_qs(sort_t * sort) {

   int move;
   int gen;

   ASSERT(sort!=NULL);

   while (true) {

      while (sort->pos < LIST_SIZE(sort->list)) {

         // next move

         move = LIST_MOVE(sort->list,sort->pos);
         sort->pos++;

         ASSERT(move!=MoveNone);

         // test

         if (false) {

         } else if (sort->test == TEST_LEGAL) {

            if (!pseudo_is_legal(move,sort->board)) continue;

         } else if (sort->test == TEST_TRANS_KILLER) {

            if (!move_is_pseudo(move,sort->board)) continue;
            if (!pseudo_is_legal(move,sort->board)) continue;

            // check

            if (!MOVE_IS_TACTICAL(move,sort->board)
             && (!move_is_check(move,sort->board) || sort->depth < 0)) continue;

         } else if (sort->test == TEST_CAPTURE_QS) {

            ASSERT(MOVE_IS_TACTICAL(move,sort->board));
            ASSERT(sort->value==NodePV||sort->value==NodeCut||sort->value==NodeAll);

            if (move == sort->trans_killer) continue;
            if (sort->value != NodePV && !capture_is_good(move,sort->board)) continue;
            if (!pseudo_is_legal(move,sort->board)) continue;

         } else if (sort->test == TEST_CHECK_QS) {

            ASSERT(!MOVE_IS_TACTICAL(move,sort->board));
            ASSERT(move_is_check(move,sort->board));

            if (move == sort->trans_killer) continue;
            if (see_move(move,sort->board,0) < 0) continue;
            if (!pseudo_is_legal(move,sort->board)) continue;

         } else {

            ASSERT(false);

            return MoveNone;
         }

         ASSERT(pseudo_is_legal(move,sort->board));

         return move;
      }

      // next stage

      gen = Code[sort->gen++];

      if (false) {

      } else if (gen == GEN_EVASION_QS) {

         gen_pseudo_evasions(sort->list,sort->board,sort->attack);
         note_moves_simple(sort->list,sort->board,sort->trans_killer);
         list_sort(sort->list);

         sort->test = TEST_LEGAL;

      } else if (gen == GEN_TRANS) {

         LIST_CLEAR(sort->list);
         if (sort->trans_killer != MoveNone) LIST_ADD(sort->list,sort->trans_killer);

         sort->test = TEST_TRANS_KILLER;

      } else if (gen == GEN_CAPTURE_QS) {

         gen_captures(sort->list,sort->board);
         note_mvv_lva(sort->list,sort->board);
         list_sort(sort->list);

         sort->test = TEST_CAPTURE_QS;

      } else if (gen == GEN_CHECK_QS) {

         gen_quiet_checks(sort->list,sort->board);

         sort->test = TEST_CHECK_QS;

      } else {

         ASSERT(gen==GEN_END);

         return MoveNone;
      }

      sort->pos = 0;
   }
}

// good_move()

void good_move(int move, const board_t * board, int depth, int height, int thread, bool cut) {

   int index;
   int i;

   ASSERT(move_is_ok(move));
   ASSERT(board!=NULL);
   ASSERT(depth_is_ok(depth));
   ASSERT(height_is_ok(height));
   ASSERT(thread_is_ok(thread));
   ASSERT(cut==true||cut==false);

   if (MOVE_IS_TACTICAL(move,board)) return;

   // killer

   if (Killer[thread][height][0] != move) {
       Killer[thread][height][1] = Killer[thread][height][0];
       Killer[thread][height][0] = move;
   }

   ASSERT(Killer[thread][height][0]==move);
   ASSERT(Killer[thread][height][1]!=move);

   // history

   index = history_index(move,board);

   History[index] += HISTORY_INC(depth);

   if (History[index] > HistoryValue) {
      for (i = 0; i < HistorySize; i++) {
         if (History[i] >= 0) {
            History[i] = (History[i] + 1) / 2;
         } else {
            History[i] = (History[i] - 1) / 2;
         }
      }
   }

   if (cut) {
      HistHit[index]++;
      HistTot[index]++;

      if (HistTot[index] >= HistoryMax) {
         HistHit[index] = (HistHit[index] + 1) / 2;
         HistTot[index] = (HistTot[index] + 1) / 2;
      }

#if DEBUG
      if (thread_number() == 1) {
         ASSERT(HistHit[index]<=HistTot[index]);
         ASSERT(HistTot[index]<HistoryMax);
      }
#endif
   }

#if DEBUG
   if (thread_number() > 1) {
      ASSERT(History[index]<=HistoryValue+8192&&History[index]>=-HistoryValue-8192);
   } else {
      ASSERT(History[index]<=HistoryValue&&History[index]>=-HistoryValue);
   }
#endif
}

// bad_move()

void bad_move(int move, const board_t * board, int depth) {

   int index;
   int i;

   ASSERT(move_is_ok(move));
   ASSERT(board!=NULL);
   ASSERT(depth_is_ok(depth));

   if (MOVE_IS_TACTICAL(move,board)) return;

   // history

   index = history_index(move,board);

   History[index] -= HISTORY_INC(depth);

   if (History[index] < -HistoryValue) {
      for (i = 0; i < HistorySize; i++) {
         if (History[i] >= 0) {
            History[i] = (History[i] + 1) / 2;
         } else {
            History[i] = (History[i] - 1) / 2;
         }
      }
   }

   HistTot[index]++;

   if (HistTot[index] >= HistoryMax) {
      HistHit[index] = (HistHit[index] + 1) / 2;
      HistTot[index] = (HistTot[index] + 1) / 2;
   }

#if DEBUG
   if (thread_number() == 1) {
      ASSERT(History[index]<=HistoryValue&&History[index]>=-HistoryValue);
      ASSERT(HistHit[index]<=HistTot[index]);
      ASSERT(HistTot[index]<HistoryMax);
   } else {
      ASSERT(History[index]<=HistoryValue+8192&&History[index]>=-HistoryValue-8192);
   }
#endif
}

// killer_copy()

void killer_copy(int dst, int src, int height) {

   ASSERT(thread_is_ok(dst));
   ASSERT(thread_is_ok(src));
   ASSERT(height_is_ok(height));

   ASSERT(dst!=src);

   for ( ; height < SearchCurrent->max_depth[src]; height++) {
       Killer[dst][height][0] = Killer[src][height][0];
       Killer[dst][height][1] = Killer[src][height][1];
   }
}

// note_moves()

void note_moves(list_t * list, const board_t * board, int height, int trans_killer, int thread) {

   int size;
   int i, move;

   ASSERT(list_is_ok(list));
   ASSERT(board!=NULL);
   ASSERT(height_is_ok(height));
   ASSERT(trans_killer==MoveNone||move_is_ok(trans_killer));
   ASSERT(thread_is_ok(thread));

   size = LIST_SIZE(list);

   if (size >= 2) {
      for (i = 0; i < size; i++) {
         move = LIST_MOVE(list,i);
         list->value[i] = move_value(move,board,height,trans_killer,thread);
      }
   }
}

// note_quiet_moves()

static void note_quiet_moves(list_t * list, const board_t * board) {

   int size;
   int i, move;

   ASSERT(list_is_ok(list));
   ASSERT(board!=NULL);

   size = LIST_SIZE(list);

   if (size >= 2) {
      for (i = 0; i < size; i++) {
         move = LIST_MOVE(list,i);
         list->value[i] = quiet_move_value(move,board);
      }
   }
}

// note_moves_simple()

static void note_moves_simple(list_t * list, const board_t * board, int trans_killer) {

   int size;
   int i, move;

   ASSERT(list_is_ok(list));
   ASSERT(board!=NULL);
   ASSERT(trans_killer==MoveNone||move_is_ok(trans_killer));

   size = LIST_SIZE(list);

   if (size >= 2) {
      for (i = 0; i < size; i++) {
         move = LIST_MOVE(list,i);
         list->value[i] = move_value_simple(move,board,trans_killer);
      }
   }
}

// note_mvv_lva()

static void note_mvv_lva(list_t * list, const board_t * board) {

   int size;
   int i, move;

   ASSERT(list_is_ok(list));
   ASSERT(board!=NULL);

   size = LIST_SIZE(list);

   if (size >= 2) {
      for (i = 0; i < size; i++) {
         move = LIST_MOVE(list,i);
         list->value[i] = mvv_lva(move,board);
      }
   }
}

// move_value()

static int move_value(int move, const board_t * board, int height, int trans_killer, int thread) {

   int value;

   ASSERT(move_is_ok(move));
   ASSERT(board!=NULL);
   ASSERT(height_is_ok(height));
   ASSERT(trans_killer==MoveNone||move_is_ok(trans_killer));
   ASSERT(thread_is_ok(thread));

   if (false) {
   } else if (move == trans_killer) { // transposition table killer
      value = TransScore;
   } else if (MOVE_IS_TACTICAL(move,board)) { // capture or promote
      value = capture_value(move,board);
   } else if (move == Killer[thread][height][0]) { // killer 1
      value = KillerScore;
   } else if (move == Killer[thread][height][1]) { // killer 2
      value = KillerScore - 1;
   } else { // quiet move
      value = quiet_move_value(move,board);
   }

   return value;
}

// capture_value()

static int capture_value(int move, const board_t * board) {

   int value;

   ASSERT(move_is_ok(move));
   ASSERT(board!=NULL);

   ASSERT(MOVE_IS_TACTICAL(move,board));

   value = mvv_lva(move,board);

   if (capture_is_good(move,board)) {
      value += GoodScore;
      ASSERT(value>KillerScore&&value<TransScore);
   } else {
      value += BadScore;
      ASSERT(value>BadScore-100&&value<BadScore+100);
   }

   return value;
}

// quiet_move_value()

static int quiet_move_value(int move, const board_t * board) {

   int value;
   int index;

   ASSERT(move_is_ok(move));
   ASSERT(board!=NULL);

   ASSERT(!MOVE_IS_TACTICAL(move,board));

   index = history_index(move,board);
   value = History[index];

#if DEBUG
   if (thread_number() > 1) {
      ASSERT(value<=HistoryValue+8192&&value>=-HistoryValue-8192);
   } else {
      ASSERT(value<=HistoryValue&&value>=-HistoryValue);
   }
#endif

   if (UseMoveEval) {
      if (value > HistoryScore) value += HistoryEvalMax;
      value += HISTORY_EVAL(HistEval[index]);
   }

   value /= HeightFull;
   ASSERT(value>=BadScore+100&&value<KillerScore-4);

   return value;
}

// move_value_simple()

static int move_value_simple(int move, const board_t * board, int trans_killer) {

   int value;

   ASSERT(move_is_ok(move));
   ASSERT(board!=NULL);
   ASSERT(trans_killer==MoveNone||move_is_ok(trans_killer));

   if (false) {
   } else if (move == trans_killer) { // transposition table killer
      value = TransScore;
   } else if (MOVE_IS_TACTICAL(move,board)) { // capture or promote
      value = GoodScore + mvv_lva(move,board);
   } else { // quiet move
      value = quiet_move_value(move,board);
   }

   return value;
}

// history_prob()

static int history_prob(int move, const board_t * board) {

   int value;
   int index;

   ASSERT(move_is_ok(move));
   ASSERT(board!=NULL);

   ASSERT(!MOVE_IS_TACTICAL(move,board));

   index = history_index(move,board);

#if DEBUG
   if (thread_number() == 1) {
      ASSERT(HistHit[index]<=HistTot[index]);
      ASSERT(HistTot[index]<HistoryMax);
   }
#endif

   value = (HistHit[index] * 16384) / HistTot[index];

   if (value > 16384) value = 16384; // HACK: Thread related
   ASSERT(value>=0&&value<=16384);

   return value;
}

// capture_is_good()

static bool capture_is_good(int move, const board_t * board) {

   int piece, capture;

   ASSERT(move_is_ok(move));
   ASSERT(board!=NULL);

   ASSERT(MOVE_IS_TACTICAL(move,board));

   // special cases

   if (MOVE_IS_EN_PASSANT(move)) return true;
   if (MOVE_IS_UNDER_PROMOTE(move)) return false; // REMOVE ME?

   // captures and queen promotes

   capture = board->square[MOVE_TO(move)];

   if (capture != Empty) {

      // capture

      ASSERT(move_is_capture(move,board));

      if (MOVE_IS_PROMOTE(move)) return true; // promote-capture

      piece = board->square[MOVE_FROM(move)];
      if (VALUE_PIECE(capture) >= VALUE_PIECE(piece)) return true;
   }

   return see_move(move,board,0) >= 0;
}

// mvv_lva()

static int mvv_lva(int move, const board_t * board) {

   int piece, capture, promote;
   int value;

   ASSERT(move_is_ok(move));
   ASSERT(board!=NULL);

   ASSERT(MOVE_IS_TACTICAL(move,board));

   if (MOVE_IS_EN_PASSANT(move)) { // en-passant capture

      value = 5; // PxP

   } else if ((capture = board->square[MOVE_TO(move)]) != Empty) { // normal capture

      piece = board->square[MOVE_FROM(move)];

      value = PIECE_ORDER(capture) * 6 - PIECE_ORDER(piece) + 5;
      ASSERT(value>=0&&value<30);

   } else { // promote

      ASSERT(MOVE_IS_PROMOTE(move));

      promote = move_promote(move);

      value = PIECE_ORDER(promote) - 5;
      ASSERT(value>=-4&&value<0);
   }

   ASSERT(value>=-4&&value<+30);

   return value;
}

// history_index()

static int history_index(int move, const board_t * board) {

   int index;

   ASSERT(move_is_ok(move));
   ASSERT(board!=NULL);

   ASSERT(!MOVE_IS_TACTICAL(move,board));

   index = PIECE_TO_12(board->square[MOVE_FROM(move)]) * 64 + SQUARE_TO_64(MOVE_TO(move));

   ASSERT(index>=0&&index<HistorySize);

   return index;
}

// end of sort.cpp


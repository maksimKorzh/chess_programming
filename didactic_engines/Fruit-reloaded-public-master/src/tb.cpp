/* Fruit reloaded, a UCI chess playing engine derived from Fruit 2.1
 *
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

// tb.cpp

// includes

#ifdef _WIN32
#  include <windows.h>
#else
#  include <cstring>
#  include <dlfcn.h>
#endif

#include "board.h"
#include "colour.h"
#include "piece.h"
#include "protocol.h"
#include "square.h"
#include "tb.h"
#include "tb_index.h"
#include "util.h"
#include "value.h"

// constants

#ifdef _WIN32
   static const char EgtbLib[] = "egtb.dll";
#else
   static const char EgtbLib[] = "egtb.so";
#endif

static const int ValueDraw = 0;

static const int CacheSize = 8 * 1024 * 1024; // option.cpp

static const int TbPieceNb = 3;
static const int EpSquare = 127;

static const sint8 TbFrom[12] = { 0, 5, 1, 6, 2, 7, 3, 8, 4, 9, 0, 0 };
static const sint8 TbPiece[12] = { 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5 };

// types

struct tablebase_t {
   bool init;
   char path[256];
   uint32 size;
   void * cache;
   int piece_nb;
   sint64 read_nb;
   sint64 read_hit;
};

// variables

static INSTANCE LibHandler;

static tablebase_t TableBase[1];

// prototypes (import library)

static IDescFindFromCounters  IDescFindFromCountersPtr;
static VTbCloseFiles          VTbCloseFilesPtr;
static FTbSetCacheSize        FTbSetCacheSizePtr;
static FRegisteredFun         FRegisteredFunPtr;
static PfnIndCalcFun          PfnIndCalcFunPtr;
static FReadTableToMemory     FReadTableToMemoryPtr;
static FMapTableToMemory      FMapTableToMemoryPtr;
static L_TbtProbeTable        L_TbtProbeTablePtr;
static IInitializeTb          IInitializeTbPtr;

// functions

// tb_init()

void tb_init(void) {

   // init

   TableBase->init = false;
   TableBase->cache = NULL;
   TableBase->size = CacheSize;
   TableBase->piece_nb = 0;
   TableBase->read_hit = 0;
   TableBase->read_nb = 0;

   // load egtb library

   LibHandler = LOAD_LIB(EgtbLib);

   if (LibHandler != NULL) {

      send("egtb library loaded");

      IDescFindFromCountersPtr =    (IDescFindFromCounters) GET_ENTRY(LibHandler,"IDescFindFromCounters");
      VTbCloseFilesPtr =            (VTbCloseFiles)         GET_ENTRY(LibHandler,"VTbCloseFiles");
      FTbSetCacheSizePtr =          (FTbSetCacheSize)       GET_ENTRY(LibHandler,"FTbSetCacheSize");
      FRegisteredFunPtr =           (FRegisteredFun)        GET_ENTRY(LibHandler,"FRegisteredFun");
      PfnIndCalcFunPtr =            (PfnIndCalcFun)         GET_ENTRY(LibHandler,"PfnIndCalcFun");
      FReadTableToMemoryPtr =       (FReadTableToMemory)    GET_ENTRY(LibHandler,"FReadTableToMemory");
      FMapTableToMemoryPtr =        (FMapTableToMemory)     GET_ENTRY(LibHandler,"FMapTableToMemory");
      L_TbtProbeTablePtr =          (L_TbtProbeTable)       GET_ENTRY(LibHandler,"L_TbtProbeTable");
      IInitializeTbPtr =            (IInitializeTb)         GET_ENTRY(LibHandler,"IInitializeTb");

      if (IDescFindFromCountersPtr == NULL)  my_fatal("tb_init(): IDescFindFromCounters() not found\n");
      if (VTbCloseFilesPtr == NULL)          my_fatal("tb_init(): VTbCloseFiles() not found\n");
      if (FTbSetCacheSizePtr == NULL)        my_fatal("tb_init(): FTbSetCacheSize() not found\n");
      if (FRegisteredFunPtr == NULL)         my_fatal("tb_init(): FRegisteredFun() not found\n");
      if (PfnIndCalcFunPtr == NULL)          my_fatal("tb_init(): PfnIndCalcFun() not found\n");
      if (FReadTableToMemoryPtr == NULL)     my_fatal("tb_init(): FReadTableToMemory() not found\n");
      if (FMapTableToMemoryPtr == NULL)      my_fatal("tb_init(): FMapTableToMemory() not found\n");
      if (L_TbtProbeTablePtr == NULL)        my_fatal("tb_init(): L_TbtProbeTable() not found\n");
      if (IInitializeTbPtr == NULL)          my_fatal("tb_init(): IInitializeTb() not found\n");

      TableBase->init = true;
   }
}

// tb_close()

void tb_close(void) {

   if (TableBase->init == 1) {

      if (TableBase->cache != NULL) my_free(TableBase->cache);
      UNLOAD_LIB(LibHandler);
   }
}

// tb_clear()

void tb_clear(void) {

   if (TableBase->init == 1) {
      TableBase->read_hit = 0;
      TableBase->read_nb = 0;
   }
}

// tb_stats()

sint64 tb_stats(void) {

   if (TableBase->init == 1) {
      return TableBase->read_hit;
   } else {
      return 0;
   }
}

// tb_path()

void tb_path(const char path[]) {

   ASSERT(path!=NULL);

   // init

   if (TableBase->init == 0) return;

   // verify

   if (!my_string_empty(path)) {
      if (strlen(path) > 256) {
         my_fatal("tb_path(): String to long\n");
      }

      if (my_string_equal(path,"<empty>")) return;

      // set

      strncpy(TableBase->path,path,256);
      TableBase->piece_nb = IInitializeTbPtr(TableBase->path);

      ASSERT(TableBase->piece_nb>0);
   }
}

// tb_cache()

void tb_cache(int size) {

   bool cache_is_ok;

   ASSERT(size>=1&&size<=1024); // option.cpp

   // init

   if (TableBase->init == 0) return;

   // verify

   if (TableBase->cache != NULL) my_free(TableBase->cache);

   // set

   TableBase->size = size * 1024 * 1024;
   TableBase->cache = my_malloc(TableBase->size);

   cache_is_ok = FTbSetCacheSizePtr(TableBase->cache,TableBase->size) > 0;
   ASSERT(cache_is_ok);
}

// tb_piece_nb()

int tb_piece_nb(void) {

   ASSERT(TableBase->piece_nb>=0&&TableBase->piece_nb<=6);

   return TableBase->piece_nb;
}

// tb_probe()

bool tb_probe(const board_t * board, int * value) {

   int i;
   int ep;
   int count[10];
   int desc, side, invert;
   int score, dist;
   int sq, sq_64;
   int piece, piece_12;
   int piece_index, count_index;
   int colour, me;
   const sq_t * ptr;
   uint32 list[2][16];
   uint32 * white_list, * black_list, * my_list;
   INDEX index;

   ASSERT(board!=NULL);
   ASSERT(value!=NULL);

   ASSERT(board_is_ok(board));

   // init

   if (TableBase->init == 0) return false;
   if (TableBase->piece_nb == 0) return false;

   ASSERT(TableBase->piece_nb>0);
   ASSERT(board->piece_nb<=TableBase->piece_nb);

   TableBase->read_nb++;

   // piece lists

   for (i = 0; i < 10; i++) count[i] = 0;

   for (i = 0; i < 16; i++) {
      list[White][i] = 0;
      list[Black][i] = 0;
   }

   for (colour = 0; colour < ColourNb; colour++) {

      me = colour;
      my_list = list[me];

      // pieces

      for (ptr = &board->piece[me][1]; (sq=*ptr) != SquareNone; ptr++) { // HACK: no king

         piece = board->square[sq];
         piece_12 = PIECE_TO_12(piece);

         // index

         count_index = count[TbFrom[piece_12]]++;
         piece_index = (TbPiece[piece_12] * TbPieceNb) + count_index;

         ASSERT(count_index>=0&&count_index<3);
         ASSERT(piece_index>2&&piece_index<16);

         // square

         sq_64 = SQUARE_TO_64(sq);

         // fill list

         my_list[piece_index] = sq_64;
      }

      // pawns

      for (ptr = &board->pawn[me][0]; (sq=*ptr) != SquareNone; ptr++) {

         piece = board->square[sq];
         piece_12 = PIECE_TO_12(piece);

         // index

         count_index = count[TbFrom[piece_12]]++;
         piece_index = (TbPiece[piece_12] * TbPieceNb) + count_index;

         ASSERT(count_index>=0&&count_index<3);
         ASSERT(piece_index>=0&&piece_index<3);

         // square

         sq_64 = SQUARE_TO_64(sq);

         // fill list

         my_list[piece_index] = sq_64;
      }

      // king

      my_list[TbPieceNb * 5] = SQUARE_TO_64(KING_POS(board,me));
   }

   desc = IDescFindFromCountersPtr(count);

   if (desc > 0) {
      side = board->turn;
      invert = 0;
      white_list = list[White];
      black_list = list[Black];
   } else if (desc < 0) {
      side = COLOUR_OPP(board->turn);
      invert = 1;
      white_list = list[Black];
      black_list = list[White];
      desc = -desc;
   } else {
      return false;
   }

   ASSERT(desc>0);
   ASSERT(side==White||side==Black);
   ASSERT(invert==0||invert==1);
   ASSERT(white_list!=NULL);
   ASSERT(black_list!=NULL);

   if (!FRegisteredFunPtr(desc,side)) return false;

   // probe

   ep = EpSquare;
   if (board->ep_square != SquareNone) ep = SQUARE_TO_64(board->ep_square);

   index = (*(PfnIndCalcFunPtr(desc,side)))(white_list,black_list,ep,invert);
   score = L_TbtProbeTablePtr(desc,side,index);

   // score

   if (score == 32767) return false;

   TableBase->read_hit++;

   if (score > 0) { // win
      dist = 32766 - score;
      *value = +ValueMate - dist * 2 - 1;
   } else if (score < 0) { // loss
      dist = score + 32766;
      *value = -ValueMate + dist * 2;
   } else { // draw
      *value = ValueDraw;
   }

   ASSERT(*value>=-ValueMate&&*value<=ValueMate);

   return true;
}

// end of tb.cpp

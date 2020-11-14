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

/*
 * EGBB idea and library by Daniel Shawul
 *
 * This is just a gambling code and might be only usefull for analysis.
 * The values doesn't fit to Fruit and the EGBB api still missing some features.
 * At least this is an example how to use it for an engine.
 */

// egbb.cpp

// includes

#ifdef _WIN32
#  include <windows.h>
#else
#  include <cstring>
#  include <dlfcn.h>
#endif

#include "colour.h"
#include "egbb.h"
#include "egbb_index.h"
#include "piece.h"
#include "protocol.h"
#include "util.h"
#include "value.h"

// constants

#ifdef _WIN32
   static const char EgbbLib[] = "egbb.dll";
#else
   static const char EgbbLib[] = "egbb.so";
#endif

static const int ValueDraw = 0;
static const int ValueError = 99999;

static const int CacheSize = 4 * 1024 * 1024; // option.cpp

static const sint8 TbPiece[16] = { 6, 12, 5, 11, 4, 10, 3, 9, 2, 8, 1, 7, 0, 0, 0, 0 };

// types

enum load_t {
    LOAD_NONE,
    LOAD_4MEN,
    LOAD_SMART,
    LOAD_5MEN
};

struct tablebase_t {
   bool init;
   bool load;
   char path[256];
   uint32 size;
   int piece_nb;
   sint64 read_nb;
   sint64 read_hit;
};

// variables

static INSTANCE LibHandler;

static tablebase_t Egbb[1];

// prototypes

static load_egbb_5men   load_egbb_ptr;
static probe_egbb_5men  probe_egbb_ptr;

// functions

// egbb_init()

void egbb_init(void) {

   // init

   Egbb->init = false;
   Egbb->load = false;
   Egbb->size = CacheSize;
   Egbb->piece_nb = 0;
   Egbb->read_hit = 0;
   Egbb->read_nb = 0;

   LibHandler = LOAD_LIB(EgbbLib);

   if (LibHandler != NULL) {

      send("egbb library loaded");

      load_egbb_ptr =    (load_egbb_5men)     GET_ENTRY(LibHandler,"load_egbb_5men");
      probe_egbb_ptr =   (probe_egbb_5men)    GET_ENTRY(LibHandler,"probe_egbb_5men");

      if (load_egbb_ptr == NULL)   my_fatal("egbb_init(): load_egbb_5men() not found\n");
      if (probe_egbb_ptr == NULL)  my_fatal("egbb_init(): probe_egbb_5men() not found\n");

      Egbb->init = true;
   }
}

// egbb_close()

void egbb_close(void) {

   if (Egbb->init == 1) {
      UNLOAD_LIB(LibHandler);
   }
}

// egbb_clear()

void egbb_clear(void) {

   if (Egbb->init == 1) {
      Egbb->read_hit = 0;
      Egbb->read_nb = 0;
   }
}

// egbb_stats()

sint64 egbb_stats(void) {

   if (Egbb->init == 1) {
      return Egbb->read_hit;
   } else {
      return 0;
   }
}

// egbb_path()

void egbb_path(const char path[]) {

   char c;

   ASSERT(path!=NULL);

   // init

   if (Egbb->init == 0) return;

   // verify

   if (!my_string_empty(path)) {
      if (strlen(path) >= 256-2) {
         my_fatal("egbb_path(): String to long\n");
      }

      if (my_string_equal(path,"<empty>")) return;

      // set

      strncpy(Egbb->path,path,256);

      // verify

      c = Egbb->path[strlen(path)-1];

      if (c != '/' && c != '\\') {
         if (strchr(Egbb->path, '\\') != NULL) {
            strcat(Egbb->path, "\\");
         } else {
            strcat(Egbb->path, "/");
         }
      }

      // load

      load_egbb_ptr(Egbb->path,Egbb->size,LOAD_SMART);

      Egbb->load = true;
      Egbb->piece_nb = 5; // HACK: assume 5-piece table
   }
}

// egbb_cache()

void egbb_cache(int size) {

   ASSERT(size>=1&&size<=1024); // option.cpp

   // init

   if (Egbb->init == 0) return;

   // set

   Egbb->size = size * 1024 * 1024;
   if (Egbb->load) load_egbb_ptr(Egbb->path,Egbb->size,LOAD_SMART);
}

// egbb_piece_nb()

int egbb_piece_nb(void) {

   ASSERT(Egbb->piece_nb>=0&&Egbb->piece_nb<=6);

   return Egbb->piece_nb;
}

// egbb_probe()

bool egbb_probe(const board_t * board, int * value) {

   int i;
   int sq;
   int piece;
   int colour, me;
   const sq_t * ptr;
   int counter;
   int white_king, black_king;
   int piece_list[4], square_list[4];

   ASSERT(board!=NULL);
   ASSERT(value!=NULL);

   ASSERT(board_is_ok(board));
   ASSERT(!board_is_check(board));

   // init

   if (Egbb->init == 0) return false;
   if (Egbb->load == 0) return false;

   ASSERT(board->piece_nb<=Egbb->piece_nb);

   // init

   for (i = 0; i < 4; i++) {
      piece_list[i] = 0;
      square_list[i] = 0;
   }

   Egbb->read_nb++;

   // king

   white_king = SQUARE_TO_64(KING_POS(board,White));
   black_king = SQUARE_TO_64(KING_POS(board,Black));

   counter = 0;

   for (colour = 0; colour < ColourNb; colour++) {

      me = colour;

      // pieces

      for (ptr = &board->piece[me][1]; (sq=*ptr) != SquareNone; ptr++) { // HACK: no king

         piece = board->square[sq];

         // index

         ASSERT(counter>=0&&counter<4);

         piece_list[counter] = TbPiece[PIECE_TO_12(piece)];
         square_list[counter] = SQUARE_TO_64(sq);

         counter++;
      }

      // pawns

      for (ptr = &board->pawn[me][0]; (sq=*ptr) != SquareNone; ptr++) {

         piece = board->square[sq];

         // index

         ASSERT(counter>=0&&counter<4);

         piece_list[counter] = TbPiece[PIECE_TO_12(piece)];
         square_list[counter] = SQUARE_TO_64(sq);

         counter++;
      }
   }

   // probe

   *value = probe_egbb_ptr(board->turn,white_king,black_king,piece_list[0],square_list[0],piece_list[1],square_list[1],piece_list[2],square_list[2]);

   // score

   if (*value != ValueError) {
      Egbb->read_hit++;
      ASSERT(!value_is_mate(*value));
      return true;
   }

   return false;
}

// end of egbb.cpp

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

// book.cpp

// includes

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "board.h"
#include "book.h"
#include "move.h"
#include "move_gen.h"
#include "util.h"

// types

struct entry_t {
   uint64 key;
   uint16 move;
   uint16 count;
   uint16 n;
   uint16 sum;
};

// variables

static FILE * BookFile;
static int BookSize;

// prototypes

static int    find_pos     (uint64 key);

static void   read_entry   (entry_t * entry, int n);
static uint64 read_integer (FILE * file, int size);

// functions

// book_init()

void book_init() {

   BookFile = NULL;
   BookSize = 0;
}

// book_open()

void book_open(const char file_name[]) {

   ASSERT(file_name!=NULL);

   BookFile = fopen(file_name,"rb");

   if (BookFile != NULL) {

      if (fseek(BookFile,0,SEEK_END) == -1) {
         my_fatal("book_open(): fseek(): %s\n",strerror(errno));
      }

      BookSize = ftell(BookFile) / 16;
      if (BookSize == -1) my_fatal("book_open(): ftell(): %s\n",strerror(errno));
   }
}

// book_close()

void book_close() {

   if (BookFile != NULL && fclose(BookFile) == EOF) {
      my_fatal("book_close(): fclose(): %s\n",strerror(errno));
   }
}

// book_move()

int book_move(board_t * board) {

   int best_move;
   int best_score;
   int pos;
   entry_t entry[1];
   int move;
   int score;
   list_t list[1];
   int i;

   ASSERT(board!=NULL);

   if (BookFile != NULL && BookSize != 0) {

      // draw a move according to a fixed probability distribution

      best_move = MoveNone;
      best_score = 0;

      for (pos = find_pos(board->key); pos < BookSize; pos++) {

         read_entry(entry,pos);
         if (entry->key != board->key) break;

         move = entry->move;
         score = entry->count;

         // pick this move?

         ASSERT(score>0);

         best_score += score;
         if (my_random(best_score) < score) best_move = move;
      }

      if (best_move != MoveNone) {

         // convert PolyGlot move into Fruit move; TODO: handle promotes

         gen_legal_moves(list,board);

         for (i = 0; i < list->size; i++) {
            move = list->move[i];

            if ((move & 07777) == best_move) return move;

            // support newer polyglot castle

            if (MOVE_IS_CASTLE(move)) { // will be generate at end of list

               // TODO: Remove me if chess960 is supported ?

               if ((move & 4) != 0 && (best_move & 263) != 0) return move; // short
               if ((move & 4) == 0 && (best_move & 256) != 0) return move; // long
            }
         }
      }
   }

   return MoveNone;
}

// find_pos()

static int find_pos(uint64 key) {

   int left, right, mid;
   entry_t entry[1];

   // binary search (finds the leftmost entry)

   left = 0;
   right = BookSize-1;

   ASSERT(left<=right);

   while (left < right) {

      mid = (left + right) / 2;
      ASSERT(mid>=left&&mid<right);

      read_entry(entry,mid);

      if (key <= entry->key) {
         right = mid;
      } else {
         left = mid+1;
      }
   }

   ASSERT(left==right);

   read_entry(entry,left);

   return (entry->key == key) ? left : BookSize;
}

// read_entry()

static void read_entry(entry_t * entry, int n) {

   ASSERT(entry!=NULL);
   ASSERT(n>=0&&n<BookSize);

   ASSERT(BookFile!=NULL);

   if (fseek(BookFile,n*16,SEEK_SET) == -1) {
      my_fatal("read_entry(): fseek(): %s\n",strerror(errno));
   }

   entry->key   = read_integer(BookFile,8);
   entry->move  = read_integer(BookFile,2);
   entry->count = read_integer(BookFile,2);
   entry->n     = read_integer(BookFile,2);
   entry->sum   = read_integer(BookFile,2);
}

// read_integer()

static uint64 read_integer(FILE * file, int size) {

   uint64 n;
   int i;
   int b;

   ASSERT(file!=NULL);
   ASSERT(size>0&&size<=8);

   n = 0;

   for (i = 0; i < size; i++) {

      b = fgetc(file);

      if (b == EOF) {
         if (feof(file)) {
            my_fatal("read_integer(): fgetc(): EOF reached\n");
         } else { // error
            my_fatal("read_integer(): fgetc(): %s\n",strerror(errno));
         }
      }

      ASSERT(b>=0&&b<256);
      n = (n << 8) | b;
   }

   return n;
}

// end of book.cpp


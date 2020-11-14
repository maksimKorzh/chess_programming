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

// trans.cpp

// includes

#include "hash.h"
#include "move.h"
#include "option.h"
#include "protocol.h"
#include "smp.h"
#include "trans.h"
#include "util.h"
#include "value.h"

// constants

static const bool UseStats = false;

static const int DateSize = 16;

static const int ClusterSize = 4;

static const int DepthNone = -128;

// types

struct entry_t {
   uint32 lock;
   uint16 move;
   uint8 date;
   sint8 move_depth;
   sint8 min_depth;
   sint8 max_depth;
   sint16 min_value;
   sint16 max_value;
   sint16 eval;
};

struct trans_t {
   entry_t * table;
   uint32 size;
   uint32 mask;
   int date;
   int age[DateSize];
   uint32 used;            // TODO: thread based
   sint64 read_nb;         // TODO: thread based
   sint64 read_hit;        // TODO: thread based
   sint64 write_nb;        // TODO: thread based
   sint64 write_hit;       // TODO: thread based
   sint64 write_collision; // TODO: thread based
};

// variables

trans_t Trans[1];

// prototypes

static void      trans_set_date (int date);
static int       trans_age      (int date);

static entry_t * trans_entry    (uint64 key);

static bool      entry_is_ok    (const entry_t * entry);

// functions

// trans_is_ok()

bool trans_is_ok(void) {

   int date;

   if (Trans == NULL) return false;

   if (Trans->table == NULL) return false;
   if (Trans->size == 0) return false;
   if (Trans->mask == 0 || Trans->mask >= Trans->size) return false;
   if (Trans->date >= DateSize) return false;

   for (date = 0; date < DateSize; date++) {
      if (Trans->age[date] != trans_age(date)) return false;
   }

   return true;
}

// trans_init()

void trans_init(void) {

   ASSERT(sizeof(entry_t)==16);

   Trans->size = 0;
   Trans->mask = 0;
   Trans->table = NULL;

   trans_set_date(0);

   trans_clear();
}

// trans_alloc()

void trans_alloc(void) {

   uint32 size, target;

   // calculate size

   target = option_get_int("Hash");

   if (target < 4) target = 16;
   if (target > 1024) target = 1024; // options.cpp

   target *= 1024 * 1024;

   for (size = 1; size != 0 && size <= target; size *= 2);

   size /= 2;
   ASSERT(size>0&&size<=target);

   // allocate table

   size /= sizeof(entry_t);
   ASSERT(size!=0&&(size&(size-1))==0); // power of 2

   Trans->size = size + (ClusterSize - 1); // HACK to avoid testing for end of table
   Trans->mask = size - 1;

   Trans->table = (entry_t *) my_malloc(Trans->size*sizeof(entry_t));

   trans_clear();

   ASSERT(trans_is_ok());
}

// trans_free()

void trans_free(void) {

   my_free(Trans->table);

   Trans->table = NULL;
   Trans->size = 0;
   Trans->mask = 0;
}

// trans_clear()

void trans_clear(void) {

   entry_t clear_entry[1];
   entry_t * entry;
   uint32 index;

   trans_set_date(0);

   clear_entry->lock = 0;
   clear_entry->move = MoveNone;
   clear_entry->date = Trans->date;
   clear_entry->move_depth = DepthNone;
   clear_entry->min_depth = DepthNone;
   clear_entry->max_depth = DepthNone;
   clear_entry->min_value = -ValueInf;
   clear_entry->max_value = +ValueInf;
   clear_entry->eval = ValueNone;

   ASSERT(entry_is_ok(clear_entry));

   entry = Trans->table;

   for (index = 0; index < Trans->size; index++) {
      *entry++ = *clear_entry;
   }
}

// trans_inc_date()

void trans_inc_date(void) {

   trans_set_date((Trans->date+1)%DateSize);
}

// trans_set_date()

static void trans_set_date(int date) {

   ASSERT(date>=0&&date<DateSize);

   Trans->date = date;

   for (date = 0; date < DateSize; date++) {
      Trans->age[date] = trans_age(date);
   }

   Trans->used = 0;
   Trans->read_nb = 0;
   Trans->read_hit = 0;
   Trans->write_nb = 0;
   Trans->write_hit = 0;
   Trans->write_collision = 0;
}

// trans_age()

static int trans_age(int date) {

   int age;

   ASSERT(date>=0&&date<DateSize);

   age = Trans->date - date;
   if (age < 0) age += DateSize;

   ASSERT(age>=0&&age<DateSize);

   return age;
}

// trans_store()

void trans_store(uint64 key, int move, int depth, int min_value, int max_value, int eval) {

   entry_t * entry, * best_entry;
   int score, best_score;
   int best_depth;
   int i;

   ASSERT(move>=0&&move<65536);
   ASSERT(depth>=-127&&depth<=+127);
   ASSERT(min_value>=-ValueInf&&min_value<=+ValueInf);
   ASSERT(max_value>=-ValueInf&&max_value<=+ValueInf);
   ASSERT(min_value<=max_value);
   ASSERT(value_is_ok(eval)||eval==ValueNone);

   // init

   if (UseStats) Trans->write_nb++;

   // probe

   best_entry = NULL;
   best_score = -32767;

   entry = trans_entry(key);

   for (i = 0; i < ClusterSize; i++, entry++) {

      if (entry->lock == KEY_LOCK(key)) {

         // hash hit => update existing entry

         if (UseStats) {
            Trans->write_hit++;
            if (entry->date != Trans->date) Trans->used++;
         }

         entry->date = Trans->date;

         if (eval != ValueNone && eval != entry->eval) entry->eval = eval; // HACK: TB case

         if (move != MoveNone && depth >= entry->move_depth) {
            entry->move_depth = depth;
            entry->move = move;
         }

         if (min_value > -ValueInf && depth >= entry->min_depth) {
            entry->min_depth = depth;
            entry->min_value = min_value;
         }

         if (max_value < +ValueInf && depth >= entry->max_depth) {
            entry->max_depth = depth;
            entry->max_value = max_value;
         }

#if DEBUG
         if (thread_number() == 1) ASSERT(entry_is_ok(entry));
#endif

         return;
      }

      // evaluate replacement score

      best_depth = MAX(entry->move_depth,MAX(entry->min_depth,entry->max_depth));
      ASSERT(best_depth>=-128&&best_depth<=+127);

      score = Trans->age[entry->date] * 256 - best_depth; // TODO: move_depth ?
      ASSERT(score>-32767);

      if (score > best_score) {
         best_entry = entry;
         best_score = score;
      }
   }

   // "best" entry found

   entry = best_entry;
   ASSERT(entry!=NULL);

#if DEBUG
   if (thread_number() == 1) ASSERT(entry->lock!=KEY_LOCK(key)); // TODO: fix me!
#endif

   if (UseStats) {
      if (entry->date == Trans->date) {
         Trans->write_collision++;
      } else {
         Trans->used++;
      }
   }

   // store

   ASSERT(entry!=NULL);

   entry->lock = KEY_LOCK(key);
   entry->date = Trans->date;

   entry->eval = eval;

   entry->move_depth = (move != MoveNone) ? depth : DepthNone;
   entry->move = move;

   entry->min_depth = (min_value > -ValueInf) ? depth : DepthNone;
   entry->max_depth = (max_value < +ValueInf) ? depth : DepthNone;
   entry->min_value = min_value;
   entry->max_value = max_value;

#if DEBUG
   if (thread_number() == 1) ASSERT(entry_is_ok(entry));
#endif
}

// trans_retrieve()

bool trans_retrieve(uint64 key, int * move, int * min_depth, int * max_depth, int * min_value, int * max_value, int * eval, bool refresh) {

   entry_t * entry;
   int i;

   ASSERT(move!=NULL);
   ASSERT(min_depth!=NULL);
   ASSERT(max_depth!=NULL);
   ASSERT(min_value!=NULL);
   ASSERT(max_value!=NULL);
   ASSERT(eval!=NULL);
   ASSERT(refresh==true||refresh==false);

   // init

   if (UseStats) Trans->read_nb++;

   // probe

   entry = trans_entry(key);

   for (i = 0; i < ClusterSize; i++, entry++) {

      if (entry->lock == KEY_LOCK(key)) {

         // found

         if (UseStats) Trans->read_hit++;
         if (refresh && entry->date != Trans->date) entry->date = Trans->date;

         *move = entry->move;
         *eval = entry->eval;

         *min_depth = entry->min_depth;
         *max_depth = entry->max_depth;
         *min_value = entry->min_value;
         *max_value = entry->max_value;

         return true;
      }
   }

   // not found

   return false;
}

// trans_stats()

void trans_stats(void) {

   double full;
   // double hit, collision;

   ASSERT(trans_is_ok());

   full = double(Trans->used) / double(Trans->size);
   // hit = double(Trans->read_hit) / double(Trans->read_nb);
   // collision = double(Trans->write_collision) / double(Trans->write_nb);

   send("info hashfull %.0f",full*1000.0);
}

// trans_entry()

static entry_t * trans_entry(uint64 key) {

   uint32 index;

   index = KEY_INDEX(key) & Trans->mask;
   ASSERT(index>=0&&index<=Trans->mask);

   return &Trans->table[index];
}

// entry_is_ok()

static bool entry_is_ok(const entry_t * entry) {

   if (entry == NULL) return false;

   if (entry->date >= DateSize) return false;

   if (entry->move == MoveNone && entry->move_depth != DepthNone) return false;
   if (entry->move != MoveNone && entry->move_depth == DepthNone) return false;

   if (entry->min_value == -ValueInf && entry->min_depth != DepthNone) return false;
   if (entry->min_value >  -ValueInf && entry->min_depth == DepthNone) return false;

   if (entry->max_value == +ValueInf && entry->max_depth != DepthNone) return false;
   if (entry->max_value <  +ValueInf && entry->max_depth == DepthNone) return false;

   if (entry->eval != ValueNone && !value_is_ok(entry->eval)) return false;

   return true;
}

// end of trans.cpp


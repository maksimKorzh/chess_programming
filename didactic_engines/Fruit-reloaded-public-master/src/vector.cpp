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

// vector.cpp

// includes

#include "piece.h"
#include "square.h"
#include "util.h"
#include "vector.h"

// variables

int Distance[DeltaNb];

// functions

void vector_init() {

   int delta;
   int x, y;
   int dist, tmp;

   // Distance[]

   for (delta = 0; delta < DeltaNb; delta++) Distance[delta] = -1;

   for (y = -7; y <= +7; y++) {

      for (x = -7; x <= +7; x++) {

         delta = y * 16 + x;
         ASSERT(delta_is_ok(delta));

         dist = 0;

         tmp = x;
         if (tmp < 0) tmp = -tmp;
         if (tmp > dist) dist = tmp;

         tmp = y;
         if (tmp < 0) tmp = -tmp;
         if (tmp > dist) dist = tmp;

         Distance[DeltaOffset+delta] = dist;
      }
   }
}

// delta_is_ok()

bool delta_is_ok(int delta) {

   if (delta < -119 || delta > +119) return false;

   if ((delta & 0xF) == 8) return false; // HACK: delta % 16 would be ill-defined for negative numbers

   return true;
}

// inc_is_ok()

bool inc_is_ok(int inc) {

   int dir;

   for (dir = 0; dir < 8; dir++) {
      if (KingInc[dir] == inc) return true;
   }

   return false;
}

// end of vector.cpp


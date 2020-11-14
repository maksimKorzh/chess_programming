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

// value.cpp

// includes

#include "piece.h"
#include "search.h"
#include "util.h"
#include "value.h"

// variables

int ValuePiece[PieceNb];

// functions

// value_init()

void value_init() {

   int piece;

   // ValuePiece[]

   for (piece = 0; piece < PieceNb; piece++) ValuePiece[piece] = -1;

   ValuePiece[Empty] = 0; // needed?
   ValuePiece[Edge]  = 0; // needed?

   ValuePiece[WP] = ValuePawn;
   ValuePiece[WN] = ValueKnight;
   ValuePiece[WB] = ValueBishop;
   ValuePiece[WR] = ValueRook;
   ValuePiece[WQ] = ValueQueen;
   ValuePiece[WK] = ValueKing;

   ValuePiece[BP] = ValuePawn;
   ValuePiece[BN] = ValueKnight;
   ValuePiece[BB] = ValueBishop;
   ValuePiece[BR] = ValueRook;
   ValuePiece[BQ] = ValueQueen;
   ValuePiece[BK] = ValueKing;
}

// value_is_ok()

bool value_is_ok(int value) {

   if (value < -ValueInf || value > +ValueInf) return false;

   return true;
}

// range_is_ok()

bool range_is_ok(int min, int max) {

   if (!value_is_ok(min)) return false;
   if (!value_is_ok(max)) return false;

   if (min >= max) return false; // alpha-beta-like ranges cannot be null

   return true;
}

// value_is_win()

bool value_is_win(int value) {

   ASSERT(value_is_ok(value));

   if (value < -ValueWin || value > +ValueWin) return true;

   return false;
}

// value_is_mate()

bool value_is_mate(int value) {

   ASSERT(value_is_ok(value));

   if (value < -ValueEvalInf || value > +ValueEvalInf) return true;

   return false;
}

// value_to_trans()

int value_to_trans(int value, int height) {

   ASSERT(value_is_ok(value));
   ASSERT(height_is_ok(height));

   if (value < -ValueEvalInf) {
      value -= height;
   } else if (value > +ValueEvalInf) {
      value += height;
   }

   ASSERT(value_is_ok(value));

   return value;
}

// value_from_trans()

int value_from_trans(int value, int height) {

   ASSERT(value_is_ok(value));
   ASSERT(height_is_ok(height));

   if (value < -ValueEvalInf) {
      value += height;
   } else if (value > +ValueEvalInf) {
      value -= height;
   }

   ASSERT(value_is_ok(value));

   return value;
}

// value_to_mate()

int value_to_mate(int value) {

   int dist;

   ASSERT(value_is_ok(value));

   if (value < -ValueEvalInf) {

      dist = (ValueMate + value) / 2;
      ASSERT(dist>0);

      return -dist;

   } else if (value > +ValueEvalInf) {

      dist = (ValueMate - value + 1) / 2;
      ASSERT(dist>0);

      return +dist;
   }

   return 0;
}

// end of value.cpp


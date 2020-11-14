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

// value.h

#ifndef VALUE_H
#define VALUE_H

// includes

#include "piece.h"
#include "util.h"

// constants

const int ValuePawn   = 100;   // was 100
const int ValueKnight = 325;   // was 300
const int ValueBishop = 325;   // was 300
const int ValueRook   = 500;   // was 500
const int ValueQueen  = 1000;  // was 900
const int ValueKing   = 10000; // was 10000

const int ValueNone    = -32767;
const int ValueWin     = 15000;
const int ValueMate    = 30000;
const int ValueInf     = ValueMate;
const int ValueEvalInf = ValueMate - 256; // handle mates upto 255 plies

// macros

#define VALUE_MATE(height) (-ValueMate+(height))
#define VALUE_PIECE(piece) (ValuePiece[piece])

// variables

extern int ValuePiece[PieceNb];

// functions

extern void value_init       ();

extern bool value_is_ok      (int value);
extern bool range_is_ok      (int min, int max);

extern bool value_is_win     (int value);
extern bool value_is_mate    (int value);

extern int  value_to_trans   (int value, int height);
extern int  value_from_trans (int value, int height);

extern int  value_to_mate    (int value);

#endif // !defined VALUE_H

// end of value.h


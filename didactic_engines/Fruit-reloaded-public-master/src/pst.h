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

// pst.h

#ifndef PST_H
#define PST_H

// includes

#include "util.h"

// constants

const int Opening = 0;
const int Endgame = 1;
const int StageNb = 2;

// macros

#define PST(piece_12,square_64,stage) (Pst[piece_12][square_64][stage])

// variables

extern sint16 Pst[12][64][StageNb];

// functions

extern void pst_init ();

#endif // !defined PST_H

// end of pst.h


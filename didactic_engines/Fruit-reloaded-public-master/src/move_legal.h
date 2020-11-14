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

// move_legal.h

#ifndef MOVE_LEGAL_H
#define MOVE_LEGAL_H

// includes

#include "board.h"
#include "list.h"
#include "util.h"

// functions

extern bool move_is_pseudo  (int move, board_t * board);
extern bool quiet_is_pseudo (int move, board_t * board);

extern bool pseudo_is_legal (int move, board_t * board);

#endif // !defined MOVE_LEGAL_H

// end of move_legal.h


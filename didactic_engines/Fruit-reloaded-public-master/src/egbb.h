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

// egbb.h

#ifndef EGBB_H
#define EGBB_H

// includes

#include "board.h"

// functions

extern void    egbb_init     (void);
extern void    egbb_close    (void);

extern void    egbb_clear    (void);

extern sint64  egbb_stats    (void);

extern void    egbb_path     (const char path[]);

extern void    egbb_cache    (int size);

extern int     egbb_piece_nb (void);

extern bool    egbb_probe    (const board_t * board, int * value);

#endif // !defined EGBB_H

// end of egbb.h

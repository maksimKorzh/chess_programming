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

// pv.h

#ifndef PV_H
#define PV_H

// includes

#include "board.h"
#include "move.h"
#include "util.h"

// macros

#define PV_CLEAR(pv) (*(pv)=MoveNone)

// functions

extern bool pv_is_ok     (const mv_t pv[]);

extern void pv_copy      (mv_t dst[], const mv_t src[]);
extern void pv_cat       (mv_t dst[], const mv_t src[], int move);

extern bool pv_to_string (const mv_t pv[], char string[], int size);

#endif // !defined PV_H

// end of pv.h


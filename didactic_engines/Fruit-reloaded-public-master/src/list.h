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

// list.h

#ifndef LIST_H
#define LIST_H

// includes

#include "board.h"
#include "util.h"

// constants

const int ListSize = 256;

// macros

#define LIST_CLEAR(list)     ((list)->size=0)
#define LIST_ADD(list,mv)    ((list)->move[(list)->size++]=(mv))

#define LIST_IS_EMPTY(list)  ((list)->size==0)
#define LIST_SIZE(list)      ((list)->size)

#define LIST_MOVE(list,pos)  ((list)->move[pos])
#define LIST_VALUE(list,pos) ((list)->value[pos])

// types

struct list_t {
   int size;
   uint16 move[ListSize];
   sint16 value[ListSize];
};

typedef bool (*move_test_t) (int move, board_t * board);

// functions

extern bool list_is_ok    (const list_t * list);

extern void list_remove   (list_t * list, int pos);

extern void list_copy     (list_t * dst, const list_t * src);

extern void list_sort     (list_t * list);

extern bool list_contain  (const list_t * list, int move);
extern void list_note     (list_t * list);

extern void list_filter   (list_t * list, board_t * board, move_test_t test, bool keep);

#endif // !defined LIST_H

// end of list.h


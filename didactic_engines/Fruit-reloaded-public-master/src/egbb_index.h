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

// egbb_index.h

#ifndef EGBB_INDEX_H
#define EGBB_INDEX_H

// functions

typedef void      (* load_egbb)        (char * path);
typedef void      (* load_egbb_5men)   (char * path, int cache_size, int load_options);
typedef void      (* load_egbb_xmen)   (char * path, int cache_size, int load_options);

typedef int       (* probe_egbb)       (int player, int w_king, int b_king, int piece1, int square1, int piece2, int square2);
typedef int       (* probe_egbb_5men)  (int player, int w_king, int b_king, int piece1, int square1, int piece2, int square2, int piece3, int square3);
typedef int       (* probe_egbb_xmen)  (int player, int * piece_list, int * square_list);

#endif // !defined EGBB_INDEX_H

// end of egbb_index.h

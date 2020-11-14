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

// tb_index.h

#ifndef TB_INDEX_H
#define TB_INDEX_H

// includes

#include "util.h"

// contans

#if defined (_MSC_VER)
#  define  TB_FASTCALL  __fastcall
#else
#  define  TB_FASTCALL
#endif

// types

typedef uint8 BYTE;
typedef int color;
typedef uint32 square;
typedef uint64 INDEX;

// functions

typedef INDEX        (TB_FASTCALL * PfnCalcIndex)  (square *, square *, square, int fInverse);
typedef int          (* IDescFindFromCounters)     (int * piCount);
typedef void         (* VTbCloseFiles)             (void);
typedef int          (* FTbSetCacheSize)           (void * pv, uint32 cbSize);
typedef int          (* FRegisteredFun)            (int iTb, color side);
typedef PfnCalcIndex (* PfnIndCalcFun)             (int iTb, color side);
typedef int          (* FReadTableToMemory)        (int iTb, color side, BYTE * pb);
typedef int          (* FMapTableToMemory)         (int iTb, color side);
typedef int          (* L_TbtProbeTable)           (int iTb, color side, INDEX indOffset);
typedef int          (* IInitializeTb)             (char * pszPath);

#endif // !defined TB_INDEX_H

// end of tb_index.h

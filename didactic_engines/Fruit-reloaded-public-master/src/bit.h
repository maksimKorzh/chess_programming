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

// bit.h

#ifndef BIT_H
#define BIT_H

// includes

#include "util.h"

// macros

#define BIT(n)       (BitEQ[n])

#define BIT_FIRST(b) (BitFirst[b])
#define BIT_LAST(b)  (BitLast[b])
#define BIT_COUNT(b) (BitCount[b])

// variables

extern int BitEQ[16];
extern int BitLT[16];
extern int BitLE[16];
extern int BitGT[16];
extern int BitGE[16];

extern int BitFirst[0x100];
extern int BitLast[0x100];
extern int BitCount[0x100];
extern int BitRev[0x100];

extern uint64 Bit[64];

static const uint64 BIT_FILE_A		= 0x0101010101010101ULL;
static const uint64 BIT_FILE_B		= 0x0202020202020202ULL;
static const uint64 BIT_FILE_C		= 0x0404040404040404ULL;
static const uint64 BIT_FILE_D		= 0x0808080808080808ULL;
static const uint64 BIT_FILE_DE		= 0x1818181818181818ULL;
static const uint64 BIT_FILE_E		= 0x1010101010101010ULL;
static const uint64 BIT_FILE_F		= 0x2020202020202020ULL;
static const uint64 BIT_FILE_G		= 0x4040404040404040ULL;
static const uint64 BIT_FILE_H		= 0x8080808080808080ULL;

static const uint64 BIT_RANK_1		= 0x00000000000000FFULL;
static const uint64 BIT_RANK_2		= 0x000000000000FF00ULL;
static const uint64 BIT_RANK_3		= 0x0000000000FF0000ULL;
static const uint64 BIT_RANK_4		= 0x00000000FF000000ULL;
static const uint64 BIT_RANK_5		= 0x000000FF00000000ULL;
static const uint64 BIT_RANK_6		= 0x0000FF0000000000ULL;
static const uint64 BIT_RANK_7		= 0x00FF000000000000ULL;
static const uint64 BIT_RANK_8		= 0xFF00000000000000ULL;

static const uint64 BIT_CORNER		= 0x8100000000000081ULL;
static const uint64 BIT_EDGE		= 0xFF818181818181FFULL;

#endif // !defined BIT_H

// end of bit.h

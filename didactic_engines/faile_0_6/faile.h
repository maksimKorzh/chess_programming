/**************************************************
 *   Faile version 0.6                            *
 *   Author: Adrien Regimbald                     *
 *   E-mail: adrien@gpu.srv.ualberta.ca           *
 *   Web Page: http://www.ualberta.ca/~adrien/    *
 *                                                *
 *   File: Faile.h                                *
 *   Purpose: Holds defines / typedefs for varied *
 *   parts of the program.                        *
 **************************************************/

#ifndef FAILE_H
#define FAILE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#define NDEBUG
#include <assert.h>

#include <time.h>
#include <ctype.h>

#define frame    0
#define wpawn    1
#define bpawn    2
#define wknight  3
#define bknight  4
#define wking    5
#define bking    6
#define wrook    7
#define brook    8
#define wqueen   9
#define bqueen   10
#define wbishop  11
#define bbishop  12
#define npiece   13
#define wep_piece 15
#define bep_piece 16

/* defines for castle_flag: */

#define wck 1
#define wcq 2
#define bck 3
#define bcq 4

/* hard-coded max depth for lightning... easier than mucking around with a
   million #ifdefs for finding time, and even then, some systems can still
   only use time(), so if you used only time() to guide you when you have < 1
   minute left, you may spend too much time on moves! */
#define lightning_depth 4

/* hard-coded max depth for search, used to ensure that we don't do check
   extensions, or quiescence search forever: */
#define Maxdepth 30

/* if lines is defined, all legal lines will be outputed to file lines.txt */
/* #define lines */

#define rank(squareid) ((((squareid) - 26) / 12) + 1)
#define file(squareid) ((((squareid) - 26) % 12) + 1)
#define TryOffset(x) do {try_move(&move2[0], n_moves, from, (from + (x)));}\
   while(0)
#define TrySlide(x) do {target = from + (x); while (try_move(&move2[0],\
   n_moves, from, target)) {target += (x);}} while(0)

/* file(26) == 1 && rank(26) == 1 */

typedef struct {
   int from;
   int target;
   int captured;
   int en_passant;
   int promoted;
   int castle;
   int captured_number;
} move_s;


/* a white/black move structure (of strings) for history: */

typedef struct {
   char white[6];
   char black[6];
} white_black;

typedef enum {FALSE, TRUE} Bool;

typedef enum {no_mate, white_won, black_won, stalemate} End_of_game;

#endif

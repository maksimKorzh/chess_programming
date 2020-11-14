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

// main.cpp

// includes

#include <cstdio>
#include <cstdlib>

#include "attack.h"
#include "book.h"
#include "egbb.h"
#include "engine.h"
#include "hash.h"
#include "move_do.h"
#include "option.h"
#include "pawn.h"
#include "piece.h"
#include "protocol.h"
#include "random.h"
#include "square.h"
#include "tb.h"
#include "trans.h"
#include "util.h"
#include "value.h"
#include "vector.h"

// functions

// main()

int main(int argc, char * argv[]) {

   char buffer[256];

   util_init();
   my_random_init(); // for opening book

   printf("Fruit reloaded %s UCI by Fabien Letouzey, Ryan Benitez and Daniel Mehrmann\n", my_version(buffer));

   // early initialisation (the rest is done after UCI options are parsed in protocol.cpp)

   option_init();
   engine_init();

   square_init();
   piece_init();
   pawn_init_bit();
   value_init();
   vector_init();
   attack_init();
   move_do_init();

   random_init();
   hash_init();

   trans_init();
   book_init();

   tb_init();
   egbb_init();

   // loop

   loop();

   return EXIT_SUCCESS;
}

// end of main.cpp


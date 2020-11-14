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

// engine.h

#ifndef ENGINE_H
#define ENGINE_H

// includes

#include "option.h"

// constants

const int EngineNb = 2;
const int OptionMax = 256;

// types

enum engine_type_t {
   ENGINE_DEFAULT,
   ENGINE_TEST,
};

typedef struct engine_t {
   option_t option[OptionMax];
} engine;

// functions

extern void     engine_init        ();

extern bool     engine_type_is_ok  (int type);
extern bool     engine_is_ok       (int engine_nb);

extern int      engine_get_type    ();
extern void     engine_set_type    (int type);

extern engine * engine_get         (int engine_nb);

extern option * engine_get_opt     (int engine_nb);

#endif // !defined ENGINE_H

// end of engine.h


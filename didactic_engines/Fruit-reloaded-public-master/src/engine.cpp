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

// engine.cpp

// includes

#include "engine.h"
#include "option.h"

// variables

static int EngineType;

static engine_t Engine[EngineNb];

// functions

void engine_init() {

   EngineType = ENGINE_DEFAULT;
}

// engine_type_is_ok()

bool engine_type_is_ok(int type) {

   return type >=ENGINE_DEFAULT && type <= ENGINE_TEST;
}

// engine_is_ok()

bool engine_is_ok(int engine_nb) {

   return engine_nb >= 0 && engine_nb < EngineNb;
}

// engine_get_type()

int engine_get_type() {

   ASSERT(engine_type_is_ok(EngineType));

   return EngineType;
}

// engine_set_type()

void engine_set_type(int type) {

   ASSERT(engine_type_is_ok(type));

   EngineType = type;
}

// engine_get()

engine * engine_get(int engine_nb) {

   engine_t * eng;

   ASSERT(engine_is_ok(engine_nb));

   eng = &Engine[engine_nb];
   ASSERT(eng!=NULL);

   return eng;
}

// engine_get_opt()

option * engine_get_opt(int engine_nb) {

   option_t * opt;

   ASSERT(engine_is_ok(engine_nb));

   opt = Engine[engine_nb].option;
   ASSERT(opt!=NULL);

   return opt;
}

// end of engine.cpp

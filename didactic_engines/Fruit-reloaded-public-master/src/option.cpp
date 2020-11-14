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

// option.cpp

// includes

#include <cstdlib>
#include <cstring>

#include "engine.h"
#include "option.h"
#include "protocol.h"
#include "util.h"

// types

struct option_uci_t {
   const char * var;
   bool declare;
   const char * init;
   const char * type;
   const char * extra;
};

// constants

static const option_uci_t OptionUCI[] = {

   { "Hash", true, "16", "spin", "min 4 max 1024"  },

   { "NalimovPath",  true, "<empty>", "string", "" },
   { "NalimovCache", true, "8",       "spin",   "min 1 max 1024" },
   { "EgbbPath",     true, "<empty>", "string", "" },
   { "EgbbCache",    true, "4",       "spin",   "min 1 max 1024" },

   { "Ponder", true, "false", "check", "" },

   { "MultiPV", true, "1", "spin", "min 1 max 256" },

   { "UCI_EngineAbout", true, "Fruit Reloaded by Fabien Letouzey, Ryan Benitez and Daniel Mehrmann", "string", "" },

   { "OwnBook",  true, "true",           "check",  "" },
   { "BookFile", true, "book_small.bin", "string", "" },

   { "Threads", true, "1", "spin", "min 1 max 16" },

   { "Aspiration Search", true, "true", "check" },
   { "Aspiration Depth",  true, "4",    "spin min 3 max 10" },
   { "Aspiration Window", true, "16",   "spin min 1 max 100" },

   { "EGTB",       true, "true",  "check", "" },
   { "EGTB Depth", true, "8",     "spin",  "min 0 max 20" },
   { "EGBB",       true, "false", "check", "" },
   { "EGBB Depth", true, "4",     "spin",  "min 0 max 20" },

   { "Material",       true, "100", "spin", "min 0 max 400" },
   { "Piece Activity", true, "100", "spin", "min 0 max 400" },
   { "King Safety",    true, "100", "spin", "min 0 max 400" },
   { "Pawn Activity",  true, "100", "spin", "min 0 max 400" },
   { "Pawn Structure", true, "100", "spin", "min 0 max 400" },
   { "Pawn",           true, "100", "spin", "min 0 max 400" },
   { "Knight",         true, "100", "spin", "min 0 max 400" },
   { "Bishop",         true, "100", "spin", "min 0 max 400" },
   { "Rook",           true, "100", "spin", "min 0 max 400" },
   { "Queen",          true, "100", "spin", "min 0 max 400" },
   { "Bishop Pair",    true, "100", "spin", "min 0 max 400" },

   { "Contempt Factor", true, "0", "spin", "min -1000 max 1000" },

   { "Log", true, "false", "check", "" },

   { NULL, false, NULL, NULL, NULL, }
};

// prototypes

static option_t * option_find (const char var[]);

// functions

// option_init()

void option_init() {

   int engine_nb;
   const option_uci_t * uci;
   option_t * opt;

   // default options

   for (engine_nb = 0; engine_nb < EngineNb; engine_nb++) {

      engine_set_type(engine_nb);

      for (uci = &OptionUCI[0], opt = engine_get_opt(engine_nb); uci->var != NULL; uci++, opt++) {
         opt->var = uci->var;
         option_set(opt->var,uci->init);
      }

      opt->var = NULL;
   }
}

// option_list()

void option_list() {

   const option_uci_t * uci;

   for (uci = &OptionUCI[0]; uci->var != NULL; uci++) {
      if (uci->declare) {
         if (uci->extra != NULL && *uci->extra != '\0') {
            send("option name %s type %s default %s %s",uci->var,uci->type,uci->init,uci->extra);
         } else {
            send("option name %s type %s default %s",uci->var,uci->type,uci->init);
         }
      }
   }
}

// option_set()

bool option_set(const char var[], const char val[]) {

   option_t * opt;

   ASSERT(var!=NULL);
   ASSERT(val!=NULL);

   opt = option_find(var);
   if (opt == NULL) return false;

   memset(opt->val,0,sizeof(opt->val));
   strncpy(opt->val,val,strlen(val));

   return true;
}

// option_get()

const char * option_get(const char var[]) {

   option_t * opt;

   ASSERT(var!=NULL);

   opt = option_find(var);
   if (opt == NULL) my_fatal("option_get(): unknown option \"%s\"\n",var);

   return opt->val;
}

// option_get_bool()

bool option_get_bool(const char var[]) {

   const char * val;

   val = option_get(var);

   if (false) {
   } else if (my_string_equal(val,"true") || my_string_equal(val,"yes") || my_string_equal(val,"1")) {
      return true;
   } else if (my_string_equal(val,"false") || my_string_equal(val,"no") || my_string_equal(val,"0")) {
      return false;
   }

   ASSERT(false);

   return false;
}

// option_get_int()

int option_get_int(const char var[]) {

   const char * val;

   val = option_get(var);

   return atoi(val);
}

// option_get_string()

const char * option_get_string(const char var[]) {

   const char * val;

   val = option_get(var);

   return val;
}

// option_find()

static option_t * option_find(const char var[]) {

   int eng;
   option_t * opt;

   ASSERT(var!=NULL);

   eng = engine_get_type();

   for (opt = engine_get_opt(eng); opt->var != NULL; opt++) {
      if (my_string_equal(opt->var,var)) return opt;
   }

   return NULL;
}

// end of option.cpp


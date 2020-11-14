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

// util.h

#ifndef UTIL_H
#define UTIL_H

// includes

#include <cstdio>

// Compiler

#ifdef _MSC_VER
#  pragma warning(disable:4996)
#endif

// constants

#undef FALSE
#define FALSE 0

#undef TRUE
#define TRUE 1

//#define DEBUG TRUE

#ifdef DEBUG
#  undef DEBUG
#  define DEBUG TRUE
#else
#  define DEBUG FALSE
#endif

#ifdef _MSC_VER
#  define S64_FORMAT "%I64d"
#  define U64_FORMAT "%016I64X"
#else
#  define S64_FORMAT "%lld"
#  define U64_FORMAT "%016llX"
#endif

// macros

#ifdef _MSC_VER
#  define S64(u) (u##i64)
#  define U64(u) (u##ui64)
#else
#  define S64(u) (u##LL)
#  define U64(u) (u##ULL)
#endif

#undef ASSERT
#if DEBUG
#  define ASSERT(a) { if (!(a)) my_fatal("file \"%s\", line %d, assertion \"" #a "\" failed\n",__FILE__,__LINE__); }
#else
#  define ASSERT(a)
#endif

#define ABS(x)   ((x)<0?-(x):(x))
#define MIN(a,b) ((a)<=(b)?(a):(b))
#define MAX(a,b) ((a)>=(b)?(a):(b))

#ifdef _WIN32
#  define INSTANCE HINSTANCE
#  define LOAD_LIB(x)      LoadLibrary(x)
#  define UNLOAD_LIB(x)    FreeLibrary(x)
#  define GET_ENTRY(x,y)   GetProcAddress(x,y)
#else
#  define INSTANCE void *
#  define LOAD_LIB(x)      dlopen(x,RTLD_LAZY)
#  define UNLOAD_LIB(x)    dlclose(x)
#  define GET_ENTRY(x,y)   dlsym(x,y)
#endif

// types

typedef signed char sint8;
typedef unsigned char uint8;

typedef signed short sint16;
typedef unsigned short uint16;

typedef signed int sint32;
typedef unsigned int uint32;

#ifdef _MSC_VER
  typedef signed __int64 sint64;
  typedef unsigned __int64 uint64;
#else
  typedef signed long long int sint64;
  typedef unsigned long long int uint64;
#endif

struct my_timer_t {
   double start_real;
   double start_cpu;
   double elapsed_real;
   double elapsed_cpu;
   bool running;
};

// functions

extern void   util_init             ();

extern void   my_random_init        ();
extern int    my_random             (int n);

extern sint64 my_atoll              (const char string[]);

extern int    my_round              (double x);

extern void * my_malloc             (int size);
extern void   my_free               (void * address);

extern void   my_fatal              (const char format[], ...);

extern bool   my_file_read_line     (FILE * file, char string[], int size);

extern bool   my_string_empty       (const char string[]);
extern bool   my_string_equal       (const char string_1[], const char string_2[]);

extern void   my_timer_reset        (my_timer_t * timer);
extern void   my_timer_start        (my_timer_t * timer);
extern void   my_timer_stop         (my_timer_t * timer);

extern double my_timer_elapsed_real (const my_timer_t * timer);
extern double my_timer_elapsed_cpu  (const my_timer_t * timer);
extern double my_timer_cpu_usage    (const my_timer_t * timer);

extern char * my_version            (char buffer[]);

#endif // !defined UTIL_H

// end of util.h


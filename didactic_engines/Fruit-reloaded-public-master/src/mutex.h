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

// mutex.h

#ifndef MUTEX_H
#define MUTEX_H

// includes

#ifdef _WIN32
#  include <windows.h>
#else
#  include <signal.h>
#  include <pthread.h>
#endif

// macros

#ifdef _WIN32
#  define MUTEX_INIT(x,y)  InitializeCriticalSection(x)
#  define MUTEX_LOCK(x)    EnterCriticalSection(x)
#  define MUTEX_FREE(x)    LeaveCriticalSection(x)
#  define MUTEX_CLOSE(x)   DeleteCriticalSection(x)
#else
#  define MUTEX_INIT(x,y)  pthread_mutex_init(x, y)
#  define MUTEX_LOCK(x)    pthread_mutex_lock(x)
#  define MUTEX_FREE(x)    pthread_mutex_unlock(x)
#  define MUTEX_CLOSE(x)   pthread_mutex_destroy(x)
#endif

// types

#ifdef _WIN32
   typedef CRITICAL_SECTION mutex_t;
#else
   typedef pthread_mutex_t mutex_t;
#endif

#ifdef _WIN32
   typedef HANDLE handle_t;
#else
   typedef pthread_cond_t handle_t;
#endif

#endif // !defined MUTEX_H

// end of mutex.h

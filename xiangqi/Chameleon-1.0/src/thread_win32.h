/*
  Chameleon, a UCI chinese chess playing engine derived from Stockfish
  Copyright (C) 2004-2008 Tord Romstad (Glaurung author)
  Copyright (C) 2008-2015 Marco Costalba, Joona Kiiski, Tord Romstad
  Copyright (C) 2015-2017 Marco Costalba, Joona Kiiski, Gary Linscott, Tord Romstad
  Copyright (C) 2017 Wilbert Lee

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifndef THREAD_WIN32_H_INCLUDED
#define THREAD_WIN32_H_INCLUDED

// STL thread library used by mingw and gcc when cross compiling for Windows
// relies on libwinpthread. Currently libwinpthread implements mutexes directly
// on top of Windows semaphores. Semaphores, being kernel objects, require kernel
// mode transition in order to lock or unlock, which is very slow compared to
// interlocked operations (about 30% slower on bench test). To workaround this
// issue, we define our wrappers to the low level Win32 calls. We use critical
// sections to support Windows XP and older versions. Unfortunately, cond_wait()
// is racy between unlock() and WaitForSingleObject() but they have the same
// speed performance of SRW locks.

#include <condition_variable>
#include <mutex>

// Default case: use STL classes
typedef std::mutex Mutex;
typedef std::condition_variable ConditionVariable;

#endif // #ifndef THREAD_WIN32_H_INCLUDED
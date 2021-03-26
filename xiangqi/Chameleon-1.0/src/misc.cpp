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

#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <mutex>

#include "misc.h"

using namespace std;

// Debug functions used mainly to collect run-time statistics
static int64_t hits[2], means[2];

/// engine_info() returns the full name of the current Chameleon version. This
/// will be either "Chameleon <Tag> DD-MM-YY" (where DD-MM-YY is the date when
/// the program was compiled) or "Chameleon <Version>", depending on whether
/// Version is empty.

const string engine_info(bool to_uci)
{
	stringstream s; // From compiler, format is "Sep 21 2008"

	if (to_uci)
		s << "id name Chameleon" << "\n"
		<< "id author Wilbert Lee" << "";

	return s.str();
}

void dbg_hit_on(bool b) { ++hits[0]; if (b) ++hits[1]; }
void dbg_hit_on(bool c, bool b) { if (c) dbg_hit_on(b); }
void dbg_mean_of(int v) { ++means[0]; means[1] += v; }

void dbg_print()
{
	if (hits[0])
		cerr << "Total " << hits[0] << " Hits " << hits[1]
		<< " hit rate (%) " << 100 * hits[1] / hits[0] << endl;

	if (means[0])
		cerr << "Total " << means[0] << " Mean "
		<< (double)means[1] / means[0] << endl;
}

// Our fancy logging facility. The trick here is to replace cin.rdbuf() and
// cout.rdbuf() with two Tie objects that tie cin and cout to a file stream. We
// can toggle the logging of std::cout and std:cin at runtime while preserving
// usual i/o functionality and without changing a single line of code!
// Idea from http://groups.google.com/group/comp.lang.c++/msg/1d941c0f26ea0d81
struct Tie : public streambuf
{
	// MSVC requires splitted streambuf for cin and cout
	Tie(streambuf* b, ofstream* f) : buf(b), file(f) {}
	int sync() { return file->rdbuf()->pubsync(), buf->pubsync(); }
	int overflow(int c) { return log(buf->sputc((char)c), "<< "); }
	int underflow() { return buf->sgetc(); }
	int uflow() { return log(buf->sbumpc(), ">> "); }

	streambuf* buf;
	ofstream* file;

	int log(int c, const char* prefix)
	{
		static int last = '\n';

		if (last == '\n')
			file->rdbuf()->sputn(prefix, 3);

		return last = file->rdbuf()->sputc((char)c);
	}
};

class Logger
{
	Logger() : in(cin.rdbuf(), &file), out(cout.rdbuf(), &file) {}
	~Logger() { start(false); }

	ofstream file;
	Tie in, out;

public:
	static void start(bool b)
	{
		static Logger l;

		if (b && !l.file.is_open())
		{
			l.file.open("io_log.txt", ifstream::out | ifstream::app);
			cin.rdbuf(&l.in);
			cout.rdbuf(&l.out);
		}
		else if (!b && l.file.is_open())
		{
			cout.rdbuf(l.out.buf);
			cin.rdbuf(l.in.buf);
			l.file.close();
		}
	}
};

// Used to serialize access to std::cout to avoid multiple threads writing at
// the same time.
std::ostream& operator<<(std::ostream& os, SyncCout sc)
{
	static std::mutex m;

	if (sc == IO_LOCK)
		m.lock();

	if (sc == IO_UNLOCK)
		m.unlock();

	return os;
}

// Trampoline helper to avoid moving Logger to misc.h
void start_logger(bool b) { Logger::start(b); }

/// prefetch() preloads the given address in L1/L2 cache. This is a non-blocking
/// function that doesn't stall the CPU waiting for data to be loaded from memory,
/// which can be quite slow.

#ifdef NO_PREFETCH

void prefetch(void*) {}

#else

void prefetch(void* addr)
{
#  if defined(__INTEL_COMPILER)
	// This hack prevents prefetches from being optimized away by
	// Intel compiler. Both MSVC and gcc seem not be affected by this.
	__asm__("");
#  endif

#  if defined(__INTEL_COMPILER) || defined(_MSC_VER)
	_mm_prefetch((char*)addr, _MM_HINT_T0);
#  else
	__builtin_prefetch(addr);
#  endif
}

#endif

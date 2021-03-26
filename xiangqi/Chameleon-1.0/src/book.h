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

#ifndef BOOK_H_INCLUDED
#define BOOK_H_INCLUDED
#define NOMINMAX

#include <fstream>
#include <string>

#include "position.h"
#if 0
#include "curl/curl.h"
#endif
#include "rkiss.h"
#include "misc.h"

#if 0
class YunBook
{
public:
	YunBook() { curl_global_init(CURL_GLOBAL_DEFAULT); };
	~YunBook() { curl_global_cleanup(); };
	Move probe(const Position& pos);
	Score probe_score(const Position& pos);
	std::vector<Move> probe_pv(const Position& pos, bool pickBest);
	std::vector<Position *> probe_pv_pos(const Position& pos, bool pickBest);

private:
	bool query(const Position& pos, const std::string act);
	static size_t cb(const char *d, size_t n, size_t l, void *p);
	static void split(const std::string str, const std::string delim, std::vector<std::string> &ret);

	CURL *curl;
	CURLcode res;

	const static std::string url;
	const static std::string pick[];
	static Depth depth;
	static Score score;
	static std::vector<std::string> moves;
};
#endif

class PolyglotBook : private std::ifstream
{
public:
	PolyglotBook() : rkiss(now()) {};
	~PolyglotBook() { if (is_open()) close(); };
	Move probe(const Position& pos, const std::string& fName, bool pickBest);

private:
	template<typename T> PolyglotBook& operator>>(T& n);

	bool open(const char* fName);
	size_t find_first(uint64_t key);

	RKISS rkiss;
	std::string fileName;
};

#endif // #ifndef BOOK_H_INCLUDED

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

#ifndef UCI_H_INCLUDED
#define UCI_H_INCLUDED

#include <map>
#include <string>

#include "types.h"

class Position;

namespace UCI
{
	class Option;

	// Custom comparator because UCI options should be case insensitive
	struct CaseInsensitiveLess
	{
		bool operator() (const std::string&, const std::string&) const;
	};

	// Our options container is actually a std::map
	typedef std::map<std::string, Option, CaseInsensitiveLess> OptionsMap;

	// Option class implements an option as defined by UCI protocol
	class Option
	{
		typedef void(*OnChange)(const Option&);
	public:
		Option(OnChange = nullptr);
		Option(bool v, OnChange = nullptr);
		Option(const char* v, OnChange = nullptr);
		Option(int v, int min, int max, OnChange = nullptr);
		Option& operator=(const std::string&);
		void operator<<(const Option&);
		operator int() const;
		operator std::string() const;
	private:
		friend std::ostream& operator<<(std::ostream&, const OptionsMap&);
	private:
		std::string defaultValue, currentValue, type;
		int min, max;
		size_t idx;
		OnChange on_change;
	};

	void init();
	void loop(int argc, char* argv[]);
	std::string value(Value v);
	std::string square(Square s);
	std::string move(Move m, bool chess960 = false);
	std::string pv(const Position& pos, Depth depth, Value alpha, Value beta);
	Move to_move(const Position& pos, std::string& str);

} // namespace UCI

extern UCI::OptionsMap Options;

#endif // #ifndef UCI_H_INCLUDED

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

#include <iostream>
#include <sstream>
#include <string>

#include "evaluate.h"
#include "movegen.h"
#include "position.h"
#include "search.h"
#include "thread.h"
#include "timeman.h"
#include "uci.h"

using namespace std;

// FEN string of the initial position, normal chess
const char* StartFEN = "rnbakabnr/9/1c5c1/p1p1p1p1p/9/9/P1P1P1P1P/1C5C1/9/RNBAKABNR w - - 0 1";

// Stack to keep track of the position states along the setup moves (from the
// start position to the position just before the search starts). Needed by
// 'draw by repetition' detection.
Search::StateStackPtr SetupStates;

/// position() is called when engine receives the "position" UCI command.
/// The function sets up the position described in the given FEN string ("fen")
/// or the starting position ("startpos") and then makes the moves given in the
/// following move list ("moves").

void position(Position& pos, istringstream& is)
{
	Move m;
	string token, fen;

	is >> token;

	if (token == "startpos")
	{
		fen = StartFEN;
		is >> token; // Consume "moves" token if any
	}
	else if (token == "fen")
		while (is >> token && token != "moves")
			fen += token + " ";

	else
	{
		while (token != "moves")
		{
			fen += token + " ";
			if (!(is >> token))
				break;
		}
	}

	pos.set(fen, false, Threads.main());
	SetupStates = Search::StateStackPtr(new std::stack<StateInfo>);

	// Parse move list (if any)
	while (is >> token && (m = UCI::to_move(pos, token)) != MOVE_NONE)
	{
		SetupStates->push(StateInfo());
		pos.do_move(m, SetupStates->top(), pos.gives_check(m, CheckInfo(pos)));
	}
}

/// setoption() is called when engine receives the "setoption" UCI command. The
/// function updates the UCI option ("name") to the given value ("value").

void setoption(istringstream& is)
{
	string token, name, value;

	is >> token; // Consume "name" token

	// Read option name (can contain spaces)
	while (is >> token && token != "value")
		name += string(" ", name.empty() ? 0 : 1) + token;

	// Read option value (can contain spaces)
	while (is >> token)
		value += string(" ", value.empty() ? 0 : 1) + token;

	if (Options.count(name))
		Options[name] = value;
	else
		sync_cout << "No such option: " << name << sync_endl;
}

/// go() is called when engine receives the "go" UCI command. The function sets
/// the thinking time and other parameters from the input string, then starts
/// the search.

void go(const Position& pos, istringstream& is)
{
	Search::LimitsType limits;
	string token;

	limits.startTime = now(); // As early as possible!

	while (is >> token)
		if (token == "searchmoves")
			while (is >> token)
				limits.searchmoves.push_back(UCI::to_move(pos, token));
		else if (token == "wtime")     is >> limits.time[WHITE];
		else if (token == "btime")     is >> limits.time[BLACK];
		else if (token == "winc")      is >> limits.inc[WHITE];
		else if (token == "binc")      is >> limits.inc[BLACK];
		else if (token == "movestogo") is >> limits.movestogo;
		else if (token == "depth")     is >> limits.depth;
		else if (token == "nodes")     is >> limits.nodes;
		else if (token == "movetime")  is >> limits.movetime;
		else if (token == "mate")      is >> limits.mate;
		else if (token == "infinite")  limits.infinite = 1;
		else if (token == "ponder")    limits.ponder = 1;

		Threads.start_thinking(pos, limits, SetupStates);
}

/// UCI::loop() waits for a command from stdin, parses it and calls the appropriate
/// function. Also intercepts EOF from stdin to ensure gracefully exiting if the
/// GUI dies unexpectedly. When called with some command line arguments, e.g. to
/// run 'bench', once the command is executed the function returns immediately.
/// In addition to the UCI ones, also some additional debug commands are supported.

void UCI::loop(int argc, char* argv[])
{
	string token, cmd;
	Position pos(StartFEN, false, Threads.main()); // The root position

	for (int i = 1; i < argc; i++)
		cmd += std::string(argv[i]) + " ";

	do
	{
		if (argc == 1 && !getline(cin, cmd)) // Block here waiting for input or EOF
			cmd = "quit";

		istringstream is(cmd);

		token.clear(); // getline() could return empty or blank line
		is >> skipws >> token;

		// The GUI sends 'ponderhit' to tell us to ponder on the same move the
		// opponent has played. In case Signals.stopOnPonderhit is set we are
		// waiting for 'ponderhit' to stop the search (for instance because we
		// already ran out of time), otherwise we should continue searching but
		// switching from pondering to normal search.
		if (token == "quit"
			|| token == "stop"
			|| (token == "ponderhit" && Search::Signals.stopOnPonderhit))
		{
			Search::Signals.stop = true;
			Threads.main()->start_searching(true); // Could be sleeping
		}
		else if (token == "ponderhit")
			Search::Limits.ponder = 0; // Switch to normal search

		else if (token == "uci")
			sync_cout << engine_info(true)
			<< Options
			<< "\nuciok" << sync_endl;

		else if (token == "ucinewgame")
		{
			Search::clear();
			Time.availableNodes = 0;
		}
		else if (token == "isready")    sync_cout << "readyok" << sync_endl;
		else if (token == "go")         go(pos, is);
		else if (token == "position" || token == "fen")   position(pos, is);
		else if (token == "setoption")  setoption(is);

		// Additional custom non-UCI commands, useful for debugging
		else if (token == "flip")       pos.flip();
		else if (token == "d")          sync_cout << pos << sync_endl;
		else
			sync_cout << "Unknown command: " << cmd << sync_endl;

	} while (token != "quit" && argc == 1); // Passed args have one-shot behaviour

	Threads.main()->wait_for_search_finished();
}

/// UCI::value() converts a Value to a string suitable for use with the UCI
/// protocol specification:
///
/// cp <x>    The score from the engine's point of view in centipawns.
/// mate <y>  Mate in y moves, not plies. If the engine is getting mated
///           use negative values for y.

string UCI::value(Value v)
{
	stringstream ss;

	if (abs(v) < VALUE_MATE - MAX_PLY)
		ss << v;
	else
		ss << "mate " << (v > 0 ? VALUE_MATE - v + 1 : -VALUE_MATE - v) / 2;

	return ss.str();
}

/// UCI::square() converts a Square to a string in algebraic notation (g1, a7, etc.)

std::string UCI::square(Square s)
{
	return std::string{ char('a' + file_of(s)), char('0' + rank_of(s)) };
}

/// UCI::move() converts a Move to a string in coordinate notation (g1f3, a7a8q).
/// The only special case is castling, where we print in the e1g1 notation in
/// normal chess mode, and in e1h1 notation in chess960 mode. Internally all
/// castling moves are always encoded as 'king captures rook'.

string UCI::move(Move m, bool chess960)
{
	Square from = from_sq(m);
	Square to = to_sq(m);

	if (m == MOVE_NONE)
		return "(none)";

	if (m == MOVE_NULL)
		return "0000";

	string move = UCI::square(from) + UCI::square(to);

	return move;
}

/// UCI::to_move() converts a string representing a move in coordinate notation
/// (g1f3, a7a8q) to the corresponding legal Move, if any.

Move UCI::to_move(const Position& pos, string& str)
{
	if (str.length() == 5) // Junior could send promotion piece in uppercase
		str[4] = char(tolower(str[4]));

	for (const auto& m : MoveList<LEGAL>(pos))
		if (str == UCI::move(m, false))
			return m;

	return MOVE_NONE;
}

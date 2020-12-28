//   GreKo chess engine
//   (c) 2002-2020 Vladimir Medvedev <vrm@bk.ru>
//   http://greko.su

#include "eval.h"
#include "learn.h"
#include "moves.h"
#include "notation.h"
#include "search.h"
#include "utils.h"

string PROGRAM_NAME = "GreKo 2020.01";
string RELEASE_DATE = "31-Jan-2020";

string WEIGHTS_FILE = "weights.txt";

const int MIN_HASH_SIZE = 1;
const int MAX_HASH_SIZE = 4096;
const int DEFAULT_HASH_SIZE = 128;

const int MIN_PAWN_HASH_SIZE = 1;
const int MAX_PAWN_HASH_SIZE = 256;
const int DEFAULT_PAWN_HASH_SIZE = 8;

extern Pair PSQ[14][64];

Position g_pos;
deque<string> g_queue;
FILE* g_log = NULL;

bool g_console = false;
bool g_xboard = false;
bool g_uci = true;

static string g_s;
static vector<string> g_tokens;
static bool g_force = false;

void OnZero();

void ComputeTimeLimits()
{
	if (g_sd > 0 || g_sn > 0)
		return;
	if (g_restTime == 0)
		return;

	if (g_inc > 0)
	{
		// time control with increment
		g_stHard = g_restTime / 2;
		g_stSoft = g_restTime / 40;
	}
	else if (g_restMoves == 0 && g_movesPerSession == 0)
	{
		// absolute time control
		g_stHard = g_restTime / 40;
		g_stSoft = g_restTime / 40;
	}
	else
	{
		// N moves in M minutes
		if (g_movesPerSession > 0)
		{
			int curr = g_pos.Ply() / 2 + 1;
			g_restMoves = g_movesPerSession - ((curr - 1) % g_movesPerSession);
		}
		int divisor = (g_restMoves > 1)? g_restMoves : 2;
		g_stHard = g_restTime / divisor;
		g_stSoft = g_stHard / 2;
	}

	stringstream ss;
	ss << "TIME: rest = " << g_restTime <<
		", moves = " << g_restMoves <<
		", inc = " << g_inc <<
		" ==> stHard = " << g_stHard << ", stSoft = " << g_stSoft;
	Log(ss.str());
}
////////////////////////////////////////////////////////////////////////////////

void OnAnalyze()
{
	g_sd = 0;
	g_sn = 0;
	g_stHard = 0;
	g_stSoft = 0;

	ClearHash();

	Position pos = g_pos;
	StartSearch(pos, MODE_ANALYZE);
}
////////////////////////////////////////////////////////////////////////////////

void OnDiff()
{
	if (g_tokens.size() < 3)
		return;

	EvalConfig cfg1, cfg2;
	if (!cfg1.Read(g_tokens[1]))
	{
		cout << "Can't read config from " << g_tokens[1] << endl;
		return;
	}
	if (!cfg2.Read(g_tokens[2]))
	{
		cout << "Can't read config from " << g_tokens[2] << endl;
		return;
	}

	vector<int> x1, x2;
	cfg1.GetValues(x1);
	cfg2.GetValues(x2);

	double d2 = 0;
	for (size_t i = 0; i < x1.size(); ++i)
		d2 += (x1[i] - x2[i]) * (x1[i] - x2[i]);
	cout << sqrt(d2 / x1.size()) << endl;
}
////////////////////////////////////////////////////////////////////////////////

void OnDump()
{
	vector<int> x;
	g_eval.GetValues(x);

	ofstream ofs("default_params.cpp");
	if (!ofs.good())
		return;
	ofs << "\tstatic const int data[] =\n";
	ofs << "\t{\n";
	ofs << "\t\t";

	stringstream ss;
	size_t cur_len = 0;
	for (size_t i = 0; i < x.size(); ++i)
	{
		ss << x[i];
		if (i != x.size() - 1)
		{
			ss << ",";
			if (ss.str().length() - cur_len > 65)
			{
				ss << "\n\t\t";
				cur_len = ss.str().length();
			}
			else
				ss << " ";
		}
	}
	ofs << ss.str() << "\n\t};\n";
}
////////////////////////////////////////////////////////////////////////////////

void OnEval()
{
	stringstream ss;
	ss << Evaluate(g_pos) << endl;
	out(ss);
}
////////////////////////////////////////////////////////////////////////////////

void OnFEN()
{
	stringstream ss;
	ss << g_pos.FEN() << endl;
	out(ss);
}
////////////////////////////////////////////////////////////////////////////////

void OnFlip()
{
	g_pos.Mirror();
	g_pos.Print();
}
////////////////////////////////////////////////////////////////////////////////

void OnGoUci()
{
	U8 mode = MODE_PLAY;

	g_stHard = 0;
	g_stSoft = 0;
	g_inc = 0;
	g_sd = 0;
	g_sn = 0;
	g_restTime = 0;
	g_restMoves = 0;
	g_movesPerSession = 0;

	for (size_t i = 1; i < g_tokens.size(); ++i)
	{
		string token = g_tokens[i];
		if (token == "infinite")
		{
			mode = MODE_ANALYZE;
		}
		else if (i < g_tokens.size() - 1)
		{
			if (token == "movetime")
			{
				int t = atoi(g_tokens[i + 1].c_str());
				g_stHard = t;
				g_stSoft = t;
				++i;
			}
			else if ((token == "wtime" && g_pos.Side() == WHITE) || (token == "btime" && g_pos.Side() == BLACK))
			{
				g_restTime = atoi(g_tokens[i + 1].c_str());
				++i;
			}
			else if ((token == "winc" && g_pos.Side() == WHITE) || (token == "binc" && g_pos.Side() == BLACK))
			{
				g_inc = atoi(g_tokens[i + 1].c_str());
				++i;
			}
			else if (token == "movestogo")
			{
				g_restMoves = atoi(g_tokens[i + 1].c_str());
				++i;
			}
			else if (token == "depth")
			{
				g_sd = atoi(g_tokens[i + 1].c_str());
				++i;
			}
			else if (token == "nodes")
			{
				g_sn = atoi(g_tokens[i + 1].c_str());
				++i;
			}
		}
	}

	ComputeTimeLimits();
	Move mv = StartSearch(g_pos, mode);

	if (mv != 0)
	{
		stringstream ss;
		ss << "bestmove " << MoveToStrLong(mv);
		out(ss);
	}
}
////////////////////////////////////////////////////////////////////////////////

void OnGo()
{
	if (g_uci)
	{
		OnGoUci();
		return;
	}

	g_force = false;

	string result, comment;
	if (IsGameOver(g_pos, result, comment))
	{
		stringstream ss;
		ss << result << " " << comment << endl;
		out(ss);
		return;
	}

	ComputeTimeLimits();
	Move mv = StartSearch(g_pos, MODE_PLAY);

	if (mv != 0)
	{
		Highlight(true);
		stringstream ss;
		ss << "move " << MoveToStrLong(mv);
		out(ss);
		Highlight(false);

		g_pos.MakeMove(mv);

		if (IsGameOver(g_pos, result, comment))
			out(result + " " + comment);
	}
}
////////////////////////////////////////////////////////////////////////////////

void OnIsready()
{
	out("readyok");
}
////////////////////////////////////////////////////////////////////////////////

void OnLearn(string fileName, int numIters, U8 alg)
{
	string pgnFile, fenFile;

	if (fileName.find(".pgn") != string::npos)
	{
		pgnFile = fileName;
		size_t extPos = fileName.find(".pgn");
		if (extPos != string::npos)
			fenFile = fileName.substr(0, extPos) + ".fen";
	}
	else if (fileName.find(".fen") != string::npos)
	{
		fenFile = fileName;
		size_t extPos = fileName.find(".fen");
		if (extPos != string::npos)
			pgnFile = fileName.substr(0, extPos) + ".pgn";
	}
	else
	{
		pgnFile = fileName + ".pgn";
		fenFile = fileName + ".fen";
	}

	{
		ifstream test(fenFile.c_str());
		if (!test.good())
		{
			if (!PgnToFen(pgnFile, fenFile, 6, 999, 1))
				return;
		}
	}

	vector<int> x0;
	g_eval.GetValues(x0);

	vector<int> learnParams;
	learnParams.resize(NUM_FEATURES);

	ifstream ifs("learn_params.txt");
	if (ifs.good())
	{
		string s;
		while (getline(ifs, s))
		{
			vector<string> tokens;
			Split(s, tokens, " ");
			if (tokens.empty())
				continue;

			size_t first = 0, last = 0;
			if (g_eval.GetParamInterval(tokens[0], first, last))
			{
				size_t N = last - first + 1;
				if (tokens.size() == N + 1)
				{
					for (size_t i = first; i <= last; ++i)
						learnParams[i] = atoi(tokens[i - first + 1].c_str());
				}
				else
				{
					for (size_t i = first; i <= last; ++i)
						learnParams[i] = 1;
				}
			}
		}
	}
	else
	{
		for (size_t i = 0; i < NUM_FEATURES; ++i)
			learnParams[i] = 1;
	}

	if (alg & (STOCHASTIC_GRADIENT_DESCENT | COORDINATE_DESCENT))
	{
		if (alg & STOCHASTIC_GRADIENT_DESCENT)
			Sgd(fenFile, x0, learnParams);
		if (alg & COORDINATE_DESCENT)
			CoordinateDescent(fenFile, x0, numIters, learnParams);
	}
	else
		cout << "Unknown algorithm: " << alg << endl;
}
////////////////////////////////////////////////////////////////////////////////

void OnLearn()
{
	if (g_tokens.size() < 2)
		return;

	string fileName = g_tokens[1];

	int alg = (g_tokens.size() > 2)?
		atoi(g_tokens[2].c_str()) :
		STOCHASTIC_GRADIENT_DESCENT;

	int numIters = (g_tokens.size() > 3)? atoi(g_tokens[3].c_str()) : 1;

	g_startTrainingTime = GetProcTime();
	OnLearn(fileName, numIters, alg);
	cout << endl;
}
////////////////////////////////////////////////////////////////////////////////

void OnLevel()
{
	if (g_tokens.size() > 1)
		g_movesPerSession = atoi(g_tokens[1].c_str());
	if (g_tokens.size() > 3)
		g_inc = 1000 * atoi(g_tokens[3].c_str());
}
////////////////////////////////////////////////////////////////////////////////

void OnList()
{
	MoveList mvlist;
	GenAllMoves(g_pos, mvlist);

	stringstream ss;
	for (size_t i = 0; i < mvlist.Size(); ++i)
	{
		Move mv = mvlist[i].m_mv;
		ss << MoveToStrLong(mv) << " ";
	}
	ss << " -- total: " << mvlist.Size() << endl;
	out(ss);
}
////////////////////////////////////////////////////////////////////////////////

void OnLoad()
{
	if (g_tokens.size() < 2)
		return;

	ifstream ifs(g_tokens[1].c_str());
	if (!ifs.good())
	{
		out("Can't open file: " + g_tokens[1]);
		return;
	}

	int line = 1;
	if (g_tokens.size() > 2)
	{
		line = atoi(g_tokens[2].c_str());
		if (line <= 0)
			line = 1;
	}

	string fen;
	for (int i = 0; i < line; ++i)
	{
		if(!getline(ifs, fen)) break;
	}

	if (g_pos.SetFEN(fen))
	{
		g_pos.Print();
		cout << fen << endl << endl;
	}
	else
		out("Illegal FEN");
}
////////////////////////////////////////////////////////////////////////////////

void OnMT()
{
	if (g_tokens.size() < 2)
		return;

	ifstream ifs(g_tokens[1].c_str());
	if (!ifs.good())
	{
		out("Can't open file: " + g_tokens[1]);
		return;
	}

	string s;
	while (getline(ifs, s))
	{
		Position pos;
		if (pos.SetFEN(s))
		{
			out(s);
			EVAL e1 = Evaluate(pos);
			pos.Mirror();
			EVAL e2 = Evaluate(pos);
			if (e1 != e2)
			{
				pos.Mirror();
				pos.Print();
				cout << "e1 = " << e1 << ", e2 = " << e2 << endl << endl;
				return;
			}
		}
	}
	cout << endl;
}
////////////////////////////////////////////////////////////////////////////////

void OnMT2()
{
	if (g_tokens.size() < 2)
		return;

	ifstream ifs(g_tokens[1].c_str());
	if (!ifs.good())
	{
		out("Can't open file: " + g_tokens[1]);
		return;
	}

	int threshold = 1;
	if (g_tokens.size() > 2)
		threshold = atoi(g_tokens[2].c_str());

	vector<int> w;
	g_eval.GetValues(w);

	string s;
	while (getline(ifs, s))
	{
		Position pos;
		if (pos.SetFEN(s))
		{
			out(s);
			EVAL e1 = Evaluate(pos);

			Features featObj(pos);
			double e = 0;

			for (size_t i = 0; i < w.size(); ++i)
				e += w[i] * featObj.GetVector()[i];

			EVAL e2 = (pos.Side() == WHITE)? (EVAL)e : -(EVAL)e;

			if (abs(e1 - e2) > threshold)
			{
				pos.Mirror();
				pos.Print();
				cout << "e1 = " << e1 << ", e2 = " << e2 << endl << endl;
				return;
			}
		}
	}
	cout << endl;
}
////////////////////////////////////////////////////////////////////////////////

void OnNew()
{
	g_force = false;
	g_pos.SetInitial();

	ClearHash();
}
////////////////////////////////////////////////////////////////////////////////

void OnPerft()
{
	if (g_tokens.size() < 2)
		return;
	int depth = atoi(g_tokens[1].c_str());
	StartPerft(g_pos, depth);
}
////////////////////////////////////////////////////////////////////////////////

void OnPgnToFen()
{
	if (g_tokens.size() < 3)
		return;

	string pgnFile = g_tokens[1];
	string fenFile = g_tokens[2];
	int fensPerGame = 1;
	if (g_tokens.size() >= 4)
		fensPerGame = atoi(g_tokens[3].c_str());
	PgnToFen(pgnFile, fenFile, 6, 999, fensPerGame);
}
////////////////////////////////////////////////////////////////////////////////

void OnPing()
{
	if (g_tokens.size() > 1)
		out("pong " + g_tokens[1]);
}
////////////////////////////////////////////////////////////////////////////////

void OnPosition()
{
	if (g_tokens.size() < 2)
		return;

	size_t movesTag = 0;
	if (g_tokens[1] == "fen")
	{
		string fen = "";
		for (size_t i = 2; i < g_tokens.size(); ++i)
		{
			if (g_tokens[i] == "moves")
			{
				movesTag = i;
				break;
			}
			if (!fen.empty())
				fen += " ";
			fen += g_tokens[i];
		}
		g_pos.SetFEN(fen);
	}
	else if (g_tokens[1] == "startpos")
	{
		g_pos.SetInitial();
		for (size_t i = 2; i < g_tokens.size(); ++i)
		{
			if (g_tokens[i] == "moves")
			{
				movesTag = i;
				break;
			}
		}
	}

	if (movesTag)
	{
		for (size_t i = movesTag + 1; i < g_tokens.size(); ++i)
		{
			Move mv = StrToMove(g_tokens[i], g_pos);
			g_pos.MakeMove(mv);
		}
	}
}
////////////////////////////////////////////////////////////////////////////////

void OnPredict()
{
	if (g_tokens.size() < 2)
		return;

	string file = g_tokens[1];
	if (file.find(".fen") == string::npos)
		file += ".fen";

	stringstream ss;
	ss << setprecision(10) << Predict(file) << endl;
	out(ss);
}
////////////////////////////////////////////////////////////////////////////////

void OnProtover()
{
	stringstream ss;
	ss << "feature myname=\"" << PROGRAM_NAME << "\" setboard=1 analyze=1 colors=0 san=0 ping=1 name=1 done=1";
	out(ss);
}
////////////////////////////////////////////////////////////////////////////////

void OnPSQ()
{
	if (g_tokens.size() < 2)
		return;

	Pair* table = NULL;

	if (g_tokens[1] == "P" || g_tokens[1] == "p")
		table = PSQ[PW];
	else if (g_tokens[1] == "N" || g_tokens[1] == "n")
		table = PSQ[NW];
	else if (g_tokens[1] == "B" || g_tokens[1] == "b")
		table = PSQ[BW];
	else if (g_tokens[1] == "R" || g_tokens[1] == "r")
		table = PSQ[RW];
	else if (g_tokens[1] == "Q" || g_tokens[1] == "q")
		table = PSQ[QW];
	else if (g_tokens[1] == "K" || g_tokens[1] == "k")
		table = PSQ[KW];

	double sum_mid = 0, sum_end = 0;
	if (table != NULL)
	{
		cout << endl << "Midgame:" << endl << endl;
		for (FLD f = 0; f < 64; ++f)
		{
			cout << setw(4) << table[f].mid;
			sum_mid += table[f].mid;

			if (f < 63)
				cout << ", ";
			if (Col(f) == 7)
				cout << endl;
		}

		cout << endl << "Endgame:" << endl << endl;
		for (FLD f = 0; f < 64; ++f)
		{
			cout << setw(4) << table[f].end;
			sum_end += table[f].end;

			if (f < 63)
				cout << ", ";
			if (Col(f) == 7)
				cout << endl;
		}
		cout << endl;

		double divisor = (table == PSQ[PW])? 48 : 64;
		cout << "avg_mid = " << sum_mid / divisor << ", avg_end = " << sum_end / divisor << endl;
		cout << endl;
	}
}
////////////////////////////////////////////////////////////////////////////////

void OnSD()
{
	if (g_tokens.size() < 2)
		return;

	g_sd = atoi(g_tokens[1].c_str());
	g_sn = 0;
	g_stHard = 0;
	g_stSoft = 0;
}
////////////////////////////////////////////////////////////////////////////////

void OnSEE()
{
	if (g_tokens.size() < 2)
		return;

	if (CanBeMove(g_tokens[1]))
	{
		Move mv = StrToMove(g_tokens[1], g_pos);
		if (mv)
			cout << SEE(g_pos, mv) << endl;
	}
}
////////////////////////////////////////////////////////////////////////////////

void OnSelfPlay(const string& pgn_file, int N)
{
	RandSeed(time(0));

	if (g_tokens.size() > 1)
		N = atoi(g_tokens[1].c_str());

	for (int games = 0; games < N; ++games)
	{
		Position pos;
		pos.SetInitial();

		string result, comment;
		string pgn, line;

		while (!IsGameOver(pos, result, comment))
		{
			Move mv = (pos.Ply() < 6)? GetRandomMove(pos) : StartSearch(pos, MODE_PLAY | MODE_SILENT);

			if (mv == 0)
			{
				result = "1/2-1/2";
				comment = "{Adjudication: zero move generated}";
				break;
			}

			MoveList mvlist;
			GenAllMoves(pos, mvlist);
			if (pos.Side() == WHITE)
			{
				stringstream ss;
				ss << pos.Ply() / 2 + 1 << ". ";
				line = line + ss.str();
			}
			line = line + MoveToStrShort(mv, pos, mvlist) + string(" ");

			pos.MakeMove(mv);

			if (line.length() > 60 && pos.Side() == WHITE)
			{
				pgn = pgn + "\n" + line;
				line.clear();
			}

			if (pos.Ply() > 600)
			{
				result = "1/2-1/2";
				comment = "{Adjudication: too long}";
				break;
			}
		}

		stringstream header;
		header << "[Event \"Self-play\"]" << endl <<
			"[Date \"" << CurrentDateStr() << "\"]" << endl <<
			"[White \"" << PROGRAM_NAME << "\"]" << endl <<
			"[Black \"" << PROGRAM_NAME << "\"]" << endl <<
			"[Result \"" << result << "\"]" << endl;

		if (!line.empty())
		{
			pgn = pgn + "\n" + line;
			line.clear();
		}

		if (pos.Ply() > 20)
		{
			std::ofstream ofs;
			ofs.open(pgn_file.c_str(), ios::app);
			if (ofs.good())
				ofs << header.str() << pgn << endl << result << " " << comment << endl << endl;
		}
		else
			--games;

		cout << "Playing: game " << games + 1 << " of " << N << "\r";
	}
	cout << endl;
}
////////////////////////////////////////////////////////////////////////////////

void OnSelfPlay()
{
	if (g_tokens.size() < 2)
		return;
	int numGames = atoi(g_tokens[1].c_str());

	OnSelfPlay("games.pgn", numGames);
}
////////////////////////////////////////////////////////////////////////////////

void OnSetboard()
{
	if (g_pos.SetFEN(g_s.c_str() + 9))
		g_pos.Print();
	else
		out("Illegal FEN");
}
////////////////////////////////////////////////////////////////////////////////

void OnSetoption()
{
	if (g_tokens.size() < 5)
		return;
	if (g_tokens[1] != "name" && g_tokens[3] != "value")
		return;

	string name = g_tokens[2];
	string value = g_tokens[4];

	if (name == "Hash")
		SetHashSize(atoi(value.c_str()));
	else if (name == "PawnHash")
		SetPawnHashSize(atoi(value.c_str()));
	else if (name == "Strength")
		SetStrength(atoi(value.c_str()));
}
////////////////////////////////////////////////////////////////////////////////

void OnSN()
{
	if (g_tokens.size() < 2)
		return;

	g_sd = 0;
	g_sn = atoi(g_tokens[1].c_str());
	g_stHard = 0;
	g_stSoft = 0;
}
////////////////////////////////////////////////////////////////////////////////

void OnST()
{
	if (g_tokens.size() < 2)
		return;

	g_sd = 0;
	g_sn = 0;
	g_stHard = U32(1000 * atof(g_tokens[1].c_str()));
	g_stSoft = U32(1000 * atof(g_tokens[1].c_str()));
}
////////////////////////////////////////////////////////////////////////////////

void OnTest()
{
	g_pos.SetFEN("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -");
}
////////////////////////////////////////////////////////////////////////////////

void OnTime()
{
	if (g_tokens.size() < 2)
		return;

	g_sd = 0;
	g_sn = 0;

	g_restTime = 10 * atoi(g_tokens[1].c_str());
}
////////////////////////////////////////////////////////////////////////////////

void OnTraining(int numGames, int firstIter, int lastIter)
{
	RandSeed(time(0));

	string backup_file = "weights_0.txt";
	g_eval.Write(backup_file.c_str());
	cout << "Weights saved in " << backup_file << endl;

	for (int iter = firstIter; iter <= lastIter; ++iter)
	{
		cout << endl << "*** MATCH " << iter - firstIter + 1
			<< " of " << lastIter - firstIter + 1<< " ***" << endl;

		/*
		char pgn_file[256];
		char fen_file[256];
		char weights_file[256];
		sprintf(pgn_file, "games_%d.pgn", iter);
		sprintf(fen_file, "games_%d.fen", iter);
		sprintf(weights_file, "weights_%d.txt", iter);
		*/

		char pgn_file[] = "games.pgn";
		char fen_file[] = "games.fen";
		char weights_file[256];
		sprintf(weights_file, "weights_%d.txt", iter);

		OnSelfPlay(pgn_file, numGames);
		PgnToFen(pgn_file, fen_file, 6, 999, 1);

		OnZero();
		OnLearn(fen_file, 2, STOCHASTIC_GRADIENT_DESCENT | COORDINATE_DESCENT);

		g_eval.Write(weights_file);
		cout << "Weights saved in " << weights_file << endl;
	}
}
////////////////////////////////////////////////////////////////////////////////

void OnTraining()
{
	int numGames = (g_tokens.size() > 1)? atoi(g_tokens[1].c_str()) : 10000;
	int firstIter = 1;
	int lastIter = 1;

	if (g_tokens.size() == 3)
		lastIter = atoi(g_tokens[2].c_str());
	else if (g_tokens.size() >= 4)
	{
		firstIter = atoi(g_tokens[2].c_str());
		lastIter = atoi(g_tokens[3].c_str());
	}

	g_startTrainingTime = GetProcTime();
	OnTraining(numGames, firstIter, lastIter);
}
////////////////////////////////////////////////////////////////////////////////

void OnUCI()
{
	g_console = false;
	g_xboard = false;
	g_uci = true;

	out("id name " + PROGRAM_NAME);
	out("id author Vladimir Medvedev");

	stringstream ss1;
	ss1 << "option name Hash type spin" <<
		" default " << DEFAULT_HASH_SIZE <<
		" min " << MIN_HASH_SIZE <<
		" max " << MAX_HASH_SIZE;
	out(ss1);

	stringstream ss2;
	ss2 << "option name PawnHash type spin" <<
		" default " << DEFAULT_PAWN_HASH_SIZE <<
		" min " << MIN_PAWN_HASH_SIZE <<
		" max " << MAX_PAWN_HASH_SIZE;
	out(ss2);

	stringstream ss3;
	ss3 << "option name Strength type spin" <<
		" default " << 100 <<
		" min " << 0 <<
		" max " << 100;
	out(ss3);

	out("uciok");
}
////////////////////////////////////////////////////////////////////////////////

void OnXboard()
{
	g_console = false;
	g_xboard = true;
	g_uci = false;

	cout << endl;
}
////////////////////////////////////////////////////////////////////////////////

void OnZero()
{
	std::vector<int> x;
	x.resize(NUM_FEATURES);

	if (g_tokens.size() > 1 && g_tokens[1].find("rand") == 0)
	{
		RandSeed(time(0));
		for (size_t i = 0; i < x.size(); ++i)
			x[i] = Rand32() % 5 - 2;
	}

	g_eval.SetValues(x);
	g_eval.Write(WEIGHTS_FILE);
}
////////////////////////////////////////////////////////////////////////////////

void RunCommandLine()
{
	while (1)
	{
		if (!g_queue.empty())
		{
			g_s = g_queue.front();
			g_queue.pop_front();
		}
		else
		{
			if (g_console)
			{
				if (g_pos.Side() == WHITE)
					cout << "White(" << g_pos.Ply() / 2 + 1 << "): ";
				else
					cout << "Black(" << g_pos.Ply() / 2 + 1 << "): ";
			}
			getline(cin, g_s);
		}

		if (g_s.empty())
			continue;

		Log(string("> ") + g_s);

		Split(g_s, g_tokens);
		if (g_tokens.empty())
			continue;

		string cmd = g_tokens[0];

#define ON_CMD(pattern, minLen, action) \
    if (Is(cmd, #pattern, minLen))      \
    {                                   \
      action;                           \
      continue;                         \
    }

		ON_CMD(analyze,    1, OnAnalyze())
		ON_CMD(board,      1, g_pos.Print())
		ON_CMD(diff,       2, OnDiff())
		ON_CMD(dump,       2, OnDump())
		ON_CMD(eval,       2, OnEval())
		ON_CMD(fen,        2, OnFEN())
		ON_CMD(flip,       2, OnFlip())
		ON_CMD(force,      2, g_force = true)
		ON_CMD(go,         1, OnGo())
		ON_CMD(isready,    1, OnIsready())
		ON_CMD(learn,      3, OnLearn())
		ON_CMD(level,      3, OnLevel())
		ON_CMD(list,       2, OnList())
		ON_CMD(load,       2, OnLoad())
		ON_CMD(mt,         2, OnMT())
		ON_CMD(mt2,        3, OnMT2())
		ON_CMD(new,        1, OnNew())
		ON_CMD(perft,      2, OnPerft())
		ON_CMD(pgntofen,   2, OnPgnToFen())
		ON_CMD(ping,       2, OnPing())
		ON_CMD(position,   2, OnPosition())
		ON_CMD(predict,    3, OnPredict())
		ON_CMD(protover,   3, OnProtover())
		ON_CMD(psq,        2, OnPSQ())
		ON_CMD(quit,       1, break)
		ON_CMD(sd,         2, OnSD())
		ON_CMD(see,        3, OnSEE())
		ON_CMD(selfplay,   4, OnSelfPlay())
		ON_CMD(sp,         2, OnSelfPlay())
		ON_CMD(setboard,   3, OnSetboard())
		ON_CMD(setoption,  3, OnSetoption())
		ON_CMD(sn,         2, OnSN())
		ON_CMD(st,         2, OnST())
		ON_CMD(test,       2, OnTest())
		ON_CMD(time,       2, OnTime())
		ON_CMD(training,   2, OnTraining())
		ON_CMD(uci,        1, OnUCI())
		ON_CMD(ucinewgame, 4, OnNew())
		ON_CMD(undo,       1, g_pos.UnmakeMove())
		ON_CMD(xboard,     1, OnXboard())
		ON_CMD(zero,       1, OnZero())
#undef ON_CMD

		if (CanBeMove(cmd))
		{
			Move mv = StrToMove(cmd, g_pos);
			if (mv)
			{
				g_pos.MakeMove(mv);
				string result, comment;
				if (IsGameOver(g_pos, result, comment))
					cout << result << " " << comment << endl << endl;
				else if (!g_force)
					OnGo();
				continue;
			}
		}

		if (!g_uci)
			out("Unknown command: " + cmd);
	}
}
////////////////////////////////////////////////////////////////////////////////

int main(int argc, const char* argv[])
{
	InitIO();

	InitBitboards();
	Position::InitHashNumbers();
	InitEval();
	g_pos.SetInitial();

	double hashMb = DEFAULT_HASH_SIZE;
	double pawnHashMb = DEFAULT_PAWN_HASH_SIZE;

	for (int i = 1; i < argc; ++i)
	{
		if (!strcmp(argv[i], "-log"))
			g_log = fopen("GreKo.log", "at");
		else if (i < argc - 1)
		{
			if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "-H") || !strcmp(argv[i], "-hash") || !strcmp(argv[i], "-Hash"))
			{
				hashMb = atof(argv[i + 1]);
				if (hashMb < MIN_HASH_SIZE || hashMb > MAX_HASH_SIZE)
					hashMb = DEFAULT_HASH_SIZE;
			}
			if (!strcmp(argv[i], "-ph") || !strcmp(argv[i], "-pawnHash"))
			{
				pawnHashMb = atof(argv[i + 1]);
				if (pawnHashMb < MIN_PAWN_HASH_SIZE || pawnHashMb > MAX_PAWN_HASH_SIZE)
					pawnHashMb = DEFAULT_PAWN_HASH_SIZE;
			}
			else if (!strcmp(argv[i], "-s") || !strcmp(argv[i], "-S") || !strcmp(argv[i], "-strength") || !strcmp(argv[i], "-Strength"))
			{
				int level = atoi(argv[i + 1]);
				SetStrength(level);
			}
		}
	}

	SetHashSize(hashMb);
	SetPawnHashSize(pawnHashMb);

	if (IsPipe())
	{
		out(PROGRAM_NAME + " by Vladimir Medvedev");
		g_uci = true;
		g_xboard = false;
		g_console = false;
	}
	else
	{
		Highlight(true);
		out("\n" + PROGRAM_NAME + " (" + RELEASE_DATE + ")\n");
		Highlight(false);
		g_uci = false;
		g_xboard = false;
		g_console = true;
	}

	RunCommandLine();
	return 0;
}
////////////////////////////////////////////////////////////////////////////////

//   GreKo chess engine
//   (c) 2002-2020 Vladimir Medvedev <vrm@bk.ru>
//   http://greko.su

#include "eval.h"
#include "eval_params.h"
#include "learn.h"
#include "moves.h"
#include "notation.h"
#include "search.h"
#include "utils.h"

const double K = 180;
U32 g_startTrainingTime = 0;

void SgdIteration(
	const string& fenFile,
	vector<int>& x,
	double LR,
	const vector<int>& learnParams)
{
	vector<double> w;
	w.resize(x.size());
	for (size_t i = 0; i < w.size(); ++i)
		w[i] = x[i];

	ifstream ifs(fenFile.c_str());
	string s;

	while (getline(ifs, s))
	{
		if (s.length() < 5)
			continue;

		char chRes = s[0];
		double result = 0;
		if (chRes == '1')
			result = 1;
		else if (chRes == '0')
			result = 0;
		else if (chRes == '=')
			result = 0.5;
		else
			continue;

		string fen = string(s.c_str() + 2);
		Position pos;
		if (!pos.SetFEN(fen))
			continue;

		Features featObj(pos);

		double e = 0;
		for (size_t i = 0; i < w.size(); ++i)
			e += w[i] * featObj.GetVector()[i];

		double prediction = 1. / (1. + exp(-e / K));
		double err = prediction - result;

		for (size_t i = 0; i < w.size(); ++i)
			w[i] -= LR * err * featObj.GetVector()[i] * learnParams[i];
	}

	for (size_t i = 0; i < w.size(); ++i)
		x[i] = int(w[i]);
}
////////////////////////////////////////////////////////////////////////////////

void SgdInfo(double y0Start, double y0, double LR, int iter)
{
	int t = (GetProcTime() - g_startTrainingTime) / 1000;
	int hh = t / 3600;
	int mm = (t - 3600 * hh) / 60;
	int ss = t - 3600 * hh - 60 * mm;

	cout << right;
	cout << setw(2) << setfill('0') << hh << ":";
	cout << setw(2) << setfill('0') << mm << ":";
	cout << setw(2) << setfill('0') << ss << " ";
	cout << setfill(' ');

	printf("      %8lf %8.6lf %c   (%d) LR = %12.10lf\r",
		y0, 100 * (y0 - y0Start) / y0Start, '%', iter, LR);
}
////////////////////////////////////////////////////////////////////////////////

void Sgd(
	const string& fenFile,
	vector<int>& x0,
	const vector<int>& learnParams)
{
	InitEval(x0);
	double y0 = Predict(fenFile);
	double y0Start = y0;

	cout << endl;
	cout << "Algorithm:     stochastic gradient descent" << endl;
	cout << "Parameters:    " << x0.size() << endl;
	cout << "Initial value: " << left << y0Start << endl;
	cout << endl;

	vector<int> x = x0;
	y0 = 1e10;

	double LR = 1.0;
	int iter = 0;

	while (LR > 1e-10)
	{
		SgdIteration(fenFile, x, LR, learnParams);

		InitEval(x);
		double y = Predict(fenFile);
		if (y < y0)
		{
			SgdInfo(y0Start, y, LR, ++iter);
			x0 = x;
			y0 = y;
			g_eval.SetValues(x0);
			g_eval.Write(WEIGHTS_FILE);
		}
		else
		{
			SgdInfo(y0Start, y0, LR, ++iter);
			cout << "\n";
			LR /= 10.0;
			iter = 0;
		}
	}

	InitEval(x0);
}
////////////////////////////////////////////////////////////////////////////////

void CoordinateDescentInfo(int param, int value, double y0Start, double y0, int iter, int numIters)
{
	int t = (GetProcTime() - g_startTrainingTime) / 1000;
	int hh = t / 3600;
	int mm = (t - 3600 * hh) / 60;
	int ss = t - 3600 * hh - 60 * mm;

	stringstream buf;
	static int prev_len = 0;

	buf << right;
	buf << setw(2) << setfill('0') << hh << ":";
	buf << setw(2) << setfill('0') << mm << ":";
	buf << setw(2) << setfill('0') << ss << " ";
	buf << setfill(' ');

	buf << left << "      " << setw(8) << y0 <<
		" " << setprecision(6) << 100 * (y0 - y0Start) / y0Start << " % " <<
		" Iter " << iter + 1 << "/" << numIters << " " <<
		g_eval.IndexToParamName(param) << " = " << value;

	int len = buf.str().length();
	for (int i = 0; i < prev_len - len; ++i)
		buf << " ";
	prev_len = len;

	buf << "\r";
	cout << buf.str();
}
////////////////////////////////////////////////////////////////////////////////

void CoordinateDescent(
	const string& fenFile,
	vector<int>& x0,
	int numIters,
	const vector<int>& learnParams)
{
	InitEval(x0);
	double y0 = Predict(fenFile);
	double y0Start = y0;

	cout << endl;
	cout << "Algorithm:     coordinate descent" << endl;
	cout << "Parameters:    " << x0.size() << endl;
	cout << "Initial value: " << left << y0Start << endl;
	cout << endl;

	for (int iter = 0; iter < numIters; ++iter)
	{
		for (size_t param = 0; param < x0.size(); ++param)
		{
			if (learnParams[param] == 0)
				continue;

			CoordinateDescentInfo(param, x0[param], y0Start, y0, iter, numIters);

			int step = (iter == 0 && numIters > 1)? 4 : 1;
			while (step > 0)
			{
				vector<int> x1 = x0;
				x1[param] += step;

				if (x1[param] != x0[param])
				{
					InitEval(x1);
					double y1 = Predict(fenFile);
					if (y1 < y0)
					{
						x0 = x1;
						y0 = y1;
						CoordinateDescentInfo(param, x0[param], y0Start, y0, iter, numIters);
						g_eval.SetValues(x0);
						g_eval.Write(WEIGHTS_FILE);
						continue;
					}
				}

				vector<int> x2 = x0;
				x2[param] -= step;

				if (x2[param] != x0[param])
				{
					InitEval(x2);
					double y2 = Predict(fenFile);
					if (y2 < y0)
					{
						x0 = x2;
						y0 = y2;
						CoordinateDescentInfo(param, x0[param], y0Start, y0, iter, numIters);
						g_eval.SetValues(x0);
						g_eval.Write(WEIGHTS_FILE);
						continue;
					}
				}

				step /= 2;
			}
		}
	}

	cout << endl;
	InitEval(x0);
}
////////////////////////////////////////////////////////////////////////////////

int CountGames(const string& file)
{
	int games = 0;
	ifstream ifs(file.c_str());
	if (ifs.good())
	{
		string s;
		while (getline(ifs, s))
		{
			if (s.find("[Event") == 0)
				++games;
		}
	}
	return games;
}
////////////////////////////////////////////////////////////////////////////////

double ErrSq(const string& s)
{
	char chRes = s[0];
	double result = 0;
	if (chRes == '1')
		result = 1;
	else if (chRes == '0')
		result = 0;
	else if (chRes == '=')
		result = 0.5;
	else
	{
		cout << "Illegal string: " << s << endl;
		return -1;
	}

	string fen = string(s.c_str() + 2);
	Position pos;
	if (!pos.SetFEN(fen))
	{
		cout << "ERR FEN: " << fen << endl;
		return -1;
	}

	EVAL e = Evaluate(pos);
	if (pos.Side() == BLACK)
		e = -e;

	double prediction = 1. / (1. + exp(-e / K));
	double errSq = (prediction - result) * (prediction - result);

	return errSq;
}
////////////////////////////////////////////////////////////////////////////////

bool PgnToFen(const string& pgnFile,
	const string& fenFile,
	int minPly,
	int maxPly,
	int fensPerGame)
{
	RandSeed(time(0));

	ifstream ifs(pgnFile.c_str());
	if (!ifs.good())
	{
		cout << "Can't open file: " << pgnFile << endl << endl;
		return false;
	}

	ofstream ofs(fenFile.c_str());
	if (!ofs.good())
	{
		cout << "Can't open file: " << fenFile << endl << endl;
		return false;
	}

	string s;
	Position pos;
	vector<string> fens;
	int games = 0;
	int total = CountGames(pgnFile);

	while (getline(ifs, s))
	{
		if (s.empty())
			continue;
		if (s.find("[Event") == 0)
		{
			++games;
			cout << "Processing: game " << games << " of " << total << "\r";
			pos.SetInitial();
			fens.clear();
			continue;
		}
		if (s.find("[") == 0)
			continue;

		vector<string> tokens;
		Split(s, tokens, ". ");

		for (size_t i = 0; i < tokens.size(); ++i)
		{
			string tk = tokens[i];
			if (tk == "1-0" || tk == "0-1" || tk == "1/2-1/2" || tk == "*")
			{
				char r = '?';
				if (tk == "1-0")
					r = '1';
				else if (tk == "0-1")
					r = '0';
				else if (tk == "1/2-1/2")
					r = '=';
				else
				{
					fens.clear();
					break;
				}

				if (!fens.empty())
				{
					for (int j = 0; j < fensPerGame; ++j)
					{
						int index = Rand32() % fens.size();
						ofs << r << " " << fens[index] << endl;
					}
				}

				fens.clear();
				break;
			}

			if (!CanBeMove(tk))
				continue;

			Move mv = StrToMove(tk, pos);
			if (mv && pos.MakeMove(mv))
			{
				if (pos.Ply() >= minPly && pos.Ply() <= maxPly)
				{
					if (!mv.Captured() && !mv.Promotion() && !pos.InCheck())
						fens.push_back(pos.FEN());
				}
			}
		}
	}
	cout << endl;
	return true;
}
////////////////////////////////////////////////////////////////////////////////

double Predict(const string& fenFile)
{
	ifstream ifs(fenFile.c_str());
	int n = 0;
	double errSqSum = 0;
	string s;

	while (getline(ifs, s))
	{
		if (s.length() < 5)
			continue;

		double errSq = ErrSq(s);
		if (errSq < 0)
			continue;

		++n;
		errSqSum += errSq;
	}

	return sqrt(errSqSum / n);
}
////////////////////////////////////////////////////////////////////////////////

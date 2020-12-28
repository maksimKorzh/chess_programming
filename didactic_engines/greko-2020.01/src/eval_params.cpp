//   GreKo chess engine
//   (c) 2002-2020 Vladimir Medvedev <vrm@bk.ru>
//   http://greko.su

#include "eval_params.h"
#include "notation.h"
#include "utils.h"

extern string PROGRAM_NAME;
extern string RELEASE_DATE;

EvalConfig g_eval;
size_t NUM_FEATURES;

void SetDefaultValues(vector<int>& x)
{
	static const int data[] =
	{
		110, -39, 7, 87, 60, -5, 15, 32, -4, 0, 0, 0, 1, 37, -1, 46, 14, -4,
		3, -52, -75, 38, -13, 66, 15, 23, -15, 13, 3, 19, 5, -6, -8, -11, -3,
		-21, 24, -4, -1, -5, 0, -1, 43, 0, 12, 5, 1, -44, 24, 0, 0, 5, 3, 2,
		-20, 9, -99, 361, -34, 5, -17, -9, 3, 75, -45, -33, -39, 36, 6, 12,
		-107, 24, 2, 0, 25, 75, 38, 269, -9, 6, -14, -11, 14, 245, -162, -19,
		-2, 36, -20, -9, 32, 17, 25, 15, 0, 58, 62, 271, -15, 6, 68, 24, 4,
		50, 10, -147, 49, 51, 19, 1, 8, 16, -56, 71, 1102, 3, -2, 31, -42,
		10, 197, -139, -79, -52, -4, -1, 3, 0, 0, 0, 3, -2, -50, -26, 0, -21,
		-39, -63, 4, -24, -24, -16, 0, 11, -8, 2, 37, -3, 0, 4, -5, 174, -13,
		-6, 0, -14, -355, 63, 255, -17, 41, 20, 112, 1, -1, 94, 43, -1, 3,
		-8, 0, -167, -34, 14, 19, 2, 7, -25, 67, 11, 142, -96, 55, 1, 4, 52,
		111, -21, -3, -26, 4, -12, 17, 0, -14, 0, 0, -22, 0, 3, 6, 5, 7, -14,
		35, 18, 15, -13, 0, 17, 56, 5, 0, 10, 1, 4, -11, -4, 37, 298, -26,
		2, -54, 21, -9, 96, -27, 52, -56, 24, -10, -5, -13, 22, 4, 0, 62, 36,
		63, 286, 2, -4, 0, 20, -3, 193, -61, 44, -33, 23, -1, 5, 18, -50, 11,
		49, 0, 59, 122, 603, 5, -3, -31, 26, -1, 224, -131, 34, 16, -12, 12,
		19, 25, 27, -39, 79, 954, 14, 7, 9, 74, 0, 317, -3, -75, -170, 3, 7,
		47, 15, 0, 0, -58, 0, -56, 50, 0, -5, -15, 40, -19, -14, -94, 19, -5,
		2, 2, 0, 37, 6, 9, 0, 0, -37, 49, 2, 13, 54, -130, 3, 77, -17, 121,
		17
	};

	size_t N = sizeof(data) / sizeof(data[0]);
	x.resize(N);
	memcpy(&(x[0]), data, N * sizeof(int));
}
////////////////////////////////////////////////////////////////////////////////

EvalConfig::EvalConfig() : m_numFeatures(0)
{
#define ADD_TAG(tag, sz) Add(tag, #tag, sz)
	ADD_TAG(Pawn,                   6);
	ADD_TAG(PawnPassedBlocked,      6);
	ADD_TAG(PawnPassedFree,         6);
	ADD_TAG(PawnPassedKingDistance, 2);
	ADD_TAG(PawnPassedSquare,       6);
	ADD_TAG(PawnDoubled,            6);
	ADD_TAG(PawnIsolated,           6);
	ADD_TAG(PawnBackwards,          6);
	ADD_TAG(PawnPhalangue,          6);
	ADD_TAG(PawnRam,                6);
	ADD_TAG(PawnLast,               1);
	ADD_TAG(Knight,                 6);
	ADD_TAG(KnightMobility,         2);
	ADD_TAG(KnightKingDistance,     2);
	ADD_TAG(KnightStrong,           6);
	ADD_TAG(KnightAttack,           4);
	ADD_TAG(Bishop,                 6);
	ADD_TAG(BishopMobility,         2);
	ADD_TAG(BishopKingDistance,     2);
	ADD_TAG(BishopStrong,           6);
	ADD_TAG(BishopAttack,           4);
	ADD_TAG(Rook,                   6);
	ADD_TAG(RookMobility,           2);
	ADD_TAG(RookKingDistance,       2);
	ADD_TAG(RookOpen,               1);
	ADD_TAG(RookSemiOpen,           1);
	ADD_TAG(Rook7th,                1);
	ADD_TAG(RookAttack,             4);
	ADD_TAG(Queen,                  6);
	ADD_TAG(QueenMobility,          2);
	ADD_TAG(QueenKingDistance,      2);
	ADD_TAG(Queen7th,               1);
	ADD_TAG(QueenAttack,            4);
	ADD_TAG(King,                   6);
	ADD_TAG(KingPawnShield,         2);
	ADD_TAG(KingPawnStorm,          2);
	ADD_TAG(KingExposed,            2);
	ADD_TAG(PiecePairs,            16);
	ADD_TAG(AttackKing,             2);
	ADD_TAG(BoardControl,           2);
	ADD_TAG(Tempo,                  1);
#undef ADD_TAG
	NUM_FEATURES = m_numFeatures;
}
////////////////////////////////////////////////////////////////////////////////

void EvalConfig::Add(size_t tagEnum, const string& tagString, size_t vectorSize)
{
	m_tagEnumToOffset[tagEnum] = m_numFeatures / 2;
	m_numFeatures += 2 * vectorSize;

	m_vectors[tagEnum].resize(vectorSize);
	m_tagStringToTagEnum[tagString] = tagEnum;
	m_tagEnumToTagString[tagEnum] = tagString;
}
////////////////////////////////////////////////////////////////////////////////

Pair EvalConfig::GetPSQ(size_t tagEnum, FLD f)
{
	// A*x*x + B*x + C*y*y + D*y + E*x*y + F

	double x = (Col(f) - 3.5) / 3.5;
	double y = (3.5 - Row(f)) / 3.5;

	const vector<Pair>& v = m_vectors[tagEnum];

	const Pair& F = v[0];
	const Pair& A = v[1];
	const Pair& B = v[2];
	const Pair& C = v[3];
	const Pair& D = v[4];
	const Pair& E = v[5];

	return Pair(
		int(A.mid * x * x + B.mid * x + C.mid * y * y + D.mid * y + E.mid * x * y) + F.mid,
		int(A.end * x * x + B.end * x + C.end * y * y + D.end * y + E.end * x * y) + F.end
	);
}
////////////////////////////////////////////////////////////////////////////////

Pair EvalConfig::GetLinear(size_t tagEnum, int value, int maxValue)
{
	double z = double(value) / maxValue;
	return m_vectors[tagEnum][0] * z;
}
////////////////////////////////////////////////////////////////////////////////

Pair EvalConfig::GetSquare(size_t tagEnum, int value, int maxValue)
{
	double z = double(value) / maxValue;
	return m_vectors[tagEnum][0] * z + m_vectors[tagEnum][1] * z * z;
}
////////////////////////////////////////////////////////////////////////////////

Pair EvalConfig::GetScaled(size_t tagEnum, int value, int maxValue)
{
	if (m_vectors[tagEnum].size() == 2)
		return GetSquare(tagEnum, value, maxValue);
	else
		return GetLinear(tagEnum, value, maxValue);
}
////////////////////////////////////////////////////////////////////////////////

bool EvalConfig::SetValues(const vector<int>& x)
{
	if (x.size() != m_numFeatures)
		return false;

	const int* ptrMid = &(x[0]);
	const int* ptrEnd = &(x[m_numFeatures / 2]);

	for (size_t i = 0; i < NUM_TAGS; ++i)
	{
		vector<Pair>& v = m_vectors[i];
		for (size_t j = 0; j < v.size(); ++j)
			v[j] = Pair(*ptrMid++, *ptrEnd++);
	}

	return true;
}
////////////////////////////////////////////////////////////////////////////////

void EvalConfig::GetValues(vector<int>& x)
{
	x.resize(m_numFeatures);

	int* ptrMid = &(x[0]);
	int* ptrEnd = &(x[m_numFeatures / 2]);

	for (size_t i = 0; i < NUM_TAGS; ++i)
	{
		const vector<Pair>& v = m_vectors[i];
		for (size_t j = 0; j < v.size(); ++j)
		{
			*ptrMid++ = v[j].mid;
			*ptrEnd++ = v[j].end;
		}
	}
}
////////////////////////////////////////////////////////////////////////////////

bool EvalConfig::GetParamInterval(const string& s, size_t& first, size_t& last) const
{
	string tagString;
	bool is_mid = false;
	
	if (s.find("Mid_") == 0)
	{
		is_mid = true;
		tagString = s.c_str() + 4;
	}
	else if (s.find("End_") == 0)
	{
		is_mid = false;
		tagString = s.c_str() + 4;
	}
	else
		return false;

	map<string, size_t>::const_iterator it = m_tagStringToTagEnum.find(tagString);
	if (it == m_tagStringToTagEnum.end())
		return false;

	size_t tagEnum = it->second;
	first = GetOffset(tagEnum);
	if (!is_mid)
		first += NUM_FEATURES / 2;

	last = first + m_vectors[tagEnum].size() - 1;
	return true;
}
////////////////////////////////////////////////////////////////////////////////

bool EvalConfig::Read(const string& filename)
{
	ifstream ifs(filename.c_str());
	if (!ifs.good())
		return false;

	string s;
	while (getline(ifs, s))
	{
		vector<string> tokens;
		Split(s, tokens, " ");
		if (tokens.size() < 2)
			continue;

		string tagString;
		bool is_mid = false;
		
		if (tokens[0].find("Mid_") == 0)
		{
			is_mid = true;
			tagString = tokens[0].c_str() + 4;
		}
		else if (tokens[0].find("End_") == 0)
		{
			is_mid = false;
			tagString = tokens[0].c_str() + 4;
		}
		else
			continue;

		map<string, size_t>::iterator it = m_tagStringToTagEnum.find(tagString);
		if (it == m_tagStringToTagEnum.end())
			continue;

		vector<Pair>& v = m_vectors[it->second];
		if (v.size() != tokens.size() - 1)
		{
			cout << "Incorrect number of parameters for " << tokens[0]
			<< ": " << tokens.size() - 1 << ", expected: " << v.size() << endl;
			continue;
		}
		for (size_t i = 0; i < v.size(); ++i)
		{
			if (is_mid)
				v[i].mid = atoi(tokens[i + 1].c_str());
			else
				v[i].end = atoi(tokens[i + 1].c_str());
		}
	}

	return true;
}
////////////////////////////////////////////////////////////////////////////////

bool EvalConfig::Write(const string& filename)
{
	ofstream ofs(filename.c_str());
	if (!ofs.good())
		return false;

	for (size_t i = 0; i < NUM_TAGS; ++i)
	{
		string tagString = m_tagEnumToTagString[i];
		const vector<Pair>& v = m_vectors[i];
		ofs << "Mid_" << tagString;
		for (size_t j = 0; j < v.size(); ++j)
			ofs << " " << v[j].mid;
		ofs << endl;
	}
	ofs << endl;

	for (size_t i = 0; i < NUM_TAGS; ++i)
	{
		string tagString = m_tagEnumToTagString[i];
		const vector<Pair>& v = m_vectors[i];
		ofs << "End_" << tagString;
		for (size_t j = 0; j < v.size(); ++j)
			ofs << " " << v[j].end;
		ofs << endl;
	}

	return true;
}
////////////////////////////////////////////////////////////////////////////////

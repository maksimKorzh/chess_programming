//   GreKo chess engine
//   (c) 2002-2020 Vladimir Medvedev <vrm@bk.ru>
//   http://greko.su

#ifndef EVAL_PARAMS_H
#define EVAL_PARAMS_H

#include "types.h"

extern string WEIGHTS_FILE;
void SetDefaultValues(vector<int>& x);

enum
{
	Pawn,
	PawnPassedBlocked,
	PawnPassedFree,
	PawnPassedKingDistance,
	PawnPassedSquare,
	PawnDoubled,
	PawnIsolated,
	PawnBackwards,
	PawnPhalangue,
	PawnRam,
	PawnLast,
	Knight,
	KnightMobility,
	KnightKingDistance,
	KnightStrong,
	KnightAttack,
	Bishop,
	BishopMobility,
	BishopKingDistance,
	BishopStrong,
	BishopAttack,
	Rook,
	RookMobility,
	RookKingDistance,
	RookOpen,
	RookSemiOpen,
	Rook7th,
	RookAttack,
	Queen,
	QueenMobility,
	QueenKingDistance,
	Queen7th,
	QueenAttack,
	King,
	KingPawnShield,
	KingPawnStorm,
	KingExposed,
	PiecePairs,
	AttackKing,
	BoardControl,
	Tempo,
	NUM_TAGS
};

class EvalConfig
{
public:
	EvalConfig();
	~EvalConfig() {}

	bool Read(const string& filename);
	bool Write(const string& filename);

	bool SetValues(const vector<int>& x);
	void GetValues(vector<int>& x);

	size_t NumFeatures() const { return m_numFeatures; }
	size_t GetOffset(size_t tagEnum) const { return m_tagEnumToOffset[tagEnum]; }

	Pair GetPSQ(size_t tagEnum, FLD f);
	Pair GetLinear(size_t tagEnum, int value, int maxValue);
	Pair GetSquare(size_t tagEnum, int value, int maxValue);
	Pair GetScaled(size_t tagEnum, int value, int maxValue);

	const vector<Pair>& operator[] (size_t tagEnum) { return m_vectors[tagEnum]; }
	bool GetParamInterval(const string& s, size_t& first, size_t& last) const;

	string IndexToParamName(size_t index) const
	{
		string prefix = "Mid_";
		if (index >= m_numFeatures / 2)
		{
			prefix = "End_";
			index -= m_numFeatures / 2;
		}

		int i = index;
		for (size_t tagEnum = 0; tagEnum < NUM_TAGS; ++tagEnum)
		{
			if (i < (int)m_vectors[tagEnum].size())
			{
				stringstream ss;
				ss << prefix << m_tagEnumToTagString[tagEnum] << "[" << i << "]";
				return ss.str();
			}
			i -= m_vectors[tagEnum].size();
		}

		return "(unknown)";
	}

private:
	void Add(size_t tagEnum, const string& tagString, size_t vectorSize);

	size_t              m_numFeatures;
	vector<Pair>        m_vectors[NUM_TAGS];
	map<string, size_t> m_tagStringToTagEnum;
	string              m_tagEnumToTagString[NUM_TAGS];
	size_t              m_tagEnumToOffset[NUM_TAGS];
};

extern EvalConfig g_eval;
extern size_t NUM_FEATURES;

#endif

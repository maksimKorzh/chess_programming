//   GreKo chess engine
//   (c) 2002-2020 Vladimir Medvedev <vrm@bk.ru>
//   http://greko.su

#ifndef EVAL_H
#define EVAL_H

#include "eval_params.h"
#include "position.h"

struct PawnHashEntry;

class Features
{
public:

	Features(const Position& pos): m_pos(pos) { GetFeatures(); }
	virtual ~Features() {}

	vector<double>& GetVector() { return m_features; }

private:

	void AddFeatureLinear(size_t tagEnum, int value, int maxValue);
	void AddFeatureSquare(size_t tagEnum, int value, int maxValue);
	void AddFeatureScaled(size_t tagEnum, int value, int maxValue);

	void AddFeatureVector(size_t tagEnum, size_t arrayIndex);
	void AddPsqFeatures(size_t tagEnum, FLD f, COLOR side);

	void GetFeatures();
	void GetFeaturesSide(COLOR side, const PawnHashEntry& pentry);

	const Position& m_pos;
	vector<double> m_features;
	double m_mid;
	double m_end;
};

EVAL Evaluate(const Position& pos, EVAL alpha = -INFINITY_SCORE, EVAL beta = INFINITY_SCORE);
void InitEval();
void InitEval(const vector<int>& x);
void SetPawnHashSize(double mb);

#endif

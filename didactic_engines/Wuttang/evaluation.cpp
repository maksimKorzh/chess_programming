#include "evaluation.h"
#include "data.h"
#include "bitboards.h"
#include <stdlib.h>

const int pawnIsolated = -10;
const int pawnDoubled = -20;
const int pawnBackward = -8;
const int pawnPassed[8] = {0, 5, 10, 20, 35, 60, 100, 200};
const int rookOpenFile = 15;
const int rookSemiOpenFile = 10;
const int rookSeventhRank = 20;
const int queenOpenFile = 5;
const int queenSemiOpenFile = 3;
const int kingPawnSafetyBonus = 10;
const int kingExtendedPawnSafetyBonus = 5;
const int kingBishopSafetyBonus = 8;
const int kingSideCastlingBonus = 12;
const int queenSideCastlingBonus = 10;
const int bishopPair = 30;

const int PawnTable[64] =
{
    0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,
    0	,	0	,	0	,	-40	,	-40	,	0	,	0	,	0	,
    1	,	2	,	3	,	-10	,	-10	,	3	,	2	,	1	,
    2	,	4	,	6	,	8	,	8	,	6	,	4	,	2	,
    3	,	6	,	9	,   12	,	12	,	9	,	6	,	3	,
    4	,	8	,	12	,	16	,	16	,	12	,	8	,	4	,
    5	,	10	,	15	,	20	,	20	,	15	,	10	,	5	,
    0	,	0	,	0	,	0	,	0	,	0	,	0	,	0
};

const int KnightTable[64] =
{
    -10	,	-30	,	-10	,	-10	,	-10	,	-10	,	-30	,	-10	,
    -10	,	0	,	0	,	0	,	0	,	0	,	0	,	-10	,
    -10	,	0	,	5	,	5	,	5	,	5	,	0	,	-10	,
    -10	,	0	,	5	,	10	,	10	,	5	,	0	,	-10	,
    -10	,	0	,	5	,	10	,	10	,	5	,	0	,	-10	,
    -10	,	0	,	5	,	5	,	5	,	5	,	0	,	-10	,
    -10	,	0	,	0	,	0	,   0	,	0	,	0	,	-10	,
    -10	,	-10	,	-10	,	-10	,	-10	,	-10	,	-10	,	-10
};

const int BishopTable[64] =
{
    -10	,	-10	,	-20	,	-10	,	-10	,	-20	,	-10	,	-10	,
    -10	,	0	,	0	,	0	,	0	,	0	,	0	,	-10	,
    -10	,	0	,	5	,	5	,	5	,	5	,	0	,	-10	,
    -10	,	0	,	5	,	10	,	10	,	5	,	0	,	-10	,
    -10	,	0	,	5	,	10	,	10	,	5	,	0	,	-10	,
    -10	,	0	,	5	,	5	,	5	,	5	,	0	,	-10	,
    -10	,	0	,	0	,	0	,   0	,	0	,	0	,	-10	,
    -10	,	-10	,	-10	,	-10	,	-10	,	-10	,	-10	,	-10
};

const int KingE[64] =
{
	0	,	10	,	20	,	30	,	30	,	20	,	10	,	0	,
	10  ,	20	,	30	,	40	,	40	,	30	,	20	,	10	,
	20	,	30	,	40	,	50	,	50	,	40	,	30	,	20	,
	30	,	40	,	50	,	60	,	60	,	50	,	40	,	30	,
	30	,	40	,	50	,	60	,	60	,	50	,	40	,	30	,
	20	,	30	,	40	,	50	,	50	,	40	,	30	,	20	,
	10  ,	20	,	30	,	40	,	40	,	30	,	20	,	10	,
	0	,	10	,	20	,	30	,	30	,	20	,	10	,	0
};

const int KingO[64] =
{
	0	,	20	,	40	,	-20	,	0	,	-20	,	40	,	20	,
	-20	,	-20	,	-20	,	-20	,	-20	,	-20	,	-20	,	-20	,
	-40	,	-40	,	-40	,	-40	,	-40	,	-40	,	-40	,	-40	,
	-40	,	-40	,	-40	,	-40	,	-40	,	-40	,	-40	,	-40	,
	-40	,	-40	,	-40	,	-40	,	-40	,	-40	,	-40	,	-40	,
	-40	,	-40	,	-40	,	-40	,	-40	,	-40	,	-40	,	-40	,
	-40	,	-40	,	-40	,	-40	,	-40	,	-40	,	-40	,	-40	,
	-40	,	-40	,	-40	,	-40	,	-40	,	-40	,	-40	,	-40
};

static bool materialDraw(const Position &pos)
{
    if (pos.pceNum[wP] || pos.pceNum[bP])
    {
        return false;
    }
    if (pos.pceNum[wQ] || pos.pceNum[wR] || pos.pceNum[bQ] || pos.pceNum[bR])
    {
        return false;
    }
    if (pos.pceNum[wB] > 1 || pos.pceNum[bB] > 1)
    {
        return false;
    }
    if (pos.pceNum[wN] > 2 || pos.pceNum[bN] > 2)
    {
        return false;
    }
    if (pos.pceNum[wN] && pos.pceNum[wB])
    {
        return false;
    }
    if (pos.pceNum[bN] && pos.pceNum[bB])
    {
        return false;
    }
    return true;
}

const int endgame_mat = pceVal[wR] + 2*pceVal[wN] + 2*pceVal[wP] + pceVal[wK];

int evalPosition(const Position &pos)
{
    int score = pos.material[white] - pos.material[black];
    int pceNum;
    int pce;
    int sq;

    if (materialDraw(pos))
    {
        return 0;
    }

    pce = wP;
    for (pceNum = 0; pceNum < pos.pceNum[pce]; ++pceNum)
    {
        sq = pos.pList[pce][pceNum];
        assert(SqOnBoard(sq));
        score += PawnTable[sq64(sq)];

        if ((isolatedMask[sq64(sq)] & pos.pawns[white]) == 0)
        {
            score += pawnIsolated;
        }
        else if ((pos.pawns[white] & whiteBackwardMask[sq64(sq)]) == 0)
        {
            score += pawnBackward;
        }

        if ((blackPassedMask[sq64(sq)] & pos.pawns[black]) == 0)
        {
            score += pawnPassed[rankSq(sq)];
        }

        if (pos.pawns[white] & whiteDoubledMask[sq64(sq)])
        {
            score += pawnDoubled;
        }
    }

    pce = bP;
    for (pceNum = 0; pceNum < pos.pceNum[pce]; ++pceNum)
    {
        sq = pos.pList[pce][pceNum];
        assert(SqOnBoard(sq));
        score -= PawnTable[MIRROR64(sq64(sq))];

        if ((isolatedMask[sq64(sq)] & pos.pawns[black]) == 0)
        {
            score -= pawnIsolated;
        }
        else if ((pos.pawns[black] & blackBackwardMask[sq64(sq)]) == 0)
        {
            score -= pawnBackward;
        }

        if ((blackPassedMask[sq64(sq)] & pos.pawns[white]) == 0)
        {
            score -= pawnPassed[7 - rankSq(sq)];
        }

        if (pos.pawns[black] & blackDoubledMask[sq64(sq)])
        {
            score -= pawnDoubled;
        }
    }

    pce = wN;
    for (pceNum = 0; pceNum < pos.pceNum[pce]; ++pceNum)
    {
        sq = pos.pList[pce][pceNum];
        assert(SqOnBoard(sq));
        score += KnightTable[sq64(sq)];
    }

    pce = bN;
    for (pceNum = 0; pceNum < pos.pceNum[pce]; ++pceNum)
    {
        sq = pos.pList[pce][pceNum];
        assert(SqOnBoard(sq));
        score -= KnightTable[MIRROR64(sq64(sq))];
    }

    pce = wB;
    for (pceNum = 0; pceNum < pos.pceNum[pce]; ++pceNum)
    {
        sq = pos.pList[pce][pceNum];
        assert(SqOnBoard(sq));
        score += BishopTable[sq64(sq)];
    }

    pce = bB;
    for (pceNum = 0; pceNum < pos.pceNum[pce]; ++pceNum)
    {
        sq = pos.pList[pce][pceNum];
        assert(SqOnBoard(sq));
        score -= BishopTable[MIRROR64(sq64(sq))];
    }

    pce = wR;
    for (pceNum = 0; pceNum < pos.pceNum[pce]; ++pceNum)
    {
        sq = pos.pList[pce][pceNum];
        assert(SqOnBoard(sq));

        if (rankSq(sq) == rank_7)
        {
            score += rookSeventhRank;
        }

        if (!(pos.pawns[both] & fileBBmask[fileSq(sq)]))
        {
            score += rookOpenFile;
        }
        else if (!(pos.pawns[white] & fileBBmask[fileSq(sq)]))
        {
            score += rookSemiOpenFile;
        }
    }

    pce = bR;
    for (pceNum = 0; pceNum < pos.pceNum[pce]; ++pceNum)
    {
        sq = pos.pList[pce][pceNum];
        assert(SqOnBoard(sq));

        if (rankSq(sq) == rank_2)
        {
            score -= rookSeventhRank;
        }

        if (!(pos.pawns[both] & fileBBmask[fileSq(sq)]))
        {
            score -= rookOpenFile;
        }
        else if (!(pos.pawns[black] & fileBBmask[fileSq(sq)]))
        {
            score -= rookSemiOpenFile;
        }
    }

    pce = wQ;
    for (pceNum = 0; pceNum < pos.pceNum[pce]; ++pceNum)
    {
        sq = pos.pList[pce][pceNum];
        assert(SqOnBoard(sq));

        if (!(pos.pawns[both] & fileBBmask[fileSq(sq)]))
        {
            score += queenOpenFile;
        }
        else if (!(pos.pawns[white] & fileBBmask[fileSq(sq)]))
        {
            score += queenSemiOpenFile;
        }
    }

    pce = bQ;
    for (pceNum = 0; pceNum < pos.pceNum[pce]; ++pceNum)
    {
        sq = pos.pList[pce][pceNum];
        assert(SqOnBoard(sq));

        if (!(pos.pawns[both] & fileBBmask[fileSq(sq)]))
        {
            score -= queenOpenFile;
        }
        else if (!(pos.pawns[black] & fileBBmask[fileSq(sq)]))
        {
            score -= queenSemiOpenFile;
        }
    }

    pce = wK;
    sq = pos.pList[pce][0];

    if (pos.material[black] <= endgame_mat)
    {
        score += KingE[sq64(sq)];
    }
    else
    {
        score += KingO[sq64(sq)];
    }

    pce = bK;
    sq = pos.pList[pce][0];

    if (pos.material[white] <= endgame_mat)
    {
        score -= KingE[MIRROR64(sq64(sq))];
    }
    else
    {
        score -= KingO[MIRROR64(sq64(sq))];
    }

    if (pos.pceNum[wB] >= 2) score += bishopPair;
    if (pos.pceNum[bB] >= 2) score -= bishopPair;

    if (pos.side == white)
    {
        return score;
    }
    else
    {
        return -score;
    }
}

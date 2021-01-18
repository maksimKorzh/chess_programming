#include "hashKey.h"
#include "data.h"
#include "validate.h"

Key pieceKeys[13][120];
Key castleKeys[16];
Key sideKey;

void initHashKey()
{
    for (int pce = 0; pce < 13; ++pce)
    {
        for (int sq = 0; sq < 120; ++sq)
        {
            pieceKeys[pce][sq] = RAND64;
        }
    }

    for (int i = 0; i < 16; ++i)
    {
        castleKeys[i] = RAND64;
    }

    sideKey = RAND64;
}

Key generatePosKey(const Position &pos)
{
    Key k = 0ULL;

    for (int sq = 0; sq < sq_no; ++sq)
    {
        if (pos.pieces[sq] != OFFBOARD && pos.pieces[sq] != EMPTY)
        {
            assert(PieceValid(pos.pieces[sq]));
            k ^= pieceKeys[pos.pieces[sq]][sq];
        }
    }

    assert(SideValid(pos.side));
    if (pos.side == white)
    {
        k ^= sideKey;
    }

    if (pos.enPass_sq != NO_SQ)
    {
        assert(SqOnBoard(pos.enPass_sq));
        assert(rankSq(pos.enPass_sq) == rank_3 || rankSq(pos.enPass_sq) == rank_6);
        k ^= pieceKeys[EMPTY][pos.enPass_sq];
    }

    assert(pos.castleRights >= 0 && pos.castleRights < 16);
    k ^= castleKeys[pos.castleRights];

    return k;
}

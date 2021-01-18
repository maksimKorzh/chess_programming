#include "position.h"
#include "validate.h"
#include "hashKey.h"

#include <iostream>
//#include <conio.h>

#define INCHECK ((isSqAttacked(kingSq[side], side^1)))

const int CastlePerm[120] =
{
    15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
    15, 13, 15, 15, 15, 12, 15, 15, 14, 15,
    15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
    15,  7, 15, 15, 15,  3, 15, 15, 11, 15,
    15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15, 15, 15
};

void Position::clearPiece(const int sq)
{
    assert(SqOnBoard(sq));

    int pce = pieces[sq];

    assert(PieceValid(pce));

    int col = pceCol[pce];

    posKey ^= pieceKeys[pce][sq];

    pieces[sq] = EMPTY;
    material[col] -= pceVal[pce];

    if (pceBig[pce])
    {
        --bigPce[col];
        if (pceMaj[pce])
        {
            --majPce[col];
        }
        else
        {
            --minPce[col];
        }
    }
    else
    {
        CLRBIT(pawns[col], sq64(sq));
        CLRBIT(pawns[both], sq64(sq));
    }

    int t_pceNum = -1;

    for (int i = 0; i < pceNum[pce]; ++i)
    {
        if (pList[pce][i] == sq)
        {
            t_pceNum = i;
            break;
        }
    }

    assert(t_pceNum != -1);
    assert(t_pceNum >= 0 && t_pceNum < 10);

    --pceNum[pce];
    pList[pce][t_pceNum] = pList[pce][pceNum[pce]];
}

void Position::addPiece(const int sq, const int pce)
{
    assert(SqOnBoard(sq));
    assert(PieceValid(pce));

    int col = pceCol[pce];

    posKey ^= pieceKeys[pce][sq];

    pieces[sq] = pce;

    if (pceBig[pce])
    {
        ++bigPce[col];
        if (pceMaj[pce])
        {
            ++majPce[col];
        }
        else
        {
            ++minPce[col];
        }
    }
    else
    {
        SETBIT(pawns[col], sq64(sq));
        SETBIT(pawns[both], sq64(sq));
    }

    material[col] += pceVal[pce];
    pList[pce][pceNum[pce]++] = sq;
}

void Position::movePiece(const int from, const int to)
{
    assert(SqOnBoard(from));
    assert(SqOnBoard(to));

    int pce = pieces[from];
    int col = pceCol[pce];

    posKey ^= pieceKeys[pce][from];
    pieces[from] = EMPTY;

    posKey ^= pieceKeys[pce][to];
    pieces[to] = pce;

    if (!pceBig[pce])
    {
        CLRBIT(pawns[col], sq64(from));
        CLRBIT(pawns[both], sq64(from));
        SETBIT(pawns[col], sq64(to));
        SETBIT(pawns[both], sq64(to));
    }

    for (int i = 0; i < pceNum[pce]; ++i)
    {
        if (pList[pce][i] == from)
        {
            pList[pce][i] = to;
            break;
        }
    }
}

bool Position::makeMove(int move)
{
    assert(checkBoard());

    const int from = FROMSQ(move);
    const int to = TOSQ(move);

    assert(SqOnBoard(from));
    assert(SqOnBoard(to));
    assert(SideValid(side));
    assert(PieceValid(pieces[from]));

    history[hisPly].posKey = posKey;

    if (move & MFLAGEP)
    {
        if (side == white)
        {
            clearPiece(to-10);
        }
        else
        {
            clearPiece(to+10);
        }
    }
    else if (move & MFLAGCA)
    {
        switch(to)
        {
        case C1 :
            movePiece(A1, D1);
            break;
        case C8 :
            movePiece(A8, D8);
            break;
        case G1 :
            movePiece(H1, F1);
            break;
        case G8 :
            movePiece(H8, F8);
            break;
        default :
            assert(false);
            break;
        }
    }

    if (enPass_sq != NO_SQ)
    {
        posKey ^= pieceKeys[EMPTY][enPass_sq];
    }
    posKey ^= castleKeys[castleRights];

    history[hisPly].move = move;
    history[hisPly].fifty_move = fifty_move;
    history[hisPly].enPass_sq = enPass_sq;
    history[hisPly].castleRights = castleRights;

    castleRights &= CastlePerm[from];
    castleRights &= CastlePerm[to];
    enPass_sq = NO_SQ;

    posKey ^= castleKeys[castleRights];

    int captured = CAPTURED(move);
    ++fifty_move;

    if (captured != EMPTY)
    {
        assert(PieceValid(captured));
        clearPiece(to);
        fifty_move = 0;
    }

    ++hisPly;
    ++ply;

    if (PiecePawn[pieces[from]])
    {
        fifty_move = 0;
        if (move & MFLAGPS)
        {
            if (side == white)
            {
                enPass_sq = from+10;
                assert(rankSq(enPass_sq) == rank_3);
            }
            else
            {
                enPass_sq = from-10;
                assert(rankSq(enPass_sq) == rank_6);
            }
            posKey ^= pieceKeys[EMPTY][enPass_sq];
        }
    }

    movePiece(from, to);

    int prPiece = PROMOTED(move);
    if (prPiece != EMPTY)
    {
        assert(PieceValid(prPiece) && !PiecePawn[prPiece]);
        clearPiece(to);
        addPiece(to, prPiece);
    }

    if (PieceKing[pieces[to]])
    {
        kingSq[side] = to;
    }

    side ^= 1;
    posKey ^= sideKey;

    assert(checkBoard());

    if (isSqAttacked(kingSq[side^1], side))
    {
        takeMove();
        return false;
    }

    return true;
}

void Position::takeMove()
{
    assert(checkBoard());

    --hisPly;
    --ply;

    int move = history[hisPly].move;
    int to = TOSQ(move);
    int from = FROMSQ(move);

    assert(SqOnBoard(to));
    assert(SqOnBoard(from));

    if (enPass_sq != NO_SQ)
    {
        posKey ^= pieceKeys[EMPTY][enPass_sq];
    }
    posKey ^= castleKeys[castleRights];

    castleRights = history[hisPly].castleRights;
    fifty_move = history[hisPly].fifty_move;
    enPass_sq = history[hisPly].enPass_sq;

    if (enPass_sq != NO_SQ)
    {
        posKey ^= pieceKeys[EMPTY][enPass_sq];
    }
    posKey ^= castleKeys[castleRights];

    side ^= 1;
    posKey ^= sideKey;

    if (move & MFLAGEP)
    {
        if (side == white)
        {
            addPiece(to-10, bP);
        }
        else
        {
            addPiece(to+10, wP);
        }
    }
    else if (move & MFLAGCA)
    {
        switch(to)
        {
        case C1 :
            movePiece(D1, A1);
            break;
        case C8 :
            movePiece(D8, A8);
            break;
        case G1 :
            movePiece(F1, H1);
            break;
        case G8 :
            movePiece(F8, H8);
            break;
        default :
            assert(false);
            break;
        }
    }

    movePiece(to, from);

    if (PieceKing[pieces[from]])
    {
        kingSq[side] = from;
    }

    int captured = CAPTURED(move);
    if (captured != EMPTY)
    {
        assert(PieceValid(captured));
        addPiece(to, captured);
    }

    int prPiece = PROMOTED(move);
    if (prPiece != EMPTY)
    {
        assert(PieceValid(prPiece) && !PiecePawn[prPiece]);
        clearPiece(from);
        addPiece(from, (pceCol[prPiece] == white ? wP : bP));
    }

    assert(checkBoard());
}

void Position::makeNullMove()
{
    assert(checkBoard());
    assert(!INCHECK);

    ++ply;
    history[hisPly].posKey = posKey;

    if (enPass_sq != NO_SQ)
    {
        posKey ^= pieceKeys[EMPTY][enPass_sq];
    }

    history[hisPly].move = NOMOVE;
    history[hisPly].fifty_move = fifty_move;
    history[hisPly].enPass_sq = enPass_sq;
    history[hisPly].castleRights = castleRights;

    enPass_sq = NO_SQ;

    side ^= 1;
    posKey ^= sideKey;

    ++hisPly;

    assert(checkBoard());
}

void Position::takeNullMove()
{
    assert(checkBoard());

    --hisPly;
    --ply;

    /*if (enPass_sq != NO_SQ)
    {
        posKey ^= pieceKeys[EMPTY][enPass_sq];
    }*/

    castleRights = history[hisPly].castleRights;
    fifty_move = history[hisPly].fifty_move;
    enPass_sq = history[hisPly].enPass_sq;

    if (enPass_sq != NO_SQ)
    {
        posKey ^= pieceKeys[EMPTY][enPass_sq];
    }

    side ^= 1;
    posKey ^= sideKey;

    assert(checkBoard());
}



#include <iostream>
#include "movegen.h"
#include "data.h"

static int MvvLvaScores[13][13];

void initMvvLva()
{
    for (int attacker = wP; attacker <= bK; ++attacker)
    {
        for (int victim = wP; victim <= bK; ++victim)
        {
            MvvLvaScores[victim][attacker] = victimScore[victim] + 6 - (victimScore[attacker] / 100);
        }
    }
}

bool MoveExists(Position &pos, int move)
{
    Movelist list;
    generateAllMoves(pos, list);

    for (int moveNum = 0; moveNum < list.count; ++moveNum)
    {
        if (list.moves[moveNum].move == move)
        {
            if (pos.makeMove(list.moves[moveNum].move))
            {
                pos.takeMove();
                return true;
            }
            else
            {
                return false;
            }
        }
    }

    return false;
}

static void addQuietMove(const Position &pos, int move, Movelist &list)
{
    assert(SqOnBoard(FROMSQ(move)));
    assert(SqOnBoard(TOSQ(move)));

    list.moves[list.count].move = move;

    if (pos.searchKillers[0][pos.ply] == move)
    {
        list.moves[list.count].score = 900000;
    }
    else if (pos.searchKillers[1][pos.ply] == move)
    {
        list.moves[list.count].score = 800000;
    }
    else
    {
        list.moves[list.count].score = pos.searchHistory[pos.pieces[FROMSQ(move)]][TOSQ(move)];
    }
    ++list.count;
}

static void addCaptureMove(const Position &pos, int move, Movelist &list)
{
    assert(SqOnBoard(FROMSQ(move)));
    assert(SqOnBoard(TOSQ(move)));
    assert(PieceValid(CAPTURED(move)));

    list.moves[list.count].move = move;
    list.moves[list.count].score = MvvLvaScores[CAPTURED(move)][pos.pieces[FROMSQ(move)]] + 1000000;
    ++list.count;
}

static void addEnPassantMove(const Position &pos, int move, Movelist &list)
{
    assert(SqOnBoard(FROMSQ(move)));
    assert(SqOnBoard(TOSQ(move)));

    list.moves[list.count].move = move;
    list.moves[list.count].score = 105 + 1000000;
    ++list.count;
}

static void addWhitePawnCapMove(const Position &pos, const int from, const int to, const int cap, Movelist &list)
{
    assert(PieceValid(cap));
    assert(SqOnBoard(from));
    assert(SqOnBoard(to));

    if (rankSq(from) == rank_7)
    {
        addCaptureMove(pos, MOVE(from, to, cap, wQ, 0), list);
        addCaptureMove(pos, MOVE(from, to, cap, wR, 0), list);
        addCaptureMove(pos, MOVE(from, to, cap, wB, 0), list);
        addCaptureMove(pos, MOVE(from, to, cap, wN, 0), list);
    }
    else
    {
        addCaptureMove(pos, MOVE(from, to, cap, EMPTY, 0), list);
    }
}

static void addWhitePawnMove(const Position &pos, const int from, const int to, Movelist &list)
{
    assert(SqOnBoard(from));
    assert(SqOnBoard(to));

    if (rankSq(from) == rank_7)
    {
        addQuietMove(pos, MOVE(from, to, EMPTY, wQ, 0), list);
        addQuietMove(pos, MOVE(from, to, EMPTY, wR, 0), list);
        addQuietMove(pos, MOVE(from, to, EMPTY, wB, 0), list);
        addQuietMove(pos, MOVE(from, to, EMPTY, wN, 0), list);
    }
    else
    {
        addQuietMove(pos, MOVE(from, to, EMPTY, EMPTY, 0), list);
    }
}

static void addBlackPawnCapMove(const Position &pos, const int from, const int to, const int cap, Movelist &list)
{
    assert(PieceValid(cap));
    assert(SqOnBoard(from));
    assert(SqOnBoard(to));

    if (rankSq(from) == rank_2)
    {
        addCaptureMove(pos, MOVE(from, to, cap, bQ, 0), list);
        addCaptureMove(pos, MOVE(from, to, cap, bR, 0), list);
        addCaptureMove(pos, MOVE(from, to, cap, bB, 0), list);
        addCaptureMove(pos, MOVE(from, to, cap, bN, 0), list);
    }
    else
    {
        addCaptureMove(pos, MOVE(from, to, cap, EMPTY, 0), list);
    }
}

static void addBlackPawnMove(const Position &pos, const int from, const int to, Movelist &list)
{
    assert(SqOnBoard(from));
    assert(SqOnBoard(to));

    if (rankSq(from) == rank_2)
    {
        addQuietMove(pos, MOVE(from, to, EMPTY, bQ, 0), list);
        addQuietMove(pos, MOVE(from, to, EMPTY, bR, 0), list);
        addQuietMove(pos, MOVE(from, to, EMPTY, bB, 0), list);
        addQuietMove(pos, MOVE(from, to, EMPTY, bN, 0), list);
    }
    else
    {
        addQuietMove(pos, MOVE(from, to, EMPTY, EMPTY, 0), list);
    }
}

void generateAllMoves(const Position &pos, Movelist &list)
{
    assert(pos.checkBoard());
    list.count = 0;

    int pce = EMPTY;
    int side = pos.side;
    int sq = 0, t_sq = 0;
    int pceIndex = 0;
    int dir = 0;

    if (side == white)
    {
        for (int pceNum = 0; pceNum < pos.pceNum[wP]; ++pceNum)
        {
            sq = pos.pList[wP][pceNum];
            assert(SqOnBoard(sq));

            if (pos.pieces[sq+10] == EMPTY)
            {
                addWhitePawnMove(pos, sq, sq+10, list);
                if (rankSq(sq) == rank_2 && pos.pieces[sq+20] == EMPTY)
                {
                    addQuietMove(pos, MOVE(sq, sq+20, EMPTY, EMPTY, MFLAGPS), list);
                }
            }
            if (!SQOFFBOARD(sq+9) && pceCol[pos.pieces[sq+9]] == black)
            {
                addWhitePawnCapMove(pos, sq, sq+9, pos.pieces[sq+9], list);
            }
            if (!SQOFFBOARD(sq+11) && pceCol[pos.pieces[sq+11]] == black)
            {
                addWhitePawnCapMove(pos, sq, sq+11, pos.pieces[sq+11], list);
            }
            if (pos.enPass_sq != NO_SQ)
            {
                if (sq+9 == pos.enPass_sq)
                {
                    addEnPassantMove(pos, MOVE(sq, sq+9, EMPTY, EMPTY, MFLAGEP), list);
                }
                else if (sq+11 == pos.enPass_sq)
                {
                    addEnPassantMove(pos, MOVE(sq, sq+11, EMPTY, EMPTY, MFLAGEP), list);
                }
            }
        }

        //Castling
        if (pos.castleRights & WKCA)
        {
            if (pos.pieces[F1] == EMPTY && pos.pieces[G1] == EMPTY)
            {
                if (!pos.isSqAttacked(E1, black) && !pos.isSqAttacked(F1, black))
                {
                    addQuietMove(pos, MOVE(E1, G1, EMPTY, EMPTY, MFLAGCA), list);
                }
            }
        }

        if (pos.castleRights & WQCA)
        {
            if (pos.pieces[D1] == EMPTY && pos.pieces[C1] == EMPTY && pos.pieces[B1] == EMPTY)
            {
                if (!pos.isSqAttacked(E1, black) && !pos.isSqAttacked(D1, black))
                {
                    addQuietMove(pos, MOVE(E1, C1, EMPTY, EMPTY, MFLAGCA), list);
                }
            }
        }
    }
    else
    {
        for (int pceNum = 0; pceNum < pos.pceNum[bP]; ++pceNum)
        {
            sq = pos.pList[bP][pceNum];
            assert(SqOnBoard(sq));

            if (pos.pieces[sq-10] == EMPTY)
            {
                addBlackPawnMove(pos, sq, sq-10, list);
                if (rankSq(sq) == rank_7 && pos.pieces[sq-20] == EMPTY)
                {
                    addQuietMove(pos, MOVE(sq, sq-20, EMPTY, EMPTY, MFLAGPS), list);
                }
            }
            if (!SQOFFBOARD(sq-9) && pceCol[pos.pieces[sq-9]] == white)
            {
                addBlackPawnCapMove(pos, sq, sq-9, pos.pieces[sq-9], list);
            }
            if (!SQOFFBOARD(sq-11) && pceCol[pos.pieces[sq-11]] == white)
            {
                addBlackPawnCapMove(pos, sq, sq-11, pos.pieces[sq-11], list);
            }
            if (pos.enPass_sq != NO_SQ)
            {
                if (sq-9 == pos.enPass_sq)
                {
                    addEnPassantMove(pos, MOVE(sq, sq-9, EMPTY, EMPTY, MFLAGEP), list);
                }
                else if (sq-11 == pos.enPass_sq)
                {
                    addEnPassantMove(pos, MOVE(sq, sq-11, EMPTY, EMPTY, MFLAGEP), list);
                }
            }
        }

        //Castling
        if (pos.castleRights & BKCA)
        {
            if (pos.pieces[F8] == EMPTY && pos.pieces[G8] == EMPTY)
            {
                if (!pos.isSqAttacked(E8, white) && !pos.isSqAttacked(F8, white))
                {
                    addQuietMove(pos, MOVE(E8, G8, EMPTY, EMPTY, MFLAGCA), list);
                }
            }
        }

        if (pos.castleRights & BQCA)
        {
            if (pos.pieces[D8] == EMPTY && pos.pieces[C8] == EMPTY && pos.pieces[B8] == EMPTY)
            {
                if (!pos.isSqAttacked(E8, white) && !pos.isSqAttacked(D8, white))
                {
                    addQuietMove(pos, MOVE(E8, C8, EMPTY, EMPTY, MFLAGCA), list);
                }
            }
        }
    }

    //Loop for sliding pieces
    pceIndex = LoopSlideIndex[side];
    pce = LoopSlidePce[pceIndex++];

    while (pce != 0)
    {
        assert(PieceValid(pce));
        for (int pceNum = 0; pceNum < pos.pceNum[pce]; ++pceNum)
        {
            sq = pos.pList[pce][pceNum];
            assert(SqOnBoard(sq));

            for (int i = 0; i < NumDir[pce]; ++i)
            {
                dir = PceDir[pce][i];
                t_sq = sq + dir;

                while (!SQOFFBOARD(sq))
                {
                    //black^1 == white   white^1 == black
                    if (pos.pieces[t_sq] != EMPTY)
                    {
                        if (pceCol[pos.pieces[t_sq]] == (side^1))
                        {
                            addCaptureMove(pos, MOVE(sq, t_sq, pos.pieces[t_sq], EMPTY, 0), list);
                        }
                        break;
                    }
                    else
                    {
                        addQuietMove(pos, MOVE(sq, t_sq, EMPTY, EMPTY, 0), list);
                    }
                    t_sq += dir;
                }
            }
        }
        pce = LoopSlidePce[pceIndex++];
    }

    //Loop for non-sliding pieces
    pceIndex = LoopNonSlideIndex[side];
    pce = LoopNonSlidePce[pceIndex++];

    while (pce != 0)
    {
        assert(PieceValid(pce));

        for (int pceNum = 0; pceNum < pos.pceNum[pce]; ++pceNum)
        {
            sq = pos.pList[pce][pceNum];
            assert(SqOnBoard(sq));
            for (int i = 0; i < NumDir[pce]; ++i)
            {
                dir = PceDir[pce][i];
                t_sq = sq + dir;

                if (!SQOFFBOARD(sq))
                {
                    //black^1 == white   white^1 == black
                    if (pos.pieces[t_sq] != EMPTY)
                    {
                        if (pceCol[pos.pieces[t_sq]] == (side^1))
                        {
                            addCaptureMove(pos, MOVE(sq, t_sq, pos.pieces[t_sq], EMPTY, 0), list);
                        }
                    }
                    else
                    {
                        addQuietMove(pos, MOVE(sq, t_sq, EMPTY, EMPTY, 0), list);
                    }
                }
            }
        }
        pce = LoopNonSlidePce[pceIndex++];
    }
}

void generateAllCaps(const Position &pos, Movelist &list)
{
    assert(pos.checkBoard());
    list.count = 0;

    int pce = EMPTY;
    int side = pos.side;
    int sq = 0, t_sq = 0;
    int pceIndex = 0;
    int dir = 0;

    if (side == white)
    {
        for (int pceNum = 0; pceNum < pos.pceNum[wP]; ++pceNum)
        {
            sq = pos.pList[wP][pceNum];
            assert(SqOnBoard(sq));

            if (!SQOFFBOARD(sq+9) && pceCol[pos.pieces[sq+9]] == black)
            {
                addWhitePawnCapMove(pos, sq, sq+9, pos.pieces[sq+9], list);
            }
            if (!SQOFFBOARD(sq+11) && pceCol[pos.pieces[sq+11]] == black)
            {
                addWhitePawnCapMove(pos, sq, sq+11, pos.pieces[sq+11], list);
            }
            if (pos.enPass_sq != NO_SQ)
            {
                if (sq+9 == pos.enPass_sq)
                {
                    addEnPassantMove(pos, MOVE(sq, sq+9, EMPTY, EMPTY, MFLAGEP), list);
                }
                else if (sq+11 == pos.enPass_sq)
                {
                    addEnPassantMove(pos, MOVE(sq, sq+11, EMPTY, EMPTY, MFLAGEP), list);
                }
            }
        }
    }
    else
    {
        for (int pceNum = 0; pceNum < pos.pceNum[bP]; ++pceNum)
        {
            sq = pos.pList[bP][pceNum];
            assert(SqOnBoard(sq));

            if (!SQOFFBOARD(sq-9) && pceCol[pos.pieces[sq-9]] == white)
            {
                addBlackPawnCapMove(pos, sq, sq-9, pos.pieces[sq-9], list);
            }
            if (!SQOFFBOARD(sq-11) && pceCol[pos.pieces[sq-11]] == white)
            {
                addBlackPawnCapMove(pos, sq, sq-11, pos.pieces[sq-11], list);
            }
            if (pos.enPass_sq != NO_SQ)
            {
                if (sq-9 == pos.enPass_sq)
                {
                    addEnPassantMove(pos, MOVE(sq, sq-9, EMPTY, EMPTY, MFLAGEP), list);
                }
                else if (sq-11 == pos.enPass_sq)
                {
                    addEnPassantMove(pos, MOVE(sq, sq-11, EMPTY, EMPTY, MFLAGEP), list);
                }
            }
        }
    }

    //Loop for sliding pieces
    pceIndex = LoopSlideIndex[side];
    pce = LoopSlidePce[pceIndex++];

    while (pce != 0)
    {
        assert(PieceValid(pce));
        for (int pceNum = 0; pceNum < pos.pceNum[pce]; ++pceNum)
        {
            sq = pos.pList[pce][pceNum];
            assert(SqOnBoard(sq));

            for (int i = 0; i < NumDir[pce]; ++i)
            {
                dir = PceDir[pce][i];
                t_sq = sq + dir;

                while (!SQOFFBOARD(sq))
                {
                    //black^1 == white   white^1 == black
                    if (pos.pieces[t_sq] != EMPTY)
                    {
                        if (pceCol[pos.pieces[t_sq]] == (side^1))
                        {
                            addCaptureMove(pos, MOVE(sq, t_sq, pos.pieces[t_sq], EMPTY, 0), list);
                        }
                        break;
                    }
                    t_sq += dir;
                }
            }
        }
        pce = LoopSlidePce[pceIndex++];
    }

    //Loop for non-sliding pieces
    pceIndex = LoopNonSlideIndex[side];
    pce = LoopNonSlidePce[pceIndex++];

    while (pce != 0)
    {
        assert(PieceValid(pce));

        for (int pceNum = 0; pceNum < pos.pceNum[pce]; ++pceNum)
        {
            sq = pos.pList[pce][pceNum];
            assert(SqOnBoard(sq));
            for (int i = 0; i < NumDir[pce]; ++i)
            {
                dir = PceDir[pce][i];
                t_sq = sq + dir;

                if (!SQOFFBOARD(sq))
                {
                    //black^1 == white   white^1 == black
                    if (pos.pieces[t_sq] != EMPTY)
                    {
                        if (pceCol[pos.pieces[t_sq]] == (side^1))
                        {
                            addCaptureMove(pos, MOVE(sq, t_sq, pos.pieces[t_sq], EMPTY, 0), list);
                        }
                    }
                }
            }
        }
        pce = LoopNonSlidePce[pceIndex++];
    }
}

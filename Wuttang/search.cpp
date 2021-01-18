#include "search.h"
#include "position.h"
#include "data.h"
#include "misc.h"
#include "evaluation.h"
#include "movegen.h"
#include "polybook.h"

#include <iostream>
#include <iomanip>

static void checkUp(searchInfo &info)
{
    // .. Check if time up, or interrupt from GUI
    if (info.timeset == true && getTimeMS() > info.stoptime)
    {
        info.stopped = true;
    }

    ReadInput(info);
}

static void pickNextMove(int moveNum, Movelist &list)
{
    Move temp;
    int bestScore = 0;
    int bestNum = moveNum;

    for (int i = moveNum; i < list.count; ++i)
    {
        if (list.moves[i].score > bestScore)
        {
            bestScore = list.moves[i].score;
            bestNum = i;
        }
    }

    temp = list.moves[moveNum];
    list.moves[moveNum] = list.moves[bestNum];
    list.moves[bestNum] = temp;
}

static bool isRepetition(const Position& pos)
{
    for (int i = pos.hisPly - pos.fifty_move; i < pos.hisPly - 1; ++i)
    {
        assert(i >= 0 && i < maxGameMoves);
        if (pos.posKey == pos.history[i].posKey)
        {
            return true;
        }
    }
    return false;
}

static void clearForSearch(Position &pos, searchInfo &info)
{
    for (int i = 0; i < 13; ++i)
    {
        for (int j = 0; j < sq_no; ++j)
        {
            pos.searchHistory[i][j] = 0;
        }
    }

    for (int i = 0; i < 2; ++i)
    {
        for (int j = 0; j < maxDepth; ++j)
        {
            pos.searchKillers[i][j] = NOMOVE;
        }
    }

    //pos.hashTable.overWrite = 0;
    //pos.hashTable.hit = 0;
    //pos.hashTable.cut = 0;
    pos.ply = 0;

    info.stopped = false;
    info.nodes = 0;
    //info.fh = 0;
    //info.fhf = 0;
}

static int quiescence(int alpha, int beta, Position &pos, searchInfo &info)
{
    assert(pos.checkBoard());

    if ((info.nodes & 2047) == 0)
    {
        checkUp(info);
    }

    ++info.nodes;

    if (isRepetition(pos) || pos.fifty_move >= 100)
    {
        return 0;
    }

    if (pos.ply >= maxDepth)
    {
        return evalPosition(pos);
    }

    int score = evalPosition(pos);

    if (score >= beta)
    {
        return beta;
    }

    if (score > alpha)
    {
        alpha = score;
    }

    Movelist list;
    generateAllCaps(pos, list);

    int moveNum = 0;
    //int legal = 0;
    score = -INFINITE;

    for (moveNum = 0; moveNum < list.count; ++moveNum)
    {
        pickNextMove(moveNum, list);

        if (!pos.makeMove(list.moves[moveNum].move))
        {
            continue;
        }

        //++legal;
        score = -quiescence(-beta, -alpha, pos, info);
        pos.takeMove();

        if (info.stopped == true)
        {
            return 0;
        }

        if (score > alpha)
        {
            if (score >= beta)
            {
                /*if (legal == 1)
                {
                    ++info.fhf;
                }
                ++info.fh;*/

                return beta;
            }
            alpha = score;
        }
    }

    return alpha;
}

static int alphaBeta(int alpha, int beta, int depth, Position &pos, searchInfo &info, bool doNull)
{
    assert(pos.checkBoard());

    bool inCheck = pos.isSqAttacked(pos.kingSq[pos.side], pos.side^1);
    bool futility = false;

    if (!inCheck)
    {
        if (depth == 2)
        {
            if (evalPosition(pos) < alpha-pceVal[wN])
            {
                futility = true;
            }
        }
        else if (depth == 3)
        {
            if (evalPosition(pos) < alpha-pceVal[wR])
            {
                futility = true;
            }
        }
        else if (depth == 4)
        {
            if (evalPosition(pos) < alpha-pceVal[wQ])
            {
                futility = true;
            }
        }
    }

    if (depth <= 0 || futility)
    {
        return quiescence(alpha, beta, pos, info);
        //return evalPosition(pos);
    }

    if ((info.nodes & 2047) == 0)
    {
        checkUp(info);
    }

    ++info.nodes;

    if ((isRepetition(pos) || pos.fifty_move >= 100) && pos.ply)
    {
        return 0;
    }

    if (pos.ply > maxDepth-1)
    {
        return evalPosition(pos);
    }

    int score = -INFINITE;
    int pvMove = NOMOVE;

    //HashTable
    if (probeHashEntry(pos, pvMove, score, alpha, beta, depth))
    {
        //++pos.hashTable.cut;
        return score;
    }

    if (inCheck)
    {
        ++depth;
    }
    else
    {
        //Null Move
        if (doNull && (pos.bigPce[pos.side] > 1) && depth > 4)
        {
            pos.makeNullMove();
            score = -alphaBeta(-beta, -beta+1, depth-4, pos, info, false);
            pos.takeNullMove();
            if (info.stopped == true)
            {
                return 0;
            }
            if (abs(score) < ISMATE)
            {
                if (score >= beta)
                //++info.nullCut;
                return beta;
            }
            /*else
            {
                //Decreases search depth considerably and hence performance
                ++depth;
            }*/
        }
    }

    Movelist list;
    generateAllMoves(pos, list);

    int moveNum = 0;
    int legal = 0;
    int oldAlpha = alpha;
    int bestMove = NOMOVE;
    int currMove = NOMOVE;

    int bestScore = -INFINITE;

    score = -INFINITE;

    if (pvMove != NOMOVE)
    {
        for (moveNum = 0; moveNum < list.count; ++moveNum)
        {
            if (list.moves[moveNum].move == pvMove)
            {
                list.moves[moveNum].score = 2000000;
                break;
            }
        }
    }

    for (moveNum = 0; moveNum < list.count; ++moveNum)
    {
        pickNextMove(moveNum, list);

        currMove = list.moves[moveNum].move;

        if (!pos.makeMove(currMove))
        {
            continue;
        }

        ++legal;

        //Late Move Reduction
        if (moveNum > 1 && !inCheck && !(currMove & MFLAGCAP) && !(currMove & MFLAGPROM))
        {
            score = -alphaBeta(-alpha-1, -alpha, depth-3, pos, info, true);

            if (score > alpha)
            {
                score = -alphaBeta(-beta, -alpha, depth-1, pos, info, true);
            }
        }
        else
        {
            score = -alphaBeta(-beta, -alpha, depth-1, pos, info, true);
        }

        pos.takeMove();

        if (info.stopped == true)
        {
            return 0;
        }

        if (score > bestScore)
        {
            bestScore = score;
            bestMove = currMove;
            if (score > alpha)
            {
                if (score >= beta)
                {
                    /*if (legal == 1)
                    {
                        ++info.fhf;
                    }
                    ++info.fh;*/

                    if (!(bestMove & MFLAGCAP))
                    {
                        pos.searchKillers[1][pos.ply] = pos.searchKillers[0][pos.ply];
                        pos.searchKillers[0][pos.ply] = bestMove;
                    }

                    storeHashEntry(pos, bestMove, beta, HFBETA, depth);

                    return beta;
                }
                alpha = score;

                if (!(bestMove & MFLAGCAP))
                {
                    pos.searchHistory[pos.pieces[FROMSQ(bestMove)]][TOSQ(bestMove)] += depth;
                }
            }
        }
    }

    if (legal == 0)
    {
        if (inCheck)
        {
            return -INFINITE + pos.ply;
        }
        else
        {
            return 0;
        }
    }

    if (alpha != oldAlpha)
    {
        storeHashEntry(pos, bestMove, bestScore, HFEXACT, depth);
    }
    else
    {
        storeHashEntry(pos, bestMove, alpha, HFALPHA, depth);
    }

    return alpha;
}

void searchPosition(Position &pos, searchInfo &info)
{
    int bestMove = NOMOVE;
    int bestScore = -INFINITE;
    int currDepth = 0;
    int pvMoves = 0;
    int pvNum = 0;

    clearForSearch(pos, info);

    /*if (engineOptions.useBook == true)
    {
        bestMove = getBookMove(pos);
    }*/

    //Iterative Deepening
    if (bestMove == NOMOVE)
    {
        for (currDepth = 1; currDepth <= info.depth; ++currDepth)
        {                         //alpha     beta
            bestScore = alphaBeta(-INFINITE, INFINITE, currDepth, pos, info, false);

            if (info.stopped == true)
            {
                break;
            }

            pvMoves = getPvLine(currDepth, pos);
            bestMove = pos.pvArray[0];
            if  (info.gameMode == UCIMODE)
            {
                std::cout << "info score cp " << bestScore << " depth " << currDepth << " nodes " << info.nodes << " time " << getTimeMS()-info.starttime << ' ';
            }
            else if (info.gameMode == XBOARDMODE)
            {
                std::cout << currDepth << ' ' << bestScore << ' ' << (getTimeMS() - info.starttime)/10 << ' ' << info.nodes << ' ';
            }
            else if (info.postThinking == true)
            {
                std::cout << "score : " << (float)bestScore/100 << " depth " << currDepth << ' ';
            }
            if (info.gameMode == UCIMODE || info.postThinking == true)
            {
                    if (info.gameMode != XBOARDMODE)
                    {
                        std::cout << "pv";
                    }
                    for (pvNum = 0; pvNum < pvMoves; ++pvNum)
                    {
                        std::cout << ' ' << getMove(pos.pvArray[pvNum]);
                    }
                    std::cout << '\n';
            }
        }
    }

    if (info.gameMode == UCIMODE)
    {
        std::cout << "bestmove " << getMove(bestMove) << '\n';
    }
    else if (info.gameMode == XBOARDMODE)
    {
        std::cout << "move " << getMove(bestMove) << '\n';
        pos.makeMove(bestMove);
    }
    else
    {
        pos.makeMove(bestMove);
        pos.display();
        std::cout << "\n***!! Wuttang makes move " << getMove(bestMove) << " !!***\n";
    }
}

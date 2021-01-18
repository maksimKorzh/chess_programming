#include <iostream>
#include "move.h"
#include "data.h"
#include "movegen.h"

std::string getSq(const int sq)
{
    std::string sqStr = "a1";
    sqStr[0] = 'a' + fileSq(sq);
    sqStr[1] = '1' + rankSq(sq);

    return sqStr;
}

int parseMove(const std::string &moveStr, const Position &pos)
{
    if (moveStr[1] > '8' || moveStr[1] < '1') return NOMOVE;
    if (moveStr[3] > '8' || moveStr[3] < '1') return NOMOVE;
    if (moveStr[0] > 'h' || moveStr[0] < 'a') return NOMOVE;
    if (moveStr[2] > 'h' || moveStr[2] < 'a') return NOMOVE;

    int from = fr2sq(moveStr[0]-'a', moveStr[1]-'1');
    int to = fr2sq(moveStr[2]-'a', moveStr[3]-'1');

    assert(SqOnBoard(from) && SqOnBoard(to));

    int move;
    Movelist list;
    generateAllMoves(pos, list);

    int promPce = EMPTY;

    for (int moveNum = 0; moveNum < list.count; ++moveNum)
    {
        move = list.moves[moveNum].move;
        if (FROMSQ(move) == from && TOSQ(move) == to)
        {
            promPce = PROMOTED(move);
            if (promPce != EMPTY)
            {
                if (IsRQ(promPce) && !IsBQ(promPce) && moveStr[4] == 'r')
                {
                    return move;
                }
                else if (IsRQ(promPce) && IsBQ(promPce) && moveStr[4] == 'q')
                {
                    return move;
                }
                else if (!IsRQ(promPce) && IsBQ(promPce) && moveStr[4] == 'b')
                {
                    return move;
                }
                else if (IsKn(promPce) && moveStr[4] == 'n')
                {
                    return move;
                }
                continue;
            }
            return move;
        }
    }

    return NOMOVE;
}

std::string getMove(const int move)
{
    std::string moveStr;

    int ff = fileSq(FROMSQ(move));
    int rf = rankSq(FROMSQ(move));
    int ft = fileSq(TOSQ(move));
    int rt = rankSq(TOSQ(move));

    int promoted = PROMOTED(move);

    if (promoted != EMPTY)
    {
        char pChar = 'q';
        if (IsKn(promoted))
        {
			pChar = 'n';
		}
		else if(IsRQ(promoted) && !IsBQ(promoted))
        {
			pChar = 'r';
		}
		else if(!IsRQ(promoted) && IsBQ(promoted))
        {
			pChar = 'b';
		}
		moveStr = "e7e8q";
		moveStr[0] = ('a'+ff);
		moveStr[1] = ('1'+rf);
		moveStr[2] = ('a'+ft);
		moveStr[3] = ('1'+rt);
		moveStr[4] = pChar;
    }
    else
    {
        moveStr = "e2e4";
		moveStr[0] = ('a'+ff);
		moveStr[1] = ('1'+rf);
		moveStr[2] = ('a'+ft);
		moveStr[3] = ('1'+rt);
    }

    return moveStr;
}

void Movelist::printList() const
{
    std::string move;
    int score;

    for (int i = 0; i < count; ++i)
    {
        move = getMove(moves[i].move);
        score = moves[i].score;
        std::cout << "\nMove " << i+1 << " > " << move << " ( score :" << score << " )";
    }
    std::cout << "\nMoveList Total " << count << " moves\n";
}

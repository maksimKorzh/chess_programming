#include "perft.h"
#include "movegen.h"
#include "misc.h"

#include <iostream>

using namespace std;

long leafNodes;

void Perft(int depth, Position &pos)
{
    if (depth == 0)
    {
        ++leafNodes;
        return;
    }
    Movelist list;
    generateAllMoves(pos, list);

    for (int moveNum = 0; moveNum < list.count; ++moveNum)
    {
        if (!pos.makeMove(list.moves[moveNum].move))
        {
            continue;
        }
        Perft(depth-1, pos);
        pos.takeMove();
    }
}

void PerftTest(int depth, Position &pos)
{
    assert(pos.checkBoard());

    pos.display();
    cout << "Starting Test To Depth : " << depth << '\n';
    int start = getTimeMS();
    leafNodes = 0;

    Movelist list;
    generateAllMoves(pos, list);

    int move;

    long cumNodes, oldNodes;

    for (int moveNum = 0; moveNum < list.count; ++moveNum)
    {
        move = list.moves[moveNum].move;

        if (!pos.makeMove(move))
        {
            continue;
        }
        cumNodes = leafNodes;
        Perft(depth-1, pos);
        pos.takeMove();
        oldNodes = leafNodes - cumNodes;
        cout << "Move " << moveNum+1 << " : " << getMove(move) << "  " << oldNodes << '\n';
    }
    cout << "\nTest complete : " << leafNodes << " nodes visited in " << getTimeMS()-start << "\n";
}

#ifndef POSITION_H
#define POSITION_H

#include <string>
#include "data.h"
#include "move.h"
#include "bitboards.h"
#include "pvTable.h"

struct posInfo
{
    int move;
    int enPass_sq;
    int castleRights;
    int fifty_move;
    Key posKey;
};

class Position
{
public :
    int pieces[sq_no];
    BitBoard pawns[3];
    int enPass_sq;
    int castleRights;
    int fifty_move;

    int side;

    int kingSq[2];

    int ply;
    int hisPly;

    Key posKey;

    int pceNum[13];
    int bigPce[2];
    int majPce[2];
    int minPce[2];
    int material[2];
    int pList[13][10];
    Hash_Table hashTable;
    int pvArray[maxDepth];

    int searchHistory[13][sq_no];
    int searchKillers[2][maxDepth];

    posInfo history[maxGameMoves];

    void resetBoard();
    void updateListMaterial();
    void parseFEN(const std::string fenStr);
    void display() const;
    bool checkBoard() const;
    bool isSqAttacked(const int sq, const int side) const;
    void clearPiece(const int sq);
    void addPiece(const int sq, const int pce);
    void movePiece(const int from, const int to);
    bool makeMove(int move);
    void takeMove();
    void makeNullMove();
    void takeNullMove();
};

#endif // POSITION_H

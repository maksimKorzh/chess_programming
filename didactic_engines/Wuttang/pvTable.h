#ifndef PVTABLE_H
#define PVTABLE_H

#include "data.h"
#include "move.h"

enum hashFlags {HFNONE, HFALPHA, HFBETA, HFEXACT};

struct Hash_Entry
{
    Key posKey;
    int move;
    int score;
    int depth;
    int flags;
};

struct Hash_Table
{
    Hash_Entry *pTable;
    int numEntries;
    //int newWrite;
	//int overWrite;
	//int hit;
	//int cut;
};

int getPvLine(const int depth, Position &pos);
void initHashTable(Hash_Table &hashTable, const int MB);
void clearHashTable(Hash_Table &hashTable);
bool probeHashEntry(Position &pos, int &move, int &score, int alpha, int beta, int depth);
void storeHashEntry(Position &pos, int move, int score, const int flags, const int depth);
int probePvMove(Position &pos);

#endif // PVTABLE_H

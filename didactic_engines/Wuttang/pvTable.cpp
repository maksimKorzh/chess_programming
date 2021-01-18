#include "pvTable.h"
#include "movegen.h"

#include <iostream>

int getPvLine(const int depth, Position &pos)
{
    assert(depth < maxDepth);

    int move = probePvMove(pos);
    int count = 0;

    while (move != NOMOVE && count < depth)
    {
        assert(count < maxDepth);

        if (MoveExists(pos, move))
        {
            pos.makeMove(move);
            pos.pvArray[count++] = move;
        }
        else
        {
            break;
        }
        move = probePvMove(pos);
    }

    while (pos.ply > 0)
    {
        pos.takeMove();
    }

    return count;
}

void initHashTable(Hash_Table &hashTable, const int MB)
{
    int hashSize = 0x100000 * MB;

    hashTable.numEntries = hashSize / sizeof(Hash_Entry);
    hashTable.numEntries -= 2;

    if (hashTable.pTable != nullptr)
    {
        delete []hashTable.pTable;
    }
    hashTable.pTable = new Hash_Entry[hashTable.numEntries];

    if (hashTable.pTable == nullptr)
    {
        std::cout << "Hash allocation failed, trying " << MB/2 << "MB...\n";
        initHashTable(hashTable, MB/2);
    }
    else
    {
        clearHashTable(hashTable);
        //std::cout << "HashTable init complete with " << hashTable.numEntries << " entries\n";
    }
}

void clearHashTable(Hash_Table &hashTable)
{
    Hash_Entry *hashEntry;

    for (hashEntry = hashTable.pTable; hashEntry < hashTable.pTable + hashTable.numEntries; ++hashEntry)
    {
        hashEntry->posKey = 0ULL;
        hashEntry->move = NOMOVE;
        hashEntry->score = 0;
        hashEntry->depth = 0;
        hashEntry->flags = HFNONE;
    }
    //hashTable.newWrite = 0;
}

bool probeHashEntry(Position &pos, int &move, int &score, int alpha, int beta, int depth)
{
    int index = pos.posKey % pos.hashTable.numEntries;

    assert(index >= 0 && index < pos.hashTable.numEntries);
    assert(depth >= 1 && depth < maxDepth);
    assert(alpha < beta);
    assert(alpha >= -INFINITE && alpha <= INFINITE);
    assert(beta >= -INFINITE && beta <= INFINITE);
    assert(pos.ply >= 0 && pos.ply < maxDepth);

    if (pos.hashTable.pTable[index].posKey == pos.posKey)
    {
        move = pos.hashTable.pTable[index].move;
        if (pos.hashTable.pTable[index].depth >= depth)
        {
            //++pos.getHashTable().hit;

            assert(pos.hashTable.pTable[index].depth >= 0 && pos.hashTable.pTable[index].depth < maxDepth);
            assert(pos.hashTable.pTable[index].flags >= HFALPHA && pos.hashTable.pTable[index].flags <= HFEXACT);

            score = pos.hashTable.pTable[index].score;

            if (score > ISMATE)
            {
                score -= pos.ply;
            }
            else if (score < -ISMATE)
            {
                score += pos.ply;
            }

            switch(pos.hashTable.pTable[index].flags)
            {
                assert(score >= -INFINITE && score <= INFINITE);

                case HFALPHA :
                    if (score <= alpha)
                    {
                        score = alpha;
                        return true;
                    }
                    break;
                case HFBETA :
                    if (score >= beta)
                    {
                        score = beta;
                        return true;
                    }
                    break;
                case HFEXACT :
                    return true;
                    break;
                default :
                    assert(false);
                    break;
            }
        }
    }

    return false;
}

void storeHashEntry(Position &pos, int move, int score, const int flags, const int depth)
{
    int index = pos.posKey % pos.hashTable.numEntries;

    assert(index >= 0 && index < pos.hashTable.numEntries);
    assert(depth >= 1 && depth < maxDepth);
    assert(flags >= HFALPHA && flags <= HFEXACT);
    assert(score > -INFINITE && score < INFINITE);
    assert(pos.ply >= 0 && pos.ply < maxDepth);

    if (score > ISMATE)
    {
        score += pos.ply;
    }
    else if (score < -ISMATE)
    {
        score -= pos.ply;
    }

    /*if (pos.hashTable.pTable[index].posKey != 0ULL)
    {
        if (pos.hashTable.pTable[index].depth > depth)
        {
            return;
        }
    }*/

    pos.hashTable.pTable[index].move = move;
    pos.hashTable.pTable[index].depth = depth;
    pos.hashTable.pTable[index].flags = flags;
    pos.hashTable.pTable[index].posKey = pos.posKey;
    pos.hashTable.pTable[index].score = score;
}

int probePvMove(Position &pos)
{
    int index = pos.posKey % pos.hashTable.numEntries;
    assert(index >= 0 && index <= pos.hashTable.numEntries - 1);

    if (pos.hashTable.pTable[index].posKey == pos.posKey)
    {
        return pos.hashTable.pTable[index].move;
    }
    return NOMOVE;
}

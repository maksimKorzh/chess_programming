#include "polybook.h"
#include "move.h"
#include "misc.h"
#include <fstream>
#include <iostream>
#include <cstdlib>
using namespace std;

unsigned long numEntries = 0;
polyBookEntry *entries;

const int PolyKindOfPiece[13] =
{
    -1, 1, 3, 5, 7, 9, 11, 0, 2, 4, 6, 8, 10
};

void initPolyBook()
{
    engineOptions.useBook = false;

    fstream pFile("performance.bin", ios::in|ios::binary);

    if (pFile.fail())
    {
        cout << "Book not found\n";
    }
    else
    {
        pFile.seekg(0, ios::end);
        unsigned long position = pFile.tellg();

        if (position < sizeof(polyBookEntry))
        {
            cout << "No Entries found in book\n";
            return;
        }
        numEntries = position / sizeof(polyBookEntry);
        //cout << numEntries << " entries found in file\n";

        entries = new polyBookEntry[numEntries];

        pFile.seekg(0);
        polyBookEntry entry;
        size_t returnValue = 0;
        while (!pFile.eof() && returnValue < numEntries)
        {
            pFile.read((char*)&entry, sizeof(entry));
            entries[returnValue++] = entry;
        }
        //cout << returnValue << " entries read from file\n";

        if (numEntries > 0)
        {
            engineOptions.useBook = true;
        }
    }
}

void cleanPolyBook()
{
    delete[] entries;
}

bool hasPawnForCapture(const Position &pos)
{
    int sqWithPawn = 0;
	int targetPce = (pos.side == white ? wP : bP);
	if(pos.enPass_sq != NO_SQ)
    {
		if(pos.side == white)
        {
			sqWithPawn = pos.enPass_sq - 10;
		}
        else
        {
			sqWithPawn = pos.enPass_sq + 10;
		}

		if(pos.pieces[sqWithPawn + 1] == targetPce)
        {
			return true;
		}
		else if(pos.pieces[sqWithPawn - 1] == targetPce)
        {
			return true;
		}
	}
	return false;
}

Key PolyKeyFromBoard(const Position &pos)
{
    int sq = 0, rank = 0, file = 0;
	Key finalKey = 0;
	int piece = EMPTY;
	int polyPiece = 0;
	int offset = 0;

	for (sq = 0; sq < sq_no; ++sq)
    {
        piece = pos.pieces[sq];
        if (piece != OFFBOARD && piece != EMPTY)
        {
            assert(piece >= wP && piece <= bK);
            polyPiece = PolyKindOfPiece[piece];
            rank = rankSq(sq);
            file = fileSq(sq);
            finalKey ^= Random64Poly[(64 * polyPiece) + (8 * rank) + file];
        }
    }

    //castling
    offset = 768;
    if (pos.castleRights & WKCA) finalKey ^= Random64Poly[offset+0];
    if (pos.castleRights & WQCA) finalKey ^= Random64Poly[offset+1];
    if (pos.castleRights & BKCA) finalKey ^= Random64Poly[offset+2];
    if (pos.castleRights & BQCA) finalKey ^= Random64Poly[offset+3];

    //enPassant
    offset = 772;
    if (hasPawnForCapture(pos))
    {
        file = fileSq(pos.enPass_sq);
        finalKey ^= Random64Poly[offset+file];
    }

    if (pos.side == white)
    {
        finalKey ^= Random64Poly[780];
    }

    return finalKey;
}

unsigned short endian_swap_u16(unsigned short x)
{
    x = (x>>8) | (x<<8);
    return x;
}

unsigned int endian_swap_u32(unsigned int x)
{
    x = (x>>24) | ((x<<8) & 0x00FF0000) | ((x>>8) & 0x0000FF00) | (x<<24);
    return x;
}

Key endian_swap_key(Key x)
{
    x = (x>>56) |
        ((x<<40) & 0x00FF000000000000) |
        ((x<<24) & 0x0000FF0000000000) |
        ((x<<8)  & 0x000000FF00000000) |
        ((x>>8)  & 0x00000000FF000000) |
        ((x>>24) & 0x0000000000FF0000) |
        ((x>>40) & 0x000000000000FF00) |
        (x<<56);
    return x;
}

int convertPolyMoveToInternalMove(unsigned short polyMove, Position &pos)
{
    int ff = (polyMove >> 6) & 7;
	int fr = (polyMove >> 9) & 7;
	int tf = (polyMove >> 0) & 7;
	int tr = (polyMove >> 3) & 7;
	int pp = (polyMove >> 12) & 7;

	char moveString[6];
	if (pp == 0)
    {
        moveString[0] = fileChar[ff];
        moveString[1] = rankChar[fr];
        moveString[2] = fileChar[tf];
        moveString[3] = rankChar[tr];
    }
    else
    {
        char promChar = 'q';
        switch(pp)
        {
			case 1: promChar = 'n'; break;
			case 2: promChar = 'b'; break;
			case 3: promChar = 'r'; break;
		}
		moveString[0] = fileChar[ff];
        moveString[1] = rankChar[fr];
        moveString[2] = fileChar[tf];
        moveString[3] = rankChar[tr];
        moveString[4] = promChar;
    }
    return parseMove(moveString, pos);
}

int getBookMove(Position &pos)
{
    Key polyKey = PolyKeyFromBoard(pos);
    polyBookEntry *entry;
    unsigned short move;
    const int MAXBOOKMOVES = 32;
    int bookMoves[MAXBOOKMOVES];
    int tempMove = NOMOVE;
    int count = 0;

    for (entry = entries; entry < entries+numEntries; ++entry)
    {
        if (polyKey == endian_swap_key(entry->key))
        {
            move = endian_swap_u16(entry->move);
            tempMove = convertPolyMoveToInternalMove(move, pos);
            if (tempMove != NOMOVE)
            {
                bookMoves[count++] = tempMove;
                if (count > MAXBOOKMOVES)
                {
                    break;
                }
            }
        }
    }

    if (count != 0)
    {
        srand(getTimeMS());
        int randMove = rand() % count;
        return bookMoves[randMove];
    }
    else
    {
        return NOMOVE;
    }
}

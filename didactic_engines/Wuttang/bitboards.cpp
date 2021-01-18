#include <iostream>
#include "bitboards.h"

BitBoard fileBBmask[8];
BitBoard rankBBmask[8];
BitBoard blackPassedMask[64];
BitBoard whitePassedMask[64];
BitBoard whiteDoubledMask[64];
BitBoard blackDoubledMask[64];
BitBoard whiteBackwardMask[64];
BitBoard blackBackwardMask[64];
BitBoard isolatedMask[64];
BitBoard centerSquares;

void initEvalMask()
{
    int sq;

    for (sq = 0; sq < 8; ++sq)
    {
        fileBBmask[sq] = 0ULL;
        rankBBmask[sq] = 0ULL;
    }

    for (int r = rank_8; r >= rank_1; --r)
    {
        for (int f = file_A; f <= file_H; ++f)
        {
            sq = r*8 + f;
            fileBBmask[f] |= (1ULL<<sq);
            rankBBmask[r] |= (1ULL<<sq);
        }
    }

    for (sq = 0; sq < 64; ++sq)
    {
        blackPassedMask[sq] = 0ULL;
        whitePassedMask[sq] = 0ULL;
        isolatedMask[sq] = 0ULL;
        whiteDoubledMask[sq] = 0ULL;
        blackDoubledMask[sq] = 0ULL;
        whiteBackwardMask[sq] = 0ULL;
        blackBackwardMask[sq] = 0ULL;
    }

    int tsq;

    for (sq = 0; sq < 64; ++sq)
    {
        tsq = sq + 8;
        while (tsq < 64)
        {
            whitePassedMask[sq] |= (1ULL << tsq);
            whiteDoubledMask[sq] |= (1ULL << tsq);
            tsq += 8;
        }

        tsq = sq - 8;
        while (tsq >= 0)
        {
            blackPassedMask[sq] |= (1ULL << tsq);
            blackDoubledMask[sq] |= (1ULL << tsq);
            tsq -= 8;
        }

        if (fileSq(sq120(sq)) > file_A)
        {
            isolatedMask[sq] |= fileBBmask[fileSq(sq120(sq)) - 1];

            tsq = sq + 7;
            while (tsq < 64)
            {
                whitePassedMask[sq] |= (1ULL << tsq);
                tsq += 8;
            }

            tsq = sq - 9;
            while (tsq >= 0)
            {
                blackPassedMask[sq] |= (1ULL << tsq);
                tsq -= 8;
            }
        }

        if (fileSq(sq120(sq)) < file_H)
        {
            isolatedMask[sq] |= fileBBmask[fileSq(sq120(sq)) + 1];

            tsq = sq + 9;
            while (tsq < 64)
            {
                whitePassedMask[sq] |= (1ULL << tsq);
                tsq += 8;
            }

            tsq = sq - 7;
            while (tsq >= 0)
            {
                blackPassedMask[sq] |= (1ULL << tsq);
                tsq -= 8;
            }
        }
        whiteBackwardMask[sq] = blackPassedMask[sq];
        blackBackwardMask[sq] = whitePassedMask[sq];
    }

    centerSquares = 0ULL;
    SETBIT(centerSquares, sq64(D4));
    SETBIT(centerSquares, sq64(D5));
    SETBIT(centerSquares, sq64(E4));
    SETBIT(centerSquares, sq64(E5));
}

void displayBitBoard(const BitBoard &b)
{
    for (int rank = rank_8; rank >= rank_1; --rank)
    {
        for (int file = file_A; file <= file_H; ++file)
        {
            if (b & setMask[8*rank + file])
                std::cout << 'X';
            else
                std::cout << '-';
        }
        std::cout << std::endl;
    }
}

int popBit(BitBoard &bb)
{
    BitBoard b = bb ^ (bb - 1);
    unsigned int fold = (unsigned) ((b & 0xffffffff) ^ (b >> 32));
    bb &= (bb - 1);
    return BitTable[(fold * 0x783a9b23) >> 26];
}

int countBits(BitBoard b)
{
    int r;
    for(r = 0; b; r++, b &= b - 1);
    return r;
}

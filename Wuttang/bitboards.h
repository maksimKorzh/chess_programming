#ifndef BITBOARDS_H
#define BITBOARDS_H

#include "data.h"

#define SETBIT(b, sq) ((b) |= setMask[(sq)])
#define CLRBIT(b, sq) ((b) &= clearMask[(sq)])

extern BitBoard fileBBmask[8];
extern BitBoard rankBBmask[8];
extern BitBoard blackPassedMask[64];
extern BitBoard whitePassedMask[64];
extern BitBoard whiteDoubledMask[64];
extern BitBoard blackDoubledMask[64];
extern BitBoard whiteBackwardMask[64];
extern BitBoard blackBackwardMask[64];
extern BitBoard isolatedMask[64];
extern BitBoard centerSquares;

const BitBoard setMask[64] =
{
    1ull, 1ull<<1, 1ull<<2, 1ull<<3, 1ull<<4, 1ull<<5, 1ull<<6, 1ull<<7,
    1ull<<8, 1ull<<9, 1ull<<10, 1ull<<11, 1ull<<12, 1ull<<13, 1ull<<14, 1ull<<15,
    1ull<<16, 1ull<<17, 1ull<<18, 1ull<<19, 1ull<<20, 1ull<<21, 1ull<<22, 1ull<<23,
    1ull<<24, 1ull<<25, 1ull<<26, 1ull<<27, 1ull<<28, 1ull<<29, 1ull<<30, 1ull<<31,
    1ull<<32, 1ull<<33, 1ull<<34, 1ull<<35, 1ull<<36, 1ull<<37, 1ull<<38, 1ull<<39,
    1ull<<40, 1ull<<41, 1ull<<42, 1ull<<43, 1ull<<44, 1ull<<45, 1ull<<46, 1ull<<47,
    1ull<<48, 1ull<<49, 1ull<<50, 1ull<<51, 1ull<<52, 1ull<<53, 1ull<<54, 1ull<<55,
    1ull<<56, 1ull<<57, 1ull<<58, 1ull<<59, 1ull<<60, 1ull<<61, 1ull<<62, 1ull<<63
};

const BitBoard clearMask[64] =
{
    ~(1ull), ~(1ull<<1), ~(1ull<<2), ~(1ull<<3), ~(1ull<<4), ~(1ull<<5), ~(1ull<<6), ~(1ull<<7),
    ~(1ull<<8), ~(1ull<<9), ~(1ull<<10), ~(1ull<<11), ~(1ull<<12), ~(1ull<<13), ~(1ull<<14), ~(1ull<<15),
    ~(1ull<<16), ~(1ull<<17), ~(1ull<<18), ~(1ull<<19), ~(1ull<<20), ~(1ull<<21), ~(1ull<<22), ~(1ull<<23),
    ~(1ull<<24), ~(1ull<<25), ~(1ull<<26), ~(1ull<<27), ~(1ull<<28), ~(1ull<<29), ~(1ull<<30), ~(1ull<<31),
    ~(1ull<<32), ~(1ull<<33), ~(1ull<<34), ~(1ull<<35), ~(1ull<<36), ~(1ull<<37), ~(1ull<<38), ~(1ull<<39),
    ~(1ull<<40), ~(1ull<<41), ~(1ull<<42), ~(1ull<<43), ~(1ull<<44), ~(1ull<<45), ~(1ull<<46), ~(1ull<<47),
    ~(1ull<<48), ~(1ull<<49), ~(1ull<<50), ~(1ull<<51), ~(1ull<<52), ~(1ull<<53), ~(1ull<<54), ~(1ull<<55),
    ~(1ull<<56), ~(1ull<<57), ~(1ull<<58), ~(1ull<<59), ~(1ull<<60), ~(1ull<<61), ~(1ull<<62), ~(1ull<<63)
};

const int BitTable[64] =
{
    63, 30, 3, 32, 25, 41, 22, 33, 15, 50, 42, 13, 11, 53, 19, 34, 61, 29, 2,
    51, 21, 43, 45, 10, 18, 47, 1, 54, 9, 57, 0, 35, 62, 31, 40, 4, 49, 5, 52,
    26, 60, 6, 23, 44, 46, 27, 56, 16, 7, 39, 48, 24, 59, 14, 12, 55, 38, 28,
    58, 20, 37, 17, 36, 8
};

void initEvalMask();
void displayBitBoard(const BitBoard &b);
int popBit(BitBoard &bb);
int countBits(BitBoard b);

#endif // BITBOARDS_H

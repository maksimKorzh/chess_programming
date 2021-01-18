#include "validate.h"
#include "data.h"

bool SqOnBoard(const int sq)
{
    return (rankSq(sq) == no_rank ? false : true);
}

bool SideValid(const int side)
{
    return (side == white || side == black) ? true : false;
}

bool FileRankValid(const int fr)
{
    return (fr >= 0 && fr <= 7) ? true : false;
}

bool PieceValidEmpty(const int piece)
{
    return (piece >= EMPTY && piece <= bK) ? true : false;
}

bool PieceValid(const int piece)
{
    return (piece >= wP && piece <= bK) ? true : false;
}


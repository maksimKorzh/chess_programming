#ifndef VALIDATE_H
#define VALIDATE_H

bool SqOnBoard(const int sq);
bool SideValid(const int side);
bool FileRankValid(const int fr);
bool PieceValidEmpty(const int piece);
bool PieceValid(const int piece);

#endif // VALIDATE_H

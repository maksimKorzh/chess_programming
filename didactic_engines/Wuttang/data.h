#ifndef DATA_H
#define DATA_H

#include <cstdlib>
#include <string>
#include <stdio.h>
#include "validate.h"

//#define DEBUG

#ifndef DEBUG
#define assert(n)
#else
#define assert(n)\
if(!(n)) { \
printf("%s - Failed",#n); \
printf("On %s ",__DATE__); \
printf("At %s ",__TIME__); \
printf("In File %s ",__FILE__); \
printf("At Line %d\n",__LINE__); \
exit(1);}

#endif // DEBUG

#define rankSq(sq) rankFromSquare[(sq)]
#define fileSq(sq) fileFromSquare[(sq)]
#define sq120(sq64) sq120FromSq64[(sq64)]
#define sq64(sq120) sq64FromSq120[(sq120)]
#define fr2sq(f, r) (21 + (f) + ((r) * 10))
#define IsBQ(p) (PieceBishopQueen[(p)])
#define IsRQ(p) (PieceRookQueen[(p)])
#define IsKn(p) (PieceKnight[(p)])
#define IsKi(p) (PieceKing[(p)])
#define MIRROR64(sq) (Mirror64[(sq)])
#define MAX_HASH 256

#define INFINITE 30000
#define ISMATE (INFINITE - maxDepth)

typedef unsigned long long Key;
typedef unsigned long long  BitBoard;

const std::string name = "Wuttang";

const int maxDepth = 128;
const int maxGameMoves = 2048;
const int maxPositionMoves = 256;

enum GAMEMODE {UCIMODE, XBOARDMODE, CONSOLEMODE};

struct Options
{
    bool useBook = false;
};

//extern Options engineOptions;

const int sq_no = 120;
const std::string startFEN = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

enum color {white, black, both};
const std::string sideChar = "wb-";

enum boardPieces {EMPTY = 0, wP, wN, wB, wR, wQ, wK, bP, bN, bB, bR, bQ, bK};
const std::string pceChar = ".PNBRQKpnbrqk";
const bool pceBig[] = {false, false, true, true, true, true, true, false, true, true, true, true, true};
const bool pceMaj[] = {false, false, false, false, true, true, true, false, false, false, true, true, true};
const bool pceMin[] = {false, false, true, true, false, false, false, false, true, true, false, false, false};
const bool PieceKnight[13] = {false, false, true, false, false, false, false, false, true, false, false, false, false };
const bool PieceKing[13] = {false, false, false, false, false, false, true, false, false, false, false, false, true };
const bool PieceRookQueen[13] = {false, false, false, false, true, true, false, false, false, false, true, true, false };
const bool PieceBishopQueen[13] = {false, false, false, true, false, true, false, false, false, true, false, true, false };
const int KnDir[8] = { -8, -19,	-21, -12, 8, 19, 21, 12 };
const int RkDir[4] = { -1, -10,	1, 10 };
const int BiDir[4] = { -9, -11, 11, 9 };
const int KiDir[8] = { -1, -10,	1, 10, -9, -11, 11, 9 };
const int pceVal[] = {0, 100, 325, 325, 550, 1000, 50000, 100, 325, 325, 550, 1000, 50000};
const int pceCol[] = {both, white, white, white, white, white, white, black, black, black, black, black, black};
const bool PieceSlides[13] = {false, false, false, true, true, true, false, false, false, true, true, true, false};
const bool PiecePawn[13] = {false, true, false, false, false, false, false, true, false, false, false, false, false };

enum castleValues {NO_CASTLING = 0, WKCA = 1, WQCA = 2, BKCA = 4, BQCA = 8};
enum castleSides {KingSide = 0, QueenSide = 1};

enum squares {
    A1 = 21, B1, C1, D1, E1, F1, G1, H1,
    A2 = 31, B2, C2, D2, E2, F2, G2, H2,
    A3 = 41, B3, C3, D3, E3, F3, G3, H3,
    A4 = 51, B4, C4, D4, E4, F4, G4, H4,
    A5 = 61, B5, C5, D5, E5, F5, G5, H5,
    A6 = 71, B6, C6, D6, E6, F6, G6, H6,
    A7 = 81, B7, C7, D7, E7, F7, G7, H7,
    A8 = 91, B8, C8, D8, E8, F8, G8, H8,
    NO_SQ, OFFBOARD
};

enum ranks {rank_1, rank_2, rank_3, rank_4, rank_5, rank_6, rank_7, rank_8, no_rank};
const std::string rankChar = "12345678.";

enum files {file_A, file_B, file_C, file_D, file_E, file_F, file_G, file_H, no_file};
const std::string fileChar = "abcdefgh.";

const int rankFromSquare[sq_no] =
{
    no_rank, no_rank, no_rank, no_rank, no_rank, no_rank, no_rank, no_rank, no_rank, no_rank,
    no_rank, no_rank, no_rank, no_rank, no_rank, no_rank, no_rank, no_rank, no_rank, no_rank,
    no_rank, rank_1, rank_1, rank_1, rank_1, rank_1, rank_1, rank_1, rank_1, no_rank,
    no_rank, rank_2, rank_2, rank_2, rank_2, rank_2, rank_2, rank_2, rank_2, no_rank,
    no_rank, rank_3, rank_3, rank_3, rank_3, rank_3, rank_3, rank_3, rank_3, no_rank,
    no_rank, rank_4, rank_4, rank_4, rank_4, rank_4, rank_4, rank_4, rank_4, no_rank,
    no_rank, rank_5, rank_5, rank_5, rank_5, rank_5, rank_5, rank_5, rank_5, no_rank,
    no_rank, rank_6, rank_6, rank_6, rank_6, rank_6, rank_6, rank_6, rank_6, no_rank,
    no_rank, rank_7, rank_7, rank_7, rank_7, rank_7, rank_7, rank_7, rank_7, no_rank,
    no_rank, rank_8, rank_8, rank_8, rank_8, rank_8, rank_8, rank_8, rank_8, no_rank,
    no_rank, no_rank, no_rank, no_rank, no_rank, no_rank, no_rank, no_rank, no_rank, no_rank,
    no_rank, no_rank, no_rank, no_rank, no_rank, no_rank, no_rank, no_rank, no_rank, no_rank
};

const int fileFromSquare[sq_no] =
{
    no_file, no_file, no_file, no_file, no_file, no_file, no_file, no_file, no_file, no_file,
    no_file, no_file, no_file, no_file, no_file, no_file, no_file, no_file, no_file, no_file,
    no_file, file_A, file_B, file_C, file_D, file_E, file_F, file_G, file_H, no_file,
    no_file, file_A, file_B, file_C, file_D, file_E, file_F, file_G, file_H, no_file,
    no_file, file_A, file_B, file_C, file_D, file_E, file_F, file_G, file_H, no_file,
    no_file, file_A, file_B, file_C, file_D, file_E, file_F, file_G, file_H, no_file,
    no_file, file_A, file_B, file_C, file_D, file_E, file_F, file_G, file_H, no_file,
    no_file, file_A, file_B, file_C, file_D, file_E, file_F, file_G, file_H, no_file,
    no_file, file_A, file_B, file_C, file_D, file_E, file_F, file_G, file_H, no_file,
    no_file, file_A, file_B, file_C, file_D, file_E, file_F, file_G, file_H, no_file,
    no_file, no_file, no_file, no_file, no_file, no_file, no_file, no_file, no_file, no_file,
    no_file, no_file, no_file, no_file, no_file, no_file, no_file, no_file, no_file, no_file
};

const int sq120FromSq64[64] =
{
    A1, B1, C1, D1, E1, F1, G1, H1,
    A2, B2, C2, D2, E2, F2, G2, H2,
    A3, B3, C3, D3, E3, F3, G3, H3,
    A4, B4, C4, D4, E4, F4, G4, H4,
    A5, B5, C5, D5, E5, F5, G5, H5,
    A6, B6, C6, D6, E6, F6, G6, H6,
    A7, B7, C7, D7, E7, F7, G7, H7,
    A8, B8, C8, D8, E8, F8, G8, H8
};

const int sq64FromSq120[sq_no] =
{
    NO_SQ, NO_SQ, NO_SQ, NO_SQ, NO_SQ, NO_SQ, NO_SQ, NO_SQ, NO_SQ, NO_SQ,
    NO_SQ, NO_SQ, NO_SQ, NO_SQ, NO_SQ, NO_SQ, NO_SQ, NO_SQ, NO_SQ, NO_SQ,
    NO_SQ, 0, 1, 2, 3, 4, 5, 6, 7, NO_SQ,
    NO_SQ, 8, 9, 10, 11, 12, 13, 14, 15, NO_SQ,
    NO_SQ, 16, 17, 18, 19, 20, 21, 22, 23, NO_SQ,
    NO_SQ, 24, 25, 26, 27, 28, 29, 30, 31, NO_SQ,
    NO_SQ, 32, 33, 34, 35, 36, 37, 38, 39, NO_SQ,
    NO_SQ, 40, 41, 42, 43, 44, 45, 46, 47, NO_SQ,
    NO_SQ, 48, 49, 50, 51, 52, 53, 54, 55, NO_SQ,
    NO_SQ, 56, 57, 58, 59, 60, 61, 62, 63, NO_SQ,
    NO_SQ, NO_SQ, NO_SQ, NO_SQ, NO_SQ, NO_SQ, NO_SQ, NO_SQ, NO_SQ, NO_SQ,
    NO_SQ, NO_SQ, NO_SQ, NO_SQ, NO_SQ, NO_SQ, NO_SQ, NO_SQ, NO_SQ, NO_SQ
};

const int Mirror64[64] =
{
    56	,	57	,	58	,	59	,	60	,	61	,	62	,	63	,
    48	,	49	,	50	,	51	,	52	,	53	,	54	,	55	,
    40	,	41	,	42	,	43	,	44	,	45	,	46	,	47	,
    32	,	33	,	34	,	35	,	36	,	37	,	38	,	39	,
    24	,	25	,	26	,	27	,	28	,	29	,	30	,	31	,
    16	,	17	,	18	,	19	,	20	,	21	,	22	,	23	,
    8	,	9	,	10	,	11	,	12	,	13	,	14	,	15	,
    0	,	1	,	2	,	3	,	4	,	5	,	6	,	7
};

#endif // DATA_H

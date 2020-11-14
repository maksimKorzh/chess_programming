#ifndef DEFINITIONS_H
#define DEFINITIONS_H

#include "stdlib.h"
#include "stdio.h"

//#define DEBUG

#ifndef DEBUG
#define ASSERT(n)
#else
#define ASSERT(n) \
if ( ! (n) ) { \
printf ("%s - Failed", #n); \
printf ("On %s ", __DATE__); \
printf ("At %s ", __TIME__); \
printf ("In %s ", __FILE__); \
printf ("At %s ", __LINE__); \
exit(1);}
#endif

#define Start_FEN "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"

#define NAME "Aiki Chess 2.0"
#define Board_Square_Number 120

#define MaxGameMoves 2048
#define MaxPositionMoves 256
#define MaxDepth 64


enum {EmptySquare, wP, wN, wB, wR, wQ, wK, bP, bN, bB, bR, bQ, bK};
enum { FILE_A, FILE_B, FILE_C, FILE_D, FILE_E, FILE_F, FILE_G, FILE_H, FILE_NONE };
enum { RANK_1, RANK_2, RANK_3, RANK_4, RANK_5, RANK_6, RANK_7, RANK_8, RANK_NONE };

enum {White, Black, Both};

enum {
	
	A1, B1, C1, D1, E1, F1, G1, H1,
	A2, B2, C2, D2, E2, F2, G2, H2,
	A3, B3, C3, D3, E3, F3, G3, H3,
	A4, B4, C4, D4, E4, F4, G4, H4,
	A5, B5, C5, D5, E5, F5, G5, H5,
	A6, B6, C6, D6, E6, F6, G6, H6,
	A7, B7, C7, D7, E7, F7, G7, H7,
	A8, B8, C8, D8, E8, F8, G8, H8, No_Sq
	
};

enum {False, True};

enum {WhiteKingSideCastling = 1, WhiteQueenSideCastling = 2, BlackKingSideCastling = 4, BlackQueenSideCastling = 8};
enum {movegen, eval};

typedef unsigned long long U64;

typedef struct {
	
	int piece;
	int move;
	int score;
	
} Move_Structure; 

typedef struct {
	
	Move_Structure moves [MaxPositionMoves];
	int count;
	
} MoveList_Structure;

typedef struct {
	
	int move;
	int castlingPermission;
	int EnPassant_Square;
	int FiftyMoveCount;
	U64 hashKey;
	
} Undo_Moves_Structure;

typedef struct {
	
	U64 pieces [13];
	U64 whitePieces;
	U64 blackPieces;
	U64 occupancy;
	
	U64 hashKey;
	
	int PieceNumber [13];
	int BigPieces [2];
	int MajorPieces [2];
	int MinorPieces [2];
	int Material [2];
	
	int BoardPosition [64];
	
	int SideToMove;
	int EnPassant_Square;
	int FiftyMoveCount;

        int ply;
        int histPly;
	
	int castlingPermission;

       	Undo_Moves_Structure history [MaxGameMoves];
	
}CHESS_BOARD;

// Game moves
#define MOVE(from, to, capture, promotion, flag)   ( (from) | ( (to) << 7) | ( (capture) << 14) | ( (promotion) << 20) | (flag) ) 

#define FROMSQ(move)    ( (move) & 0x7F )
#define TOSQ(move)    ( ( (move) >> 7 ) & 0x7F )	
#define CAPTUREDPIECE(move)    ( ( (move) >> 14 ) & 0xF )	
#define PROMOTEDPIECE(move)    ( ( (move) >> 20 ) & 0xF )

#define MOVEFLAGENPASSANT 0x40000   
#define MOVEFLAGPAWNSTART 0x80000 
#define MOVEFLAGCASTLING 0x1000000

#define MOVEFLAGCAPTURE 0x7C000 
#define MOVEFLAGPROMOTION 0xF00000 

#define NOMOVE 0	


// Macros

#define Files_Ranks_To_Squares(f, r)   (f + r*8)
#define CLEARBIT(U64,square) ((U64) &= ClearMask[(square)])
#define SETBIT(U64,square) ((U64) |= SetMask[(square)])
#define GETBIT(U64, square)  ((U64) & SetMask [(square)])
#define AND(bitboard1, bitboard2)    (bitboard1 & bitboard2)
#define OR(bitboard1, bitboard2)    (bitboard1 | bitboard2)
#define XOR(bitboard1, bitboard2)    (bitboard1 ^ bitboard2)


#define IsBishopQueen(p)    (PieceBishopQueen [ (p) ]) 	
#define IsRookQueen(p)    (PieceRookQueen [ (p) ]) 	
#define IsKnight(p)    (PieceKnight [ (p) ]) 	
#define IsKing(p)    (PieceKing [ (p) ]) 		


// Globals

extern int BBits [64];
extern int RBits [64];

extern U64 SetMask[64];
extern U64 ClearMask[64];

extern U64 rookMagics [64];
extern U64 bishopMagics [64];

extern U64 whitePawn_Attacks [64];
extern U64 blackPawn_Attacks [64];
extern U64 pawn_attacks[2][64];
extern U64 Knight_Attacks [64];
extern U64 Bishop_Attacks [64];
extern U64 Rook_Attacks [64];
extern U64 Queen_Attacks [64];
extern U64 King_Attacks [64];

extern U64 bishopMask [64];
extern U64 rookMask [64];
extern U64 bishopShifts [64];
extern U64 rookShifts [64];
extern U64 bishopMagics [64];
extern U64 rookMagics [64];
extern U64 bishopOccupancy [64][512];
extern U64 rookOccupancy [64][4096];
extern U64 Bishop_Table [64][512];
extern U64 Rook_Table [64][4096];

extern int PieceBig [13];
extern int PieceMajor [13];
extern int PieceMinor [13];
extern int PieceValue [13];
extern int PieceColour [13];
extern int PiecePawn[13];

extern int FilesBoard [Board_Square_Number];
extern int RanksBoard [Board_Square_Number];

extern int PieceKnight [13];
extern int PieceKing [13];
extern int PieceRookQueen [13];
extern int PieceBishopQueen [13];
extern int PieceSlides [13];

extern U64 PieceKeys[13][64];
extern U64 SideKey;
extern U64 CastleKeys[16];

extern char PceChar[];
extern char SideChar[];
extern char RankChar[];
extern char FileChar[];

extern U64 whiteAttackedSquares;
extern U64 blackAttackedSquares;

extern U64 notHFile;
extern U64 notGHFile;
extern U64 notAFile;
extern U64 notABFile;

extern int timeTaken;

// Functions

// Bitboards
extern int bitScanForward (U64 bb);
extern int count_1s (U64 bit);
//extern int scan_bitboard (U64 *bitboard);
extern U64 setOccupancy (int index, int bits, U64 m);
extern void Reset_Board (CHESS_BOARD *chessBoard);
extern void Print_Bitboard (CHESS_BOARD *chessBoard, U64 bitboard);
extern void Print_All_Bitboards (CHESS_BOARD *chessBoard);
extern void Update_Lists_of_Material (CHESS_BOARD *chessBoard);
extern int Parse_FEN (char *FEN, CHESS_BOARD *chessBoard);
extern void Print_Board (CHESS_BOARD *chessBoard);
//extern void Setup_Pieces_and_Occupancy (CHESS_BOARD *chessBoard);
extern void Setup_Board_Position (CHESS_BOARD *chessBoard);

// Attack.c

extern U64 bmask (int sq);
extern U64 rmask (int sq);
extern U64 batt (int sq, U64 block);
extern U64 ratt (int sq, U64 block);
extern U64 pawnMask (int sq, int side);
extern U64 knightMask (int sq);
extern U64 kingMask (int sq);
extern U64 pawnAttackBitboard (int sq,  U64 opponentBlockers, int side);
extern U64 knightAttackBitboard (int sq, U64 ownBlockers);
extern U64 bishopAttackBitboard (int sq, U64 opponentBlockers, U64 ownBlockers);
extern U64 rookAttackBitboard (int sq, U64 opponentBlockers, U64 ownBlockers);
extern U64 queenAttackBitboard (int sq, U64 opponentBlockers, U64 ownBlockers);
extern U64 kingAttackBitboard (int sq, U64 ownBlockers);


extern U64 bishopMagicAttacks(U64 occ, int sq);
extern U64 rookMagicAttacks(U64 occ, int sq);
extern U64 queenMagicAttacks(U64 occ, int sq);

//extern U64 GenerateAllAttacks (CHESS_BOARD *chessBoard, int side);
extern int Is_Square_Attacked (CHESS_BOARD *chessBoard, int sq, int side);
//extern int Is_sq_att (CHESS_BOARD *chessBoard, int sq, int side);

// Makemove.c
extern int MakeMove (CHESS_BOARD *chessBoard, int move);
extern void TakeMove (CHESS_BOARD *chessBoard);

// InputOutput.c
extern char *Print_square (const int sq);
extern char *Print_move (const int move);
//extern void Print_MoveList (const MoveList_Structure *moveList);

// MoveGenerator.c
extern void GenerateAllMoves (CHESS_BOARD *chessBoard, MoveList_Structure *moveList);

// Init.c
extern void All_Init ();

// HashKeys.c
extern U64 Generate_hashKey(const CHESS_BOARD *chessBoard);

// Misc.c
extern int GetTimeInMilliseconds ();

// Perft.c
extern void PerftTest (int depth, CHESS_BOARD *chessBoard);
extern void Perft (int depth, CHESS_BOARD *chessBoard);

// Magics


extern int pop_1st_bit (U64 *U64);
extern void Init_Magics ();

extern int Print_Magics();








#endif

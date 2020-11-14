#include "stdio.h"
#include "Definitions.h"
#include "stdlib.h"

#define RAND_64 	((U64)rand() | \
					(U64)rand() << 15 | \
					(U64)rand() << 30 | \
					(U64)rand() << 45 | \
					((U64)rand() & 0xf) << 60 )
					


U64 SetMask[64];
U64 ClearMask[64];

int FilesBoard [Board_Square_Number];
int RanksBoard [Board_Square_Number];

U64 whitePawn_Attacks [64];
U64 blackPawn_Attacks [64];
U64 Knight_Attacks [64];
U64 Bishop_Attacks [64];
U64 Rook_Attacks [64];
U64 Queen_Attacks [64];
U64 King_Attacks [64];

U64 bishopMask [64];
U64 rookMask [64];
U64 bishopShifts [64];
U64 rookShifts [64];
U64 bishopMagics [64];
U64 rookMagics [64];
U64 bishopOccupancy [64][512];
U64 rookOccupancy [64][4096];
U64 Bishop_Table [64][512];
U64 Rook_Table [64][4096];
U64 PieceKeys[13][64];
U64 SideKey;
U64 CastleKeys[16];

void Init_BitMasks () {
	
	int index;
	
	for (index = 0; index < 64; index++) {
		SetMask [index] = 0ULL;
		ClearMask [index] = 0ULL;	
	}
	
	for (index = 0; index < 64; index++) {
		SetMask[index] |= (1ULL << index);
		ClearMask[index] = ~SetMask[index];
	}
	
}

void Init_Files_and_Ranks_Board () {
	
	int index = 0;
	int file = FILE_A;
	int rank = RANK_1;
	int sq = A1;
	int sq64 = 0;
	
	for (index = 0; index < 64; index++) {
		FilesBoard [index] = 0;
		RanksBoard [index] = 0;
	}
	
	for (rank = RANK_1; rank <= RANK_8; rank++) {
		for (file = FILE_A; file <= FILE_H; file++) {
			sq = Files_Ranks_To_Squares (file, rank);
			FilesBoard [sq] = file;
		        RanksBoard [sq] = rank;
		}
	}
}

U64 pawn_attacks[2][64];

void Init_NonSliders_Attacks () {
	
	int sq;
	
	for (sq = 0; sq < 64; sq++) {
		whitePawn_Attacks [sq] = pawnMask (sq, White);
		blackPawn_Attacks [sq] = pawnMask (sq, Black);
		pawn_attacks[White][sq] = pawnMask(sq, White);
		pawn_attacks[Black][sq] = pawnMask(sq, Black);
		Knight_Attacks [sq] = knightMask (sq);
		King_Attacks [sq] = kingMask (sq);
	}
	
}

void Init_Sliders_Masks_Shifts_Occupancies (int isRook) {
	
	int i, bitRef, bitCount, variationCount;
        U64 mask;
	
	for (bitRef=0; bitRef < 64; bitRef++) {
		rookMask [bitRef] = rmask (bitRef);
		bishopMask [bitRef] = bmask (bitRef);
		rookShifts [bitRef] = 64 - RBits [bitRef];
		bishopShifts [bitRef] = 64 - BBits [bitRef];
		mask = isRook ? rookMask [bitRef] : bishopMask [bitRef];
		bitCount = count_1s (mask);
		variationCount = 1 << bitCount;
		for(i = 0; i < variationCount; i++) {
			if (isRook) {
				rookOccupancy [bitRef][i] = 0ULL;
				rookOccupancy [bitRef][i] = setOccupancy (i, bitCount, mask);
			}
			else {
				bishopOccupancy [bitRef][i] = 0ULL;
				bishopOccupancy [bitRef][i] = setOccupancy (i, bitCount, mask);
			}		
		}
	}
	
}	

void Init_Sliders_moveDatabase (int isRook) {
	
	U64 validMoves, mask;
        int variations, bitCount;
        int bitRef, i, magicIndex;
    
        for (bitRef=0; bitRef<=63; bitRef++) {
		mask = isRook ? rookMask [bitRef] : bishopMask [bitRef];
		bitCount = count_1s (mask);
		variations = 1ULL << bitCount;
            
		for (i=0; i<variations; i++) {
			
			if (isRook) {
				validMoves = ratt (bitRef, rookOccupancy [bitRef][i]);
				magicIndex = rookOccupancy [bitRef][i] * rookMagics [bitRef] >> rookShifts [bitRef];
				Rook_Table [bitRef][magicIndex] = validMoves;
			}
			else {
				validMoves = batt (bitRef, bishopOccupancy [bitRef][i]);
				magicIndex = bishopOccupancy [bitRef][i] * bishopMagics [bitRef] >> bishopShifts [bitRef];
				Bishop_Table [bitRef][magicIndex] = validMoves;
			}
		}	
	}	
	
}
 
void InitHashKeys() {

	int index = 0;
	int index2 = 0;
	
	for (index = 0; index < 13; index++) {
		for (index2 = 0; index2 < 64; index2++) {
			PieceKeys[index][index2] = RAND_64;
		}
	}
	
	SideKey = RAND_64;
	
	for(index = 0; index < 16; index++) {
		CastleKeys [index] = RAND_64;
	}

}

void All_Init () {
	
	Init_BitMasks ();
	Init_Files_and_Ranks_Board ();
	Init_NonSliders_Attacks ();
	Init_Sliders_Masks_Shifts_Occupancies (0);
	Init_Sliders_Masks_Shifts_Occupancies (1);
	Init_Sliders_moveDatabase (0);
	Init_Sliders_moveDatabase (1);
	//Init_Magics ();
	InitHashKeys();
	
}

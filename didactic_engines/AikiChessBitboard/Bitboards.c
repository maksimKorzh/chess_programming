#include "stdio.h"
#include "Definitions.h"
#include "math.h"

int RBits[64] = {
	
	12, 11, 11, 11, 11, 11, 11, 12,
	11, 10, 10, 10, 10, 10, 10, 11,
	11, 10, 10, 10, 10, 10, 10, 11,
	11, 10, 10, 10, 10, 10, 10, 11,
	11, 10, 10, 10, 10, 10, 10, 11,
	11, 10, 10, 10, 10, 10, 10, 11,
	11, 10, 10, 10, 10, 10, 10, 11,
	12, 11, 11, 11, 11, 11, 11, 12
	
};
 
int BBits[64] = {
	
	6, 5, 5, 5, 5, 5, 5, 6,
	5, 5, 5, 5, 5, 5, 5, 5,
	5, 5, 7, 7, 7, 7, 5, 5,
	5, 5, 7, 9, 9, 7, 5, 5,
	5, 5, 7, 9, 9, 7, 5, 5,
	5, 5, 7, 7, 7, 7, 5, 5,
	5, 5, 5, 5, 5, 5, 5, 5,
	6, 5, 5, 5, 5, 5, 5, 6
	
};

const int BitTable[64] = {
	
	63, 30, 3, 32, 25, 41, 22, 33, 15, 50, 42, 13, 11, 53, 19, 34, 61, 29, 2,
	51, 21, 43, 45, 10, 18, 47, 1, 54, 9, 57, 0, 35, 62, 31, 40, 4, 49, 5, 52,
	26, 60, 6, 23, 44, 46, 27, 56, 16, 7, 39, 48, 24, 59, 14, 12, 55, 38, 28,
	58, 20, 37, 17, 36, 8
	
};

const int lsb_64_table[64] =
{
   63, 30,  3, 32, 59, 14, 11, 33,
   60, 24, 50,  9, 55, 19, 21, 34,
   61, 29,  2, 53, 51, 23, 41, 18,
   56, 28,  1, 43, 46, 27,  0, 35,
   62, 31, 58,  4,  5, 49, 54,  6,
   15, 52, 12, 40,  7, 42, 45, 16,
   25, 57, 48, 13, 10, 39,  8, 44,
   20, 47, 38, 22, 17, 37, 36, 26
};

int bitScanForward (U64 bb) {
	
	unsigned int folded;
	//assert (bb != 0);
	bb ^= bb - 1;
	folded = (int) bb ^ (bb >> 32);
	return lsb_64_table[folded * 0x78291ACF >> 26];
	
}



int count_1s (U64 bit) {
	
	//int start = GetTimeInMilliseconds ();
		
	
	int r;
	for (r = 0; bit; r++, bit &= bit - 1);
	
	//int finish = GetTimeInMilliseconds () - start;
	//timeTaken += finish;
	
	return r;
	
}

int scan_bitboard (U64 *bitboard) {
	
	//int start = GetTimeInMilliseconds ();
	
	U64 bit = *bitboard ^ (*bitboard - 1);
	unsigned int fold = (unsigned) ((bit & 0xffffffff) ^ (bit >> 32));
	
	*bitboard &= (*bitboard - 1);
	
	//int finish = GetTimeInMilliseconds () - start;
	//timeTaken += finish;
	
	return BitTable [(fold * 0x783a9b23) >> 26];
	
}

U64 setOccupancy (int index, int bits, U64 m) {
	
	int i, j;
	U64 result = 0ULL;
	
	for (i = 0; i < bits; i++) {
		j = scan_bitboard(&m);
		if (index & (1 << i)) 
			result |= (1ULL << j);
	}
	
	return result;
	  
}

void Reset_Board (CHESS_BOARD *chessBoard) {
	
	int index;
	
	for (index = 0; index <= 12; index++) {
		chessBoard->pieces [index] = 0ULL;
	}
	
	chessBoard->whitePieces = 0ULL;
	chessBoard->blackPieces = 0ULL;
	chessBoard->occupancy = 0ULL;
	
	for (index = 0; index < 2; index++) {
		chessBoard->BigPieces [index] = 0;
		chessBoard->MajorPieces [index] = 0;
		chessBoard->MinorPieces [index] = 0;
		chessBoard->Material [index] = 0;
	}
	
	for (index = 0; index < 13; index++) {
		chessBoard->PieceNumber [index] = 0;
	}
	
	chessBoard->SideToMove = Both;
	chessBoard->EnPassant_Square = No_Sq;
	chessBoard->FiftyMoveCount = 0;

	chessBoard->ply = 0;
	chessBoard->histPly = 0;

	chessBoard->castlingPermission = 0;

	chessBoard->hashKey = 0ULL;
	
	for (index = 0; index < 64; index++) {
		chessBoard->BoardPosition [index] = 0;
	}
	
}

void Print_Bitboard (CHESS_BOARD *chessBoard, U64 bitboard) {
	
	int rank;
	int file;
	int sq;
	
	printf("\n\n");
	
	
	for(rank = RANK_8; rank >= RANK_1; rank--) {
		for(file = FILE_A; file <= FILE_H; file++) {
			
			sq = Files_Ranks_To_Squares (file, rank);
			
			if ((1ULL << sq) & bitboard)
				printf ("1");
			else
				printf ("0");	
				
		}
		printf("\n");
	}  

}

void Print_All_Bitboards (CHESS_BOARD *chessBoard) {
	
	int index;
	
	for (index = 1; index <= 12; index++) {
		Print_Bitboard (chessBoard, chessBoard->pieces [index]);
	}
	
}

void Setup_Pieces_and_Occupancy (CHESS_BOARD *chessBoard) {
	
	int index;
	
	for (index = 1; index <= 6; index++) {
		chessBoard->whitePieces |= chessBoard->pieces [index];
	}
	
	for (index = 7; index <= 12; index++) {
		chessBoard->blackPieces |= chessBoard->pieces [index];
	}
	
	chessBoard->occupancy |= chessBoard->whitePieces;
	chessBoard->occupancy |= chessBoard->blackPieces;
	
}

void Setup_Board_Position (CHESS_BOARD *chessBoard) {
	
	int pieceIndex;
	int sqIndex;
	
	for (pieceIndex = 1; pieceIndex <=12; pieceIndex++) {
		
		int count = count_1s (chessBoard->pieces [pieceIndex]);
		
		for (sqIndex = 0; sqIndex < count; sqIndex++) {
			U64 temp_bitboard = chessBoard->pieces [pieceIndex];
			for (sqIndex = 0; sqIndex < count; sqIndex++)
				chessBoard->BoardPosition [scan_bitboard (&temp_bitboard)] = pieceIndex;
			
		}
		
	}
}

void Update_Lists_of_Material (CHESS_BOARD *chessBoard) {
	
	int piece, sq, index, pieceIndex, colour;
	
	for (index = 0; index < 64; index++) {
		sq = index;
		for (pieceIndex = 0; pieceIndex < 13; pieceIndex++) {
			if (GETBIT (chessBoard->pieces [pieceIndex], sq)) 
				piece = pieceIndex;
		}
			
		colour = PieceColour [piece];
		if (PieceBig [piece] == True)  
			chessBoard->BigPieces [colour]++;
		if (PieceMajor [piece] == True)  
			chessBoard->MajorPieces [colour]++;
		if (PieceMinor [piece] == True)  
			chessBoard->MinorPieces [colour]++;
			
		chessBoard->Material [colour] += PieceValue [piece];
		chessBoard->PieceNumber [piece]++;
	}

}

int Parse_FEN (char *FEN, CHESS_BOARD *chessBoard) {
	
	ASSERT (FEN != NULL);
	ASSERT (chessBoard != NULL);
	
	int index;
	int rank = RANK_8;
	int file = FILE_A;
	int piece = 0;
	int count = 0;
	int i = 0;
	int sq = 0;
	
	Reset_Board (chessBoard);
	
	while ( (rank >= RANK_1) && *FEN) {
		count = 1;
		
		switch (*FEN) {
			case 'p': piece = bP; break;
                        case 'r': piece = bR; break;
                        case 'n': piece = bN; break;
                        case 'b': piece = bB; break;
                        case 'k': piece = bK; break;
                        case 'q': piece = bQ; break;
                        case 'P': piece = wP; break;
                        case 'R': piece = wR; break;
                        case 'N': piece = wN; break;
			case 'B': piece = wB; break;
                        case 'K': piece = wK; break;
                        case 'Q': piece = wQ; break;

			case '1':
                        case '2':
                        case '3':
                        case '4':
                        case '5':
                        case '6':
                        case '7':
			case '8':
				piece = EmptySquare;
                                count = *FEN - '0';
                                break;

                         case '/':
                         case ' ':
                                rank--;
		        	file = FILE_A;
                                FEN++;
                                continue;

                        default:
				printf("FEN error \n");
                                return -1;
                }
	
		for (i = 0; i < count; i++) {
			sq = file + rank * 8;
			if (piece != EmptySquare) {
				SETBIT (chessBoard->pieces [piece], sq);
			}
			
			file++;
		}
		FEN++;
	}
	
	ASSERT (*FEN == 'w' || *FEN == 'b');
	
	chessBoard->SideToMove = (*FEN == 'w') ? White : Black;
	FEN += 2;
	
	for (i = 0; i < 4; i++) {
		if (*FEN == ' ') {
		  	break;
		}
		
		switch (*FEN) {
			case 'K' : chessBoard->castlingPermission |= WhiteKingSideCastling; break;
			case 'Q' : chessBoard->castlingPermission |= WhiteQueenSideCastling;	break;
			case 'k' : chessBoard->castlingPermission |= BlackKingSideCastling; break;
			case 'q' : chessBoard->castlingPermission |= BlackQueenSideCastling; break;
			default: break;
		}
		
		FEN++;
	}
	FEN++;

	ASSERT (chessBoard->castlingPermission >= 0 && chessBoard->castlingPermission <= 15);
	
	if (*FEN != '-') {
		file = FEN [0] - 'a';
		rank = FEN [1] - '1';
		
		ASSERT (file >= FILE_A && file <= FILE_H);
		ASSERT (rank >= RANK_1 && file <= RANK_8);
		
		chessBoard->EnPassant_Square = Files_Ranks_To_Squares (file, rank);
	}
		
	Setup_Pieces_and_Occupancy (chessBoard);
	Setup_Board_Position (chessBoard);	
	chessBoard->hashKey = Generate_hashKey (chessBoard);
	//Update_Lists_of_Material (chessBoard);
	
	return 0;
}

void Print_Board (CHESS_BOARD *chessBoard) {
	
	int sq, file, rank, piece;

	printf("\nChess Board:\n\n");

	for (rank = RANK_8; rank >= RANK_1; rank--) {
		printf ("%d  ", rank + 1);
		for (file = FILE_A; file <= FILE_H; file++) {
			sq = Files_Ranks_To_Squares (file, rank);
			
			
			piece = chessBoard->BoardPosition [sq] ;
			
			printf ("%3c", PceChar [piece]);
		}
		
		printf ("\n");
	}
	printf("\n   ");
	for(file = FILE_A; file <= FILE_H; file++) {
		printf("%3c",'a'+file);
	}
	
	printf ("\n");
	printf ("\nside:%c\n", SideChar [chessBoard->SideToMove]);
	printf ("EnPasssant:%d\n", chessBoard->EnPassant_Square);
	printf ("Castling:%c%c%c%c\n",
			chessBoard->castlingPermission & WhiteKingSideCastling ? 'K' : '-',
			chessBoard->castlingPermission & WhiteQueenSideCastling ? 'Q' : '-',
			chessBoard->castlingPermission & BlackKingSideCastling ? 'k' : '-',
			chessBoard->castlingPermission & BlackQueenSideCastling ? 'q' : '-'
			);
	printf("HashKey:%llX\n\n",chessBoard->hashKey);

}

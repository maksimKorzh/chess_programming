#include "stdio.h"
#include "Definitions.h"

U64 Generate_hashKey(const CHESS_BOARD *chessBoard) {

	int sq = 0;
	U64 finalKey = 0;
	int piece = EmptySquare;
	int index;
	int piece1 = EmptySquare;
	
	// pieces
	for(sq = 0; sq < 64; sq++) {
		
		for (index = 1; index <=12; index++) {
				if ((1ULL << sq) & chessBoard->pieces [index]) {
					piece = index;
					finalKey ^= PieceKeys [piece][sq];
				}
			}
			
	}

	if(chessBoard->SideToMove == White) {
		finalKey ^= SideKey;
	}
		
	if(chessBoard->EnPassant_Square != No_Sq) {
		ASSERT(chessBoard->EnPassant_Square >=0 && chessBoard->EnPassant_Square < 64);
		//ASSERT(SqOnBoard(pos->enPas));
		//ASSERT(RanksBrd[pos->enPas] == RANK_3 || RanksBrd[pos->enPas] == RANK_6);
		finalKey ^= PieceKeys [EmptySquare][chessBoard->EnPassant_Square];
	}
	
	//ASSERT(pos->castlePerm>=0 && pos->castlePerm<=15);
	
	finalKey ^= CastleKeys [chessBoard->castlingPermission];
	
	return finalKey;
}

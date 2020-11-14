#include "stdio.h"
#include "Definitions.h"

#define FEN1 "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1" 

//#define FEN1 "8/8/8/8/8/8/1p4p1/B1R2B1R b - e3 0 1  "      r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1   // tricky position
// bug r4r1R/p1ppqpbk/bn2pnp1/3PN3/1p2P3/2N4p/PPPBBPPP/R3K2R b KQ - 3 1  
// rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1  


int main () {
	
	All_Init ();
	
	CHESS_BOARD chessBoard [1];
	
	Parse_FEN (Start_FEN, chessBoard);
	
	Print_Board (chessBoard);

	
	
	PerftTest (6, chessBoard);

	//printf ("\ntime gen: %d\n", timeTaken);
	
	
	
	
	return 0;	
}

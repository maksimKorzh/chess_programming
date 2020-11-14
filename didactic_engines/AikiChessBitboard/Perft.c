#include "Definitions.h"
#include "stdio.h"

long leafNodes;

void Perft (int depth, CHESS_BOARD *chessBoard) {
	
	if (depth == 0) {
		leafNodes++;
		return;
	}	
		
	MoveList_Structure moveList [1];
	GenerateAllMoves(chessBoard, moveList);
	//Print_Board (chessBoard);
	//Print_MoveList (moveList);
	
	int MoveNum = 0;
	
	//Print_Board (chessBoard);
	//getchar ();
	
	for(MoveNum = 0; MoveNum < moveList->count; ++MoveNum) {
		if ( !MakeMove(chessBoard, moveList->moves[MoveNum].move) ) {
			//printf ("\nIllegal Move!!!\n");
			continue;
		}	
		//printf ("\ndepth: %d\n", depth);
		//Print_Board (chessBoard);
		 
		Perft (depth - 1, chessBoard);
		TakeMove(chessBoard);
		
		//getchar ();
		
		//Print_Board (chessBoard);
		//getchar ();
	}
	
	return;
}

void PerftTest (int depth, CHESS_BOARD *chessBoard) {

	printf("\nStarting Test To Depth:%d\n",depth);	
	
	leafNodes = 0;
	int start = GetTimeInMilliseconds ();
	
        MoveList_Structure moveList[1];
	GenerateAllMoves(chessBoard, moveList);	
    
        int move;
	//int piece;	
        int MoveNumber = 0;
	
	for(MoveNumber = 0; MoveNumber < moveList->count; ++MoveNumber) {
		move = moveList->moves[MoveNumber].move;
			
		if (!MakeMove(chessBoard, move)) 
			continue;
     
                long cumulativenodes = leafNodes;
                Perft (depth - 1, chessBoard);
                TakeMove (chessBoard);    	
                long oldnodes = leafNodes - cumulativenodes;
                printf ("move %d : %s : oldnodes: %ld\n", MoveNumber+1, Print_move(move), oldnodes);
		
        }
	
	printf("\nTest Complete : %ld nodes visited in %d ms\n", leafNodes, GetTimeInMilliseconds () - start);
	

    return;
}
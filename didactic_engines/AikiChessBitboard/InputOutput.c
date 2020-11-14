#include "stdio.h"
#include "Definitions.h"

char *Print_square (const int sq) {
	
	static char SquareString [3];
	
	int file = FilesBoard [sq];
	int rank = RanksBoard [sq];
	
	sprintf (SquareString, "%c%c", ('a' + file), ('1' + rank));
	
	return SquareString;
	
}

char *Print_move (const int move) {
	
	static char MoveString [6];
	
	int filefrom = FilesBoard [FROMSQ (move)];
	int rankfrom = RanksBoard [FROMSQ (move)];
	int fileto = FilesBoard [TOSQ (move)];
	int rankto =RanksBoard [TOSQ (move)];
	
	int promoted = PROMOTEDPIECE (move);
	
	if (promoted) {
		char promotechar = 'q';
		if (IsKnight (promoted))
			promotechar = 'n';
		else if (IsRookQueen(promoted) && !IsBishopQueen (promoted))
			promotechar = 'r';
		else if (!IsRookQueen(promoted) && IsBishopQueen (promoted))
			promotechar = 'b';
		sprintf (MoveString, "%c%c%c%c%c", ('a' + filefrom), ('1' + rankfrom), ('a' + fileto), ('1' + rankto), promotechar);
	}
	else 
		sprintf (MoveString, "%c%c%c%c", ('a' + filefrom), ('1' + rankfrom), ('a' + fileto), ('1' + rankto));
	
	
	
	return MoveString;
}

/*int Parse_move (char *pointerChar, Board_Structure *position) {
	
	if (pointerChar [1] > '8' || pointerChar [1] < '1') return NOMOVE;
	if (pointerChar [3] > '8' || pointerChar [3] < '1') return NOMOVE;
	if (pointerChar [0] > 'h' || pointerChar [0] < 'a') return NOMOVE;
	if (pointerChar [2] > 'h' || pointerChar [2] < 'a') return NOMOVE;
	
	int fromSq = Files_Ranks_to_Squares (pointerChar [0] - 'a', pointerChar [1] - '1');
	int toSq = Files_Ranks_to_Squares (pointerChar [2] - 'a', pointerChar [3] - '1');
	
	ASSERT (IsSquareOnBoard (fromSq) && IsSquareOnBoard (toSq));
	
	MoveList_Structure moveList [1];
	GenerateAllMoves (position, moveList);
	
	int moveNumber = 0;
	int move = 0;
	int promotedPiece = EmptySquare;
	
	for (moveNumber = 0; moveNumber < moveList->count; moveNumber++) {
		move = moveList->moves [moveNumber].move;
		if (FROMSQ (move) == fromSq && TOSQ (move) == toSq) {
			promotedPiece = PROMOTEDPIECE (move);
			if (promotedPiece != EmptySquare) {
				if (IsRookQueen (promotedPiece) && !IsBishopQueen (promotedPiece) && pointerChar [4] == 'r')
					return move;
				else if (!IsRookQueen (promotedPiece) && IsBishopQueen (promotedPiece) && pointerChar [4] == 'b')
					return move;
				else if (IsRookQueen (promotedPiece) && IsBishopQueen (promotedPiece) && pointerChar [4] == 'q')
					return move;
				else if (IsKnight (promotedPiece) && pointerChar [4] == 'n')
					return move;
				continue;
			}
			return move;
		}
	}
	return NOMOVE;
	
}*/

void Print_MoveList (const MoveList_Structure *moveList) {
	
	int index = 0;
	int score = 0;
	int move = 0;
	
	printf ("\nMoveList:\n", moveList->count);
	
	for (index = 0; index < moveList->count; index++) {
		move = moveList->moves [index].move;
		score = moveList->moves [index].score;
		
		printf ("Move:%d > %s (score:%d)\n", index + 1, Print_move (move), score);
	}
	printf ("\nMoveList total %d moves:\n\n", moveList->count);
	
}

















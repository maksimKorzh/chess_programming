#include "Definitions.h"
#include "stdio.h"

#define HASH_PIECE(piece, sq)    (chessBoard->hashKey ^= (PieceKeys [(piece)][(sq)]))
#define HASH_CASTLING (chessBoard->hashKey ^= (CastleKeys [(chessBoard->castlingPermission)]))
#define HASH_SIDE (chessBoard->hashKey ^= (SideKey))
#define HASH_ENPASSANT (chessBoard->hashKey ^= (PieceKeys [EmptySquare][chessBoard->EnPassant_Square]))

const int CastlePermission[64] = {
	
	13, 15, 15, 15, 12, 15, 15, 14,
	15, 15, 15, 15, 15, 15, 15, 15,
	15, 15, 15, 15, 15, 15, 15, 15,
	15, 15, 15, 15, 15, 15, 15, 15,
	15, 15, 15, 15, 15, 15, 15, 15,
	15, 15, 15, 15, 15, 15, 15, 15,
	15, 15, 15, 15, 15, 15, 15, 15,
	7, 15, 15, 15, 3, 15, 15, 11
	
};

static void ClearPiece (const int sq, CHESS_BOARD *chessBoard) {
	
	//int index;
	int colour;
	int piece = chessBoard->BoardPosition [sq];
	
	colour = PieceColour [piece];
		
	chessBoard->BoardPosition [sq] = EmptySquare;
	CLEARBIT (chessBoard->pieces [piece], sq);  
			
	HASH_PIECE (piece, sq);
			
	chessBoard->Material [colour] -= PieceValue [piece];
	
	/*if (PieceBig [piece]) {
		chessBoard->BigPieces [colour]--;
		if (PieceMajor [piece]) {
			chessBoard->MajorPieces [colour]--;
		}
		else
			chessBoard->MinorPieces [colour]--;
	}*/
	
}

static void AddPiece (const int sq, CHESS_BOARD *chessBoard, const int piece) {
	
	int colour = PieceColour [piece];
	
	HASH_PIECE (piece, sq);
	
	chessBoard->BoardPosition [sq] = piece;
	SETBIT (chessBoard->pieces [piece], sq);
	
	/*if (PieceBig [piece]) {
		chessBoard->BigPieces [colour]++;
		if (PieceMajor [piece]) {
			chessBoard->MajorPieces [colour]++;
		}
		else
			chessBoard->MinorPieces [colour]++;
	}*/
	
	chessBoard->Material [colour] += PieceValue [piece];
	
}

static void MovePiece (const int fromSq, const int toSq, CHESS_BOARD *chessBoard) {
	
	//int index;
	int piece = chessBoard->BoardPosition [fromSq];
	
	HASH_PIECE (piece, fromSq);
	CLEARBIT (chessBoard->pieces [piece], fromSq); 
	chessBoard->BoardPosition [fromSq] = EmptySquare;
	
	HASH_PIECE (piece, toSq);
	SETBIT (chessBoard->pieces [piece], toSq);
	chessBoard->BoardPosition [toSq] = piece;	
	
}


int MakeMove (CHESS_BOARD *chessBoard, int move) {
	 
	int fromSq = FROMSQ (move);
	int toSq = TOSQ (move);
	int side = chessBoard->SideToMove;
	//int index;
	
	chessBoard->history [chessBoard->histPly].hashKey = chessBoard->hashKey;
	
	if (move & MOVEFLAGENPASSANT) {
	
		if (side == White) {
			ClearPiece (toSq - 8, chessBoard);
		}
		else {
			ClearPiece (toSq + 8, chessBoard);
		}	
	}
	
	else if (move & MOVEFLAGCASTLING) {
		switch (toSq) {
			case C1: 
				
				MovePiece (A1, D1, chessBoard);
				break;
			
			case C8:
				
				MovePiece (A8, D8, chessBoard);
				break;
			
			case G1:
				
				MovePiece (H1, F1, chessBoard);
				break;
			
			case G8:
				
				MovePiece (H8, F8, chessBoard);
				break;
		}
	}
	
	if (chessBoard->EnPassant_Square != No_Sq) HASH_ENPASSANT;
	HASH_CASTLING;
	
	chessBoard->history [chessBoard->histPly].move = move;
	chessBoard->history [chessBoard->histPly].FiftyMoveCount = chessBoard->FiftyMoveCount;
	chessBoard->history [chessBoard->histPly].EnPassant_Square = chessBoard->EnPassant_Square;
	chessBoard->history [chessBoard->histPly].castlingPermission = chessBoard->castlingPermission;
	chessBoard->castlingPermission &= CastlePermission [fromSq];
	chessBoard->castlingPermission &= CastlePermission [toSq];
		
	chessBoard->EnPassant_Square = No_Sq;
	
	HASH_CASTLING;
	
	int captured = CAPTUREDPIECE(move);

        chessBoard->FiftyMoveCount++;
	
	if (captured != EmptySquare) {
		ClearPiece (toSq, chessBoard);
		chessBoard->FiftyMoveCount = 0;
	}
	
	chessBoard->histPly++;
	chessBoard->ply++;
	
	if (PiecePawn [chessBoard->BoardPosition [fromSq]]) {
		chessBoard->FiftyMoveCount = 0;
		if (move & MOVEFLAGPAWNSTART) {
			if (side == White) {
				chessBoard->EnPassant_Square = fromSq + 8;
				ASSERT (RanksBoard [chessBoard->EnPassant_Square] == RANK_3);
			}
			else {
				chessBoard->EnPassant_Square = fromSq - 8;
				ASSERT (RanksBoard [chessBoard->EnPassant_Square] == RANK_6);
			}
			HASH_ENPASSANT;
		}
	}
	
	MovePiece (fromSq, toSq, chessBoard);
	
	int promotedPiece = PROMOTEDPIECE (move);
	if (promotedPiece != EmptySquare) {
	        ClearPiece (toSq, chessBoard);
		AddPiece (toSq, chessBoard, promotedPiece);
	}
	
	chessBoard->SideToMove ^= 1;
	HASH_SIDE;
	
	U64 temp_kingBitboard;
	
	if (side == White)
		temp_kingBitboard = chessBoard->pieces [wK];
	else
		temp_kingBitboard = chessBoard->pieces [bK];
	
	if (!captured && !(MOVEFLAGCASTLING & move) && !(MOVEFLAGENPASSANT & move)) {
		chessBoard->SideToMove ? CLEARBIT (chessBoard->whitePieces, fromSq) : CLEARBIT (chessBoard->blackPieces, fromSq);
		chessBoard->SideToMove ? SETBIT (chessBoard->whitePieces, toSq) : SETBIT (chessBoard->blackPieces, toSq);
		CLEARBIT (chessBoard->occupancy, fromSq);
		SETBIT (chessBoard->occupancy, toSq);
	}
	else if (captured != 0) {
		chessBoard->SideToMove ? CLEARBIT (chessBoard->whitePieces, fromSq) : CLEARBIT (chessBoard->blackPieces, fromSq);
		chessBoard->SideToMove ? SETBIT (chessBoard->whitePieces, toSq) : SETBIT (chessBoard->blackPieces, toSq);
		chessBoard->SideToMove ? CLEARBIT (chessBoard->blackPieces, toSq) : CLEARBIT (chessBoard->whitePieces, toSq);
		CLEARBIT (chessBoard->occupancy, fromSq);
	}
	else if (MOVEFLAGCASTLING & move) {
		chessBoard->SideToMove ? CLEARBIT (chessBoard->whitePieces, fromSq) : CLEARBIT (chessBoard->blackPieces, fromSq);
		chessBoard->SideToMove ? SETBIT (chessBoard->whitePieces, toSq) : SETBIT (chessBoard->blackPieces, toSq);
		CLEARBIT (chessBoard->occupancy, fromSq);
		SETBIT (chessBoard->occupancy, toSq);
		switch (toSq) {
			case C1:
				CLEARBIT (chessBoard->whitePieces, A1);
				SETBIT (chessBoard->whitePieces, D1);
				CLEARBIT (chessBoard->occupancy, A1);
				SETBIT (chessBoard->occupancy, D1);
				break;
			
			case G1:
				CLEARBIT (chessBoard->whitePieces, H1);
				SETBIT (chessBoard->whitePieces, F1);
				CLEARBIT (chessBoard->occupancy, H1);
				SETBIT (chessBoard->occupancy, F1);
				break;
			
			case C8:
				CLEARBIT (chessBoard->blackPieces, A8);
				SETBIT (chessBoard->blackPieces, D8);
				CLEARBIT (chessBoard->occupancy, A8);
				SETBIT (chessBoard->occupancy, D8);
				break;
			
			case G8:
				CLEARBIT (chessBoard->blackPieces, H8);
				SETBIT (chessBoard->blackPieces, F8);
				CLEARBIT (chessBoard->occupancy, H8);
				SETBIT (chessBoard->occupancy, F8);
				break;
			
		}
	}
	else if (MOVEFLAGENPASSANT & move) {
		chessBoard->SideToMove ? CLEARBIT (chessBoard->whitePieces, fromSq) : CLEARBIT (chessBoard->blackPieces, fromSq);
		chessBoard->SideToMove ? SETBIT (chessBoard->whitePieces, toSq) : SETBIT (chessBoard->blackPieces, toSq);
		CLEARBIT (chessBoard->occupancy, fromSq);
		SETBIT (chessBoard->occupancy, toSq);
		chessBoard->SideToMove ? CLEARBIT (chessBoard->blackPieces, toSq - 8) : CLEARBIT (chessBoard->whitePieces, toSq + 8);
		chessBoard->SideToMove ? CLEARBIT (chessBoard->occupancy, toSq - 8) : CLEARBIT (chessBoard->occupancy, toSq + 8);
	}
	
	if (Is_Square_Attacked (chessBoard, bitScanForward (temp_kingBitboard), chessBoard->SideToMove)) {
		TakeMove (chessBoard);
		return False;
	}
	
	return True;
}


void TakeMove (CHESS_BOARD *chessBoard) {
	
	chessBoard->histPly--;
	chessBoard->ply--;
	
	int move = chessBoard->history [chessBoard->histPly].move;
	int fromSq = FROMSQ (move);
	int toSq = TOSQ (move);
	
	if (chessBoard->EnPassant_Square != No_Sq) HASH_ENPASSANT;
	HASH_CASTLING;

        chessBoard->castlingPermission = chessBoard->history [chessBoard->histPly].castlingPermission;
        chessBoard->FiftyMoveCount = chessBoard->history [chessBoard->histPly].FiftyMoveCount;
	chessBoard->EnPassant_Square = chessBoard->history [chessBoard->histPly].EnPassant_Square;
	
	if (chessBoard->EnPassant_Square != No_Sq) HASH_ENPASSANT;
	HASH_CASTLING;
	
	chessBoard->SideToMove ^= 1;
	HASH_SIDE;
	
	if (MOVEFLAGENPASSANT & move) {
		if (chessBoard->SideToMove == White) {
			AddPiece (toSq - 8, chessBoard, bP);
		}	
		else {
			AddPiece (toSq + 8, chessBoard, wP);
		}	
	}
	        else if (MOVEFLAGCASTLING & move) {
			switch (toSq) {
				case C1: 
					
					MovePiece (D1, A1, chessBoard); 
					break;
				
				case C8: 
					
					MovePiece (D8, A8, chessBoard); 
					break;
				
				case G1: 
					
					MovePiece (F1, H1, chessBoard); 
					break;
				
				case G8: 
					MovePiece (F8, H8, chessBoard); 
					break;
			}
		 
	        }
		
	MovePiece (toSq, fromSq, chessBoard);
	
	int captured = CAPTUREDPIECE (move);
	
	if (captured != EmptySquare) {
		AddPiece (toSq, chessBoard, captured);
	}
	
	int promotedPiece = PROMOTEDPIECE (move);	
	
	if (promotedPiece != EmptySquare) {
		ClearPiece (fromSq, chessBoard);
		AddPiece (fromSq, chessBoard, (PieceColour [PROMOTEDPIECE (move)] == White ? wP : bP));
	}
	
	if (!captured && !(MOVEFLAGCASTLING & move) && !(MOVEFLAGENPASSANT & move)) {
		chessBoard->SideToMove ? CLEARBIT (chessBoard->blackPieces, toSq) : CLEARBIT (chessBoard->whitePieces, toSq);
		chessBoard->SideToMove ? SETBIT (chessBoard->blackPieces, fromSq) : SETBIT (chessBoard->whitePieces, fromSq);
		CLEARBIT (chessBoard->occupancy, toSq);
		SETBIT (chessBoard->occupancy, fromSq);
	}
	else if (captured != 0) {
		chessBoard->SideToMove ? CLEARBIT (chessBoard->blackPieces, toSq) : CLEARBIT (chessBoard->whitePieces, toSq);
		chessBoard->SideToMove ? SETBIT (chessBoard->blackPieces, fromSq) : SETBIT (chessBoard->whitePieces, fromSq);
		chessBoard->SideToMove ? SETBIT (chessBoard->whitePieces, toSq) : SETBIT (chessBoard->blackPieces, toSq);
		SETBIT (chessBoard->occupancy, fromSq);
	}
	else if (MOVEFLAGCASTLING & move) {
		chessBoard->SideToMove ? CLEARBIT (chessBoard->blackPieces, toSq) : CLEARBIT (chessBoard->whitePieces, toSq);
		chessBoard->SideToMove ? SETBIT (chessBoard->blackPieces, fromSq) : SETBIT (chessBoard->whitePieces, fromSq);
		CLEARBIT (chessBoard->occupancy, toSq);
		SETBIT (chessBoard->occupancy, fromSq);
		switch (toSq) {
			case C1:
				CLEARBIT (chessBoard->whitePieces, D1);
				SETBIT (chessBoard->whitePieces, A1);
				CLEARBIT (chessBoard->occupancy, D1);
				SETBIT (chessBoard->occupancy, A1);
				break;
			
			case G1:
				CLEARBIT (chessBoard->whitePieces, F1);
				SETBIT (chessBoard->whitePieces, H1);
				CLEARBIT (chessBoard->occupancy, F1);
				SETBIT (chessBoard->occupancy, H1);
				break;
			
			case C8:
				CLEARBIT (chessBoard->blackPieces, D8);
				SETBIT (chessBoard->blackPieces, A8);
				CLEARBIT (chessBoard->occupancy, D8);
				SETBIT (chessBoard->occupancy, A8);
				break;
			
			case G8:
				CLEARBIT (chessBoard->blackPieces, F8);
				SETBIT (chessBoard->blackPieces, H8);
				CLEARBIT (chessBoard->occupancy, F8);
				SETBIT (chessBoard->occupancy, H8);
				break;
			
		}
	}
	else if (MOVEFLAGENPASSANT & move) {
		chessBoard->SideToMove ? CLEARBIT (chessBoard->blackPieces, toSq) : CLEARBIT (chessBoard->whitePieces, toSq);
		chessBoard->SideToMove ? SETBIT (chessBoard->blackPieces, fromSq) : SETBIT (chessBoard->whitePieces, fromSq);
		CLEARBIT (chessBoard->occupancy, toSq);
		SETBIT (chessBoard->occupancy, fromSq);
		chessBoard->SideToMove ? SETBIT (chessBoard->whitePieces, toSq + 8) : SETBIT (chessBoard->blackPieces, toSq - 8);
		chessBoard->SideToMove ? SETBIT (chessBoard->occupancy, toSq + 8) : SETBIT (chessBoard->occupancy, toSq - 8);
	}
	
}












































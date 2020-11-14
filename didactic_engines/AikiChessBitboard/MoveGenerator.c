#include "stdio.h"
#include "Definitions.h"

static void AddQuietMove (const CHESS_BOARD *chessBoard, int move, MoveList_Structure *moveList) {
	
	//ASSERT (IsSquareOnBoard (FROMSQ (move)));
	//ASSERT (IsSquareOnBoard (TOSQ (move)));
	
	moveList->moves [moveList->count].move = move;
	
	//if (position->searchKillers [0] [position->ply] == move)
	//	moveList->moves [moveList->count].score = 900000;
	//else if (position->searchKillers [1] [position->ply] == move)
	//	moveList->moves [moveList->count].score = 800000;
	//else
	//	moveList->moves [moveList->count].score = position->searchHistory [position->pieces [FROMSQ (move)]] [TOSQ (move)];
	moveList->moves [moveList->count].score = 0;
	moveList->count++;
	
}

static void AddCaptureMove (const CHESS_BOARD *chessBoard, int move, MoveList_Structure *moveList) {
	
	//ASSERT (IsSquareOnBoard (FROMSQ (move)));
	//ASSERT (IsSquareOnBoard (TOSQ (move)));
	//ASSERT (IsPieceValid (CAPTUREDPIECE (move)));
	
	moveList->moves [moveList->count].move = move;
	//moveList->moves [moveList->count].score = MvvLvaScores [CAPTUREDPIECE (move)] [position->pieces [FROMSQ (move)]] + 1000000;
	moveList->moves [moveList->count].score = 0;
	moveList->count++;
	
}

static void AddEnPassantMove (const CHESS_BOARD *chessBoard, int move, MoveList_Structure *moveList) {
	
	//ASSERT (IsSquareOnBoard (FROMSQ (move)));
	//ASSERT (IsSquareOnBoard (TOSQ (move)));
	
	moveList->moves [moveList->count].move = move;
	//moveList->moves [moveList->count].score = 105 + 1000000;
	moveList->moves [moveList->count].score = 0;
	moveList->count++;
	
}

static void AddWhitePawnCaptureMove (const CHESS_BOARD *chessBoard, const int from, const int to, const int capture, MoveList_Structure *moveList ) {
	
	//ASSERT (IsPieceValidEmpty (capture));
	//ASSERT (IsSquareOnBoard (from));
	//ASSERT (IsSquareOnBoard (to));
	
	if (RanksBoard [from] == RANK_7) {
		AddCaptureMove (chessBoard, MOVE (from, to, capture, wQ, 0), moveList);
		AddCaptureMove (chessBoard, MOVE (from, to, capture, wR, 0), moveList);
		AddCaptureMove (chessBoard, MOVE (from, to, capture, wB, 0), moveList);
		AddCaptureMove (chessBoard, MOVE (from, to, capture, wN, 0), moveList);
	}
	else
		AddCaptureMove (chessBoard, MOVE (from, to, capture, EmptySquare, 0), moveList);
	
}

static void AddBlackPawnCaptureMove (const CHESS_BOARD *chessBoard, const int from, const int to, const int capture, MoveList_Structure *moveList ) {
	
	//ASSERT (IsPieceValidEmpty (capture));
	//ASSERT (IsSquareOnBoard (from));
	//ASSERT (IsSquareOnBoard (to));
	
	if (RanksBoard [from] == RANK_2) {
		AddCaptureMove (chessBoard, MOVE (from, to, capture, bQ, 0), moveList);
		AddCaptureMove (chessBoard, MOVE (from, to, capture, bR, 0), moveList);
		AddCaptureMove (chessBoard, MOVE (from, to, capture, bB, 0), moveList);
		AddCaptureMove (chessBoard, MOVE (from, to, capture, bN, 0), moveList);
	}
	else
		AddCaptureMove (chessBoard, MOVE (from, to, capture, EmptySquare, 0), moveList);
	
}

static void AddWhitePawnMove (const CHESS_BOARD *chessBoard, const int from, const int to, MoveList_Structure *moveList ) {
	
	//ASSERT (IsSquareOnBoard (from));
	//ASSERT (IsSquareOnBoard (to));
	
	if (RanksBoard [from] == RANK_7) {
		AddQuietMove (chessBoard, MOVE (from, to, EmptySquare, wQ, 0), moveList);
		AddQuietMove (chessBoard, MOVE (from, to, EmptySquare, wR, 0), moveList);
		AddQuietMove (chessBoard, MOVE (from, to, EmptySquare, wB, 0), moveList);
		AddQuietMove (chessBoard, MOVE (from, to, EmptySquare, wN, 0), moveList);
	}
	else
		AddQuietMove (chessBoard, MOVE (from, to, EmptySquare, EmptySquare, 0), moveList);
	
}

static void AddBlackPawnMove (const CHESS_BOARD *chessBoard, const int from, const int to, MoveList_Structure *moveList ) {
	
	//ASSERT (IsSquareOnBoard (from));
	//ASSERT (IsSquareOnBoard (to));
	
	if (RanksBoard [from] == RANK_2) {
		AddQuietMove (chessBoard, MOVE (from, to, EmptySquare, bQ, 0), moveList);
		AddQuietMove (chessBoard, MOVE (from, to, EmptySquare, bR, 0), moveList);
		AddQuietMove (chessBoard, MOVE (from, to, EmptySquare, bB, 0), moveList);
		AddQuietMove (chessBoard, MOVE (from, to, EmptySquare, bN, 0), moveList);
	}
	else
		AddQuietMove (chessBoard, MOVE (from, to, EmptySquare, EmptySquare, 0), moveList);
	
}


void GenerateAllMoves (CHESS_BOARD *chessBoard, MoveList_Structure *moveList) {
	
	//int start = GetTimeInMilliseconds ();
	
	moveList->count = 0;
	
	int side = chessBoard->SideToMove;
	int pieceIndex;
	int fromSq, toSq;
	
	if (side == White) {
	
		for (pieceIndex = 1; pieceIndex <=6; pieceIndex++) {
			
				U64 pieceBoard = chessBoard->pieces [pieceIndex];
				U64 attackBoard = 0ULL;
				
				switch (pieceIndex) {
					
					case wP:
						
						while (pieceBoard) {
							
							fromSq = bitScanForward (pieceBoard);
							attackBoard = AND (whitePawn_Attacks [fromSq], chessBoard->blackPieces);
						
							if (!(GETBIT (chessBoard->occupancy, fromSq + 8))) {
								AddWhitePawnMove (chessBoard, fromSq, fromSq + 8, moveList);
								if (RanksBoard [fromSq] == RANK_2 && !(GETBIT (chessBoard->occupancy, fromSq + 16))) {
									AddQuietMove (chessBoard, MOVE (fromSq, (fromSq + 16), EmptySquare, EmptySquare, MOVEFLAGPAWNSTART), moveList);
								}
							}
							
							if (chessBoard->EnPassant_Square != No_Sq) {
									
								U64 enPassant = 0Ull;
								SETBIT (enPassant,chessBoard->EnPassant_Square);
									
								if (fromSq + 7 == chessBoard->EnPassant_Square && (enPassant & 0x7f7f7f7f7f7f7f7f)) {
									AddEnPassantMove (chessBoard, MOVE (fromSq, fromSq + 7, EmptySquare, EmptySquare, MOVEFLAGENPASSANT), moveList);
								}
									
								if (fromSq + 9 == chessBoard->EnPassant_Square && (enPassant & 0xfefefefefefefefe)) {
									AddEnPassantMove (chessBoard, MOVE (fromSq, fromSq + 9, EmptySquare, EmptySquare, MOVEFLAGENPASSANT), moveList);
								}
								
							}
								
							
							while (attackBoard) {
								
								toSq = bitScanForward (attackBoard);
								
								if (chessBoard->BoardPosition [toSq] != EmptySquare)
									AddWhitePawnCaptureMove (chessBoard, fromSq, toSq, chessBoard->BoardPosition [toSq], moveList);
										
								attackBoard &= attackBoard - 1;
								
							}
						
							pieceBoard &= (pieceBoard - 1);
						
						}	
				
						break;
					
					case wN:
						
						while (pieceBoard) {
							
							fromSq = bitScanForward (pieceBoard);
							attackBoard = AND (Knight_Attacks [fromSq], ~chessBoard->whitePieces);
							
							while (attackBoard) {
								
								toSq = bitScanForward (attackBoard);
										
								if (chessBoard->BoardPosition [toSq] != EmptySquare) 
									AddCaptureMove (chessBoard, MOVE (fromSq, toSq, chessBoard->BoardPosition [toSq], EmptySquare, 0), moveList);
								else
									AddQuietMove (chessBoard, MOVE (fromSq, toSq, EmptySquare, EmptySquare, 0), moveList);
								
								attackBoard &= attackBoard - 1;
								
							}
						
							pieceBoard &= (pieceBoard - 1);
						
						}	
					
						break;
					
					case wB:
						
						while (pieceBoard) {
							
							fromSq = bitScanForward (pieceBoard);
							attackBoard = AND (bishopMagicAttacks (chessBoard->occupancy, fromSq), ~chessBoard->whitePieces);							
							while (attackBoard) {
								
								toSq = bitScanForward (attackBoard);
										
								if (chessBoard->BoardPosition [toSq] != EmptySquare) 
									AddCaptureMove (chessBoard, MOVE (fromSq, toSq, chessBoard->BoardPosition [toSq], EmptySquare, 0), moveList);
								else
									AddQuietMove (chessBoard, MOVE (fromSq, toSq, EmptySquare, EmptySquare, 0), moveList);								
								
								attackBoard &= attackBoard - 1;
								
							}
						
							pieceBoard &= (pieceBoard - 1);
						
						}	
				
						break;
					
					case wR:
						
						while (pieceBoard) {
							
							fromSq = bitScanForward (pieceBoard);
							attackBoard = AND (rookMagicAttacks (chessBoard->occupancy, fromSq), ~chessBoard->whitePieces);	
							
							while (attackBoard) {
								
								toSq = bitScanForward (attackBoard);
										
								if (chessBoard->BoardPosition [toSq] != EmptySquare) 
									AddCaptureMove (chessBoard, MOVE (fromSq, toSq, chessBoard->BoardPosition [toSq], EmptySquare, 0), moveList);
								else
									AddQuietMove (chessBoard, MOVE (fromSq, toSq, EmptySquare, EmptySquare, 0), moveList);
								
								attackBoard &= attackBoard - 1;
								
							}
						
							pieceBoard &= (pieceBoard - 1);
						
						}	
					
						break;
					
					case wQ:
						
						while (pieceBoard) {
							
							fromSq = bitScanForward (pieceBoard);
							attackBoard = AND (queenMagicAttacks (chessBoard->occupancy, fromSq), ~chessBoard->whitePieces);							
							while (attackBoard) {
								
								toSq = bitScanForward (attackBoard);
								
								if (chessBoard->BoardPosition [toSq] != EmptySquare) 
									AddCaptureMove (chessBoard, MOVE (fromSq, toSq, chessBoard->BoardPosition [toSq], EmptySquare, 0), moveList);
								else
									AddQuietMove (chessBoard, MOVE (fromSq, toSq, EmptySquare, EmptySquare, 0), moveList);
								
								attackBoard &= attackBoard - 1;
								
							}
						
							pieceBoard &= (pieceBoard - 1);
						
						}	
				
						break;
						
					case wK:
						
						if (chessBoard->castlingPermission & WhiteKingSideCastling) {
								if (!(GETBIT (chessBoard->occupancy, F1)) && !(GETBIT (chessBoard->occupancy, G1))) {
									if (!Is_Square_Attacked (chessBoard, E1, Black) && !Is_Square_Attacked (chessBoard, F1, Black)) {
										AddQuietMove (chessBoard, MOVE (E1, G1, EmptySquare, EmptySquare, MOVEFLAGCASTLING), moveList);
									}									
								} 
							}
							
						if (chessBoard->castlingPermission & WhiteQueenSideCastling) {
							if (!(GETBIT (chessBoard->occupancy, D1)) && !(GETBIT (chessBoard->occupancy, C1)) && !(GETBIT (chessBoard->occupancy, B1))) {
								if (!Is_Square_Attacked (chessBoard, E1, Black) && !Is_Square_Attacked (chessBoard, D1, Black)) {
									AddQuietMove (chessBoard, MOVE (E1, C1, EmptySquare, EmptySquare, MOVEFLAGCASTLING), moveList);
								} 
							} 
						}
							
						while (pieceBoard) {
							
							fromSq = bitScanForward (pieceBoard);
							attackBoard = AND (King_Attacks [fromSq], ~chessBoard->whitePieces);							
							while (attackBoard) {
								
								toSq = bitScanForward (attackBoard);
										
								if (chessBoard->BoardPosition [toSq] != EmptySquare) 
									AddCaptureMove (chessBoard, MOVE (fromSq, toSq, chessBoard->BoardPosition [toSq], EmptySquare, 0), moveList);
								else
									AddQuietMove (chessBoard, MOVE (fromSq, toSq, EmptySquare, EmptySquare, 0), moveList);
								
								attackBoard &= attackBoard - 1;
								
							}
						
							pieceBoard &= (pieceBoard - 1);
						
						}	
				
						break;
			
			}
		}
	}
	else {
		
		for (pieceIndex = 7; pieceIndex < 13; pieceIndex++) {
			
				U64 pieceBoard = chessBoard->pieces [pieceIndex];
				U64 attackBoard = 0ULL;
				
				switch (pieceIndex) {
					
					case bP:
						
						while (pieceBoard) {
							
							fromSq = bitScanForward (pieceBoard);
							attackBoard = AND (blackPawn_Attacks [fromSq], chessBoard->whitePieces);
							
							if (!(GETBIT (chessBoard->occupancy, fromSq - 8))) {
								AddBlackPawnMove (chessBoard, fromSq, fromSq - 8, moveList);
								if (RanksBoard [fromSq] == RANK_7 && !(GETBIT (chessBoard->occupancy, fromSq - 16))) {
									AddQuietMove (chessBoard, MOVE (fromSq, (fromSq - 16), EmptySquare, EmptySquare, MOVEFLAGPAWNSTART), moveList);
								}
							}	
							
							if (chessBoard->EnPassant_Square != No_Sq) {
									
								U64 enPassant = 0Ull;
								SETBIT (enPassant, chessBoard->EnPassant_Square);
									
									
								if (fromSq - 7 == chessBoard->EnPassant_Square && (enPassant & 0xfefefefefefefefe)) {
									AddEnPassantMove (chessBoard, MOVE (fromSq, fromSq - 7, EmptySquare, EmptySquare, MOVEFLAGENPASSANT), moveList);
								}
										
								if (fromSq - 9 == chessBoard->EnPassant_Square && (enPassant & 0x7f7f7f7f7f7f7f7f)) {
									AddEnPassantMove (chessBoard, MOVE (fromSq, fromSq - 9, EmptySquare, EmptySquare, MOVEFLAGENPASSANT), moveList);
								}
							}
							
							while (attackBoard) {
								
								toSq = bitScanForward (attackBoard);
								
								if (chessBoard->BoardPosition [toSq] != EmptySquare) 
									AddBlackPawnCaptureMove (chessBoard, fromSq, toSq, chessBoard->BoardPosition [toSq], moveList);
								
								attackBoard &= attackBoard - 1;
								
							}
						
							pieceBoard &= (pieceBoard - 1);
						
						}	
					
						break;
					
					case bN:
						
							while (pieceBoard) {
							
							fromSq = bitScanForward (pieceBoard);
							attackBoard = AND (Knight_Attacks [fromSq], ~chessBoard->blackPieces);							
							while (attackBoard) {
								
								toSq = bitScanForward (attackBoard);
										
								if (chessBoard->BoardPosition [toSq] != EmptySquare) 
									AddCaptureMove (chessBoard, MOVE (fromSq, toSq, chessBoard->BoardPosition [toSq], EmptySquare, 0), moveList);
								else
									AddQuietMove (chessBoard, MOVE (fromSq, toSq, EmptySquare, EmptySquare, 0), moveList);
								
								attackBoard &= attackBoard - 1;
								
							}
						
							pieceBoard &= (pieceBoard - 1);
						
						}	
					
						break;
					
					case bB:
						
						while (pieceBoard) {
							
							fromSq = bitScanForward (pieceBoard);
							attackBoard = AND (bishopMagicAttacks (chessBoard->occupancy, fromSq), ~chessBoard->blackPieces);	
							
							while (attackBoard) {
				
								toSq = bitScanForward (attackBoard);
								
								if (chessBoard->BoardPosition [toSq] != EmptySquare) 
									AddCaptureMove (chessBoard, MOVE (fromSq, toSq, chessBoard->BoardPosition [toSq], EmptySquare, 0), moveList);
								else
									AddQuietMove (chessBoard, MOVE (fromSq, toSq, EmptySquare, EmptySquare, 0), moveList);
								
								attackBoard &= attackBoard - 1;
								
							}
						
							pieceBoard &= (pieceBoard - 1);
						
						}	
					
						break;
					
					case bR:
						
						while (pieceBoard) {
							
							fromSq = bitScanForward (pieceBoard);
							attackBoard = AND (rookMagicAttacks (chessBoard->occupancy, fromSq), ~chessBoard->blackPieces);
							
							while (attackBoard) {
								
								toSq = bitScanForward (attackBoard);
								
								if (chessBoard->BoardPosition [toSq] != EmptySquare) 
									AddCaptureMove (chessBoard, MOVE (fromSq, toSq, chessBoard->BoardPosition [toSq], EmptySquare, 0), moveList);
								else
									AddQuietMove (chessBoard, MOVE (fromSq, toSq, EmptySquare, EmptySquare, 0), moveList);
								
								attackBoard &= attackBoard - 1;
								
							}
						
							pieceBoard &= (pieceBoard - 1);
						
						}	
					
						break;
					
					case bQ:
						
						while (pieceBoard) {
							
							fromSq = bitScanForward (pieceBoard);
							attackBoard = AND (queenMagicAttacks (chessBoard->occupancy, fromSq), ~chessBoard->blackPieces);							
							while (attackBoard) {
								
								toSq = bitScanForward (attackBoard);
										
								if (chessBoard->BoardPosition [toSq] != EmptySquare) 
									AddCaptureMove (chessBoard, MOVE (fromSq, toSq, chessBoard->BoardPosition [toSq], EmptySquare, 0), moveList);
								else
									AddQuietMove (chessBoard, MOVE (fromSq, toSq, EmptySquare, EmptySquare, 0), moveList);
								
								attackBoard &= attackBoard - 1;
								
							}
						
							pieceBoard &= (pieceBoard - 1);
						
						}	
							
						break;
						
					case bK:
						
						if (chessBoard->castlingPermission & BlackKingSideCastling) {
							if (!(GETBIT (chessBoard->occupancy, F8)) && !(GETBIT (chessBoard->occupancy, G8))) {
								if (!Is_Square_Attacked (chessBoard, E8, White) && !Is_Square_Attacked (chessBoard, F8, White)) {
									AddQuietMove (chessBoard, MOVE (E8, G8, EmptySquare, EmptySquare, MOVEFLAGCASTLING), moveList);
								} 
							} 
						}
							
						if (chessBoard->castlingPermission & BlackQueenSideCastling) {
							if (!(GETBIT (chessBoard->occupancy, D8)) && !(GETBIT (chessBoard->occupancy, C8)) && !(GETBIT (chessBoard->occupancy, B8))) {
								if (!Is_Square_Attacked (chessBoard, E8, White) && !Is_Square_Attacked (chessBoard, D8, White)) {
									AddQuietMove (chessBoard, MOVE (E8, C8, EmptySquare, EmptySquare, MOVEFLAGCASTLING), moveList);
								} 
							} 
						}
					
						while (pieceBoard) {
							
							fromSq = bitScanForward (pieceBoard);
							attackBoard = AND (King_Attacks [fromSq], ~chessBoard->blackPieces);							
							while (attackBoard) {
								
								toSq = bitScanForward (attackBoard);
								
								if (chessBoard->BoardPosition [toSq] != EmptySquare) 
									AddCaptureMove (chessBoard, MOVE (fromSq, toSq, chessBoard->BoardPosition [toSq], EmptySquare, 0), moveList);
								else
									AddQuietMove (chessBoard, MOVE (fromSq, toSq, EmptySquare, EmptySquare, 0), moveList);
								
								attackBoard &= attackBoard - 1;
								
							}
						
							pieceBoard &= (pieceBoard - 1);
						
						}	
					
						break;
					
				}
				
			
		}
		
	}
	//int finish = GetTimeInMilliseconds () - start;
	//timeTaken += finish;
}






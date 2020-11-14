#include "stdio.h"
#include "Definitions.h"
U64 rmask (int sq) {
	
	U64 result = 0ULL;
	int rk = sq/8, fl = sq%8, r, f;
	
	for (r = rk+1; r <= 6; r++) result |= (1ULL << (fl + r*8));
	for (r = rk-1; r >= 1; r--) result |= (1ULL << (fl + r*8));
	for (f = fl+1; f <= 6; f++) result |= (1ULL << (f + rk*8));
	for (f = fl-1; f >= 1; f--) result |= (1ULL << (f + rk*8));
	
	return result;
	
}
 
U64 bmask (int sq) {
	
	U64 result = 0ULL;
	int rk = sq/8, fl = sq%8, r, f;
	
	for (r=rk+1, f=fl+1; r<=6 && f<=6; r++, f++) result |= (1ULL << (f + r*8));
	for (r=rk+1, f=fl-1; r<=6 && f>=1; r++, f--) result |= (1ULL << (f + r*8));
	for (r=rk-1, f=fl+1; r>=1 && f<=6; r--, f++) result |= (1ULL << (f + r*8));
	for (r=rk-1, f=fl-1; r>=1 && f>=1; r--, f--) result |= (1ULL << (f + r*8));
	
	return result;
	
}
 
U64 ratt (int sq, U64 block) {
	
	U64 result = 0ULL;
	int rk = sq/8, fl = sq%8, r, f;
	
	for (r = rk+1; r <= 7; r++) {
		result |= (1ULL << (fl + r*8));
		if (block & (1ULL << (fl + r*8))) 
			break;
	}
	
	for (r = rk-1; r >= 0; r--) {
		result |= (1ULL << (fl + r*8));
		if (block & (1ULL << (fl + r*8))) 
			break;
	}
	
	for (f = fl+1; f <= 7; f++) {
		result |= (1ULL << (f + rk*8));
		if (block & (1ULL << (f + rk*8))) 
			break;
	}
	
	for (f = fl-1; f >= 0; f--) {
		result |= (1ULL << (f + rk*8));
		if (block & (1ULL << (f + rk*8))) 
			break;
	}
	
	return result;
	  
}
 
U64 batt (int sq, U64 block) {
	
	U64 result = 0ULL;
	int rk = sq/8, fl = sq%8, r, f;
	
	for (r = rk+1, f = fl+1; r <= 7 && f <= 7; r++, f++) {
		
		result |= (1ULL << (f + r*8));
		
		if (block & (1ULL << (f + r * 8))) 
			break;
	
	}
	
	for (r = rk+1, f = fl-1; r <= 7 && f >= 0; r++, f--) {
		
		result |= (1ULL << (f + r*8));
		
		if (block & (1ULL << (f + r * 8))) 
			break;
	}
	
	for (r = rk-1, f = fl+1; r >= 0 && f <= 7; r--, f++) {
		
		result |= (1ULL << (f + r*8));
		
		if (block & (1ULL << (f + r * 8))) 
			break;
	}
	
	for (r = rk-1, f = fl-1; r >= 0 && f >= 0; r--, f--) {
		
		result |= (1ULL << (f + r*8));
		
		if (block & (1ULL << (f + r * 8))) 
			break;
		
	}
	
	return result;

}

U64 pawnMask (int sq, int side) {
	
	U64 result = 0ULL;
	U64 fromSq = 0ULL;
	SETBIT (fromSq, sq);
	
	if (side == White) {
		
		if (fromSq << 7 & notHFile)   
			result |= fromSq << 7;
	
		if (fromSq << 9 & notAFile)
			result |= fromSq << 9;
		
	}
	else {
		
		if (fromSq >> 7 & notAFile)
			result |= fromSq >> 7;
	
		if (fromSq >> 9 & notHFile)
			result |= fromSq >> 9;
		
	}
		
	
	return result;
	
}

U64 knightMask (int sq) {
	
//	int start = GetTimeInMilliseconds ();
	
	U64 result = 0ULL;
	U64 fromSq = 0ULL;
	SETBIT (fromSq, sq);
	
	if (fromSq >> 17 & notHFile)
		result |= fromSq >> 17 & notHFile;
	
	if (fromSq >> 15 & notAFile)
		result |= fromSq >> 15 & notAFile;
	
	if (fromSq >> 10 & notGHFile)
		result |= fromSq >> 10 & notGHFile;
	
	if (fromSq >> 6 & notABFile)
		result |= fromSq >> 6 & notABFile;
	
	if (fromSq << 17 & notAFile)
		result |= fromSq << 17 & notAFile;
	
	if (fromSq << 15 & notHFile)
		result |= fromSq << 15 & notHFile;
	
	if (fromSq << 10 & notABFile)
		result |= fromSq << 10 & notABFile;
	
	if (fromSq << 6 & notGHFile)
		result |= fromSq << 6 & notGHFile;
	
	//int finish = GetTimeInMilliseconds () - start;
	//timeTaken += finish;
	
	return result;
	
}

U64 kingMask (int sq) {
	
	U64 result = 0ULL;
	U64 fromSq = 0ULL;
	SETBIT (fromSq, sq);
	
	if (fromSq >> 1 & notHFile)
		result |= fromSq >> 1 & notHFile;
	
	if (fromSq << 1 & notAFile)
		result |= fromSq << 1 & notAFile;
	
	if (fromSq >> 8)
		result |= fromSq >> 8;
	
	if (fromSq << 8)
		result |= fromSq << 8;
	
	if (fromSq >> 9 & notHFile)
		result |= fromSq >> 9 & notHFile;
	
	if (fromSq >> 7 & notAFile)
		result |= fromSq >> 7 & notAFile;
	
	if (fromSq << 7 & notHFile)
		result |= fromSq << 7 & notHFile;
	
	if (fromSq << 9 & notAFile)
		result |= fromSq << 9 & notAFile;
	
	return result;
	
}

U64 pawnAttackBitboard (int sq,  U64 opponentBlockers, int side) {
	
	U64 result = 0ULL;
	U64 fromSq = 0ULL;
	SETBIT (fromSq, sq);
	
	if (side == White) {
		
		if (opponentBlockers & (fromSq << 7 & notHFile))   
			result |= fromSq << 7;
	
		if (opponentBlockers & (fromSq << 9 & notAFile))
			result |= fromSq << 9;
		
	}
	else {
		
		if (opponentBlockers & (fromSq >> 7 & notAFile))
			result |= fromSq >> 7;
	
		if (opponentBlockers & (fromSq >> 9 & notHFile))
			result |= fromSq >> 9;
		
	}
		
	
	return result;
	
}

U64 knightAttackBitboard (int sq, U64 ownBlockers) {
	
//	int start = GetTimeInMilliseconds ();
	
	U64 result = 0ULL;
	U64 fromSq = 0ULL;
	SETBIT (fromSq, sq);
	
	if (!(ownBlockers & fromSq >> 17 & notHFile))
		result |= fromSq >> 17 & notHFile;
	
	if (!(ownBlockers & fromSq >> 15 & notAFile))
		result |= fromSq >> 15 & notAFile;
	
	if (!(ownBlockers & fromSq >> 10 & notGHFile))
		result |= fromSq >> 10 & notGHFile;
	
	if (!(ownBlockers & fromSq >> 6 & notABFile))
		result |= fromSq >> 6 & notABFile;
	
	if (!(ownBlockers & fromSq << 17 & notAFile))
		result |= fromSq << 17 & notAFile;
	
	if (!(ownBlockers & fromSq << 15 & notHFile))
		result |= fromSq << 15 & notHFile;
	
	if (!(ownBlockers & fromSq << 10 & notABFile))
		result |= fromSq << 10 & notABFile;
	
	if (!(ownBlockers & fromSq << 6 & notGHFile))
		result |= fromSq << 6 & notGHFile;
	
	//int finish = GetTimeInMilliseconds () - start;
	//timeTaken += finish;
	
	return result;
	
}

U64 rookAttackBitboard (int sq, U64 opponentBlockers, U64 ownBlockers) {
	
	U64 result = 0ULL;
	int rk = sq/8, fl = sq%8, r, f;
	
	for (r = rk+1; r <= 7; r++) {
		
		if (ownBlockers & (1ULL << (fl + r*8))) 
			break;
		
		result |= (1ULL << (fl + r*8));
		
		if (opponentBlockers & (1ULL << (fl + r*8))) 
			break;
		
	}
	
	for (r = rk-1; r >= 0; r--) {
		
		if (ownBlockers & (1ULL << (fl + r*8))) 
			break;
		
		result |= (1ULL << (fl + r*8));
		
		if (opponentBlockers & (1ULL << (fl + r*8))) 
			break;
		
	}
	
	for (f = fl+1; f <= 7; f++) {
		
		if (ownBlockers & (1ULL << (f + rk*8))) 
			break;
		
		result |= (1ULL << (f + rk*8));
		
		if (opponentBlockers & (1ULL << (f + rk*8))) 
			break;
		
	}
	
	for (f = fl-1; f >= 0; f--) {
		
		if (ownBlockers & (1ULL << (f + rk*8))) 
			break;
		
		result |= (1ULL << (f + rk*8));
		
		if (opponentBlockers & (1ULL << (f + rk*8))) 
			break;
	}
	
	return result;
	  
}

U64 bishopAttackBitboard (int sq, U64 opponentBlockers, U64 ownBlockers) {
	
	U64 result = 0ULL;
	int rk = sq/8, fl = sq%8, r, f;
	
	for (r = rk+1, f = fl+1; r <= 7 && f <= 7; r++, f++) {
		
		if (ownBlockers & (1ULL << (f + r * 8))) 
			break;
		
		result |= (1ULL << (f + r*8));
		
		if (opponentBlockers & (1ULL << (f + r * 8))) 
			break;
	
	}
	
	for (r = rk+1, f = fl-1; r <= 7 && f >= 0; r++, f--) {
		
		if (ownBlockers & (1ULL << (f + r * 8))) 
			break;
		
		result |= (1ULL << (f + r*8));
		
		if (opponentBlockers & (1ULL << (f + r * 8))) 
			break;
	}
	
	for (r = rk-1, f = fl+1; r >= 0 && f <= 7; r--, f++) {
		
		if (ownBlockers & (1ULL << (f + r * 8))) 
			break;
		
		result |= (1ULL << (f + r*8));
		
		if (opponentBlockers & (1ULL << (f + r * 8))) 
			break;
	}
	
	for (r = rk-1, f = fl-1; r >= 0 && f >= 0; r--, f--) {
		
		if (ownBlockers & (1ULL << (f + r * 8))) 
			break;
		
		result |= (1ULL << (f + r*8));
		
		if (opponentBlockers & (1ULL << (f + r * 8))) 
			break;
		
	}
	
	return result;

}

U64 queenAttackBitboard (int sq, U64 opponentBlockers, U64 ownBlockers) {
	
	//int start = GetTimeInMilliseconds ();
	
	U64 result = 0ULL;
	int rk = sq/8, fl = sq%8, r, f;
	
	for (r = rk+1; r <= 7; r++) {
		
		if (ownBlockers & (1ULL << (fl + r*8))) 
			break;
		
		result |= (1ULL << (fl + r*8));
		
		if (opponentBlockers & (1ULL << (fl + r*8))) 
			break;
		
	}
	
	for (r = rk-1; r >= 0; r--) {
		
		if (ownBlockers & (1ULL << (fl + r*8))) 
			break;
		
		result |= (1ULL << (fl + r*8));
		
		if (opponentBlockers & (1ULL << (fl + r*8))) 
			break;
		
	}
	
	for (f = fl+1; f <= 7; f++) {
		
		if (ownBlockers & (1ULL << (f + rk*8))) 
			break;
		
		result |= (1ULL << (f + rk*8));
		
		if (opponentBlockers & (1ULL << (f + rk*8))) 
			break;
		
	}
	
	for (f = fl-1; f >= 0; f--) {
		
		if (ownBlockers & (1ULL << (f + rk*8))) 
			break;
		
		result |= (1ULL << (f + rk*8));
		
		if (opponentBlockers & (1ULL << (f + rk*8))) 
			break;
	}
	
	
	for (r = rk+1, f = fl+1; r <= 7 && f <= 7; r++, f++) {
		
		if (ownBlockers & (1ULL << (f + r * 8))) 
			break;
		
		result |= (1ULL << (f + r*8));
		
		if (opponentBlockers & (1ULL << (f + r * 8))) 
			break;
	
	}
	
	for (r = rk+1, f = fl-1; r <= 7 && f >= 0; r++, f--) {
		
		if (ownBlockers & (1ULL << (f + r * 8))) 
			break;
		
		result |= (1ULL << (f + r*8));
		
		if (opponentBlockers & (1ULL << (f + r * 8))) 
			break;
	}
	
	for (r = rk-1, f = fl+1; r >= 0 && f <= 7; r--, f++) {
		
		if (ownBlockers & (1ULL << (f + r * 8))) 
			break;
		
		result |= (1ULL << (f + r*8));
		
		if (opponentBlockers & (1ULL << (f + r * 8))) 
			break;
	}
	
	for (r = rk-1, f = fl-1; r >= 0 && f >= 0; r--, f--) {
		
		if (ownBlockers & (1ULL << (f + r * 8))) 
			break;
		
		result |= (1ULL << (f + r*8));
		
		if (opponentBlockers & (1ULL << (f + r * 8))) 
			break;
		
	}
	
	//int finish = GetTimeInMilliseconds () - start;
	//timeTaken += finish;
	
	return result;	
	
}

U64 kingAttackBitboard (int sq, U64 ownBlockers) {
	
	U64 result = 0ULL;
	U64 fromSq = 0ULL;
	SETBIT (fromSq, sq);
	
	if (!(ownBlockers & fromSq >> 1 & notHFile))
		result |= fromSq >> 1 & notHFile;
	
	if (!(ownBlockers & fromSq << 1 & notAFile))
		result |= fromSq << 1 & notAFile;
	
	if (!(ownBlockers & fromSq >> 8))
		result |= fromSq >> 8;
	
	if (!(ownBlockers & fromSq << 8))
		result |= fromSq << 8;
	
	if (!(ownBlockers & fromSq >> 9 & notHFile))
		result |= fromSq >> 9 & notHFile;
	
	if (!(ownBlockers & fromSq >> 7 & notAFile))
		result |= fromSq >> 7 & notAFile;
	
	if (!(ownBlockers & fromSq << 7 & notHFile))
		result |= fromSq << 7 & notHFile;
	
	if (!(ownBlockers & fromSq << 9 & notAFile))
		result |= fromSq << 9 & notAFile;
	
	return result;
	
}
	


int Is_Square_Attacked (CHESS_BOARD *chessBoard, int sq, int side) {  //Is_Square_Attacked
    if ((side == White) && (pawn_attacks[Black][sq] & chessBoard->pieces[wP])) return True;
    if ((side == Black) && (pawn_attacks[White][sq] & chessBoard->pieces[bP])) return True;
    
    if (Knight_Attacks[sq] & ((side == White) ? chessBoard->pieces[wN] : chessBoard->pieces[bN])) return True;
    if (King_Attacks[sq] & ((side == White) ? chessBoard->pieces[wK] : chessBoard->pieces[bK])) return True;
    if (bishopMagicAttacks(chessBoard->occupancy, sq) & ((side == White) ? chessBoard->pieces[wB] : chessBoard->pieces[bB])) return True;
    if (rookMagicAttacks(chessBoard->occupancy, sq) & ((side == White) ? chessBoard->pieces[wR] : chessBoard->pieces[bR])) return True;    
    if (queenMagicAttacks(chessBoard->occupancy, sq) & ((side == White) ? chessBoard->pieces[wQ] : chessBoard->pieces[bQ])) return True;
	
	return False;	
}

U64 bishopMagicAttacks(U64 occ, int sq) {
	
	occ &= bishopMask [sq];
	occ *=  bishopMagics [sq];
	occ >>= bishopShifts [sq];
	
	return Bishop_Table [sq][occ];
	
}

U64 rookMagicAttacks(U64 occ, int sq) {
	
	occ &= rookMask [sq];
	occ *=  rookMagics [sq];
	occ >>= rookShifts [sq];
	
	return Rook_Table [sq][occ];
	
}

U64 queenMagicAttacks(U64 occ, int sq) {
	
	U64 result = 0ULL;
	U64 temp_occ = occ;
	
	occ &= bishopMask [sq];
	occ *=  bishopMagics [sq];
	occ >>= bishopShifts [sq];
	
	result = Bishop_Table [sq][occ];
	
	temp_occ &= rookMask [sq];
	temp_occ *=  rookMagics [sq];
	temp_occ >>= rookShifts [sq];
	
	result |= Rook_Table [sq][temp_occ];
	
	return result;
	
}



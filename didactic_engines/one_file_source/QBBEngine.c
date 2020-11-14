/*
 QBBEngine

 This is a basic engine that shows how to use quadbitboards. Quadbitboards is a very compact board representation,
 it uses only 256bits, good for a very fast Make/UnMake functions.

 Some parts of this source call gcc intrinsic functions. If you are not using gcc you need to
 change them with the functions of your compiler.
*/

#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdio.h>

/* define the mate value */
#define MATE 10000

#define WHITE 0
#define BLACK 8

/* define the piece type: empty, pawn, knight, bishop, rook, queen, king */
typedef enum {EMPTY,PAWN,KNIGHT,BISHOP,ROOK,QUEEN,KING} TPieceType;

/* define the move type, for example
   KING|CASTLE is a castle move
   PAWN|CAPTURE|EP is an enpassant move
   PAWN|PROMO|CAPTURE is a promotion with a capture */
typedef enum {CASTLE=0x40,PROMO=0x20,EP=0x10,CAPTURE=0x08} TMoveType;

/* bitboard types */
typedef uint64_t TBB;

/* move structure */
typedef union
{
   struct{
   uint8_t MoveType;
   uint8_t From;
   uint8_t To;
   uint8_t Prom;};
   unsigned int Move;
}TMove;

/* structure for a capture, it needs the Eval field for MVV/LVA ordering */
typedef struct
{
   TMove Move;
   int Eval;
} TMoveEval;

/*
Board structure definition

PM,P0,P1,P2 are the 4 bitboards that contain the whole board
PM is the bitboard with the side to move pieces
P0,P1 and P2: with these bitboards you can obtain every type of pieces and every pieces combinations.
*/
typedef struct
{
   TBB PM;
   TBB P0;
   TBB P1;
   TBB P2;
   uint8_t CastleFlags; /* ..sl..SL  short long opponent SHORT LONG side to move */
   uint8_t EnPassant; /* enpassant column, =8 if not set */
   uint8_t Count50; /* 50 move rule counter */
   uint8_t Rep; /* 0 if it's not a repetition, 1 if it is */
   uint8_t STM; /* side to move */
} TBoard;


int64_t Nodes; /* used in the alphabeta function, it saves the numbers of nodes searched */

/*
Into Game are saved all the positions from the last 50 move counter reset
Position is the pointer to the last position of the game
*/
TBoard Game[512];
TBoard *Position;

/* Pieces static values */
const int StaticValue[8]={0,100,300,300,500,950,0,0};

/* Piece Square Tables */
const int PST[8][64]=
{
{  0,  0,  0,  0,  0,  0,  0,  0, /* empty */
   0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0},
{  0,  0,  0,  0,  0,  0,  0,  0, /* pawn */
   5,  5,  5,-20,-20,  5,  5,  5,
   5, -5, -5,  0,  0, -5, -5,  5,
   0,  0,  0, 12, 12,  0,  0,  0,
  10, 10, 10, 16, 16, 10, 10, 10,
  30, 30, 30, 40, 40, 30, 30, 30,
 100,100,100,100,100,100,100,100,
   0,  0,  0,  0,  0,  0,  0,  0},
{ -5, -5,  0,  0,  0,  0, -5, -5,  /* knight */
  -5,  0, 10, 10, 10, 10,  0, -5,
   0, 10, 15, 15, 15, 15, 10,  0,
   0, 10, 15, 20, 20, 15, 10,  0,
   0, 10, 15, 20, 20, 15, 10,  0,
   0, 10, 15, 15, 15, 15, 10,  0,
  -5,  0, 10, 10, 10, 10,  0, -5,
  -5, -5,  0,  0,  0,  0, -5, -5},
{ -5, -5,  0,  0,  0,  0, -5, -5,  /* bishop */
  -5,  0, 10, 10, 10, 10,  0, -5,
  -5, 10, 15, 15, 15, 15, 10, -5,
  -5, 10, 15, 20, 20, 15, 10, -5,
  -5, 10, 15, 20, 20, 15, 10, -5,
  -5, 10, 15, 15, 15, 15, 10, -5,
  -5,  0, 10, 10, 10, 10,  0, -5,
  -5, -5,  0,  0,  0,  0, -5, -5},
{  0, -5, -5, 10, 10, -5, -5,  0, /* rook */
   0,  0,  0, 10, 10,  0,  0,  0,
   0,  0,  0, 10, 10,  0,  0,  0,
   0,  0,  0, 10, 10,  0,  0,  0,
   0,  0,  0, 10, 10,  0,  0,  0,
   0,  0,  0, 10, 10,  0,  0,  0,
  20, 20, 20, 20, 20, 20, 20, 20,
  10, 10, 10, 10, 10, 10, 10, 10},
{ -4, -4, -4, -4, -4, .4, -4, -4,  /* queen */
  -4,  0,  0,  0,  0,  0,  0, -4,
  -4,  0,  4,  4,  4,  4,  0, -4,
  -4,  0,  4,  8,  8,  4,  0, -4,
  -4,  0,  4,  8,  8,  4,  0, -4,
  -4,  0,  4,  4,  4,  4,  0, -4,
  -4,  0,  0,  0,  0,  0,  0, -4,
  -4, -4, -4, -4, -4, -4, -4, -4},
{ -5, 25, 15,-10,  0,-10, 30, -5, /* king middlegame */
  -5, -5, -5, -5, -5, -5, -5, -5,
  -5, -5, -5, -5, -5, -5, -5, -5,
  -5, -5, -5, -5, -5, -5, -5, -5,
  -5, -5, -5, -5, -5, -5, -5, -5,
  -5, -5, -5, -5, -5, -5, -5, -5,
  -5, -5, -5, -5, -5, -5, -5, -5,
  -5, -5, -5, -5, -5, -5, -5, -5},
{  0, -5, -5,  0,  0,  0, -5,  0, /* king endgame */
   0,  5,  5,  5,  5,  5,  5,  0,
   0,  5, 10, 10, 10, 10,  5,  0,
   0,  5, 10, 15, 15, 10,  5,  0,
   0,  5, 10, 15, 15, 10,  5,  0,
   0,  5, 10, 10, 10, 10,  5,  0,
   0,  5,  5,  5,  5,  5,  5,  0,
   0,  0,  0,  0,  0,  0,  0,  0}
};

/* array of bitboards that contains all the knight destination for every square */
const TBB KnightDest[64]= {0x0000000000020400ULL,0x0000000000050800ULL,0x00000000000a1100ULL,0x0000000000142200ULL,
                           0x0000000000284400ULL,0x0000000000508800ULL,0x0000000000a01000ULL,0x0000000000402000ULL,
                           0x0000000002040004ULL,0x0000000005080008ULL,0x000000000a110011ULL,0x0000000014220022ULL,
                           0x0000000028440044ULL,0x0000000050880088ULL,0x00000000a0100010ULL,0x0000000040200020ULL,
                           0x0000000204000402ULL,0x0000000508000805ULL,0x0000000a1100110aULL,0x0000001422002214ULL,
                           0x0000002844004428ULL,0x0000005088008850ULL,0x000000a0100010a0ULL,0x0000004020002040ULL,
                           0x0000020400040200ULL,0x0000050800080500ULL,0x00000a1100110a00ULL,0x0000142200221400ULL,
                           0x0000284400442800ULL,0x0000508800885000ULL,0x0000a0100010a000ULL,0x0000402000204000ULL,
                           0x0002040004020000ULL,0x0005080008050000ULL,0x000a1100110a0000ULL,0x0014220022140000ULL,
                           0x0028440044280000ULL,0x0050880088500000ULL,0x00a0100010a00000ULL,0x0040200020400000ULL,
                           0x0204000402000000ULL,0x0508000805000000ULL,0x0a1100110a000000ULL,0x1422002214000000ULL,
                           0x2844004428000000ULL,0x5088008850000000ULL,0xa0100010a0000000ULL,0x4020002040000000ULL,
                           0x0400040200000000ULL,0x0800080500000000ULL,0x1100110a00000000ULL,0x2200221400000000ULL,
                           0x4400442800000000ULL,0x8800885000000000ULL,0x100010a000000000ULL,0x2000204000000000ULL,
                           0x0004020000000000ULL,0x0008050000000000ULL,0x00110a0000000000ULL,0x0022140000000000ULL,
                           0x0044280000000000ULL,0x0088500000000000ULL,0x0010a00000000000ULL,0x0020400000000000ULL};
/* The same for the king */
const TBB KingDest[64] = {0x0000000000000302ULL,0x0000000000000705ULL,0x0000000000000e0aULL,0x0000000000001c14ULL,
                          0x0000000000003828ULL,0x0000000000007050ULL,0x000000000000e0a0ULL,0x000000000000c040ULL,
                          0x0000000000030203ULL,0x0000000000070507ULL,0x00000000000e0a0eULL,0x00000000001c141cULL,
                          0x0000000000382838ULL,0x0000000000705070ULL,0x0000000000e0a0e0ULL,0x0000000000c040c0ULL,
                          0x0000000003020300ULL,0x0000000007050700ULL,0x000000000e0a0e00ULL,0x000000001c141c00ULL,
                          0x0000000038283800ULL,0x0000000070507000ULL,0x00000000e0a0e000ULL,0x00000000c040c000ULL,
                          0x0000000302030000ULL,0x0000000705070000ULL,0x0000000e0a0e0000ULL,0x0000001c141c0000ULL,
                          0x0000003828380000ULL,0x0000007050700000ULL,0x000000e0a0e00000ULL,0x000000c040c00000ULL,
                          0x0000030203000000ULL,0x0000070507000000ULL,0x00000e0a0e000000ULL,0x00001c141c000000ULL,
                          0x0000382838000000ULL,0x0000705070000000ULL,0x0000e0a0e0000000ULL,0x0000c040c0000000ULL,
                          0x0003020300000000ULL,0x0007050700000000ULL,0x000e0a0e00000000ULL,0x001c141c00000000ULL,
                          0x0038283800000000ULL,0x0070507000000000ULL,0x00e0a0e000000000ULL,0x00c040c000000000ULL,
                          0x0302030000000000ULL,0x0705070000000000ULL,0x0e0a0e0000000000ULL,0x1c141c0000000000ULL,
                          0x3828380000000000ULL,0x7050700000000000ULL,0xe0a0e00000000000ULL,0xc040c00000000000ULL,
                          0x0203000000000000ULL,0x0507000000000000ULL,0x0a0e000000000000ULL,0x141c000000000000ULL,
                          0x2838000000000000ULL,0x5070000000000000ULL,0xa0e0000000000000ULL,0x40c0000000000000ULL};

/* masks for finding the pawns that can capture with an enpassant (in move generation) */
const TBB EnPassant[8] = {
0x0000000200000000ULL,0x0000000500000000ULL,0x0000000A00000000ULL,0x0000001400000000ULL,
0x0000002800000000ULL,0x0000005000000000ULL,0x000000A000000000ULL,0x0000004000000000ULL
};

/* masks for finding the pawns that can capture with an enpassant (in make move) */
const TBB EnPassantM[8] = {
0x0000000002000000ULL,0x0000000005000000ULL,0x000000000A000000ULL,0x0000000014000000ULL,
0x0000000028000000ULL,0x0000000050000000ULL,0x00000000A0000000ULL,0x0000000040000000ULL
};

/*
reverse a bitboard:
A bitboard is an array of byte: Byte0,Byte1,Byte2,Byte3,Byte4,Byte5,Byte6,Byte7
after this function the bitboard will be: Byte7,Byte6,Byte5,Byte4,Byte3,Byte2,Byte1,Byte0

The board is saved always with the side to move in the low significant bits of the bitboard, so this function
is used to change the side to move
*/
#define RevBB(bb) (__builtin_bswap64(bb))
/* return the index of the most significant bit of the bitboard, bb must always be !=0 */
#define MSB(bb) (0x3F ^ __builtin_clzll(bb))
/* return the index of the least significant bit of the bitboard, bb must always be !=0 */
#define LSB(bb) (__builtin_ctzll(bb))
/* extract the least significant bit of the bitboard */
#define ExtractLSB(bb) ((bb)&(-(bb)))
/* reset the least significant bit of bb */
#define ClearLSB(bb) ((bb)&((bb)-1))
/* return the number of bits sets of a bitboard */
#define PopCount(bb) (__builtin_popcountll(bb))

/* Macro to check and reset the castle rights:
   CastleSM: short castling side to move
   CastleLM: long castling side to move
   CastleSO: short castling opponent
   CastleLO: long castling opponent
 */
#define CastleSM (Position->CastleFlags&0x02)
#define CastleLM (Position->CastleFlags&0x01)
#define CastleSO (Position->CastleFlags&0x20)
#define CastleLO (Position->CastleFlags&0x10)
#define ResetCastleSM (Position->CastleFlags&=0xFD)
#define ResetCastleLM (Position->CastleFlags&=0xFE)
#define ResetCastleSO (Position->CastleFlags&=0xDF)
#define ResetCastleLO (Position->CastleFlags&=0xEF)

/* these Macros are used to calculate the bitboard of a particular kind of piece

   P2 P1 P0
    0  0  0    empty
    0  0  1    pawn
    0  1  0    knight
    0  1  1    bishop
    1  0  0    rook
    1  0  1    queen
    1  1  0    king
*/
#define Occupation (Position->P0 | Position->P1 | Position->P2) /* board occupation */
#define Pawns (Position->P0 & ~Position->P1 & ~Position->P2) /* all the pawns on the board */
#define Knights (~Position->P0 & Position->P1 & ~Position->P2)
#define Bishops (Position->P0 & Position->P1)
#define Rooks (~Position->P0 & ~Position->P1 & Position->P2)
#define Queens (Position->P0 & Position->P2)
#define Kings (Position->P1 & Position->P2) /* a bitboard with the 2 kings */

/* get the piece type giving the square */
#define Piece(sq) (((Position->P2>>(sq))&1)<<2 | ((Position->P1>>(sq))&1)<<1 | ((Position->P0>>(sq))&1))

/* calculate the square related to the opponent */
#define OppSq(sp) ((sp)^0x38)
/* Absolute Square, we need this macro to return the move in long algebric notation  */
#define AbsSq(sq,col) ((col)==WHITE ? (sq):OppSq(sq))

/* get the corresponding string to the given move  */
static inline void MoveToStr(char *strmove,TMove move,uint8_t tomove)
{
   const char promo[7]="\0\0nbrq";
   strmove[0] = 'a' + AbsSq(move.From,tomove) % 8;
   strmove[1] = '1' + AbsSq(move.From,tomove) / 8;
   strmove[2] = 'a' + AbsSq(move.To,tomove) % 8;
   strmove[3] = '1' + AbsSq(move.To,tomove) / 8;
   strmove[4] = promo[move.Prom];
   strmove[5] = '\0';
}

/* get the corresponding move to the given string */
static inline TMove StrToMove(char *strmove)
{
   TMove move;
   move.From = AbsSq((uint8_t)(strmove[0] - 'a' + (strmove[1] - '1')*8),Position->STM);
   move.To = AbsSq((uint8_t)(strmove[2] - 'a' + (strmove[3] - '1')*8),Position->STM);
   move.Prom=EMPTY;
   if (strmove[4]=='n') move.Prom=KNIGHT;
   else if (strmove[4]=='b') move.Prom=BISHOP;
   else if (strmove[4]=='r') move.Prom=ROOK;
   else if (strmove[4]=='q') move.Prom=QUEEN;
   move.MoveType=Piece(move.From);
   if (move.MoveType==PAWN)
   {
      if ((1ULL<<move.To)&0xFF00000000000000ULL) move.MoveType|=PROMO;
      else if (Position->EnPassant!=8 && move.To==(40+Position->EnPassant)) move.MoveType=PAWN|EP|CAPTURE;
   }
   else if (move.MoveType==KING && (move.To-move.From==2 || move.From-move.To==2)) move.MoveType=KING|CASTLE;
   if (Piece(move.To)) move.MoveType|=CAPTURE;
   return move;
}

/*
The board is always saved with the side to move in the lower part of the bitboards to use the same generation and
make for the Black and the White side.
This needs the inversion of the 4 bitboards, roll the Castle rights and update the side to move.
*/
#define ChangeSide \
do{ \
   Position->PM^=Occupation; /* update the side to move pieces */\
   Position->PM=RevBB(Position->PM);\
   Position->P0=RevBB(Position->P0);\
   Position->P1=RevBB(Position->P1);\
   Position->P2=RevBB(Position->P2);/* reverse the board */\
   Position->CastleFlags=(Position->CastleFlags>>4)|(Position->CastleFlags<<4);/* roll the castle rights */\
   Position->STM ^= BLACK; /* change the side to move */\
}while(0)

/* Extract the best capture (with the greater eval) from the array */
static inline TMove ExtractBest(TMoveEval *const capture,TMoveEval *const pcapture)
{
   TMoveEval *imax=pcapture-1;
   for (TMoveEval *i=pcapture-2;capture<=i;i--)
      if (i->Eval>imax->Eval) imax = i;

   TMove best = imax->Move;
   *imax=*(pcapture-1);
   return best;
}

/* return the bitboard with the rook destinations */
static inline TBB GenRook(uint64_t sq,TBB occupation)
{
   TBB piece = 1ULL<<sq;
   occupation ^= piece; /* remove the selected piece from the occupation */
   TBB piecesup=(0x0101010101010101ULL<<sq)&(occupation|0xFF00000000000000ULL); /* find the pieces up */
   TBB piecesdo=(0x8080808080808080ULL>>(63-sq))&(occupation|0x00000000000000FFULL); /* find the pieces down */
   TBB piecesri=(0x00000000000000FFULL<<sq)&(occupation|0x8080808080808080ULL); /* find pieces on the right */
   TBB piecesle=(0xFF00000000000000ULL>>(63-sq))&(occupation|0x0101010101010101ULL); /* find pieces on the left */
   return (((0x8080808080808080ULL>>(63-LSB(piecesup)))&(0x0101010101010101ULL<<MSB(piecesdo))) |
          ((0xFF00000000000000ULL>>(63-LSB(piecesri)))&(0x00000000000000FFULL<<MSB(piecesle))))^piece;
   /* From every direction find the first piece and from that piece put a mask in the opposite direction.
      Put togheter all the 4 masks and remove the moving piece */
}

/* return the bitboard with the bishops destinations */
static inline TBB GenBishop(uint64_t sq,TBB occupation)
{  /* it's the same as the rook */
   TBB piece = 1ULL<<sq;
   occupation ^= piece;
   TBB piecesup=(0x8040201008040201ULL<<sq)&(occupation|0xFF80808080808080ULL);
   TBB piecesdo=(0x8040201008040201ULL>>(63-sq))&(occupation|0x01010101010101FFULL);
   TBB piecesle=(0x8102040810204081ULL<<sq)&(occupation|0xFF01010101010101ULL);
   TBB piecesri=(0x8102040810204081ULL>>(63-sq))&(occupation|0x80808080808080FFULL);
   return (((0x8040201008040201ULL>>(63-LSB(piecesup)))&(0x8040201008040201ULL<<MSB(piecesdo))) |
          ((0x8102040810204081ULL>>(63-LSB(piecesle)))&(0x8102040810204081ULL<<MSB(piecesri))))^piece;
}

/* return the bitboard with pieces of the same type */
static inline TBB BBPieces(TPieceType piece)
{
   switch(piece) // find the bb with the pieces of the same type
   {
      case PAWN: return Pawns;
      case KNIGHT: return Knights;
      case BISHOP: return Bishops;
      case ROOK: return Rooks;
      case QUEEN: return Queens;
      case KING: return Kings;
   }
}

/* return the bitboard with the destinations of a piece in a square (exept for pawns) */
static inline TBB BBDestinations(TPieceType piece,uint64_t sq,TBB occupation)
{
   switch(piece) // generate the destination squares of the piece
   {
      case KNIGHT: return KnightDest[sq];
      case BISHOP: return GenBishop(sq,occupation);
      case ROOK: return GenRook(sq,occupation);
      case QUEEN: return GenRook(sq,occupation)|GenBishop(sq,occupation);
      case KING: return KingDest[sq];
   }
}

/* If the king is in check this function return the pieces that are attacking the king. If there aren't it returns 0 */
static inline TBB InCheck(void)
{
   TBB occupation,opposing;
   TBB king = Kings&Position->PM;
   uint64_t kingsq = LSB(king);
   occupation = Occupation;
   opposing = Position->PM^occupation;
   return (((KnightDest[kingsq]&Knights)|
       (GenRook(kingsq,occupation)&(Rooks|Queens))|
       (GenBishop(kingsq,occupation)&(Bishops|Queens))|
       ((((king<<9)&0xFEFEFEFEFEFEFEFEULL)|((king<<7)&0x7F7F7F7F7F7F7F7FULL))&Pawns)|
       (KingDest[kingsq]&Kings))&opposing);
}

/* try the move and see if the king is in check. If so return the attacking pieces, if not return 0 */
static inline TBB Illegal(TMove move)
{
   TBB From,To;
   From = 1ULL<<move.From;
   To = 1ULL<<move.To;
   TBB occupation,opposing;
   occupation = Occupation;
   opposing = Position->PM^occupation;
   TBB newoccupation,newopposing;
   TBB king;
   uint64_t kingsq;
   newoccupation = (occupation^From)|To;
   newopposing = opposing&~To;
   if ((move.MoveType&0x07)==KING)
   {
      king = To;
      kingsq = move.To;
   }
   else
   {
      king = Kings&Position->PM;
      kingsq = LSB(king);
      if (move.MoveType&EP) {newopposing^=To>>8; newoccupation^=To>>8;}
   }
   return (((KnightDest[kingsq]&Knights)|
       (GenRook(kingsq,newoccupation)&(Rooks|Queens))|
       (GenBishop(kingsq,newoccupation)&(Bishops|Queens))|
       ((((king<<9)&0xFEFEFEFEFEFEFEFEULL)|((king<<7)&0x7F7F7F7F7F7F7F7FULL))&Pawns)|
       (KingDest[kingsq]&Kings))&newopposing);
}

/* Generate all pseudo-legal quiet moves */
static inline TMove *GenerateQuiets(TMove *const quiets)
{
   TBB occupation,opposing;
   occupation = Occupation;
   opposing = occupation^Position->PM;

   TMove *pquiets=quiets;
   for (TPieceType piece=KING;piece>=KNIGHT;piece--) // generate moves from king to knight
   {
      // generate moves for every piece of the same type of the side to move
      for (TBB pieces=BBPieces(piece)&Position->PM;pieces;pieces=ClearLSB(pieces))
      {
         uint64_t sq = LSB(pieces);
         // for every destinations on a free square generate a move
         for (TBB destinations = ~occupation&BBDestinations(piece,sq,occupation);destinations;destinations=ClearLSB(destinations))
         {
            pquiets->MoveType = piece;
            pquiets->From = sq;
            pquiets->To = LSB(destinations);
            pquiets->Prom = EMPTY;
            pquiets++;
         }
      }
   }

   /* one pawns push */
   TBB push1 = (((Pawns&Position->PM)<<8) & ~occupation)&0x00FFFFFFFFFFFFFFULL;
   for (TBB pieces=push1;pieces;pieces=ClearLSB(pieces))
   {
      pquiets->MoveType = PAWN;
      pquiets->From = LSB(pieces)-8;
      pquiets->To = LSB(pieces);
      pquiets->Prom = EMPTY;
      pquiets++;
   }

   /* double pawns pushes */
   for (TBB push2 = (push1<<8) & ~occupation & 0x00000000FF000000ULL;push2;push2=ClearLSB(push2))
   {
      pquiets->MoveType = PAWN;
      pquiets->From = LSB(push2)-16;
      pquiets->To = LSB(push2);
      pquiets->Prom = EMPTY;
      pquiets++;
   }

   /* check if long castling is possible */
   if (CastleLM && !(occupation & 0x0EULL))
   {
      TBB roo,bis;
      roo = ExtractLSB(0x1010101010101000ULL&occupation); /* column e */
      roo |= ExtractLSB(0x0808080808080800ULL&occupation); /*column d */
      roo |= ExtractLSB(0x0404040404040400ULL&occupation); /*column c */
      roo |= ExtractLSB(0x00000000000000E0ULL&occupation);  /* row 1 */
      bis = ExtractLSB(0x0000000102040800ULL&occupation); /*antidiag from e1/e8 */
      bis |= ExtractLSB(0x0000000001020400ULL&occupation); /*antidiag from d1/d8 */
      bis |= ExtractLSB(0x0000000000010200ULL&occupation); /*antidiag from c1/c8 */
      bis |= ExtractLSB(0x0000000080402000ULL&occupation); /*diag from e1/e8 */
      bis |= ExtractLSB(0x0000008040201000ULL&occupation); /*diag from d1/d8 */
      bis |= ExtractLSB(0x0000804020100800ULL&occupation); /*diag from c1/c8 */
      if (!(((roo&(Rooks|Queens)) | (bis&(Bishops|Queens)) | (0x00000000003E7700ULL&Knights)|
            (0x0000000000003E00ULL&Pawns) | (Kings&0x0000000000000600ULL))&opposing))
      {  /* check if c1/c8 d1/d8 e1/e8 are not attacked */
         pquiets->MoveType = KING|CASTLE;
         pquiets->From = 4;
         pquiets->To = 2;
         pquiets->Prom = EMPTY;
         pquiets++;
      }
   }
   /* check if short castling is possible */
   if (CastleSM && !(occupation & 0x60ULL))
   {
      TBB roo,bis;
      roo = ExtractLSB(0x1010101010101000ULL&occupation); /* column e */
      roo |= ExtractLSB(0x2020202020202000ULL&occupation); /* column f */
      roo |= ExtractLSB(0x4040404040404000ULL&occupation); /* column g */
      roo |= 1ULL<<MSB(0x000000000000000FULL&(occupation|0x1ULL));/* row 1 */
      bis = ExtractLSB(0x0000000102040800ULL&occupation); /* antidiag from e1/e8 */
      bis |= ExtractLSB(0x0000010204081000ULL&occupation); /*antidiag from f1/f8 */
      bis |= ExtractLSB(0x0001020408102000ULL&occupation); /*antidiag from g1/g8 */
      bis |= ExtractLSB(0x0000000080402000ULL&occupation); /*diag from e1/e8 */
      bis |= ExtractLSB(0x0000000000804000ULL&occupation); /*diag from f1/f8 */
      bis |= 0x0000000000008000ULL; /*diag from g1/g8 */
      if (!(((roo&(Rooks|Queens)) | (bis&(Bishops|Queens)) | (0x0000000000F8DC00ULL&Knights)|
           (0x000000000000F800ULL&Pawns) | (Kings&0x0000000000004000ULL))&opposing))
      {  /* check if e1/e8 f1/f8 g1/g8 are not attacked */
         pquiets->MoveType = KING|CASTLE;
         pquiets->From = 4;
         pquiets->To = 6;
         pquiets->Prom = EMPTY;
         pquiets++;
      }
   }
   return pquiets;
}

/* Generate all pseudo-legal capture and promotions */
static inline TMoveEval *GenerateCapture(TMoveEval *const capture)
{
   TBB opposing,occupation;
   occupation = Occupation;
   opposing = Position->PM ^ occupation;

   TMoveEval *pcapture=capture;
   for (TPieceType piece=KING;piece>=KNIGHT;piece--) // generate moves from king to knight
   {
      // generate moves for every piece of the same type of the side to move
      for (TBB pieces=BBPieces(piece)&Position->PM;pieces;pieces=ClearLSB(pieces))
      {
         uint64_t sq = LSB(pieces);
         // for every destinations on an opponent pieces generate a move
         for (TBB destinations = opposing&BBDestinations(piece,sq,occupation);destinations;destinations=ClearLSB(destinations))
         {
            pcapture->Move.MoveType = piece|CAPTURE;
            pcapture->Move.From = sq;
            pcapture->Move.To = LSB(destinations);
            pcapture->Move.Prom = EMPTY;
            pcapture->Eval = (Piece(LSB(destinations))<<4)|(KING-piece);
            pcapture++;
         }
      }
   }

   /* Generate pawns right captures */
   TBB pieces = Pawns&Position->PM;
   for (TBB captureri = (pieces<<9) & 0x00FEFEFEFEFEFEFEULL & opposing;captureri;captureri = ClearLSB(captureri))
   {
      pcapture->Move.MoveType = PAWN|CAPTURE;
      pcapture->Move.From = LSB(captureri)-9;
      pcapture->Move.To = LSB(captureri);
      pcapture->Move.Prom = EMPTY;
      pcapture->Eval = (Piece(LSB(captureri))<<4)|(KING-PAWN);
      pcapture++;
   }
   /* Generate pawns left captures */
   for (TBB capturele = (pieces<<7) & 0x007F7F7F7F7F7F7FULL & opposing;capturele;capturele = ClearLSB(capturele))
   {
      pcapture->Move.MoveType = PAWN|CAPTURE;
      pcapture->Move.From = LSB(capturele)-7;
      pcapture->Move.To = LSB(capturele);
      pcapture->Move.Prom = EMPTY;
      pcapture->Eval = (Piece(LSB(capturele))<<4)|(KING-PAWN);
      pcapture++;
   }

   /* Generate pawns promotions */
   if (pieces&0x00FF000000000000ULL)
   {
      /* promotions with left capture */
      for (TBB promo = (pieces<<9) & 0xFE00000000000000ULL & opposing;promo;promo = ClearLSB(promo))
      {
         TMove move;
         move.MoveType = PAWN|PROMO|CAPTURE;
         move.From = LSB(promo)-9;
         move.To = LSB(promo);
         move.Prom = QUEEN;
         pcapture->Move = move;
         pcapture->Eval = (QUEEN<<4)|(KING-PAWN);
         pcapture++;
         for (TPieceType piece=ROOK;piece>=KNIGHT;piece--) /* generate underpromotions */
         {
            move.Prom = piece;
            pcapture->Move = move;
            pcapture->Eval = piece-ROOK-1; /* keep behind the other captures-promotions */
            pcapture++;
         }
      }
      /* promotions with right capture */
      for (TBB promo = (pieces<<7) & 0x7F00000000000000ULL & opposing;promo;promo = ClearLSB(promo))
      {
         TMove move;
         move.MoveType = PAWN|PROMO|CAPTURE;
         move.From = LSB(promo)-7;
         move.To = LSB(promo);
         move.Prom = QUEEN;
         pcapture->Move = move;
         pcapture->Eval = (QUEEN<<4)|(KING-PAWN);
         pcapture++;
         for (TPieceType piece=ROOK;piece>=KNIGHT;piece--) /* generate underpromotions */
         {
            move.Prom = piece;
            pcapture->Move = move;
            pcapture->Eval = piece-ROOK-1; /* keep behind the other captures-promotions */
            pcapture++;
         }
      }
      /* no capture promotions */
      for (TBB promo = ((pieces<<8) & ~occupation)&0xFF00000000000000ULL;promo;promo = ClearLSB(promo))
      {
         TMove move;
         move.MoveType = PAWN|PROMO;
         move.From = LSB(promo)-8;
         move.To = LSB(promo);
         move.Prom = QUEEN;
         pcapture->Move = move;
         pcapture->Eval = (QUEEN<<4)|(KING-PAWN);
         pcapture++;
         for (TPieceType piece=ROOK;piece>=KNIGHT;piece--) /* generate underpromotions */
         {
            move.Prom = piece;
            pcapture->Move = move;
            pcapture->Eval = piece-ROOK-1; /* keep behind the other captures-promotions */
            pcapture++;
         }
      }
   }

   if (Position->EnPassant!=8)
   {  /* Generate EnPassant captures */
      for (TBB enpassant = pieces&EnPassant[Position->EnPassant]; enpassant; enpassant=ClearLSB(enpassant))
      {
         pcapture->Move.MoveType = PAWN|EP|CAPTURE;
         pcapture->Move.From = LSB(enpassant);
         pcapture->Move.To = 40+Position->EnPassant;
         pcapture->Move.Prom = EMPTY;
         pcapture->Eval = (PAWN<<4)|(KING-PAWN);
         pcapture++;
      }
   }
   return pcapture;
}

/* Check if the current position is a repetition or not, and set Rep field */
static inline void UpdateRepetition(void)
{
   TBoard *limite50mosse = Position - Position->Count50, *posuguale = Position - 2;
   while (posuguale >= limite50mosse && (posuguale->PM != Position->PM || posuguale->P0 != Position->P0 ||
          posuguale->P1 != Position->P1 || posuguale->P2 != Position->P2)) posuguale-=2;
   if (posuguale>=limite50mosse) Position->Rep = 1;/* this position is a repetition */
}

/* Make the move */
static inline void Make(TMove move)
{
   Position++;
   *Position=*(Position-1); /* copy the previous position into the last one */
   Position->Rep=0;
   TBB part = 1ULL<<move.From;
   TBB dest = 1ULL<<move.To;
   switch (move.MoveType&0x07)
   {
      case PAWN:
         if (move.MoveType&EP)
         {  /* EnPassant */
            Position->PM^=part|dest;
            Position->P0^=part|dest;
            Position->P0^=dest>>8; /* delete the captured pawn */
            Position->EnPassant=8;
         }
         else
         {
            if (move.MoveType&CAPTURE)
            {  /* Delete the captured piece */
               Position->P0&=~dest;
               Position->P1&=~dest;
               Position->P2&=~dest;
            }
            if (move.MoveType&PROMO)
            {
               Position->PM^=part|dest;
               Position->P0^=part;
               Position->P0|=(TBB)(move.Prom&1)<<(move.To);
               Position->P1|=(TBB)(((move.Prom)>>1)&1)<<(move.To);
               Position->P2|=(TBB)((move.Prom)>>2)<<(move.To);
               Position->EnPassant=8; /* clear enpassant */
            }
            else /* capture or push */
            {
               Position->PM^=part|dest;
               Position->P0^=part|dest;
               Position->EnPassant=8; /* clear enpassant */
               if (move.To==move.From+16 && EnPassantM[move.To&0x07]&Pawns&(Position->PM^(Occupation)))
                  Position->EnPassant=move.To&0x07; /* save enpassant column */
            }
            if (move.MoveType&CAPTURE)
            {
               if (CastleSO && move.To==63) ResetCastleSO; /* captured the opponent king side rook */
               else if (CastleLO && move.To==56) ResetCastleLO; /* captured the opponent quuen side rook */
            }
         }
         Position->Count50=0;
         ChangeSide;
         break;
      case KNIGHT:
      case BISHOP:
      case ROOK:
      case QUEEN:
         if (move.MoveType&CAPTURE)
         {
            Position->P0&=~dest;
            Position->P1&=~dest;
            Position->P2&=~dest;
         }
         Position->PM^=part|dest;
         Position->P0^=(move.MoveType&1) ? part|dest : 0;
         Position->P1^=(move.MoveType&2) ? part|dest : 0;
         Position->P2^=(move.MoveType&4) ? part|dest : 0;
         Position->EnPassant=8;
         if ((move.MoveType&0x7)==ROOK) /* update the castle rights */
         {
            if (CastleSM && move.From==7) ResetCastleSM;
            else if (CastleLM && move.From==0) ResetCastleLM;
         }
         if (move.MoveType&CAPTURE) /* update the castle rights */
         {
            if (CastleSO && move.To==63) ResetCastleSO;
            else if (CastleLO && move.To==56) ResetCastleLO;
            Position->Count50=0;
         }
         ChangeSide;
         if (!(move.MoveType&CAPTURE))
         {
            Position->Count50++;
            UpdateRepetition();
         }
         else Position->Count50=0;
         break;
      case KING:
         if (move.MoveType&CAPTURE)
         {
            Position->P0&=~dest;
            Position->P1&=~dest;
            Position->P2&=~dest;
         }
         Position->PM^=part|dest;
         Position->P1^=part|dest;
         Position->P2^=part|dest;
         if (CastleSM) ResetCastleSM; /* update the castle rights */
         if (CastleLM) ResetCastleLM;
         Position->EnPassant=8;
         if (move.MoveType&CAPTURE)
         {
            if (CastleSO && move.To==63) ResetCastleSO;
            else if (CastleLO && move.To==56) ResetCastleLO;
            Position->Count50=0;
         }
         else if (move.MoveType&CASTLE)
         {
            if (move.To==6) { Position->PM^=0x00000000000000A0ULL; Position->P2^=0x00000000000000A0ULL; } /* short castling */
            else { Position->P2^=0x0000000000000009ULL; Position->PM^=0x0000000000000009ULL; } /* long castling */
         }
         ChangeSide;
         if (!(move.MoveType&CAPTURE))
         {
            Position->Count50++;
            UpdateRepetition();
         }
         else Position->Count50=0;
      default: break;
   }
}

/* Evaluate the leaf positions */
static inline int Evaluate(void)
{
   int eval=0;
   int gamephase=PopCount(Knights|Bishops|Rooks|Queens|Kings);
   for (int sides=0;sides<2;sides++) /* evaluate the 2 sides */
   {
      /* for every piece sum the static value and the pst value */
      for (TPieceType piece=PAWN;piece<=QUEEN;piece++)
         for (TBB pieces=BBPieces(piece)&Position->PM;pieces;pieces = ClearLSB(pieces))
            eval += PST[piece][LSB(pieces)]+StaticValue[piece];
      /* interpolate the 2 pst of the king */
      uint64_t kingsq = LSB(Kings&Position->PM);
      eval += (PST[KING][kingsq]*gamephase+PST[KING+1][kingsq]*(16-gamephase))/16;
      ChangeSide;
      eval = -eval;
   }
   return eval;
}

/* number of ply that the search can reach */
#define PLY 6

static int AlphaBeta(int ply,int alpha,int beta)
{
   Nodes++;
   /* check for a draw */
   if (Position->Rep) return 0; /* if it's a repetition */
   if (Position->Count50 >= 100) /* fifty rule move */
   {
      if (!InCheck()) return 0; // it's a draw
      TMoveEval capture[64];
      for (TMoveEval *pcapture = GenerateCapture(capture);pcapture>capture;pcapture--)
      {
         TMove move = (pcapture-1)->Move;
         if (!Illegal(move)) return 0; /* if there is a legal move it's a draw */
      }
      TMove quiets[256];
      for (TMove *pquiets=GenerateQuiets(quiets);pquiets>quiets;pquiets--)
      {
         TMove move = *(pquiets-1);
         if (!Illegal(move)) return 0; /* if there is a legal move it's a draw */
      }
      // if here there are no legal move return a checkmate
      return -MATE+ply;
   }
   // if insufficient mating material it's a draw
   if (!(Pawns|Rooks|Queens|ClearLSB(Knights|Bishops))) return 0;

   if (ply==PLY+1) return Evaluate();

   int legalmoves=0;
   /* Generate pseudo-legal capture and promotions first */
   TMoveEval capture[64];
   for (TMoveEval *pcapture = GenerateCapture(capture);pcapture>capture;pcapture--)
   {
      TMove move = ExtractBest(capture,pcapture);
      if (Illegal(move)) continue; /* if it's an illegal move go on */
      Make(move);
      int eval = -AlphaBeta(ply+1,-beta,-alpha);
      Position--; /* unmake the move bringing the previous position */
      legalmoves++; /* increment the number of legal moves found */
      if (eval>alpha) alpha = eval;
      if (alpha >= beta) goto exit;
   }
   /* Now generate all legal quiet moves */
   TMove quiets[256];
   for (TMove *pquiets=GenerateQuiets(quiets);pquiets>quiets;pquiets--)
   {
      TMove move = *(pquiets-1);
      if (Illegal(move)) continue; /* if it's an illegal move go on */
      Make(move);
      int eval = -AlphaBeta(ply+1,-beta,-alpha);
      Position--; /* unmake the move bringing the previous position */
      legalmoves++; /* increment the number of legal moves found */
      if (eval>alpha) alpha = eval;
      if (alpha >= beta) goto exit;
   }

   /* if no legal move found it's checkmate or stalemate */
   if (!legalmoves)  alpha = InCheck() ? -MATE+ply : 0;

   exit:
   return alpha;
}

/* Search all the moves at the root and send to the GUI the best one */
static void RootSearch(void)
{
   TMove move,bestmove;
   char strmove[8];
   struct timespec begin,end;
   clock_gettime(CLOCK_MONOTONIC,&begin);

   bestmove.Move=0;
   Nodes=0;
   int eval;
   int alpha = -MATE, beta = MATE;
   /* Generate captures and promotions */
   TMoveEval capture[64];
   for (TMoveEval *pcapture=GenerateCapture(capture);pcapture>capture;pcapture--)
   {
      move = ExtractBest(capture,pcapture);
      if (Illegal(move)) continue;
      Make(move);
      eval = -AlphaBeta(1,-beta,-alpha);
      Position--;
      if (eval>alpha) { alpha = eval; bestmove = move; } /* save the new alpha and the new bestmove */
   }
   /* Generete quiet moves */
   TMove quiets[256];
   for (TMove *pquiets = GenerateQuiets(quiets);pquiets>quiets;pquiets--)
   {
      move = *(pquiets-1);
      if (Illegal(move)) continue;
      Make(move);
      eval = -AlphaBeta(1,-beta,-alpha);
      Position--;
      if (eval>alpha) { alpha = eval; bestmove=move; }
   }

   clock_gettime(CLOCK_MONOTONIC,&end);
   if (begin.tv_nsec>end.tv_nsec) { end.tv_nsec+=1000000000; end.tv_sec--; }
   end.tv_nsec-=begin.tv_nsec; /* compute the time spent by the search */
   end.tv_sec-=begin.tv_sec;

   MoveToStr(strmove,bestmove,Position->STM);
   printf("info depth %2d time %"PRId64" nodes %"PRId64" score ", PLY, (int64_t) (end.tv_sec*1000 + end.tv_nsec/1000000),Nodes);
   /* UCI uses a special format for the mate */
   if (abs(alpha)<MATE-PLY) printf("cp %d ",alpha);
   else printf("mate %d ",alpha>0 ? (MATE-alpha+1)/2 : (-MATE-alpha)/2);
   printf("pv %s\n",strmove);
   printf("bestmove %s\n",strmove);
   fflush(stdout);
}

/*
Load a position starting from a fen and a list of moves.
This function doesn't check the correctness of the fen and the moves sent.
*/
static void LoadPosition(const char *fen, char *moves)
{
   /* Clear the board */
   Position=Game;
   Position->P0 = Position->P1 = Position->P2 = Position->PM = 0;
   Position->EnPassant=8;
   Position->STM=WHITE;
   Position->Rep=0;
   Position->Count50=0;
   Position->CastleFlags=0;

   /* translate the fen to the relative position */
   uint8_t pieceside=WHITE;
   uint8_t piece=PAWN;
   uint8_t sidetomove=WHITE;
   uint64_t square=0;
   const char *cursor;
   for (cursor=fen;*cursor!=' ';cursor++)
   {
      if (*cursor>='1' && *cursor<='8') square += *cursor - '0';
      else if (*cursor=='/') continue;
      else
      {
         uint64_t pos = OppSq(square);
         if (*cursor=='p') { piece = PAWN; pieceside = BLACK; }
         else if (*cursor=='n') { piece = KNIGHT; pieceside = BLACK; }
         else if (*cursor=='b') { piece = BISHOP; pieceside = BLACK; }
         else if (*cursor=='r') { piece = ROOK; pieceside = BLACK; }
         else if (*cursor=='q') { piece = QUEEN; pieceside = BLACK; }
         else if (*cursor=='k') { piece = KING; pieceside = BLACK; }
         else if (*cursor=='P') { piece = PAWN; pieceside = WHITE; }
         else if (*cursor=='N') { piece = KNIGHT; pieceside = WHITE; }
         else if (*cursor=='B') { piece = BISHOP; pieceside = WHITE; }
         else if (*cursor=='R') { piece = ROOK; pieceside = WHITE; }
         else if (*cursor=='Q') { piece = QUEEN; pieceside = WHITE; }
         else if (*cursor=='K') { piece = KING; pieceside = WHITE; }
         Position->P0|=((uint64_t)piece&1)<<pos;
         Position->P1|=((uint64_t)(piece>>1)&1)<<pos;
         Position->P2|=((uint64_t)piece>>2)<<pos;
         if (pieceside==WHITE) { Position->PM |= 1ULL<<pos; piece|=BLACK; }
         square++;
      }
   }
   cursor++; /* read the side to move  */
   if (*cursor=='w') sidetomove=WHITE;
   else if (*cursor=='b') sidetomove=BLACK;
   cursor+=2;
   if (*cursor!='-') /* read the castle rights */
   {
      for (;*cursor!=' ';cursor++)
      {
         if (*cursor=='K') Position->CastleFlags|=0x02;
         else if (*cursor=='Q') Position->CastleFlags|=0x01;
         else if (*cursor=='k') Position->CastleFlags|=0x20;
         else if (*cursor=='q') Position->CastleFlags|=0x10;
      }
      cursor++;
   }
   else cursor+=2;
   if (*cursor!='-') /* read the enpassant column */
   {
      Position->EnPassant= *cursor - 'a';
      cursor++;
   }
   else cursor+=2;
   char counter50moves[4];
   char *pcounter;
   for (pcounter=counter50moves;*cursor!=' ';cursor++,pcounter++) *pcounter = *cursor; /* copy the string */
   *pcounter='\0';
   Position->Count50 = atoi(counter50moves); /* convert the string counter to integer */
   if (sidetomove==BLACK) ChangeSide;

   if (moves!=0) /* make all the moves */
   {
      char *strmove;
      for (strmove=strtok(moves," "); strmove; strmove = strtok(NULL," "))
      {
         TMove move = StrToMove(strmove);
         Make(move);
         if (Position->Count50==0)
         {  /* if the counter is zeroed we can put this position in the beginnig of the array because there can't be repetitions before */
            Game[0]=*Position;
            Position=Game;
         }
      }
   }
}

/* Check the correctness of the move generator with the Perft function */
static int64_t Perft(int depth)
{
   TMove quiets[256];
   TMoveEval capture[64];
   TMove move;
   move.Move=0;

   int64_t tot = 0;

   for (TMoveEval *pcapture=GenerateCapture(capture);pcapture>capture;pcapture--)
   {
      move = (pcapture-1)->Move;
      if (Illegal(move)) continue;
      if (depth>1)
      {
         Make(move);
         tot+=Perft(depth-1);
         Position--;
      }
      else tot++;
   }
   for (TMove *pquiets = GenerateQuiets(quiets);pquiets>quiets;pquiets--)
   {
      move = *(pquiets-1);
      if (Illegal(move)) continue;
      if (depth>1)
      {
         Make(move);
         tot+=Perft(depth-1);
         Position--;
      }
      else tot++;
   }
   return tot;
}

/* Run the Perft with this 6 test positions */
static void TestPerft(void)
{
   struct{
   char fen[200];
   int depth;
   int64_t count;
   }Test[] = { {"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",6,119060324},
             {"r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",5,193690690},
             {"8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1", 7,178633661},
             {"r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1",6,706045033},
             {"rnbqkb1r/pp1p1ppp/2p5/4P3/2B5/8/PPP1NnPP/RNBQK2R w KQkq - 0 6",3,53392},
             {"r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10",5,164075551}};

   for (unsigned int i=0;i<(sizeof Test) / (sizeof (Test[0])); i++)
   {
      LoadPosition(Test[i].fen,"");
      printf("%s%"PRId64"%s%"PRId64"%s","Right count: ",Test[i].count," computed count: ",Perft(Test[i].depth),"\n");
   }
   exit(0);
}

char STARTPOS[]="rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

int main (int argc, char *argv[])
{
   char Command[8192];
   Command[0]=0;
   /* remove this comment to run the perft */
   //TestPerft();
   while(strncmp(Command,"quit",4))
   {
      fgets(Command, 8192, stdin);
      if (!strncmp(Command,"uci\n",4))
      {
         printf("id name QBBEngine\nid author Fabio Gobbato\n");
         printf("uciok\n");
         fflush(stdout);
      }
      else if (!strncmp(Command,"isready",7)) { printf("readyok\n"); fflush(stdout); }
      else if (!strncmp(Command,"position",8))
      {
         char *fen=STARTPOS;
         char *moves=strstr(&Command[18],"moves");
         if (moves) {*(moves-1)='\0'; moves+=6;}
         if (!strncmp(strtok(&Command[9]," "),"fen",3)) fen=&Command[13];
         LoadPosition(fen,moves);
      }
      else if (!strncmp(Command,"go",2)) RootSearch();
   }
   return 0;
}


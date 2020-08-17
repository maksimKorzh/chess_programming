/***************************************\

          Leaper pieces attacks
    
                   by
                  
            Code Moneky King
           
           
\**************************************/

// headers
#include <stdio.h>

// define bitboard type
#define U64 unsigned long long

// bit manipulation macros
#define get_bit(bitboard, index) (bitboard & (1ULL << index))
#define set_bit(bitboard, index) (bitboard |= (1ULL << index))
#define pop_bit(bitboard, index) (get_bit(bitboard, index) ? bitboard ^= (1ULL << index) : 0)

// define squares
enum {
    a8, b8, c8, d8, e8, f8, g8, h8,
    a7, b7, c7, d7, e7, f7, g7, h7,
    a6, b6, c6, d6, e6, f6, g6, h6,
    a5, b5, c5, d5, e5, f5, g5, h5,
    a4, b4, c4, d4, e4, f4, g4, h4,
    a3, b3, c3, d3, e3, f3, g3, h3,
    a2, b2, c2, d2, e2, f2, g2, h2,
    a1, b1, c1, d1, e1, f1, g1, h1
};

// define sides
enum { white, black };


// print bitboard
void print_bitboard(U64 bitboard)
{
    printf("\n");
    
    // loop over board ranks
    for (int rank = 0; rank < 8; rank++)
    {
        // loop over board files
        for (int file = 0; file < 8; file++)
        {
            // init square
            int square = rank * 8 + file;
            
            // print ranks
            if (!file)
                printf("  %d ", 8 - rank);
            
            // print bit at the current square
            printf(" %d", get_bit(bitboard, square) ? 1 : 0);
        }
        
        printf("\n");
    }
    
    // print files
    printf("\n     a b c d e f g h\n\n");
    
    // print bitboard as decimal number
    printf("     bitboard: %llud\n\n", bitboard);
}

// not A file bitboard

/*
  8  0 1 1 1 1 1 1 1
  7  0 1 1 1 1 1 1 1
  6  0 1 1 1 1 1 1 1
  5  0 1 1 1 1 1 1 1
  4  0 1 1 1 1 1 1 1
  3  0 1 1 1 1 1 1 1
  2  0 1 1 1 1 1 1 1
  1  0 1 1 1 1 1 1 1
     
     a b c d e f g h
*/

U64 not_a_file = 18374403900871474942ULL;

// not H file bitboard

/*
  8  1 1 1 1 1 1 1 0
  7  1 1 1 1 1 1 1 0
  6  1 1 1 1 1 1 1 0
  5  1 1 1 1 1 1 1 0
  4  1 1 1 1 1 1 1 0
  3  1 1 1 1 1 1 1 0
  2  1 1 1 1 1 1 1 0
  1  1 1 1 1 1 1 1 0

     a b c d e f g h
*/

U64 not_h_file = 9187201950435737471ULL;

// not HG file

/*
  8  1 1 1 1 1 1 0 0
  7  1 1 1 1 1 1 0 0
  6  1 1 1 1 1 1 0 0
  5  1 1 1 1 1 1 0 0
  4  1 1 1 1 1 1 0 0
  3  1 1 1 1 1 1 0 0
  2  1 1 1 1 1 1 0 0
  1  1 1 1 1 1 1 0 0

     a b c d e f g h
*/

U64 not_hg_file = 4557430888798830399ULL;

// not AB file

/*
  8  0 0 1 1 1 1 1 1
  7  0 0 1 1 1 1 1 1
  6  0 0 1 1 1 1 1 1
  5  0 0 1 1 1 1 1 1
  4  0 0 1 1 1 1 1 1
  3  0 0 1 1 1 1 1 1
  2  0 0 1 1 1 1 1 1
  1  0 0 1 1 1 1 1 1

     a b c d e f g h
*/

U64 not_ab_file = 18229723555195321596ULL;


// pawn attacks array [side][square]
U64 pawn_attacks[2][64];

// knight attacks array [square]
U64 knight_attacks[64];

// king attacks array [square]
U64 king_attacks[64];

// mask pawn attacks
U64 mask_pawn_attacks(int side, int square)
{
    // attack bitboard
    U64 attacks = 0;
    
    // piece bitboard
    U64 bitboard = 0;
    
    // set piece on bitboard
    set_bit(bitboard, square);
    
    // white pawn attacks
    if (!side)
    {
        // make sure attack is on board
        if ((bitboard >> 7) & not_a_file) attacks |= (bitboard >> 7);
        if((bitboard >> 9) & not_h_file) attacks |= (bitboard >> 9);
    }
    
    // black pawn atacks
    else
    {
        // make sure attack is on board
        if ((bitboard << 7) & not_h_file) attacks |= (bitboard << 7);
        if((bitboard << 9) & not_a_file) attacks |= (bitboard << 9);
    }
    
    // return attack map for pawn on a given square
    return attacks;  
}

// mask knight attacks
U64 mask_knight_attacks(int square)
{
    // attack bitboard
    U64 attacks = 0;
    
    // piece bitboard
    U64 bitboard = 0;
    
    // set piece on bitboard
    set_bit(bitboard, square);
    
    // generate knight
    if ((bitboard >> 17) & not_h_file) attacks |= (bitboard >> 17);
    if ((bitboard >> 15) & not_a_file) attacks |= (bitboard >> 15);
    if ((bitboard >> 10) & not_hg_file) attacks |= (bitboard >> 10);
    if ((bitboard >> 6) & not_ab_file) attacks |= (bitboard >> 6);
    if ((bitboard << 17) & not_a_file) attacks |= (bitboard << 17);
    if ((bitboard << 15) & not_h_file) attacks |= (bitboard << 15);
    if ((bitboard << 10) & not_ab_file) attacks |= (bitboard << 10);
    if ((bitboard << 6) & not_hg_file) attacks |= (bitboard << 6);
    
    // return attack map for knight on a given square
    return attacks;
}

// mask king attacks
U64 mask_king_attacks(int square)
{
    // attack bitboard
    U64 attacks = 0;
    
    // piece bitboard
    U64 bitboard = 0;
    
    // set piece on bitboard
    set_bit(bitboard, square);
    
    // generate king attacks
    if (bitboard >> 8) attacks |= (bitboard >> 8);
    if (bitboard << 8) attacks |= (bitboard << 8);
    if ((bitboard >> 1) & not_h_file) attacks |= (bitboard >> 1);
    if ((bitboard >> 9) & not_h_file) attacks |= (bitboard >> 9);
    if ((bitboard >> 7) & not_a_file) attacks |= (bitboard >> 7);
    if ((bitboard << 1) & not_a_file) attacks |= (bitboard << 1);
    if ((bitboard << 9) & not_a_file) attacks |= (bitboard << 9);
    if ((bitboard << 7) & not_h_file) attacks |= (bitboard << 7);

    // return attack map for king on a given square
    return attacks;
}

// init pre-calculated attack tables for leaper pieces (pawns, knights, kings)
void init_leaper_attacks()
{
    // loop over 64 board squares
    for (int square = 0; square < 64; square++)
    {
        // init leaper attacks
        pawn_attacks[white][square] = mask_pawn_attacks(white, square);
        pawn_attacks[black][square] = mask_pawn_attacks(black, square);
        knight_attacks[square] = mask_knight_attacks(square);
        king_attacks[square] = mask_king_attacks(square);
    }
}

// main driver
int main()
{
    // init leaper attacks
    init_leaper_attacks();
    
    // print king attacks
    for (int square = 0; square < 64; square++)
        print_bitboard(king_attacks[square]);
    
    return 0;
}






























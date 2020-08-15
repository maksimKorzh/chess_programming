/***************************************\

             Print bitboard
    
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
    printf("     bitboard: %lld\n\n", bitboard);
}

// main driver
int main()
{
    // define white pawns bitboard
    U64 white_pawns = 71776119061217280ULL;
    
    /* set white pawn on a2 square
    set_bit(white_pawns, a2);
    
    // set white pawn on b2 square
    set_bit(white_pawns, b2);
    
    // set white pawn on c2 square
    set_bit(white_pawns, c2);
    
    // set white pawn on d2 square
    set_bit(white_pawns, d2);
    
    // set white pawn on e2 square
    set_bit(white_pawns, e2);
    
    // set white pawn on f2 square
    set_bit(white_pawns, f2);
    
    // set white pawn on g2 square
    set_bit(white_pawns, g2);
    
    // set white pawn on h2 square
    set_bit(white_pawns, h2);*/
    
    // print bitboard
    printf("\n\n  white pawns bitboard\n");
    print_bitboard(white_pawns);
        
    return 0;
}






























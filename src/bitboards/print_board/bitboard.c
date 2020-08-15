/***************************************\

            Print chess board
         represented as bitboards
    
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

// define pieces
enum { P, N, B, R, Q, K, p, n, b, r, q, k };

// define ascii pieces
char ascii_pieces[] = "PNBRQKpnbrqk";

// unicode pieces
char *unicode_pieces[] = {"♙", "♘", "♗", "♖", "♕", "♔", "♟︎", "♞", "♝", "♜", "♛", "♚"};

// define bitboards
U64 bitboards[12];

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

// print board
void print_board()
{
    printf("\n");

    // loop over board ranks
    for (int rank = 0; rank < 8; rank++)
    {
        for (int file = 0; file < 8; file++)
        {
            // init square
            int square = rank * 8 + file;
            
            // print ranks
            if (!file)
                printf("  %d ", 8 - rank);
            
            // init piece
            int piece = -1;
            
            // loop over bitboards
            for (int bb_piece = P; bb_piece <= k; bb_piece++)
            {
                // is there a piece on a square on the current bitboard?
                if (get_bit(bitboards[bb_piece], square))
                    piece = bb_piece;
            }
            
            // print piece
            //printf(" %c", (piece == -1) ? '.' : ascii_pieces[piece]);
            printf(" %s", (piece == -1) ? "." : unicode_pieces[piece]);
        }
        
        printf("\n");
    }
    
    // print files
    printf("\n     a b c d e f g h\n\n");
}

// main driver
int main()
{
    // init white pawns bitboard
    set_bit(bitboards[P], e2);
    set_bit(bitboards[P], d2);
    set_bit(bitboards[P], f3);
    
    // init white knights bitboard
    set_bit(bitboards[N], g1);
    set_bit(bitboards[N], b1);
    
    // init black king bitboard
    set_bit(bitboards[k], e8);
    
    // init white kine bitboard
    set_bit(bitboards[K], e1);
    
    // print bitboards
    print_bitboard(bitboards[P]);
    print_bitboard(bitboards[N]);
    print_bitboard(bitboards[k]);
    
    // print board
    print_board();
    
    return 0;
}






























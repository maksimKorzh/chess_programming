/***************************************\

    Bit manipulations within bitboard
    
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

//       bin      dec
// 0000 0001        1

// bit shift
// 0000 0001 << 1 = 0000 0010
// 0000 0001 << 4 = 0001 0000 


// main driver
int main()
{
    U64 bitboard = 0;
    
    set_bit(bitboard, 36);
    set_bit(bitboard, 28);
    
    printf("\nset bits at indicies 36, 28 on bitboard: %lld\n", bitboard);
    
    for (int index = 63; index >= 0; index--)
        printf("%d", get_bit(bitboard, index) ? 1 : 0);
    
    printf("\n");
    
    pop_bit(bitboard, 28);
    
    printf("\nremove index of 28 from bitboard: %lld\n", bitboard);
    
    for (int index = 63; index >= 0; index--)
        printf("%d", get_bit(bitboard, index) ? 1 : 0);
    
    printf("\n");
    
    return 0;
}









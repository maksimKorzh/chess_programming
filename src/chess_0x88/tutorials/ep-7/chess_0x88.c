/************************************************\

               Mailbox Chess Engine
                      (0x88)

                        by

                 Code Monkey King

\************************************************/

// headers
#include <stdio.h>

// FEN dedug positions
char start_position[] = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
char tricky_position[] = "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1";

// piece encoding
enum pieces {e, P, N, B, R, Q, K, p, n, b, r, q, k, o};

// square encoding
enum squares {
    a8 = 0,   b8, c8, d8, e8, f8, g8, h8,
    a7 = 16,  b7, c7, d7, e7, f7, g7, h7,
    a6 = 32,  b6, c6, d6, e6, f6, g6, h6,
    a5 = 48,  b5, c5, d5, e5, f5, g5, h5,
    a4 = 64,  b4, c4, d4, e4, f4, g4, h4,
    a3 = 80,  b3, c3, d3, e3, f3, g3, h3,
    a2 = 96,  b2, c2, d2, e2, f2, g2, h2,
    a1 = 112, b1, c1, d1, e1, f1, g1, h1, no_sq
};

// castling binary representation
//
//  bin  dec
// 0001    1  white king can castle to the king side
// 0010    2  white king can castle to the queen side
// 0100    4  black king can castle to the king side
// 1000    8  black king can castle to the queen side
//
// examples
// 1111       both sides an castle both directions
// 1001       black king => queen side
//            white king => king side

// castling writes
enum castling { KC = 1, QC = 2, kc = 4, qc = 8 };

// sides to move
enum sides { white, black };

// ascii pieces
char ascii_pieces[] = ".PNBRQKpnbrqk";

// unicode pieces
char *unicode_pieces[] = {".", "♙", "♘", "♗", "♖", "♕", "♔", "♟︎", "♞", "♝", "♜", "♛", "♚"};

// encode ascii pieces
int char_pieces[] = {
    ['P'] = P,
    ['N'] = N,
    ['B'] = B,
    ['R'] = R,
    ['Q'] = Q,
    ['K'] = K,
    ['p'] = p,
    ['n'] = n,
    ['b'] = b,
    ['r'] = r,
    ['q'] = q,
    ['k'] = k,
};

// chess board representation
int board[128] = {
    r, n, b, q, k, b, n, r,  o, o, o, o, o, o, o, o,
    p, p, p, p, p, p, p, p,  o, o, o, o, o, o, o, o,
    e, e, e, e, e, e, e, e,  o, o, o, o, o, o, o, o,
    e, e, e, e, e, e, e, e,  o, o, o, o, o, o, o, o,
    e, e, e, e, e, e, e, e,  o, o, o, o, o, o, o, o,
    e, e, e, e, e, e, e, e,  o, o, o, o, o, o, o, o,
    P, P, P, P, P, P, P, P,  o, o, o, o, o, o, o, o,
    R, N, B, Q, K, B, N, R,  o, o, o, o, o, o, o, o
};

// side to move
int side = white;

// enpassant square
int enpassant = no_sq;

// castling rights (dec 15 => bin 1111 => both kings can castle to both sides)
int castle = 15;

// convert board square indexes to coordinates
char *square_to_coords[] = {
    "a8", "b8", "c8", "d8", "e8", "f8", "g8", "h8", "i8", "j8", "k8", "l8", "m8", "n8", "o8", "p8",
    "a7", "b7", "c7", "d7", "e7", "f7", "g7", "h7", "i7", "j7", "k7", "l7", "m7", "n7", "o7", "p7",
    "a6", "b6", "c6", "d6", "e6", "f6", "g6", "h6", "i6", "j6", "k6", "l6", "m6", "n6", "o6", "p6",
    "a5", "b5", "c5", "d5", "e5", "f5", "g5", "h5", "i5", "j5", "k5", "l5", "m5", "n5", "o5", "p5",
    "a4", "b4", "c4", "d4", "e4", "f4", "g4", "h4", "i4", "j4", "k4", "l4", "m4", "n4", "o4", "p4",
    "a3", "b3", "c3", "d3", "e3", "f3", "g3", "h3", "i3", "j3", "k3", "l3", "m3", "n3", "o3", "p3",
    "a2", "b2", "c2", "d2", "e2", "f2", "g2", "h2", "i2", "j2", "k2", "l2", "m2", "n2", "o2", "p2",
    "a1", "b1", "c1", "d1", "e1", "f1", "g1", "h1", "i1", "j1", "k1", "l1", "m1", "n1", "o1", "p1"
};

// piece move offsets
int knight_offsets[8] = {33, 31, 18, 14, -33, -31, -18, -14};
int bishop_offsets[4] = {15, 17, -15, -17};
int rook_offsets[4] = {16, -16, 1, -1};
int king_offsets[8] = {16, -16, 1, -1, 15, 17, -15, -17};

// print board
void print_board()
{
    // print new line
    printf("\n");

    // loop over board ranks
    for (int rank = 0; rank < 8; rank++)
    {
        // loop over board files
        for (int file = 0; file < 16; file++)
        {
            // init square
            int square = rank * 16 + file;
            
            // print ranks
            if (file == 0)
                printf(" %d  ", 8 - rank);
            
            // if square is on board
            if (!(square & 0x88))
                //printf("%c ", ascii_pieces[board[square]]);
                printf("%s ", unicode_pieces[board[square]]);
        }
        
        // print new line every time new rank is encountered
        printf("\n");
    }
    
    // print files
    printf("\n    a b c d e f g h\n\n");
    
    // print board stats
    printf("    Side:     %s\n", (side == white) ? "white": "black");
    printf("    Castling:  %c%c%c%c\n", (castle & KC) ? 'K' : '-', 
                                    (castle & QC) ? 'Q' : '-',
                                    (castle & kc) ? 'k' : '-',
                                    (castle & qc) ? 'q' : '-');
    printf("    Enpassant:   %s\n\n", (enpassant == no_sq)? "no" : square_to_coords[enpassant]);
}

// reset board
void reset_board()
{
    // loop over board ranks
    for (int rank = 0; rank < 8; rank++)
    {
        // loop over board files
        for (int file = 0; file < 16; file++)
        {
            // init square
            int square = rank * 16 + file;
        
            // if square is on board
            if (!(square & 0x88))
                // reset current board square
                board[square] = e;
        }
    }
    
    // reset stats
    side = -1;
    castle = 0;
    enpassant = no_sq;
}

// parse FEN
void parse_fen(char *fen)
{
    // reset board
    reset_board();
    
    // loop over board ranks
    for (int rank = 0; rank < 8; rank++)
    {
        // loop over board files
        for (int file = 0; file < 16; file++)
        {
            // init square
            int square = rank * 16 + file;
        
            // if square is on board
            if (!(square & 0x88))
            {
                // match pieces
                if ((*fen >= 'a' && *fen <= 'z') || (*fen >= 'A' && *fen <= 'Z'))
                {
                    // set the piece on board
                    board[square] = char_pieces[*fen];
                    
                    // increment FEN pointer
                    *fen++;
                }
                
                // match empty squares
                if (*fen >= '0' && *fen <= '9')
                {
                    // calculate offset
                    int offset = *fen - '0';
                    
                    // decrement file on empty squares
                    if (!(board[square]))
                        file--;
                    
                    // skip empty squares
                    file += offset;
                    
                    // increment FEN pointer
                    *fen++;
                }
                
                // match end of rank
                if (*fen == '/')
                    // increment FEN pointer
                    *fen++;
                
            }
        }
    }
    
    // go to side parsing
    *fen++;
    
    // parse side to move
    side = (*fen == 'w') ? white : black;
    
    // go to castling rights parsing
    fen += 2;
    
    // parse castling rights
    while (*fen != ' ')
    {
        switch(*fen)
        {
            case 'K': castle |= KC; break;
            case 'Q': castle |= QC; break;
            case 'k': castle |= kc; break;
            case 'q': castle |= qc; break;
            case '-': break;
        }
        
        // increment pointer
        *fen++;
    }
    
    // got to empassant square
    *fen++;
    
    // parse empassant square
    if (*fen != '-')
    {
        // parse enpassant square's file & rank
        int file = fen[0] - 'a';
        int rank = 8 - (fen[1] - '0');
        
        // set up enpassant square
        enpassant = rank * 16 + file;
    }
    
    else
        enpassant = no_sq;   
}

// is square attacked
int is_square_attacked(int square, int side)
{
    // pawn attacks
    if (!side)
    {
        // if target square is on board and is white pawn
        if (!((square + 17) & 0x88) && (board[square + 17] == P))
            return 1;
        
        // if target square is on board and is white pawn
        if (!((square + 15) & 0x88) && (board[square + 15] == P))
            return 1;
    }
    
    else
    {
        // if target square is on board and is black pawn
        if (!((square - 17) & 0x88) && (board[square - 17] == p))
            return 1;
        
        // if target square is on board and is black pawn
        if (!((square - 15) & 0x88) && (board[square - 15] == p))
            return 1;
    }
    
    // knight attacks
    for (int index = 0; index < 8; index++)
    {
        // init target square
        int target_square = square + knight_offsets[index];
        
        // lookup target piece
        int target_piece = board[target_square];
        
        // if target square is on board
        if (!(target_square & 0x88))
        {
            if (!side ? target_piece == N : target_piece == n)
                return 1;
        } 
    }
    
    // king attacks
    for (int index = 0; index < 8; index++)
    {
        // init target square
        int target_square = square + king_offsets[index];
        
        // lookup target piece
        int target_piece = board[target_square];
        
        // if target square is on board
        if (!(target_square & 0x88))
        {
            // if target piece is either white or black king
            if (!side ? target_piece == K : target_piece == k)
                return 1;
        } 
    }
    
    // bishop & queen attacks
    for (int index = 0; index < 4; index++)
    {
        // init target square
        int target_square = square + bishop_offsets[index];
        
        // loop over attack ray
        while (!(target_square & 0x88))
        {
            // target piece
            int target_piece = board[target_square];
            
            // if target piece is either white or black bishop or queen
            if (!side ? (target_piece == B || target_piece == Q) : (target_piece == b || target_piece == q))
                return 1;

            // break if hit a piece
            if (target_piece)
                break;
        
            // increment target square by move offset
            target_square += bishop_offsets[index];
        }
    }
    
    // rook & queen attacks
    for (int index = 0; index < 4; index++)
    {
        // init target square
        int target_square = square + rook_offsets[index];
        
        // loop over attack ray
        while (!(target_square & 0x88))
        {
            // target piece
            int target_piece = board[target_square];
            
            // if target piece is either white or black bishop or queen
            if (!side ? (target_piece == R || target_piece == Q) : (target_piece == r || target_piece == q))
                return 1;

            // break if hit a piece
            if (target_piece)
                break;
        
            // increment target square by move offset
            target_square += rook_offsets[index];
        }
    }
    
    return 0;
}

// print board
void print_attacked_squares(int side)
{
    // print new line
    printf("\n");
    printf("    Attacking side: %s\n\n", !side ? "white" : "black");

    // loop over board ranks
    for (int rank = 0; rank < 8; rank++)
    {
        // loop over board files
        for (int file = 0; file < 16; file++)
        {
            // init square
            int square = rank * 16 + file;
            
            // print ranks
            if (file == 0)
                printf(" %d  ", 8 - rank);
            
            // if square is on board
            if (!(square & 0x88))
                printf("%c ", is_square_attacked(square, side) ? 'x' : '.');
                
        }
        
        // print new line every time new rank is encountered
        printf("\n");
    }
    
    printf("\n    a b c d e f g h\n\n");
}

// move generator
void generate_moves()
{
    // loop over all board squares
    for (int square = 0; square < 128; square++)
    {
        // check if the square is on board
        if (!(square & 0x88))
        {
            // white pawn and castling moves
            if (!side)
            {
                // white pawn moves
                if (board[square] == P)
                {
                    // init target square
                    int to_square = square - 16;
                    
                    // quite white pawn moves (check if target square is on board)
                    if (!(to_square & 0x88) && !board[to_square])
                    {   
                        // pawn promotions
                        if (square >= a7 && square <= h7)
                        {
                            printf("%s%sq\n", square_to_coords[square], square_to_coords[to_square]);
                            printf("%s%sr\n", square_to_coords[square], square_to_coords[to_square]);
                            printf("%s%sb\n", square_to_coords[square], square_to_coords[to_square]);
                            printf("%s%sn\n", square_to_coords[square], square_to_coords[to_square]);
                        }
                        
                        else
                        {
                            // one square ahead pawn move
                            printf("%s%s\n", square_to_coords[square], square_to_coords[to_square]);
                            
                            // two squares ahead pawn move
                            if ((square >= a2 && square <= h2) && !board[square - 32])
                                printf("%s%s\n", square_to_coords[square], square_to_coords[square - 32]);
                        }
                    }
                    
                    // white pawn capture moves
                    for (int index = 0; index < 4; index++)
                    {
                        // init pawn offset
                        int pawn_offset = bishop_offsets[index];
                        
                        // white pawn offsets
                        if (pawn_offset < 0)
                        {
                            // init target square
                            int to_square = square + pawn_offset;
                            
                            // check if target square is on board
                            if (!(to_square & 0x88))
                            {
                                // capture pawn promotion
                                if (
                                     (square >= a7 && square <= h7) &&
                                     (board[to_square] >= 7 && board[to_square] <= 12)
                                   )
                                {
                                    printf("%s%sq\n", square_to_coords[square], square_to_coords[to_square]);
                                    printf("%s%sr\n", square_to_coords[square], square_to_coords[to_square]);
                                    printf("%s%sb\n", square_to_coords[square], square_to_coords[to_square]);
                                    printf("%s%sn\n", square_to_coords[square], square_to_coords[to_square]);
                                }
                                
                                else
                                {
                                    // casual capture
                                    if (board[to_square] >= 7 && board[to_square] <= 12)
                                        printf("%s%s\n", square_to_coords[square], square_to_coords[to_square]);
                                    
                                    // enpassant capture
                                    if (to_square == enpassant)
                                        printf("%s%s\n", square_to_coords[square], square_to_coords[to_square]);
                                }
                            }
                        }
                    }
                }
            }
            
            // black pawn and castling moves
            else
            {
                // black pawn moves
                if (board[square] == p)
                {
                    // init target square
                    int to_square = square + 16;
                    
                    // quite black pawn moves (check if target square is on board)
                    if (!(to_square & 0x88) && !board[to_square])
                    {   
                        // pawn promotions
                        if (square >= a2 && square <= h2)
                        {
                            printf("%s%sq\n", square_to_coords[square], square_to_coords[to_square]);
                            printf("%s%sr\n", square_to_coords[square], square_to_coords[to_square]);
                            printf("%s%sb\n", square_to_coords[square], square_to_coords[to_square]);
                            printf("%s%sn\n", square_to_coords[square], square_to_coords[to_square]);
                        }
                        
                        else
                        {
                            // one square ahead pawn move
                            printf("%s%s\n", square_to_coords[square], square_to_coords[to_square]);
                            
                            // two squares ahead pawn move
                            if ((square >= a7 && square <= h7) && !board[square + 32])
                                printf("%s%s\n", square_to_coords[square], square_to_coords[square + 32]);
                        }
                    }
                    
                    // black pawn capture moves
                    for (int index = 0; index < 4; index++)
                    {
                        // init pawn offset
                        int pawn_offset = bishop_offsets[index];
                        
                        // white pawn offsets
                        if (pawn_offset > 0)
                        {
                            // init target square
                            int to_square = square + pawn_offset;
                            
                            // check if target square is on board
                            if (!(to_square & 0x88))
                            {
                                // capture pawn promotion
                                if (
                                     (square >= a2 && square <= h2) &&
                                     (board[to_square] >= 1 && board[to_square] <= 6)
                                   )
                                {
                                    printf("%s%sq\n", square_to_coords[square], square_to_coords[to_square]);
                                    printf("%s%sr\n", square_to_coords[square], square_to_coords[to_square]);
                                    printf("%s%sb\n", square_to_coords[square], square_to_coords[to_square]);
                                    printf("%s%sn\n", square_to_coords[square], square_to_coords[to_square]);
                                }
                                
                                else
                                {
                                    // casual capture
                                    if (board[to_square] >= 1 && board[to_square] <= 6)
                                        printf("%s%s\n", square_to_coords[square], square_to_coords[to_square]);
                                    
                                    // enpassant capture
                                    if (to_square == enpassant)
                                        printf("%s%s\n", square_to_coords[square], square_to_coords[to_square]);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

// main driver
int main()
{
    parse_fen("r3k2r/p1ppqpb1/bn2pnp1/3PN3/Pp2P3/2N2Q1p/1pPBBPPP/r3K2R b KQkq - 0 1");
    print_board();
    generate_moves();
    
    

    return 0;
}









/************************************************\

               Mailbox Chess Engine
                      (0x88)

                        by

                 Code Monkey King

\************************************************/

// headers
#include <stdio.h>
#include <string.h>

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

// decode promoted pieces
int promoted_pieces[] = {
    [Q] = 'q',
    [R] = 'r',
    [B] = 'b',
    [N] = 'n',
    [q] = 'q',
    [r] = 'r',
    [b] = 'b',
    [n] = 'n',
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

// kings' squares
int king_square[2] = {e1, e8};

/*
    Move formatting
    
    0000 0000 0000 0000 0111 1111       source square
    0000 0000 0011 1111 1000 0000       target square
    0000 0011 1100 0000 0000 0000       promoted piece
    0000 0100 0000 0000 0000 0000       capture flag
    0000 1000 0000 0000 0000 0000       double pawn flag
    0001 0000 0000 0000 0000 0000       enpassant flag
    0010 0000 0000 0000 0000 0000       castling

*/

// encode move
#define encode_move(source, target, piece, capture, pawn, enpassant, castling) \
(                          \
    (source) |             \
    (target << 7) |        \
    (piece << 14) |        \
    (capture << 18) |      \
    (pawn << 19) |         \
    (enpassant << 20) |    \
    (castling << 21)       \
)

// decode move's source square
#define get_move_source(move) (move & 0x7f)

// decode move's target square
#define get_move_target(move) ((move >> 7) & 0x7f)

// decode move's promoted piece
#define get_move_piece(move) ((move >> 14) & 0xf)

// decode move's capture flag
#define get_move_capture(move) ((move >> 18) & 0x1)

// decode move's double pawn push flag
#define get_move_pawn(move) ((move >> 19) & 0x1)

// decode move's enpassant flag
#define get_move_enpassant(move) ((move >> 20) & 0x1)

// decode move's castling flag
#define get_move_castling(move) ((move >> 21) & 0x1)

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

// move list structure
typedef struct {
    // move list
    int moves[256];
    
    // move count
    int count;
} moves;

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
    printf("    Enpassant:   %s\n", (enpassant == no_sq)? "no" : square_to_coords[enpassant]);
    printf("    King square: %s\n\n", square_to_coords[king_square[side]]);
}

// print move list
void print_move_list(moves *move_list)
{
    // print table header
    printf("\n    Move     Capture  Double   Enpass   Castling\n\n");

    // loop over moves in a movelist
    for (int index = 0; index < move_list->count; index++)
    {
        int move = move_list->moves[index];
        printf("    %s%s", square_to_coords[get_move_source(move)], square_to_coords[get_move_target(move)]);
        printf("%c    ", get_move_piece(move) ? promoted_pieces[get_move_piece(move)] : ' ');
        printf("%d        %d        %d        %d\n", get_move_capture(move), get_move_pawn(move), get_move_enpassant(move), get_move_castling(move));
    }
    
    printf("\n    Total moves: %d\n\n", move_list->count);
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
                    // set up kings' squares
                    if (*fen == 'K')
                        king_square[white] = square;
                    
                    else if (*fen == 'k')
                        king_square[black] = square;
                    
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

// print attack map
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

// populate move list
void add_move(moves *move_list, int move)
{
    // push move into the move list
    move_list->moves[move_list->count] = move;
    
    // increment move count
    move_list->count++;
}


// move generator
void generate_moves(moves *move_list)
{
    // reset move count
    move_list->count = 0;

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
                            add_move(move_list, encode_move(square, to_square, Q, 0, 0, 0, 0));
                            add_move(move_list, encode_move(square, to_square, R, 0, 0, 0, 0));
                            add_move(move_list, encode_move(square, to_square, B, 0, 0, 0, 0));
                            add_move(move_list, encode_move(square, to_square, N, 0, 0, 0, 0));                            
                        }
                        
                        else
                        {
                            // one square ahead pawn move
                            add_move(move_list, encode_move(square, to_square, 0, 0, 0, 0, 0));
                            
                            // two squares ahead pawn move
                            if ((square >= a2 && square <= h2) && !board[square - 32])
                                add_move(move_list, encode_move(square, to_square, 0, 0, 1, 0, 0));
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
                                    add_move(move_list, encode_move(square, to_square, Q, 1, 0, 0, 0));
                                    add_move(move_list, encode_move(square, to_square, R, 1, 0, 0, 0));
                                    add_move(move_list, encode_move(square, to_square, B, 1, 0, 0, 0));
                                    add_move(move_list, encode_move(square, to_square, N, 1, 0, 0, 0));
                                }
                                
                                else
                                {
                                    // casual capture
                                    if (board[to_square] >= 7 && board[to_square] <= 12)
                                        add_move(move_list, encode_move(square, to_square, 0, 1, 0, 0, 0));
                                    
                                    // enpassant capture
                                    if (to_square == enpassant)
                                        add_move(move_list, encode_move(square, to_square, 0, 1, 0, 1, 0));
                                }
                            }
                        }
                    }
                }
                
                // white king castling
                if (board[square] == K)
                {
                    // if king side castling is available
                    if (castle & KC)
                    {
                        // make sure there are empty squares between king & rook
                        if (!board[f1] && !board[g1])
                        {
                            // make sure king & next square are not under attack
                            if (!is_square_attacked(e1, black) && !is_square_attacked(f1, black))
                                add_move(move_list, encode_move(e1, g1, 0, 0, 0, 0, 1));
                        }
                    }
                    
                    // if queen side castling is available
                    if (castle & QC)
                    {
                        // make sure there are empty squares between king & rook
                        if (!board[d1] && !board[b1] && !board[c1])
                        {
                            // make sure king & next square are not under attack
                            if (!is_square_attacked(e1, black) && !is_square_attacked(d1, black))
                                add_move(move_list, encode_move(e1, c1, 0, 0, 0, 0, 1));
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
                            add_move(move_list, encode_move(square, to_square, q, 0, 0, 0, 0));
                            add_move(move_list, encode_move(square, to_square, r, 0, 0, 0, 0));
                            add_move(move_list, encode_move(square, to_square, b, 0, 0, 0, 0));
                            add_move(move_list, encode_move(square, to_square, n, 0, 0, 0, 0));
                        }
                        
                        else
                        {
                            // one square ahead pawn move
                            add_move(move_list, encode_move(square, to_square, 0, 0, 0, 0, 0));
                            
                            // two squares ahead pawn move
                            if ((square >= a7 && square <= h7) && !board[square + 32])
                                add_move(move_list, encode_move(square, to_square, 0, 0, 1, 0, 0));
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
                                    add_move(move_list, encode_move(square, to_square, q, 1, 0, 0, 0));
                                    add_move(move_list, encode_move(square, to_square, r, 1, 0, 0, 0));
                                    add_move(move_list, encode_move(square, to_square, b, 1, 0, 0, 0));
                                    add_move(move_list, encode_move(square, to_square, n, 1, 0, 0, 0));
                                }
                                
                                else
                                {
                                    // casual capture
                                    if (board[to_square] >= 1 && board[to_square] <= 6)
                                        add_move(move_list, encode_move(square, to_square, 0, 1, 0, 0, 0));
                                    
                                    // enpassant capture
                                    if (to_square == enpassant)
                                        add_move(move_list, encode_move(square, to_square, 0, 1, 0, 1, 0));
                                }
                            }
                        }
                    }
                }
                
                // black king castling
                if (board[square] == k)
                {
                    // if king side castling is available
                    if (castle & kc)
                    {
                        // make sure there are empty squares between king & rook
                        if (!board[f8] && !board[g8])
                        {
                            // make sure king & next square are not under attack
                            if (!is_square_attacked(e8, white) && !is_square_attacked(f8, white))
                                add_move(move_list, encode_move(e8, g8, 0, 0, 0, 0, 1));
                        }
                    }
                    
                    // if queen side castling is available
                    if (castle & qc)
                    {
                        // make sure there are empty squares between king & rook
                        if (!board[d8] && !board[b8] && !board[c8])
                        {
                            // make sure king & next square are not under attack
                            if (!is_square_attacked(e8, white) && !is_square_attacked(d8, white))
                                add_move(move_list, encode_move(e8, c8, 0, 0, 0, 0, 1));
                        }
                    }
                }
            }
            
            // knight moves
            if (!side ? board[square] == N : board[square] == n)
            {
                // loop over knight move offsets
                for (int index = 0; index < 8; index++)
                {
                    // init target square
                    int to_square = square + knight_offsets[index];
                    
                    // init target piece
                    int piece = board[to_square];
                    
                    // make sure target square is onboard
                    if (!(to_square & 0x88))
                    {
                        //
                        if (
                             !side ?
                             (!piece || (piece >= 7 && piece <= 12)) : 
                             (!piece || (piece >= 1 && piece <= 6))
                           )
                        {
                            // on capture
                            if (piece)
                                add_move(move_list, encode_move(square, to_square, 0, 1, 0, 0, 0));
                                
                            // on empty square
                            else
                                add_move(move_list, encode_move(square, to_square, 0, 0, 0, 0, 0));
                        }
                    }
                }
            }
            
            // king moves
            if (!side ? board[square] == K : board[square] == k)
            {
                // loop over king move offsets
                for (int index = 0; index < 8; index++)
                {
                    // init target square
                    int to_square = square + king_offsets[index];
                    
                    // init target piece
                    int piece = board[to_square];
                    
                    // make sure target square is onboard
                    if (!(to_square & 0x88))
                    {
                        //
                        if (
                             !side ?
                             (!piece || (piece >= 7 && piece <= 12)) : 
                             (!piece || (piece >= 1 && piece <= 6))
                           )
                        {
                            // on capture
                            if (piece)
                                add_move(move_list, encode_move(square, to_square, 0, 1, 0, 0, 0));
                                
                            // on empty square
                            else
                                add_move(move_list, encode_move(square, to_square, 0, 0, 0, 0, 0));
                        }
                    }
                }
            }
            
            // bishop & queen moves
            if (
                 !side ?
                 (board[square] == B) || (board[square] == Q) :
                 (board[square] == b) || (board[square] == q)
               )
            {
                // loop over bishop & queen offsets
                for (int index = 0; index < 4; index++)
                {
                    // init target square
                    int to_square = square + bishop_offsets[index];
                    
                    // loop over attack ray
                    while (!(to_square & 0x88))
                    {
                        // init target piece
                        int piece = board[to_square];
                        
                        // if hits own piece
                        if (!side ? (piece >= 1 && piece <= 6) : ((piece >= 7 && piece <= 12)))
                            break;
                        
                        // if hits opponent's piece
                        if (!side ? (piece >= 7 && piece <= 12) : ((piece >= 1 && piece <= 6)))
                        {
                            add_move(move_list, encode_move(square, to_square, 0, 1, 0, 0, 0));
                            break;
                        }
                        
                        // if steps into an empty squre
                        if (!piece)
                            add_move(move_list, encode_move(square, to_square, 0, 0, 0, 0, 0));
                        
                        // increment target square
                        to_square += bishop_offsets[index];
                    }
                }
            }
            
            // rook & queen moves
            if (
                 !side ?
                 (board[square] == R) || (board[square] == Q) :
                 (board[square] == r) || (board[square] == q)
               )
            {
                // loop over bishop & queen offsets
                for (int index = 0; index < 4; index++)
                {
                    // init target square
                    int to_square = square + rook_offsets[index];
                    
                    // loop over attack ray
                    while (!(to_square & 0x88))
                    {
                        // init target piece
                        int piece = board[to_square];
                        
                        // if hits own piece
                        if (!side ? (piece >= 1 && piece <= 6) : ((piece >= 7 && piece <= 12)))
                            break;
                        
                        // if hits opponent's piece
                        if (!side ? (piece >= 7 && piece <= 12) : ((piece >= 1 && piece <= 6)))
                        {
                            add_move(move_list, encode_move(square, to_square, 0, 1, 0, 0, 0));
                            break;
                        }
                        
                        // if steps into an empty squre
                        if (!piece)
                            add_move(move_list, encode_move(square, to_square, 0, 0, 0, 0, 0));
                        
                        // increment target square
                        to_square += rook_offsets[index];
                    }
                }
            }
        }
    }
}

// make move
int make_move(int move)
{
    // define board state variable copies
    int board_copy[128], king_square_copy[2];
    int side_copy, enpassant_copy, castle_copy;
    
    // copy board state
    memcpy(board_copy, board, 512);
    side_copy = side;
    enpassant_copy = enpassant;
    castle_copy = castle;
    memcpy(king_square_copy, king_square,8);
    
    // extact source & target squares from move
    int from_square = get_move_source(move);
    int to_square = get_move_target(move);
    
    // move piece
    board[to_square] = board[from_square];
    board[from_square] = e;
    
    /* restore board position
    memcpy(board, board_copy, 512);
    side = side_copy;
    enpassant = enpassant_copy;
    castle = castle_copy;
    memcpy(king_square, king_square_copy,8);
    */
}

// main driver
int main()
{
    parse_fen(tricky_position);
    print_board();
    
    // create move list variable
    moves move_list[1];
    
    // generate moves
    generate_moves(move_list);
    
    // create move
    int move = encode_move(d5, d6, 0, 0, 0, 0, 0);
    
    // define board state variable copies
    int board_copy[128], king_square_copy[2];
    int side_copy, enpassant_copy, castle_copy;
    
    // copy board state
    memcpy(board_copy, board, 512);
    side_copy = side;
    enpassant_copy = enpassant;
    castle_copy = castle;
    memcpy(king_square_copy, king_square,8);
    
    // make move
    make_move(move);
    print_board();
    
    // restore board position
    memcpy(board, board_copy, 512);
    side = side_copy;
    enpassant = enpassant_copy;
    castle = castle_copy;
    memcpy(king_square, king_square_copy,8);
    
    print_board();

    return 0;
}









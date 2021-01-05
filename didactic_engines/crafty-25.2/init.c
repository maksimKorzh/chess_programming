#include <stddef.h>
#include <math.h>
#include "chess.h"
#include "data.h"
#if defined(UNIX)
#  include <unistd.h>
#  include <sys/types.h>
#else
#  include <fcntl.h> /* needed for definition of "_O_BINARY" */
#endif
/*
 *******************************************************************************
 *                                                                             *
 *   Initialize() performs routine initialization before anything else is      *
 *   attempted.  It uses a group of service routines to initialize various     *
 *   data structures that are needed before the engine can do anything at all. *
 *                                                                             *
 *******************************************************************************
 */
void Initialize() {
  TREE *tree;
  int j, v, major, id;
#if defined(UNIX)
  int i, node;
#endif

  tree = block[0];
  for (j = 1; j <= MAX_BLOCKS; j++)
    block[j] = NULL;
  InitializeMasks();
  InitializeMagic();
  InitializeSMP();
  InitializeAttackBoards();
  InitializePawnMasks();
  InitializeChessBoard(tree);
  InitializeKillers();
#if !defined(UNIX)
  _fmode = _O_BINARY; /* set file mode binary to avoid text translation */
#endif
#if defined(EPD)
  EGInit();
#endif
  tree->last[0] = tree->move_list;
  tree->last[1] = tree->move_list;
  sprintf(log_filename, "%s/book.bin", book_path);
  book_file = fopen(log_filename, "rb+");
  if (!book_file) {
    book_file = fopen(log_filename, "rb");
    if (!book_file) {
      Print(2048, "unable to open book file [%s/book.bin].\n", book_path);
      Print(32, "book is disabled\n");
    } else {
      Print(2048, "unable to open book file [%s/book.bin] for \"write\".\n",
          book_path);
      Print(32, "learning is disabled\n");
    }
  }
  sprintf(log_filename, "%s/books.bin", book_path);
  normal_bs_file = fopen(log_filename, "rb");
  books_file = normal_bs_file;
  if (!normal_bs_file)
    Print(32, "unable to open book file [%s/books.bin].\n", book_path);
  sprintf(log_filename, "%s/bookc.bin", book_path);
  computer_bs_file = fopen(log_filename, "rb");
  if (computer_bs_file)
    Print(32, "found computer opening book file [%s/bookc.bin].\n",
        book_path);
  if (book_file) {
    int maj_min;
    fseek(book_file, - (long) sizeof(int), SEEK_END);
    v = fread(&maj_min, 4, 1, book_file);
    if (v <= 0)
      perror("Initialize() fread error: ");
    major = BookIn32((unsigned char *) &maj_min);
    major = major >> 16;
    if (major < 23) {
      Print(4095, "\nERROR!  book.bin not made by version 23.0 or later\n");
      fclose(book_file);
      fclose(books_file);
      book_file = 0;
      books_file = 0;
    }
  }
  id = InitializeGetLogID();
  sprintf(log_filename, "%s/log.%03d", log_path, id);
  sprintf(history_filename, "%s/game.%03d", log_path, id);
  log_file = fopen(log_filename, "w");
  history_file = fopen(history_filename, "w+");
  if (!history_file) {
    printf("ERROR, unable to open game history file, exiting\n");
    CraftyExit(1);
  }
  AlignedMalloc((void *) &hash_table, 64,
      sizeof(HASH_ENTRY) * hash_table_size);
  AlignedMalloc((void *) &hash_path, 64,
      sizeof(HPATH_ENTRY) * hash_path_size);
  AlignedMalloc((void *) &pawn_hash_table, 64,
      sizeof(PAWN_HASH_ENTRY) * pawn_hash_table_size);
  if (!hash_table) {
    Print(2048,
        "AlignedMalloc() failed, not enough memory (primary trans/ref table).\n");
    hash_table_size = 0;
    hash_table = 0;
  }
  if (!pawn_hash_table) {
    Print(2048,
        "AlignedMalloc() failed, not enough memory (pawn hash table).\n");
    pawn_hash_table_size = 0;
    pawn_hash_table = 0;
  }
/*
 ************************************************************
 *                                                          *
 *  Now for some NUMA stuff.  We need to allocate the local *
 *  memory for each processor, but we can't touch it here   *
 *  or it will be faulted in and be allocated on the        *
 *  current CPU, which is not where it should be located    *
 *  for optimal NUMA performance.  ThreadInit() will do the *
 *  actual initialization after each new process is created *
 *  so that the pages of local memory will be faulted in on *
 *  the correct processor and use local node memory for     *
 *  optimal performance.                                    *
 *                                                          *
 *  If we are using CPU affinity, we need to set this up    *
 *  for thread 0 BEFORE we initialize the split blocks so   *
 *  that they will page fault in on the correct NUMA node.  *
 *                                                          *
 ************************************************************
 */
#if (CPUS > 1)
  ThreadAffinity(smp_affinity);
#  if !defined(UNIX)
  ThreadMalloc((int) 0);
#  else
  for (i = 0; i < CPUS; i++) {
    for (j = 0; j < 64; j++) {
      AlignedMalloc((void **) &block[i * 64 + j + 1], 2048,
          (size_t) sizeof(TREE));
    }
  }
  for (i = 1; i < 64; i++) {
    memset((char *) block[i], 0, sizeof(TREE));
    LockInit(block[i]->lock);
  }
  for (node = 1; node < CPUS; node++) {
    ThreadAffinity(node);
    for (i = 0; i < 64; i++) {
      memset((char *) block[node * 64 + i], 0, sizeof(TREE));
      LockInit(block[node * 64 + i]->lock);
    }
  }
  ThreadAffinity(0);
#  endif
#endif
  thread[0].blocks = 0xffffffffffffffffull;
  initialized_threads++;
  InitializeHashTables(1);
  InitializeKingSafety();
  InitializeLMP();
  InitializeLMR();
}

/*
 *******************************************************************************
 *                                                                             *
 *   InitializeAttackBoards() is used to initialize the basic bitboards that   *
 *   deal with what squares a piece attacks.                                   *
 *                                                                             *
 *******************************************************************************
 */
void InitializeAttackBoards(void) {
  int i, j, d, s, t, frank, ffile, trank, tfile;
  int sq, lastsq;
  static const int knightsq[8] = { -17, -15, -10, -6, 6, 10, 15, 17 };
  static const int bishopsq[4] = { -9, -7, 7, 9 };
  static const int rooksq[4] = { -8, -1, 1, 8 };
  uint64_t sqs;

/*
 initialize pawn attack boards
 */
  for (i = 0; i < 64; i++) {
    pawn_attacks[white][i] = 0;
    if (i < 56)
      for (j = 2; j < 4; j++) {
        sq = i + bishopsq[j];
        if (Abs(Rank(sq) - Rank(i)) == 1 && Abs(File(sq) - File(i)) == 1 &&
            sq < 64 && sq > -1)
          pawn_attacks[white][i] =
              pawn_attacks[white][i] | (uint64_t) 1 << sq;
      }
    pawn_attacks[black][i] = 0;
    if (i > 7)
      for (j = 0; j < 2; j++) {
        sq = i + bishopsq[j];
        if (Abs(Rank(sq) - Rank(i)) == 1 && Abs(File(sq) - File(i)) == 1 &&
            sq < 64 && sq > -1)
          pawn_attacks[black][i] =
              pawn_attacks[black][i] | (uint64_t) 1 << sq;
      }
  }
/*
 initialize knight attack board
 */
  for (i = 0; i < 64; i++) {
    knight_attacks[i] = 0;
    frank = Rank(i);
    ffile = File(i);
    for (j = 0; j < 8; j++) {
      sq = i + knightsq[j];
      if (sq < 0 || sq > 63)
        continue;
      trank = Rank(sq);
      tfile = File(sq);
      if (Abs(frank - trank) > 2 || Abs(ffile - tfile) > 2)
        continue;
      knight_attacks[i] = knight_attacks[i] | (uint64_t) 1 << sq;
    }
  }
/*
 initialize bishop/queen attack boards and masks
 */
  for (i = 0; i < 64; i++) {
    for (j = 0; j < 4; j++) {
      sq = i;
      lastsq = sq;
      sq = sq + bishopsq[j];
      while (Abs(Rank(sq) - Rank(lastsq)) == 1 &&
          Abs(File(sq) - File(lastsq)) == 1 && sq < 64 && sq > -1) {
        if (bishopsq[j] == 7)
          plus7dir[i] = plus7dir[i] | (uint64_t) 1 << sq;
        else if (bishopsq[j] == 9)
          plus9dir[i] = plus9dir[i] | (uint64_t) 1 << sq;
        else if (bishopsq[j] == -7)
          minus7dir[i] = minus7dir[i] | (uint64_t) 1 << sq;
        else
          minus9dir[i] = minus9dir[i] | (uint64_t) 1 << sq;
        lastsq = sq;
        sq = sq + bishopsq[j];
      }
    }
  }
  plus1dir[64] = 0;
  plus7dir[64] = 0;
  plus8dir[64] = 0;
  plus9dir[64] = 0;
  minus1dir[64] = 0;
  minus7dir[64] = 0;
  minus8dir[64] = 0;
  minus9dir[64] = 0;
/*
 initialize rook/queen attack boards
 */
  for (i = 0; i < 64; i++) {
    for (j = 0; j < 4; j++) {
      sq = i;
      lastsq = sq;
      sq = sq + rooksq[j];
      while (((Abs(Rank(sq) - Rank(lastsq)) == 1 &&
                  Abs(File(sq) - File(lastsq)) == 0)
              || (Abs(Rank(sq) - Rank(lastsq)) == 0 &&
                  Abs(File(sq) - File(lastsq)) == 1)) && sq < 64 && sq > -1) {
        if (rooksq[j] == 1)
          plus1dir[i] = plus1dir[i] | (uint64_t) 1 << sq;
        else if (rooksq[j] == 8)
          plus8dir[i] = plus8dir[i] | (uint64_t) 1 << sq;
        else if (rooksq[j] == -1)
          minus1dir[i] = minus1dir[i] | (uint64_t) 1 << sq;
        else
          minus8dir[i] = minus8dir[i] | (uint64_t) 1 << sq;
        lastsq = sq;
        sq = sq + rooksq[j];
      }
    }
  }
/*
 initialize bishop attack board
 */
  for (i = 0; i < 64; i++) {
    bishop_attacks[i] =
        plus9dir[i] | minus9dir[i] | plus7dir[i] | minus7dir[i];
  }
/*
 initialize rook attack board
 */
  for (i = 0; i < 64; i++) {
    rook_attacks[i] = file_mask[File(i)] | rank_mask[Rank(i)];
  }
/*
 initialize king attack board
 */
  for (i = 0; i < 64; i++) {
    king_attacks[i] = 0;
    for (j = 0; j < 64; j++) {
      if (Distance(i, j) == 1)
        king_attacks[i] = king_attacks[i] | SetMask(j);
    }
  }
/*
 directions[sq1][sq2] gives the "move direction" to move from
 sq1 to sq2.  intervening[sq1][sq2] gives a bit vector that indicates
 which squares must be unoccupied in order for <sq1> to attack <sq2>,
 assuming a sliding piece is involved.  To use this, you simply have
 to Or(intervening[sq1][sq2],occupied_squares) and if the result is
 "0" then a sliding piece on sq1 would attack sq2 and vice-versa.
 */
  for (i = 0; i < 64; i++) {
    for (j = 0; j < 64; j++)
      intervening[i][j] = 0;
    sqs = plus1dir[i];
    while (sqs) {
      j = LSB(sqs);
      directions[i][j] = 1;
      intervening[i][j] = plus1dir[i] ^ plus1dir[j - 1];
      sqs &= sqs - 1;
    }
    sqs = plus7dir[i];
    while (sqs) {
      j = LSB(sqs);
      directions[i][j] = 7;
      intervening[i][j] = plus7dir[i] ^ plus7dir[j - 7];
      sqs &= sqs - 1;
    }
    sqs = plus8dir[i];
    while (sqs) {
      j = LSB(sqs);
      directions[i][j] = 8;
      intervening[i][j] = plus8dir[i] ^ plus8dir[j - 8];
      sqs &= sqs - 1;
    }
    sqs = plus9dir[i];
    while (sqs) {
      j = LSB(sqs);
      directions[i][j] = 9;
      intervening[i][j] = plus9dir[i] ^ plus9dir[j - 9];
      sqs &= sqs - 1;
    }
    sqs = minus1dir[i];
    while (sqs) {
      j = LSB(sqs);
      directions[i][j] = -1;
      intervening[i][j] = minus1dir[i] ^ minus1dir[j + 1];
      sqs &= sqs - 1;
    }
    sqs = minus7dir[i];
    while (sqs) {
      j = LSB(sqs);
      directions[i][j] = -7;
      intervening[i][j] = minus7dir[i] ^ minus7dir[j + 7];
      sqs &= sqs - 1;
    }
    sqs = minus8dir[i];
    while (sqs) {
      j = LSB(sqs);
      directions[i][j] = -8;
      intervening[i][j] = minus8dir[i] ^ minus8dir[j + 8];
      sqs &= sqs - 1;
    }
    sqs = minus9dir[i];
    while (sqs) {
      j = LSB(sqs);
      directions[i][j] = -9;
      intervening[i][j] = minus9dir[i] ^ minus9dir[j + 9];
      sqs &= sqs - 1;
    }
  }
/*
  distance_ring[square][distance] has a ring of 1's around "square" with a
  distance of "distance".  IE for e4, we have a ring of adjacent squares
  [e4][1], the next ring (2 squares away) for [e4][2], etc.  In this code,
  s = square being set up, d = distance from square to "ring" and t = target
  square that is on the ring if distance is correct.
 */
  for (s = 0; s < 64; s++) {
    distance_ring[s][0] = SetMask(s);
    for (d = 1; d < 8; d++) {
      distance_ring[s][d] = 0;
      for (t = 0; t < 64; t++)
        if (Distance(s, t) == d)
          distance_ring[s][d] |= SetMask(t);
    }
  }
}

/*
 *******************************************************************************
 *                                                                             *
 *   InitializeMagic() initializes the magic number tables used in the new     *
 *   magic move generation algorithm.  We also initialize a set of parallel    *
 *   tables that contain mobility scores for each possible set of magic attack *
 *   vectors, which saves significant time in the evaluation, since it is done *
 *   here before the game actually starts.                                     *
 *                                                                             *
 *******************************************************************************
 */
void InitializeMagic(void) {
  int i, j, m;
  int initmagicmoves_bitpos64_database[64] = {
    63, 0, 58, 1, 59, 47, 53, 2,
    60, 39, 48, 27, 54, 33, 42, 3,
    61, 51, 37, 40, 49, 18, 28, 20,
    55, 30, 34, 11, 43, 14, 22, 4,
    62, 57, 46, 52, 38, 26, 32, 41,
    50, 36, 17, 19, 29, 10, 13, 21,
    56, 45, 25, 31, 35, 16, 9, 12,
    44, 24, 15, 8, 23, 7, 6, 5
  };
/*
 Bishop attacks and mobility
 */
  for (i = 0; i < 64; i++) {
    int squares[64];
    int numsquares = 0;
    uint64_t temp = magic_bishop_mask[i];

    while (temp) {
      uint64_t abit = temp & -temp;

      squares[numsquares++] =
          initmagicmoves_bitpos64_database[(abit *
              0x07EDD5E59A4E28C2ull) >> 58];
      temp ^= abit;
    }
    for (temp = 0; temp < (uint64_t) 1 << numsquares; temp++) {
      uint64_t moves;
      uint64_t tempoccupied =
          InitializeMagicOccupied(squares, numsquares, temp);
      moves = InitializeMagicBishop(i, tempoccupied);
      *(magic_bishop_indices[i] +
          (tempoccupied * magic_bishop[i] >> magic_bishop_shift[i])) = moves;
      moves |= SetMask(i);
      m = -lower_b;
      for (j = 0; j < 4; j++)
        m += PopCnt(moves & mobility_mask_b[j]) * mobility_score_b[j];
      if (m < 0)
        m *= 2;
      *(magic_bishop_mobility_indices[i] +
          (tempoccupied * magic_bishop[i] >> magic_bishop_shift[i])) = m;
    }
  }
/*
 Rook attacks and mobility
 */
  for (i = 0; i < 64; i++) {
    int squares[64];
    int numsquares = 0;
    uint64_t temp = magic_rook_mask[i];

    while (temp) {
      uint64_t abit = temp & -temp;

      squares[numsquares++] =
          initmagicmoves_bitpos64_database[(abit *
              0x07EDD5E59A4E28C2ull) >> 58];
      temp ^= abit;
    }
    for (temp = 0; temp < (uint64_t) 1 << numsquares; temp++) {
      uint64_t tempoccupied =
          InitializeMagicOccupied(squares, numsquares, temp);
      uint64_t moves = InitializeMagicRook(i, tempoccupied);
      *(magic_rook_indices[i] +
          (tempoccupied * magic_rook[i] >> magic_rook_shift[i])) = moves;
      moves |= SetMask(i);
      m = -1;
      for (j = 0; j < 4; j++)
        m += PopCnt(moves & mobility_mask_r[j]) * mobility_score_r[j];
      *(magic_rook_mobility_indices[i] +
          (tempoccupied * magic_rook[i] >> magic_rook_shift[i])) =
          mob_curve_r[m];
    }
  }
}

/*
 *******************************************************************************
 *                                                                             *
 *   InitializeMagicBishop() does the bishop-specific initialization for a     *
 *   particular square on the board.                                           *
 *                                                                             *
 *******************************************************************************
 */
uint64_t InitializeMagicBishop(int square, uint64_t occupied) {
  uint64_t ret = 0;
  uint64_t abit;
  uint64_t abit2;
  uint64_t rowbits = (uint64_t) 0xFF << 8 * (square / 8);

  abit = (uint64_t) 1 << square;
  abit2 = abit;
  do {
    abit <<= 8 - 1;
    abit2 >>= 1;
    if (abit2 & rowbits)
      ret |= abit;
    else
      break;
  } while (abit && !(abit & occupied));
  abit = (uint64_t) 1 << square;
  abit2 = abit;
  do {
    abit <<= 8 + 1;
    abit2 <<= 1;
    if (abit2 & rowbits)
      ret |= abit;
    else
      break;
  } while (abit && !(abit & occupied));
  abit = (uint64_t) 1 << square;
  abit2 = abit;
  do {
    abit >>= 8 - 1;
    abit2 <<= 1;
    if (abit2 & rowbits)
      ret |= abit;
    else
      break;
  } while (abit && !(abit & occupied));
  abit = (uint64_t) 1 << square;
  abit2 = abit;
  do {
    abit >>= 8 + 1;
    abit2 >>= 1;
    if (abit2 & rowbits)
      ret |= abit;
    else
      break;
  } while (abit && !(abit & occupied));
  return ret;
}

/*
 *******************************************************************************
 *                                                                             *
 *   InitializeMagicOccupied() generates a specific occupied-square bitboard   *
 *   needed during initialization.                                             *
 *                                                                             *
 *******************************************************************************
 */
uint64_t InitializeMagicOccupied(int *squares, int numSquares,
    uint64_t linoccupied) {
  int i;
  uint64_t ret = 0;

  for (i = 0; i < numSquares; i++)
    if (linoccupied & (uint64_t) 1 << i)
      ret |= (uint64_t) 1 << squares[i];
  return ret;
}

/*
 *******************************************************************************
 *                                                                             *
 *   InitializeMagicRook() does the rook-specific initialization for a         *
 *   particular square on the board.                                           *
 *                                                                             *
 *******************************************************************************
 */
uint64_t InitializeMagicRook(int square, uint64_t occupied) {
  uint64_t ret = 0;
  uint64_t abit;
  uint64_t rowbits = (uint64_t) 0xFF << 8 * (square / 8);

  abit = (uint64_t) 1 << square;
  do {
    abit <<= 8;
    ret |= abit;
  } while (abit && !(abit & occupied));
  abit = (uint64_t) 1 << square;
  do {
    abit >>= 8;
    ret |= abit;
  } while (abit && !(abit & occupied));
  abit = (uint64_t) 1 << square;
  do {
    abit <<= 1;
    if (abit & rowbits)
      ret |= abit;
    else
      break;
  } while (!(abit & occupied));
  abit = (uint64_t) 1 << square;
  do {
    abit >>= 1;
    if (abit & rowbits)
      ret |= abit;
    else
      break;
  } while (!(abit & occupied));
  return ret;
}

/*
 *******************************************************************************
 *                                                                             *
 *   InitializeChessBoard() initializes the chess board to the normal starting *
 *   position.  It then calls SetChessBitboards() to correctly set the usual   *
 *   occupied-square bitboards to correspond to the starting position.         *
 *                                                                             *
 *******************************************************************************
 */
void InitializeChessBoard(TREE * tree) {
  int i;

  if (strlen(initial_position)) {
    int nargs;

    nargs = ReadParse(initial_position, args, " \t;");
    SetBoard(tree, nargs, args, 1);
  } else {
    for (i = 0; i < 64; i++)
      PcOnSq(i) = empty;
    game_wtm = 1;
/*
 place pawns
 */
    for (i = 0; i < 8; i++) {
      PcOnSq(i + 8) = pawn;
      PcOnSq(i + 48) = -pawn;
    }
/*
 place knights
 */
    PcOnSq(B1) = knight;
    PcOnSq(G1) = knight;
    PcOnSq(B8) = -knight;
    PcOnSq(G8) = -knight;
/*
 place bishops
 */
    PcOnSq(C1) = bishop;
    PcOnSq(F1) = bishop;
    PcOnSq(C8) = -bishop;
    PcOnSq(F8) = -bishop;
/*
 place rooks
 */
    PcOnSq(A1) = rook;
    PcOnSq(H1) = rook;
    PcOnSq(A8) = -rook;
    PcOnSq(H8) = -rook;
/*
 place queens
 */
    PcOnSq(D1) = queen;
    PcOnSq(D8) = -queen;
/*
 place kings
 */
    PcOnSq(E1) = king;
    PcOnSq(E8) = -king;
/*
 initialize castling status so all castling is legal.
 */
    Castle(0, black) = 3;
    Castle(0, white) = 3;
/*
 initialize enpassant status.
 */
    EnPassant(0) = 0;
/*
 now, set the bit-boards.
 */
    SetChessBitBoards(tree);
  }
/*
 initialize 50 move counter and repetition list/index.
 */
  Reversible(0) = 0;
  rep_index = 0;
  tree->rep_list[0] = HashKey;
}

/*
 *******************************************************************************
 *                                                                             *
 *   SetChessBitBoards() is used to set the occupied-square bitboards so that  *
 *   they agree with the current real chessboard.                              *
 *                                                                             *
 *******************************************************************************
 */
void SetChessBitBoards(TREE * tree) {
  int side, piece, square;

  HashKey = 0;
  PawnHashKey = 0;
  Material = 0;
  for (side = black; side <= white; side++)
    for (piece = empty; piece <= king; piece++)
      Pieces(side, piece) = 0;
  for (square = 0; square < 64; square++) {
    if (!PcOnSq(square))
      continue;
    piece = PcOnSq(square);
    side = (piece > 0) ? 1 : 0;
    Pieces(side, Abs(piece)) |= SetMask(square);
    Occupied(side) |= SetMask(square);
    Hash(side, Abs(piece), square);
    if (Abs(piece) == pawn)
      HashP(side, square);
    Material += PieceValues(side, Abs(piece));
  }
  if (Pieces(white, king))
    KingSQ(white) = LSB(Pieces(white, king));
  if (Pieces(black, king))
    KingSQ(black) = LSB(Pieces(black, king));
  if (EnPassant(0))
    HashEP(EnPassant(0));
  if (!(Castle(0, white) & 1))
    HashCastle(0, white);
  if (!(Castle(0, white) & 2))
    HashCastle(1, white);
  if (!(Castle(0, black) & 1))
    HashCastle(0, black);
  if (!(Castle(0, black) & 2))
    HashCastle(1, black);
/*
 initialize black/white piece counts.
 */
  for (side = black; side <= white; side++)
    for (piece = pawn; piece <= king; piece++)
      TotalPieces(side, piece) = PopCnt(Pieces(side, piece));
  for (side = black; side <= white; side++) {
    TotalPieces(side, occupied) = 0;
    for (piece = knight; piece < king; piece++)
      TotalPieces(side, occupied) +=
          PopCnt(Pieces(side, piece)) * p_vals[piece];
  }
  TotalAllPieces = PopCnt(OccupiedSquares);
  rep_index = 0;
  tree->rep_list[0] = HashKey;
}

/*
 *******************************************************************************
 *                                                                             *
 *   InitializeGetLogID() is used to determine the nnn (in log.nnn) to use for *
 *   the current game.  It is typically the ID of the last log + 1, but we do  *
 *   not know what that is if we just started the engine.  We simply look thru *
 *   existing log files in the current directory and use the next un-used name *
 *   in sequence.                                                              *
 *                                                                             *
 *******************************************************************************
 */
int InitializeGetLogID(void) {
  int t, total_files = 0;
#if defined(UNIX)
  struct stat *fileinfo = malloc(sizeof(struct stat));
#endif

  for (; total_files < 300; log_id++) {
    sprintf(log_filename, "%s/log.%03d", log_path, log_id);
    sprintf(history_filename, "%s/game.%03d", log_path, log_id);
    log_file = fopen(log_filename, "r");
    if (!log_file)
      break;
    fclose(log_file);
    total_files++;
  }
#if defined(UNIX)
/*  a kludge to work around an xboard 4.2.3 problem.  It sends two "quit"
   commands, which causes every other log.nnn file to be empty.  this code
   looks for a very small log.nnn file as the last one, and if it is small,
   then we simply overwrite it to solve this problem temporarily.  this will
   be removed when the nexto xboard version comes out to fix this extra quit
   problem.                                                                  */
  {
    char tfn[128];
    FILE *tlog;

    sprintf(tfn, "%s/log.%03d", log_path, log_id - 1);
    tlog = fopen(tfn, "r+");
    if (tlog) {
      fstat(fileno(tlog), fileinfo);
      if (fileinfo->st_size < 2000)
        log_id--;
    }
  }
#endif
  t = log_id++;
  return t;
}

/*
 *******************************************************************************
 *                                                                             *
 *   InitializeHashTables() is used to clear all hash entries completely, so   *
 *   that no old information remains to interefere with a new game or test     *
 *   position.                                                                 *
 *                                                                             *
 *   Whenever any hash table size is changed, they are initialized by calling  *
 *   this procedure to make sure that in the case of NUMA hardware, the trick  *
 *   explained below is always executed.                                       *
 *                                                                             *
 *   This code uses the NUMA fix when using MT threads.  It clears size / MT   *
 *   bytes per cpu, after pinning the current thread to the correct cpu, so    *
 *   that the data will fault in to the correct NUMA node.  If the size is not *
 *   perfectly divisible by MT (max threads) it clears the final piece at the  *
 *   end of each loop.                                                         *
 *                                                                             *
 *   Note that if no size has changed, (fault_in = 0) then we skip the NUMA    *
 *   stuff and just clear the tables, period.                                  *
 *                                                                             *
 *******************************************************************************
 */
void InitializeHashTables(int fault_in) {
  uint64_t mem_per_node;
  int node;

  transposition_age = 0;
  if (fault_in && smp_numa) {
/*
 ************************************************************
 *                                                          *
 *  First, initialize the primary transposition/refutation  *
 *  (hash) table, using the NUMA trick to place part of     *
 *  the trans/ref on each node of the NUMA system.          *
 *                                                          *
 ************************************************************
 */
    mem_per_node =
        hash_table_size * sizeof(HASH_ENTRY) / Max(smp_max_threads, 1);
    for (node = 0; node < smp_max_threads; node++) {
      ThreadAffinity(node);
      memset((char *) hash_table + node * mem_per_node, 0, mem_per_node);
    }
    ThreadAffinity(0);
    if (mem_per_node * Max(smp_max_threads,
            1) < hash_table_size * sizeof(HASH_ENTRY))
      memset((char *) hash_table + smp_max_threads * mem_per_node, 0,
          hash_table_size * sizeof(HASH_ENTRY) -
          mem_per_node * smp_max_threads);
/*
 ************************************************************
 *                                                          *
 *  Second, initialize the primary hash path table, using   *
 *  the NUMA trick to place part of the hash path table on  *
 *  each node of the NUMA system.                           *
 *                                                          *
 ************************************************************
 */
    mem_per_node =
        hash_path_size * sizeof(HPATH_ENTRY) / Max(smp_max_threads, 1);
    for (node = 0; node < smp_max_threads; node++) {
      ThreadAffinity(node);
      memset((char *) hash_path + node * mem_per_node, 0, mem_per_node);
    }
    ThreadAffinity(0);
    if (mem_per_node * Max(smp_max_threads,
            1) < hash_path_size * sizeof(HPATH_ENTRY))
      memset((char *) hash_path + smp_max_threads * mem_per_node, 0,
          hash_path_size * sizeof(HPATH_ENTRY) -
          mem_per_node * smp_max_threads);
/*
 ************************************************************
 *                                                          *
 *  Finally, initialize the primary pawn hash table, using  *
 *  the NUMA trick to place part of the pawn hash table on  *
 *  each node of the NUMA system.                           *
 *                                                          *
 ************************************************************
 */
    mem_per_node =
        pawn_hash_table_size * sizeof(PAWN_HASH_ENTRY) / Max(smp_max_threads,
        1);
    for (node = 0; node < smp_max_threads; node++) {
      ThreadAffinity(node);
      memset((char *) pawn_hash_table + node * mem_per_node, 0, mem_per_node);
    }
    ThreadAffinity(0);
    if (mem_per_node * Max(smp_max_threads,
            1) < pawn_hash_table_size * sizeof(PAWN_HASH_ENTRY))
      memset((char *) pawn_hash_table + smp_max_threads * mem_per_node, 0,
          pawn_hash_table_size * sizeof(PAWN_HASH_ENTRY) -
          mem_per_node * smp_max_threads);
/*
 ************************************************************
 *                                                          *
 *  Before we return, we need to re-pin this thread to the  *
 *  correct processor.                                      *
 *                                                          *
 ************************************************************
 */
    ThreadAffinity(smp_affinity);
  } else {
/*
 ************************************************************
 *                                                          *
 *  Otherwise we only need to use memset() to clear the     *
 *  tables since they have already been faulted in to the   *
 *  correct NUMA node.                                      *
 *                                                          *
 ************************************************************
 */
    memset((char *) hash_table, 0, hash_table_size * sizeof(HASH_ENTRY));
    memset((char *) hash_path, 0, hash_path_size * sizeof(HPATH_ENTRY));
    memset((char *) pawn_hash_table, 0,
        pawn_hash_table_size * sizeof(PAWN_HASH_ENTRY));
  }
}

/*
 *******************************************************************************
 *                                                                             *
 *   InitializeKillers() is used to zero the killer moves so that old killers  *
 *   don't screw up ordering while processing test suites.  Ditto for history  *
 *   counters.                                                                 *
 *                                                                             *
 *******************************************************************************
 */
void InitializeKillers(void) {
  int i;

  for (i = 0; i < MAXPLY; i++) {
    block[0]->killers[i].move1 = 0;
    block[0]->killers[i].move2 = 0;
  }
  for (i = 0; i < 1024; i++)
    history[i] = 1024;
}

/*
 *******************************************************************************
 *                                                                             *
 *   InitializeMasks() is used to initialize the various bitboard masks that   *
 *   are used throughout Crafty.                                               *
 *                                                                             *
 *******************************************************************************
 */
void InitializeMasks(void) {
  int i, j;

/*
 masks to set/clear a bit on a specific square
 */
  for (i = 0; i < 64; i++) {
    ClearMask(i) = ~((uint64_t) 1 << i);
    SetMask(i) = (uint64_t) 1 << i;
  }
  ClearMask(BAD_SQUARE) = 0;
  SetMask(BAD_SQUARE) = 0;
/*
 masks to select bits on a specific rank or file
 */
  rank_mask[0] = (uint64_t) 255;
  for (i = 1; i < 8; i++)
    rank_mask[i] = rank_mask[i - 1] << 8;
  file_mask[FILEA] = (uint64_t) 1;
  for (i = 1; i < 8; i++)
    file_mask[FILEA] = file_mask[FILEA] | file_mask[FILEA] << 8;
  for (i = 1; i < 8; i++)
    file_mask[i] = file_mask[i - 1] << 1;
/*
 masks to determine if a pawn has nearby neighbors or not.
 */
#if !defined(INLINEASM)
  msb[0] = 64;
  lsb[0] = 16;
  for (i = 1; i < 65536; i++) {
    lsb[i] = 16;
    for (j = 0; j < 16; j++)
      if (i & (1 << j)) {
        msb[i] = j;
        if (lsb[i] == 16)
          lsb[i] = j;
      }
  }
#endif
  msb_8bit[0] = 8;
  lsb_8bit[0] = 8;
  pop_cnt_8bit[0] = 0;
  for (i = 1; i < 256; i++) {
    pop_cnt_8bit[i] = 0;
    for (j = 0; j < 8; j++)
      if (i & (1 << j))
        pop_cnt_8bit[i]++;
    lsb_8bit[i] = 8;
    for (j = 0; j < 8; j++) {
      if (i & (1 << j)) {
        msb_8bit[i] = j;
        if (lsb_8bit[i] == 8)
          lsb_8bit[i] = j;
      }
    }
  }
}

/*
 *******************************************************************************
 *                                                                             *
 *   InitializePawnMasks() is used to initialize the various bitboard masks    *
 *   that are used in pawn evaluation.                                         *
 *                                                                             *
 *******************************************************************************
 */
void InitializePawnMasks(void) {
  int i, j;

/*
 initialize isolated pawn masks, which are nothing more than 1's on
 the files adjacent to the pawn file.
 */
  for (i = 0; i < 64; i++) {
    if (!File(i))
      mask_pawn_isolated[i] = file_mask[File(i) + 1];
    else if (File(i) == 7)
      mask_pawn_isolated[i] = file_mask[File(i) - 1];
    else
      mask_pawn_isolated[i] = file_mask[File(i) - 1] | file_mask[File(i) + 1];
  }
/*
 initialize passed pawn masks, which are nothing more than 1's on
 the pawn's file and the adjacent files, but only on ranks that are
 in "front" of the pawn.
 */
  for (i = 0; i < 64; i++) {
    if (!File(i)) {
      mask_passed[white][i] = plus8dir[i] | plus8dir[i + 1];
      mask_passed[black][i] = minus8dir[i] | minus8dir[i + 1];
    } else if (File(i) == 7) {
      mask_passed[white][i] = plus8dir[i - 1] | plus8dir[i];
      mask_passed[black][i] = minus8dir[i - 1] | minus8dir[i];
    } else {
      mask_passed[white][i] = plus8dir[i - 1] | plus8dir[i] | plus8dir[i + 1];
      mask_passed[black][i] =
          minus8dir[i - 1] | minus8dir[i] | minus8dir[i + 1];
    }
  }
/*
 masks to determine if a pawn has supporting contact with friendly pawns.
 */
  for (i = 8; i < 56; i++) {
    if (File(i) > 0 && File(i) < 7) {
      mask_pawn_connected[white][i] =
          SetMask(i - 1) | SetMask(i + 1) | SetMask(i - 9) | SetMask(i - 7);
      mask_pawn_connected[black][i] =
          SetMask(i - 1) | SetMask(i + 1) | SetMask(i + 9) | SetMask(i + 7);
    } else if (File(i) == 0) {
      mask_pawn_connected[white][i] = SetMask(i + 1) | SetMask(i - 7);
      mask_pawn_connected[black][i] = SetMask(i + 1) | SetMask(i + 9);
    } else if (File(i) == 7) {
      mask_pawn_connected[white][i] = SetMask(i - 1) | SetMask(i - 9);
      mask_pawn_connected[black][i] = SetMask(i - 1) | SetMask(i + 7);
    }
  }
/*
 these masks are used to determine if the other side has any pawns
 that can attack [square].
 */
  for (i = 8; i < 56; i++) {
    if (!File(i)) {
      mask_pattacks[white][i] = minus8dir[i + 1];
      mask_pattacks[black][i] = plus8dir[i + 1];
    } else if (File(i) == 7) {
      mask_pattacks[white][i] = minus8dir[i - 1];
      mask_pattacks[black][i] = plus8dir[i - 1];
    } else {
      mask_pattacks[white][i] = minus8dir[i - 1] | minus8dir[i + 1];
      mask_pattacks[black][i] = plus8dir[i + 1] | plus8dir[i - 1];
    }
  }
/*
 enpassant pawns are on either file adjacent to the current file, and
 on the same rank.
 */
  for (i = 0; i < 64; i++)
    mask_eptest[i] = 0;
  for (i = 25; i < 31; i++)
    mask_eptest[i] = SetMask(i - 1) | SetMask(i + 1);
  for (i = 33; i < 39; i++)
    mask_eptest[i] = SetMask(i - 1) | SetMask(i + 1);
  mask_eptest[A4] = SetMask(B4);
  mask_eptest[H4] = SetMask(G4);
  mask_eptest[A5] = SetMask(B5);
  mask_eptest[H5] = SetMask(G5);
/*
 Initialize masks used to evaluate pawn races.  These masks are
 used to determine if the opposing king is in a position to stop a
 passed pawn from racing down and queening.  The data is organized
 as pawn_race[side][onmove][square], where side is black or white,
 and onmove indicates which side is to move for proper tempo
 evaluation.
 */
  for (i = 0; i < 64; i++) {
    pawn_race[white][white][i] = 0;
    pawn_race[white][black][i] = 0;
    pawn_race[black][white][i] = 0;
    pawn_race[black][black][i] = 0;
  }
  for (j = 8; j < 56; j++) {
    for (i = 0; i < 64; i++) {
/* white pawn, wtm */
      if (j < 16) {
        if (KingPawnSquare(j + 8, i, File(j) + 56, 1))
          pawn_race[white][white][j] |= SetMask(i);
      } else {
        if (KingPawnSquare(j, i, File(j) + 56, 1))
          pawn_race[white][white][j] |= SetMask(i);
      }
/* white pawn, btm */
      if (j < 16) {
        if (KingPawnSquare(j + 8, i, File(j) + 56, 0))
          pawn_race[white][black][j] |= SetMask(i);
      } else {
        if (KingPawnSquare(j, i, File(j) + 56, 0))
          pawn_race[white][black][j] |= SetMask(i);
      }
/* black pawn, wtm */
      if (j > 47) {
        if (KingPawnSquare(j - 8, i, File(j), 0))
          pawn_race[black][white][j] |= SetMask(i);
      } else {
        if (KingPawnSquare(j, i, File(j), 0))
          pawn_race[black][white][j] |= SetMask(i);
      }
/* black pawn, btm */
      if (j > 47) {
        if (KingPawnSquare(j - 8, i, File(j), 1))
          pawn_race[black][black][j] |= SetMask(i);
      } else {
        if (KingPawnSquare(j, i, File(j), 1))
          pawn_race[black][black][j] |= SetMask(i);
      }
    }
  }
}

/*
 *******************************************************************************
 *                                                                             *
 *   InitializeLMP() is used to initialize the LMP thresholds used to decide   *
 *   when to stop searching additional branches near the tips of the tree.     *
 *                                                                             *
 *******************************************************************************
 */
void InitializeLMP() {
  int i;

  for (i = 0; i < LMP_depth; i++)
    LMP[i] = LMP_base + pow(i + .5, LMP_scale);
}

/*
 *******************************************************************************
 *                                                                             *
 *   InitializeLMR() is used to initialize the reduction matrix used to set    *
 *   the reduction value for LMR for each move searched.  It is indexed by     *
 *   depth remaining and # moves searched.                                     *
 *                                                                             *
 *******************************************************************************
 */
void InitializeLMR() {
  int d, m;

  for (d = 0; d < 32; d++)
    for (m = 0; m < 64; m++)
      LMR[d][m] = 0;
  for (d = 3; d < 32; d++)
    for (m = 1; m < 64; m++) {
      LMR[d][m] =
          Max(Min(log(d * LMR_db) * log(m * LMR_mb) / LMR_s, LMR_max),
          LMR_min);
      LMR[d][m] = Min(LMR[d][m], Max(d - 1 - LMR_rdepth, 0));
    }
}

/*
 *******************************************************************************
 *                                                                             *
 *   InitlializeSMP() is used to initialize the pthread lock variables.        *
 *                                                                             *
 *******************************************************************************
 */
void InitializeSMP(void) {
  LockInit(lock_smp);
  LockInit(lock_io);
  LockInit(block[0]->lock);
#if defined(UNIX) && (CPUS > 1)
  pthread_attr_init(&attributes);
  pthread_attr_setdetachstate(&attributes, PTHREAD_CREATE_DETACHED);
#endif
}

/*
 *******************************************************************************
 *                                                                             *
 *   configuration information:  the following variables need to be set to     *
 *   indicate the machine configuration/capabilities.                          *
 *                                                                             *
 *   UNIX:  define this if the program is being run on a unix-based system,    *
 *   which causes the executable to use unix-specific runtime utilities.       *
 *                                                                             *
 *   CPUS=N:  this sets up data structures to the appropriate size to support  *
 *   up to N simultaneous search engines.  note that you can set this to a     *
 *   value larger than the max processors you currently have, because the mt=n *
 *   command (added to the command line or your crafty.rc/.craftyrc file) will *
 *   control how many threads are actually spawned.                            *
 *                                                                             *
 *******************************************************************************
 */
/* *INDENT-OFF* */

#if defined(AFFINITY)
#  define _GNU_SOURCE
#  include <sched.h>
#endif
#if defined(UNIX)
#  define _GNU_SOURCE
#  if (CPUS > 1)
#    include <pthread.h>
#  endif
#  include <unistd.h>
#  include <sys/types.h>
#  include <sys/stat.h>
#endif
#include <stdint.h>
#include <inttypes.h>
#include <time.h>
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#pragma once
#  if !defined (UNIX)
#    define RESTRICT __restrict
#  else
#    define RESTRICT
#  endif
#  if !defined(CPUS)
#    define CPUS 1
#  endif
#  if !defined(UNIX)
#    include <windows.h>
#    include <process.h>
#  endif
#  define CDECL
#  define STDCALL
/* Provide reasonable defaults for UNIX systems. */
#  if !defined(UNIX)
#    undef  STDCALL
#    define STDCALL __stdcall
#    ifdef  VC_INLINE32
#      undef  CDECL
#      define CDECL __cdecl
#    endif
#  endif
#  if !defined(BOOKDIR)
#    define   BOOKDIR      "."
#  endif
#  if !defined(LOGDIR)
#    define   LOGDIR       "."
#  endif
#  if !defined(TBDIR)
#    define   TBDIR        "./syzygy"
#  endif
#  if !defined(RCDIR)
#    define   RCDIR        "."
#  endif
#  include "lock.h"
#  define MAXPLY                                 129
#  define MAX_TC_NODES                       3000000
#  define MAX_BLOCKS                       64 * CPUS
#  define BOOK_CLUSTER_SIZE                     8000
#  define MERGE_BLOCK                           1000
#  define SORT_BLOCK                         4000000
#  define LEARN_INTERVAL                          10
#  define LEARN_COUNTER_BAD                      -80
#  define LEARN_COUNTER_GOOD                    +100
#  define MATE                                 32768
#  define TBWIN                                31000
#  define PAWN_VALUE                             100
#  define KNIGHT_VALUE                           305
#  define BISHOP_VALUE                           305
#  define ROOK_VALUE                             490
#  define QUEEN_VALUE                           1000
#  define KING_VALUE                           40000
#  define FOREVER                                  1
#  if !defined(CLOCKS_PER_SEC)
#    define CLOCKS_PER_SEC 1000000
#  endif
typedef enum {
  A1, B1, C1, D1, E1, F1, G1, H1,
  A2, B2, C2, D2, E2, F2, G2, H2,
  A3, B3, C3, D3, E3, F3, G3, H3,
  A4, B4, C4, D4, E4, F4, G4, H4,
  A5, B5, C5, D5, E5, F5, G5, H5,
  A6, B6, C6, D6, E6, F6, G6, H6,
  A7, B7, C7, D7, E7, F7, G7, H7,
  A8, B8, C8, D8, E8, F8, G8, H8,
  BAD_SQUARE
} squares;
typedef enum { FILEA, FILEB, FILEC, FILED, FILEE, FILEF, FILEG, FILEH } files;
typedef enum { RANK1, RANK2, RANK3, RANK4, RANK5, RANK6, RANK7, RANK8 } ranks;
typedef enum { empty = 0, occupied = 0, pawn = 1, knight = 2, bishop = 3,
  rook = 4, queen = 5, king = 6
} PIECE;
typedef enum { black = 0, white = 1 } COLOR;
typedef enum { mg = 0, eg = 1 } PHASE;
typedef enum { empty_v = 0, pawn_v = 1, knight_v = 3,
               bishop_v = 3, rook_v = 5, queen_v = 9, king_v = 99
} PIECE_V;
typedef enum { serial = 0, parallel = 1} SEARCH_MODE;
typedef enum { think = 1, puzzle = 2, book = 3, annotate = 4 } SEARCH_TYPE;
typedef enum { normal_mode, tournament_mode } PLAYING_MODE;
typedef struct {
  int8_t castle[2];
  uint8_t enpassant_target;
  uint8_t reversible;
} SEARCH_POSITION;
typedef struct {
  uint32_t move1;
  uint32_t move2;
} KILLER;
typedef struct {
  uint64_t pieces[7];
} BB_PIECES;
typedef struct {
  BB_PIECES color[2];
  uint64_t hash_key;
  uint64_t pawn_hash_key;
  int material_evaluation;
  int kingsq[2];
  int8_t board[64];
  char pieces[2][7];
  char total_all_pieces;
} POSITION;
typedef struct {
  uint64_t word1;
  uint64_t word2;
} HASH_ENTRY;
typedef struct {
  uint64_t key;
  int32_t score_mg, score_eg;
  unsigned char defects_k[2];
  unsigned char defects_m[2];
  unsigned char defects_q[2];
  unsigned char passed[2];
} PAWN_HASH_ENTRY;
typedef struct {
  uint64_t entry[3];
} PXOR;
typedef struct {
  int path[MAXPLY];
  int pathh;
  int pathl;
  int pathd;
  int pathv;
} PATH;
typedef struct {
  uint64_t path_sig;
  int hash_pathl;
  int hash_path_age;
  int hash_path_moves[MAXPLY];
} HPATH_ENTRY;
typedef struct {
  int phase;
  int order;
  int remaining;
  unsigned *last;
  unsigned done[10];
  unsigned *exclude;
} NEXT_MOVE;
/*
   root_move.status:

   xxx1 = failed low this iteration
   xx1x = failed high this iteration
   x1xx = don't search in parallel or reduce
   1xxx = move has been searched
 */
typedef struct {
  int move;
  unsigned status;
  int bm_age;
  PATH path;
} ROOT_MOVE;
#  if !defined(UNIX)
#    pragma pack(4)
#  endif
typedef struct {
  uint64_t position;
  unsigned status_played;
  float learn;
} BOOK_POSITION;
#  if !defined(UNIX)
#    pragma pack()
#  endif
typedef struct {
  unsigned char position[8];
  unsigned char status;
  unsigned char percent_play;
} BB_POSITION;
struct personality_term {
  char *description;
  int type;
  int size;
  void *value;
};
struct autotune {
  unsigned int min;
  unsigned int max;
  unsigned int increment;
  char description[64];
  char command[16];
  unsigned int *parameter;
};
typedef struct tree {
/* commonly used variables */
  SEARCH_POSITION status[MAXPLY + 3];
  NEXT_MOVE next_status[MAXPLY];
  KILLER killers[MAXPLY];
  KILLER counter_move[4096];
  KILLER move_pair[4096];
  POSITION position;
  uint64_t save_hash_key[MAXPLY + 3];
  uint64_t save_pawn_hash_key[MAXPLY + 3];
  uint64_t rep_list[256];
  int curmv[MAXPLY];
  int phase[MAXPLY];
  int hash_move[MAXPLY];
  unsigned *last[MAXPLY];
  unsigned move_list[5120];
  PATH pv[MAXPLY];
/* variables used by Evaluate() */
  PAWN_HASH_ENTRY pawn_score;
  int tropism[2];
  int dangerous[2];
  uint64_t all_pawns;
  int score_mg, score_eg;
/* statistical counters */
  uint64_t nodes_searched;
  uint64_t fail_highs;
  uint64_t fail_high_first_move;
  uint64_t evaluations;
  uint64_t egtb_probes;
  uint64_t egtb_hits;
  uint64_t extensions_done;
  uint64_t qchecks_done;
  uint64_t moves_fpruned;
  uint64_t moves_mpruned;
  uint64_t LMR_done[16];
  uint64_t null_done[16];
/* thread stuff */
  lock_t lock;
  int thread_id;
  volatile int joinable;
  volatile int joined;
  volatile int stop;
  volatile int nprocs;
  int alpha;
  int beta;
  volatile int value;
  int wtm;
  int depth;
  int ply;
  int in_check;
  int *searched;
  int cutmove;
  struct tree *volatile siblings[CPUS], *parent;
/* rarely accessed */
  char root_move_text[16];
  char remaining_moves_text[16];
} TREE;
typedef struct thread {
  TREE *tree;
  uint64_t blocks;
  uint64_t max_blocks;
  unsigned int idle;
  volatile unsigned int terminate;
  char filler[40];
} THREAD;
/*
   DO NOT modify these.  these are constants, used in multiple modules.
   modification may corrupt the search in any number of ways, all bad.
 */
#  define WORTHLESS                    0
#  define UPPER                        1
#  define LOWER                        2
#  define EXACT                        3
#  define HASH_MISS                    0
#  define HASH_HIT                     1
#  define AVOID_NULL_MOVE              2
#  define NO_NULL                      0
#  define DO_NULL                      1
#  define NONE                         0
#  define NULL_MOVE                    1
#  define DO_NOT_REDUCE                1
#  define HASH                         2
#  define GENERATE_CAPTURES            3
#  define CAPTURES                     4
#  define KILLER1                      5
#  define KILLER2                      6
#  define KILLER3                      7
#  define KILLER4                      8
#  define COUNTER_MOVE1                9
#  define COUNTER_MOVE2               10
#  define MOVE_PAIR1                  11
#  define MOVE_PAIR2                  12
#  define GENERATE_QUIET              13
#  define HISTORY                     14
#  define REMAINING                   15
#  define EVALUATION                  16
#  define ILLEGAL                      0
#  define LEGAL                        1
#  define IN_WINDOW                    2
#  define FAIL_HIGH                    3
#define PopCnt(v) __builtin_popcountll(v)
#define LSB(v)    __builtin_ctzll(v)
#define MSB(v)    (63 - __builtin_clzll(v))
void AlignedMalloc(void **, uint64_t, size_t);
void AlignedRemalloc(void **, uint64_t, size_t);
void Analyze(void);
void Annotate(void);
void AnnotateHeaderHTML(char *, FILE *);
void AnnotateFooterHTML(FILE *);
void AnnotatePositionHTML(TREE *RESTRICT, int, FILE *);
char *AnnotateVtoNAG(int, int, int, int);
void AnnotateHeaderTeX(FILE *);
void AnnotateFooterTeX(FILE *);
void AnnotatePositionTeX(TREE *, int, FILE *);
uint64_t atoiKMB(char *);
int Attacks(TREE *RESTRICT, int, int);
uint64_t Attacked(TREE *RESTRICT, int, uint64_t);
uint64_t AttacksFrom(TREE *RESTRICT, int, int);
uint64_t AttacksTo(TREE *RESTRICT, int);
void AutoTune(int, char **);
int Bench(int, int);
int Book(TREE *RESTRICT, int);
void BookClusterIn(FILE *, int, BOOK_POSITION *);
void BookClusterOut(FILE *, int, BOOK_POSITION *);
int BookIn32(unsigned char *);
float BookIn32f(unsigned char *);
uint64_t BookIn64(unsigned char *);
int BookMask(char *);
unsigned char *BookOut32(int);
unsigned char *BookOut32f(float);
unsigned char *BookOut64(uint64_t);
int BookPonderMove(TREE *RESTRICT, int);
void Bookup(TREE *RESTRICT, int, char **);
void BookSort(BB_POSITION *, int, int);
int BookupCompare(const void *, const void *);
BB_POSITION BookupNextPosition(int, int);
int CheckInput(void);
void ClearHashTableScores(void);
int ComputeDifficulty(int, int);
void CopyFromParent(TREE *RESTRICT);
void CopyToParent(TREE *RESTRICT, TREE *RESTRICT, int);
void CraftyExit(int);
void DisplayArray(int *, int);
void DisplayArrayX2(int *, int *, int);
void DisplayBitBoard(uint64_t);
void Display2BitBoards(uint64_t, uint64_t);
void DisplayChessBoard(FILE *, POSITION);
char *DisplayEvaluation(int, int);
char *DisplayEvaluationKibitz(int, int);
void DisplayFT(int, int, int);
char *DisplayHHMM(unsigned);
char *DisplayHHMMSS(unsigned);
char *DisplayKMB(uint64_t, int);
void DisplayFail(TREE *RESTRICT, int, int, int, int, int, int, int);
char *DisplayPath(TREE *RESTRICT, int, PATH *);
void DisplayPV(TREE *RESTRICT, int, int, int, PATH *, int);
char *DisplayTime(unsigned);
char *Display2Times(unsigned);
char *DisplayTimeKibitz(unsigned);
void DisplayChessMove(char *, int);
int Drawn(TREE *RESTRICT, int);
int  DTZtoWDL(int, int);
void Edit(void);
int Evaluate(TREE *RESTRICT, int, int, int, int);
void EvaluateBishops(TREE *RESTRICT, int);
void EvaluateCastling(TREE *RESTRICT, int, int);
int EvaluateDraws(TREE *RESTRICT, int, int, int);
int EvaluateHasOpposition(int, int, int);
void EvaluateKing(TREE *RESTRICT, int, int);
int EvaluateKingsFile(TREE * RESTRICT, int, int, int);
void EvaluateKnights(TREE *RESTRICT, int);
void EvaluateMate(TREE *RESTRICT, int);
void EvaluateMaterial(TREE *RESTRICT, int);
void EvaluatePassedPawns(TREE *RESTRICT, int, int);
void EvaluatePassedPawnRaces(TREE *RESTRICT, int);
void EvaluatePawns(TREE *RESTRICT, int);
void EvaluateQueens(TREE *RESTRICT, int);
void EvaluateRooks(TREE *RESTRICT, int);
int EvaluateWinningChances(TREE *RESTRICT, int, int);
void EVTest(char *);
int Exclude(TREE *RESTRICT, int, int);
int FindBlockID(TREE *RESTRICT);
char *FormatPV(TREE *RESTRICT, int, PATH);
int FTbSetCacheSize(void *, unsigned long);
int GameOver(int);
unsigned *GenerateCaptures(TREE *RESTRICT, int, int, unsigned *);
unsigned *GenerateCheckEvasions(TREE *RESTRICT, int, int, unsigned *);
unsigned *GenerateChecks(TREE *RESTRICT, int, unsigned *);
unsigned *GenerateNoncaptures(TREE *RESTRICT, int, int, unsigned *);
TREE *GetBlock(TREE *, int);
void Initialize(void);
void InitializeAttackBoards(void);
void InitializeChessBoard(TREE *);
int InitializeGetLogID();
void InitializeHashTables(int);
void InitializeKillers(void);
void InitializeKingSafety(void);
void InitializeMagic(void);
uint64_t InitializeMagicBishop(int, uint64_t);
uint64_t InitializeMagicRook(int, uint64_t);
uint64_t InitializeMagicOccupied(int *, int, uint64_t);
void InitializeMasks(void);
void InitializePawnMasks(void);
void InitializeLMP(void);
void InitializeLMR(void);
void InitializeSMP(void);
int IInitializeTb(char *);
int InputMove(TREE *RESTRICT, int, int, int, int, char *);
int InputMoveICS(TREE *RESTRICT, int, int, int, int, char *);
uint64_t InterposeSquares(int, int, int);
void Interrupt(int);
int InvalidPosition(TREE *RESTRICT);
int Iterate(int, int, int);
int Join(int64_t);
void Kibitz(int, int, int, int, int, uint64_t, int, int, char *);
void History(TREE *RESTRICT, int, int, int, int, int*);
int KingPawnSquare(int, int, int, int);
int LearnAdjust(int);
void LearnBook(void);
int LearnFunction(int, int, int, int);
void LearnValue(int, int);
void MakeMove(TREE *RESTRICT, int, int, int);
void MakeMoveRoot(TREE *RESTRICT, int, int);
int Mated(TREE *RESTRICT, int, int);
int RootMoveEGTB(int);
int NextMove(TREE *RESTRICT, int, int, int, int);
int NextRootMove(TREE *RESTRICT, TREE *RESTRICT, int);
int NextRootMoveParallel(void);
void NextSort(TREE *RESTRICT, int);
int Option(TREE *RESTRICT);
int OptionMatch(char *, char *);
void OptionPerft(TREE *RESTRICT, int, int, int);
void Output(TREE *RESTRICT);
char *OutputMove(TREE *RESTRICT, int, int, int);
int ParseTime(char *);
void Pass(void);
int PinnedOnKing(TREE *RESTRICT, int, int);
int Ponder(int);
void Print(int, char *, ...);
int ProbeDTZ(TREE * RESTRICT tree, int ply, int wtm);
int HashProbe(TREE *RESTRICT, int, int, int, int, int, int*);
void HashStore(TREE *RESTRICT, int, int, int, int, int, int);
void HashStorePV(TREE *RESTRICT, int, int);
int Quiesce(TREE *RESTRICT, int, int, int, int, int);
int QuiesceEvasions(TREE *RESTRICT, int, int, int, int);
unsigned Random32(void);
uint64_t Random64(void);
int Read(int, char *);
int ReadChessMove(TREE *RESTRICT, FILE *, int, int);
void ReadClear(void);
unsigned ReadClock(void);
int ReadPGN(FILE *, int);
int ReadNextMove(TREE *RESTRICT, char *, int, int);
int ReadParse(char *, char *args[], char *);
int ReadInput(void);
int Repeat(TREE *RESTRICT, int);
int Repeat3x(TREE *RESTRICT);
void ResignOrDraw(TREE *RESTRICT, int);
void RestoreGame(void);
void RootMoveList(int);
int Search(TREE *RESTRICT, int, int, int, int, int, int, int);
int SearchMove(TREE *RESTRICT, int, int, int, int, int, int, int, int, int);
int SearchMoveList(TREE *RESTRICT, int, int, int, int, int, int *, int, int, int);
int SearchNull(TREE * RESTRICT, int, int, int, int);
void Trace(TREE *RESTRICT, int, int, int, int, int, const char *, int, int, int);
void SetBoard(TREE *, int, char **, int);
void SetChessBitBoards(TREE *);
void SharedFree(void *address);
void SortRootMoves(TREE *RESTRICT, int);
int Split(TREE *RESTRICT);
int StrCnt(char *, char);
int SEE(TREE *RESTRICT, int, int);
int SEEO(TREE *RESTRICT, int, int);
void Test(char *, FILE *, int, int);
void TestEPD(char *, FILE *, int, int);
void ThreadAffinity(int);
void *STDCALL ThreadInit(void *);
#  if !defined(UNIX)
void ThreadMalloc(int64_t);
#  endif
int ThreadSplit(TREE *RESTRICT, int, int, int, int, int);
void ThreadStop(TREE *RESTRICT);
void ThreadTrace(TREE * RESTRICT, int, int);
int ThreadWait(int, TREE *RESTRICT);
int Threat(TREE *, int, int, int, int);
void TimeAdjust(int, int);
int TimeCheck(TREE *RESTRICT, int);
void TimeSet(int);
void UnmakeMove(TREE *RESTRICT, int, int, int);
int ValidMove(TREE *RESTRICT, int, int, int);
int VerifyMove(TREE *RESTRICT, int, int, int);
void ValidatePosition(TREE *RESTRICT, int, int, char *);
void WaitForAllThreadsInitialized(void);
#  if !defined(UNIX)
extern void *WinMallocInterleaved(size_t, int);
extern void WinFreeInterleaved(void *, size_t);
#    define MallocInterleaved(cBytes, cThreads)     \
       WinMallocInterleaved(cBytes, cThreads)
#    define FreeInterleaved(pMemory, cBytes)        \
       WinFreeInterleaved(pMemory, cBytes)
#  else
#    if defined(NUMA)
#      define MallocInterleaved(cBytes, cThreads) numa_alloc_interleaved(cBytes)
#      define FreeInterleaved(pMemory, cBytes)          numa_free(pMemory, 1)
#    else
#      define MallocInterleaved(cBytes, cThreads) malloc(cBytes)
#      define FreeInterleaved(pMemory, cBytes)          free(pMemory)
#    endif
#  endif
#  define Abs(a)    (((a) > 0) ? (a) : -(a))
#  define Max(a,b)  (((a) > (b)) ? (a) : (b))
#  define Min(a,b)  (((a) < (b)) ? (a) : (b))
#  define Sign(a)   ((a) < 0 ? -1 : +1)
#  define FileDistance(a,b) abs(File(a) - File(b))
#  define RankDistance(a,b) abs(Rank(a) - Rank(b))
#  define Distance(a,b) Max(FileDistance(a,b), RankDistance(a,b))
#  define DrawScore(side)                  (draw_score[side])
#  define PopCnt8Bit(a) (pop_cnt_8bit[a])
#  define MSB8Bit(a) (msb_8bit[a])
#  define LSB8Bit(a) (lsb_8bit[a])
#  define HistoryIndex(side, m) ((side << 9) + (Piece(m) << 6) + To(m))
/*
   side = side to move
   mptr = pointer into move list
   m = bit vector of to squares to unpack
   t = pre-computed from + moving piece
 */
#  define Extract(side, mptr, m, t)                                         \
  for ( ; m ; Clear(to, m)) {                                               \
    to = MostAdvanced(side, m);                                             \
    *mptr++ = t | (to << 6) | (Abs(PcOnSq(to)) << 15);                      \
  }
#  define Check(side) Attacks(tree, Flip(side), KingSQ(side))
#  define Attack(from,to) (!(intervening[from][to] & OccupiedSquares))
#  define BishopAttacks(square, occ) *(magic_bishop_indices[square]+((((occ)&magic_bishop_mask[square])*magic_bishop[square])>>magic_bishop_shift[square]))
#  define BishopMobility(square, occ) *(magic_bishop_mobility_indices[square]+((((occ)&magic_bishop_mask[square])*magic_bishop[square])>>magic_bishop_shift[square]))
#  define KingAttacks(square) king_attacks[square]
#  define KnightAttacks(square) knight_attacks[square]
#  define PawnAttacks(side, square)   pawn_attacks[side][square]
#  define Reversible(p)               (tree->status[p].reversible)
#  define ReversibleMove(m)           (!CaptureOrPromote(m) && Piece(m) != pawn)
#  define RookAttacks(square, occ) *(magic_rook_indices[square]+((((occ)&magic_rook_mask[square])*magic_rook[square])>>magic_rook_shift[square]))
#  define RookMobility(square, occ) *(magic_rook_mobility_indices[square]+((((occ)&magic_rook_mask[square])*magic_rook[square])>>magic_rook_shift[square]))
#  define QueenAttacks(square, occ)   (BishopAttacks(square, occ)|RookAttacks(square, occ))
#  define Rank(x)        ((x)>>3)
#  define File(x)        ((x)&7)
#  define Flip(x)        ((x)^1)
#  define MostAdvanced(side, squares) ((side) ? MSB(squares) : LSB(squares))
#  define LeastAdvanced(side, squares) ((side) ? LSB(squares) : MSB(squares))
#  define MinMax(side, v1, v2) ((side) ? Min((v1), (v2)) : Max((v1), (v2)))
#  define InFront(side, k, p) ((side) ? k > p : k < p)
#  define Behind(side, k, p) ((side) ? k < p : k > p)
#  define Passed(sq, wtm)        (!(mask_passed[wtm][sq] & Pawns(Flip(wtm))))
#  define RankAttacks(a) (RookAttacks(a, OccupiedSquares) & rank_mask[Rank(a)])
#  define FileAttacks(a) (RookAttacks(a, OccupiedSquares) & file_mask[File(a)])
#  define Diaga1Attacks(a) (BishopAttacks(a, OccupiedSquares) & (plus9dir[a] | minus9dir[a]))
#  define Diagh1Attacks(a) (BishopAttacks(a, OccupiedSquares) & (plus7dir[a] | minus7dir[a]))
#  define InterposeSquares(kingsq, checksq) intervening[kingsq][checksq]
/*
   the following macros are used to extract the pieces of a move that are
   kept compressed into the rightmost 21 bits of a simple integer.
 */
#  define From(a)               ((a) & 63)
#  define To(a)                 (((a)>>6) & 63)
#  define Piece(a)              (((a)>>12) & 7)
#  define Captured(a)           (((a)>>15) & 7)
#  define Promote(a)            (((a)>>18) & 7)
#  define Move(a)               (a & 0x1fffff)
#  define SortV(a)              (a >> 21)
#  define CaptureOrPromote(a)   (((a)>>15) & 63)
#  define PawnPush(c, a)        (Piece(a) == pawn &&                          \
                                     rankflip[c][Rank(To(a))] >= RANK6        \
                                     && !(mask_passed[c][To(a)] &             \
                                          Pawns(Flip(c))))
#  define CastleMove(c, a)       (Piece(a) == king && Abs(File(To(a)) -       \
                                     File(From(a))) > 1)
#  define SetMask(a)             (set_mask[a])
#  define ClearMask(a)           (clear_mask[a])
#  define Pawns(c)               (tree->position.color[c].pieces[pawn])
#  define Knights(c)             (tree->position.color[c].pieces[knight])
#  define Bishops(c)             (tree->position.color[c].pieces[bishop])
#  define Rooks(c)               (tree->position.color[c].pieces[rook])
#  define Queens(c)              (tree->position.color[c].pieces[queen])
#  define Kings(c)               (tree->position.color[c].pieces[king])
#  define KingSQ(c)              (tree->position.kingsq[c])
#  define Occupied(c)            (tree->position.color[c].pieces[occupied])
#  define Pieces(c, p)           (tree->position.color[c].pieces[p])
#  define TotalPieces(c, p)      (tree->position.pieces[c][p])
#  define PieceValues(c, p)      (piece_values[c][p])
#  define TotalAllPieces         (tree->position.total_all_pieces)
#  define Material               (tree->position.material_evaluation)
#  define MaterialSTM(side)      ((side) ? Material : -Material)
#  define MateScore(s)           (Abs(s) > 32000)
#  define EGTBScore(s)           (Abs(s) > 30000 && Abs(s) < 32000)
#  define Castle(ply, c)         (tree->status[ply].castle[c])
#  define HashKey                (tree->position.hash_key)
#  define PawnHashKey            (tree->position.pawn_hash_key)
#  define EnPassant(ply)         (tree->status[ply].enpassant_target)
#  define EnPassantTarget(ply)   (EnPassant(ply) ? SetMask(EnPassant(ply)) : 0)
#  define PcOnSq(sq)             (tree->position.board[sq])
#  define OccupiedSquares        (Occupied(white) | Occupied(black))
#  define Color(square)       (square_color[square] ? dark_squares : ~dark_squares)
#  define SideToMove(c)          ((c) ? "White" : "Black")
/*
   the following macros are used to Set and Clear a specific bit in the
   second argument.  this is done to make the code more readable, rather
   than to make it faster.
 */
#  define ClearSet(a,b)         b=((a) ^ (b))
#  define Clear(a,b)            b=ClearMask(a) & (b)
#  define Set(a,b)              b=SetMask(a) | (b)
/*
   the following macros are used to update the hash signatures.
 */
#  define Hash(stm,piece,square)     (HashKey^=randoms[stm][piece][square])
#  define HashP(stm,square)          (PawnHashKey^=randoms[stm][pawn][square])
#  define HashCastle(stm,direction)  (HashKey^=castle_random[stm][direction])
#  define HashEP(sq)                 (HashKey^=enpassant_random[sq])
#  define SavePV(tree,ply,ph)        do {                                     \
        tree->pv[ply-1].path[ply-1]=tree->curmv[ply-1];                       \
        tree->pv[ply-1].pathl=ply;                                            \
        tree->pv[ply-1].pathh=ph;                                             \
        tree->pv[ply-1].pathd=iteration;} while(0)
#  if defined(INLINEASM)
#    include "inline.h"
#  endif
/* *INDENT-ON* */

#include <stdarg.h>
#include <errno.h>
#include <ctype.h>
#include "chess.h"
#include "data.h"
#if defined(UNIX)
#  include <unistd.h>
#  include <sys/types.h>
#  include <signal.h>
#  include <sys/wait.h>
#  include <sys/times.h>
#  include <sys/time.h>
#else
#  include <windows.h>
#  include <winbase.h>
#  include <wincon.h>
#  include <io.h>
#  include <time.h>
#endif

/*
 *******************************************************************************
 *                                                                             *
 *   AlignedMalloc() is used to allocate memory on a precise boundary,         *
 *   primarily to optimize cache performance by forcing the start of the       *
 *   memory region being allocated to match up so that a structure will lie    *
 *   on a single cache line rather than being split across two, assuming the   *
 *   structure is 64 bytes or less of course.                                  *
 *                                                                             *
 *******************************************************************************
 */
void AlignedMalloc(void **pointer, uint64_t alignment, size_t size) {
  uint64_t temp;

  segments[nsegments][0] = malloc(size + alignment - 1);
  segments[nsegments][1] = segments[nsegments][0];
  temp = (uint64_t) segments[nsegments][0];
  temp = (temp + alignment - 1) & ~(alignment - 1);
  segments[nsegments][1] = (void *) temp;
  *pointer = segments[nsegments][1];
  nsegments++;
}

/*
 *******************************************************************************
 *                                                                             *
 *   atoiKMB() is used to read in an integer value that can have a "K" or "M"  *
 *   appended to it to multiply by 1024 or 1024*1024.  It returns a 64 bit     *
 *   value since memory sizes can exceed 4gb on modern hardware.               *
 *                                                                             *
 *******************************************************************************
 */
uint64_t atoiKMB(char *input) {
  uint64_t size;

  size = atoi(input);
  if (strchr(input, 'K') || strchr(input, 'k'))
    size *= 1 << 10;
  if (strchr(input, 'M') || strchr(input, 'm'))
    size *= 1 << 20;
  if (strchr(input, 'B') || strchr(input, 'b') || strchr(input, 'G') ||
      strchr(input, 'g'))
    size *= 1 << 30;
  return size;
}

/*
 *******************************************************************************
 *                                                                             *
 *   AlignedRemalloc() is used to change the size of a memory block that has   *
 *   previously been allocated using AlignedMalloc().                          *
 *                                                                             *
 *******************************************************************************
 */
void AlignedRemalloc(void **pointer, uint64_t alignment, size_t size) {
  uint64_t temp;
  int i;

  for (i = 0; i < nsegments; i++)
    if (segments[i][1] == *pointer)
      break;
  if (i == nsegments) {
    Print(4095, "ERROR  AlignedRemalloc() given an invalid pointer\n");
    exit(1);
  }
  free(segments[i][0]);
  segments[i][0] = malloc(size + alignment - 1);
  temp = (uint64_t) segments[i][0];
  temp = (temp + alignment - 1) & ~(alignment - 1);
  segments[i][1] = (void *) temp;
  *pointer = segments[i][1];
}

/*
 *******************************************************************************
 *                                                                             *
 *   BookClusterIn() is used to read a cluster in as characters, then stuff    *
 *   the data into a normal array of structures that can be used within Crafty *
 *   without any endian issues.                                                *
 *                                                                             *
 *******************************************************************************
 */
void BookClusterIn(FILE * file, int positions, BOOK_POSITION * buffer) {
  int i;
  char file_buffer[BOOK_CLUSTER_SIZE * sizeof(BOOK_POSITION)];

  i = fread(file_buffer, positions, sizeof(BOOK_POSITION), file);
  if (i <= 0)
    perror("BookClusterIn fread error: ");
  for (i = 0; i < positions; i++) {
    buffer[i].position =
        BookIn64((unsigned char *) (file_buffer + i * sizeof(BOOK_POSITION)));
    buffer[i].status_played =
        BookIn32((unsigned char *) (file_buffer + i * sizeof(BOOK_POSITION) +
            8));
    buffer[i].learn =
        BookIn32f((unsigned char *) (file_buffer + i * sizeof(BOOK_POSITION) +
            12));
  }
}

/*
 *******************************************************************************
 *                                                                             *
 *   BookClusterOut() is used to write a cluster out as characters, after      *
 *   converting the normal array of structures into character data that is     *
 *   Endian-independent.                                                       *
 *                                                                             *
 *******************************************************************************
 */
void BookClusterOut(FILE * file, int positions, BOOK_POSITION * buffer) {
  int i;
  char file_buffer[BOOK_CLUSTER_SIZE * sizeof(BOOK_POSITION)];

  for (i = 0; i < positions; i++) {
    memcpy(file_buffer + i * sizeof(BOOK_POSITION),
        BookOut64(buffer[i].position), 8);
    memcpy(file_buffer + i * sizeof(BOOK_POSITION) + 8,
        BookOut32(buffer[i].status_played), 4);
    memcpy(file_buffer + i * sizeof(BOOK_POSITION) + 12,
        BookOut32f(buffer[i].learn), 4);
  }
  fwrite(file_buffer, positions, sizeof(BOOK_POSITION), file);
}

/*
 *******************************************************************************
 *                                                                             *
 *   BookIn32f() is used to convert 4 bytes from the book file into a valid 32 *
 *   bit binary value.  This eliminates endian worries that make the binary    *
 *   book non-portable across many architectures.                              *
 *                                                                             *
 *******************************************************************************
 */
float BookIn32f(unsigned char *ch) {
  union {
    float fv;
    int iv;
  } temp;

  temp.iv = ch[3] << 24 | ch[2] << 16 | ch[1] << 8 | ch[0];
  return temp.fv;
}

/*
 *******************************************************************************
 *                                                                             *
 *   BookIn32() is used to convert 4 bytes from the book file into a valid 32  *
 *   bit binary value.  this eliminates endian worries that make the  binary   *
 *   book non-portable across many architectures.                              *
 *                                                                             *
 *******************************************************************************
 */
int BookIn32(unsigned char *ch) {
  return ch[3] << 24 | ch[2] << 16 | ch[1] << 8 | ch[0];
}

/*
 *******************************************************************************
 *                                                                             *
 *   BookIn64() is used to convert 8 bytes from the book file into a valid 64  *
 *   bit binary value.  this eliminates endian worries that make the  binary   *
 *   book non-portable across many architectures.                              *
 *                                                                             *
 *******************************************************************************
 */
uint64_t BookIn64(unsigned char *ch) {
  return (uint64_t) ch[7] << 56 | (uint64_t) ch[6] << 48 | (uint64_t)
      ch[5] << 40 | (uint64_t) ch[4] << 32 | (uint64_t) ch[3]
      << 24 | (uint64_t) ch[2] << 16 | (uint64_t) ch[1] << 8 | (uint64_t)
      ch[0];
}

/*
 *******************************************************************************
 *                                                                             *
 *   BookOut32() is used to convert 4 bytes from a valid 32 bit binary value   *
 *   to a book value.  this eliminates endian worries that make the  binary    *
 *   book non-portable across many architectures.                              *
 *                                                                             *
 *******************************************************************************
 */
unsigned char *BookOut32(int val) {
  convert_buff[3] = val >> 24 & 0xff;
  convert_buff[2] = val >> 16 & 0xff;
  convert_buff[1] = val >> 8 & 0xff;
  convert_buff[0] = val & 0xff;
  return convert_buff;
}

/*
 *******************************************************************************
 *                                                                             *
 *   BookOut32f() is used to convert 4 bytes from a valid 32 bit binary value  *
 *   to a book value.  this eliminates endian worries that make the  binary    *
 *   book non-portable across many architectures.                              *
 *                                                                             *
 *******************************************************************************
 */
unsigned char *BookOut32f(float val) {
  union {
    float fv;
    int iv;
  } temp;

  temp.fv = val;
  convert_buff[3] = temp.iv >> 24 & 0xff;
  convert_buff[2] = temp.iv >> 16 & 0xff;
  convert_buff[1] = temp.iv >> 8 & 0xff;
  convert_buff[0] = temp.iv & 0xff;
  return convert_buff;
}

/*
 *******************************************************************************
 *                                                                             *
 *   BookOut64() is used to convert 8 bytes from a valid 64 bit binary value   *
 *   to a book value.  this eliminates endian worries that make the  binary    *
 *   book non-portable across many architectures.                              *
 *                                                                             *
 *******************************************************************************
 */
unsigned char *BookOut64(uint64_t val) {
  convert_buff[7] = val >> 56 & 0xff;
  convert_buff[6] = val >> 48 & 0xff;
  convert_buff[5] = val >> 40 & 0xff;
  convert_buff[4] = val >> 32 & 0xff;
  convert_buff[3] = val >> 24 & 0xff;
  convert_buff[2] = val >> 16 & 0xff;
  convert_buff[1] = val >> 8 & 0xff;
  convert_buff[0] = val & 0xff;
  return convert_buff;
}

/*
 *******************************************************************************
 *                                                                             *
 *   the following functions are used to determine if keyboard input is        *
 *   present.  there are several ways this is done depending on which          *
 *   operating system is used.  The primary function name is CheckInput() but  *
 *   for simplicity there are several O/S-specific versions.                   *
 *                                                                             *
 *******************************************************************************
 */
#if !defined(UNIX)
#  include <windows.h>
#  include <conio.h>
/* Windows NT using PeekNamedPipe() function */
int CheckInput(void) {
  int i;
  static int init = 0, pipe;
  static HANDLE inh;
  DWORD dw;

  if (!xboard && !isatty(fileno(stdin)))
    return 0;
  if (batch_mode)
    return 0;
  if (strchr(cmd_buffer, '\n'))
    return 1;
  if (xboard) {
#  if defined(FILE_CNT)
    if (stdin->_cnt > 0)
      return stdin->_cnt;
#  endif
    if (!init) {
      init = 1;
      inh = GetStdHandle(STD_INPUT_HANDLE);
      pipe = !GetConsoleMode(inh, &dw);
      if (!pipe) {
        SetConsoleMode(inh, dw & ~(ENABLE_MOUSE_INPUT | ENABLE_WINDOW_INPUT));
        FlushConsoleInputBuffer(inh);
      }
    }
    if (pipe) {
      if (!PeekNamedPipe(inh, NULL, 0, NULL, &dw, NULL)) {
        return 1;
      }
      return dw;
    } else {
      GetNumberOfConsoleInputEvents(inh, &dw);
      return dw <= 1 ? 0 : dw;
    }
  } else {
    i = _kbhit();
  }
  return i;
}
#endif
#if defined(UNIX)
/* Simple UNIX approach using select with a zero timeout value */
int CheckInput(void) {
  fd_set readfds;
  struct timeval tv;
  int data;

  if (!xboard && !isatty(fileno(stdin)))
    return 0;
  if (batch_mode)
    return 0;
  if (strchr(cmd_buffer, '\n'))
    return 1;
  FD_ZERO(&readfds);
  FD_SET(fileno(stdin), &readfds);
  tv.tv_sec = 0;
  tv.tv_usec = 0;
  select(16, &readfds, 0, 0, &tv);
  data = FD_ISSET(fileno(stdin), &readfds);
  return data;
}
#endif

/*
 *******************************************************************************
 *                                                                             *
 *   ClearHashTableScores() is used to clear hash table scores without         *
 *   clearing the best move, so that move ordering information is preserved.   *
 *   We clear the scorew as we approach a 50 move rule so that hash scores     *
 *   won't give us false scores since the hash signature does not include any  *
 *   search path information in it.                                            *
 *                                                                             *
 *******************************************************************************
 */
void ClearHashTableScores(void) {
  int i;

  if (hash_table)
    for (i = 0; i < hash_table_size; i++) {
      (hash_table + i)->word2 ^= (hash_table + i)->word1;
      (hash_table + i)->word1 =
          ((hash_table + i)->word1 & mask_clear_entry) | (uint64_t) 65536;
      (hash_table + i)->word2 ^= (hash_table + i)->word1;
    }
}

/* last modified 02/28/14 */
/*
 *******************************************************************************
 *                                                                             *
 *   ComputeDifficulty() is used to compute the difficulty rating for the      *
 *   current position, which really is based on nothing more than how many     *
 *   times we changed our mind in an iteration.  No changes caused the         *
 *   difficulty to drop (easier, use less time), while more changes ramps the  *
 *   difficulty up (harder, use more time).  It is called at the end of an     *
 *   iteration as well as when displaying fail-high/fail-low moves, in an      *
 *   effort to give the operator a heads-up on how long we are going to be     *
 *   stuck in an active search.                                                *
 *                                                                             *
 *******************************************************************************
 */
int ComputeDifficulty(int difficulty, int direction) {
  int searched = 0, i;

/*
 ************************************************************
 *                                                          *
 *  Step 1.  Handle fail-high-fail low conditions, which    *
 *  occur in the middle of an iteration.  The actions taken *
 *  are as follows:                                         *
 *                                                          *
 *  (1) Determine how many moves we have searched first, as *
 *  this is important.  If we have not searched anything    *
 *  (which means we failed high on the first move at the    *
 *  root, at the beginning of a new iteration), a fail low  *
 *  will immediately set difficult back to 100% (if it is   *
 *  currently below 100%).  A fail high on the first move   *
 *  will not change difficulty at all.  Successive fail     *
 *  highs or fail lows will not change difficulty, we will  *
 *  not even get into this code on the repeats.             *
 *                                                          *
 *  (2) If we are beyond the first move, then this must be  *
 *  a fail high condition.  Since we are changing our mind, *
 *  we need to increase the difficulty level to expend more *
 *  time on this iteration.  If difficulty is currently     *
 *  less than 100%, we set it to 120%.  If it is currently  *
 *  at 100% or more, we simply add 20% to the value and     *
 *  continue searching, but with a longer time constraint.  *
 *  Each time we fail high, we are changing our mind, and   *
 *  we will increase difficulty by another 20%.             *
 *                                                          *
 *  (3) Direction = 0 means we are at the end of an the     *
 *  iteration.  Here we simply note if we changed our mind  *
 *  during this iteration.  If not, we reduce difficulty    *
 *  to 90% of its previous value.                           *
 *                                                          *
 *  After any of these changes, we enforce a lower bound of *
 *  60% and an upperbound of 200% before we return.         *
 *                                                          *
 *  Note:  direction = +1 means we failed high on the move, *
 *  direction = -1 means we failed low on the move, and     *
 *  direction = 0 means we have completed the iteration and *
 *  all moves were searched successfully.                   *
 *                                                          *
 ************************************************************
 */
  if (direction) {
    for (i = 0; i < n_root_moves; i++)
      if (root_moves[i].status & 8)
        searched++;
    if (searched == 0) {
      if (direction > 0)
        return difficulty;
      if (direction < 0)
        difficulty = Max(100, difficulty);
    } else {
      if (difficulty < 100)
        difficulty = 120;
      else
        difficulty = difficulty + 20;
    }
  }
/*
 ************************************************************
 *                                                          *
 *  Step 2.  We are at the end of an iteration.  If we did  *
 *  not change our mind and stuck with one move, we reduce  *
 *  difficulty by 10% since the move looks to be a little   *
 *  "easier" when we don't change our mind.                 *
 *                                                          *
 ************************************************************
 */
  else {
    searched = 0;
    for (i = 0; i < n_root_moves; i++)
      if (root_moves[i].bm_age == 3)
        searched++;
    if (searched <= 1)
      difficulty = 90 * difficulty / 100;
  }
/*
 ************************************************************
 *                                                          *
 *  Step 4.  Apply limits.  We don't let difficulty go      *
 *  above 200% (take 2x the target time) nor do we let it   *
 *  drop below 60 (take .6x target time) to avoid moving    *
 *  too quickly and missing something tactically where the  *
 *  move initially looks obvious but really is not.         *
 *                                                          *
 ************************************************************
 */
  difficulty = Max(60, Min(difficulty, 200));
  return difficulty;
}

/*
 *******************************************************************************
 *                                                                             *
 *   CraftyExit() is used to terminate the program.  the main functionality    *
 *   is to make sure the "quit" flag is set so that any spinning threads will  *
 *   also exit() rather than spinning forever which can cause GUIs to hang     *
 *   since all processes have not terminated.                                  *
 *                                                                             *
 *******************************************************************************
 */
void CraftyExit(int exit_type) {
  int proc;

  for (proc = 1; proc < CPUS; proc++)
    thread[proc].terminate = 1;
  while (smp_threads);
  exit(exit_type);
}

/*
 *******************************************************************************
 *                                                                             *
 *   DisplayArray() prints array data either 8 or 16 values per line, and also *
 *   reverses the output for arrays that overlay the chess board so that the   *
 *   'white side" is at the bottom rather than the top.  this is mainly used   *
 *   from inside Option() to display the many evaluation terms.                *
 *                                                                             *
 *******************************************************************************
 */
void DisplayArray(int *array, int size) {
  int i, j, len = 16;

  if (Abs(size) % 10 == 0)
    len = 10;
  else if (Abs(size) % 8 == 0)
    len = 8;
  if (size > 0 && size % 16 == 0 && len == 8)
    len = 16;
  if (size > 0) {
    printf("    ");
    for (i = 0; i < size; i++) {
      printf("%3d ", array[i]);
      if ((i + 1) % len == 0) {
        printf("\n");
        if (i < size - 1)
          printf("    ");
      }
    }
    if (i % len != 0)
      printf("\n");
  }
  if (size < 0) {
    for (i = 0; i < 8; i++) {
      printf("    ");
      for (j = 0; j < 8; j++) {
        printf("%3d ", array[(7 - i) * 8 + j]);
      }
      printf(" | %d\n", 8 - i);
    }
    printf("    ---------------------------------\n");
    printf("      a   b   c   d   e   f   g   h\n");
  }
}

/*
 *******************************************************************************
 *                                                                             *
 *   DisplayArray() prints array data either 8 or 16 values per line, and also *
 *   reverses the output for arrays that overlay the chess board so that the   *
 *   'white side" is at the bottom rather than the top.  this is mainly used   *
 *   from inside Option() to display the many evaluation terms.                *
 *                                                                             *
 *******************************************************************************
 */
void DisplayArrayX2(int *array, int *array2, int size) {
  int i, j;

  if (size == 256) {
    printf("    ----------- Middlegame -----------   ");
    printf("    ------------- Endgame -----------\n");
    for (i = 0; i < 8; i++) {
      printf("    ");
      for (j = 0; j < 8; j++)
        printf("%3d ", array[(7 - i) * 8 + j]);
      printf("  |  %d  |", 8 - i);
      printf("  ");
      for (j = 0; j < 8; j++)
        printf("%3d ", array2[(7 - i) * 8 + j]);
      printf("\n");
    }
    printf
        ("    ----------------------------------       ---------------------------------\n");
    printf("      a   b   c   d   e   f   g   h        ");
    printf("      a   b   c   d   e   f   g   h\n");
  } else if (size == 32) {
    printf("    ----------- Middlegame -----------   ");
    printf("    ------------- Endgame -----------\n");
    printf("    ");
    for (i = 0; i < 8; i++)
      printf("%3d ", array[i]);
    printf("  |     |");
    printf("  ");
    for (i = 0; i < 8; i++)
      printf("%3d ", array2[i]);
    printf("\n");
  } else if (size <= 20) {
    size = size / 2;
    printf("    ");
    for (i = 0; i < size; i++)
      printf("%3d ", array[i]);
    printf("  |<mg    eg>|");
    printf("  ");
    for (i = 0; i < size; i++)
      printf("%3d ", array2[i]);
    printf("\n");
  } else if (size > 128) {
    printf("    ----------- Middlegame -----------   ");
    printf("    ------------- Endgame -----------\n");
    for (i = 0; i < size / 32; i++) {
      printf("    ");
      for (j = 0; j < 8; j++)
        printf("%3d ", array[(7 - i) * 8 + j]);
      printf("  |  %d  |", 8 - i);
      printf("  ");
      for (j = 0; j < 8; j++)
        printf("%3d ", array2[(7 - i) * 8 + j]);
      printf("\n");
    }
  } else
    Print(4095, "ERROR, invalid size = -%d in packet\n", size);
}

/*
 *******************************************************************************
 *                                                                             *
 *   DisplayBitBoard() is a debugging function used to display bitboards in a  *
 *   more visual way.  they are displayed as an 8x8 matrix oriented as the     *
 *   normal chess board is, with a1 at the lower left corner.                  *
 *                                                                             *
 *******************************************************************************
 */
void DisplayBitBoard(uint64_t board) {
  int i, j, x;

  for (i = 56; i >= 0; i -= 8) {
    x = (board >> i) & 255;
    for (j = 1; j < 256; j = j << 1)
      if (x & j)
        Print(4095, "X ");
      else
        Print(4095, "- ");
    Print(4095, "\n");
  }
}

/*
 *******************************************************************************
 *                                                                             *
 *   Display2BitBoards() is a debugging function used to display bitboards in  *
 *   a more visual way.  they are displayed as an 8x8 matrix oriented as the   *
 *   normal chess board is, with a1 at the lower left corner.  this function   *
 *   displays 2 boards side by side for comparison.                            *
 *                                                                             *
 *******************************************************************************
 */
void Display2BitBoards(uint64_t board1, uint64_t board2) {
  int i, j, x, y;

  for (i = 56; i >= 0; i -= 8) {
    x = (board1 >> i) & 255;
    for (j = 1; j < 256; j = j << 1)
      if (x & j)
        printf("X ");
      else
        printf("- ");
    printf("    ");
    y = (board2 >> i) & 255;
    for (j = 1; j < 256; j = j << 1)
      if (y & j)
        printf("X ");
      else
        printf("- ");
    printf("\n");
  }
}

/*
 *******************************************************************************
 *                                                                             *
 *   DisplayChessBoard() is used to display the board since it is kept in      *
 *   both the bit-board and array formats, here we use the array format which  *
 *   is nearly ready for display as is.                                        *
 *                                                                             *
 *******************************************************************************
 */
void DisplayChessBoard(FILE * display_file, POSITION pos) {
  int display_board[64], i, j;
  static const char display_string[16][4] =
      { "<K>", "<Q>", "<R>", "<B>", "<N>", "<P>", "   ",
    "-P-", "-N-", "-B-", "-R-", "-Q-", "-K-", " . "
  };

/*
 ************************************************************
 *                                                          *
 *  First, convert square values to indices to the proper   *
 *  text string.                                            *
 *                                                          *
 ************************************************************
 */
  for (i = 0; i < 64; i++) {
    display_board[i] = pos.board[i] + 6;
    if (pos.board[i] == 0) {
      if (((i / 8) & 1) == ((i % 8) & 1))
        display_board[i] = 13;
    }
  }
/*
 ************************************************************
 *                                                          *
 *  Now that that's done, simply display using 8 squares    *
 *  per line.                                               *
 *                                                          *
 ************************************************************
 */
  fprintf(display_file, "\n       +---+---+---+---+---+---+---+---+\n");
  for (i = 7; i >= 0; i--) {
    fprintf(display_file, "   %2d  ", i + 1);
    for (j = 0; j < 8; j++)
      fprintf(display_file, "|%s", display_string[display_board[i * 8 + j]]);
    fprintf(display_file, "|\n");
    fprintf(display_file, "       +---+---+---+---+---+---+---+---+\n");
  }
  fprintf(display_file, "         a   b   c   d   e   f   g   h\n\n");
}

/*
 *******************************************************************************
 *                                                                             *
 *   DisplayChessMove() is a debugging function that displays a chess move in  *
 *   a very simple (non-algebraic) form.                                       *
 *                                                                             *
 *******************************************************************************
 */
void DisplayChessMove(char *title, int move) {
  Print(4095, "%s  piece=%d, from=%d, to=%d, captured=%d, promote=%d\n",
      title, Piece(move), From(move), To(move), Captured(move),
      Promote(move));
}

/*
 *******************************************************************************
 *                                                                             *
 *   DisplayEvaluation() is used to convert the evaluation to a string that    *
 *   can be displayed.  The length is fixed so that screen formatting will     *
 *   look nice and aligned.                                                    *
 *                                                                             *
 *******************************************************************************
 */
char *DisplayEvaluation(int value, int wtm) {
  int tvalue;
  static char out[20];

  tvalue = (wtm) ? value : -value;
  if (!MateScore(value) && !EGTBScore(value))
    sprintf(out, "%7.2f", ((float) tvalue) / 100.0);
  else if (Abs(value) > MATE) {
    if (tvalue < 0)
      sprintf(out, " -infnty");
    else
      sprintf(out, " +infnty");
  } else {
    if (EGTBScore(value)) {
      if (wtm) {
        if (value == TBWIN)
          sprintf(out, "   Won ");
        else if (value == -TBWIN)
          sprintf(out, "  Lost ");
      } else {
        if (value == TBWIN)
          sprintf(out, "  -Won ");
        else if (value == -TBWIN)
          sprintf(out, " -Lost ");
      }
    }
    if (MateScore(value)) {
      if (value == MATE - 2 && wtm)
        sprintf(out, "   Mate");
      else if (value == MATE - 2 && !wtm)
        sprintf(out, "  -Mate");
      else if (value == -(MATE - 1) && wtm)
        sprintf(out, "  -Mate");
      else if (value == -(MATE - 1) && !wtm)
        sprintf(out, "   Mate");
      else if (value > 0 && wtm)
        sprintf(out, "  Mat%.2d", (MATE - value) / 2);
      else if (value > 0 && !wtm)
        sprintf(out, " -Mat%.2d", (MATE - value) / 2);
      else if (wtm)
        sprintf(out, " -Mat%.2d", (MATE - Abs(value)) / 2);
      else
        sprintf(out, "  Mat%.2d", (MATE - Abs(value)) / 2);
    }
  }
  return out;
}

/*
 *******************************************************************************
 *                                                                             *
 *   DisplayEvaluationKibitz() is used to convert the evaluation to a string   *
 *   that can be displayed.  The length is variable so that ICC kibitzes and   *
 *   whispers will look nicer.                                                 *
 *                                                                             *
 *******************************************************************************
 */
char *DisplayEvaluationKibitz(int value, int wtm) {
  int tvalue;
  static char out[10];

  tvalue = (wtm) ? value : -value;
  if (!MateScore(value))
    sprintf(out, "%+.2f", ((float) tvalue) / 100.0);
  else if (Abs(value) > MATE) {
    if (tvalue < 0)
      sprintf(out, "-infnty");
    else
      sprintf(out, "+infnty");
  } else if (value == MATE - 2 && wtm)
    sprintf(out, "Mate");
  else if (value == MATE - 2 && !wtm)
    sprintf(out, "-Mate");
  else if (value == -(MATE - 1) && wtm)
    sprintf(out, "-Mate");
  else if (value == -(MATE - 1) && !wtm)
    sprintf(out, "Mate");
  else if (value > 0 && wtm)
    sprintf(out, "Mat%.2d", (MATE - value) / 2);
  else if (value > 0 && !wtm)
    sprintf(out, "-Mat%.2d", (MATE - value) / 2);
  else if (wtm)
    sprintf(out, "-Mat%.2d", (MATE - Abs(value)) / 2);
  else
    sprintf(out, "Mat%.2d", (MATE - Abs(value)) / 2);
  return out;
}

/*
 *******************************************************************************
 *                                                                             *
 *   DisplayPath() is used to display a PV during the root move search.        *
 *                                                                             *
 *******************************************************************************
 */
char *DisplayPath(TREE * RESTRICT tree, int wtm, PATH * pv) {
  static char buffer[4096];
  int i, t_move_number;

/*
 ************************************************************
 *                                                          *
 *  Initialize.                                             *
 *                                                          *
 ************************************************************
 */
  t_move_number = move_number;
  sprintf(buffer, " %d.", move_number);
  if (!wtm)
    sprintf(buffer + strlen(buffer), " ...");
  for (i = 1; i < (int) pv->pathl; i++) {
    if (i > 1 && wtm)
      sprintf(buffer + strlen(buffer), " %d.", t_move_number);
    sprintf(buffer + strlen(buffer), " %s", OutputMove(tree, i, wtm,
            pv->path[i]));
    MakeMove(tree, i, wtm, pv->path[i]);
    wtm = Flip(wtm);
    if (wtm)
      t_move_number++;
  }
  if (pv->pathh == 1)
    sprintf(buffer + strlen(buffer), " <HT>");
  else if (pv->pathh == 2)
    sprintf(buffer + strlen(buffer), " <3-fold>");
  else if (pv->pathh == 3)
    sprintf(buffer + strlen(buffer), " <50-move>");
  else if (pv->pathh == 4)
    sprintf(buffer + strlen(buffer), " <EGTB>");
  if (strlen(buffer) < 30)
    for (i = 0; i < 30 - strlen(buffer); i++)
      strcat(buffer, " ");
  strcpy(kibitz_text, buffer);
  for (i = pv->pathl - 1; i > 0; i--) {
    wtm = Flip(wtm);
    UnmakeMove(tree, i, wtm, pv->path[i]);
  }
  return buffer;
}

/*
 *******************************************************************************
 *                                                                             *
 *   DisplayFail() is used to display a move that fails high or low during     *
 *   the search.  Normally disabled.                                           *
 *                                                                             *
 *******************************************************************************
 */
void DisplayFail(TREE * RESTRICT tree, int type, int level, int wtm, int time,
    int move, int value, int force) {
  char buffer[4096], *fh_indicator;

/*
 ************************************************************
 *                                                          *
 *  If we have not used "noise_level" units of time, we     *
 *  return immediately.  Otherwise we add the fail high/low *
 *  indicator (++/--) and then display the times.           *
 *                                                          *
 ************************************************************
 */
  if (time < noise_level)
    return;
  if (type == 1)
    fh_indicator = (wtm) ? "++" : "--";
  else
    fh_indicator = (wtm) ? "--" : "++";
  Print(4, "         %2i   %s     %2s   ", iteration,
      Display2Times(end_time - start_time), fh_indicator);
/*
 ************************************************************
 *                                                          *
 *  If we are pondering, we need to add the (ponder-move)   *
 *  to the front of the buffer, correcting the move number  *
 *  if necessary.  Then fill in the move number and the     *
 *  fail high/low bound.                                    *
 *                                                          *
 ************************************************************
 */
  if (!pondering) {
    sprintf(buffer, "%d.", move_number);
    if (!wtm)
      sprintf(buffer + strlen(buffer), " ...");
  } else {
    if (wtm)
      sprintf(buffer, "%d. ... (%s) %d.", move_number - 1, ponder_text,
          move_number);
    else
      sprintf(buffer, "%d. (%s)", move_number, ponder_text);
  }
  sprintf(buffer + strlen(buffer), " %s%c", OutputMove(tree, 1, wtm, move),
      (type == 1) ? '!' : '?');
  strcpy(kibitz_text, buffer);
  if (time >= noise_level || force) {
    noise_block = 0;
    Lock(lock_io);
    Print(4, "%s", buffer);
    Unlock(lock_io);
    if (type == 1)
      Print(4, " (%c%s)                   \n", (wtm) ? '>' : '<',
          DisplayEvaluationKibitz(value, wtm));
    else
      Print(4, " (%c%s)                   \n", (wtm) ? '<' : '>',
          DisplayEvaluationKibitz(value, wtm));
  }
}

/*
 *******************************************************************************
 *                                                                             *
 *   DisplayPV() is used to display a PV during the search.                    *
 *                                                                             *
 *******************************************************************************
 */
void DisplayPV(TREE * RESTRICT tree, int level, int wtm, int time, PATH * pv,
    int force) {
  char buffer[4096], *buffp, *bufftemp;
  char blanks[40] = { "                                        " };
  int i, len, t_move_number, nskip = 0, twtm = wtm, pv_depth = pv->pathd;;
  unsigned int idle_time;

/*
 ************************************************************
 *                                                          *
 *  Initialize.                                             *
 *                                                          *
 ************************************************************
 */
  for (i = 0; i < n_root_moves; i++)
    if (root_moves[i].status & 4)
      nskip++;
  for (i = 0; i < 4096; i++)
    buffer[i] = ' ';
  t_move_number = move_number;
  if (!pondering || analyze_mode) {
    sprintf(buffer, "%d.", move_number);
    if (!wtm)
      sprintf(buffer + strlen(buffer), " ...");
  } else {
    if (wtm)
      sprintf(buffer, "%d. ... (%s) %d.", move_number - 1, ponder_text,
          move_number);
    else
      sprintf(buffer, "%d. (%s)", move_number, ponder_text);
  }
  for (i = 1; i < (int) pv->pathl; i++) {
    if (i > 1 && wtm)
      sprintf(buffer + strlen(buffer), " %d.", t_move_number);
    sprintf(buffer + strlen(buffer), " %s", OutputMove(tree, i, wtm,
            pv->path[i]));
    MakeMove(tree, i, wtm, pv->path[i]);
    wtm = Flip(wtm);
    if (wtm)
      t_move_number++;
  }
  if (pv->pathh == 1)
    sprintf(buffer + strlen(buffer), " <HT>");
  else if (pv->pathh == 2)
    sprintf(buffer + strlen(buffer), " <3-fold>");
  else if (pv->pathh == 3)
    sprintf(buffer + strlen(buffer), " <50-move>");
  else if (pv->pathh == 4)
    sprintf(buffer + strlen(buffer), " <EGTB>");
  if (nskip > 1 && smp_max_threads > 1)
    sprintf(buffer + strlen(buffer), " (s=%d)", nskip);
  if (strlen(buffer) < 30) {
    len = 30 - strlen(buffer);
    for (i = 0; i < len; i++)
      strcat(buffer, " ");
  }
  strcpy(kibitz_text, buffer);
  if (time >= noise_level || force) {
    noise_block = 0;
    Lock(lock_io);
    Print(2, "         ");
    if (level == 6)
      Print(2, "%2i   %s%s   ", pv_depth, Display2Times(time),
          DisplayEvaluation(pv->pathv, twtm));
    else
      Print(2, "%2i-> %s%s   ", pv_depth, Display2Times(time)
          , DisplayEvaluation(pv->pathv, twtm));
    buffp = buffer;
    do {
      if ((int) strlen(buffp) > line_length - 38) {
        bufftemp = buffp + line_length - 38;
        while (*bufftemp != ' ')
          bufftemp--;
        if (*(bufftemp - 1) == '.')
          while (*(--bufftemp) != ' ');
      } else
        bufftemp = 0;
      if (bufftemp)
        *bufftemp = 0;
      Print(2, "%s\n", buffp);
      buffp = bufftemp + 1;
      if (bufftemp)
        if (!strncmp(buffp, blanks, strlen(buffp)))
          bufftemp = 0;
      if (bufftemp)
        Print(2, "                                     ");
    } while (bufftemp);
    idle_time = 0;
    for (i = 0; i < smp_max_threads; i++)
      idle_time += thread[i].idle;
    busy_percent =
        100 - Min(100,
        100 * idle_time / (smp_max_threads * (end_time - start_time) + 1));
    Kibitz(level, twtm, pv_depth, end_time - start_time, pv->pathv,
        tree->nodes_searched, busy_percent, tree->egtb_hits, kibitz_text);
    Unlock(lock_io);
  }
  for (i = pv->pathl - 1; i > 0; i--) {
    wtm = Flip(wtm);
    UnmakeMove(tree, i, wtm, pv->path[i]);
  }
}

/*
 *******************************************************************************
 *                                                                             *
 *   DisplayHHMMSS is used to convert integer time values in 1/100th second    *
 *   units into a traditional output format for time, hh:mm:ss rather than     *
 *   just nnn.n seconds.                                                       *
 *                                                                             *
 *******************************************************************************
 */
char *DisplayHHMMSS(unsigned int time) {
  static char out[32];

  time = time / 100;
  sprintf(out, "%3u:%02u:%02u", time / 3600, (time % 3600) / 60, time % 60);
  return out;
}

/*
 *******************************************************************************
 *                                                                             *
 *   DisplayHHMM is used to convert integer time values in 1/100th second      *
 *   units into a traditional output format for time, mm:ss rather than just   *
 *   nnn.n seconds.                                                            *
 *                                                                             *
 *******************************************************************************
 */
char *DisplayHHMM(unsigned int time) {
  static char out[10];

  time = time / 6000;
  sprintf(out, "%3u:%02u", time / 60, time % 60);
  return out;
}

/*
 *******************************************************************************
 *                                                                             *
 *   DisplayKMB() takes an integer value that represents nodes per second, or  *
 *   just total nodes, and converts it into a more compact form, so that       *
 *   instead of nps=57200931, we get nps=57.2M.  We use units of "K", "M",     *
 *   "B" and "T".  If type==0, K=1000, etc.  If type=1, K=1024, etc.           *
 *                                                                             *
 *******************************************************************************
 */
char *DisplayKMB(uint64_t val, int type) {
  static char out[10];

  if (type == 0) {
    if (val < 1000)
      sprintf(out, "%" PRIu64, val);
    else if (val < 1000000)
      sprintf(out, "%.1fK", (double) val / 1000);
    else if (val < 1000000000)
      sprintf(out, "%.1fM", (double) val / 1000000);
    else
      sprintf(out, "%.1fB", (double) val / 1000000000);
  } else {
    if (val > 0 && !(val & 0x000000003fffffffULL))
      sprintf(out, "%dG", (int) (val / (1 << 30)));
    else if (val > 0 && !(val & 0x00000000000fffffULL))
      sprintf(out, "%dM", (int) (val / (1 << 20)));
    else if (val > 0 && !(val & 0x00000000000003ffULL))
      sprintf(out, "%dK", (int) (val / (1 << 10)));
    else
      sprintf(out, "%" PRIu64, val);
  }
  return out;
}

/*
 *******************************************************************************
 *                                                                             *
 *   DisplayTime() is used to display search times, and shows times in one of  *
 *   two ways depending on the value passed in.  If less than 60 seconds is to *
 *   be displayed, it is displayed as a decimal fraction like 32.7, while if   *
 *   more than 60 seconds is to be displayed, it is converted to the more      *
 *   traditional mm:ss form.  The string it produces is of fixed length to     *
 *   provide neater screen formatting.                                         *
 *                                                                             *
 *******************************************************************************
 */
char *DisplayTime(unsigned int time) {
  static char out[10];

  if (time < 6000)
    sprintf(out, "%6.2f", (float) time / 100.0);
  else {
    time = time / 100;
    sprintf(out, "%3u:%02u", time / 60, time % 60);
  }
  return out;
}

/*
 *******************************************************************************
 *                                                                             *
 *   Display2Times() is used to display search times, and shows times in one   *
 *   of two ways depending on the value passed in.  If less than 60 seconds is *
 *   to be displayed, it is displayed as a decimal fraction like 32.7, while   *
 *   if more than 60 seconds is to be displayed, it is converted to the more   *
 *   traditional mm:ss form.  The string it produces is of fixed length to     *
 *   provide neater screen formatting.                                         *
 *                                                                             *
 *   The second argument is the "difficulty" value which lets us display the   *
 *   target time (as modified by difficulty) so that it is possible to know    *
 *   roughly when the move will be announced.                                  *
 *                                                                             *
 *******************************************************************************
 */
char *Display2Times(unsigned int time) {
  int ttime, c, spaces;
  static char out[20], tout[10];

  if (time < 6000)
    sprintf(out, "%6.2f", (float) time / 100.0);
  else {
    time = time / 100;
    sprintf(out, "%3u:%02u", time / 60, time % 60);
  }
  if (search_time_limit)
    ttime = search_time_limit;
  else
    ttime = difficulty * time_limit / 100;
  if (ttime < 360000) {
    if (ttime < 6000)
      sprintf(tout, "%6.2f", (float) ttime / 100.0);
    else {
      ttime = ttime / 100;
      sprintf(tout, "%3u:%02u", ttime / 60, ttime % 60);
    }
    c = strspn(tout, " ");
    strcat(out, "/");
    strcat(out, tout + c);
  } else
    strcat(out, "       ");
  spaces = 13 - strlen(out);
  for (c = 0; c < spaces; c++)
    strcat(out, " ");
  return out;
}

/*
 *******************************************************************************
 *                                                                             *
 *   DisplayTimeKibitz() behaves just like DisplayTime() except that the       *
 *   string it produces is a variable-length string that is as short as        *
 *   possible to make ICC kibitzes/whispers look neater.                       *
 *                                                                             *
 *******************************************************************************
 */
char *DisplayTimeKibitz(unsigned int time) {
  static char out[10];

  if (time < 6000)
    sprintf(out, "%.2f", (float) time / 100.0);
  else {
    time = time / 100;
    sprintf(out, "%u:%02u", time / 60, time % 60);
  }
  return out;
}

/*
 *******************************************************************************
 *                                                                             *
 *   FormatPV() is used to display a PV during the search.  It will also note  *
 *   when the PV was terminated by a hash table hit.                           *
 *                                                                             *
 *******************************************************************************
 */
char *FormatPV(TREE * RESTRICT tree, int wtm, PATH pv) {
  int i, t_move_number;
  static char buffer[4096];

/*
 ************************************************************
 *                                                          *
 *  Initialize.                                             *
 *                                                          *
 ************************************************************
 */
  t_move_number = move_number;
  sprintf(buffer, " %d.", move_number);
  if (!wtm)
    sprintf(buffer + strlen(buffer), " ...");
  for (i = 1; i < (int) pv.pathl; i++) {
    if (i > 1 && wtm)
      sprintf(buffer + strlen(buffer), " %d.", t_move_number);
    sprintf(buffer + strlen(buffer), " %s", OutputMove(tree, i, wtm,
            pv.path[i]));
    MakeMove(tree, i, wtm, pv.path[i]);
    wtm = Flip(wtm);
    if (wtm)
      t_move_number++;
  }
  for (i = pv.pathl - 1; i > 0; i--) {
    wtm = Flip(wtm);
    UnmakeMove(tree, i, wtm, pv.path[i]);
  }
  return buffer;
}

/* last modified 02/26/14 */
/*
 *******************************************************************************
 *                                                                             *
 *   GameOver() is used to determine if the game is over by rule.  More        *
 *   specifically, after our move, the opponent has no legal move to play.  He *
 *   is either checkmated or stalemated, either of which is sufficient reason  *
 *   to terminate the game.                                                    *
 *                                                                             *
 *******************************************************************************
 */
int GameOver(int wtm) {
  TREE *const tree = block[0];
  unsigned *mvp, *lastm, rmoves[256], over = 1;

/*
 ************************************************************
 *                                                          *
 *  First, use GenerateMoves() to generate the set of       *
 *  legal moves from the root position.                     *
 *                                                          *
 ************************************************************
 */
  lastm = GenerateCaptures(tree, 1, wtm, rmoves);
  lastm = GenerateNoncaptures(tree, 1, wtm, lastm);
/*
 ************************************************************
 *                                                          *
 *  Now make each move and determine if we are in check     *
 *  after each one.  Any move that does not leave us in     *
 *  check is good enough to prove that the game is not yet  *
 *  officially over.                                        *
 *                                                          *
 ************************************************************
 */
  for (mvp = rmoves; mvp < lastm; mvp++) {
    MakeMove(tree, 1, wtm, *mvp);
    if (!Check(wtm))
      over = 0;
    UnmakeMove(tree, 1, wtm, *mvp);
  }
/*
 ************************************************************
 *                                                          *
 *  If we did not make it thru the complete move list, we   *
 *  must have at least one legal move so the game is not    *
 *  over.  return 0.  Otherwise, we have no move and the    *
 *  game is over.  We return 1 if this side is stalmated or *
 *  we return 2 if this side is mated.                      *
 *                                                          *
 ************************************************************
 */
  if (!over)
    return 0;
  else if (!Check(wtm))
    return 1;
  else
    return 2;
}

/*
 *******************************************************************************
 *                                                                             *
 *   ReadClock() is a procedure used to read the elapsed time.  Since this     *
 *   varies from system to system, this procedure has several flavors to       *
 *   provide portability.                                                      *
 *                                                                             *
 *******************************************************************************
 */
unsigned int ReadClock(void) {
#if defined(UNIX)
  struct timeval timeval;
  struct timezone timezone;
#endif
#if defined(UNIX)
  gettimeofday(&timeval, &timezone);
  return timeval.tv_sec * 100 + (timeval.tv_usec / 10000);
#else
  return (unsigned int) GetTickCount() / 10;
#endif
}

/*
 *******************************************************************************
 *                                                                             *
 *   FindBlockID() converts a thread block pointer into an ID that is easier   *
 *   to understand when debugging.                                             *
 *                                                                             *
 *******************************************************************************
 */
int FindBlockID(TREE * RESTRICT which) {
  int i;

  for (i = 0; i <= smp_max_threads * 64; i++)
    if (which == block[i])
      return i;
  return -1;
}

/*
 *******************************************************************************
 *                                                                             *
 *   InvalidPosition() is used to determine if the position just entered via a *
 *   FEN-string or the "edit" command is legal.  This includes the expected    *
 *   tests for too many pawns or pieces for one side, pawns on impossible      *
 *   squares, and the like.                                                    *
 *                                                                             *
 *******************************************************************************
 */
int InvalidPosition(TREE * RESTRICT tree) {
  int error = 0, wp, wn, wb, wr, wq, wk, bp, bn, bb, br, bq, bk;

  wp = PopCnt(Pawns(white));
  wn = PopCnt(Knights(white));
  wb = PopCnt(Bishops(white));
  wr = PopCnt(Rooks(white));
  wq = PopCnt(Queens(white));
  wk = PopCnt(Kings(white));
  bp = PopCnt(Pawns(black));
  bn = PopCnt(Knights(black));
  bb = PopCnt(Bishops(black));
  br = PopCnt(Rooks(black));
  bq = PopCnt(Queens(black));
  bk = PopCnt(Kings(black));
  if (wp > 8) {
    Print(4095, "illegal position, too many white pawns\n");
    error = 1;
  }
  if (wn && wp + wn > 10) {
    Print(4095, "illegal position, too many white knights\n");
    error = 1;
  }
  if (wb && wp + wb > 10) {
    Print(4095, "illegal position, too many white bishops\n");
    error = 1;
  }
  if (wr && wp + wr > 10) {
    Print(4095, "illegal position, too many white rooks\n");
    error = 1;
  }
  if (wq && wp + wq > 10) {
    Print(4095, "illegal position, too many white queens\n");
    error = 1;
  }
  if (wk == 0) {
    Print(4095, "illegal position, no white king\n");
    error = 1;
  }
  if (wk > 1) {
    Print(4095, "illegal position, multiple white kings\n");
    error = 1;
  }
  if ((wn + wb + wr + wq) && wp + wn + wb + wr + wq > 15) {
    Print(4095, "illegal position, too many white pieces\n");
    error = 1;
  }
  if (Pawns(white) & (rank_mask[RANK1] | rank_mask[RANK8])) {
    Print(4095, "illegal position, white pawns on first/eighth rank(s)\n");
    error = 1;
  }
  if (bp > 8) {
    Print(4095, "illegal position, too many black pawns\n");
    error = 1;
  }
  if (bn && bp + bn > 10) {
    Print(4095, "illegal position, too many black knights\n");
    error = 1;
  }
  if (bb && bp + bb > 10) {
    Print(4095, "illegal position, too many black bishops\n");
    error = 1;
  }
  if (br && bp + br > 10) {
    Print(4095, "illegal position, too many black rooks\n");
    error = 1;
  }
  if (bq && bp + bq > 10) {
    Print(4095, "illegal position, too many black queens\n");
    error = 1;
  }
  if (bk == 0) {
    Print(4095, "illegal position, no black king\n");
    error = 1;
  }
  if (bk > 1) {
    Print(4095, "illegal position, multiple black kings\n");
    error = 1;
  }
  if ((bn + bb + br + bq) && bp + bn + bb + br + bq > 15) {
    Print(4095, "illegal position, too many black pieces\n");
    error = 1;
  }
  if (Pawns(black) & (rank_mask[RANK1] | rank_mask[RANK8])) {
    Print(4095, "illegal position, black pawns on first/eighth rank(s)\n");
    error = 1;
  }
  if (error == 0 && Check(!game_wtm)) {
    Print(4095, "ERROR side not on move is in check!\n");
    error = 1;
  }
  return error;
}

/*
 *******************************************************************************
 *                                                                             *
 *   KingPawnSquare() is used to initialize some of the passed pawn race       *
 *   tables used by Evaluate().  It simply answers the question "is the king   *
 *   in the square of the pawn so the pawn can't outrun it and promote?"       *
 *                                                                             *
 *******************************************************************************
 */
int KingPawnSquare(int pawn, int king, int queen, int ptm) {
  int pdist, kdist;

  pdist = Abs(Rank(pawn) - Rank(queen)) + !ptm;
  kdist = Distance(king, queen);
  return pdist >= kdist;
}

/* last modified 07/13/16 */
/*
 *******************************************************************************
 *                                                                             *
 *   Mated() is used to determine if the game has ended by checkmate.          *
 *                                                                             *
 *   We return 0 if the game doesn't end here, 1 if the side on move is mated  *
 *   and 2 if the side on move is stalemated.                                  *
 *                                                                             *
 *******************************************************************************
 */
int Mated(TREE * RESTRICT tree, int ply, int wtm) {
  unsigned int rmoves[256], *mvp, *lastm;
  int temp = 0;

/*
 ************************************************************
 *                                                          *
 *   first, use GenerateMoves() to generate the set of      *
 *   legal moves from the root position, after making the   *
 *   test move passed in.                                   *
 *                                                          *
 ************************************************************
 */
  lastm = GenerateCaptures(tree, ply, wtm, rmoves);
  lastm = GenerateNoncaptures(tree, ply, wtm, lastm);
/*
 ************************************************************
 *                                                          *
 *   now make each move and use eliminate any that leave    *
 *   king in check (which makes those moves illegal.)       *
 *                                                          *
 ************************************************************
 */
  for (mvp = rmoves; mvp < lastm; mvp++) {
    MakeMove(tree, ply, wtm, *mvp);
    temp = Check(wtm);
    UnmakeMove(tree, ply, wtm, *mvp);
    if (!temp)
      break;
  }
/*
 ************************************************************
 *                                                          *
 *   if there is one move that did not leave us in check,   *
 *   then it can't be checkmate/stalemate.                  *
 *                                                          *
 ************************************************************
 */
  if (!temp)
    return 0;
/*
 ************************************************************
 *                                                          *
 *   No legal moves.  If we are in check, we have been      *
 *   checkmated, otherwise we are stalemated.               *
 *                                                          *
 ************************************************************
 */
  if (Check(wtm))
    return 1;
  return 2;
}

/*
 *******************************************************************************
 *                                                                             *
 *   ParseTime() is used to parse a time value that could be entered as s.ss,  *
 *   mm:ss, or hh:mm:ss.  It is converted to Crafty's internal 1/100th second  *
 *   time resolution.                                                          *
 *                                                                             *
 *******************************************************************************
 */
int ParseTime(char *string) {
  int time = 0, minutes = 0;

  while (*string) {
    switch (*string) {
      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9':
        minutes = minutes * 10 + (*string) - '0';
        break;
      case ':':
        time = time * 60 + minutes;
        minutes = 0;
        break;
      default:
        Print(4095, "illegal character in time, please re-enter\n");
        break;
    }
    string++;
  }
  return time * 60 + minutes;
}

/*
 *******************************************************************************
 *                                                                             *
 *   Pass() was written by Tim Mann to handle the case where a position is set *
 *   using a FEN string, and then black moves first.  The game.nnn file was    *
 *   designed to start with a white move, so "pass" is now a "no-op" move for  *
 *   the side whose turn it is to move.                                        *
 *                                                                             *
 *******************************************************************************
 */
void Pass(void) {
  const int halfmoves_done = 2 * (move_number - 1) + (1 - game_wtm);
  int prev_pass = 0;
  char buffer[128];

/* Was previous move a pass? */
  if (halfmoves_done > 0) {
    if (history_file) {
      fseek(history_file, (halfmoves_done - 1) * 10, SEEK_SET);
      if (fscanf(history_file, "%s", buffer) == 0 ||
          strcmp(buffer, "pass") == 0)
        prev_pass = 1;
    }
  }
  if (prev_pass) {
    if (game_wtm)
      move_number--;
  } else {
    if (history_file) {
      fseek(history_file, halfmoves_done * 10, SEEK_SET);
      fprintf(history_file, "%9s\n", "pass");
    }
    if (!game_wtm)
      move_number++;
  }
  game_wtm = Flip(game_wtm);
}

/*
 *******************************************************************************
 *                                                                             *
 *   Print() is the main output procedure.  The first argument is a bitmask    *
 *   that identifies the type of output.  If this argument is anded with the   *
 *   "display" control variable, and a non-zero result is produced, then the   *
 *   print is done, otherwise the print is skipped and we return (more details *
 *   can be found in the display command comments in option.c).  This also     *
 *   uses the "variable number of arguments" facility in ANSI C since the      *
 *   normal printf() function accepts a variable number of arguments.          *
 *                                                                             *
 *   Print() also sends output to the log.nnn file automatically, so that it   *
 *   is recorded even if the above display control variable says "do not send  *
 *   this to stdout"                                                           *
 *                                                                             *
 *******************************************************************************
 */
void Print(int vb, char *fmt, ...) {
  va_list ap;

  va_start(ap, fmt);
  if (vb == 4095 || vb & display_options) {
    vprintf(fmt, ap);
    fflush(stdout);
  }
  if (time_limit > 5 || tc_time_remaining[root_wtm] > 1000 || vb == 4095) {
    va_start(ap, fmt);
    if (log_file) {
      vfprintf(log_file, fmt, ap);
      fflush(log_file);
    }
  }
  va_end(ap);
}

/*
 *******************************************************************************
 *                                                                             *
 *  A 32 bit random number generator. An implementation in C of the algorithm  *
 *  given by Knuth, the art of computer programming, vol. 2, pp. 26-27. We use *
 *  e=32, so we have to evaluate y(n) = y(n - 24) + y(n - 55) mod 2^32, which  *
 *  is implicitly done by unsigned arithmetic.                                 *
 *                                                                             *
 *******************************************************************************
 */
unsigned int Random32(void) {
/*
 random numbers from Mathematica 2.0.
 SeedRandom = 1;
 Table[Random[Integer, {0, 2^32 - 1}]
 */
  static const uint64_t x[55] = {
    1410651636UL, 3012776752UL, 3497475623UL, 2892145026UL, 1571949714UL,
    3253082284UL, 3489895018UL, 387949491UL, 2597396737UL, 1981903553UL,
    3160251843UL, 129444464UL, 1851443344UL, 4156445905UL, 224604922UL,
    1455067070UL, 3953493484UL, 1460937157UL, 2528362617UL, 317430674UL,
    3229354360UL, 117491133UL, 832845075UL, 1961600170UL, 1321557429UL,
    747750121UL, 545747446UL, 810476036UL, 503334515UL, 4088144633UL,
    2824216555UL, 3738252341UL, 3493754131UL, 3672533954UL, 29494241UL,
    1180928407UL, 4213624418UL, 33062851UL, 3221315737UL, 1145213552UL,
    2957984897UL, 4078668503UL, 2262661702UL, 65478801UL, 2527208841UL,
    1960622036UL, 315685891UL, 1196037864UL, 804614524UL, 1421733266UL,
    2017105031UL, 3882325900UL, 810735053UL, 384606609UL, 2393861397UL
  };
  static int init = 1;
  static uint64_t y[55];
  static int j, k;
  uint64_t ul;

  if (init) {
    int i;

    init = 0;
    for (i = 0; i < 55; i++)
      y[i] = x[i];
    j = 24 - 1;
    k = 55 - 1;
  }
  ul = (y[k] += y[j]);
  if (--j < 0)
    j = 55 - 1;
  if (--k < 0)
    k = 55 - 1;
  return (unsigned int) ul;
}

/*
 *******************************************************************************
 *                                                                             *
 *   Random64() uses two calls to Random32() and then concatenates the two     *
 *   values into one 64 bit random number, used for hash signature updates on  *
 *   the Zobrist hash signatures.                                              *
 *                                                                             *
 *******************************************************************************
 */
uint64_t Random64(void) {
  uint64_t result;
  unsigned int r1, r2;

  r1 = Random32();
  r2 = Random32();
  result = r1 | (uint64_t) r2 << 32;
  return result;
}

/*
 *******************************************************************************
 *                                                                             *
 *   Read() copies data from the command_buffer into a local buffer, and then  *
 *   uses ReadParse to break this command up into tokens for processing.       *
 *                                                                             *
 *******************************************************************************
 */
int Read(int wait, char *buffer) {
  char *eol, *ret, readdata;

  *buffer = 0;
/*
 case 1:  We have a complete command line, with terminating
 N/L character in the buffer.  We can simply extract it from
 the I/O buffer, parse it and return.
 */
  if (strchr(cmd_buffer, '\n'));
/*
 case 2:  The buffer does not contain a complete line.  If we
 were asked to not wait for a complete command, then we first
 see if I/O is possible, and if so, read in what is available.
 If that includes a N/L, then we are ready to parse and return.
 If not, we return indicating no input available just yet.
 */
  else if (!wait) {
    if (CheckInput()) {
      readdata = ReadInput();
      if (!strchr(cmd_buffer, '\n'))
        return 0;
      if (!readdata)
        return -1;
    } else
      return 0;
  }
/*
 case 3:  The buffer does not contain a complete line, but we
 were asked to wait until a complete command is entered.  So we
 hang by doing a ReadInput() and continue doing so until we get
 a N/L character in the buffer.  Then we parse and return.
 */
  else
    while (!strchr(cmd_buffer, '\n')) {
      readdata = ReadInput();
      if (!readdata)
        return -1;
    }
  eol = strchr(cmd_buffer, '\n');
  *eol = 0;
  ret = strchr(cmd_buffer, '\r');
  if (ret)
    *ret = ' ';
  strcpy(buffer, cmd_buffer);
  memmove(cmd_buffer, eol + 1, strlen(eol + 1) + 1);
  return 1;
}

/*
 *******************************************************************************
 *                                                                             *
 *   ReadClear() clears the input buffer when input_stream is being switched   *
 *   to a file, since we have info buffered up from a different input stream.  *
 *                                                                             *
 *******************************************************************************
 */
void ReadClear() {
  cmd_buffer[0] = 0;
}

/*
 *******************************************************************************
 *                                                                             *
 *   ReadParse() takes one complete command-line, and breaks it up into tokens.*
 *   common delimiters are used, such as " ", ",", "/" and ";", any of which   *
 *   delimit fields.                                                           *
 *                                                                             *
 *******************************************************************************
 */
int ReadParse(char *buffer, char *args[], char *delims) {
  int nargs;
  char *next, tbuffer[4096];

  strcpy(tbuffer, buffer);
  for (nargs = 0; nargs < 512; nargs++)
    *(args[nargs]) = 0;
  next = strtok(tbuffer, delims);
  if (!next)
    return 0;
  if (strlen(next) > 255)
    Print(4095, "ERROR, ignoring token %s, max allowable len = 255\n", next);
  else
    strcpy(args[0], next);
  for (nargs = 1; nargs < 512; nargs++) {
    next = strtok(0, delims);
    if (!next)
      break;
    if (strlen(next) > 255)
      Print(4095, "ERROR, ignoring token %s, max allowable len = 255\n",
          next);
    else
      strcpy(args[nargs], next);
  }
  return nargs;
}

/*
 *******************************************************************************
 *                                                                             *
 *   ReadInput() reads data from the input_stream, and buffers this into the   *
 *   command_buffer for later processing.                                      *
 *                                                                             *
 *******************************************************************************
 */
int ReadInput(void) {
  int bytes;
  char buffer[4096], *end;

  do
    bytes = read(fileno(input_stream), buffer, 2048);
  while (bytes < 0 && errno == EINTR);
  if (bytes == 0) {
    if (input_stream != stdin)
      fclose(input_stream);
    input_stream = stdin;
    return 0;
  } else if (bytes < 0) {
    Print(4095, "ERROR!  input I/O stream is unreadable, exiting.\n");
    CraftyExit(1);
  }
  end = cmd_buffer + strlen(cmd_buffer);
  memcpy(end, buffer, bytes);
  *(end + bytes) = 0;
  return 1;
}

/*
 *******************************************************************************
 *                                                                             *
 *   ReadChessMove() is used to read a move from an input file.  The main      *
 *   issue is to skip over "trash" like move numbers, times, comments, and so  *
 *   forth, and find the next actual move.                                     *
 *                                                                             *
 *******************************************************************************
 */
int ReadChessMove(TREE * RESTRICT tree, FILE * input, int wtm, int one_move) {
  int move = 0, status;
  static char text[128];
  char *tmove;

  while (move == 0) {
    status = fscanf(input, "%s", text);
    if (status <= 0)
      return -1;
    if (strcmp(text, "0-0") && strcmp(text, "0-0-0"))
      tmove = text + strspn(text, "0123456789.");
    else
      tmove = text;
    if (((tmove[0] >= 'a' && tmove[0] <= 'z') || (tmove[0] >= 'A' &&
                tmove[0] <= 'Z')) || !strcmp(tmove, "0-0")
        || !strcmp(tmove, "0-0-0")) {
      if (!strcmp(tmove, "exit"))
        return -1;
      move = InputMove(tree, 0, wtm, 1, 0, tmove);
    }
    if (one_move)
      break;
  }
  return move;
}

/*
 *******************************************************************************
 *                                                                             *
 *   ReadNextMove() is used to take a text chess move from a file, and see if  *
 *   if is legal, skipping a sometimes embedded move number (1.e4 for example) *
 *   to make PGN import easier.                                                *
 *                                                                             *
 *******************************************************************************
 */
int ReadNextMove(TREE * RESTRICT tree, char *text, int ply, int wtm) {
  char *tmove;
  int move = 0;

  if (strcmp(text, "0-0") && strcmp(text, "0-0-0"))
    tmove = text + strspn(text, "0123456789./-");
  else
    tmove = text;
  if (((tmove[0] >= 'a' && tmove[0] <= 'z') || (tmove[0] >= 'A' &&
              tmove[0] <= 'Z')) || !strcmp(tmove, "0-0")
      || !strcmp(tmove, "0-0-0")) {
    if (!strcmp(tmove, "exit"))
      return -1;
    move = InputMove(tree, ply, wtm, 1, 0, tmove);
  }
  return move;
}

/*
 *******************************************************************************
 *                                                                             *
 *   This routine reads a move from a PGN file to build an opening book or for *
 *   annotating.  It returns a 1 if a header is read, it returns a 0 if a move *
 *   is read, and returns a -1 on end of file.  It counts lines and this       *
 *   counter can be accessed by calling this function with a non-zero second   *
 *   formal parameter.                                                         *
 *                                                                             *
 *******************************************************************************
 */
int ReadPGN(FILE * input, int option) {
  static int data = 0, lines_read = 0;
  int braces = 0, parens = 0, brackets = 0, analysis = 0, last_good_line;
  static char input_buffer[4096];
  char *eof, analysis_move[64];

/*
 ************************************************************
 *                                                          *
 *  If the line counter is being requested, return it with  *
 *  no other changes being made.  If "purge" is true, clear *
 *  the current input buffer.                               *
 *                                                          *
 ************************************************************
 */
  pgn_suggested_percent = 0;
  if (!input) {
    lines_read = 0;
    data = 0;
    return 0;
  }
  if (option == -1)
    data = 0;
  if (option == -2)
    return lines_read;
/*
 ************************************************************
 *                                                          *
 *  If we don't have any data in the buffer, the first step *
 *  is to read the next line.                               *
 *                                                          *
 ************************************************************
 */
  while (FOREVER) {
    if (!data) {
      eof = fgets(input_buffer, 4096, input);
      if (!eof)
        return -1;
      if (strchr(input_buffer, '\n'))
        *strchr(input_buffer, '\n') = 0;
      if (strchr(input_buffer, '\r'))
        *strchr(input_buffer, '\r') = ' ';
      lines_read++;
      buffer[0] = 0;
      sscanf(input_buffer, "%s", buffer);
      if (buffer[0] == '[')
        do {
          char *bracket1, *bracket2, value[128];

          strcpy(buffer, input_buffer);
          bracket1 = strchr(input_buffer, '\"');
          if (bracket1 == 0)
            return 1;
          bracket2 = strchr(bracket1 + 1, '\"');
          if (bracket2 == 0)
            return 1;
          *bracket1 = 0;
          *bracket2 = 0;
          strcpy(value, bracket1 + 1);
          if (strstr(input_buffer, "Event"))
            strcpy(pgn_event, value);
          else if (strstr(input_buffer, "Site"))
            strcpy(pgn_site, value);
          else if (strstr(input_buffer, "Round"))
            strcpy(pgn_round, value);
          else if (strstr(input_buffer, "Date"))
            strcpy(pgn_date, value);
          else if (strstr(input_buffer, "WhiteElo"))
            strcpy(pgn_white_elo, value);
          else if (strstr(input_buffer, "White"))
            strcpy(pgn_white, value);
          else if (strstr(input_buffer, "BlackElo"))
            strcpy(pgn_black_elo, value);
          else if (strstr(input_buffer, "Black"))
            strcpy(pgn_black, value);
          else if (strstr(input_buffer, "Result"))
            strcpy(pgn_result, value);
          else if (strstr(input_buffer, "FEN")) {
            sprintf(buffer, "setboard %s", value);
            Option(block[0]);
            continue;
          }
          return 1;
        } while (0);
      data = 1;
    }
/*
 ************************************************************
 *                                                          *
 *  If we already have data in the buffer, it is just a     *
 *  matter of extracting the next move and returning it to  *
 *  the caller.  If the buffer is empty, another line has   *
 *  to be read in.                                          *
 *                                                          *
 ************************************************************
 */
    else {
      buffer[0] = 0;
      sscanf(input_buffer, "%s", buffer);
      if (strlen(buffer) == 0) {
        data = 0;
        continue;
      } else {
        char *skip;

        skip = strstr(input_buffer, buffer) + strlen(buffer);
        if (skip)
          memmove(input_buffer, skip, strlen(skip) + 1);
      }
/*
 ************************************************************
 *                                                          *
 *  This skips over nested {} or () characters and finds    *
 *  the 'mate', before returning any more moves.  It also   *
 *  stops if a PGN header is encountered, probably due to   *
 *  an incorrectly bracketed analysis variation.            *
 *                                                          *
 ************************************************************
 */
      last_good_line = lines_read;
      analysis_move[0] = 0;
      if (strchr(buffer, '{') || strchr(buffer, '('))
        while (FOREVER) {
          char *skip, *ch;

          analysis = 1;
          while ((ch = strpbrk(buffer, "(){}[]"))) {
            if (*ch == '(') {
              *strchr(buffer, '(') = ' ';
              if (!braces)
                parens++;
            }
            if (*ch == ')') {
              *strchr(buffer, ')') = ' ';
              if (!braces)
                parens--;
            }
            if (*ch == '{') {
              *strchr(buffer, '{') = ' ';
              braces++;
            }
            if (*ch == '}') {
              *strchr(buffer, '}') = ' ';
              braces--;
            }
            if (*ch == '[') {
              *strchr(buffer, '[') = ' ';
              if (!braces)
                brackets++;
            }
            if (*ch == ']') {
              *strchr(buffer, ']') = ' ';
              if (!braces)
                brackets--;
            }
          }
          if (analysis && analysis_move[0] == 0) {
            if (strspn(buffer, " ") != strlen(buffer)) {
              char *tmove = analysis_move;

              sscanf(buffer, "%64s", analysis_move);
              strcpy(buffer, analysis_move);
              if (strcmp(buffer, "0-0") && strcmp(buffer, "0-0-0"))
                tmove = buffer + strspn(buffer, "0123456789.");
              else
                tmove = buffer;
              if ((tmove[0] >= 'a' && tmove[0] <= 'z') || (tmove[0] >= 'A' &&
                      tmove[0] <= 'Z') || !strcmp(tmove, "0-0")
                  || !strcmp(tmove, "0-0-0"))
                strcpy(analysis_move, buffer);
              else
                analysis_move[0] = 0;
            }
          }
          if (parens == 0 && braces == 0 && brackets == 0)
            break;
          buffer[0] = 0;
          sscanf(input_buffer, "%s", buffer);
          if (strlen(buffer) == 0) {
            eof = fgets(input_buffer, 4096, input);
            if (!eof) {
              parens = 0;
              braces = 0;
              brackets = 0;
              return -1;
            }
            if (strchr(input_buffer, '\n'))
              *strchr(input_buffer, '\n') = 0;
            if (strchr(input_buffer, '\r'))
              *strchr(input_buffer, '\r') = ' ';
            lines_read++;
            if (lines_read - last_good_line >= 100) {
              parens = 0;
              braces = 0;
              brackets = 0;
              Print(4095,
                  "ERROR.  comment spans over 100 lines, starting at line %d\n",
                  last_good_line);
              break;
            }
          }
          skip = strstr(input_buffer, buffer) + strlen(buffer);
          memmove(input_buffer, skip, strlen(skip) + 1);
      } else {
        int skip;

        if ((skip = strspn(buffer, "0123456789./-"))) {
          if (skip > 1)
            memmove(buffer, buffer + skip, strlen(buffer + skip) + 1);
        }
        if (isalpha(buffer[0]) || strchr(buffer, '-')) {
          char *first, *last, *percent;

          first = input_buffer + strspn(input_buffer, " ");
          if (first == 0 || *first != '{')
            return 0;
          last = strchr(input_buffer, '}');
          if (last == 0)
            return 0;
          percent = strstr(first, "play");
          if (percent == 0)
            return 0;
          pgn_suggested_percent =
              atoi(percent + 4 + strspn(percent + 4, " "));
          return 0;
        }
      }
      if (analysis_move[0] && option == 1) {
        strcpy(buffer, analysis_move);
        return 2;
      }
    }
  }
}

/*
 *******************************************************************************
 *                                                                             *
 *   RestoreGame() resets the position to the beginning of the game, and then  *
 *   reads in the game.nnn history file to set the position up so that the     *
 *   game position matches the position at the end of the history file.        *
 *                                                                             *
 *******************************************************************************
 */
void RestoreGame(void) {
  int i, v, move;
  char cmd[16];

  if (!history_file)
    return;
  game_wtm = 1;
  InitializeChessBoard(block[0]);
  for (i = 0; i < 500; i++) {
    fseek(history_file, i * 10, SEEK_SET);
    strcpy(cmd, "");
    v = fscanf(history_file, "%s", cmd);
    if (v < 0)
      perror("RestoreGame fscanf error: ");
    if (strcmp(cmd, "pass")) {
      move = InputMove(block[0], 0, game_wtm, 1, 0, cmd);
      if (move)
        MakeMoveRoot(block[0], game_wtm, move);
      else
        break;
    }
    game_wtm = Flip(game_wtm);
  }
}

/*
 *******************************************************************************
 *                                                                             *
 *   Kibitz() is used to whisper/kibitz information to a chess server.  It has *
 *   to handle the xboard whisper/kibitz interface.                            *
 *                                                                             *
 *******************************************************************************
 */
void Kibitz(int level, int wtm, int depth, int time, int value,
    uint64_t nodes, int ip, int tb_hits, char *pv) {
  int nps;

  nps = (int) ((time) ? 100 * nodes / (uint64_t) time : nodes);
  if (!puzzling) {
    char prefix[128];

    if (!(kibitz & 16))
      sprintf(prefix, "kibitz");
    else
      sprintf(prefix, "whisper");
    switch (level) {
      case 1:
        if ((kibitz & 15) >= 1) {
          if (value > 0)
            printf("%s mate in %d moves.\n\n", prefix, value);
          if (value < 0)
            printf("%s mated in %d moves.\n\n", prefix, -value);
        }
        break;
      case 2:
        if ((kibitz & 15) >= 2) {
          printf("%s ply=%d; eval=%s; nps=%s; time=%s(%d%%)", prefix, depth,
              DisplayEvaluationKibitz(value, wtm), DisplayKMB(nps, 0),
              DisplayTimeKibitz(time), ip);
          printf("; egtb=%s\n", DisplayKMB(tb_hits, 0));
        }
        break;
      case 3:
        if ((kibitz & 15) >= 3 && (nodes > 5000 || level == 2))
          printf("%s %s\n", prefix, pv);
        break;
      case 4:
        if ((kibitz & 15) >= 4)
          printf("%s %s\n", prefix, pv);
        break;
      case 5:
        if ((kibitz & 15) >= 5 && nodes > 5000) {
          printf("%s d%d-> %s/s %s(%d%%) %s %s ", prefix, depth,
              DisplayKMB(nps, 0), DisplayTimeKibitz(time), ip,
              DisplayEvaluationKibitz(value, wtm), pv);
          if (tb_hits)
            printf("egtb=%s", DisplayKMB(tb_hits, 0));
          printf("\n");
        }
        break;
    }
    value = (wtm) ? value : -value;
    if (post && level > 1) {
      if (strstr(pv, "book"))
        printf("      %2d  %5d %7d %" PRIu64 " %s\n", depth, value, time,
            nodes, pv + 10);
      else
        printf("      %2d  %5d %7d %" PRIu64 " %s\n", depth, value, time,
            nodes, pv);
    }
    fflush(stdout);
  }
}

/*
 *******************************************************************************
 *                                                                             *
 *   Output() is used to print the principal variation whenever it changes.    *
 *                                                                             *
 *******************************************************************************
 */
void Output(TREE * RESTRICT tree) {
  int wtm, i;

/*
 ************************************************************
 *                                                          *
 *  Output the PV by walking down the path being backed up. *
 *  We do set the "age" for this move to "4" which will     *
 *  keep it in the group of "search with all threads" moves *
 *  so that it will be searched faster.                     *
 *                                                          *
 ************************************************************
 */
  wtm = root_wtm;
  if (!abort_search) {
    kibitz_depth = iteration;
    end_time = ReadClock();
    DisplayPV(tree, 6, wtm, end_time - start_time, &tree->pv[1], 0);
    for (i = 0; i < n_root_moves; i++)
      if (tree->pv[1].path[1] == root_moves[i].move)
        break;
    root_moves[i].path = tree->pv[1];
    root_moves[i].bm_age = 4;
  }
}

/*
 *******************************************************************************
 *                                                                             *
 *   Trace() is used to print the search trace output each time a node is      *
 *   traversed in the tree.                                                    *
 *                                                                             *
 *******************************************************************************
 */
void Trace(TREE * RESTRICT tree, int ply, int depth, int wtm, int alpha,
    int beta, const char *name, int mode, int phase, int order) {
  int i;

  Lock(lock_io);
  for (i = 1; i < ply; i++)
    Print(-1, "  ");
  if (phase != EVALUATION) {
    Print(-1, "%d  %s(%d) d:%2d [%s,", ply, OutputMove(tree, ply, wtm,
            tree->curmv[ply]), order, depth, DisplayEvaluation(alpha, 1));
    Print(-1, "%s] n:%" PRIu64 " %s(%c:%d)", DisplayEvaluation(beta, 1),
        tree->nodes_searched, name, (mode) ? 'P' : 'S', phase);
    Print(-1, " (t=%d)\n", tree->thread_id);
  } else {
    Print(-1, "%d window/eval(%s) = {", ply, name);
    Print(-1, "%s, ", DisplayEvaluation(alpha, 1));
    Print(-1, "%s, ", DisplayEvaluation(depth, 1));
    Print(-1, "%s}\n", DisplayEvaluation(beta, 1));
  }
  fflush(0);
  Unlock(lock_io);
}

/*
 *******************************************************************************
 *                                                                             *
 *   StrCnt() counts the number of times a character occurs in a string.       *
 *                                                                             *
 *******************************************************************************
 */
int StrCnt(char *string, char testchar) {
  int count = 0, i;

  for (i = 0; i < strlen(string); i++)
    if (string[i] == testchar)
      count++;
  return count;
}

/*
 *******************************************************************************
 *                                                                             *
 *   ValidMove() is used to verify that a move is playable.  It is mainly      *
 *   used to confirm that a move retrieved from the transposition/refutation   *
 *   and/or killer move is valid in the current position by checking the move  *
 *   against the current chess board, castling status, en passant status, etc. *
 *                                                                             *
 *******************************************************************************
 */
int ValidMove(TREE * RESTRICT tree, int ply, int wtm, int move) {
  int btm = Flip(wtm);

/*
 ************************************************************
 *                                                          *
 *  Make sure that the piece on <from> is the right color.  *
 *                                                          *
 ************************************************************
 */
  if (PcOnSq(From(move)) != ((wtm) ? Piece(move) : -Piece(move)))
    return 0;
  switch (Piece(move)) {
/*
 ************************************************************
 *                                                          *
 *  Null-moves are caught as it is possible for a killer    *
 *  move entry to be zero at certain times.                 *
 *                                                          *
 ************************************************************
 */
    case empty:
      return 0;
/*
 ************************************************************
 *                                                          *
 *  King moves are validated here if the king is moving two *
 *  squares at one time (castling moves).  Otherwise fall   *
 *  into the normal piece validation routine below.  For    *
 *  castling moves, we need to verify that the castling     *
 *  status is correct to avoid "creating" a new rook or     *
 *  king.                                                   *
 *                                                          *
 ************************************************************
 */
    case king:
      if (Abs(From(move) - To(move)) == 2) {
        if (Castle(ply, wtm) > 0) {
          if (To(move) == csq[wtm]) {
            if ((!(Castle(ply, wtm) & 2)) || (OccupiedSquares & OOO[wtm])
                || (AttacksTo(tree, csq[wtm]) & Occupied(btm))
                || (AttacksTo(tree, dsq[wtm]) & Occupied(btm))
                || (AttacksTo(tree, esq[wtm]) & Occupied(btm)))
              return 0;
          } else if (To(move) == gsq[wtm]) {
            if ((!(Castle(ply, wtm) & 1)) || (OccupiedSquares & OO[wtm])
                || (AttacksTo(tree, esq[wtm]) & Occupied(btm))
                || (AttacksTo(tree, fsq[wtm]) & Occupied(btm))
                || (AttacksTo(tree, gsq[wtm]) & Occupied(btm)))
              return 0;
          }
        } else
          return 0;
        return 1;
      }
      break;
/*
 ************************************************************
 *                                                          *
 *  Check for a normal pawn advance.                        *
 *                                                          *
 ************************************************************
 */
    case pawn:
      if (((wtm) ? To(move) - From(move) : From(move) - To(move)) < 0)
        return 0;
      if (Abs(From(move) - To(move)) == 8)
        return PcOnSq(To(move)) ? 0 : 1;
      if (Abs(From(move) - To(move)) == 16)
        return (PcOnSq(To(move)) || PcOnSq(To(move) + epdir[wtm])) ? 0 : 1;
      if (!Captured(move))
        return 0;
/*
 ************************************************************
 *                                                          *
 *  Check for an en passant capture which is somewhat       *
 *  unusual in that the [to] square does not contain the    *
 *  pawn being captured.  Make sure that the pawn being     *
 *  captured advanced two ranks the previous move.          *
 *                                                          *
 ************************************************************
 */
      if ((PcOnSq(To(move)) == 0)
          && (PcOnSq(To(move) + epdir[wtm]) == ((wtm) ? -pawn : pawn))
          && (EnPassantTarget(ply) & SetMask(To(move))))
        return 1;
      break;
/*
 ************************************************************
 *                                                          *
 *  Normal moves are all checked the same way.              *
 *                                                          *
 ************************************************************
 */
    case queen:
    case rook:
    case bishop:
      if (Attack(From(move), To(move)))
        break;
      return 0;
    case knight:
      break;
  }
/*
 ************************************************************
 *                                                          *
 *  All normal moves are validated in the same manner, by   *
 *  checking the from and to squares and also the attack    *
 *  status for completeness.                                *
 *                                                          *
 ************************************************************
 */
  return ((Captured(move) == ((wtm) ? -PcOnSq(To(move)) : PcOnSq(To(move))))
      && Captured(move) != king) ? 1 : 0;
}

/* last modified 02/26/14 */
/*
 *******************************************************************************
 *                                                                             *
 *   VerifyMove() tests a move to confirm it is absolutely legal. It shouldn't *
 *   be used inside the search, but can be used to check a 21-bit (compressed) *
 *   move to be sure it is safe to make it on the permanent game board.        *
 *                                                                             *
 *******************************************************************************
 */
int VerifyMove(TREE * RESTRICT tree, int ply, int wtm, int move) {
  unsigned moves[256], *mv, *mvp;

/*
 Generate moves, then eliminate any that are illegal.
 */
  if (move == 0)
    return 0;
  tree->status[MAXPLY] = tree->status[ply];
  mvp = GenerateCaptures(tree, MAXPLY, wtm, moves);
  mvp = GenerateNoncaptures(tree, MAXPLY, wtm, mvp);
  for (mv = &moves[0]; mv < mvp; mv++) {
    MakeMove(tree, MAXPLY, wtm, *mv);
    if (!Check(wtm) && move == *mv) {
      UnmakeMove(tree, MAXPLY, wtm, *mv);
      return 1;
    }
    UnmakeMove(tree, MAXPLY, wtm, *mv);
  }
  return 0;
}

/*
 *******************************************************************************
 *                                                                             *
 *   Windows NUMA support                                                      *
 *                                                                             *
 *******************************************************************************
 */
#if !defined(UNIX)
static BOOL(WINAPI * pGetNumaHighestNodeNumber) (PULONG);
static BOOL(WINAPI * pGetNumaNodeProcessorMask) (UCHAR, PULONGLONG);
static DWORD(WINAPI * pSetThreadIdealProcessor) (HANDLE, DWORD);
static volatile BOOL fThreadsInitialized = FALSE;
static BOOL fSystemIsNUMA = FALSE;
static ULONGLONG ullProcessorMask[256];
static ULONG ulNumaNodes;
static ULONG ulNumaNode = 0;

// Get NUMA-related information from Windows
static void WinNumaInit(void) {
  HMODULE hModule;
  ULONG ulCPU, ulNode;
  ULONGLONG ullMask;
  DWORD dwCPU;

  if (!fThreadsInitialized) {
    Lock(lock_smp);
    if (!fThreadsInitialized) {
      printf("\nInitializing multiple threads.\n");
      fThreadsInitialized = TRUE;
      hModule = GetModuleHandle("kernel32");
      pGetNumaHighestNodeNumber =
          (void *) GetProcAddress(hModule, "GetNumaHighestNodeNumber");
      pGetNumaNodeProcessorMask =
          (void *) GetProcAddress(hModule, "GetNumaNodeProcessorMask");
      pSetThreadIdealProcessor =
          (void *) GetProcAddress(hModule, "SetThreadIdealProcessor");
      if (pGetNumaHighestNodeNumber && pGetNumaNodeProcessorMask &&
          pGetNumaHighestNodeNumber(&ulNumaNodes) && (ulNumaNodes > 0)) {
        fSystemIsNUMA = TRUE;
        if (ulNumaNodes > 255)
          ulNumaNodes = 255;
        printf("System is NUMA. " PRId64 " nodes reported by Windows\n",
            ulNumaNodes + 1);
        for (ulNode = 0; ulNode <= ulNumaNodes; ulNode++) {
          pGetNumaNodeProcessorMask((UCHAR) ulNode,
              &ullProcessorMask[ulNode]);
          printf("Node " PRId64 " CPUs: ", ulNode);
          ullMask = ullProcessorMask[ulNode];
          if (0 == ullMask)
            fSystemIsNUMA = FALSE;
          else {
            ulCPU = 0;
            do {
              if (ullMask & 1)
                printf("" PRId64 " ", ulCPU);
              ulCPU++;
              ullMask >>= 1;
            } while (ullMask);
          }
          printf("\n");
        }
// Thread 0 was already started on some CPU. To simplify things further,
// exchange ProcessorMask[0] and ProcessorMask[node for that CPU],
// so ProcessorMask[0] would always be node for thread 0
        dwCPU =
            pSetThreadIdealProcessor(GetCurrentThread(), MAXIMUM_PROCESSORS);
        printf("Current ideal CPU is %llu\n", dwCPU);
        pSetThreadIdealProcessor(GetCurrentThread(), dwCPU);
        if ((((DWORD) - 1) != dwCPU) && (MAXIMUM_PROCESSORS != dwCPU)
            && !(ullProcessorMask[0] & (1u << dwCPU))) {
          for (ulNode = 1; ulNode <= ulNumaNodes; ulNode++) {
            if (ullProcessorMask[ulNode] & (1u << dwCPU)) {
              printf("Exchanging nodes 0 and " PRId64 "\n", ulNode);
              ullMask = ullProcessorMask[ulNode];
              ullProcessorMask[ulNode] = ullProcessorMask[0];
              ullProcessorMask[0] = ullMask;
              break;
            }
          }
        }
      } else
        printf("System is SMP, not NUMA.\n");
    }
    Unlock(lock_smp);
  }
}

// Start thread. For NUMA system set its affinity.
#  if (CPUS > 1)
pthread_t NumaStartThread(void *func, void *args) {
  HANDLE hThread;
  ULONGLONG ullMask;

  WinNumaInit();
  if (fSystemIsNUMA) {
    ulNumaNode++;
    if (ulNumaNode > ulNumaNodes)
      ulNumaNode = 0;
    ullMask = ullProcessorMask[ulNumaNode];
    printf("Starting thread on node " PRId64 " CPU mask %I64d\n", ulNumaNode,
        ullMask);
    SetThreadAffinityMask(GetCurrentThread(), (DWORD_PTR) ullMask);
    hThread = (HANDLE) _beginthreadex(0, 0, func, args, CREATE_SUSPENDED, 0);
    SetThreadAffinityMask(hThread, (DWORD_PTR) ullMask);
    ResumeThread(hThread);
    SetThreadAffinityMask(GetCurrentThread(), ullProcessorMask[0]);
  } else
    hThread = (HANDLE) _beginthreadex(0, 0, func, args, 0, 0);
  return hThread;
}
#  endif

// Allocate memory for thread #N
void *WinMalloc(size_t cbBytes, int iThread) {
  HANDLE hThread;
  DWORD_PTR dwAffinityMask;
  void *pBytes;
  ULONG ulNode;

  WinNumaInit();
  if (fSystemIsNUMA) {
    ulNode = iThread % (ulNumaNodes + 1);
    hThread = GetCurrentThread();
    dwAffinityMask = SetThreadAffinityMask(hThread, ullProcessorMask[ulNode]);
    pBytes = VirtualAlloc(NULL, cbBytes, MEM_COMMIT, PAGE_READWRITE);
    if (pBytes == NULL)
      ExitProcess(GetLastError());
    memset(pBytes, 0, cbBytes);
    SetThreadAffinityMask(hThread, dwAffinityMask);
  } else {
    pBytes = VirtualAlloc(NULL, cbBytes, MEM_COMMIT, PAGE_READWRITE);
    if (pBytes == NULL)
      ExitProcess(GetLastError());
    memset(pBytes, 0, cbBytes);
  }
  return pBytes;
}

// Allocate interleaved memory
void *WinMallocInterleaved(size_t cbBytes, int cThreads) {
  char *pBase;
  char *pEnd;
  char *pch;
  HANDLE hThread;
  DWORD_PTR dwAffinityMask;
  ULONG ulNode;
  SYSTEM_INFO sSysInfo;
  size_t dwStep;
  int iThread;
  DWORD dwPageSize;             // the page size on this computer
  LPVOID lpvResult;

  WinNumaInit();
  if (fSystemIsNUMA && (cThreads > 1)) {
    GetSystemInfo(&sSysInfo); // populate the system information structure
    dwPageSize = sSysInfo.dwPageSize;
// Reserve pages in the process's virtual address space.
    pBase = (char *) VirtualAlloc(NULL, cbBytes, MEM_RESERVE, PAGE_NOACCESS);
    if (pBase == NULL) {
      printf("VirtualAlloc() reserve failed\n");
      CraftyExit(0);
    }
// Now walk through memory, committing each page
    hThread = GetCurrentThread();
    dwStep = dwPageSize * cThreads;
    pEnd = pBase + cbBytes;
    for (iThread = 0; iThread < cThreads; iThread++) {
      ulNode = iThread % (ulNumaNodes + 1);
      dwAffinityMask =
          SetThreadAffinityMask(hThread, ullProcessorMask[ulNode]);
      for (pch = pBase + iThread * dwPageSize; pch < pEnd; pch += dwStep) {
        lpvResult = VirtualAlloc(pch, // next page to commit
            dwPageSize, // page size, in bytes
            MEM_COMMIT, // allocate a committed page
            PAGE_READWRITE); // read/write access
        if (lpvResult == NULL)
          ExitProcess(GetLastError());
        memset(lpvResult, 0, dwPageSize);
      }
      SetThreadAffinityMask(hThread, dwAffinityMask);
    }
  } else {
    pBase = VirtualAlloc(NULL, cbBytes, MEM_COMMIT, PAGE_READWRITE);
    if (pBase == NULL)
      ExitProcess(GetLastError());
    memset(pBase, 0, cbBytes);
  }
  return (void *) pBase;
}

// Free interleaved memory
void WinFreeInterleaved(void *pMemory, size_t cBytes) {
  VirtualFree(pMemory, 0, MEM_RELEASE);
}
#endif

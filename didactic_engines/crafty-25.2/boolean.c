#include "chess.h"
#include "data.h"
/* last modified 02/16/14 */
/*
 *******************************************************************************
 *                                                                             *
 *   This group of procedures provide the three basic bitboard operators,      *
 *   MSB(x) that determines the Most Significant Bit, LSB(x) that determines   *
 *   the Least Significant Bit, and PopCnt(x) which returns the number of one  *
 *   bits set in the word.                                                     *
 *                                                                             *
 *   We prefer to use hardware facilities (such as intel BSF/BSR) when they    *
 *   are available, otherwise we resort to C and table lookups to do this in   *
 *   the most efficient way possible.                                          *
 *                                                                             *
 *******************************************************************************
 */
#if !defined(INLINEASM)
int MSB(uint64_t arg1) {
  if (arg1 >> 48)
    return msb[arg1 >> 48] + 48;
  if (arg1 >> 32 & 65535)
    return msb[arg1 >> 32 & 65535] + 32;
  if (arg1 >> 16 & 65535)
    return msb[arg1 >> 16 & 65535] + 16;
  return msb[arg1 & 65535];
}

int LSB(uint64_t arg1) {
  if (arg1 & 65535)
    return lsb[arg1 & 65535];
  if (arg1 >> 16 & 65535)
    return lsb[arg1 >> 16 & 65535] + 16;
  if (arg1 >> 32 & 65535)
    return lsb[arg1 >> 32 & 65535] + 32;
  return lsb[arg1 >> 48] + 48;
}

int PopCnt(uint64_t arg1) {
  int c;

  for (c = 0; arg1; c++)
    arg1 &= arg1 - 1;
  return c;
}
#endif

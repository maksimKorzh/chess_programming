/* last modified 01/18/09 */
/*
 *******************************************************************************
 *                                                                             *
 *   This module is designed for the most efficient compiling, as it includes  *
 *   all of the source files into one large wad so that the compiler can see   *
 *   all function calls and inline whatever is appropriate.  The includes are  *
 *   loosely ordered so that the most common functions occur first, to help    *
 *   with cache layout when the code is actually loaded.                       *
 *                                                                             *
 *******************************************************************************
 */
#include "iterate.c"
#include "search.c"
#include "movgen.c"
#include "make.c"
#include "unmake.c"
#include "thread.c"
#include "repeat.c"
#include "next.c"
#include "history.c"
#include "quiesce.c"
#include "evaluate.c"
#include "hash.c"
#include "attacks.c"
#include "see.c"
#include "utility.c"
#include "tbprobe.c"
#include "book.c"
#include "autotune.c"
#include "analyze.c"
#include "annotate.c"
#include "bench.c"
#include "data.c"
#include "drawn.c"
#include "edit.c"
#include "epd.c"
#include "epdglue.c"
#include "evtest.c"
#include "init.c"
#include "input.c"
#include "interrupt.c"
#include "learn.c"
#include "main.c"
#include "option.c"
#include "output.c"
#include "ponder.c"
#include "resign.c"
#include "root.c"
#include "setboard.c"
#include "test.c"
#include "time.c"
#include "validate.c"

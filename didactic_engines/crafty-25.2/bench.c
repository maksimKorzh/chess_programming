#include "chess.h"
#include "data.h"
/* last modified 09/29/16 */
/*
 *******************************************************************************
 *                                                                             *
 *   Bench() runs a 64 position benchmark during the build process.  The test  *
 *   positons are hard-coded. This is designed as a stand-alone benchmark to   *
 *   comper different machines, or as a profile-guided-optimization test that  *
 *   produces good profile data.                                               *
 *                                                                             *
 *******************************************************************************
 */
int Bench(int increase, int autotune) {
  uint64_t nodes = 0;
  int old_do, old_st, old_sd, total_time_used, pos, old_mt = smp_max_threads;
  FILE *old_books, *old_book;
  TREE *const tree = block[0];
  char fen[64][80] = {
    {"3q2k1/pb3p1p/4pbp1/2r5/PpN2N2/1P2P2P/5PP1/Q2R2K1 b"},
    {"2r2rk1/1bqnbpp1/1p1ppn1p/pP6/N1P1P3/P2B1N1P/1B2QPP1/R2R2K1 b"},
    {"3rr1k1/pp3pp1/1qn2np1/8/3p4/PP1R1P2/2P1NQPP/R1B3K1 b"},
    {"4r1k1/r1q2ppp/ppp2n2/4P3/5Rb1/1N1BQ3/PPP3PP/R5K1 w"},
    {"1r3k2/4q3/2Pp3b/3Bp3/2Q2p2/1p1P2P1/1P2KP2/3N4 w"},
    {"3r1rk1/p5pp/bpp1pp2/8/q1PP1P2/b3P3/P2NQRPP/1R2B1K1 b"},
    {"8/R7/2q5/8/6k1/8/1P5p/K6R w"},
    {"2r3k1/1p2q1pp/2b1pr2/p1pp4/6Q1/1P1PP1R1/P1PN2PP/5RK1 w"},
    {"4rrk1/pp1n3p/3q2pQ/2p1pb2/2PP4/2P3N1/P2B2PP/4RRK1 b"},
    {"5r1k/6p/1n2Q2p/4p//7P/PP4PK/R1B1q/ w"},
    {"4b3/p3kp2/6p1/3pP2p/2pP1P2/4K1P1/P3N2P/8 w"},
    {"r1bqkb1r/4npp1/p1p4p/1p1pP1B1/8/1B6/PPPN1PPP/R2Q1RK1 w"},
    {"7k/3p2pp/4q3/8/4Q3/5Kp1/P6b/8 w"},
    {"2r1nrk1/p2q1ppp/bp1p4/n1pPp3/P1P1P3/2PBB1N1/4QPPP/R4RK1 w"},
    {"8/3k4/8/8/8/4B3/4KB2/2B5 w"},
    {"6k1/6p1/P6p/r1N5/5p2/7P/1b3PP1/4R1K1 w"},
    {"8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w"},
    {"r2q1rk1/1ppnbppp/p2p1nb1/3Pp3/2P1P1P1/2N2N1P/PPB1QP2/R1B2RK1 b"},
    {"2kr1bnr/pbpq4/2n1pp2/3p3p/3P1P1B/2N2N1Q/PPP3PP/2KR1B1R w"},
    {"8/2p5/8/2kPKp1p/2p4P/2P5/3P4/8 w"},
    {"3rn2k/ppb2rpp/2ppqp2/5N2/2P1P3/1P5Q/PB3PPP/3RR1K1 w"},
    {"rnbqkb1r/p3pppp/1p6/2ppP3/3N4/2P5/PPP1QPPP/R1B1KB1R w"},
    {"2r3k1/pppR1pp1/4p3/4P1P1/5P2/1P4K1/P1P5/8 w"},
    {"/k/rnn////5RBB/K/ w"},
    {"r1bq1rk1/ppp1nppp/4n3/3p3Q/3P4/1BP1B3/PP1N2PP/R4RK1 w"},
    {"8/8/3P3k/8/1p6/8/1P6/1K3n2 b"},
    {"8/2p4P/8/kr6/6R1/8/8/1K6 w"},
    {"r2qnrnk/p2b2b1/1p1p2pp/2pPpp2/1PP1P3/PRNBB3/3QNPPP/5RK1 w"},
    {"5k2/7R/4P2p/5K2/p1r2P1p/8/8/8 b"},
    {"rq3rk1/ppp2ppp/1bnpb3/3N2B1/3NP3/7P/PPPQ1PP1/2KR3R w"},
    {"2q1rr1k/3bbnnp/p2p1pp1/2pPp3/PpP1P1P1/1P2BNNP/2BQ1PRK/7R b"},
    {"2rqkb1r/ppp2p2/2npb1p1/1N1Nn2p/2P1PP2/8/PP2B1PP/R1BQK2R b"},
    {"r2q1rk1/4bppp/p2p4/2pP4/3pP3/3Q4/PP1B1PPP/R3R1K1 w"},
    {"6k1/6p1/6Pp/ppp5/3pn2P/1P3K2/1PP2P2/3N4 b"},
    {"8/3p3B/5p2/5P2/p7/PP5b/k7/6K1 w"},
    {"r1bbk1nr/pp3p1p/2n5/1N4p1/2Np1B2/8/PPP2PPP/2KR1B1R w"},
    {"8/1p3pp1/7p/5P1P/2k3P1/8/2K2P2/8 w"},
    {"8/8/8/5N2/8/p7/8/2NK3k w"},
    {"r1bq1r1k/b1p1npp1/p2p3p/1p6/3PP3/1B2NN2/PP3PPP/R2Q1RK1 w"},
    {"8/8/1P6/5pr1/8/4R3/7k/2K5 w"},
    {"4k2r/1pb2ppp/1p2p3/1R1p4/3P4/2r1PN2/P4PPP/1R4K1 b"},
    {"r1bq1r1k/1pp1n1pp/1p1p4/4p2Q/4Pp2/1BNP4/PPP2PPP/3R1RK1 w"},
    {"3r1k2/4npp1/1ppr3p/p6P/P2PPPP1/1NR5/5K2/2R5 w"},
    {"r3r1k1/2p2ppp/p1p1bn2/8/1q2P3/2NPQN2/PPP3PP/R4RK1 b"},
    {"6k1/4pp1p/3p2p1/P1pPb3/R7/1r2P1PP/3B1P2/6K1 w"},
    {"r1bqk2r/pp2bppp/2p5/3pP3/P2Q1P2/2N1B3/1PP3PP/R4RK1 b"},
    {"r1q2rk1/2p1bppp/2Pp4/p6b/Q1PNp3/4B3/PP1R1PPP/2K4R w"},
    {"8/pp2r1k1/2p1p3/3pP2p/1P1P1P1P/P5KR/8/8 w"},
    {"3b4/5kp1/1p1p1p1p/pP1PpP1P/P1P1P3/3KN3/8/8 w"},
    {"r1bq1rk1/pp2ppbp/2np2p1/2n5/P3PP2/N1P2N2/1PB3PP/R1B1QRK1 b"},
    {"r3r1k1/ppqb1ppp/8/4p1NQ/8/2P5/PP3PPP/R3R1K1 b"},
    {"rnbqkb1r/p3pppp/1p6/2ppP3/3N4/2P5/PPP1QPPP/R1B1KB1R w"},
    {"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w"},
    {"/k/3p/p2P1p/P2P1P///K/ w"},
    {"rnb2r1k/pp2p2p/2pp2p1/q2P1p2/8/1Pb2NP1/PB2PPBP/R2Q1RK1 w"},
    {"8/6pk/1p6/8/PP3p1p/5P2/4KP1q/3Q4 w"},
    {"8/3p4/p1bk3p/Pp6/1Kp1PpPp/2P2P1P/2P5/5B2 b"},
    {"1nk1r1r1/pp2n1pp/4p3/q2pPp1N/b1pP1P2/B1P2R2/2P1B1PP/R2Q2K1 w"},
    {"2K5/p7/7P/5pR1/8/5k2/r7/8 w"},
    {"3rr3/2pq2pk/p2p1pnp/8/2QBPP2/1P6/P5PP/4RRK1 b"},
    {"r1b2rk1/2q1b1pp/p2ppn2/1p6/3QP3/1BN1B3/PPP3PP/R4RK1 w"},
    {"8/8/8/8/5kp1/P7/8/1K1N4 w"},
    {"r4k2/pb2bp1r/1p1qp2p/3pNp2/3P1P2/2N3P1/PPP1Q2P/2KRR3 w"},
    {"r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w"}
  };
  int fen_depth = 16;

 /*
  ************************************************************
  *                                                          *
  *  Initialize.                                             *
  *                                                          *
  ************************************************************
  */
  total_time_used = 0;
  old_st = search_time_limit;
  old_sd = search_depth;
  old_do = display_options;
  search_time_limit = 90000;
  display_options = 1;
  old_book = book_file;
  book_file = 0;
  old_books = books_file;
  books_file = 0;
  if (!autotune) {
    if (increase)
      Print(4095,
          "Running serial benchmark (modifying depth by %d plies). . .\n",
          increase);
    else
      Print(4095, "Running serial benchmark. . .\n");
    fflush(stdout);
  }
 /*
  ************************************************************
  *                                                          *
  *  Now we loop through the 64 positions.  We use the      *
  *  ReadParse() procedure to break the FEN into tokens and  *
  *  then call SetBoard() to set up the positions.  Then a   *
  *  call to Iterate() and we are done.                      *
  *                                                          *
  ************************************************************
  */
  for (pos = 0; pos < 64; pos++) {
    strcpy(buffer, fen[pos]);
    nargs = ReadParse(buffer, args, " \t;=");
    SetBoard(tree, nargs, args, 0);
    search_depth = fen_depth + increase;
    last_pv.pathd = 0;
    thinking = 1;
    tree->status[1] = tree->status[0];
    InitializeHashTables(0);
    Iterate(game_wtm, think, 0);
    thinking = 0;
    nodes += tree->nodes_searched;
    total_time_used += (program_end_time - program_start_time);
    nodes_per_second =
        (uint64_t) tree->nodes_searched * 100 /
        Max((uint64_t) program_end_time - program_start_time, 1);
    if (pos % 7 == 0)
      Print(4095, "pos: ");
    Print(4095, "%2d(%s) ", pos + 1, DisplayKMB(nodes_per_second, 0));
    if (pos % 7 == 6)
      Print(4095, "\n");
    fflush(stdout);
  }
 /*
  ************************************************************
  *                                                          *
  *  Serial benchmark done.  Now dump the results.           *
  *                                                          *
  ************************************************************
  */
  if (!autotune)
    printf("\n");
  if (!autotune) {
    Print(4095, "\nTotal nodes: %" PRIu64 "\n", nodes);
    Print(4095, "Raw nodes per second: %d\n",
        (int) ((double) nodes / ((double) total_time_used / (double) 100.0)));
    Print(4095, "Total elapsed time: %.2f\n\n",
        ((double) total_time_used / (double) 100.0));
  }
 /*
  ************************************************************
  *                                                          *
  *  Now we repeat for two threads to provide PGO data for   *
  *  the compiler.                                           *
  *                                                          *
  ************************************************************
  */
  if (smp_max_threads == 0) {
    smp_max_threads = 2;
    Print(4095, "Running SMP benchmark (%d threads)...\n", smp_max_threads);
    fflush(stdout);
    Print(4095, "pos: ");
    for (pos = 0; pos < 2 && old_mt == 0; pos++) {
      strcpy(buffer, fen[pos]);
      nargs = ReadParse(buffer, args, " \t;=");
      SetBoard(tree, nargs, args, 0);
      search_depth = fen_depth + increase;
      last_pv.pathd = 0;
      thinking = 1;
      tree->status[1] = tree->status[0];
      InitializeHashTables(0);
      Iterate(game_wtm, think, 0);
      thinking = 0;
      nodes += tree->nodes_searched;
      total_time_used += (program_end_time - program_start_time);
      nodes_per_second =
          (uint64_t) tree->nodes_searched * 100 /
          Max((uint64_t) program_end_time - program_start_time, 1);
      Print(4095, "%2d(%s) ", pos + 1, DisplayKMB(nodes_per_second, 0));
    }
  }
 /*
  ************************************************************
  *                                                          *
  *  Benchmark done.  Now dump the results.                  *
  *                                                          *
  ************************************************************
  */
  if (!autotune)
    printf("\n");
  if (!autotune && old_mt == 0) {
    Print(4095, "\nTotal nodes: %" PRIu64 "\n", nodes);
    Print(4095, "Raw nodes per second: %d\n",
        (int) ((double) nodes / ((double) total_time_used / (double) 100.0)));
    Print(4095, "Total elapsed time: %.2f\n\n",
        ((double) total_time_used / (double) 100.0));
  }
  input_stream = stdin;
  early_exit = 99;
  display_options = old_do;
  search_time_limit = old_st;
  search_depth = old_sd;
  smp_max_threads = Max(0, old_mt);
  books_file = old_books;
  book_file = old_book;
  InitializeChessBoard(tree);
  return total_time_used;
}

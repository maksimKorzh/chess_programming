#include "chess.h"
#include "data.h"
#if defined(UNIX)
#  include <unistd.h>
#  include <pwd.h>
#  include <sys/types.h>
#endif
#include <signal.h>
#if defined(SYZYGY)
#  include "tbprobe.h"
#endif
/* last modified 08/03/16 */
/*
 *******************************************************************************
 *                                                                             *
 *  Crafty, copyright 1996-2016 by Robert M. Hyatt, Ph.D.                      *
 *                                                                             *
 *  Crafty is a team project consisting of the following members.  These are   *
 *  the people involved in the continuing development of this program, there   *
 *  are no particular members responsible for any specific aspect of Crafty,   *
 *  although R. Hyatt wrote 99%+ of the existing code, excepting the Magic .   *
 *  move stuff by Pradu Kaanan, syzygy code written by Ronald de Man, and the  *
 *  epd stuff written by S. Edwards.                                           *
 *                                                                             *
 *     Robert Hyatt, Pelham, AL.                                               *
 *     Mike Byrne, Pen Argyl, PA.                                              *
 *     Tracy Riegle, Hershey, PA.                                              *
 *     Peter Skinner, Edmonton, AB  Canada.                                    *
 *                                                                             *
 *  All rights reserved.  No part of this program may be reproduced in any     *
 *  form or by any means, for other than your personal use, without the        *
 *  express written permission of the authors.  This program may not be used   *
 *  in whole, nor in part, to enter any computer chess competition without     *
 *  written permission from the authors.  Such permission will include the     *
 *  requirement that the program be entered under the name "Crafty" so that    *
 *  the program's ancestry will be known.                                      *
 *                                                                             *
 *  Copies of the source must contain the original copyright notice intact.    *
 *                                                                             *
 *  Any changes made to this software must also be made public to comply with  *
 *  the original intent of this software distribution project.  These          *
 *  restrictions apply whether the distribution is being done for free or as   *
 *  part or all of a commercial product.  The authors retain sole ownership    *
 *  and copyright on this program except for 'personal use' explained below.   *
 *                                                                             *
 *  Personal use includes any use you make of the program yourself, either by  *
 *  playing games with it yourself, or allowing others to play it on your      *
 *  machine,  and requires that if others use the program, it must be clearly  *
 *  identified as "Crafty" to anyone playing it (on a chess server as one      *
 *  example).  Personal use does not allow anyone to enter this into a chess   *
 *  tournament where other program authors are invited to participate.  IE you *
 *  can do your own local tournament, with Crafty + other programs, since this *
 *  is for your personal enjoyment.  But you may not enter Crafty into an      *
 *  event where it will be in competition with other programs/programmers      *
 *  without permission as stated previously.                                   *
 *                                                                             *
 *  Crafty is the "son" (direct descendent) of Cray Blitz.  it is designed     *
 *  totally around the bit-board data structure for reasons of speed of ex-    *
 *  ecution, ease of adding new knowledge, and a *significantly* cleaner       *
 *  overall design.  it is written totally in ANSI C with some few UNIX system *
 *  calls required for I/O, etc.                                               *
 *                                                                             *
 *  main() is the driver for the chess program.  Its primary function is to    *
 *  cycle between asking the user for a move and calling for a tree search     *
 *  to produce a move for the program.  After accepting an input string from   *
 *  the user, this string is first passed to Option() which checks to see if   *
 *  it is a command to the program.  If Option() returns a non-zero result,    *
 *  the input was a command and was executed, otherwise the input should be    *
 *  passed to input() for conversion to the internal move format.              *
 *                                                                             *
 *  The following diagram shows how bit numbers / squares match up in Crafty's *
 *  bitboard arrays.  Note that bit zero (LSB) corresponds to square A, which  *
 *  makes this a mental challenge to take a 64 bit value and visualize it as   *
 *  chess board, since the files are reversed.  This was done for the          *
 *  following reasons:  (1) bit 0 needs to be the LSB, and bit 63 needs to be  *
 *  the MSB so that the Intel BSF/BSR instructions return the proper square    *
 *  number without any remapping.  (2) the lower 3 bits (file number) are now  *
 *  0 for A, 1 for B, ..., 7 for H.  The upper 3 bits (rank number) are then   *
 *  0 for rank 1, 1 for rank 2, ..., 7 for rank 8.                             *
 *                                                                             *
 *  A8 B8 C8 D8 E8 F8 G8 H8                                                    *
 *  A7 B7 C7 D7 E7 F7 G7 H7                                                    *
 *  A6 B6 C6 D6 E6 F6 G6 H6                                                    *
 *  A5 B5 C5 D5 E5 F5 G5 H5                                                    *
 *  A4 B4 C4 D4 E4 F4 G4 H4                                                    *
 *  A3 B3 C3 D3 E3 F3 G3 H3                                                    *
 *  A2 B2 C2 D2 E2 F2 G2 H2                                                    *
 *  A1 B1 C1 D1 E1 F1 G1 H1                                                    *
 *                                                                             *
 *  56 57 58 59 60 61 62 63                                                    *
 *  48 49 50 51 52 53 54 55                                                    *
 *  40 41 42 43 44 45 46 47                                                    *
 *  32 33 34 35 36 37 38 39                                                    *
 *  24 25 26 27 28 29 30 31                                                    *
 *  16 17 18 19 20 21 22 23                                                    *
 *   8  9 10 11 12 13 14 15                                                    *
 *   0  1  2  3  4  5  6  7                                                    *
 *                                                                             *
 *  version  description                                                       *
 *                                                                             *
 *    1.0    First version of the bit-board program to play a legitimate game  *
 *           even though significant features are missing:  transposition      *
 *           table, sophisticated scoring, etc.                                *
 *                                                                             *
 *    1.1    Added the minimal window search.  At each ply in the tree, the    *
 *           first branch is searched with the normal alpha/beta window, while *
 *           remaining nodes are searched with value/value+1 which was         *
 *           returned from searching the first branch.  If such a search       *
 *           returns the upper bound, this position is once again searched     *
 *           with the normal window.                                           *
 *                                                                             *
 *    1.2    Added the classic null-move search.  The program searches the     *
 *           sub-tree below the null move (typically) one play shallower than  *
 *           the normal search, saving a lot of time.                          *
 *                                                                             *
 *    1.3    Added the transposition table support.  This also includes the    *
 *           code in Iterate() necessary to propagate the principal variation  *
 *           from one iteration to the next to improve move ordering.          *
 *                                                                             *
 *    1.4    Modified the transposition table to use three tables, one for 64  *
 *           bits of white entry, one for 64 bits of black entry, and one with *
 *           the remaining 32 bits of white and black entry combined into one  *
 *           64 bit word.  Eliminated the "bit fields" since they seemed to be *
 *           erratic on most compilers and made portability nearly impossible. *
 *                                                                             *
 *    1.5    Pawn scoring added.  This required three additions to the code:   *
 *           (1) InitializePawnMasks() which produces the necessary masks to   *
 *           let us efficiently detect isolated, backward, passed, etc. pawns; *
 *           (2) a pawn hash table to store recently computed pawn scores so   *
 *           it won't be too expensive to do complex analysis;  (3) Evaluate() *
 *           now evaluates pawns if the current pawn position is not in the    *
 *           pawn hash table.                                                  *
 *                                                                             *
 *    1.6    Piece scoring added, although it is not yet complete.  Also, the  *
 *           search now uses the familiar zero-width window search for all but *
 *           the first move at the root, in order to reduce the size of the    *
 *           tree to a nearly optimal size.  This means that a move that is    *
 *           only one point better than the first move will "fail high" and    *
 *           have to be re-searched a second time to get the true value.  If   *
 *           the new best move is significantly better, it may have to be      *
 *           searched a third time to increase the window enough to return the *
 *           score.                                                            *
 *                                                                             *
 *    1.7    Replaced the old "killer" move ordering heuristic with the newer  *
 *           "history" ordering heuristic.   This version uses the 12-bit key  *
 *           formed by <from><to> to index into the history table.             *
 *                                                                             *
 *    1.8    Added pondering for both PC and UNIX-based machines.  Also other  *
 *           improvements include the old Cray Blitz algorithm that takes the  *
 *           P.V. returned by the tree search, and "extends" it by looking up  *
 *           positions in the transposition table, which improves move         *
 *           ordering significantly.  Repetitions are now handled correctly.   *
 *                                                                             *
 *    1.9    Added an opening book with flags to control which moves are       *
 *           selected for play.  The book maintenance code is stubbed directly *
 *           into the program, so that no additional executables are needed.   *
 *                                                                             *
 *    1.10   Added king safety + hashing (as done in the old Cray Blitz).      *
 *           Nothing novel other than the use of bit-masks to speed up some of *
 *           the tests.                                                        *
 *                                                                             *
 *    1.11   Additional evaluation knowledge plus log_file so that program     *
 *           saves a record of the game statistics for later analysis.  Added  *
 *           the "annotate" command to make the program analyze a game or part *
 *           of a game for testing/debugging.                                  *
 *                                                                             *
 *    1.12   Added ICS (Internet Chess Server) support for Xboard support so   *
 *           that Crafty can play on ICS in an automated manner.  Added new    *
 *           commands to be compatible with Xboard.  Crafty also has a new     *
 *           command-line interface that allows options to be entered on the   *
 *           command-line directly, ie:  "crafty alarm=off verbose=2" will     *
 *           start the program, set the move alarm off, and set verbose to     *
 *           2.  This allows an "alias" to easily customize the program.       *
 *                                                                             *
 *    1.13   Added time-control logic to the program.  It now monitors how     *
 *           much time both it and its opponent uses when thinking about a     *
 *           move to make.  When pondering, it only times itself from the      *
 *           point that a move is actually entered.                            *
 *                                                                             *
 *    1.14   Added the piece/square tables to the program to move static       *
 *           scoring to the root of the tree since it never changes (things    *
 *           like centralization of pieces, etc.)  also some minor tuning of   *
 *           numbers to bring positional score "swings" down some.             *
 *                                                                             *
 *    1.15   Added edit so that positions can be altered (and so ICS games     *
 *           can be resumed after they are adjourned).  Also added a "force"   *
 *           command to force the program to play a different move than the    *
 *           search selected.  Search timing allocation slightly altered to    *
 *           improve ICS performance.                                          *
 *                                                                             *
 *    1.16   Significant adjustments to evaluation weights to bring positional *
 *           scores down to below a pawn for "normal" positions.  Some small   *
 *           "tweaks" to king safety as well to keep the king safer.           *
 *                                                                             *
 *    1.17   Added "development" evaluation routine to encourage development   *
 *           and castling before starting on any tactical "sorties."  Also     *
 *           repaired many "bugs" in evaluation routines.                      *
 *                                                                             *
 *    1.18   Added the famous Cray Blitz "square of the king" passed pawn      *
 *           evaluation routine.  This will evaluate positions where one       *
 *           side has passed pawns and the other side has no pieces, to see    *
 *           if the pawn can outrun the defending king and promote.  Note that *
 *           this is ridiculously fast because it only requires one AND        *
 *           to declare that a pawn can't be caught!                           *
 *                                                                             *
 *    2.0    initial version preparing for search extension additions.  This   *
 *           version has a new "next_evasion()" routine that is called when    *
 *           the king is in check.  It generates sensible (nearly all are      *
 *           legal) moves which include capturing the checking piece (if there *
 *           is only one), moving the king (but not along the checking ray),   *
 *           and interpositions that block the checking ray (if there is only  *
 *           one checking piece.                                               *
 *                                                                             *
 *    2.1    Search is now broken down into three cleanly-defined phases:      *
 *           (1) basic full-width search [Search()]; (2) extended tactical     *
 *           search [extend()] which includes winning/even captures and then   *
 *           passed pawn pushes to the 6th or 7th rank (if the pawn pushes     *
 *           are safe);  (3) normal quiescence search [Quiesce()] which only   *
 *           includes captures that appear to gain material.                   *
 *                                                                             *
 *    2.2    King safety code re-written and cleaned up.  Crafty was giving    *
 *           multiple penalties which was causing the king safety score to be  *
 *           extremely large (and unstable.)  Now, if it finds something wrong *
 *           with king safety, it only penalizes a weakness once.              *
 *                                                                             *
 *    2.3    King safety code modified further, penalties were significant     *
 *           enough to cause search anomolies.  Modified rook scoring to avoid *
 *           having the rook trapped in the corner when the king is forced to  *
 *           move rather than castle, and to keep the rook in a position to be *
 *           able to reach open files in one move, which avoids getting them   *
 *           into awkward positions where they become almost worthless.        *
 *                                                                             *
 *    2.4    King safety code completely rewritten.  It now analyzes "defects" *
 *           in the king safety field and counts them as appropriate.  These   *
 *           defects are then summed up and used as an index into a vector of  *
 *           of values that turns them into a score in a non-linear fashion.   *
 *           Increasing defects quickly "ramp up" the score to about 1/3+ of   *
 *           a pawn, then additional faults slowly increase the penalty.       *
 *                                                                             *
 *    2.5    First check extensions added.  In the extension search (stage     *
 *           between full-width and captures-only, up to two checking moves    *
 *           can be included, if there are checks in the full-width part of    *
 *           the search.  If only one check occurs in the full-width, then     *
 *           only one check will be included in the extension phase of the     *
 *           selective search.                                                 *
 *                                                                             *
 *    2.6    Evaluation modifications attempting to cure Crafty's frantic      *
 *           effort to develop all its pieces without giving a lot of regard   *
 *           to the resulting position until after the pieces are out.         *
 *                                                                             *
 *    2.7    New evaluation code to handle the "outside passed pawn" concept.  *
 *           Crafty now understands that a passed pawn on the side away from   *
 *           the rest of the pawns is a winning advantage due to decoying the  *
 *           king away from the pawns to prevent the passer from promoting.    *
 *           The advantage increases as material is removed from the board.    *
 *                                                                             *
 *    3.0    The 3.* series of versions will primarily be performance en-      *
 *           hancements.  The first version (3.0) has a highly modified        *
 *           version of MakeMove() that tries to do no unnecessary work.  It   *
 *           is about 25% faster than old version of MakeMove() which makes    *
 *           Crafty roughly 10% faster.  Also calls to Mask() have been        *
 *           replaced by constants (which are replaced by calls to Mask() on   *
 *           the Crays for speed) to eliminate function calls.                 *
 *                                                                             *
 *    3.1    Significantly modified king safety again, to better detect and    *
 *           react to king-side attacks.  Crafty now uses the recursive null-  *
 *           move search to depth-2 instead of depth-1, which results in       *
 *           slightly improved speed.                                          *
 *                                                                             *
 *    3.2    Null-move restored to depth-1.  Depth-2 proved unsafe as Crafty   *
 *           was overlooking tactical moves, particularly against itself,      *
 *           which lost several "won" games.                                   *
 *                                                                             *
 *    3.3    Additional king-safety work.  Crafty now uses the king-safety     *
 *           evaluation routines to compare king safety for both sides.  If    *
 *           one side is "in distress" pieces are attracted to that king in    *
 *           "big hurry" to either attack or defend.                           *
 *                                                                             *
 *    3.4    "Threat extensions" added.  Simply, this is a null-move search    *
 *           used to determine if a move is good only because it is a horizon  *
 *           effect type move.  We do a null move search after a move fails    *
 *           high anywhere in the tree.  The window is normally lowered by 1.5 *
 *           pawns, the idea being that if the fail-high move happens in a     *
 *           position that fails "really low" with a null move, then this move *
 *           might be a horizon move.  To test this, re-search this move with  *
 *           the depth increased by one.                                       *
 *                                                                             *
 *    3.5    50-move rule implemented.  A count of moves since the last pawn   *
 *           move or capture is kept as part of the position[] structure.  It  *
 *           is updated by MakeMove().  When this number reaches 100 (which    *
 *           in plies is 50 moves) Repeat() will return the draw indication    *
 *           immediately, just as though the position was a repetition draw.   *
 *                                                                             *
 *    3.6    Search extensions cleaned up to avoid excessive extensions which  *
 *           produced some wild variations, but which was also slowing things  *
 *           down excessively.                                                 *
 *                                                                             *
 *    3.7    Endgame strategy added.  Two specifics: KBN vs K has a piece/sq   *
 *           table that will drive the losing king to the correct corner.  For *
 *           positions with no pawns, scoring is altered to drive the losing   *
 *           king to the edge and corner for mating purposes.                  *
 *                                                                             *
 *    3.8    Hashing strategy modified.  Crafty now stores search value or     *
 *           bound, *and* positional evaluation in the transposition table.    *
 *           This avoids about 10-15% of the "long" evaluations during the     *
 *           middlegame, and avoids >75% of them in endgames.                  *
 *                                                                             *
 *    4.0    Evaluation units changed to "millipawns" where a pawn is now      *
 *           1000 rather than the prior 100 units.  This provides more         *
 *           "resolution" in the evaluation and will let Crafty have large     *
 *           positional bonuses for significant things, without having the     *
 *           small positional advantages add up and cause problems.  V3.8      *
 *           exhibited a propensity to "sac the exchange" frequently because   *
 *           of this.  It is hoped that this is a thing of the past now.       *
 *           Also, the "one legal reply to check" algorithm is now being used. *
 *           In short, if one side has only one legal reply to a checking move *
 *           then the other side is free to check again on the next ply. this  *
 *           only applies in the "extension" search, and allows some checks    *
 *           that extend() would normally not follow.                          *
 *                                                                             *
 *    4.1    Extension search modified.  It now "tracks" the basic iteration   *
 *           search depth up to some user-set limit (default=6).  For shallow  *
 *           depths, this speeds things up, while at deeper depths it lets the *
 *           program analyze forcing moves somewhat deeper.                    *
 *                                                                             *
 *    4.2    Extension search modified.  Fixed a problem where successive      *
 *           passed-pawn pushes were not extended correctly by extend() code.  *
 *           Threat-extension margin was wrong after changing the value of a   *
 *           pawn to 1000, it is now back to 1.5 pawns.                        *
 *                                                                             *
 *    4.3    Hash scoring "repaired."  As the piece/square tables changed, the *
 *           transposition table (positional evaluation component) along with  *
 *           the pawn and king-safety hash tables prevented the changes from   *
 *           affecting the score, since the hashed scores were based on the    *
 *           old piece/square table values.  Now, when any piece/square table  *
 *           is modified, the affected hash tables are cleared of evaluation   *
 *           information.                                                      *
 *                                                                             *
 *    4.4    Piece/square tables simplified, king tropism scoring moved to     *
 *           Evaluate() where it is computed dynamically now as it should be.  *
 *                                                                             *
 *    4.5    Book move selection algorithm replaced.  Crafty now counts the    *
 *           number of times each book move was played as the book file is     *
 *           created.  Since these moves come (typically) from GM games, the   *
 *           more frequently a move appears, the more likely it is to lead to  *
 *           a sound position.  Crafty then enumerates all possible book moves *
 *           for the current position, computes a probability distribution for *
 *           each move so that Crafty will select a move proportional to the   *
 *           number of times it was played in GM games (for example, if e4 was *
 *           played 55% of the time, then Crafty will play e4 55% of the time) *
 *           although the special case of moves played too infrequently is     *
 *           handled by letting the operator set a minimum threshold (say 5)   *
 *           Crafty won't play moves that are not popular (or are outright     *
 *           blunders as the case may be.)  Pushing a passed pawn to the 7th   *
 *           rank in the basic search [Search() module] now extends the search *
 *           by two plies unless we have extended this ply already (usually a  *
 *           check evasion) or unless we have extended the whole line to more  *
 *           than twice the nominal iteration depth.                           *
 *                                                                             *
 *    5.0    Selective search extensions removed.  With the extensions now in  *
 *           the basic full-width search, these became more a waste of time    *
 *           than anything useful, and removing them simplifies things quite   *
 *           a bit.                                                            *
 *                                                                             *
 *    5.1    Pondering logic now has a "puzzling" feature that is used when    *
 *           the search has no move to ponder.  It does a short search to find *
 *           a move and then ponders that move, permanently eliminating the    *
 *           "idle" time when it has nothing to ponder.                        *
 *                                                                             *
 *    5.2    Evaluation terms scaled down to prevent positional scores from    *
 *           reaching the point that sacrificing material to avoid simple      *
 *           positional difficulties begins to look attractive.                *
 *                                                                             *
 *    5.3    Performance improvement produced by avoiding calls to MakeMove()  *
 *           when the attack information is not needed.  Since the first test  *
 *           done in Quiesce() is to see if the material score is so bad that  *
 *           a normal evaluation and/or further search is futile, this test    *
 *           has been moved to inside the loop *before* Quiesce() is           *
 *           recursively called, avoiding the MakeMove() work only to          *
 *           discover that the resulting position won't be searched further.   *
 *                                                                             *
 *    5.4    New SEE() function that now understands indirect attacks through  *
 *           the primary attacking piece(s).  This corrects a lot of sloppy    *
 *           move ordering as well as make the "futility" cutoffs added in     *
 *           in version 5.3 much more reliable.                                *
 *                                                                             *
 *    5.5    Checks are now back in the quiescence search.  Crafty now stores  *
 *           a negative "draft" in the transposition table rather than forcing *
 *           any depth < 0 to be zero.  The null-move search now searches the  *
 *           null-move to depth-1 (R=1) rather than depth-2 (R=2) which seemed *
 *           to cause some tactical oversights in the search.                  *
 *                                                                             *
 *    5.6    Improved move ordering by using the old "killer move" idea.  An   *
 *           additional advantage is that the killers can be tried before      *
 *           generating any moves (after the captures.)  Quiescence now only   *
 *           includes "safe" checks (using SEE() to determine if it is a safe  *
 *           checking move.                                                    *
 *                                                                             *
 *    5.7    King safety now "hates" a pawn at b3/g3 (white) or b6/g6 (black)  *
 *           to try and avoid the resulting mate threats.                      *
 *                                                                             *
 *    5.8    EvaluateOutsidePassedPawns() fixed to correctly evaluate those    *
 *           positions where both sides have outside passed pawns.  Before     *
 *           this fix, it only evaluated positions where one side had a passed *
 *           pawn, which overlooked those won/lost positions where both sides  *
 *           have passed pawns but one is "distant" or "outside" the other.    *
 *                                                                             *
 *    5.9    Removed "futility" forward pruning.  Exhaustive testing has       *
 *           proved that this causes significant tactical oversights.          *
 *                                                                             *
 *    5.10   Added code to handle king and pawn vs king, which understands     *
 *           the cases where the pawn can't outrun the king, but has to be     *
 *           assisted along by the king.                                       *
 *                                                                             *
 *    5.11   Two king-safety changes.  (1) The program's king safety score is  *
 *           now "doubled" to make it much less likely that it will try to win *
 *           a pawn but destroy it's king-side.  (2) The attack threshold has  *
 *           been lowered which is used to key the program that it is now      *
 *           necessary to move pieces to defend the king-side, in an effort to *
 *           protect a king-side that is deemed somewhat unsafe.               *
 *                                                                             *
 *    5.12   Improved null-move code by computing the Knuth/Moore node type    *
 *           and then only trying null-moves at type 2 nodes.  This has        *
 *           resulted in a 2x speedup in test positions.                       *
 *                                                                             *
 *    5.13   Optimization to avoid doing a call to MakeMove() if it can be     *
 *           avoided by noting that the move is not good enough to bring the   *
 *           score up to an acceptable level.                                  *
 *                                                                             *
 *    5.14   Artificially isolated pawns recognized now, so that Crafty won't  *
 *           push a pawn so far it can't be defended.  Also, the bonus for a   *
 *           outside passed pawn was nearly doubled.                           *
 *                                                                             *
 *    5.15   Passed pawns supported by a king, or connected passed pawns now   *
 *           get a large bonus as they advance.  Minor fix to the ICC resume   *
 *           feature to try to avoid losing games on time.                     *
 *                                                                             *
 *    6.0    Converted to rotated bitboards to make attack generation *much*   *
 *           faster.  MakeMove() now can look up the attack bit vectors        *
 *           rather than computing them which is significantly faster.         *
 *                                                                             *
 *    6.1    Added a "hung piece" term to Evaluate() which penalizes the side  *
 *           to move if a piece is attacked by a lesser piece, or a piece is   *
 *           attacked and not defended at all.  Additionally, a new scoring    *
 *           term was added to detect a weak back rank (where the king does    *
 *           not attack an empty square on the 2nd rank, and there are no      *
 *           horizontally sliding pieces [rook or queen] on the first rank to  *
 *           guard against back-rank mates.                                    *
 *                                                                             *
 *    6.2    Modified the "futility" cutoff to (a) emulate the way the program *
 *           normally searches, but avoiding MakeMove() calls when possible,   *
 *           and (2) not searching a move near the horizon if the material     *
 *           score is hopeless.                                                *
 *                                                                             *
 *    6.3    Null-move code moved from NextMove() directly into Search().      *
 *           this results is a "cleaner" implementation, as well as fixing an  *
 *           error in the node type, caused by Search() thinking that after    *
 *           the first move tried fails to cause a cutoff, that suddenly this  *
 *           node is a type=3 node (Knuth and Moore).  This bug would then     *
 *           introduce null-moves into later nodes that are a waste of time.   *
 *                                                                             *
 *    6.4    Pawn scoring modified.  Passed pawns were scored low enough that  *
 *           Crafty would let the opponent create one in the endgame (as long  *
 *           as it wasn't an "outside" passed pawn).  This required adjusting  *
 *           isolated pawns as well.  All "weak" pawns (pawns on open files    *
 *           that aren't defended by a pawn) are not penalized so much if      *
 *           there aren't any rooks/queens to attack on the file.              *
 *                                                                             *
 *    7.0    Removed the to.attack and from.attack bitboards.  These are now   *
 *           computed as needed, using the rotated bitboards.  Hung piece      *
 *           stuff removed due to lack of any verified benefit.                *
 *                                                                             *
 *    7.1    Modified next.c and nextc.c so that they check "mvv_lva_ordering" *
 *           to determine which capture ordering strategy to use.  Note that   *
 *           setting this to "1" (default at present) disables the forward     *
 *           pruning in quiescence search that wants to cull losing captures.  *
 *                                                                             *
 *    7.2    Major clean-up of MakeMove() using macros to make it easier to    *
 *           read and understand.  Ditto for other modules as well.  Fixed a   *
 *           bug in Evaluate() where bishop scoring failed in endgame          *
 *           situations, and discouraged king centralization.                  *
 *                                                                             *
 *    7.3    Evaluate() is no longer called if material is too far outside the *
 *           current alpha/beta window.  The time-consuming part of Evaluate() *
 *           is no longer done if the major scoring contributors in Evaluate() *
 *           haven't pulled the score within the alpha/beta window, and the    *
 *           remainder of Evaluate() can't possible accomplish this either.    *
 *           Gross error in EvaluatePawns() fixed, which would "forget" all    *
 *           of the pawn scoring done up to the point where doubled white      *
 *           pawns were found.  Error was xx=+ rather than xx+=, which had     *
 *           an extremely harmful effect on pawn structure evaluation.         *
 *                                                                             *
 *    7.4    Performance improvements produced by elimination of bit-fields.   *
 *           This was accomplished by hand-coding the necessary ANDs, ORs, and *
 *           SHIFTs necessary to accomplish the same thing, only faster.       *
 *                                                                             *
 *    7.5    Repetition code modified.  It could store at replist[-1] which    *
 *           clobbered the long long word just in front of this array.         *
 *                                                                             *
 *    7.6    Null move search now searches with R=2, unless within 3 plies     *
 *           of the quiescence search, then it searches nulls with R=1.        *
 *                                                                             *
 *    8.0    Re-vamp of evaluation, bringing scores back down so that Crafty   *
 *           will stop sacrificing the exchange, or a piece for two pawns just *
 *           it thinks it has lots of positional compensation.  The next few   *
 *           versions are going to be tuning or coding modifications to eval.  *
 *                                                                             *
 *    8.1    Futility() re-written to be more efficient, as well as to stop    *
 *           tossing moves out based on the +/- 2 pawn window, which was too   *
 *           speculative.  It still saves roughly 20% in speed over the same   *
 *           searches without futility, but seems to have *no* harmful effects *
 *           in tests run to date.                                             *
 *                                                                             *
 *    8.2    Futility() removed once and for all.  Since MakeMove() is so      *
 *           fast after the new attack generation algorithm, this was actually *
 *           slower than not using it.                                         *
 *                                                                             *
 *    8.3    EvaluatePawns() weak pawn analysis bug fixed.  Other minor        *
 *           performance enhancements and cleanup.                             *
 *                                                                             *
 *    8.4    Dynamic "king tropism" evaluation term now used, which varies     *
 *           based on how exposed the target king is.  The more exposed, the   *
 *           larger the bonus for placing pieces near the king.  New "analyze" *
 *           feature that complements the "annotate" feature that Crafty has   *
 *           had for a while.  Annotate is used to play over moves that are    *
 *           either in the current game history, or have been read in using    *
 *           the "read <filename>" command.  Analyze, on the other hand, will  *
 *           immediately begin searching the current position.  Each time the  *
 *           operator enters a move, it will make that move on the board, and  *
 *           then search the resulting position for the other side.  In effect *
 *           this gives a running commentary on a "game in progress".  These   *
 *           moves can be pumped into Crafty in many ways so that "on the fly" *
 *           analysis of a game-in-progress is possible.                       *
 *                                                                             *
 *    8.5    More cleanup.  Pawn promotions were searched twice, thanks to the *
 *           killer moves, although often they resulted in no search overhead  *
 *           due to transposition table hits the second time around.  Other    *
 *           minor fixes, including one serious "undefined variable" in        *
 *           Evaluate() that caused grief in endings.                          *
 *                                                                             *
 *    8.6    New book file structure.  It is no longer necessary to enter the  *
 *           "number of records" as Crafty now uses a new compressed hashing   *
 *           technique so that the book has no empty space in it, and the size *
 *           is computed "on the fly" producing a smaller book without the     *
 *           user having to compute the size.  Tweaks to Evaluate() to cure    *
 *           minor irritating habits.                                          *
 *                                                                             *
 *    8.7    Repaired optimization made in null-move search, that forgot to    *
 *           clear "EnPassant_Target".  A double pawn push, followed by a null *
 *           left the side on move "very confused" and would let it play an    *
 *           illegal enpassant capture in the tree. Sort.<n> files are now     *
 *           removed after book.bin is created.  Also, minor change to book    *
 *           format makes it incompatible with older book versions, but also   *
 *           allows book lines to be as long as desired, and the book size to  *
 *           reach any reasonable size.  Dynamic king tropism replaced by a    *
 *           dynamic computation, but with a static king tropism score, rather *
 *           than a score based on king exposure.  There was simply too much   *
 *           interaction, although this may prove workable later.              *
 *                                                                             *
 *    8.8    Tweaks to passed pawn scoring.  Scores were too low, making       *
 *           Crafty "ignore" them too much.                                    *
 *                                                                             *
 *    8.9    Internal iterative deepening is now used in those rare positions  *
 *           where a PV node has no hash move to search.  This does a shallow  *
 *           search to depth-2 to find a good move for ordering.               *
 *                                                                             *
 *    8.10   Internal interative deepening modified to handle cases where lots *
 *           of tactics make the deepening search fail high or low, and        *
 *           occasionally end up with no move to try.  This produced a "bad    *
 *           move from hash table" error.                                      *
 *                                                                             *
 *    8.11   Minor bug in internal iterative deepening fixed.  Also, macros    *
 *           for accessing the "position" data structure are now used to make  *
 *           things a little more readable.                                    *
 *                                                                             *
 *    8.12   Fixed minor bugs in quiescence checks, which let Crafty include   *
 *           more checks than intended (default quiescence_checks=2, but the   *
 *           comparison was <= before including a check, which made it include *
 *           three.  Additionally, a hashed check was *always* tried, even if  *
 *           quiescence_checks had already been satisfied.  Pawn scoring also  *
 *           had a bug, in that there was a "crack" between opening and middle *
 *           game, where Crafty would conclude that it was in an endgame, and  *
 *           adjust the pawn advance scores accordingly, sometimes causing an  *
 *           odd a4/a5/etc move "out of the blue."  Minor bug in next_capture  *
 *           was pruning even exchanges very early in quiescence search.  That *
 *           has now been removed, so only losing exchanges are pruned.        *
 *                                                                             *
 *    8.13   NextCapture() now *always* tries checking moves if the move at    *
 *           the previous ply was a null-move.  This avoids letting the null   *
 *           move hide some serious mating threats.  A minor bug where Crafty  *
 *           could draw by repetition after announcing a mate.  This was a     *
 *           result of finding a mate score in hash table, and, after the      *
 *           search iteration completed, the mate score would terminate the    *
 *           search completely.  Now, the search won't terminate until it      *
 *           finds a mate shorter than the previous search did.  Minor eval    *
 *           tweaks and bugfixes as well.                                      *
 *                                                                             *
 *    8.14   Checks after null moves discontinued.  Worked in some cases, but, *
 *           in general made the tree larger for nothing.  Eval tweaks to stop *
 *           sacs that resulted in connected passed pawns, often at the        *
 *           expense of a minor piece for a pawn or two, which usually lost.   *
 *           Mobility coeffecient increased for all pieces.  Asymmetric king   *
 *           safety discontinued.  King safety for both sides is now equally   *
 *           important.  King safety is now also proportional to the number of *
 *           pieces the other side has, so that a disrupted king-side will     *
 *           encourage trading pieces to reduce attacking chances.             *
 *                                                                             *
 *    8.15   Weak pawn scoring modified further.  The changes are designed to  *
 *           cause Crafty to keep pawns "mobile" where they can advance,       *
 *           rather than letting them become blocked or locked.                *
 *                                                                             *
 *    8.16   Still more weak pawn modifications.  In addition, there is no     *
 *           "rook on half-open file" any longer.  If there are only enemy     *
 *           pawns on the file, and the most advanced one is weak, then the    *
 *           file is treated as though it were open.  Technical error in the   *
 *           EvaluateDevelopment() code that caused screwey evaluations is     *
 *           fixed, making Crafty more likely to castle.  :)  Book() now       *
 *           verifies that the current position is in book, before trying any  *
 *           moves to see if the resulting positions are in book.  This avoids *
 *           something like e4 e5 Bb5 a6 Nf3 from causing Crafty to play Nc6   *
 *           which takes it back into book, rather than axb5 winning a piece.  *
 *           This will occasionally backfire and prevent Crafty from trans-    *
 *           posing back into book, but seems safer at present.                *
 *                                                                             *
 *    8.17   Piece values changed somewhat, to avoid Crafty's propensity to    *
 *           make unfavorable trades.  An example:  Crafty is faced with the   *
 *           loss of a pawn.  Instead, it trades the queen for a rook and      *
 *           bishop, which used to be the same as losing a pawn.  This is not  *
 *           so good, and often would result in a slow loss.  This version     *
 *           also implements several "user-supplied" patches to make Crafty    *
 *           compile cleanly on various machines.  In addition, you no longer  *
 *           have to continually modify types.h for different configurations.  *
 *           the Makefile now supplies a -D<target> to the compiler.  You need *
 *           to edit Makefile and set "target" to the appropriate value from   *
 *           the list provided.  Then, when getting a new version, save your   *
 *           Makefile, extract the new source, copy in your makefile and you   *
 *           will be ready, except for those rare occasions where I add a new  *
 *           source module.  Other changes are performance tweaks.  One is a   *
 *           simple trick to order captures while avoiding SEE() if possible.  *
 *           If the captured piece is more valuable than the capturing piece,  *
 *           we can simply use the difference (pessimistic value) rather than  *
 *           calling SEE() since this pessimistic value is still > 0.  Other   *
 *           tweaks to the various Next_*() routines to avoid a little un-     *
 *           necessary work.                                                   *
 *                                                                             *
 *    8.18   Book move selection algorithm changed.  Note that this is highly  *
 *           speculative, but when best book play is selected, Crafty will     *
 *           use the books.bin file as always.  If there is no move in books,  *
 *           Crafty reverts to book.bin, but now finds all known book moves    *
 *           and puts them into the root move list.  It then does a fairly     *
 *           short search, only considering these moves, and it will play the  *
 *           best one so long as the evaluation is acceptable.  If not, it     *
 *           simply returns as though there was no book move, and executes a   *
 *           search as it normally would.  The book hashing algorithm was also *
 *           modified so that the 64-bit hash key is slightly different from   *
 *           the real hash key.  In effect, the upper 16 bits are only set as  *
 *           a result of the side-not-to-move's pieces.  This means that for a *
 *           given position, all the book moves will be in the same "cluster"  *
 *           which makes the book dramatically faster, since one seek and read *
 *           produces all the possible book moves (plus some others too, since *
 *           the upper 16 bits are not unique enough.)  A significant speed    *
 *           improvement is noticable, particularly with a 60mb opening book.  *
 *           Crafty now understands (if that's the right word) that endings    *
 *           with bishops of opposite colors are to be avoided.  When such an  *
 *           ending is encountered, the total score is simply divided by two   *
 *           to make the ending look more drawish.                             *
 *                                                                             *
 *    8.19   Book selection algorithm further modified, so that the result of  *
 *           each game in the GM database is known.  Now Crafty will not play  *
 *           a move from book, if all games were lost by the side on move,     *
 *           hopefully avoiding many blunders.  Crafty now understands how to  *
 *           attack if both players castle on opposite sides, by encouraging   *
 *           pawn advances against the kings.  Other minor modifications to    *
 *           the eval values.                                                  *
 *                                                                             *
 *    8.20   PVS search finally implemented fully.  Problems several months    *
 *           ago prevented doing the PVS search along the PV itself.  This is  *
 *           therefore quite a bit faster, now that it's fully implemented and *
 *           working properly.  Elapsed time code modified so that Crafty now  *
 *           uses gettimeofday() to get fractions of a second elapsed time.    *
 *           Slight modification time allocation algorithm to avoid using too  *
 *           much time early in the game.                                      *
 *                                                                             *
 *    8.21   Courtesy of Mark Bromley, Crafty may run significantly faster on  *
 *           your machine.  There are some options in the Makefile that will   *
 *           eliminate many of the large attack computation long longs, which  *
 *           helps prevent cache thrashing.  On some machines this will be     *
 *           slower, on others 20% (or maybe more) faster.  You'll have to     *
 *           try -DCOMPACT_ATTACKS, and if that's faster, then it's time to    *
 *           try -DUSE_SPLIT_SHIFTS which may help even more.  Finally, if you *
 *           are running on a supersparc processor, you can use the fastattack *
 *           assembly module fastattack.s and get another big boost.  (Attack  *
 *           function is largest compute user in Crafty at present.) Serious   *
 *           search problem fixed.  It turns out that Crafty uses the hash     *
 *           table to pass PV moves from one iteration to the next, but for    *
 *           moves at the root of the tree the hash table has no effect, so a  *
 *           special case was added to RootMoveList to check the list of moves *
 *           and if one matches the PV move, move it to the front of the list. *
 *           This turned out to be critical because after completing a search, *
 *           (say to 9 plies) Crafty makes its move, then chops the first two  *
 *           moves off of the PV and then passes that to iterate to start a    *
 *           search.  This search starts at lastdepth-1 (8 in this case) since *
 *           the n-2 search was already done.  The "bug" showed up however, in *
 *           that RootMoveList() was not checking for the PV move correctly,   *
 *           which meant the root moves were ordered by static eval and static *
 *           exchange evaluation.  Normally not a serious problem, just that   *
 *           move ordering is not so good.  However, suppose that the opponent *
 *           takes a real long time to make a move (say 30 seconds) so that    *
 *           Crafty completes a 10 ply search.  It then starts the next search *
 *           at depth=9, but with the wrong move first.  If the target time is *
 *           not large enough to let it resolve this wrong move and get to the *
 *           other moves on the list, Crafty is "confused" into playing this   *
 *           move knowing absolutely nothing about it.  The result is that it  *
 *           can play a blunder easily.  The circumstances leading to this are *
 *           not common, but in a 60 move game, once is enough.  PV was pretty *
 *           well mis-handled in carrying moves from one search to another     *
 *           (not from one iteration to another, but from one complete search  *
 *           to another) and has been fixed.  A cute glitch concerning storing *
 *           PV from one iteration to another was also fixed, where the score  *
 *           stored was confusing the following search.                        *
 *                                                                             *
 *    8.22   EPD support (courtesy of Steven Edwards [thanks]) is now standard *
 *           in Crafty.  For porting, I'll provide a small test run which can  *
 *           be used to validate Crafty once it's been compiled.               *
 *                                                                             *
 *    8.23   Cleanup/speedup in hashing.  ProbeTransRef() and StoreTransRef()  *
 *           now carefully cast the boolean operations to the most efficient   *
 *           size to avoid 64bit operations when only the right 32 bits are    *
 *           significant. Repeat() code completely re-written to maintain two  *
 *           repetition lists, one for each side.  Quiesce() now handles the   *
 *           repetition check a little different, being careful to not call it *
 *           when it's unimportant, but calling it when repetitions are        *
 *           possible.                                                         *
 *                                                                             *
 *    8.24   Tweaks for king tropism to encourage pieces to collect near the   *
 *           king, or to encourage driving them away when being attacked.  A   *
 *           modification to Evaluate() to address the problem where Crafty    *
 *           is forced to play Kf1 or Kf8, blocking the rook in and getting    *
 *           into tactical difficulties as a result.  Book problem fixed where *
 *           Bookup was attempting to group moves with a common ancestor       *
 *           position together.  Unfortunately, captures were separated from   *
 *           this group, meaning capture moves in the book could never be      *
 *           played.  If a capture was the only move in book, it sort of       *
 *           worked because Crafty would drop out of book (not finding the     *
 *           capture) and then the search would "save" it.  However, if there  *
 *           was a non-capture alternative, it would be forced to play it,     *
 *           making it play gambits, and, often, bad ones.  Tablebase support  *
 *           is now in.  The tablebase files can be downloaded from the ftp    *
 *           machine at chess.onenet.net, pub/chess/TB.  Currently, Steven     *
 *           Edwards has all interesting 4 piece endings done.  To make this   *
 *           work, compile with -DTABLEBASES, and create a TB directory where  *
 *           Crafty is run, and locate the tablebase files in that directory.  *
 *           If you are running under UNIX, TB can be a symbolic or hard link  *
 *           to a directory anywhere you want.                                 *
 *                                                                             *
 *    8.25   Minor repair on draw by repetition.  Modified how books.bin was   *
 *           being used.  Now, a move in books.bin has its flags merged with   *
 *           the corresponding move from book.bin, but if the move does not    *
 *           exist in book.bin, it is still kept as a book move.  Repetitions  *
 *           are now counted as a draw if they occur two times, period.  The   *
 *           last approach was causing problems due to hashing, since the hash *
 *           approach used in Crafty is fairly efficient and frequently will   *
 *           carry scores across several searches.  This resulted in Crafty    *
 *           stumbling into a draw by repetition without knowing.  The con-    *
 *           figuration switch HAS-64BITS has been cleaned up and should be    *
 *           set for any 64bit architecture now, not just for Cray machines.   *
 *                                                                             *
 *    8.26   New search extension added, the well-known "one legal response to *
 *           check" idea.  If there's only one legal move when in check, we    *
 *           extend two plies rather than one, since this is a very forcing    *
 *           move.  Also, when one side has less than a rook, and the other    *
 *           has passed pawns, pushing these pawns to the 6th or 7th rank      *
 *           will extend the search one ply, where in normal positions, we     *
 *           only extend when a pawn reaches the 7th rank.                     *
 *                                                                             *
 *    9.0    Minor constraint added to most extensions:  we no longer extend   *
 *           if the side on move is either significantly ahead or behind,      *
 *           depending on the extension.  For example, getting out of check    *
 *           won't extend if the side on move is a rook behind, since it's     *
 *           already lost anyway.  We don't extend on passed pawn pushes if    *
 *           the side on move is ahead a rook, since he's already winning.     *
 *           minor adjustments for efficiency as well.  The next few versions  *
 *           in this series will have module names in these comments           *
 *           indicating which modules have been "cleaned" up.  This is an      *
 *           effort at optimizing, although it is currently directed at a line *
 *           by line analysis within modules, rather than major changes that   *
 *           effect more global ideas.  This type of optimization will come at *
 *           a later point in time.                                            *
 *                                                                             *
 *    9.1    NextMove(), NextCapture() and NextEvasion() were re-structured    *
 *           to eliminate unnecessary testing and speed them up significantly. *
 *           EvaluateTrades() was completely removed, since Crafty already has *
 *           positional scores that encourage/discourage trading based on the  *
 *           status of the game.  EvaluateTempo() was evaluated to be a        *
 *           failure and was removed completely.                               *
 *                                                                             *
 *    9.2    Quiesce()) and Search() were modified to be more efficient.  In   *
 *           addition, the null-move search was relaxed so that it always does *
 *           a search to depth-R, even if close to the end of the tree.        *
 *                                                                             *
 *    9.3    Check() is no longer used to make sure a position is legal after  *
 *           MakeMove() called, but before Search() is called recursively.  We *
 *           now use the same approach as Cray Blitz, we make a move and then  *
 *           capture the king at the next ply to prove the position is not a   *
 *           valid move.  If the side-on-move is already in check, we use      *
 *           NextEvasion() which only generates legal moves anyway, so this    *
 *           basically eliminates 1/2 of the calls to Check(), a big win.      *
 *           This resulted in modifications to Search(), Quiesce(), and        *
 *           QuiesceFull().  Connected passed pawns on 6th-7th no longer       *
 *           trigger search extensions.  Solved some problems like Win at      *
 *           Chess #2, but overall was a loser.                                *
 *                                                                             *
 *    9.4    Lookup now checks the transposition table entry and then informs  *
 *           Search() when it appears that a null move is useless.  This is    *
 *           found when the draft is too low to use the position, but it is at *
 *           least as deep as a null-move search would go, and the position    *
 *           did not cause a fail-high when it was stored.  King-safety was    *
 *           modified again.  The issue here is which king should attract the  *
 *           pieces, and for several months the answer has been the king that  *
 *           is most exposed attracts *all* pieces.  This has been changed to  *
 *           always attract one side's pieces to the other king.  Crafty's     *
 *           asymmetric king-safety was making it overly-defensive, even when  *
 *           the opponent's king was more exposed.  This should cure that and  *
 *           result in more aggressive play.                                   *
 *                                                                             *
 *    9.5    "Vestigial code" (left over from who-knows-when) removed from     *
 *           Evaluate().  This code used an undefined variable when there was  *
 *           no bishop or knight left on the board, and could add in a penalty *
 *           on random occasions.  Quiescence checks removed completely to see *
 *           how this works, since checks extend the normal search significant *
 *           amounts in any case.  QuiesceFull() removed and merged into       *
 *           Quiesce() which is faster and smaller.  First impression: faster, *
 *           simpler, better.  The benefit of no quiescence checks is that the *
 *           exhaustive search goes deeper to find positional gains, rather    *
 *           than following checks excessively.  No noticable loss in tactical *
 *           strength (so far).                                                *
 *                                                                             *
 *    9.6    legal move test re-inserted in Search() since null-moves could    *
 *           make it overlook the fact that a move was illegal.  New assembly  *
 *           code for X86 improves performance about 1/3, very similarly to    *
 *           the results obtained with the sparc-20 assembly code.  This was   *
 *           contributed by Eugene Nalimov and is a welcome addition.          *
 *                                                                             *
 *    9.7    Optimizations continuing.  Minor bug in pawn evaluation repaired. *
 *           Crafty looked at most advanced white pawn, but least-advanced     *
 *           black pawn on a file when doing its weak pawn evaluation.  The    *
 *           history heuristic was slightly broken, in that it was supposed to *
 *           be incrementing the history count by depth*depth, but this was    *
 *           somehow changed to 7*depth, which was not as good.  Assembly      *
 *           language interface fixed to cleanly make Crafty on all target     *
 *           architectures.                                                    *
 *                                                                             *
 *    9.8    The first change was to borrow a time-allocation idea from Cray   *
 *           Blitz.  Essentially, when Crafty notices it has used the normal   *
 *           time allotment, rather than exiting the search immediately, it    *
 *           will wait until it selects the next move at the root of the tree. *
 *           The intent of this is that if it has started searching a move     *
 *           that is going to be better than the best found so far, this will  *
 *           take more time, while a worse move will fail low quickly.  Crafty *
 *           simply spends more time to give this move a chance to fail high   *
 *           or else quickly fail low.  A fairly serious bug, that's been      *
 *           around for a long time, has finally been found/fixed.  In simple  *
 *           terms, if the "noise" level was set too high, it could make       *
 *           Crafty actually play a bad move.  The more likely effect was to   *
 *           see "bad move hashed" messages and the first few lines of output  *
 *           from the search often looked funny.  The bug was "noise" was used *
 *           as a limit, until that many nodes had been searched, no output    *
 *           would be produced.  Unfortunately, on a fail-high condition, the  *
 *           same code that produced the "++  Nh5" message also placed the     *
 *           fail-high move in the PV.  Failing to do this meant that the      *
 *           search could fail high, but not play the move it found.  Most     *
 *           often this happened in very fast games, or near the end of long   *
 *           zero-increment games.  It now always saves this move as it should *
 *           regardless of whether it prints the message or not.               *
 *                                                                             *
 *    9.9    Interface to Xboard changed from -ics to -xboard, so that -ics    *
 *           can be used with the custom interface.  Additional code to        *
 *           communicate with custom interface added.  Search() extensions are *
 *           restricted so that there can not be more than one extension at a  *
 *           node, rather than the two or (on occasion) more of the last       *
 *           version.                                                          *
 *                                                                             *
 *    9.10   Converted to Ken Thompson's "Belle" hash table algorithm.  Simply *
 *           put, each side has two tables, one that has entries replaced on a *
 *           depth-priority only, the other is an "always replace" table, but  *
 *           is twice as big.  This allows "deep" entries to be kept along     *
 *           with entries close to search point.  Basically, when the depth-   *
 *           priority table gets a position stored in it, the displaced        *
 *           position is moved to the always-replace table.  If the position   *
 *           can not replace the depth-priority entry, it always overwrites    *
 *           the other always-replace entry.  However, a position is never put *
 *           in both tables at the same time.                                  *
 *                                                                             *
 *    9.11   Bug in Search() that could result in the "draft" being recorded   *
 *           incorrectly in the hash table.  Caused by passing depth, which    *
 *           could be one or two plies more than it should be due to the last  *
 *           move extending the search.  Repeat() completely removed from      *
 *           Quiesce() since only capture moves are included, and it's         *
 *           impossible to repeat a position once a capture has been made.     *
 *                                                                             *
 *    9.12   optimizations: history.c, other minor modifications.              *
 *                                                                             *
 *    9.13   EvaluatePawns() modified significantly to simplify and be more    *
 *           accurate as well.  Pawns on open files are now not directly       *
 *           penalized if they are weak, rather the penalty is added when the  *
 *           rooks are evaluated.  Pawn hashing modified to add more info to   *
 *           the data that is hashed.  King safety scores toned down almost    *
 *           50% although it is still asymmetric.                              *
 *                                                                             *
 *    9.14   EvaluatePawns() modified further so that it now does the same     *
 *           computations for king safety as the old EvaluateKingSafety*()     *
 *           modules did, only it produces four values, two for each king,     *
 *           for each side of the board (king or queen-side).  This is now     *
 *           hashed with the rest of the pawn scoring, resulting in less       *
 *           hashing and computation, and more hits for king-safety too.  King *
 *           safety modified further.  Asymmetric scoring was accidentally     *
 *           disabled several versions ago, this is now enabled again.         *
 *                                                                             *
 *    9.15   Evaluate() now attempts to recognize the case where a knight is   *
 *           trapped on one of the four corner squares, much like a bishop     *
 *           trapped at a2/a7/h2/h7.  If a knight is on a corner square, and   *
 *           neither of the two potential flight squares are safe (checked by  *
 *           SEE()) then a large penalty is given.  This was done to avoid     *
 *           positions where Crafty would give up a piece to fork the king and *
 *           rook at (say) c7, and pick up the rook at a8 and think it was an  *
 *           exchange ahead, when often the knight was trapped and it was      *
 *           two pieces for a rook and pawn (or worse.)  Two timing bugs fixed *
 *           in this version:  (1) nodes_per_second was getting clobbered at   *
 *           the end of iterate, which would then corrupt the value stored in  *
 *           nodes_between_time_checks and occasionally make Crafty not check  *
 *           the time very often and use more time than intended; (2) the      *
 *           "don't stop searching after time is out, until the current move   *
 *           at the root of the tree has been searched" also would let Crafty  *
 *           use time unnecessarily.                                           *
 *                                                                             *
 *    9.16   Significant changes to the way MakeMove() operates.  Rather than  *
 *           copying the large position[ply] structure around, the piece       *
 *           location bitmaps and the array of which piece is on which square  *
 *           are now simple global variables.  This means that there is now an *
 *           UnmakeMove() function that must be used to restore these after a  *
 *           a move has been made.  The benefit is that we avoid copying 10    *
 *           bitmaps for piece locations when typically only one is changed,   *
 *           Ditto for the which piece is on which square array which is 64    *
 *           bytes long.  The result is much less memory traffic, with the     *
 *           probability that the bitmaps now stay in cache all the time. The  *
 *           EvaluatePawns() code now has an exponential-type penalty for      *
 *           isolated pawns, but it penalizes the difference between white and *
 *           black isolated pawns in this manner.  King evaluation now         *
 *           evaluates cases where the king moves toward one of the rooks and  *
 *           traps it in either corner.                                        *
 *                                                                             *
 *    9.17   Xboard compatibility version!  Now supports, so far as I know,    *
 *           the complete xboard interface, including hint, show thinking,     *
 *           taking back moves, "move now" and so forth.  Additionally, Crafty *
 *           now works with standard xboard.  Notice that this means that a    *
 *           ^C (interrupt) will no long terminate Crafty, because xboard uses *
 *           this to implement the "move now" facility.  This should also      *
 *           work with winboard and allow pondering, since there is now a      *
 *           special windows version of CheckInput() that knows how to check   *
 *           Win95 or WinNT pipes when talking with winboard.                  *
 *                                                                             *
 *    9.18   Xboard compatibility update.  "force" (gnu) mode was broken, so   *
 *           that reading in a PGN file (via xboard) would break.  Now, Crafty *
 *           can track the game perfectly as xboard reads the moves in.  King  *
 *           safety modified to discourage g3/g6 type moves without the bishop *
 *           to defend the weak squares.  Significant other changes to the     *
 *           Evaluate() procedure and its derivatives.  Very nasty trans/ref   *
 *           bug fixed.  Been there since transposition tables added.  When    *
 *           storing a mate score, it has to be adjusted since it's backed up  *
 *           as "mate in n plies from root" but when storing, it must be saved *
 *           as "mate in n plies from current position".  Trivial, really, but *
 *           I overlooked one important fact:  mate scores can also be upper   *
 *           or lower search bounds, and I was correcting them in the same way *
 *           which is an error.  The effect was that Crafty would often find a *
 *           mate in 2, fail high on a move that should not fail high, and end *
 *           up making a move that would mate in 3.  In particularly bad cases *
 *           this would make the search oscillate back and forth and draw a    *
 *           won position.  The fix was to only adjust the score if it's a     *
 *           score, not if it's a bound.  The "age" field of the trans/ref     *
 *           table was expanded to 3 bits.  Iterate now increments a counter   *
 *           modulo 8, and this counter is stored in the 3 ID bits of the      *
 *           trans/ref table.  If they are == the transposition_id counter,    *
 *           we know that this is a position stored during the current search. *
 *           If not, it's an "old" position.  This avoids the necessity of     *
 *           stepping through each entry in the trans/ref table (at the begin- *
 *           ning of a search) to set the age bit.  Much faster in blitz games *
 *           as stepping through the trans/ref table, entry by entry, would    *
 *           blow out cache.  This idea was borrowed from Cray Blitz, which    *
 *           used this same technique for many years.                          *
 *                                                                             *
 *    9.19   New evaluation code for passed pawns.  Crafty now gives a large   *
 *           bonus for two or more connected passed pawns, once they reach the *
 *           sixth rank, when the opponent has less than a queen remaining.    *
 *           this will probably be tuned as experience produces the special    *
 *           cases that "break" it.  Such things as the king too far away or   *
 *           the amount of enemy material left might be used to further im-    *
 *           prove the evaluation.                                             *
 *                                                                             *
 *    9.20   Bug in SetBoard() fixed.  Validity checks for castling status and *
 *           en passant status was broken for castle status.  It was checking  *
 *           the wrong rook to match castling status, and would therefore not  *
 *           accept some valid positions, including #8 in Win At Chess suite.  *
 *           Minor repetition bug fixed, where Crafty would recognize that a   *
 *           three-fold repetition had occurred, but would not claim it when   *
 *           playing on a chess server.  Crafty now does not immediately re-   *
 *           search the first move when it fails low.  Rather, it behaves like *
 *           Cray Blitz, and searches all moves.  If all fail low, Crafty will *
 *           then relax (lower) alpha and search the complete list again.      *
 *           This is a simple gamble that there is another move that will      *
 *           "save the day" so that we avoid the overhead of finding out just  *
 *           how bad the first move is, only to have more overhead when a good *
 *           move fails high.  Minor repetition bug fixed.  When running test  *
 *           suites of problems, SetBoard() neglected to put the starting      *
 *           position in the repetition list, which would occasionally waste   *
 *           time.  One notable case was Win At Chess #8, where Crafty would   *
 *           find Nf7+ Kg8 Nh6+ Kh8 (repeating the original position) followed *
 *           by Rf7 (the correct move, but 4 tempi behind.)  Display() bug     *
 *           fixed.  Crafty now displays the current board position, rather    *
 *           than the position that has been modified by a search in progress. *
 *           Castling evaluation modified including a bug in the black side    *
 *           that would tend to make Crafty not castle queen-side as black if  *
 *           the king-side was shredded.  Minor bug in Book(). If the book was *
 *           turned off (which is done automatically when using the test com-  *
 *           mand since several of the Win At Chess positions are from games   *
 *           in Crafty's large opening book) Book() would still try to read    *
 *           the file and then use the data from the uninitialized buffer.     *
 *           This would, on occasion, cause Crafty to crash in the middle of   *
 *           a test suite run.                                                 *
 *                                                                             *
 *    9.21   Castling evaluation has been significantly modified to handle the *
 *           following three cases better:                                     *
 *           (a) One side can castle at the root of the tree, but can not      *
 *           castle at a tip position.  If the move that gave up the right to  *
 *           castle was *not* a castle move, penalize the score.               *
 *           (b) One side can castle short or long at the root, but has al-    *
 *           ready castled by the time the search reaches a tip position.      *
 *           Here the goal is to make sure the player castled to the "better"  *
 *           side by comparing king safety where the king really is, to king   *
 *           safety had the king castled to the other side.  If the latter is  *
 *           "safer" then penalize the current position by the "difference"    *
 *           in king safety to encourage that side to delay castling.  This is *
 *           a problem when Crafty can castle kingside *now*, but the kingside *
 *           has been disrupted somehow.  If the search is not deep enough to  *
 *           see clearing out the queenside and castling long, then it will    *
 *           often castle short rather than not castle at all.  This fix will  *
 *           make it delay, develop the queenside, and castle to a safer king  *
 *           shelter, unless tactics force it to castle short.                 *
 *           (c) One side can castle short or long at the root, but can only   *
 *           castle one way at a tip position, because a rook has been moved.  *
 *           If the remaining castle option is worse than castling to the side *
 *           where the rook was moved, then we penalize the rook move to keep  *
 *           castling to that side open as an option.                          *
 *                                                                             *
 *    9.22   More castling evaluation changes, to tune how/when Crafty chooses *
 *           to castle.  Bugs in Option() where certain commands could be ex-  *
 *           ecuted while a search was in progress.  If these commands did     *
 *           anything to the board position, It would break Crafty badly.      *
 *           All that was needed was to check for an active search before some *
 *           commands were attempted, and if a search was in progress, return  *
 *           to abort the search then re-try the command.  When playing games  *
 *           with computers (primarily a chess-server consideration) Crafty is *
 *           now defaulting to "book random 0" to use a short search to help   *
 *           avoid some ugly book lines on occasion.  In this version, *all*   *
 *           positional scores were divided by 2 to reduce the liklihood that  *
 *           Crafty would sacrifice material and make up the loss with some    *
 *           sort of positional gain that was often only temporary.  Note that *
 *           king safety scores were not reduced while doing this.  New book   *
 *           random options:  0=search, 1=most popular move, 2=move that       *
 *           produces best positional value, 3=choose from most frequently     *
 *           played moves, 4=emulate GM frequency of move selection, 5=use     *
 *           sqrt() on frequencies to give more randomness, 6=random.  For     *
 *           now, computers get book_random=1, while GM's now get 4 (same as   *
 *           before), everyone else gets 5.  Until now, there has been no      *
 *           penalty for doubled/tripled pawns.  It now exists.  Also, if the  *
 *           king stands on E1/E8, then the king-side king safety term is used *
 *           to discourage disrupting the king-side pawns before castling.     *
 *           Crafty now has a start-up initialization file (as most Unix       *
 *           programs do).  This file is named ".craftyrc" if the macro UNIX   *
 *           is defined, otherwise a "dossy" filename "crafty.rc" is used.     *
 *           Crafty opens this file and executes every command in it as though *
 *           they were typed by the operator.  The last command *must* be      *
 *           "exit" to revert input back to STDIN.                             *
 *                                                                             *
 *    9.23   More evaluation tuning, including the complete removal of the     *
 *           mobility term for rooks/queens.  Since rooks already have lots    *
 *           of scoring terms like open files, connected, etc, mobility is     *
 *           redundant at times and often misleading.  The queen's mobility    *
 *           is so variable it is nearly like introducing a few random points  *
 *           at various places in the search.  Minor Xboard compatibility bug  *
 *           fixed where you could not load a game file, then have Crafty play *
 *           by clicking the "machine white" or "machine black" options.       *
 *                                                                             *
 *    9.24   Minor bug in Quiesce() repaired.  When several mate-in-1 moves    *
 *           were available at the root, Crafty always took the last one,      *
 *           which was often an under-promotion to a rook rather than to a     *
 *           queen.  It looked silly and has been fixed so that it will play   *
 *           the first mate-in-1, not the last.  RootMoveList() went to great  *
 *           pains to make sure that promotion to queen occurs before rook.    *
 *           Bug in EvaluateOutsidePassedPawns() would fail to recognize that  *
 *           one side had an outside passer, unless both sides had at least    *
 *           one passer, which failed on most cases seen in real games.        *
 *           Minor "tweak" to move ordering.  GenerateMoves() now uses the     *
 *           LSB() function to enumerate white moves, while still using MSB()  *
 *           for black moves.  This has the effect of moving pieces toward     *
 *           your opponent before moving them away.  A severe oversight        *
 *           regarding the use of "!" (as in wtm=!wtm) was found and corrected *
 *           to speed things up some.  !wtm was replaced by wtm^1 (actually,   *
 *           a macro Flip(wtm) is used to make it more readable) which         *
 *           resulted in significant speedup on the sparc, but a more modest   *
 *           improvement on the pentium.                                       *
 *                                                                             *
 *    9.25   Minor bug in Book() fixed where it was possible for Crafty to     *
 *           play a book move that was hardly ever played.  Minor change to    *
 *           time utilization to use a little more time "up front".  Tuned     *
 *           piece/square tables to improve development some.  Saw Crafty lose *
 *           an occasional game because (for example) the bishop at c1 or c8   *
 *           had not moved, but the king had already castled, which turned the *
 *           EvaluateDevelopment() module "off".  first[ply] removed, since it *
 *           was redundant with last[ply-1].  The original intent was to save  *
 *           a subscript math operation, but the compilers are quite good with *
 *           the "strength-reduction" optimization, making referencing a value *
 *           by first[ply] or last[ply-1] exactly the same number of cycles.   *
 *           Passed pawn scores increased.  The reduction done to reduce all   *
 *           scores seemed to make Crafty less attentive to passed pawns and   *
 *           the threats they incur.  Minor bug in Book() fixed, the variable  *
 *           min_percent_played was inoperative due to incorrect test on the   *
 *           book_status[n] value.  Pawn rams now scored a little differently, *
 *           so that pawn rams on Crafty's side of the board are much worse    *
 *           than rams elsewhere.  This avoids positional binds where, for ex- *
 *           ample Crafty would like black with pawns at e6 d5 c6, white with  *
 *           pawns at e5 and c5.  Black has a protected passed pawn at d5 for  *
 *           sure, but the cramp makes it easy for white to attack black, and  *
 *           makes it difficult for black to defend because the black pawns    *
 *           block the position badly.  King safety tweaked up about 30%, due  *
 *           to using nearly symmetrical scoring, the values had gotten too    *
 *           small and were not overriding minor positional things.            *
 *                                                                             *
 *    9.26   Minor bug fixes in time allocation.  This mainly affects Crafty   *
 *           as it plays on a chess server.  The problem is, simply, that the  *
 *           current internal timing resolution is 1/10 of a second, kept in   *
 *           an integer variable.  .5 seconds is stored as 5, for example.     *
 *           This became a problem when the target time for a search dropped   *
 *           below .3 seconds, because the "easy move" code would terminate    *
 *           the search after 1/3 of the target time elapsed if there were no  *
 *           fail lows or PV changes.  Unfortunately, 2 divided by 3 (.2 secs  *
 *           /3) is "zero" which would let the search quickly exit, possibly   *
 *           without producing a PV with a move for Crafty to play.  Crafty    *
 *           would play the first PV move anyway, which was likely the PV move *
 *           from the last search.  This would corrupt the board information,  *
 *           often produce the error "illegal move" when the opponent tried to *
 *           move, or, on occasion, blow Crafty up.  On a chess server, Crafty *
 *           would simply flag and lose.  After this fix, I played several     *
 *           hundred games with a time control of 100 moves in 1 second and    *
 *           there were no failures.  Before the fix, this time control could  *
 *           not result in a single completed game.  If you don't run Crafty   *
 *           on a chess server, this version is not important, probably. a     *
 *           minor fix for move input, so that moves like e2e4 will always be  *
 *           accepted, where before they would sometimes be flagged as errors. *
 *                                                                             *
 *    9.27   This version has an interesting modification to NextCapture().    *
 *           Quiesce() now passes alpha to NextCapture() and if a capture      *
 *           appears to win material, but doesn't bring the score up to the    *
 *           value of alpha, the capture is ignored.  In effect, if we are so  *
 *           far behind that a capture still leaves us below alpha, then there *
 *           is nothing to gain by searching the capture because our opponent  *
 *           can simply stand pat at the next ply leaving the score very bad.  *
 *           One exception is if the previous ply was in check, we simply look *
 *           for any safe-looking capture just in case it leads to mate.  Bad  *
 *           book bug with book random=1 or 2 fixed (this is set when playing  *
 *           a computer).  This bug would cause Crafty to play an illegal move *
 *           and bust wide open with illegal move errors, bad move hashed, and *
 *           other things.  Book randomness also quite a bit better now.       *
 *                                                                             *
 *    9.28   Piece/square table added for rooks.  Primary purpose is to dis-   *
 *           courage rooks moving to the edge of the board but in front of     *
 *           pawns, where it is easy to trap it.  Crafty now understands the   *
 *           concept of blockading a passed pawn to prevent it from advancing. *
 *           it always understood that advancing one was good, which would     *
 *           tend to encourage blockading, but it would also often ignore the  *
 *           pawn and let it advance a little here, a little there, until it   *
 *           suddenly realized that it was a real problem.  The command to set *
 *           the size of the hash table has been modified.  This command is    *
 *           now "hash n", "hash nK" or "hash nM" to set the hash table to one *
 *           of bytes, Kbytes, or Mbytes.  Note that if the size is not an     *
 *           exact multiple of what Crafty needs, it silently reduces the size *
 *           to an optimum value as close to the suggested size as possible.   *
 *           Trade bonus/penalty is now back in, after having been removed     *
 *           when the new UnmakeMove() code was added.  Crafty tries to trade  *
 *           pawns but not pieces when behind, and tries to trade pieces but   *
 *           not pawns when ahead.  EvaluateDraws() now understands that rook  *
 *           pawns + the wrong bishop is a draw.  Minor adjustment to the      *
 *           quiescence-search pruning introduced in 9.27, to avoid excluding  *
 *           captures that didn't lose material, but looked bad because the    *
 *           positional score was low.  Slightly increased the values of the   *
 *           pieces relative to that of a pawn to discourage the tendency to   *
 *           trade a piece for two or three pawns plus some positional plusses *
 *           that often would slowly vanish.  Null-move search modified to try *
 *           null-moves, even after captures.  This seems to hurt some prob-   *
 *           lem positions, but speeds up others significantly.  Seems better  *
 *           but needs close watching.  EvaluateDraws() call moved to the top  *
 *           of Evaluate() to check for absolute draws first.  Quiesce() now   *
 *           always calls Evaluate() which checks for draws before trying the  *
 *           early exit tests to make sure that draws are picked up first.     *
 *           EvaluateDraws() substantially re-written to get rid of lots of    *
 *           unnecessary instructions, and to also detect RP+B of wrong color  *
 *           even if the losing side has a pawn of its own.  Crafty would      *
 *           often refuse to capture that last pawn to avoid reaching an end-  *
 *           game it knew was drawn.                                           *
 *                                                                             *
 *    9.29   New time.c as contributed by Mike Byrne.  Basically makes Crafty  *
 *           use more time up front, which is a method of anticipating that    *
 *           Crafty will predict/ponder pretty well and save time later on     *
 *           anyway.  InCheck() is never called in Quiesce() now.  As a        *
 *           result, Crafty won't recognize mates in the q-search.  This has   *
 *           speeded the code up with no visible penalty, other than it will   *
 *           occasionally find a win of material rather than a mate score, but *
 *           this has not affected the problem suite results in a measurable   *
 *           way, other than Crafty is now about 10% faster in average type    *
 *           position, and much faster in others.  The latest epd code from    *
 *           Steven Edwards is a part of this version, which includes updated  *
 *           tablebase access code.                                            *
 *                                                                             *
 *   10.0    New time.c with a "monitoring feature" that has two distinct      *
 *           functions:  (1) monitor Crafty's time remaining and if it is too  *
 *           far behind the opponent, force it to speed up, or if it is well   *
 *           ahead on time, slow down some;  (2) if opponent starts moving     *
 *           quickly, Crafty will speed up as well to avoid getting too far    *
 *           behind.  EvaluateDraws() modified to detect that if one side has  *
 *           insufficient material to win, the score can never favor that side *
 *           which will make Crafty avoid trading into an ending where it has  *
 *           KNN vs KB for example, where KNN seems to be ahead.               *
 *                                                                             *
 *   10.1    New book.c.  Crafty now counts wins, losses and draws for each    *
 *           possible book move and can use these to play a move that had a    *
 *           high percentage of wins, vs the old approach of playing a move    *
 *           just because it was played a lot in the past.                     *
 *                                                                             *
 *   10.2    Evaluation tuning.  Rook on 7th, connected rooks on 7th, etc.,    *
 *           are now more valuable.  Crafty was mistakenly evaluating this     *
 *           too weakly.  Book() changed so that the list of moves is first    *
 *           sorted based on winning percentages, but then, after selecting    *
 *           the sub-set of this group, the list is re-sorted based on the     *
 *           raw total games won so that the most popular opening is still the *
 *           most likely to be played.                                         *
 *                                                                             *
 *   10.3    Timer resolution changed to 1/100th of a second units, with all   *
 *           timing variables conforming to this, so that it is unnecessary    *
 *           to multiply by some constant to convert one type of timing info   *
 *           to the standard resolution.  Bug in evaluate.c fixed.  Trapped    *
 *           rook (rook at h1, king at g1 for example) code had a bad sub-     *
 *           script that caused erroneous detection of a trapped rook when     *
 *           there was none.                                                   *
 *                                                                             *
 *   10.4    Quiesce() no longer checks for time exceeded, nor does it do any  *
 *           hashing whatsoever, which makes it significantly faster, while    *
 *           making the trees only marginally bigger.                          *
 *                                                                             *
 *   10.5    Quiesce() now calls GenerateCaptures() to generate capture moves. *
 *           This new move generator module is significantly faster, which     *
 *           speeds the quiescence search up somewhat.                         *
 *                                                                             *
 *   10.6    CastleStatus is now handled slightly differently.  The right two  *
 *           bits still indicate whether or not that side can castle to either *
 *           side, and 0 -> no castling at all, but now, <0 means that no      *
 *           castling can be done, and that castling rights were lost by       *
 *           making a move that was not castling.  This helps in Evaluate()    *
 *           by eliminating a loop to see if one side castled normally or had  *
 *           to give up the right to castle for another (bad) reason.  This is *
 *           simply an efficiency issue, and doesn't affect play or anything   *
 *           at all other than to make the search faster.                      *
 *                                                                             *
 *   10.7    NextMove() now doesn't try history moves forever, rather it will  *
 *           try the five most popular history moves and if it hasn't gotten a *
 *           cutoff by then, simply tries the rest of the moves without the    *
 *           history testing, which is much faster.                            *
 *                                                                             *
 *   10.8    Book() now "puzzles" differently.  In puzzling, it is trying to   *
 *           find a move for the opponent.  Since a book move by the opponent  *
 *           will produce an instant move, puzzle now enumerates the set of    *
 *           legal opponent moves, and from this set, removes the set of known *
 *           book moves and then does a quick search to choose the best.  We   *
 *           then ponder this move just in case the opponent drops out of book *
 *           and uses a normal search to find a move.  Crafty now penalizes    *
 *           a pawn that advances, leaving its neighbors behind where both     *
 *           the advancing pawn, and the ones being left behind become weaker. *
 *           the opening phase of the game has been modified to "stay active"  *
 *           until all minor pieces are developed and Crafty has castled, to   *
 *           keep EvaluateDevelopment() active monitoring development.  Minor  *
 *           change to DrawScore() so that when playing a computer, it will    *
 *           return "default_draw_score" under all circumstances rather than   *
 *           adjusting this downward, which makes Crafty accept positional     *
 *           weaknesses rather than repeat a position.                         *
 *                                                                             *
 *   10.9    Evaluate() now handles king safety slightly differently, and      *
 *           increases the penalty for an exposed king quite a bit when there  *
 *           is lots of material present, but scales this increase down as the *
 *           material comes off.  This seems to help when attacking or getting *
 *           attacked.  Since Quiesce() no longer probes the hash table, the   *
 *           idea of storing static evaluations there became worthless and was *
 *           burning cycles that were wasted.  This has been removed. Check    *
 *           extensions relaxed slightly so that if ply is < iteration+10 it   *
 *           will allow the extension.  Old limit was 2*iteration.  Connected  *
 *           passed pawns on 6th now evaluated differently.  They get a 1 pawn *
 *           bonus, then if material is less than a queen, another 1 pawn      *
 *           bonus, and if not greater than a rook is left, and the king can't *
 *           reach the queening square of either of the two pawns, 3 more      *
 *           more pawns are added in for the equivalent of + a rook.           *
 *                                                                             *
 *   10.10   Evaluate() modified.  King safety was slightly broken, in that    *
 *           an uncastled king did not get a penalty for open center files.    *
 *           new evaluation term for controlling the "absolute 7th rank", a    *
 *           concept from "My System".                                         *
 *                                                                             *
 *   10.11   Lazy evaluation implemented.  Now as the Evaluate() code is ex-   *
 *           ecuted, Crafty will "bail out" when it becomes obvious that the   *
 *           remainder of the evaluation can't bring the score back inside the *
 *           alpha/beta window, saving time.                                   *
 *                                                                             *
 *   10.12   More Jakarta time control commands added so operator can set the  *
 *           time accurately if something crashes, and so the operator can set *
 *           a "cushion" to compensate for operator inattention.  "Easy" is    *
 *           more carefully qualified to be a recapture, or else a move that   *
 *           preserves the score, rather than one that appears to win material *
 *           as was done in the past.  King safety tweaked a little so that if *
 *           one side gives up the right to castle, and could castle to a safe *
 *           position, the penalty for giving up the right is substantially    *
 *           larger than before.  A minor bug also made it more likely for     *
 *           black to give up the right to castle than for white.              *
 *                                                                             *
 *   10.13   Modifications to Book() so that when using "book random 0" mode   *
 *           (planned for Jakarta) and the search value indicates that we are  *
 *           losing a pawn, we return "out of book" to search *all* the moves, *
 *           Not just the moves that are "in book."  This hopefully avoids     *
 *           some book lines that lose (gambit) material unsoundly.            *
 *                                                                             *
 *   10.14   Final timing modifications.  Puzzling/booking searches now take   *
 *           1/30th of the normal time, rather than 1/10th.  New command       *
 *           "mode=tournament" makes Crafty assess DRAW as "0" and also makes  *
 *           it prompt the operator for time corrections as required by WMCCC  *
 *           rules.                                                            *
 *                                                                             *
 *   10.15   Evaluate() tuning to reduce development bonuses, which were able  *
 *           to overwhelm other scores and lead to poor positions.             *
 *                                                                             *
 *   10.16   "Lazy" (early exit) caused some problems in Evaluate() since so   *
 *           many scores are very large.  Scaled this back significantly and   *
 *           also EvaluateOutsidePassedPawns() could double the score if one   *
 *           side had passers on both sides.  This was fixed.                  *
 *                                                                             *
 *   10.17   Crafty's Book() facility has been significantly modified to be    *
 *           more understandable.  book random <n> has the following options:  *
 *           (0) do a search on set of book moves; (1) choose from moves that  *
 *           are played most frequently; (2) choose from moves with best win   *
 *           ratio; (3) choose from moves with best static evaluation; and     *
 *           (4) choose completely randomly.  book width <n> can be used to    *
 *           control how large the "set" of moves will be, with the moves in   *
 *           the "set" being the best (according to the criteria chosen) from  *
 *           the known book moves.  Minor tweak to Search() so that it extends *
 *           passed pawn pushes to the 6th or 7th rank if the move is safe,    *
 *           and this is an endgame.                                           *
 *                                                                             *
 *   10.18   New annotate command produces a much nicer analysis file and also *
 *           lets you exert control over what has to happen to trigger a com-  *
 *           ment among other things.  AKA Crafty/Jakarta version.             *
 *                                                                             *
 *   11.1    Fractional depth allows fractional ply increments.  The reso-     *
 *           lution is 1/4 of a ply, so all of the older "depth++" lines now   *
 *           read depth+=PLY, where #define PLY 4 is used everywhere.  This    *
 *           gives added flexibility in that some things can increment the     *
 *           depth by < 1 ply, so that it takes two such things before the     *
 *           search actually extends one ply deeper.  Crafty now has full PGN  *
 *           support.  A series of pgn commands (pgn Event 14th WMCCC) allow   *
 *           the PGN tags to be defined, when Crafty reads or annotates a PGN  *
 *           file it reads and parses the headers and will display the info    *
 *           in the annotation (.can) file or in the file you specify when you *
 *           execute a "savegame <filename>" command.                          *
 *                                                                             *
 *   11.2    Fractional ply increments now implemented.  11.1 added the basic  *
 *           fractional ply extension capabilities, but this version now uses  *
 *           them in Search().  There are several fractional ply extensions    *
 *           being used, all are #defined in types.h to make playing with them *
 *           somewhat easier.  One advantage is that now all extensions are    *
 *           3/4 of a ply, which means the first time any extension is hit, it *
 *           has no effect, but the next three extensions will result in an    *
 *           additional ply, followed by another ply where the extension does  *
 *           nothing, followed by ...  This allowed me to remove the limit on  *
 *           how deep in the tree the extensions were allowed, and also seems  *
 *           to not extend as much unless there's "something" going on.  When  *
 *           the position gets interesting, these kick in.  Asymmetric king    *
 *           safety is back in.  Crafty's king safety is a little more im-     *
 *           portant than the opponent's now to help avoid getting its king-   *
 *           side shredded to win a pawn.  A new term, ROOK_ON_7TH_WITH_PAWN   *
 *           to help Crafty understand how dangerous a rook on the 7th is if   *
 *           it is supported by a pawn.  It is difficult to drive off and the  *
 *           pawn provides additional threats as well.                         *
 *                                                                             *
 *   11.3    King safety modifications.  One fairly serious bug is that Crafty *
 *           considered that king safety became unimportant when the total     *
 *           pieces dropped below 16 material points.  Note that this was      *
 *           pieces only, not pieces and pawns, which left plenty of material  *
 *           on the board to attack with.  As a result, Crafty would often     *
 *           allow its king-side to be weakened because it was either in an    *
 *           endgame, or close to an endgame.  *Incorrectly*, it turns out. :) *
 *           Going back to a very old Crafty idea, king tropism is now tied to *
 *           how exposed each king is, with pieces being attracted to the most *
 *           exposed king for attack/defense.                                  *
 *                                                                             *
 *   11.4    Tuning on outside passed pawn code.  Values were not large enough *
 *           and didn't emphasize how strong such a pawn is.  General passed   *
 *           pawn value increased as well.  Minor bug in edit that would let   *
 *           Crafty flag in some wild games was fixed.  Basically, edit has no *
 *           way to know if castling is legal, and has to assume if a rook and *
 *           king are on the original squares, castling is o.k.  For wild2     *
 *           games in particular, this can fail because castling is illegal    *
 *           (by the rules for wild 2) while it might be that rooks and king   *
 *           are randomly configured to make it look possible.  Crafty would   *
 *           then try o-o or o-o-o and flag when the move was rejected as      *
 *           illegal.  richard Lloyd suggested a speed-up for the annotate     *
 *           command which I'm using temporarily.  Crafty now searches all     *
 *           legal moves in a position, and if the best move matches the game  *
 *           move, the second search of just the move played is not needed.  I *
 *           was searching just the game move first, which was a waste of time *
 *           in many positions.  I'll resume this later, because I want to see *
 *           these values:  (1) score for move actually played; (2) score for  *
 *           the rest of the moves only, not including the move actually       *
 *           played, to see how often a player finds a move that is the *only* *
 *           good move in a position.                                          *
 *                                                                             *
 *   11.5    More fractional ply extension tuning.  A few positions could blow *
 *           up the search and go well beyond 63 plies, the max that Crafty    *
 *           can handle.  Now, extensions are limited to no more than one ply  *
 *           of extensions for every ply of search.  Note that "in check" is   *
 *           extended on the checking move ply, not on the out of check ply,   *
 *           so that one-reply-to-check still extends further.  Minor changes  *
 *           to time.c to further help on long fast games.  Another important  *
 *           timing bug repaired.  If Crafty started an iteration, and failed  *
 *           high on the first move, it would not terminate the search until   *
 *           this move was resolved and a score returned.  This could waste a  *
 *           lot of time in already won positions.  Interesting bug in the     *
 *           capture search pruning fixed.  At times, a piece would be hung,   *
 *           but the qsearch would refuse to try the capture because it would  *
 *           not bring the score back to above alpha.  Unfortunately, taking   *
 *           the piece could let passed pawns race in undisturbed which would  *
 *           affect the score.  This pruning is turned off when there are not  *
 *           at least two pieces left on the board.  Another bug in the move   *
 *           generator produced pawn promotions twice, once when captures were *
 *           generated, then again when the rest of the moves are produced.    *
 *           this was also repaired.  Small book now will work.  The Bookup()  *
 *           code now assumes white *and* black won in the absence of a valid  *
 *           PGN Result tag, so that the small book will work correctly.       *
 *                                                                             *
 *   11.6    Modifications to time allocation in a further attempt to make     *
 *           Crafty speed up if time gets short, to avoid flagging.  It is now *
 *           *very* difficult to flag this thing.  :)  Minor evaluation tweaks *
 *           and a fix to Quiesce() to speed it up again after a couple of bad *
 *           fixes slowed things down.                                         *
 *                                                                             *
 *   11.7    Minor modification to Evaluate().  Rooks behind a passed pawn are *
 *           good, but two rooks behind the pawn don't help if the pawn is     *
 *           blockaded and can't move, unless the two rooks are not the same   *
 *           color as the pawn.                                                *
 *                                                                             *
 *   11.8    First stage of "book learning" implemented.  Crafty monitors the  *
 *           search evaluation for the first 10 moves out of book.  It then    *
 *           computes a "learn value" if it thinks this set of values shows    *
 *           that the book line played was very good or very bad.  This value  *
 *           is added to a "learn count" for each move in the set of book      *
 *           moves it played, with the last move getting the full learn value, *
 *           and moves closer to the start of the game getting a smaller       *
 *           percentage.  (See learn.c for more details).  These values are    *
 *           used by Book() to help in selecting/avoiding book lines.  Crafty  *
 *           produces a "book.lrn" file that synthesizes this information into *
 *           a portable format that will be used by other Crafty programs,     *
 *           once the necessary code is added later on.                        *
 *                                                                             *
 *   11.9    An age-old problem caused by null-move searching was eliminated   *
 *           in this version.  Often the null-move can cause a move to fail    *
 *           high when it should not, but then the move will fail low on the   *
 *           re-search when beta is relaxed.  This would, on occasion, result  *
 *           in Crafty playing a bad move for no reason.  Now, if a fail high  *
 *           is followed by a fail-low, the fail high condition is ignored.    *
 *           This can be wrong, but with null-move pruning, it can also be     *
 *           even "more wrong" to accept a fail high move after it fails low   *
 *           due to a null-move failure, than it would be right to accept the  *
 *           fail high/fail low that is caused by trans/ref table anomalies.   *
 *                                                                             *
 *   11.10   Crafty is now using the Kent, et. al, "razoring" algorithm, which *
 *           simply eliminates the last ply of full-width searching and goes   *
 *           directly to the capture search if the move before the last full-  *
 *           width ply is uninteresting, and the Material eval is 1.5 pawns    *
 *           below alpha.  It's a "modest futility" idea that doesn't give up  *
 *           on the uninteresting move, but only tries captures after the move *
 *           is played in the tree.  Net result is 20-30% average faster times *
 *           to reach the same depth.  Minor mod to book.lrn to include Black, *
 *           White and Date PGN tags just for reference.  Next learning mod is *
 *           another "book random n" option, n=3.  This will use the "learned" *
 *           score to order book moves, *if* any of them are > 0.  What this   *
 *           does is to encourage Crafty to follow opening lines that have     *
 *           been good, even if the learn count hasn't reached the current     *
 *           threshold of 1,000.  This makes learning "activate" faster.  This *
 *           has one hole in it, in that once Crafty learns that one move has  *
 *           produced a positive learn value, it will keep playing that move   *
 *           (since no others will yet have a positive value) until it finally *
 *           loses enough to take the learn value below zero.  This will be    *
 *           taken care of in the next version, hopefully.                     *
 *                                                                             *
 *   11.11   New "book random 4" mode that simply takes *all* learning scores  *
 *           for the set of legal book moves, translates them so that the      *
 *           worst value is +1 (by adding -Min(all_scores)+1 to every learn    *
 *           value) and then squaring the result.  This encourages Crafty to   *
 *           follow book lines that it has learned to be good, while still     *
 *           letting it try openings that it has not yet tried (learn value    *
 *           =0) and even lets it try (albeit rarely) moves that it has        *
 *           learned to be bad.  Minor evaluation tweaking for king placement  *
 *           in endgames to encourage penetration in the center rather than on *
 *           a wing where the king might get too far from the "action."        *
 *                                                                             *
 *   11.12   LearnFunction() now keeps positional score, rather than using a   *
 *           constant value for scores < PAWN_VALUE, to improve learning       *
 *           accuracy.  Book learn <filename> [clear] command implemented to   *
 *           import learning files from other Crafty programs.                 *
 *                                                                             *
 *   11.13   Endgame threshold lowered to the other side having a queen or     *
 *           less (9) rather than (13) points.  Queen+bishop can be very       *
 *           dangerous to the king for example, or two rooks and a bishop.     *
 *           Learning "curve" modified.  It was accidentally left in a sort    *
 *           of "geometric" shape, with the last move getting the full learn   *
 *           value, the next one back getting 1/2, the next 1/3, etc.  Now     *
 *           it is uniformly distributed from front to back.  If there are 20  *
 *           moves, the last gets the full value, the next gets 19/20, etc..   *
 *           Also the "percentage played" was hosed and is fixed.              *
 *                                                                             *
 *   11.14   Minor modification to book learn 4 code so that when you start    *
 *           off with *no* learned data, Book() won't exclude moves from the   *
 *           possible set of opening moves just because one was not played     *
 *           very often.  Due to the lack of "ordering" info in this case, it  *
 *           would often discover the second or third move in the list had not *
 *           been played very often and cull the rest of the list thinking it  *
 *           was ordered by the number of times it was played.  This code      *
 *           contains all of Frank's xboard patches so that analyze mode and   *
 *           so forth works cleanly.  A new book random option <5> modifies    *
 *           the probability of playing a move by letting the popularity of a  *
 *           move affect the learned-value sorting algorithm somewhat. Minor   *
 *           adjustment in pawn ram asymmetric term to simply count rams and   *
 *           not be too concerned about which half of the board they are on.   *
 *           adjustment to weak pawn code.  Also increased bonus for king      *
 *           tropism to attract pieces toward enemy king to improve attacking. *
 *           code that detects drawn K+RP+wrong bishop had a bug in that on    *
 *           occasion, the defending king might not be able to reach the       *
 *           queening square before the opposing king blocks it out.  This     *
 *           would let some positions look like draws when they were not. King *
 *           safety now uses square(7-distance(piece,king)) as one multiplier  *
 *           to really encourage pieces to get closer to the king.  In cases   *
 *           whre one king is exposed, all pieces are attracted to that king   *
 *           to attack or defend.   This is turned off during the opening.     *
 *           minor 50-move rule draw change to fix a bug, in that the game is  *
 *           not necessarily a draw after 50 moves by both sides, because one  *
 *           side might be mated by the 50th move by the other side.  This     *
 *           occurred in a game Crafty vs Ferret, and now works correctly.     *
 *                                                                             *
 *   11.15   Modified LearnBook() so that when Crafty terminates, or resigns,  *
 *           or sees a mate, it will force LearnBook() to execute the learning *
 *           analysis even if 10 moves have not been made sine leaving the     *
 *           book.  Crafty now trusts large positive evals less when learning  *
 *           about an opening, since such evals are often the result of the    *
 *           opponent's blunder, rather than a really  good opening line.      *
 *           second learning stage implemented.  Crafty maintains a permanent  *
 *           hash file that it uses to store positions where the search value  *
 *           (at the root of the tree) suddenly drop, and then it "seeds" the  *
 *           real transposition table with these values at the start of each   *
 *           of each search so that information is available earlier the next  *
 *           time this game is played.  Position.bin has the raw binary data   *
 *           this uses, while position.lrn has the "importable" data.  New     *
 *           import <filename> [clear] command imports all learning data now,  *
 *           eliminating the old book learn command.  LearnFunction() modified *
 *           to handle "trusted" values (negative scores, because we are sure  *
 *           that Crafty won't make an outright blunder very often) and        *
 *           "untrusted" values (positive values often caused by the opponent  *
 *           hanging a piece.  Trusted values are scaled up if the opponent is *
 *           weaker than Crafty since a weaker opponent forcing a negative     *
 *           eval means the position must really be bad, and are scaled down   *
 *           somewhat if the opponent is stronger than Crafty (from the rating *
 *           information from ICC or the operator using the rating command.)   *
 *           Untrusted values are scaled down harshly, unless the opponent is  *
 *           much stronger than Crafty, since it is unlikely such an opponent  *
 *           would really blunder, while weaker opponents drastically scale    *
 *           untrusted value (which is positive, remember) down.  minor depth  *
 *           problem fixed in learning code.  The depth used in LearnFunction  *
 *           was not the depth for the search that produced the "learn value"  *
 *           the LearnBook() chooses, it was the depth for the search last     *
 *           done.  Now, in addition to the 10 last search values, I store the *
 *           depth for each as well.  When a particular search value is used   *
 *           to "learn", the corresponding (correct) depth is also now used.   *
 *           These learn values are now limited to +6000/-6000 because in one  *
 *           book line, Crafty discovered it was getting mated in 2 moves at   *
 *           the end of the book line which was 30 moves deep.  If you try to  *
 *           distribute a "mate score" as the learned value, it will instantly *
 *           make every move for the last 26 moves look too bad to play.  This *
 *           is not desirable, rather the last few moves should be unplayable, *
 *           but other moves further back should be o.k.  Another bug that let *
 *           Crafty overlook the above mate also fixed.  The search value was  *
 *           not correct if Crafty got mated, which would avoid learning in    *
 *           that case.  This has been fixed, although it was obviously a rare *
 *           problem anyway.  Minor draw bug fixed, where Crafty would offer   *
 *           a draw when time was way low, caused by the resolution change a   *
 *           few months back, and also it would offer a draw after the         *
 *           opponent moved, even if he made a blunder.  It now only offers a  *
 *           draw after moving to make sure it's reasonable.  Crafty now won't *
 *           resign if there is a deep mate, such as a mate in 10+, because we *
 *           want to be sure that the opponent can demonstrate making progress *
 *           before we toss in the towel.  LearnBook() now learns on all games *
 *           played.  The old algorithm had a serious problem in that negative *
 *           scores were trusted fully, positive scores were treated with low  *
 *           confidence, and scores near zero were ignored totally.  The net   *
 *           result was that learning values became more and more negative as  *
 *           games were played.  Now equal positions are also factored in to   *
 *           the learning equation to help pull some of these negative values  *
 *           back up.  Book() now excludes moves that have not been played     *
 *           many times (if there is at least one move that has) for book      *
 *           random = 2, 3 and 4.  This stops some of the silly moves.         *
 *           position learning is turned off after 15 moves have passed since  *
 *           leaving book, to avoid learning things that won't be useful at a  *
 *           later time.  Hashing changed slightly to include "permanent"      *
 *           entries so that the learned stuff can be copied into the hash     *
 *           table with some confidence that it will stay there long enough to *
 *           be used.                                                          *
 *                                                                             *
 *   11.16   King tropism toned down a little to not be quite so intent on     *
 *           closeness to the kings, so that other positional ideas don't get  *
 *           "lost" in the eval.  Back to normal piece values (1,3,3,5,9) for  *
 *           a while.  Optimizations in Quiesce() speeded things up about 2%.  *
 *           Pawns pushed toward the king are not so valuable any more so that *
 *           Crafty won't prefer to hold on to them rather than trading to     *
 *           open a file(s) on the enemy king.  BookLearn() call was moved in  *
 *           Resign() so that it first learns, then resigns, so that it won't  *
 *           get zapped by a SIGTERM right in the middle of learning when      *
 *           xboard realizes the game is over.  Edit/setboard logic modified   *
 *           so that it is now possible to back up, and move forward in a game *
 *           that was set up via these commands.                               *
 *                                                                             *
 *   11.17   EvaluatePawns() modified to eliminate "loose pawn" penalty which  *
 *           was not particularly effective and produced pawn scores that were *
 *           sometimes misleading.  Several optimizations dealing with cache   *
 *           efficiency, primarily changing int's to signed char's so that     *
 *           multiple values fit into a cache line together, to eliminate some *
 *           cache thrashing.  PopCnt() no longer used to recognize that we    *
 *           have reached an endgame database, there's now a counter for the   *
 *           total number of pieces on the board, as it's much faster to inc   *
 *           and dec that value rather than count 1 bits.  Tweaks to MakeMove  *
 *           and UnmakeMove to make things a couple of percent faster, but a   *
 *           whole lot cleaner.  Ditto for SEE and Quiesce.  More speed, and   *
 *           cleaner code.  Outside passed pawn scored scaled down except when *
 *           the opponent has no pieces at all.  "Lazy eval" cleaned up.  We   *
 *           now have two early exits, one before king safety and one after    *
 *           king safety.  There are two saved values so we have an idea of    *
 *           how large the positional scores have gotten to make this work.    *
 *           Bug in passed pawn code was evaluating black passed pawns as less *
 *           valuable than white.  Fixed and passed pawn scores increased      *
 *           slightly.  Isolated pawn scoring modified so the penalty is pro-  *
 *           portional to the number of isolani each side has, rather than the *
 *           old approach of penalizing the "excess" isolani one side has over *
 *           the other side.  Seems to be more accurate, but we'll see.  Fixed *
 *           a rather gross bug in ValidateMove() related to the way castling  *
 *           status not only indicates whether castling is legal or not, but   *
 *           if it's not, whether that side castled or move the king/rooks.    *
 *           this let ValidateMove() fail to correctly detect that a castling  *
 *           move from the hash table might be invalid, which could wreck the  *
 *           search easily.  This was an oversight from the new status code.   *
 *           Serious book bug fixed.  Bookup() was not counting wins and       *
 *           losses correctly, a bug introduced to make small.pgn work. Minor  *
 *           bug in the "trapped bishop" (a2/h2/a7/h7) code fixed.  Minor bug  *
 *           in SEE() also fixed (early exit that was not safe in rare cases.) *
 *                                                                             *
 *   11.18   EvaluatePawns() modified to recognize that if there are two white *
 *           pawns "rammed" by black pawns at (say) c4/c5 and e4/e5, then any  *
 *           pawns on the d-file are also effectively rammed as well since     *
 *           there is no hope for advancing it.  Auto232 automatic play code   *
 *           seems to be working, finally.  The problem was Crafty delayed     *
 *           echoing the move back to auto232, if the predicted move was the   *
 *           move played by the opponent.  Auto232 gave Crafty 3 seconds and   *
 *           then said "too late, you must be hung."  Outpost knight scoring   *
 *           modified to produce scores between 0 and .2 pawns.  A knight was  *
 *           getting too valuable, which caused some confusion in the scoring. *
 *                                                                             *
 *   11.19   Slight reduction in the value of passed pawns, as they were       *
 *           getting pretty large.  Cleaned up annotate.c to get rid of code   *
 *           that didn't work out.  Annotate now gives the PV for the move     *
 *           actually played as well as the move Crafty suggests is best.      *
 *           Crafty now has a reasonable "trade" bonus that kicks in when it   *
 *           has 20 points of material (or less) and is ahead or behind at     *
 *           least 2 pawns.  This trade bonus ranges from 0 to 1000 (one pawn) *
 *           and encourages the side that is winning to trade, while the side  *
 *           that is losing is encouraged to avoid trades.  More minor tweaks  *
 *           for auto232 including trying a delay of 100ms before sending      *
 *           a move.  New annotate command gives the option to get more than   *
 *           one suggested best move displayed, as well as fixes a few bugs    *
 *           in the older code.  Fix to bishop + wrong rook pawn code to also  *
 *           recognize the RP + no pieces is just as bad if the king can reach *
 *           the queening square first.                                        *
 *                                                                             *
 *   11.20   Cleanup to annotate command.  It now won't predict that one side  *
 *           will follow a book line that only resulted in losses.  Also a     *
 *           couple of glitches in annotate where it was possible for a move   *
 *           to take an exhorbitant amount of time if the best move was a      *
 *           book move, but there were no other possible book moves.  The      *
 *           trade bonus now includes trade pawns if behind, but not if ahead  *
 *           as well as now considering being ahead or behind only one pawn    *
 *           rather than the two pawns used previously in 11.19.               *
 *                                                                             *
 *   11.21   Additional bug fix in annotate command that would fail if there   *
 *           was only one legal move to search.  Passed pawn values now have   *
 *           an added evaluation component based on material on the board, so  *
 *           that they become more valuable as material is traded.             *
 *                                                                             *
 *   11.22   Test command modified.  Test <filename> [N] (where N is optional  *
 *           and defaults to 99) will terminate a test position after N con-   *
 *           secutive iterations have had the correct solution.  Bug in        *
 *           Quiesce() fixed, which would incorrectly prune (or fail to prune) *
 *           some captures, due to using "Material" which is computed with     *
 *           respect to white-to-move.  This was fixed many versions ago, but  *
 *           the fix was somehow lost.  Fix to EvaluateDraws that mistakenly   *
 *           declared some endings drawn when there were rookpawns on both     *
 *           sides of the board, and the king could get in front of one. Minor *
 *           fix to Repeat() that was checking one too many entries.  No       *
 *           harmful effect other than one extra iteration in the loop that    *
 *           would make "purify/purity" fail as well as slow things down 1% or *
 *           so.                                                               *
 *                                                                             *
 *   11.23   logpath=, bookpath= and tbpath= command line options added to     *
 *           direct logs, books and tb files to the indicated path.  These     *
 *           only work as command line options, and can't be used in normal    *
 *           places since the files are already opened and in use.  A bug in   *
 *           Evaluate() would call EvaluatePawns() when there were no pawns,   *
 *           breaking things on many systems.                                  *
 *                                                                             *
 *   12.0    Rewrite of Input/Output routines.  Crafty no longer relies on the *
 *           C library buffered I/O routine scanf() for input.  This made it   *
 *           basically impossible to determine when to read data from the key- *
 *           board, which made xboard/winboard difficult to interface with.    *
 *           Now Crafty does its own I/O using read(), which is actually much  *
 *           cleaner and easier.  Minor bug where a fail high in very short    *
 *           time controls might not get played...                             *
 *                                                                             *
 *   12.1    Clean-up of Main().  The code had gotten so obtuse with the       *
 *           pondering call, plus handling Option() plus move input, that it   *
 *           was nearly impossible to follow.  It is now *much* cleaner and    *
 *           easier to understand/follow.                                      *
 *                                                                             *
 *   12.2    Continued cleanup of I/O, particularly the way main() is          *
 *           structured, to make it more understandable.  Rewritten time.c     *
 *           to make it more readable and understandable, not to mention       *
 *           easier to modify.  Fixed a timing problem where Crafty could be   *
 *           pondering, and the predicted move is made followed immediately by *
 *           "force", caused when the opponent makes a move and loses on time, *
 *           or makes a move and resigns.  This would leave the engine state   *
 *           somewhat confused, and would result in the "new" command not      *
 *           reinitializing things at all, which would hang Crafty on xboard   *
 *           or winboard.                                                      *
 *                                                                             *
 *   12.3    Modified piece values somewhat to increase their worth relative   *
 *           to a pawn, to try to stop the tendency to sac a piece for 3 pawns *
 *           and get into trouble for doing so.  Minor fixes in NewGame() that *
 *           caused odd problems, such as learning positions after 1. e4.  The *
 *           position.lrn/.bin files were growing faster than they should.     *
 *           Other minor tweaks to fix a few broken commands in Option().      *
 *           Slight reduction in trade bonus to match slightly less valuable   *
 *           pawns.                                                            *
 *                                                                             *
 *   12.4    Added ABSearch() macro, which makes Search() much easier to read  *
 *           by doing the test to call Search() or Quiesce() in the macro      *
 *           where it is hidden from sight.  Bugs in the way the log files     *
 *           were closed and then used caused crashed with log=off.            *
 *                                                                             *
 *   12.5    Development bonus for central pawns added to combat the h4/a3     *
 *           sort of openings that would get Crafty into trouble.  It now will *
 *           try to seize the center if the opponent dallies around.           *
 *                                                                             *
 *   12.6    Bug in EvaluateDraws() that would erroneously decide that a KNP   *
 *           vs K was drawn if the pawn was a rook pawn and the king could get *
 *           in front of it, which was wrong.  Another minor bug would use     *
 *           EvaluateMate() when only a simple exchange ahead, which would     *
 *           result in sometimes giving up a pawn to reach such a position.    *
 *           Minor bug in SetBoard() would incorrectly reset many important    *
 *           game state variables if the initial position was set to anything  *
 *           other than the normal starting position, such as wild 7.  This    *
 *           would make learning add odd PV's to the .lrn file.  Development   *
 *           bonus modified to penalize moving the queen before minor pieces,  *
 *           and not moving minor pieces at all.  It is now possible to build  *
 *           a "wild 7" book, by first typing "wild 7", and then typing the    *
 *           normal book create command.  Crafty will remember to reset to the *
 *           "wild 7" position at the start of each book line.  Note that it   *
 *           is not possible to have both wild 7 book lines *and* regular book *
 *           lines in the same book.                                           *
 *                                                                             *
 *   12.7    Failsoft added, so that scores can now be returned outside the    *
 *           alpha/beta window.  Minor null-move threat detection added where  *
 *           a mate in 1 score, returned by a null-move search, causes a one   *
 *           ply search extension, since a mate in one is a serious threat.    *
 *           razoring is now disabled if there is *any* sort of extension in   *
 *           the search preceeding the point where razoring would be applied,  *
 *           to avoid razoring taking away one ply from a search that was ex-  *
 *           tended (for good reasons) earlier.  Fail-soft discarded, due to   *
 *           no advantages at all, and a slight slowdown in speed.  Rewrite of *
 *           pawn hashing, plus a rewrite of the weak pawn scoring code to     *
 *           make it significantly more accurate, while going a little slower  *
 *           (but since hashing hits 99% of the time, the slower speed is not  *
 *           noticable at all.)  Evaluation tweaking as well.                  *
 *                                                                             *
 *   12.8    Continued work on weak pawn analysis, as well as a few minor      *
 *           changes to clean the code up.                                     *
 *                                                                             *
 *   13.0    Preparation for the Paris WMCCC version starts here.  First new   *
 *           thing is the blocked pawn evaluation code.  This recognizes when  *
 *           a pawn is immobolized and when a pawn has lever potential, but    *
 *           is disabled if playing a computer opponent.  Passed pawn scoring  *
 *           modified to eliminate duplicate bonuses that were added in        *
 *           EvaluatePawns as well as EvaluatePassedPawns.  Trade bonus code   *
 *           rewritten to reflect trades made based on the material when the   *
 *           search begins.  This means that some things have to be cleared in *
 *           the hash table after a capture is really made on the board, but   *
 *           prevents the eval from looking so odd near the end of a game.     *
 *           minor bug with book random 0 fixed.  If there was only one book   *
 *           move, the search could blow up.                                   *
 *                                                                             *
 *   13.1    All positional scores reduced by 1/4, in an attempt to balance    *
 *           things a little better.  Also more modifications to the weak and  *
 *           blocked pawn code.  Added evaluation code to help with KRP vs KR, *
 *           so that the pawn won't be advanced too far before the king comes  *
 *           to its aid.  Ditto for defending, it knows that once the king is  *
 *           in front of the pawn, it is a draw.                               *
 *                                                                             *
 *   13.2    Blocked pawn scoring changed slightly to not check too far to see *
 *           if a pawn can advance.  Also pawns on center squares are weighted *
 *           more heavily to encourage keeping them there, or trying to get    *
 *           rid of them if the opponent has some there.  Discovered that      *
 *           somehow, the two lines of code that add in bonuses for passed     *
 *           pawns were deleted as I cleaned up and moved code around.  These  *
 *           were reinserted (missing from 13.0 on).                           *
 *                                                                             *
 *   13.3    Modified tablebase code to support 5 piece endings.  There is now *
 *           a configuration option (see Makefile) to specify 3, 4 or 5 piece  *
 *           endgame databases.  More evaluation tuning.                       *
 *                                                                             *
 *   13.4    Modified null-move search.  If there is more than a rook left for *
 *           the side on move, null is always tried, as before.  If there is   *
 *           a rook or less left, the null move is only tried within 5 plies   *
 *           of the leaf positions, so that as the search progresses deeper,   *
 *           null-move errors are at least moved away from the root.  Many     *
 *           evaluation changes, particularly to king safety and evaluating    *
 *           castling to one side while the other side is safer.  Also it now  *
 *           handles pre-castled positions better.                             *
 *                                                                             *
 *   13.5    "From-to" display patch from Jason added.  This shows the from    *
 *           and to squares on a small diagram, oriented to whether Crafty is  *
 *           playing black or white.  The intent is to minimize operator       *
 *           errors when playing black and the board coordinates are reversed. *
 *           More eval changes.                                                *
 *                                                                             *
 *   13.6    Weak pawns on open files now add value to the opponent's rooks    *
 *           since they are the pieces that will primarily use an open file to *
 *           attack a weak pawn.                                               *
 *                                                                             *
 *   13.7    Minor eval tweaks, plus fixes for the CR/LF file reading problems *
 *           in windows NT.                                                    *
 *                                                                             *
 *   13.8    Adjustments to book.c, primarily preventing Crafty from playing   *
 *           lines that had very few games in the file, because those are      *
 *           often quite weak players.  Minor fix to InputMoves() so that a    *
 *           move like bxc3 can *never* be interpreted as a bishop move.  A    *
 *           bishop move must use an uppercase B, as in Bxc3.  Bug in the      *
 *           king safety for white only was not checking for open opponent     *
 *           files on the king correctly.                                      *
 *                                                                             *
 *   13.9    Minor adjustment to time allocation to allow "operator time" to   *
 *           affect time per move.  This is set by "operator <n>" where <n> is *
 *           time in seconds per move that should be "reserved" to allow the   *
 *           operator some typing/bookkeeping time.  New command "book         *
 *           trigger <n>" added so that in tournament mode, when choosing a    *
 *           book line, Crafty will revert to book random 0 if the least       *
 *           popular move was played fewer times than <n>.  This is an attempt *
 *           to avoid trusting narrow book lines too much while still allowing *
 *           adequate randomness.  If the least frequently played move in the  *
 *           set of book moves was played fewer times than this, then a search *
 *           over all the book moves is done.  If the best one doesn't look    *
 *           so hot, Crafty drops out of book and searches *all* legal moves   *
 *           instead.                                                          *
 *                                                                             *
 *   13.10   Minor adjustments.  This is the gold Paris version that is going  *
 *           with Jason.  Any changes made during the event will be included   *
 *           in the next version.                                              *
 *                                                                             *
 *   14.0    Major eval modification to return to centipawn evaluation, rather *
 *           then millipawns.  This reduces the resolution, but makes it a lot *
 *           easier to understand evaluation changes.  Significant eval mods   *
 *           regarding king safety and large positional bonuses, where the ad- *
 *           vantage is not pretty solid.  Note that old book learning will    *
 *           not work now, since the evals are wrong.  Old position learning   *
 *           must be deleted completely.  See the read.me file for details on  *
 *           what must be done about book learning results.  New egtb=n option *
 *           to replace the older -DTABLEBASES=n Makefile option, so that the  *
 *           tablebases can be enabled or disabled without recompiling.        *
 *                                                                             *
 *   14.1    Glitch in EvaluateDevelopment() was producing some grossly large  *
 *           positional scores.  Also there were problems with where the       *
 *           PreEvaluate() procedure were called, so that it could be called   *
 *           *after* doing RootMoveslist(), but before starting a search.  This*
 *           caused minor problems with hashing scores.  One case was that it  *
 *           was possible for a "test suite" to produce slightly different     *
 *           scores than the same position run in normal mode.  This has been  *
 *           corrected.  St=n command now supports fractional times to .01     *
 *           second resolution.                                                *
 *                                                                             *
 *   14.2    MATE reduced to 32768, which makes all scores now fit into 16     *
 *           bits.  Added code to EvaluatePawns() to detect the Stonewall      *
 *           pawn formation and not like it as black.  Also it detects a       *
 *           reversed Stonewall formation (for black) and doesn't like it as   *
 *           white.  EvaluateOutsidePassedPawns() removed.  Functionality is   *
 *           incorporated in EvaluatePawns() and is hashed, which speeds the   *
 *           evaluation up a bit.  King safety is now fully symmetric and is   *
 *           folded into individual piece scoring to encourage pieces to       *
 *           attack an unsafe king.                                            *
 *                                                                             *
 *   14.3    Minor adjustments to the king safety code for detecting attacks   *
 *           initiated by pushing kingside pawns.  Flip/flop commands added to *
 *           mirror the board either horizontally or vertically to check for   *
 *           symmetry bugs in the evaluation.  Other eval tuning changes.      *
 *                                                                             *
 *   14.4    Queen value increased to stop trading queen for two rooks.  Book  *
 *           learning slightly modified to react quicker.  Also scores are now *
 *           stored in the book file as floats to get rid of an annoying trun- *
 *           cation problem that was dragging scores toward zero.  The book    *
 *           file now has a "version" so that old book versions won't cause    *
 *           problems due to incorrect formats.  Null-move mated threat ex-    *
 *           tension was modified.  If null-move search fails high, I fail     *
 *           high as always, if it fails low, and it says I get mated in N     *
 *           moves, I extend all moves at this ply, since something is going   *
 *           on.  I also carry this threat extension into the hash table since *
 *           a hash entry can say "don't do null move, it will fail low."  I   *
 *           now have a flag that says not only will null-move fail low, it    *
 *           fails to "mated in N" so this ply needs extending even without a  *
 *           null-move search to tell it so.  Book learning greatly modified   *
 *           in the way it "distributes" the learned values, in an effort to   *
 *           make learning happen faster.  I now construct a sort of "tree"    *
 *           so I know how many book alternatives Crafty had at each move it   *
 *           at each book move it played.  I then assign the whole learn value *
 *           to all moves below the last book move where there was a choice,   *
 *           and also assign the whole learn value to the move where there     *
 *           was a choice.  I then divide the learn value by the number of     *
 *           choices and assign this to each book move (working backward once  *
 *           again) until the next point where there were choices.  This makes *
 *           sure that if there is a choice, and it is bad, it won't be played *
 *           again, even if this choice was at move 2, and the remainder of    *
 *           the book line was completely forced.  All in an effort to learn   *
 *           more quickly.                                                     *
 *                                                                             *
 *   14.5    annotate bug fixed where if there was only one legal move, the    *
 *           annotation would stop with no warning.  Bookup() now produces a   *
 *           more useful error message, giving the exact line number in the    *
 *           input file where the error occurred, to make fixing them easier.  *
 *           If you are playing without a book.bin, 14.4 would crash in the    *
 *           book learning code.  This is now caught and avoided.  Bookup()    *
 *           now knows how to skip analysis in PGN files, so you can use such  *
 *           files to create your opening book, without having problems.  It   *
 *           understands that nesting () {} characters "hides" the stuff in-   *
 *           side them, as well as accepting non-standard PGN comments that    *
 *           are inside [].  Annotate() now will annotate a PGN file with      *
 *           multiple games, although it will use the same options for all of  *
 *           the games.  PGN headers are preserved in the .can file.  Annotate *
 *           now will recognize the first move in a comment (move) or {move}   *
 *           and will search that move and give analysis for it in addition to *
 *           the normal output.                                                *
 *                                                                             *
 *   14.6    Minor change to book random 4 (use learned value.)  if all the    *
 *           learned values are 0, then use the book random 3 option instead.  *
 *           Bug in 50-move counter fixed.  Learn() could cause this to be set *
 *           to a value that didn't really reflect the 50-move status, and     *
 *           cause games to be drawn when they should not be.  Modified the    *
 *           "Mercilous" attack code to recognize a new variation he worked    *
 *           out that sacrifices a N, R and B for a mating attack.  No more.   *
 *                                                                             *
 *   14.7    "Verbose" command removed and replaced by a new "display" command *
 *           that sets display options.  Use "help display" for more info on   *
 *           how this works.  The "mercilous" attack scoring is only turned on *
 *           when playing "mercilous".  But there is a new "special" list that *
 *           you can add players too if you want, and this special scoring is  *
 *           turned on for them as well.                                       *
 *                                                                             *
 *   14.8    New scoring term to penalize unbalanced trades.  IE if Crafty has *
 *           to lose a pawn, it would often trade a knight for two pawns in-   *
 *           stead, which is actually worse.  This new term prevents this sort *
 *           of trades.                                                        *
 *                                                                             *
 *   14.9    "Mercilous" attack code modified to handle a new wrinkle he (and  *
 *           one other player) found.                                          *
 *                                                                             *
 *   14.10   New "bench" command runs a test and gives a benchmark comparison  *
 *           for Crafty and compares this to a P6/200 base machine.  Minor bug *
 *           disabled the "dynamic draw score" and fixed it at zero, even if   *
 *           the opponent was much weaker (rating) or behind on time.  Also    *
 *           draw_score was not reset to 0 at start of each new game, possibly *
 *           leaving it at +.20 after playing a higher-rated opponent.         *
 *                                                                             *
 *   14.11   Release to make mercilous attack detection code the default, to   *
 *           prevent the rash of mercilous attacks on the chess servers.       *
 *                                                                             *
 *   14.12   Modified tuning of evaluation.  Scores were reduced in the past,  *
 *           to discourage the piece for two pawns sort of trades, but since   *
 *           this was basically solved in 14.8, scores are going "back up".    *
 *           Particularly passed pawn scores.  Crafty now accepts draw offers  *
 *           if the last search result was <= the current result returned by   *
 *           DrawScore().  It also honors draw requests via xboard when        *
 *           against a human opponent as well, and will flag the game as over. *
 *           Minor modification to annotate.c to display move numbers in the   *
 *           PV's to match the PV output format used everywhere else in        *
 *           Crafty.  A new type of learning, based on results, has been added *
 *           and is optional (learn=7 now enables every type of learning.)     *
 *           this type of learning uses the win/lose game result to flag an    *
 *           opening that leads to a lost game as "never" play again.  The     *
 *           book move selection logic has been modified.  There are four com- *
 *           ponents used in choosing book moves:  frequency of play; win/lose *
 *           ratio; static evaluation; and learned score.  There are four      *
 *           weights that can be modified with the bookw command, that control *
 *           how these 4 values are used to choose book moves.  Each weight is *
 *           a floating point value between 0 and 1, and controls how much the *
 *           corresponding term factors in to move selection.                  *
 *                                                                             *
 *   14.13   Fixed a book parsing bug that now requires a [Site "xxx"] tag in  *
 *           any book file between lines for the "minplay" option of book      *
 *           create to work properly.                                          *
 *                                                                             *
 *   15.0    This is the first version to support a parallel search using      *
 *           multiple processors on a shared-memory machinel.  This version    *
 *           uses the "PVS" algorithm (Principal Variation Search) that is an  *
 *           early form of "Young Brothers Wait".  This is not the final       *
 *           search algorithm that will be used, but it is relatively easy to  *
 *           implement, so that the massive data structure changes can be de-  *
 *           bugged before getting too fancy.  The major performance problem   *
 *           with this approach is that all processors work together at a node *
 *           and when one finishes, it has to wait until all others finish     *
 *           before it can start doing anything more.  This adds up to a       *
 *           significant amount of time and really prevents good speedups on   *
 *           multiprocessor machines.  This restriction will be eliminanted in *
 *           future versions, but it makes debugging much simpler for the      *
 *           first version.  After much agonizing, I finally chose to write my *
 *           own "spinlock" macros to avoid using the pthread_mutex_lock stuff *
 *           that is terribly inefficient due to its trying to be cute and     *
 *           yield the processor rather than spinning, when the lock is al-    *
 *           ready set.  i'd rather spin than context-switch.                  *
 *                                                                             *
 *   15.1    This version actually unleashes the full design of the parallel   *
 *           search, which lets threads split away from the "pack" when        *
 *           they become idle, and they may then jump in and help another      *
 *           busy thread.  This eliminates the significant delays that         *
 *           occur near the end of a search at a given ply.  Now as those pro- *
 *           cessors drop out of the search at that ply, they are reused at    *
 *           other locations in the tree without any significant delays.       *
 *                                                                             *
 *   15.2    WinNT support added.  Also fixed a small bug in Interrupt() that  *
 *           could let it exit with "busy" set so that it would ignore all     *
 *           input from that point forward, hanging the game up.  Added a new  *
 *           "draw accept" and "draw decline" option to enable/disable Crafty  *
 *           accepting draw offers if it is even or behind.  Turned on by      *
 *           an IM/GM opponent automatically.  Thread pools now being used to  *
 *           avoid the overhead of creating/destroying threads at a high rate  *
 *           rate of speed.  It is now quite easy to keep N processors busy.   *
 *                                                                             *
 *   15.3    CPU time accounting modified to correctly measure the actual time *
 *           spent in a parallel search, as well as to modify the way Crafty   *
 *           handles time management in an SMP environment.  A couple of bugs  *
 *           fixed in book.c, affecting which moves were played.  Also veri-   *
 *           fied that this version will play any ! move in books.bin, even if *
 *           it was not played in the games used to build the main book.bin    *
 *           file.                                                             *
 *                                                                             *
 *   15.4    This version will terminate threads once the root position is in  *
 *           a database, to avoid burning cpu cycles that are not useful.  A   *
 *           flag "done" is part of each thread block now.  When a thread goes *
 *           to select another move to search, and there are no more left, it  *
 *           sets "done" in the parent task thread block so that no other pro- *
 *           cessors will attempt to "join the party" at that block since no   *
 *           work remains to be done.  Book selection logic modified so that   *
 *           frequency data is "squared" to improve probabilities.  Also, the  *
 *           frequency data is used to cull moves, in that if a move was not   *
 *           played 1/10th of the number of times that the "most popular" move *
 *           was played, then that move is eliminated from consideration.      *
 *                                                                             *
 *   15.5    Changes to "volatile" variables to enhance compiler optimization  *
 *           in places where it is safe.  Added a function ThreadStop() that   *
 *           works better at stopping threads when a fail high condition       *
 *           occurs at any nodes.  It always stopped all sibling threads at    *
 *           the node in question, but failed to stop threads that might be    *
 *           working somewhere below the fail-high node.  ThreadStop() will    *
 *           recursively call itself to stop all of those threads. Significant *
 *           increase in king centralization (endgame) scoring to encourage    *
 *           the king to come out and fight rather than stay back and get      *
 *           squeezed to death.  Minor change to queen/king tropism to only    *
 *           conside the file distance, not ranks also.  Bug in interrupt.c    *
 *           fixed.  This bug would, on very rare occasion, let Crafty read in *
 *           a move, but another thread could read on top of this and lose the *
 *           move.  Xboard would then watch the game end with a <flag>.        *
 *                                                                             *
 *   15.6    Ugly repetition draw bug (SMP only) fixed.  I carefully copied    *
 *           the repetition list from parent to child, but also copied the     *
 *           repetition list pointer...  which remained pointing at the parent *
 *           repetition list.  Which failed, of course.                        *
 *                                                                             *
 *   15.7    Problem in passing PV's back up the search fixed.  The old bug of *
 *           a fail high/fail low was not completely handled, so that if the   *
 *           *last* move on a search failed high, then low, the PV could be    *
 *           broken or wrong, causing Crafty to make the wrong move, or else   *
 *           break something when the bogus PV was used.  Piece/square tables  *
 *           now only have the _w version initialized in data.c, to eliminate  *
 *           the second copy which often caused errors.  Initialize() now will *
 *           copy the _w tables to the _b tables (reflected of course.)        *
 *                                                                             *
 *   15.8    trade bonus modified to not kick in until one side is two pawns   *
 *           ahead or behind, so that trading toward a draw is not so common.  *
 *           Fixed a whisper bug that would cause Crafty to whisper nonsense   *
 *           if it left book, then got back in book.  It also now whispers the *
 *           book move it played, rather than the move it would play if the    *
 *           opponent played the expected book move, which was confusing.      *
 *                                                                             *
 *   15.9    Bug in positions with one legal move would abort the search after *
 *           a 4 ply search, as it should, but it was possible that the result *
 *           from the aborted search could be "kept" which would often make    *
 *           Crafty just "sit and wait" and flag on a chess server.            *
 *                                                                             *
 *   15.10   Fixed "draw" command to reject draw offers if opponent has < ten  *
 *           seconds left and no increment is being used.  Fixed glitch in     *
 *           LearnResult() that would crash Crafty if no book was being used,  *
 *           and it decides to resign.  Modified trade bonus code to eliminate *
 *           a hashing inconsistency.  Minor glitch in "show thinking" would   *
 *           show bad value if score was mate.  Added Tim Mann's new xboard/   *
 *           winboard "result" command and modified the output from Crafty to  *
 *           tell xboard when a game is over, using the new result format that *
 *           Tim now expects.  Note that this is xboard 3.6.9, and earlier     *
 *           versions will *not* work correctly (ok on servers, but not when   *
 *           you play yourself) as the game won't end correctly due to the     *
 *           change in game-end message format between the engine and xboard.  *
 *                                                                             *
 *   15.11   Modified SMP code to not start a parallel search at a node until  *
 *           "N" (N=2 at present) moves have been search.  N was 1, and when   *
 *           move ordering breaks down the size of the tree gets larger.  This *
 *           controls that much better.  Fixed an ugly learning bug.  If the   *
 *           last move actually in the game history was legal for the other    *
 *           side, after playing it for the correct side, it could be played   *
 *           a second time, which would break things after learning was done.  *
 *           An example was Rxd4 for white, where black can respond with Rxd4. *
 *           Changes for the new 3.6.10 xboard/winboard are also included as   *
 *           well as a fix to get book moves displayed in analysis window as   *
 *           it used to be done.                                               *
 *                                                                             *
 *   15.12   "The" SMP repetition bug was finally found and fixed.  CopyToSMP  *
 *           was setting the thread-specific rephead_* variable *wrong* which  *
 *           broke so many things it was not funny.  Evaluation tuning in this *
 *           version as well to try to "center" values in a range of -X to +X  *
 *           rather than 0 to 2X.  Trade bonus had a bug in the pawn part that *
 *           not only encouraged trading pawns when it should be doing the     *
 *           opposite, but the way it was written actually made it look bad to *
 *           win a pawn.  More minor xboard/winboard compatibility bugs fixed  *
 *           so that games end when they should.  DrawScore() had a quirk that *
 *           caused some problems, in that it didn't know what to do with non- *
 *           zero draw scores, and could return a value with the wrong sign at *
 *           times.  It is now passed a flag, "Crafty_is_white" to fix this.   *
 *           Modification so that Crafty can handle the "FEN" PGN tag and set  *
 *           the board to the correct position when it is found.  Xboard       *
 *           options "hard" and "easy" now enable and disable pondering.       *
 *                                                                             *
 *   15.13   Modification to "bad trade" code to avoid any sort of unbalanced  *
 *           trades, like knight/bishop for 2-3 pawns, or two pieces for a     *
 *           rook and pawn.  More new xboard/winboard fixes for Illegal move   *
 *           messages and other engine interface changes made recently.  Minor *
 *           change to null move search, to always search with the alpha/beta  *
 *           window of (beta-1,beta) rather than (alpha,beta), which is very   *
 *           slightly more efficient.                                          *
 *                                                                             *
 *   15.14   Rewrite of ReadPGN() to correctly handle nesting of various sorts *
 *           of comments.  This will now correctly parse wall.pgn, except for  *
 *           promotion moves with no promotion piece specified.  The book      *
 *           create code also now correctly reports when you run out of disk   *
 *           space and this should work under any system to let you know when  *
 *           you run out of space.                                             *
 *                                                                             *
 *   15.15   Major modifications to Book() and Bookup().  Bookup() now needs   *
 *           1/2 the temp space to build a book that it used to need (even     *
 *           less under windows).  Also, a book position is now 16 bytes in-   *
 *           stead of 20 (or 24 under windows) further reducing the space      *
 *           requirements.  The "win/lose/draw" counts are no longer absolute, *
 *           rather they are relative so they can be stored in one byte. A new *
 *           method for setting "!" moves in books.bin is included in this     *
 *           version.  For any move in the input file used to make books.bin,  *
 *           you can add a {play nn%} string, which says to play this move nn% *
 *           of the time, regardless of how frequently it was played in the    *
 *           big book file.  For example, e4 {play 50%} says to play e4 50% of *
 *           the time.  Note two things here:  (1) If the percentages for all  *
 *           of white's first moves add up to a number > 100, then the         *
 *           percentage played is taken as is, but all will be treated as if   *
 *           they sum to 100 (ie if there are three first moves, each with a   *
 *           percentage of 50%, they will behave as though they are all 33%    *
 *           moves).  (2) For any moves not marked with a % (assuming at least *
 *           one move *is* marked with a percent) then such moves will use the *
 *           remaining percentage left over applied to them equally. Note that *
 *           using the % *and* the ! flags together slightly changes things,   *
 *           because normally only the set of ! moves will be considered, and  *
 *           within that set, any with % specified will have that percent used *
 *           as expected.  But if you have some moves with !, then a percent   *
 *           on any of the *other* moves won't do a thing, because only the !  *
 *           moves will be included in the set to choose from.                 *
 *                                                                             *
 *   15.16   Bug in the king tropism code, particularly for white pieces that  *
 *           want to be close to the black king, fixed in this version.  New   *
 *           History() function that handles all history/killer cases, rather  *
 *           than the old 2-function approach.  Ugly Repeat() bug fixed in     *
 *           CopyToChild().  This was not copying the complete replist due to  *
 *           a ply>>2 rather than ply>>1 counter.                              *
 *                                                                             *
 *   15.17   Adjustments to "aggressiveness" to slightly tone it down.  Minor  *
 *           fix to avoid the odd case of whispering one move and playing an-  *
 *           other, caused by a fail-high/fail-low overwriting the pv[1] stuff *
 *           and then Iterate() printing that rather than pv[0] which is the   *
 *           correct one.  New LegalMove() function that strengthens the test  *
 *           for legality at the root, so that it is impossible to ponder a    *
 *           move that looks legal, but isn't.  Modifications to king safety   *
 *           to better handle half-open files around the king as well as open  *
 *           files.  EGTB "swindler" added.  If the position is drawn at the   *
 *           root of the tree, then all losing moves are excluded from the     *
 *           move list and the remainder are searched normally with the EGTB   *
 *           databases disabled, in an effort to give the opponent a chance to *
 *           go wrong and lose.  Fixed "mode tournament" and "book random 0"   *
 *           so that they will work correctly with xboard/winboard, and can    *
 *           even be used to play bullet games on a server if desired. Minor   *
 *           fix to attempt to work around a non-standard I/O problem on the   *
 *           Macintosh platform dealing with '\r' as a record terminator       *
 *           rather than the industry/POSIX-standard \n terminator character.  *
 *                                                                             *
 *   15.18   Fix to the "material balance" evaluation term so that it will not *
 *           like two pieces for a rook (plus pawn) nor will it like trading a *
 *           piece for multiple pawns.  A rather gross bug in the repetition   *
 *           code (dealing with 50 move rule) could overwrite random places in *
 *           memory when the 50 move rule approached, because the arrays were  *
 *           not long enough to hold 50 moves, plus allow the search to go     *
 *           beyond the 50-move rule (which is legal and optional) and then    *
 *           output moves (which uses ply=65 data structures).  This let the   *
 *           repetition (50-move-rule part) overwrite things by storing well   *
 *           beyond the end of the repetition list for either side.  Minor fix *
 *           to EGTB "swindle" code.  It now only tries the searches if it is  *
 *           the side with swindling chances.  Ie it is pointless to try for   *
 *           a swindle in KRP vs KR if you don't have the P.  :)  Some very    *
 *           substantial changes to evaluate.c to get things back into some    *
 *           form of synch with each other.                                    *
 *                                                                             *
 *   15.19   More evaluation changes.  Slight change to the Lock() facility    *
 *           as it is used to lock the hash table.  Modifications to the book  *
 *           selection logic so that any move in books.bin that is not in the  *
 *           regular book.bin file still can be played, and will be played if  *
 *           it has a {play xxx%} or ! option.  New option added to the book   *
 *           create command to allow the creation of a better book.  This      *
 *           option lets you specify a percentage that says "exclude a book    *
 *           move it if didn't win xx% as many games as it lost.  IE, if the   *
 *           book move has 100 losses, and you use 50% for the new option, the *
 *           move will only be included if there are at least 50 wins.  This   *
 *           allows culling lines that only lost, for example.  New bad trade  *
 *           scoring that is simpler.  If one side has one extra minor piece   *
 *           and major pieces are equal, the other side has traded a minor for *
 *           pawns and made a mistake.  If major pieces are not matched, and   *
 *           one side has two or more extra minors, we have seen either a rook *
 *           for two minor pieces (bad) or a queen for three minor pieces (bad *
 *           also).  Horrible bug in initializing min_thread_depth, which is   *
 *           supposed to control thrashing.  The SMP search won't split the    *
 *           tree within "min_thread_depth" of the leaf positions.  Due to a   *
 *           gross bug, however, this was initialized to "2", and would have   *
 *           been the right value except that a ply in Crafty is 4.  The       *
 *           mtmin command that adjusts this correctly multiplied it by 4,     *
 *           but in data.c, it was left at "2" which lets the search split way *
 *           too deeply and can cause thrashing.  It now correctly defaults to *
 *           120 (2*PLY) as planned.  Crafty now *only* supports               *
 *           winboard/xboard 4.0 or higher, by sending the string "move xxx"   *
 *           to indicate its move.  This was done to eliminate older xboard    *
 *           versions that had some incompatibilities with Crafty that were    *
 *           causing lots of questions/problems.  Xboard 4.0 works perfectly   *
 *           and is the only recommended GUI now.                              *
 *                                                                             *
 *   15.20   New evaluation term to ensure that pawns are put on squares of    *
 *           the opposite color as the bishop in endings where one side only   *
 *           has a single bishop.  Another new evaluation term to favor        *
 *           bishops over knights in endings with pawns on both sides of the   *
 *           board.  Search extensions now constrained so that when doing an   *
 *           N-ply search, the first 2N plies can extend normally, but beyond  *
 *           that the extensions are reduced by one-half.  This eliminates a   *
 *           few tree explosions that wasted lots of time but didn't produce   *
 *           any useful results.  This version solves 299/300 of the Win at    *
 *           Chess positions on a single-cpu pentium pro 200mhz machine now.   *
 *           New evaluation term to reward "pawn duos" (two pawns side by side *
 *           which gives them the most flexibility in advancing.  A change to  *
 *           the bishops of opposite color to not just consider B vs B drawish *
 *           when the B's are on opposite colors, but to also consider other   *
 *           positons like RB vs RB drawish with opposite B's.                 *
 *                                                                             *
 *   16.0    Hash functions renamed to ProbeTransRef() and StoreTransRef().    *
 *           The store functions (2) were combined to eliminate some duplicate *
 *           code and shrink the cache footprint a bit.  Adjustment to "losing *
 *           the right to castle" so that the penalty is much less if this is  *
 *           done when trading queens, since a king in the center becomes less *
 *           of a problem there. Several eval tweaks.  Support for Eugene      *
 *           Nalimov's new endgame databases that reduce the size by 50%.      *
 *           Added a new cache=xxx command to set the tablebase cache (more    *
 *           memory means less I/O of course).  The "egtb" command now has two *
 *           uses.  If you enter egtb=0, you can disable tablebases for        *
 *           testing.  Otherwise, you just add "egtb" to your command line or  *
 *           .craftyrc file and Crafty automatically recognizes the files that *
 *           are present and sets up to probe them properly.  Added an eval    *
 *           term to catch a closed position with pawns at e4/d3/c4 and then   *
 *           avoiding the temptation to castle into an attack.  Most of the    *
 *           evaluation discontinuities have been removed, using the two new   *
 *           macros ScaleToMaterial() and InverseScaleToMaterial().  Scale()   *
 *           is used to reduce scores as material is removed from the board,   *
 *           while InverseScale() is used to increase the score as material    *
 *           is removed.  This removed a few ugly effects of things happening  *
 *           right around the EG_MAT threshold.                                *
 *                                                                             *
 *   16.1    Bug in EGTBProbe() which used the wrong variable to access the    *
 *           enpassant target square was fixed.  The cache=x command now       *
 *           checks for a failed malloc() and reverts to the default cache     *
 *           size to avoid crashes.  Major eval tuning changes.  Cache bug     *
 *           would do an extra malloc() and if it is done after the "egtb"     *
 *           command it would crash.  Analyze mode now turns off the "swindle" *
 *           mode.                                                             *
 *                                                                             *
 *   16.2    More evaluation changes, specifically to bring king safety down   *
 *           to reasonable levels.  Support for the DGT electronic chess board *
 *           is now included (but only works in text mode, not in conjunction  *
 *           with xboard/winboard yet.  Fix to StoreTransRef() that corrected  *
 *           a problem with moving an entry from tablea to tableb, but to the  *
 *           wrong address 50% of the time (thanks to J. Wesley Cleveland).    *
 *                                                                             *
 *   16.3    Performance tweaks plus a few evaluation changes.  An oversight   *
 *           would let Crafty play a move like exd5 when cxd5 was playable.    *
 *           IE it didn't follow the general rule "capture toward the center." *
 *           this has been fixed.  King safety ramped up just a bit.           *
 *                                                                             *
 *   16.4    Search extension fixed.  One-reply was being triggered when there *
 *           was only one capture to look at as well, which was wrong.  Minor  *
 *           fix to king safety in Evaluate().  Adjustments to preeval to fix  *
 *           some "space" problems caused by not wanting to advance pawns at   *
 *           all, plus some piece/square pawn values that were too large near  *
 *           the opponent's king position.  New swindle on*off command allows  *
 *           the user to disable "swindle mode" so that Crafty will report     *
 *           tablebase draws as draws.  Swindle mode is now disabled auto-     *
 *           matically in analysis or annotate modes.  DrawScore() now returns *
 *           a draw score that is not nearly so negative when playing humans.  *
 *           The score was a bit large (-.66 and -.33) so that this could tend *
 *           to force Crafty into losing rather than drawn positions at times. *
 *           EGTB probe code now monitors how the probes are affecting the nps *
 *           and modifies the probe depth to control this.  IE it adjusts the  *
 *           max depth of probes to attempt to keep the NPS at around 50% or   *
 *           thereabouts of the nominal NPS rate.  Fixed a nasty bug in the    *
 *           EGTB 'throttle' code.  It was possible that the EGTB_mindepth     *
 *           could get set to something more than 4 plies, then the next       *
 *           search starts off already in a tablebase ending, but with this    *
 *           probe limit, it wouldn't probe yet Iterate() would terminate the  *
 *           search after a couple of plies.  And the move chosen could draw.  *
 *           New code submitted by George Barrett, which adds a new command    *
 *           "html on".  Using this will cause the annotate command to produce *
 *           a game.html file (rather than game.can) which includes nice board *
 *           displays (bitmapped images) everywhere Crafty suggests a          *
 *           different move.  You will need to download the bitmaps directory  *
 *           files to make this work.  Note that html on enables this or you   *
 *           will get normal annotation output in the .can file.  Final        *
 *           hashing changes made.  Now crafty has only two hash tables, the   *
 *           'depth preferred' and 'always store' tables, but not one for each *
 *           side.                                                             *
 *                                                                             *
 *   16.5    Minor glitch in internal iterative deepening code (in search.c)   *
 *           could sometimes produce an invalid move.  Crafty now probes the   *
 *           hash table to find the best move which works in all cases.        *
 *           Interesting tablebase bug fixed.  By probing at ply=2, the        *
 *           search could overlook a repetition/50 move draw, and turn a won   *
 *           position into a draw.  Crafty now only probes at ply > 2, to      *
 *           let the Repeat() function have a chance.  Added code by Dan       *
 *           Corbitt to add environment variables for the tablebases, books,   *
 *           logfiles and the .craftyrc file paths.  The environment variables *
 *           CRAFTY_TB_PATH, CRAFTY_BOOK_PATH, CRAFTY_LOG_PATH and             *
 *           CRAFTY_RC_PATH should point to where these files are located and  *
 *           offer an alternative to specifying them on the command line.      *
 *           Final version of the EGTB code is included in this version.  This *
 *           allows Crafty to use either compressed (using Andrew Kadatch's    *
 *           compressor, _not_ zip or gzip) or non-compressed tablebases.  It  *
 *           will use non-compressed files when available, and will also use   *
 *           compressed files that are available so long as there is no non-   *
 *           compressed version of the same file.  It is allowable to mix and  *
 *           match as you see fit, but with all of the files compressed, the   *
 *           savings is tremendous, roughly a factor of 5 in space reduction.  *
 *                                                                             *
 *   16.6    Major rewrite of king-safety pawn shelter code to emphasize open  *
 *           files (and half-open files) moreso than just pawns that have been *
 *           moved.  Dynamic EGTB probe depth removed.  This caused far too    *
 *           many search inconsistencies.  A fairly serious SMP bug that could *
 *           produce a few hash glitches was found.  Rewrite of the 'king-     *
 *           tropism' code completed.  Now the pieces that are 'crowding' the  *
 *           king get a bigger bonus if there are more of them.  Ie it is more *
 *           than twice as good if we have two pieces close to the king, while *
 *           before this was purely 'linear'.  Minor fix to book.c to cure an  *
 *           annotate command bug that would produce odd output in multi-game  *
 *           PGN files.                                                        *
 *                                                                             *
 *   16.7    Minor bug in 'book create' command would produce wrong line num-  *
 *           bers if you used two book create commands without restarting      *
 *           Crafty between uses.  Replaced "ls" command with a !cmd shell     *
 *           escape to allow _any_ command to be executed by a shell. Change   *
 *           to bishop scoring to avoid a bonus for a bishop pair _and_ a good *
 *           bishop sitting at g2/g7/etc to fill a pawn hole.  If the bishop   *
 *           fills that hole, the bishop pair score is reduced by 1/2 to avoid *
 *           scores getting too large.  Another ugly SMP bug fixed.  It was    *
 *           for the CopyFromSMP() function to fail because Search() put the   *
 *           _wrong_ value into tree->value.  The "don't castle into an unsafe *
 *           king position" code was scaled up a bit as it wasn't quite up to  *
 *           detecting all cases of this problem.  The parallel search has     *
 *           been modified so that it can now split the tree at the root, as   *
 *           well as at deeper nodes.  This has improved the parallel search   *
 *           efficiency noticably while also making the code a bit more        *
 *           complex.  This uses an an adaptive algorithm that pays close      *
 *           attention to the node count for each root move.  If one (or more) *
 *           moves below the best move have high node counts (indicating that  *
 *           these moves might become new 'best' moves) then these moves are   *
 *           flagged as "don't search in parallel" so that all such moves are  *
 *           searched one by one, using all processors so that a new best move *
 *           if found faster.  Positional scores were scaled back 50% and the  *
 *           king safety code was again modified to incorporate pawn storms,   *
 *           which was intentionally ignored in the last major revision.  Many *
 *           other scoring terms have been modified as well to attempt to      *
 *           bring the scores back closer to something reasonable.             *
 *                                                                             *
 *   16.8    Bug in evaluate.c (improper editing) broke the scoring for white  *
 *           rooks on open files (among other things.)  New twist on null-     *
 *           move search...  Crafty now uses R=3 near the root of the tree,    *
 *           and R=2 near the top.  It also uses null-move so long as there is *
 *           a single piece on the board, but when material drops to a rook or *
 *           less, null move is turned off and only used in the last 7 plies   *
 *           to push the zugzwang errors far enough off that they can be       *
 *           avoided, hopefully.                                               *
 *                                                                             *
 *   16.9    Tuning to king safety.  King in center of board was way too high  *
 *           which made the bonus for castling way too big.  More fine-tuning  *
 *           on the new 'tropism' code (that tries to coordinate piece attacks *
 *           on the king).  It understands closeness and (now) open files for  *
 *           the rooks/queens.  Modified blocked center pawn code so that an   *
 *           'outpost' in front of an unmoved D/E pawn is made even stronger   *
 *           since it restrains that pawn and cramps the opponent.             *
 *                                                                             *
 *   16.10   All scores are back to 16.6 values except for king safety, as     *
 *           these values played well.  King safety is quite a bit different   *
 *           and now has two fine-tuning paramaters that can be set in the     *
 *           crafty.rc file (try help ksafety for more details, or see the new *
 *           crafty.doc for more details.  Suffice it to say that you can now  *
 *           tune it to be aggressive or passive as you wish.                  *
 *                                                                             *
 *   16.11   Adjustment to king safety asymmetry to make Crafty a bit more     *
 *           cautious about starting a king-side attack.  New "ksafe tropism"  *
 *           command allows the tropism scores (pulling pieces toward opponent *
 *           king) to be adjusted up or down (up is more aggressive).          *
 *                                                                             *
 *   16.12   evaluation adjustment options changed.  The new command to modify *
 *           this stuff is "evaluation option value".  For example, to change  *
 *           the asymmetry scoring for king safety, you now use "evaluation    *
 *           asymmetry 120".  See "help evaluation" for more details.  There   *
 *           is a new "bscale" option to adjust the scores for 'blocked' type  *
 *           positions, and I added a new "blockers_list" so that players that *
 *           continually try to lock things up to get easy draws can be better *
 *           handled automatically.  The command is "list B +name".  There are *
 *           other adjustable evaluation scaling parameters now (again, use    *
 *           "help evaluation" to see the options.)                            *
 *                                                                             *
 *   16.13   Bug in Evaluate() fixed.  King safety could produce impossibly    *
 *           large scores due to negative subscript values.  Minor change to   *
 *           root.c to allow partial tablebase sets while avoiding the ugly    *
 *           potential draw problem.  This was supplied by Tim Hoooebeek and   *
 *           seems to work ok for those wanting to have files like kpk with-   *
 *           out the corresponding promotion files.                            *
 *                                                                             *
 *   16.14   Bug in Evaluate() fixed.  King safety could produce impossibly    *
 *           large scores due to a subscript two (2) larger than expected for  *
 *           max values.  A couple of other minor tweaks to rook on 7th as     *
 *           well.  Setboard now validates enpassant target square to be sure  *
 *           that an enpassant capture is physically possible, and that the    *
 *           target square is on the right rank with pawns in the right place. *
 *                                                                             *
 *   16.15   Bug in EGTBProbe() usage fixed.  It was possible to call this     *
 *           function before tablebases were initialized, or when there were   *
 *           more than 3 pieces on one side, which would break a subscript.    *
 *                                                                             *
 *   16.16   Changes to the way EvaluatePassedPawn() works.  It now does a     *
 *           reasonable job of penalizing blockaded passed pawns.  Also a bug  *
 *           in king scoring used a variable set from the previous call to the *
 *           Evaluate() procedure, which could produce bizarre effects.  A few *
 *           minor eval glitches that caused asymmetric scores unintentionally *
 *           were removed.                                                     *
 *                                                                             *
 *   16.17   Minor "hole" in SMP code fixed.  It was possible that a move      *
 *           displayed as the best in Output() would not get played.           *
 *           Very subtle timing issue dealing with the search timing out and   *
 *           forgetting to copy the best move back to the right data area.     *
 *           Minor fixes to the modified -USE_ATTACK_FUNCTIONS option that     *
 *           was broken when the And()/Or()/ShiftX() macros were replaced by   *
 *           their direct bitwise operator replacements.                       *
 *                                                                             *
 *   16.18   Adjustments to king tropism scoring terms (these attract pieces   *
 *           toward the opponent king in a coordinated way).  The scoring term *
 *           that kept the queen from going to the opposite side of the board  *
 *           was not working well which would also cause attacking problems.   *
 *           Razoring code has been disabled, as it neither helps or hurts on  *
 *           tactical tests, and it creates a few hashing problems that are    *
 *           annoying.  Bug in StoreTransRefPV() that would store the PV into  *
 *           the hash table, but possibly in the wrong table entry, which      *
 *           could stuff a bad move in a hash entry, or overwrite a key entry  *
 *           that could have been useful later.  Book random 0 was slightly    *
 *           wrong as it could not ignore book moves if the search said that   *
 *           the score was bad.  Also the search would not time-out properly   *
 *           on a forced move, it would use the entire time alotted which was  *
 *           a big waste of time when there was only one legal move to make.   *
 *           A very interesting bug was found in storing mate bounds in the    *
 *           hash table, one I had not heard of before.  Another change to     *
 *           Iterate() to make it avoid exiting as soon as a mate is found, so *
 *           that it can find the shortest mate possible.                      *
 *                                                                             *
 *   16.19   Savepos now pads each rank to 8 characters so that programs that  *
 *           don't handle abbreviated FEN will be able to parse Crafty's FEN   *
 *           output without problems.  Threads are no longer started and       *
 *           stopped before after searches that seem to not need multiple      *
 *           threads (such as when the starting position is in the databases   *
 *           or whatever) because in fast games, it can start and stop them    *
 *           after each search, due to puzzling for a ponder move, and this is *
 *           simply to inefficient to deal with, and burning the extra cpus is *
 *           only bad if the machine is used for other purposes.  And when the *
 *           machine is not dedicated to chess only, the SMP search is very    *
 *           poor anyway.  New lockless hashing (written by Tim Mann) added    *
 *           to improve SMP performance.  New eval term that handles various   *
 *           types of candidate passed pawns, most commonly the result of a    *
 *           position where one side has an offside majority that later turns  *
 *           into an outside passed pawn.  New EGTBPV() procedure added.  This *
 *           code will print the entire PV of a tablebase mating line.  If it  *
 *           given an option of ! (command is "egtb !") it will add a ! to any *
 *           move where there is only one optimal choice.  Crafty will now     *
 *           automatically display the EGTB PV if you give it a setboard FEN   *
 *           command, or if you just give it a raw FEN string (the setboard    *
 *           command is now optional except in test suites).  This version now *
 *           supports the 6 piece database files.  To include this support,    *
 *           you need to -DEGTB6 when compiling everything.  The majority code *
 *           has been temporarily removed so that this version can be released *
 *           to get the 6 piece ending stuff out.                              *
 *                                                                             *
 *   17.0    Connected passed pawn scoring modified so that connected passers  *
 *           don't get any sort of bonus, except for those cases where the     *
 *           opponent has a rook or more.  This will avoid liking positions    *
 *           where Crafty has two connected passers on the d/e files, and the  *
 *           opponent has passers on the b/g files.  The split passers win if  *
 *           all pieces are traded, while the connected passers are better     *
 *           when pieces are present.  A new scoring term evaluates the        *
 *           "split" passers similar to outside passers, this at the request   *
 *           (make that demand) of GM Roman Dzindzichashvili. Book() now finds *
 *           the most popular move as the move to ponder.  This eliminated all *
 *           'puzzling' searches which result in clearing the hash tables two  *
 *           times for every book move, which can slow it down when a large    *
 *           hash table is used.  New book option for playing against computer *
 *           opponents was written for this version.  Crafty now supports two  *
 *           "books.bin" type files, books.bin and bookc.bin (the bookc create *
 *           command can be used to make bookc.bin).  Bookc will only be used  *
 *           when Crafty is playing a computer.  This is supplied by xboard or *
 *           winboard when playing on a server, or can be included int the     *
 *           crafty.rc/.craftyrc file when playing directly.  Bookc.bin will   *
 *           be used when playing a computer only, and should be used to avoid *
 *           unsound lines that are fine against humans, but not against other *
 *           computers.  If you don't use a bookc.bin, it will use the normal  *
 *           books.bin as always.  If you use both, it will only use bookc.bin *
 *           in games that have been started and then given the 'computer'     *
 *           command.  Minor adjustment to EGTB probe code so that it will     *
 *           always probe TBs at ply=2 (if the right number of pieces are on   *
 *           the board) but won't probe beyond ply=2 unless the move at the    *
 *           previous ply was a capture/promotion and the total pieces drops   *
 *           into the proper range for TB probes.  This makes the 6 piece      *
 *           files far more efficient as before it would continuously probe    *
 *           after reaching 6 piece endings, even if it didn't have the right  *
 *           file.  now after the capture that takes us to a 6 piece ending    *
 *           we probe one time...  and if there is no hit we don't probe at    *
 *           the next node (or any successors) unless there is another capture *
 *           or promotion that might take us to a database we have.  The book  *
 *           (binary) format has once again been modified, meaning that to use *
 *           16.20 and beyond the book.bin/books.bin files must be re-built    *
 *           from the raw PGN input.  This new format includes an entry for    *
 *           the CAP project score, so that deep searches can be used to guide *
 *           opening book lines.  A new weight (bookw CAP) controls how much   *
 *           this affects book move ordering/selection.  A new import command  *
 *           will take the raw CAPS data and merge it into the book.bin after  *
 *           the file has been built.  Serious bug in SetBoard() would leave   *
 *           old EP status set in test suites.  This has been fixed.  Outpost  *
 *           knight code was modified so that a knight in a hole is good, a    *
 *           knight in a hole supported by a pawn is better, supported by two  *
 *           pawns is still better, and if the opponent has no knights or a    *
 *           bishop that can attack this knight it is even better.  The out-   *
 *           post bonus for a bishop was removed.  A pretty serious parallel   *
 *           search bug surfaced.  In non-parallel searches, Crafty _always_   *
 *           completed the current ply=1 move after the time limit is reached, *
 *           just to be sure that this move isn't going to become a new best   *
 *           move.  However, when parallel searching, this was broken, and at  *
 *           the moment time runs out, the search would be stopped if the      *
 *           parallel split was done at the root of the tree, which means that *
 *           where the search would normally find a new best move, it would    *
 *           not.  This has been fixed.  New "store <val>" command can be used *
 *           to make "position learning" remember the current position and the *
 *           score (val) you supply.  This is useful in analyze mode to move   *
 *           into the game, then let Crafty know that the current position is  *
 *           either lost or won (with a specific score).                       *
 *                                                                             *
 *   17.1    Book structure fixed, since compilers can't quite agree on how    *
 *           structures ought to be aligned, for reasons not clear to me.  A   *
 *           serious eval bug that could produce gross scores when one side    *
 *           had two queens was fixed.                                         *
 *                                                                             *
 *   17.2    Isolated pawn scoring tweaked a bit, plus a couple of bugs in the *
 *           way EvaluateDevelopment() was called were fixed.                  *
 *                                                                             *
 *   17.3    Passed pawn scores increased somewhat to improve endgame play.    *
 *                                                                             *
 *   17.4    Minor bug with "black" command (extra line from unknown origin)   *
 *           would make "black" command fail to do anything.  Minor tweaks to  *
 *           passed pawn scoring again...  And a slight performance improve-   *
 *           ment in how EvaluateKingSafety() is called.  Code for "bk"        *
 *           from xboard was somehow lost.  It now provides a book hint.       *
 *                                                                             *
 *   17.5    Rewrite of outside passed pawn/pawn majority code.  It is now     *
 *           much faster by using pre-computed bitmaps to recognize the right  *
 *           patterns.                                                         *
 *                                                                             *
 *   17.6    Minor fix in interrupt.c, which screwed up handling some commands *
 *           while pondering.  Minor fix to score for weak back rank.  Score   *
 *           was in units of 'defects' but should have been in units of        *
 *           "centipawns".   Minor bug in "drawn.c" could mis-classify some    *
 *           positions as drawn when they were not.                            *
 *                                                                             *
 *   17.7    Repair to DrawScore() logic to stop the occasional backward sign  *
 *           that could create some funny-looking scores (-20 and +20 for      *
 *           example).  Minor fix to majority code to recognize the advantage  *
 *           of having a majority when the opponent has no passers or majority *
 *           even if the candidate in the majority is not an 'outside' passer  *
 *           candidate.  Minor change to passed pawns so that a protected      *
 *           passed pawn is not considered a winning advantage if the          *
 *           opponent has two or more passed pawns.  But in input_status       *
 *           would cause an infinite loop if you reset a game to a position    *
 *           that was in book, after searching a position that was not in      *
 *           the book.  Bug in position learning fixed also.  This was caused  *
 *           by the new hashing scheme Tim Mann introduced to avoid locking    *
 *           the hash table.  I completed the changes he suggested, but forgot *
 *           about how it might affect the position learning since it is also  *
 *           hash-based.  Needless to say, I broke it quite nicely, thank you. *
 *                                                                             *
 *   17.8    This is the version used in the first ICC computer chess          *
 *           tournament.  Crafty won with a score of 7.0/8.0 which included    *
 *           only 2 draws and no losses.  Changes are minimal except for a few *
 *           non-engine syntax changes to eliminate warnings and fix a bad bug *
 *           in 'bench.c' that would crash if there was no books.bin file.     *
 *                                                                             *
 *   17.9    LearnPosition() called with wrong arguments from main() which     *
 *           effectively disabled position learning.  This was broken in 17.7  *
 *           but is now fixed.                                                 *
 *                                                                             *
 *   17.10   Minor change to "threat" extension now only extends if the null-  *
 *           move search returns "mated in 1" and not "mated in N".  This      *
 *           tends to shrink the trees a bit with no noticable effect in       *
 *           tactical strength.  EvaluatePawns() was not doing a reasonable    *
 *           job of recognizing blocked pawns and candidate passers.  It did   *
 *           not do well in recognizing that the pawn supporting a candidate   *
 *           could not advance far enough to help make a real passed pawn.     *
 *           Minor change to Repeat() to not count two-fold repeats as draws   *
 *           in the first two plies, which prevents some odd-looking repeats   *
 *           at the expense of a little inefficiency.  Ugly repetition bug     *
 *           fixed.  Rephead was off by one for whatever side Crafty was       *
 *           playing which would screw up repetition detection in some cases.  *
 *           Minor bug in main() that would report stalemate on some mates     *
 *           when the ponder score was forgotten.                              *
 *                                                                             *
 *   17.11   Fail high/low logic changed to move the ++ output into Iterate()  *
 *           where the -- output was already located.  (This was done for      *
 *           consistecy only, it was not a problem.  New function to detect    *
 *           draws RepetitionCheckBook() was added to count two-fold repeats   *
 *           as draws while in book.  This lets Crafty search to see if it     *
 *           wants tht draw or not, rather than just repeating blindly.  Minor *
 *           bug in "go"/"move" command fixed.  The command did not work if    *
 *           the engine was pondering.  Minor change to Evaluate() that makes  *
 *           BAD_TRADE code more acurate.  Minor change to pawn majority code  *
 *           to fix two bugs.  White pawns on g2/h2 with black pawn on h7 was  *
 *           not seen as a majority (outside candidate passer) but moving the  *
 *           black pawn to g7 made it work fine.   Also if both sides had an   *
 *           equal number of pawns (black pawn at a7/b7, white pawns at b2/c2  *
 *           the code would not notice black had an outside passer candidate   *
 *           since pawns were 'equal' in number.                               *
 *                                                                             *
 *   17.12   Bookup() will now create a book with all sorts of games in it, so *
 *           long as the various games have the FEN string to properly set the *
 *           initial chess board position.                                     *
 *                                                                             *
 *   17.13   Endgame evaluation problem fixed.  King scoring now dynamically   *
 *           chooses the right piece/square table depending on which side of   *
 *           the board has pawns, rather than using root pre-processing which  *
 *           could make some gross errors.  Glitch with majorities would make  *
 *           an "inside majority candidate" offset a real outside passed pawn  *
 *           even if the candidate would not be outside.  A suggestion by      *
 *           Blass Uri (CCC) was added.  This adds .01 to tablebase scores if  *
 *           the side on move is ahead in material, and -.01 if the side on    *
 *           move is behind in material (only for drawn positions.)  the       *
 *           intent is to favor positions where you are ahead in material      *
 *           rather than even in material, which gives 'swindle mode' a chance *
 *           to work.  Bug in EvaluateDraws() would mis-categorize some B+RP   *
 *           (wrong bishop) as drawn, even if the losing king could not make   *
 *           it to the corner square in time.                                  *
 *                                                                             *
 *   17.14   Another endgame evaluation problem fixed.  The outside passed     *
 *           pawn code worked well, up until the point the pawn had to be      *
 *           given up to decoy the other side's king away from the remainder   *
 *           of the pawns.  Crafty now understands the king being closer to    *
 *           the pawns than the enemy king, and therefore transitions from     *
 *           outside passer to won king-pawn ending much cleaner.  New command *
 *           "selective" as requested by S. Lim, which allows the user to      *
 *           set the min/max null move R values (default=2/3).  They can be    *
 *           set to 0 which disables null-move totally, or they can be set     *
 *           larger than the default for testing.  Minor changes to init.c     *
 *           sent by Eugene Nalimov to handle 64 bit pointer declarations for  *
 *           win64 executable compilation.  NetBSD changes included along with *
 *           a new Makefile that requires no editing to use for any known      *
 *           configuration ("make help" will explain how to use it).  This was *
 *           submitted by Johnny Lam.  Serious changes to the outside passed   *
 *           pawn code.  The evaluator now understands that outside passers    *
 *           on _both_ sides of the board is basically winning.  Same goes for *
 *           candidate passers.                                                *
 *                                                                             *
 *   18.0    Bug in EvaluateDraws() could incorrectly decide that if one side  *
 *           had only rook pawns in a king/pawn ending, the ending was a draw. *
 *           Even if the other side had pawns all over the board.  Passed pawn *
 *           scores increased a bit.  Minor change to "bad trade" code to turn *
 *           off in endings.  S list removed.  The trojan horse check is now   *
 *           automatically enabled when the key position is recognized at the  *
 *           root of the tree.  It is disabled once the key position is no     *
 *           longer seen at the root.  Bug in Bookup() would break if a FEN    *
 *           string was included in a game used for input.  SetBoard() was     *
 *           setting castling status incorrectly.  Internal iterative          *
 *           deepening bug could (on very rare occasions) cause Crafty to try  *
 *           to search an illegal move that would bomb the search.  This       *
 *           version fully supports the new xboard protocol version 2.  One    *
 *           other major change is that the eval is now consistently displayed *
 *           as +=good for white, -=good for black.  In the log file.  In the  *
 *           console output window.  And when whispering/kibitzing on a chess  *
 *           server.  This should eliminate _all_ confusion about the values   *
 *           that are displayed as there are now no exceptions to the above    *
 *           policy of any kind.  Book() was changed so that if it notices     *
 *           that the opponent has a "stonewall pattern" set up (IE Crafty     *
 *           is black, has a pawn at e7 or e6, and the opponent has pawns at   *
 *           d4/f4) then it won't play a castle move from book.  It will do a  *
 *           normal search instead and let the evaluation guide it on whether  *
 *           to castle or not (generally it won't unless it is forced).        *
 *                                                                             *
 *   18.1    Easy move code broken, so that no move ever appeared to be "easy" *
 *           even if it was the only possible recapture move to make.  A minor *
 *           problem in analysis mode dealing with pawn promotions was fixed.  *
 *           It was possible for a move like h8=Q to be rejected even though   *
 *           it was perfectly legal.                                           *
 *                                                                             *
 *   18.2    Minor bug in analyze mode would break the "h" command when black  *
 *           was on move, and show one less move for either side that had      *
 *           actually been played in the game.  Another bug also reversed the  *
 *           sign of a score whispered in analysis mode.  Recapture extension  *
 *           is disabled by default.  To turn it on, it is necessary to use    *
 *           -DRECAPTURE when compiling search.c and option.c.  LearnBook()    *
 *           has been modified to 'speed up' learning.  See the comments in    *
 *           that module to see how it was changed.  LearnResult() has been    *
 *           removed.  LearnBook() is now used to do the same thing, except    *
 *           that a good (or bad) score is used.                               *
 *                                                                             *
 *   18.3    Minor bug in "avoid_null_move" test used R=2 for the test rather  *
 *           than testing R=2/3 as the real null-move search uses.  The kibitz *
 *           for "Hello from Crafty Vx.xx" has been moved so that it works     *
 *           with the new xboard/winboard 4.2.2 versions.  Book learning was   *
 *           badly broken in the previous version and has been fixed/tested.   *
 *                                                                             *
 *   18.4    Recapture extension was left in SearchParallel() by mistake.      *
 *           This has now been protected by a #ifdef just like it was in       *
 *           Search().  Bug in Repeat() was causing problems in SMP versions.  *
 *           The entire repetition list code was modified to clean this up.    *
 *           The problem was most noticable on things like fine #70.  Bug in   *
 *           LearnImportBook() confused the learn value sign, due to the other *
 *           changes to make +=white all the time.  Opposite bishop scoring    *
 *           has been beefed up a bit to avoid these drawish endings.          *
 *                                                                             *
 *   18.5    Minor change to RootMove() to use Quiesce() rather than the more  *
 *           complicated way it was ordering with Evaluate()/EnPrise().  This  *
 *           is no faster, but it is simpler and eliminated the need for the   *
 *           EnPrise() function totally, making the code a bit smaller.  Bug   *
 *           in EvaluateDraws() would let it think that the bishop+wrong rook  *
 *           pawn endings were winnable if both kings were very close to the   *
 *           queening square, even with the wrong bishop.                      *
 *                                                                             *
 *   18.6    "New" no longer produces a new log.nnn/game.nnn file if no moves  *
 *           have actually been played.  Minor change to rook scoring gives a  *
 *           penalty when a rook has no horizontal (rank) mobility, to avoid   *
 *           moves like Ra2 protecting the pawn on b2, etc.  Glitch in the     *
 *           code that initializes is_outside[][] and is_outside_c[][] could   *
 *           cause missed outside pawn cases to happen.  This has been there   *
 *           a long time.                                                      *
 *                                                                             *
 *   18.7    BOOK_CLUSTER_SIZE increased to 2000 to handle making really large *
 *           books.  A book made without this change could produce clusters    *
 *           that would cause memory overwrites.                               *
 *                                                                             *
 *   18.8    Recapture extension turned back on for a while.  Changes to the   *
 *           evaluation code, particularly EvaluatePawns() to make it more     *
 *           efficient and accurate.  IE it was possible for an isolated pawn  *
 *           to be penalized for being isolated, weak, and blocked, which made *
 *           little sense.                                                     *
 *                                                                             *
 *   18.9    Book() modified to increase the responsiveness of book learning.  *
 *           The new code, plus the default weights for the book parameters    *
 *           now make Crafty learn very aggressively and repeat good opening   *
 *           lines and avoid bad ones.                                         *
 *                                                                             *
 *   18.10   Minor bug in book.c would let Crafty play lines that were very    *
 *           rarely played even though there were others that had been played  *
 *           far more times and were more reliable.  King safety scores ramped *
 *           up a bit and made more "responsive".                              *
 *                                                                             *
 *   18.11   Minor bug in EvaluateDevelopment() found by Bruce Moreland.  The  *
 *           pawn ram code is now disabled when playing a computer, although   *
 *           the normal 'blocked pawn' code is always active.  Bug in the code *
 *           that penalizes a rook with no horizontal mobility was fixed.  If  *
 *           the first rook scored had horizontal mobility, the second rook    *
 *           appeared to have this mobility as well, which was wrong.  Pawn    *
 *           hash statistics were wrong on longer searches due to an int       *
 *           overflow on a multiply and divide calculation.  This has been     *
 *           re-ordered to avoid the overflow.  For unknown reasons, epd       *
 *           support was disabled.  It is now enabled as it should be.  Some   *
 *           strange adjustements to root_alpha/root_beta were removed from    *
 *           searchr.c.  They were probably left-over from some sort of test.  *
 *                                                                             *
 *   18.12   Bug in EvaluateDraws() fixed to not call KBB vs KN a draw if the  *
 *           correct tablebase is not available.  Bishop pair scores now vary  *
 *           depending on how many pawns are left on the board.  A pair is not *
 *           worth a lot if there are 7-8 pawns left as most diagonals will be *
 *           blocked by pawns.  Test() modified so that it will now run using  *
 *           an EPD test suite with am/bm and id tags.  The old testfile for-  *
 *           mat will continue to work for a while, but Test() now notices the *
 *           EPD format and processes it correctly.  A new way of handling the *
 *           search extensions is in place.  With the old approach, one ply    *
 *           could not extend more than one full ply.  With the new approach,  *
 *           borrowed from Deep Blue, two consecutive plies can not extend     *
 *           more than two plies total.  It averages out to be the same, of    *
 *           course, but the effect is a bit different.  Now it is possible    *
 *           for a check and recapture to be applied at the same ply, where    *
 *           they could not before (since a check was already a full one-ply   *
 *           extension).  Whether this is better or not is not clear yet, but  *
 *           it is worth careful analysis.  EvaluateDraws() has been replaced  *
 *           by EvaluateWinner().  This function returns an indicator that     *
 *           specifices whether white, black, both or neither can win in the   *
 *           present position.                                                 *
 *                                                                             *
 *   18.13   Deep Blue extension limit removed and restored to one ply of      *
 *           extension per ply of search.  Pruning in q-search fixed so that   *
 *           a capture will be considered if it takes the total material on    *
 *           the board down to a bishop or less for the opponent, as that can  *
 *           greatly influence the evaluation with the EvaluateWinner() code.  *
 *           As the 50 move rule draws near, the hash table scores are marked  *
 *           as invalid every 5 moves so that hashing can't hide potential     *
 *           50-move-rule draws.  Lazy evaluation code modified to be more     *
 *           conservative, since in endgames the positional score can be a     *
 *           large change.  EvaluatePassedPawnRaces() fixed so that if one     *
 *           side has two pawns that are far enough apart, it will recognize   *
 *           that even though the king is "in the square" of either, it can't  *
 *           always stay in the square of one if it has to capture the other.  *
 *           Pawn hash signature restored to 64 bits after testing proved that *
 *           32 was producing an unacceptable number of collisions.  Search    *
 *           node counter is now 64 bits as well to avoid overflows.  Bug in   *
 *           the outside passed pawn code fixed (bad mask).                    *
 *                                                                             *
 *   18.14   Minor bug in ResignOrDraw() code caused Crafty to not offer draws *
 *           although it would accept them when appropriate.  Rook vs Minor    *
 *           is now evaluated as "neither side can win" an oversight in the    *
 *           EvaluateWinner() code.  Minor bug in ResignOrDraw() would fail to *
 *           offer draws due to the +0.01/-0.01 draw scores returned by the    *
 *           EGTB probe code.                                                  *
 *                                                                             *
 *   18.15   Change in endgame draw recognition to handle the case where one   *
 *           side appears to be in a lost ending but is stalemated.  The code  *
 *           now evaluates such positions as "DrawScore()" instead.  The code  *
 *           to accept/decline draws has been modified.  When a draw offer is  *
 *           received, a global variable "draw_offer_pending" is set to 1.     *
 *           When the search for a move for Crafty terminates, Crafty then     *
 *           uses this value to decide whether to accept or decline the draw.  *
 *           This means that the accept/decline won't happen until _after_ the *
 *           search has a chance to see if something good is happening that    *
 *           should cause the draw to be declined, closing a timing hole that  *
 *           used to exist that let a few "suspects" get away with draws that  *
 *           should not have happened (ie Crafty has - scores for a long time, *
 *           the opponent suddenly fails low and sees he is losing and offers  *
 *           a draw quickly.  Crafty would accept before doing a search and    *
 *           noticing that it was suddenly winning.)  minor evaluation change  *
 *           to notice that K+B+right RP vs K+B is not necessarily won if the  *
 *           weaker side has a bishop of the right color.                      *
 *                                                                             *
 *   19.0    Significant change to the search extension limits.  First, the    *
 *           limit of no more than 1 ply of extensions per ply has been tossed *
 *           out.  Now, any number of extensions are allowed in the first N    *
 *           plies, where N=iteration_depth of the current iteration.  After   *
 *           the first N plies, extensions are reduced by 50% for the next N   *
 *           plies.  They are then reduced by another 50% for the next N plies *
 *           and then completely disabled after that.  IE for a 12 ply search, *
 *           all extensions (even > one ply) are allowed for the first 12      *
 *           plies, then the extensions are reduced by 1/2 for the next 12     *
 *           plies, then by 1/4 for the next 12 plies, then turned off from    *
 *           that point forward.  Minor tweak (suggested by GCP) to reduce the *
 *           time limit by 1/4 for ponder=off games was done in time.c.  Fix   *
 *           to EvaluateWinner() to correctly realize that KR[BN] vs KRR is a  *
 *           draw for the KRR side if it doesn't have at least a pawn left.    *
 *           Minor bug in RejectBookMove() would reject all moves if the op-   *
 *           ponent had a stonewall-type pawn set-up.  It was supposed to only *
 *           reject castling into the attack.  Minor change to code in the     *
 *           EvaluateMaterial() function to lower the penalty for sacrificing  *
 *           the exchange.  it was considered just as bad as trading a rook    *
 *           for two pawns which was wrong.  Change to hashing code so that we *
 *           can now determine that a hash entry came from the EGTB so that    *
 *           PVs are displayed with <EGTB> when appropriate, not <EGTB> if it  *
 *           originally came from the EGTB but later <HT> when it was picked   *
 *           up from the hash table instead.  Minor changes to search/searchmp *
 *           to make them identical in "look" where possible, for consistency  *
 *           in source reading if nothing else.  If the first argument on the  *
 *           command-line is "xboard" or if the environment variable           *
 *           "CRAFTY_XBOARD" is set, Crafty sends a "feature done=0" command   *
 *           to tell xboard it has not yet initialized things.  Significant    *
 *           changes to EvaluateWinner() to recognize new cases of draws such  *
 *           as Q+minor+nopawns vs Q+pawns as unwinnable by the Q+minor side.  *
 *           Other cases were fixed/improved as well.                          *
 *                                                                             *
 *   19.1    Changes to the outside passed pawn and outside candidate pawn     *
 *           code to more correctly recognize when one side has a simple end-  *
 *           game position that is easy to win.  Futility pruning and razoring *
 *           (Jeremiah Penery) was added.  To endable it, you will need to add *
 *           -DFUTILITY to the Makefile options, otherwise it is disabled by   *
 *           default.  EvaluateWinner() had a bug dealing with KR vs KN or     *
 *           KRR vs KRN(B) that has been fixed.                                *
 *                                                                             *
 *   19.2    CCT-5 version 01/20/03.                                           *
 *           Changes to the LimitExtensions() macro.  The extensions are now   *
 *           a bit more aggressive, but after 2*iteration_depth, then they     *
 *           taper off smoothly so that by the time the depth reaches          *
 *           4*iteration_depth, the extensions are cut to zero.  Fixed bug in  *
 *           EvaluatePawns() that missed candidate passed pawns so that the    *
 *           new endgame stuff didn't work completely.  Change to hash.c to    *
 *           combine the two hash tables into one so that the two probed       *
 *           entries are adjacent in memory to be more cache friendly.  A bug  *
 *           in moving a replaced entry from depth-preferred to always-store   *
 *           caused the moved entry to go to the wrong address which would     *
 *           make it impossible to match later.  New hash table layout is more *
 *           cache-friendly by putting one entry from depth-preferred table    *
 *           with two entries from always-store, so that they often end up in  *
 *           the same cache-line.                                              *
 *                                                                             *
 *   19.3    Change to EvaluateMaterial to realize that a rook for five pawns  *
 *           is also likely a "bad trade."  Adaptive hash table size code was  *
 *           added so that the hash size is set automatically based on the     *
 *           estimated NPS and time per move values for a specific "level"     *
 *           command setting.  Repeat() rewritten.  The old code had an        *
 *           unexplained bug that would overlook repetitions in a parallel     *
 *           search in rare cases.  The old cold was complex enough that it    *
 *           was time to rewrite it and simplify it significantly.             *
 *                                                                             *
 *   19.4    Initial depth "seed" value changed to iteration_depth + 1/2 ply,  *
 *           so that fractional extensions kick in earlier rather than deeper  *
 *           in the search.  This performs _much_ better in tactical tests     *
 *           such as WAC and similar suites.  Minor book fix for a bug that    *
 *           could cause a crash with book=off in xboard/winboard.             *
 *                                                                             *
 *   19.5    Default draw score set to 1, because endgame table draws can be   *
 *           0, -.01 or +.01, which translates to -1, 0 or 1 in real scores    *
 *           inside Crafty.  +1 means a draw where white has extra material    *
 *           and that would break accepting draws with such a score.  A few    *
 *           NUMA-related changes.  One global variable was made thread-       *
 *           private to avoid cache thrashing.  Split blocks are now allocated *
 *           by each individual processor to make them local to the specific   *
 *           processor so that access is much faster.  CopyToSMP() now tries   *
 *           to allocate a block for a thread based on the thread's ID so that *
 *           the split block will be in that thread's local memory.  A few     *
 *           other NUMA-related changes to help scaling on NUMA machines.      *
 *                                                                             *
 *   19.6    New egtb.cpp module that cuts decompression indices memory by 50% *
 *           with no harmful side-effects.  Fixes to NUMA code to make SMP and *
 *           non-SMP windows compiles go cleanly.                              *
 *                                                                             *
 *   19.7    Changes to draw code so that Crafty will first claim a draw and   *
 *           then play the move, to avoid any confusion in whether the draw    *
 *           was made according to FIDE rules or not.  Minor bug in evaluate() *
 *           dealing with candidate passed pawns fixed.  A few additions to    *
 *           support my AMD Opteron inline assembly for MSB(), LSB()           *
 *           and PopCnt() procedures.                                          *
 *                                                                             *
 *   19.8    Changes to lock.h to (a) eliminate NT/alpha, since Microsoft no   *
 *           longer supports the alpha, which makes lock.h much more readable. *
 *           CLONE is no longer an option either, further simplyfying the .h   *
 *           files.  New mode option "match" which sets an aggressive learning *
 *           mode, which is now the default.  The old learning mode can be set *
 *           by using the command "mode normal".  Memory leak in new windows   *
 *           NUMA code fixed by Eugene Nalimov.  AMD fix to make the history   *
 *           counts thread-local rather than global (to avoid excessive cache  *
 *           invalidate traffic).                                              *
 *                                                                             *
 *   19.9    Two new pieces of code submitted by Alexander Wagner.  The first  *
 *           adds speech to Crafty for unix users.  /opt/chess/sounds needs to *
 *           exist and the common/sounds.tar.gz file needs to be untarred in   *
 *           directory.  "Speech on*off" controls it.  A new annotatet command *
 *           outputs the annotated file in LaTeX format for those that prefer  *
 *           that platform.  Lots of source reformatting to make indenting and *
 *           other things more consistent, using the unix "indent" utility.    *
 *           Singular extension (simplistic approach) has been added and needs *
 *           -DSINGULAR to activate it.  Once it has been compiled in, you can *
 *           turn it on/off by setting the singular extension depth using the  *
 *           ext command.  Default is on if you compile it in.  Ext/sing=0     *
 *           disable it.  Changes to non-COMPACT_ATTACKS code to shrink the    *
 *           array sizes.  This was done primarily for the Opteron where the   *
 *           COMPACT_ATTACKS stuff was a bit slower than the direct look-up    *
 *           approach.  This change reduced the table sizes by 75%.  A small   *
 *           change to Evaluate() to better handle the 50-move draw problem.   *
 *           What used to happen was when the counter hit 100 (100 plies since *
 *           last capture or pawn push) the score just dumped to DrawScore()   *
 *           instantly.  It was possible that by the time the draw was seen,   *
 *           it was too late to do anything to avoid it without wrecking the   *
 *           position.  what happens now is that once the 50-move counter      *
 *           reaches 80 plies (40 moves by both sides with no pawn push or     *
 *           capture) the score starts dropping toward DrawScore() no matter   *
 *           which side is winning.  Dave Slate called this a "weariness       *
 *           factor" in that after playing 40 moves with no progress, the      *
 *           evaluation shows that the program is getting "weary".  The idea   *
 *           is that the score starts to slip over the last 10 moves by each   *
 *           side to that if either can do something to reset the counter,     *
 *           they will do it sooner rather than later.                         *
 *                                                                             *
 *   19.10   A fix to the EvaluateWinner() code to recognize that while KRB vs *
 *           KR can not normally be won by the KRB side, we require that the   *
 *           KR side not be trapped on the edge of the board, where it can be  *
 *           mated.  FUTILITY code put back in.  19.9 will be the SE version   *
 *           for those wanting to play with it.  New "learn" command option to *
 *           allow the user to set the position learning parameters that are   *
 *           used to trigger position learning and disable position learning   *
 *           after the game is already lost.  The command syntax is as follows *
 *           "learn trigger cutoff" and are expressed as fractions of a pawn   *
 *           with a decimel point.  The default is learn .33 -2.0 which says   *
 *           to do position learning when the score drops 1/3 of a pawn, but   *
 *           turn it off after the score reaches -2.00 as the game is already  *
 *           lost and learning won't help at this point.  This is the CCT-6    *
 *           version exactly as played in the CCT-6 tournament.                *
 *                                                                             *
 *   19.11   A fix to the Annotate() code that could, in certain cases, fail   *
 *           to display N moves when asked.  This happened when there were     *
 *           fewer than N moves total, and on occasion it would not display    *
 *           all the moves that were possible, omitting the last one.  Fix to  *
 *           egtb.cpp to correctly set 16 bit index mode for a few of the      *
 *           recent 6-piece tables.                                            *
 *                                                                             *
 *   19.12   Fix to allocating split blocks that did not let all blocks get    *
 *           used, which would produce slow-downs on large parallel boxes      *
 *           (cpus > 8-16).  Change to fail-high/fail-low window adjustment.   *
 *           I now use the old Cray Blitz approach ("Using time wisely -       *
 *           revisited", JICCA) where a fail-high (or low) now adjusts the     *
 *           root alpha or beta bound by 1 pawn on the first fail, another two *
 *           pawns on the second failure, and finally relaxes the bound to     *
 *           +/- infinity on the third fail for the same move.  This speeds up *
 *           searching many fail-high/fail-low conditions, although it will    *
 *           occasionally cause the search to take a bit longer.               *
 *                                                                             *
 *   19.13   Fix to evaluating split passed pawns.  Code is cleaner and is not *
 *           applied if the opposing side has any pieces at all since a piece  *
 *           and the king can stop both.  New egtb.cpp that opens the tables   *
 *           approximately 20-25% faster is included in this version (code by  *
 *           Eugene Nalimov of course).                                        *
 *                                                                             *
 *   19.14   New "eval" command (eval list and eval help to start) allow a     *
 *           user to modify all internal evaluation values via commands that   *
 *           may be entered directly or via the .craftyrc/crafty.rc init file. *
 *           This was mainly done for the automated evaluation tuning project  *
 *           I am working on with Anthony Cozzie, but it will be useful for    *
 *           merging Mike Byrne's "personality" stuff as well.  New command    *
 *           "personality load/save <filename>" which saves eval and search    *
 *           parameters to create new personalities that can be loaded at any  *
 *           time.  "Crafty.cpf" is the "default" personality file that is     *
 *           used at start-up if it exists.  Evaluation tuning changes based   *
 *           on some results from the new annealing code, although all of the  *
 *           annealed values are not yet fully understood nor utilized yet.    *
 *           LearnBook() is now used to learn good and bad game results.  IE   *
 *           if Crafty wins with a particular opening, it will remember it as  *
 *           good and try it again, in addition to the previous method that    *
 *           would remember bad openings and avoid them.  It also now handles  *
 *           draws as well which can make slightly bad openings appear better. *
 *           Minor change to SMP code to eliminate the possibility of a shared *
 *           variable getting overwritten unintentionally.  Several changes to *
 *           evaluation weights and evaluation code as testing for the WCCC    *
 *           continues.  New "help" command uses a file "crafty.hlp" which now *
 *           contains all the help information.  This makes the help info      *
 *           easier to maintain and gets rid of a bunch of printf()s in the    *
 *           Option() source (option.c).  Rewrite of the "list" code.  Crafty  *
 *           now supports six lists, AK (auto-kibitz), B (blocker), C (comp),  *
 *           GM, IM and SP (special player).  The SP list has two extra items  *
 *           that can be specified to go with a player's name, namely a book   *
 *           filename (to replace the normal books.bin, but not book.bin) and  *
 *           a personality file that will be loaded each time this particular  *
 *           named opponent plays.  The format is available in the new help    *
 *           facility.  Bug fix for outside passed pawn code that would cause  *
 *           an outside passer to appear to be a winning advantage even if the *
 *           opponent has a protected passer, which easily negates the outside *
 *           passer's threat.                                                  *
 *                                                                             *
 *   19.15   Fix to outside passed pawn code that requires pawns on both sides *
 *           of the board for the side with an "outside passer" or "outside    *
 *           candidate" to avoid some bizarre evaluations. Sel 0/0 now works   *
 *           without crashing Crafty.  This would fail in previous versions as *
 *           the hash signature would be modified but not restored.  Slightly  *
 *           more conservative limit on using null-move search to head off a   *
 *           few notable zugzwang problems was added.  Fix to time control     *
 *           code to remove a hole that could cause a divide-by-zero at a time *
 *           control boundary.  Stonewall detection removed completely as it   *
 *           appears to be no longer needed.  Rook scoring changed to better   *
 *           evaluate "open files" by measuring mobility on them.  Complete    *
 *           removal of Phase() (phase.c) and references to the opening,       *
 *           middlegame and endgame phases as they were no longer referenced   *
 *           anywhere in the code.                                             *
 *                                                                             *
 *   19.16   Fix to "Trojan code" to eliminate the time limit exclusion since  *
 *           many users still have old and slow hardware, and the time limit   *
 *           was not set correctly when PreEvaluate() was called anyway.  The  *
 *           code to display fail-high/fail-low information was cleaned up so  *
 *           that the +1 or +3 now makes sense from the black side where the   *
 *           score is really going down (good for black) rather than showing   *
 *           a +3 fail high (when Crafty is black) and the score is really     *
 *           going to drop (get better for black).  Now the fail-high-fail-low *
 *           +/- sign is also relative to +=good for white like the scores     *
 *           have been for years.  Adjustments to pawn evaluation terms to     *
 *           improve the scoring balance.  "New" now terminates parallel       *
 *           threads (they will be re-started when needed) so that we don't    *
 *           burn CPU time when not actually playing a game.                   *
 *                                                                             *
 *   19.17   Changes to pawn evaluation to limit positional scores that could  *
 *           get a bit out of sane boundaries in some positions.               *
 *                                                                             *
 *   19.18   ProbeTransRef() no longer adjusts alpha/beta bounds if the entry  *
 *           is not good enough to terminate the search here.  This has helped *
 *           speed things up (reduced size of tree) over many test positions   *
 *           so either it was buggy or not worthwhile.  Regardless, it is now  *
 *           'gone'.  Connected passed pawns now scored as a simple pair of    *
 *           pawns that are better as they are advanced, the old connected     *
 *           passed pawns on the 6th rank special code has been removed.       *
 *                                                                             *
 *   19.19   Repeat3x() had a bug that would cause it to miss a draw claim on  *
 *           the 50th move, often making a strange (losing) move that would    *
 *           not lose if the draw is was claimed, but which would cause a loss *
 *           if the draw was not claimed because the piece might be instantly  *
 *           captured if the opponent can play a move.                         *
 *                                                                             *
 *   19.20   Bug in the EvaluateMaterial() (bad trade) code that would not     *
 *           penalize a single piece vs 3 pawns properly.  Now the penalty is  *
 *           added in unless the side with the piece has nothing else to go    *
 *           with it at all (no pawns or other pieces).  Pawn scoring changed, *
 *           doubled pawns were scored too badly because the penalties were    *
 *           added twice (as expected) but they were set as if they were just  *
 *           added once.  Eval command also had a bug in displaying the pawn   *
 *           evaluation parameters.  Move input changed to accept pawn         *
 *           promotions of the form a8Q (this is needed as ChessBase violates  *
 *           the PGN/SAN standard that mandates a8=Q as the proper syntax for  *
 *           pawn promotions.)  annoying glitch in epdglue.c that would        *
 *           produce the wrong score for positions with exactly one legal move *
 *           was fixed.  New InvalidPosition() function verifies that FEN      *
 *           setup positions are reasonable, without extra pawns or too many   *
 *           pieces total, etc.  Passed pawn to 7th search extension removed,  *
 *           mate threat extension tuned down to 1/2 ply.  Bug in SetBoard()   *
 *           fixed.  This bug made it impossible to import many epd records    *
 *           due to incorrectly handling the wtm flag.  This was introduced in *
 *           the recent changes to disallow certain types of illegal positions *
 *           that were impossible (more than 9 queens of one color, etc.) new  *
 *           eval term for space added.  This simply counts the number of      *
 *           squares on each rank attacked by pawns, and then multiplies by a  *
 *           constant that scales the attacks so that attacking ranks on the   *
 *           opponent's side of the board is more valuable than attacking      *
 *           squares on our side of the board and vice-versa.  Significant     *
 *           coding changes for the unix parallel search.  POSIX threads are   *
 *           gone, due to too many differences between vendor's implementation *
 *           details.  Crafty now uses SYSV shared memory to share the search  *
 *           data (this is exactly the same parallel search approach always    *
 *           used, but with a completely different implementation) and fork()  *
 *           to spawn processes for each CPU.  It simply became too much work  *
 *           to figure out what each vendor was doing about CPU time, how      *
 *           threads were scheduled, plus debug thread library implementations *
 *           that were often buggy.  There are no performance benefits to this *
 *           approach other than falling back to features found in all Unix    *
 *           implementations.  New NUMA changes to make sure that the local    *
 *           thread "blocks" are actually allocated on the NUMA node where     *
 *           the process actually runs, rather than on a remote node that      *
 *           will cause extra memory latency for often-used memory data.  Bug  *
 *           in EvaluatePawns() caused the outside passer / outside            *
 *           candidate bonus to be triggered when the opponent has a simple    *
 *           protected passed pawn.  Since the decoy value is nil with a       *
 *           protected passer (we can't take the supporing pawn or the passer  *
 *           will run) the outside candidate or passer doesn't help.  Bug in   *
 *           king safety caused pieces to be attracted to the opponent's king, *
 *           even in simple endgames, inflating/deflating the score for a      *
 *           feature that was pointless.  (2005 WCCC final version).           *
 *                                                                             *
 *   20.0    First in a new series.  First change is to produce an endian-     *
 *           independent opening book.  The format has not changed so that old *
 *           book.bin/books.bin files will work fine, but they may now be      *
 *           freely between big-endian and little-endian architectures.  This  *
 *           makes the binary format compatible between a PC and a Sun or HP   *
 *           box, for example, so that just one book.bin file is needed.       *
 *                                                                             *
 *   20.1    First step toward a complete redesign of the evaluation code in   *
 *           Evaluate() (evaluate.c).  This first set of changes are mainly    *
 *           restructuring related, an attempt to collect the various bits of  *
 *           evaluation code that are related to a common theme (i.e. All      *
 *           bishop scoring, all passed pawn scoring, etc) are not contained   *
 *           in one routine, with a sensible name.  This makes it much easier  *
 *           to find evaluation code and modify it, without having bits and    *
 *           pieces scattered all over the place.  Perhaps the most signifi-   *
 *           cant is that the current evaluation has dropped the asymmetric    *
 *           king safety, and later the asymmetric blocked pawn code will go   *
 *           as well.  To do this, I did have to make some changes in terms    *
 *           of the "tropism" code.  The basic idea is that white's "tropism"  *
 *           bonus in incremented for each white piece that attacks the black  *
 *           king, but it is also decrement for each black piece that attacks  *
 *           the black king, so that a defender offsets an attacker.  This has *
 *           certainly changed the "personality" of how it plays.  It is now   *
 *           far more aggressive, and further changes planned for the future   *
 *           will hopefully improve on this change.  It is playing quite a bit *
 *           better, but whether this aggressiveness is "over the top" remains *
 *           to be seen.  Fix to "won kp ending" logic.  If one king is closer *
 *           to the opponent's "average pawn square" than the other, by at     *
 *           least 2 moves, then that side is credited as having a won k+p     *
 *           ending. New code for time allocation testing added.  A new        *
 *           "timebook <factor> <moves>" command that says use "factor" extra  *
 *           time (factor is expressed as a percent, 100 means use 100% extra  *
 *           time) tapered off for "moves" out of book.  For example,          *
 *           "timebook 100 10" says use 100% extra time on first non-book move *
 *           followed by 90% extra on the next move, 80% on the next, until    *
 *           after 10 moves we are back to the normal time average.            *
 *                                                                             *
 *   20.2    Significant evaluation changes to try to limit positional scores  *
 *           so that new tuning can be accomplished.  In particular, the king  *
 *           safety code has been greatly simplified, now having two separate  *
 *           components, one for pawn structure, one for piece tropism.  A new *
 *           SearchControl() procedure now handles all the search extensions   *
 *           in one centralized place, shared by normal and parallel search    *
 *           code.  The plan is that eventually this piece of code will also   *
 *           be able to recommend search reductions as well as extensions to   *
 *           speed the tree traversal process significantly with a form of     *
 *           of forward pruning that is commonly called "search reductions".   *
 *                                                                             *
 *   20.3    Search reduction code added.  Part of this dates back to 1997     *
 *           when Bruce Moreland and I played with the idea of reducing the    *
 *           depth on the last part of the move list when it seemed to become  *
 *           fairly certain that no fail-high was going to happen.  That was a *
 *           big search speed improvement, but also risky.  Recently, several  *
 *           started to do this again, except they added one fairly useful     *
 *           constraint, that the "history value" for a specific move has to   *
 *           be below some threshold before a reduction can happen.  The idea  *
 *           is that we don't want to reduce the depth of moves that have      *
 *           failed high in the past, because they might fail high again if    *
 *           they are not reduced too much.  Tord Romstad suggested an         *
 *           additional improvement, namely that if a reduced move does fail   *
 *           high, re-search with the proper (non-reduced) depth to verify     *
 *           that it will fail high there as well.  All of this is included    *
 *           in version 20.3, but the values (reduction amount, history        *
 *           threshold, min depth to do a reduction) need serious testing and  *
 *           tuning.  The current values are simply "legal" but hardly could   *
 *           be considered optimal without substantial testing.  New code to   *
 *           "age" history counters added.  After each iteration, this code    *
 *           divides each counter by 2, to scale the values back, so that      *
 *           moves that are no longer causing fail highs will have a history   *
 *           value that won't prevent reduction pruning.  The "minimum depth"  *
 *           value indicates how much search to leave after doing a reduction  *
 *           in the tree.  A value of 1 ply guarantees that after a reduction  *
 *           is done, there will always be one full-width ply of search done   *
 *           after such reductions to avoid gross tactical oversights.  Tuning *
 *           to extensions to reduce the tree size.  The recapture extension   *
 *           has been completely removed, while the check extension has been   *
 *           left at 1.0 as in the past.  The mate threat and one-legal-reply  *
 *           extensions are now both set to .75 after extensive testing.  This *
 *           is the CCT8 version of Crafty.                                    *
 *                                                                             *
 *   20.4    History counters changed.  There is now one large history[8192]   *
 *           array to simplify indexing.  More importantly, these values are   *
 *           now limited to a max value of 32000.  During the update process,  *
 *           if any value reaches that limit, all values are divided by 2 to   *
 *           prevent deep searches from producing very large history values    *
 *           which would essentially disable late move reductions based on the *
 *           big history values.  The reduce/history=n command now uses values *
 *           0 <= n <= 100.  A value of 100 will cause all non-tactical moves  *
 *           to be reduced (most aggressive) while a value of 0 will result in *
 *           no moves being reduced (most conservative).  The value, loosely   *
 *           interpreted, means "reduce if a move fails high < n% of the time  *
 *           when at least one move fails high in a search."  the default is   *
 *           currently 50%, but further testing is needed.                     *
 *                                                                             *
 *   20.5    Old "mask" code removed and replaced by constants, since this is  *
 *           never intended to run on a Cray specifically any longer.  Change  *
 *           to "won king and pawn ending" to make it more accurate, this code *
 *           is used when there are nothing but pawns left, all on one side of *
 *           board, and one king is closer to a capturable pawn than the other *
 *           king is.  This is intended to help in outside passed pawn games   *
 *           where one side has to eventually give up the outside passer to    *
 *           decoy the opposing king away from the rest of the pawns.  New     *
 *           king safety now factors in king pawn shelter and opponent piece   *
 *           tropism into one score, but the score is now an arithmetic        *
 *           value that goes up more quickly as both indices increase, to do a *
 *           more accurate evaluation of safety and attacking chances.  Change *
 *           to bishop mobility, which now goes in units of 2 rather than 1,   *
 *           to increase the value of mobility somewhat.  Also mobility is now *
 *           'centered' in that it is computed as (squares - 6) * 2, to keep   *
 *           the bishop value from going way over its normal range with these  *
 *           new and somewhat larger numbers.                                  *
 *                                                                             *
 *   20.6    New eval code to look for bishops that are blocked in the forward *
 *           direction by friendly pawns.  That basically turns a bishop into  *
 *           a "tall pawn" since movement in the critical direction is not     *
 *           possible.                                                         *
 *                                                                             *
 *   20.7    Manually tuned kinf_safety[][] array values.  Modified the new    *
 *           bishop code to only consider the "long diagonal" if the bishop    *
 *           is in either 9-square corner on its own side of the board, so     *
 *           that the short diagonal being open won't offset the long          *
 *           diagonal that is blocked.  Bishop-pair bonus is now adjusted by   *
 *           how open or closed the center is.  If 3 of the four center pawns  *
 *           are gone, the center is open and the bishop pair bonus is set to  *
 *           the max.  If the four center pawns are locked (e4/e5/d5/d6 for    *
 *           example) then the bishop pair bonus is eliminated.  if the center *
 *           is somewhere in the middle of those extremes, then the bishop     *
 *           pair bonus is present but reduced.  Castling now is a bit more    *
 *           intelligent.  If a side has not castled, then moving either rook  *
 *           receives a penalty, where moving the king or the last unmoved     *
 *           rook receives a larger penalty.  If the king has not castled, its *
 *           'safety' score is based on the most safe position of the possible *
 *           options.  If neither rook has moved, then the score is based on   *
 *           the safest of the three locations, either in the center where it  *
 *           is, or on the safest wing.  If uncastled, this safaty is always   *
 *           reduced a bit to encourage castling before it becomes too late to *
 *           do so.  This was done to avoid a problem where the queen rook had *
 *           been moved, Crafty had not castled, and it pushed the kingside    *
 *           pawns making it unsafe to castle there, leaving the center as the *
 *           best place, yet had it not pushed the kingside pawns, castling    *
 *           would have been a better alternative.   Two new 16 word vectors,  *
 *           safety_vector[]/tropism_vectorp[] are used to set the edge values *
 *           for the king_safetyp[][] array.  Interior values are filled in    *
 *           based on these two vectors, making it much easier to tweak the    *
 *           king safety scores without pulling your hair out.                 *
 *                                                                             *
 *   20.8    When queens come off, the previous version cut the tropism scores *
 *           and pawn shelter scores by 1/2 each, which had the effect of      *
 *           dropping the overall score by 75% (the score is a geometric       *
 *           computation).  The safety/tropism values are now dropped, but by  *
 *           a smaller amount, since bishops + rooks + knights are still a     *
 *           dangerous threat.                                                 *
 *                                                                             *
 *   20.9    Simple lazy eval (Mike Byrne) added.  This simply avoids the more *
 *           expensive piece evaluations if the pawn scoring and material      *
 *           leave the score well outside the alpha/beta search window.  Looks *
 *           like a 20% speed improvement in general.                          *
 *                                                                             *
 *   20.10   Change to king safety pawn structure.  Open files around the king *
 *           are dangerous, but the h/g (or b/a) files are more dangerous than *
 *           the f-e-d-c files since they are much harder to defend against    *
 *           with the castled king in the way.  Modified the way the king      *
 *           safety matrix is computed so that at the tropism/pawnstructure    *
 *           values are more sane.  It currently will get the same value for   *
 *           array indices of 5,0, 4,1, 3,2, 2,3, 1,4, or 0,5, which is better *
 *           than what it was doing.  I plan to tweak this more later to make  *
 *           3,2 or 2,3 worse than 4,1 or 1,4 and 0,5 or 5,0, as the latter    *
 *           mean "unsafe king but no piece pressure" or "piece pressure but   *
 *           king pawn shelter is safe" whereas the middle values mean both    *
 *           components of an attack are present, unsafe king position and the *
 *           pieces are closing in.                                            *
 *                                                                             *
 *   20.11   Minor fix to late move reduction code.  Turns out it is possible  *
 *           to reach the block of code if (first_move) { } with status set to *
 *           REMAINING_MOVES (this means no captures, no killer, no hash move, *
 *           no history move, etc).  That would let me try a reduction, and if *
 *           it failed high, we would use that score as-is.  We now re-search  *
 *           a fail-high here just like we did in the other block of code so   *
 *           that a fail-high on a reduction search is never trusted.          *
 *                                                                             *
 *   20.12   Further modifications to king safety initialization code to try   *
 *           scale back the values a bit to control score limits.              *
 *                                                                             *
 *   20.13   Fix to bench.c to remove two file closes that would wreck the     *
 *           program after the bench command was finished.  Also the           *
 *           SMP-time-to-ply measurement was removed since it had no meaning.  *
 *           Old FUTILITY code re-inserted along with an extension limit       *
 *           modification by Mike Byrne to avoid limiting the search extension *
 *           on one-reply extensions only.  New lock.h submitted by Andreas    *
 *           Guettinger to support spinlocks on the PPC architecture (Linux).  *
 *           Some -D configuration options were changed so that none are       *
 *           needed to compile the "standard" Crafty.  Futility is normally on *
 *           now, and -DNOFUTILITY is required to compile it out.  The new     *
 *           Makefile gives the configuration options, but the only ones that  *
 *           would normally need setting are the SMP and CPUS= options if you  *
 *           have a SMP platform.                                              *
 *                                                                             *
 *   20.14   Fix to EvaluateWinningChances() to correctly evaluate KRP vs KR   *
 *           type endings.  Previously this was done in the wrong place and    *
 *           the score would favor the side with the pawn significantly, where *
 *           the score is now zero.  Minor bugfix to code that detects that an *
 *           illegal position has been set up.  It caught every possible       *
 *           problem except for positions with a king (or kings) missing.      *
 *                                                                             *
 *   20.15   Change to how Crafty chooses where to split.  Previously, there   *
 *           was a min remaining depth limit to prevent Crafty from trying to  *
 *           split too close to the frontier nodes.  Now that is specified as  *
 *           a percentage of the nominal search depth.  For example, the       *
 *           default value is 50%, which says that on a 16 ply search, we can  *
 *           split if at least 8 plies of search are left.  This provides      *
 *           better split overhead than a simple limit that doesn't take the   *
 *           actual search depth into consideration.  History data arranged to *
 *           be more cache friendly.  The three values for a common history    *
 *           index value are now stored in adjacent memory words to take       *
 *           advantage of pre-fetching within a cache line.  Additional        *
 *           tuning of the king safety matrix.                                 *
 *                                                                             *
 *   20.16   Evaluation of knights, bishops, rooks, queens and kings is now    *
 *           done on a 1000 point scale, then the sum is divided by 10.  This  *
 *           allows us to use the same hashing mechanism, but with finer       *
 *           tuning precision.  Minor queen eval tweak for 7th rank position.  *
 *                                                                             *
 *   20.17   New book learning, with a simplified book system.  The export     *
 *           files (book.lrn, position.lrn) no longer are created/used, the    *
 *           learn mechanism no longer touches the real chess board which      *
 *           could cause problems in certain xboard/winboard timing            *
 *           situations.  One major learning change is that we now only learn  *
 *           things from "Crafty's perspective".  That is, if Crafty loses a   *
 *           game as black, it won't think that opening is good when it plays  *
 *           white since it might well not understand how to win using that    *
 *           particular opening.  It will only "learn" on the actual moves it  *
 *           plays in a game.  Old "history heuristic" removed.  Testing       *
 *           showed that the LMR idea produced better results without the      *
 *           history ordering, since now more moves are reduced, but moves     *
 *           with good history values don't get reduced.                       *
 *                                                                             *
 *   21.0    Finally did the bit-renumbering task so that bit 0 = LSB = A1,    *
 *           to eliminate the 63-n translation to the numbering scheme used    *
 *           on the Cray architecture.  For all bit operations now, LSB=0,     *
 *           MSB=7, 15, 31 or 63 depending on the data size.  A few minor      *
 *           bugs were fixed along the way as these changes were debugged.     *
 *           Minor bug in EvaluateKings() where it tested for a "won ending"   *
 *           and used a test that was too aggressive.  This test did not get   *
 *           things quite right when the two kings were very close together    *
 *           and "opposition" prevented one king from moving closer to the     *
 *           pawns.                                                            *
 *                                                                             *
 *   21.1    New piece scoring from Tracy, which changes the way pieces are    *
 *           scored based on squares they attack.                              *
 *                                                                             *
 *   21.2    The scores are now scaled better for the transition from middle-  *
 *           game to end-game.  Passed pawn scores climb as material is        *
 *           removed from the board, to prevent them from overwhelming the     *
 *           middlegame king safety scores.  We do exactly the same scaling on *
 *           king-safety scores, but they are scaled downward as material is   *
 *           removed so that they "phase out" as the endgame is reached.       *
 *           Weak pawn code changed to more accurately identify weak pawns.    *
 *           These are totaled and then used to index a scoring array that     *
 *           penalizes the number of weak pawns on each side.  Pawn islands    *
 *           are now recognized and each side is penalized according to the    *
 *           number of islands he has, more being worse.                       *
 *                                                                             *
 *   21.3    "Magic" move generation is now used, which eliminates the rotated *
 *           bitboard approach completely.  Not a significant speed change,    *
 *           but it is far simpler overall.  Original code by Pradu Kannan as  *
 *           posted on CCC/Winboard forums, modified to work with Crafty.      *
 *                                                                             *
 *   21.4    Misc eval changes.  More king safety changes to include a tropism *
 *           distance of 1 for bishops or rooks that can slide to intercept    *
 *           any square surrounding the enemy king.  Reduced rook slide score  *
 *           by 40%.
 *                                                                             *
 *   21.5    passed pawn extension revisited.  Bad trade was giving a penalty  *
 *           for being down an exchange (or a bonus for being up an exchange)  *
 *           which was not expected behavior.  This has been fixed.  EGTB      *
 *           initialization was being done during the first pass over the      *
 *           RC file, which was too early as the rest of the engine had not    *
 *           been initialized.  Now the "egtb" command is the only thing that  *
 *           triggers initialization, and is only processed during the second  *
 *           pass over the RC file after paths have been properly set up.      *
 *           Abs() function bug fix.                                           *
 *                                                                             *
 *   21.6    Passed pawn evaluation changed with respect to distant passed     *
 *           pawns and distant majorities.  Now, if both have a pawn majority  *
 *           we check to see if one is "distant" (away from the enemy king) as *
 *           this represents a significant time advantage because the other    *
 *           side has to waste moves to reach and capture the pawn to prevent  *
 *           promotion.  LMR no longer uses the history counters, which proved *
 *           to be essentially random numbers.  LMR decisions are now based    *
 *           solely on static characteristics of an individual move, rather    *
 *           than trying to determine how the move affected the search at      *
 *           other points in the tree.  New wtm bonus added to evaluation.     *
 *           EvaluateMate() bug fixed where the score was set to a non-zero    *
 *           value which broke EvaluateMate() completely.  New mobility code   *
 *           for bishops/rooks encourages better piece placement.  Note that   *
 *           there are many other evaluation changes too numerous to list here *
 *           in detail.                                                        *
 *                                                                             *
 *   21.7    New reduce at root code added to do reductions at the root, but   *
 *           not on any move that has been a "best move" during the current    *
 *           search, unless the move drops too far in the move list.  New nice *
 *           mode for parallel search terminates threads while waiting on the  *
 *           opponent's move to avoid burning unnecessary CPU time (this nice  *
 *           mode is NOT the default mode however.) -DNOFUTILITY removed as    *
 *           futility is on by default and should not be removed.  All of the  *
 *           search() code has been cleaned up and streamlined a bit for more  *
 *           clarity when reading the code.  Nasty bug in EvaluatePawns() that *
 *           unfortunattely used the king squares to make a decision dealing   *
 *           with outside passed pawns, but the hash signature for pawns does  *
 *           not include king position.  This code was deemed unnecessary and  *
 *           was summarily removed (not summarily executed, it has had that    *
 *           done far too many times already).                                 *
 *                                                                             *
 *   22.0    Redesign of the data structures completed so that Crafty will no  *
 *           longer have duplicated code for wtm and btm cases.  This has      *
 *           already resulted in a rewrite of movgen(), SEE(), Make() and      *
 *           Unmake() and cut that code size by almost exactly 50%. The result *
 *           has been slightly faster speeds, but also the future benefits     *
 *           will be enormous by eliminating a lot of debugging caused by the  *
 *           duplicated code.  Timing change to avoid starting an iteration    *
 *           and wasting significant time.  We still search enough to get a    *
 *           quick fail-low, otherwise we stop and make the move and save the  *
 *           time we used to burn.  Massive cleanup in the macros used to      *
 *           access much of the data since the new [wtm] approach resulted in  *
 *           many old macros becoming unused.  Yet another rewrite of the code *
 *           that handles repetition detection.  There is an undetected bug in *
 *           the previous code, related to pondering and SMP, that was not     *
 *           obvious.  The code was completely rewritten and is now much       *
 *           simpler to understand, and has been verified to be bug-free with  *
 *           massive cluster testing.                                          *
 *                                                                             *
 *   22.1    Minor fix for CPUS=1, which would cause compile errors.  Other    *
 *           eval tweaks to improve scoring.  New "skill" command that can be  *
 *           used to "dumb down" Crafty.  "Skill <n>" where n is a number      *
 *           between 1 and 100.  100 is max (default) skill.  Skill 70 will    *
 *           drop the playing Elo by about 200 points.  Skill 50 will drop it  *
 *           about 400 points.  The curve is not linear, and the closer you    *
 *           get to 1, the lower the rating.  To use this feature, you need to *
 *           add -DSKILL to your Makefile options otherwise it is not included *
 *           in the executable.                                                *
 *                                                                             *
 *   22.2    We are now back to using POSIX threads, since all current Linux   *
 *           distributions now use the posix-conforming NTPL implementation    *
 *           which should eliminate the various compatibility issues that      *
 *           caused problems in the past.  This also should make the new       *
 *           smpnice mode work correctly for Linux and Windows since they will *
 *           now both use threads for the SMP search.  Fruit-like scoring      *
 *           (interpolation between mg and eg scores) fully implemented.  AEL  *
 *           pruning (Heinz 2000) also fully implemented (we had razoring and  *
 *           futility, but not extended futility).  "Eval" command has been    *
 *           removed and combined with the "personality" command so that eval  *
 *           parameters can be modified, in addition to some search parameters *
 *           that also belong in the personality data.  Mate threat extension  *
 *           and one legal reply to check extensions have been removed.  Tests *
 *           proved them to be absolutely useless, over 30,000 games for each  *
 *           test showed no gain and sometimes a loss in playing strength with *
 *           them so we followed the "simple is better" and removed them.  The *
 *           fractional ply code was also removed since the only extension we  *
 *           now use is the "give check" extension which is a whole ply.  A    *
 *           significant number of evaluation parameters have been changed,    *
 *           a few even removed as cluster testing helped us find optimal      *
 *           values.  There are a few new terms, with more planned.            *
 *                                                                             *
 *   22.3    Corrected a bug in analysis mode where "back n" would crash.  A   *
 *           bad array reference broke this during recent changes.             *
 *                                                                             *
 *   22.4    Corrected a bug in NextRootMove() that tries to calculate the NPS *
 *           by adding up the node counters in each used split block.  However *
 *           should you not use all possible threads (mt=n where n is < the    *
 *           max CPUS value compiled in) then all split blocks will not be     *
 *           properly initialized (not even allocated) which will cause a      *
 *           segfault instantly.  Pawn island evaluation term has also been    *
 *           removed as cluster testing showed that setting these values to    *
 *           zero produced a +14 Elo gain.  There might be a better value for  *
 *           them, but the old value was too high.  Further cluster testing is *
 *           underway to test other (but smaller than original) values to see  *
 *           if one of them turns out to be better than zero.  So far, no, but *
 *           the test has not completed.                                       *
 *                                                                             *
 *   22.5    Minor eval tweaks.  Pthread_create was called without giving it   *
 *           any attributes for the new thread(s) which makes them default to  *
 *           "joinable".  When smpnice=1 terminates threads, the threads hang  *
 *           around using memory expecting that the parent will join with them *
 *           to check the exit status.  We now set "detached" as the proper    *
 *           attribute so that this memory leak no longer happens.             *
 *                                                                             *
 *   22.6    Compile issue fixed which would cause the pthread_lib calls to    *
 *           be used in a windows compile, which was incorrect.  They are now  *
 *           protected by a #if defined(UNIX) && (CPUS > 1) conditional test.  *
 *                                                                             *
 *   22.7    Windows NUMA support fix to eliminate a memory leak caused by     *
 *           re-malloc'ing the split blocks each time a new game is started.   *
 *           This was not found in the previous memory leak testing as that    *
 *           problem was related to stopping/re-starting threads when using    *
 *           smpnice=1.  This is unrelated, but similar, and has been around   *
 *           a long time.                                                      *
 *                                                                             *
 *   22.8    Modified material imbalance fix from Tracy included.  This        *
 *           produced an immediate +7 Elo improvement, and additional tuning   *
 *           is under way.  Fix to GenerateChecks().  This code had bugs a few *
 *           weeks back and they were fixed, but the fixes were somehow lost   *
 *           during copying files among multiple machines.  The fixes are now  *
 *           back in and properly saved.                                       *
 *                                                                             *
 *   22.9    More eval tuning producing another +10 Elo gain.  Curmv[n] was    *
 *           "overloaded" in that it was used to obtain the next move for a    *
 *           parallel thread and was also used to pass back a "fail high" move *
 *           to store in a "LOWER" hash table entry.  This led to some error   *
 *           messages internal to Crafty that could possibly cause problems.   *
 *           There is now a second variable "cutmove" used to pass the move    *
 *           back to eliminate race conditions.  Hash.c modified extensively   *
 *           to improve readability as well as to eliminate duplicated code.   *
 *           Internal iterative deepening code removed.  No measurable benefit *
 *           in real game testing led to removal since it is simpler without   *
 *           the code.  EvaluateMaterialDynamic() also removed.  This was code *
 *           that implemented ideas suggested by IM Larry Kaufman, but careful *
 *           testing showed absolutely no effect on game results.  Again, we   *
 *           chose "simpler is better" and removed it, as no tuning attempts   *
 *           helped for this code.  Nasty pawn hashing bug fix, something that *
 *           has been around for years.  This was essentially the same sort of *
 *           bug that the "lockless hashing algorithm" addressed for the main  *
 *           hash table, where out-of-order memory writes could produce a hash *
 *           signature and the 64 bit word with score and best move which did  *
 *           not "go together".  The same thing happens in pawn hashing,       *
 *           except that an entry is 32 bytes rather than 16, giving even      *
 *           higher probability of an entry that contains mismatched pieces.   *
 *           In the best case, we just got a bad score.  In the worst case,    *
 *           this could cause a program crash since some of the data might be  *
 *           invalid, yet it can be used in ways that produce impossible array *
 *           subscripts that would cause (very rarely) a segmentation fault.   *
 *           The solution was to do the same lockless hashing algorithm where  *
 *           the key is XORed with the other three 64 bit words when the entry *
 *           is stored, then XORed with them again when we try to match.  If   *
 *           the entry is mismatched, the signature will not match anything    *
 *           and we simply will not use the bad data and do a normal pawn      *
 *           evaluation to compute valid values.                               *
 *                                                                             *
 *    23.0   Essentially a cleaned up 22.9 version.  Comments have been        *
 *           reviewed to make sure they are consistent with what is actually   *
 *           done in the program.  Major change is that the random numbers     *
 *           used to produce the Zobrist hash signature are now statically     *
 *           initialized which eliminates a source of compatibility issues     *
 *           where a different stream of random numbers is produced if an      *
 *           architecture has some feature that changes the generator, such    *
 *           as a case on an older 30/36 bit word machine.  The issue with     *
 *           this change is that the old binary books are not compatible and   *
 *           need to be re-created with the current random numbers.  The       *
 *           "lockless hash table" idea is back in.  It was removed because    *
 *           the move from the hash table is recognized as illegal when this   *
 *           is appropriate, and no longer causes crashes.  However, the above *
 *           pawn hash issue showed that this happens enough that it is better *
 *           to avoid any error at all, including the score, for safety.  We   *
 *           made a significant change to the parallel search split logic in   *
 *           this version.  We now use a different test to limit how near the  *
 *           tips we split.  This test measures how large the sub-tree is for  *
 *           the first move at any possible split point, and requires that     *
 *           this be at least some minimum number of nodes before a split can  *
 *           be considered at this node.  The older approach, which based this *
 *           decsion on remaining search depth at a node led to some cases     *
 *           where the parallel search overhead was excessively high, or even  *
 *           excessively low (when we chose to not split frequently enough).   *
 *           This version appears to work well on a variety of platforms, even *
 *           though NUMA architectures may well need additional tuning of this *
 *           paramenter (smpsn) as well as (smpgroup) to try to contain most   *
 *           splits on a single NUMA node where memory is local.  I attempted  *
 *           to automate this entire process, and tune for all sorts of plat-  *
 *           forms, but nothing worked for the general case, which leaves the  *
 *           current approach.  When I converted back to threads from          *
 *           processes, I forgot to restore the alignment for the hash/pawn-   *
 *           hash tables.  The normal hash table needs to be on a 16 byte      *
 *           boundary, which normally happens automatically, but pawn hash     *
 *           entries should be on a 32 byte boundary to align them properly in *
 *           cache to avoid splitting an entry across two cache blocks and     *
 *           hurting performance.  New rook/bishop cache introduced to limit   *
 *           overhead caused by mobility calculations.  If the ranks/files the *
 *           rook is on are the same as the last time we evaluated a rook on   *
 *           this specific square, we can reuse the mobility score with no     *
 *           calculation required.  The code for "rook behind passed pawns"    *
 *           was moved to EvaluatePassedPawns() so that it is only done when   *
 *           there is a passed pawn on the board, not just when rooks are      *
 *           present.  Book learning has been greatly cleaned up and           *
 *           simplified.  The old "result learning" (which used the game       *
 *           result to modify the book) and "book learning" (which used the    *
 *           first N search scores to modify the book) were redundant, since   *
 *           result learning would overwrite whatever book learning did.  The  *
 *           new approach uses the game result (if available) to update the    *
 *           book database when the program exits or starts a new game.  If    *
 *           a result is not available, it will then rely on the previous      *
 *           search results so that it has some idea of whether this was a     *
 *           good opening or not, even if the game was not completed.  Minor   *
 *           LMR bug in SearchRoot() could do a PVS fail-high research using   *
 *           the wrong depth (off by -1) because of the LMR reduction that had *
 *           been set.  The normal search module had this correct, but the     *
 *           SearchRoot() module did not.  EvaluateDevelopment() was turned    *
 *           off for a side after castling.  This caused a problem in that we  *
 *           do things like checking for a knight blocking the C-pawn in queen *
 *           pawn openings.  Unfortunately, the program could either block the *
 *           pawn and then castle, which removed the blocked pawn penalty, or  *
 *           it could castle first and then block the pawn, making it more     *
 *           difficult to develop the queen-side.  We now enable this code     *
 *           always and never disable it.  This disable was done years ago     *
 *           when the castling evaluation of Crafty would scan the move list   *
 *           to see if Crafty had castled, when it noticed castling was no     *
 *           longer possible.  Once it had castled, we disabled this to avoid  *
 *           the lengthy loop and the overhead it caused.  Today the test is   *
 *           quite cheap anyway, and I measured no speed difference with it on *
 *           or off, to speak of.  Now we realize that the knight in front of  *
 *           the C-pawn is bad and needs to either not go there, or else move  *
 *           out of the way.                                                   *
 *                                                                             *
 *    23.1   Xboard protocol support slightly modified so that draw results    *
 *           are now sent after the move that causes the draw.  They were      *
 *           always sent before a move if the position prior to the move was   *
 *           drawn, but they were also sent just before playing a move to work *
 *           around a xboard protocol race condition where I can send a move,  *
 *           and then a draw claim (xboard 'result string') and the GUI sees   *
 *           my move, relays it to my opponent, and gets an instant reply that *
 *           it then sees before my draw claim.  After making this move and    *
 *           forwarding it to me, it now thinks the draw claim is invalid.     *
 *           slight tweak to queen material value and the way our bishop pair  *
 *           scoring is done.  The old razoring code has been removed.  It is  *
 *           redundant with LMR, and it had the unwanted side-effect of        *
 *           reducing a few of the moves two plies.  The futility pruning has  *
 *           been modified to work at the last N plies.  We have an array of   *
 *           pruning margins (tuned on the cluster) where anytime the depth is *
 *           less than "pruning_depth" we try the futility test.  If the test  *
 *           succeeds, that move is discarded and not searched at all.  Move   *
 *           ordering code modified slightly to call SEE() fewer times if the  *
 *           goodness of the capture can still be accurately assessed without  *
 *           the call.  Hash table has been changed to use a bucket of 4       *
 *           entries similar to a 4-way set associative cache.  When a store   *
 *           is attempted, we replace the least useful entry (see hash.c for   *
 *           details).  Additionally the hash table is forced to a 64 byte     *
 *           boundary alignment so that a bucket aligns on the start of a      *
 *           cache line for efficiency.  The old approach used 3 entries per   *
 *           bucket, the Belle two-table approach, which caused 1/2 of the     *
 *           cache probes to result in two cache line reads rather than just   *
 *           one.  Slight modification to "time overflow" logic.  Now, if the  *
 *           score drops on the iteration where we reach the normal time       *
 *           allocation, Crafty will continue to search.  Previously the score *
 *           had to drop by 1/4 pawn, but cluster testing has shown that this  *
 *           change is better.  Final bit of old rotated bitboard logic        *
 *           removed.  We had two bitboards, RooksQueens and BishopsQueens     *
 *           that were updated in MakeMove() and UnmakeMove().  It turned out  *
 *           to be faster to just OR the needed piece bitboards together since *
 *           they were infrequently used in the new "magic" approach, which    *
 *           allowed me to remove them completely from the position structure  *
 *           and make/unmake.  Net result was a percent or two faster search   *
 *           overall.  Old bug fixed related to EGTB usage.  The Edwards       *
 *           format did not include en passant pawn captures, so it was        *
 *           necessary to avoid probing any position where both sides had a    *
 *           pawn and there was any possibility of an en passant capture (one  *
 *           pawn on original square, enemy pawn anywhere on either adjacent   *
 *           file).  The Nalimov tables consider en passant correctly, but     *
 *           somehow this old code was either left in iterate.c or else added  *
 *           back in by accident.  It is now no longer present.  Minor changes *
 *           (suggested by J. Cleveland on CCC) that avoids trying a null-move *
 *           search when remaining depth == 1, and which saves some effort by  *
 *           remembering, when a node fails low, the first move searched.      *
 *           This move then gets stored in the hash table as the "best move".  *
 *           While this might be wrong, it has a chance of being right and     *
 *           when it is good enough to cause a cutoff, we can avoid a move     *
 *           generation which saves a bit of time.  Fixed a significant bug in *
 *           EvaluateWinningChances() where it was using a global variable     *
 *           "wtm" which tells who is to move in the real game, not at the     *
 *           current position in the treee.  This value  is used for tempo     *
 *           calculations to determine if the losing king (in a K + rook-pawn  *
 *           ending (with or without wrong bishop)) can reach the queening     *
 *           square before the winning side's king can block acccess.  This    *
 *           would randomly make the tempo calculation off by one, so that the *
 *           score could bounce from draw to win and back as the game          *
 *           progresses, since about 1/2 the time the wtm  value is wrong.  I  *
 *           simply passed the current wtm in as a formal parameter to solve   *
 *           this (a good example of why having a global variable and a formal *
 *           argument paramenter using the same name is a bad idea.)  In this  *
 *           same general area, EvaluateDraws() could set the score to         *
 *           DrawScore, which was somewhat "iffy".  I changed this to simply   *
 *           determine if the side that ahead is score has any realistic       *
 *           winning chances.  If not, I divide the score by 10 rather than    *
 *           setting it to the absolute drawscore.  This has the same effect,  *
 *           but does let the evaluation guide the search to better moves when *
 *           it is convinced all of them lead to draws.  This version is about *
 *           +100 Elo stronger than 23.0 based on cluster testing results.     *
 *                                                                             *
 *    23.2   Two changes related to draw handling.  First, the 50-move draw    *
 *           rule is tricky.  It is possible that the move on the 100th ply    *
 *           after the last non-reversible move might not be a draw, even if   *
 *           it is reversible itself, because it might deliver mate.  In this  *
 *           specific case, the mate will end the game as a win.  This has     *
 *           been fixed in Crafty to match this specific rule exception.  Also *
 *           draws by "insufficient material" have been changed to match the   *
 *           FIDE rules which say that mate can't be possible, even with worst *
 *           possible play by the opponent.  Crafty would claim a draw in a    *
 *           simple KN vs KN position, which is incorrect.  Even though        *
 *           neither side can force mate, mate is possible and so the position *
 *           is not a draw by FIDE rules.  Mobility scoring changed for        *
 *           sliding pieces.  Now the mobility scores are pre-computed and     *
 *           stored in a table that is addressed by the "magic number move     *
 *           generation idea".  This completely eliminates the computational   *
 *           cost of the mobility scoring since all of the scores are pre-     *
 *           computed before the game starts.  This was a modest speed         *
 *           improvement but really made the code simpler and smaller.  Change *
 *           to lazy eval cutoff code to improve accuracy.  BookUP() had a     *
 *           minor bug that caused it to not report how many moves were not    *
 *           included because of the "maxply" limit.  This has been fixed also *
 *           so that it now reports the correct value.  New xboard "cores" and *
 *           "memory" command support added.  Code to malloc() both was also   *
 *           rewritten completely to clean it up and simplify things.  In an   *
 *           effort to avoid confusion, there is one caveat here.  If your     *
 *           .craftyrc/crafty.rc file has an option to set the number of       *
 *           threads (mt=n/smpnt=n), then the "cores" option from xboard is    *
 *           automatically disabled so that the .craftyrc option effectively   *
 *           overrides any potential cores setting.  If you set hash or hashp  *
 *           in the .craftyrc/crafty.rc file, then the "memory" option from    *
 *           xboard is automatically disabled, for the same reason.  In short, *
 *           you can let xboard set the number of threads and memory usage     *
 *           limit, or you can set them in your .craftyrc/crafty.rc file.  But *
 *           if you use the .craftyrc/crafty.rc file to set either, then the   *
 *           corresponding xboard command will be disabled.                    *
 *                                                                             *
 *    23.3   Null-move search restriction changed to allow null-move searches  *
 *           at any node where the side on move has at least one piece of any  *
 *           type.  Minor bug in ValidMove() fixed to catch "backward" pawn    *
 *           moves and flag them as illegal.  This sometimes caused an error   *
 *           when annotating games and annotating for both sides.  The killer  *
 *           move array could have "backward" moves due to flipping sides back *
 *           and forth, which would cause some odd PV displays.  Small change  *
 *           to "reduce-at-root".  We no longer reduce moves that are flagged  *
 *           as "do not search in parallel".  Check extension modified so that *
 *           if "SEE" says the check is unsafe, the extension is not done.  We *
 *           found this to be worth about +10 Elo.  We also now reduce any     *
 *           capture move that appears in the "REMAINING_MOVES" phase since    *
 *           they can only appear there if SEE returns a score indicating loss *
 *           of material.  We now reduce a bit more aggressively, reducing by` *
 *           2 plies once we have searched at least 4 moves at any ply.  I     *
 *           tried going to 3 on very late moves, but could not find any case  *
 *           where this was better, even limiting it to near the root or other *
 *           ideas.  But reducing by 2 plies after the at least 2 moves are    *
 *           searched was an improvement.  Minor change is that the first move *
 *           is never reduced.  It is possible that there is no hash move, no  *
 *           non-losing capture, and no killer move.  That drops us into the   *
 *           REMAINING_MOVES phase which could reduce the first move searched, *
 *           which was not intended.  Very minor tweak, but a tweak all the    *
 *           same.                                                             *
 *                                                                             *
 *    23.4   Bug in hash implementation fixed.  It was possible to get a hash  *
 *           hit at ply=2, and alter the ply=1 PV when it should be left with  *
 *           no change.  This could cause Crafty to display one move as best   *
 *           and play another move entirely.  Usually this move was OK, but it *
 *           could, on occasion, be an outright blunder.  This has been fixed. *
 *           The search.c code has been re-written to eliminate SearchRoot()   *
 *           entirely, which simplifies the code.  The only duplication is in  *
 *           SearchParallel() which would be messier to eliminate.  Ditto for  *
 *           quiesce.c, which now only has Quiesce() and QuiesceEvasions().    *
 *           The old QuiesceChecks() has been combined with Quiesce, again to  *
 *           eliminate duplication and simplify the code.  Stupid bug in code  *
 *           that handles time-out.  One place checked the wrong variable and  *
 *           could cause a thread to attempt to output a PV after the search   *
 *           was in the process of timing out.  That thread could back up an   *
 *           incomplete/bogus search result to the root in rare occasions,     *
 *           which would / could result in Crafty kibitzing one move but       *
 *           playing a different move.  On occasion, the move played could be  *
 *           a horrible blunder.  Minor bug caused StoreTransRef() to lose a   *
 *           best move when overwriting an old position with a null-move       *
 *           search result.  Minor change by Tracy to lazy evaluation cutoff   *
 *           to speed up evaluation somewhat.  Old "Trojan Horse" attack       *
 *           detection was removed.  At today's depths, it was no longer       *
 *           needed.  New hash idea stores the PV for an EXACT hash entry in a *
 *           separate hash table.  When an EXACT hit happens, this PV can be   *
 *           added to the incomplete PV we have so far so that we have the     *
 *           exact path that leads to the backed up score.  New "phash" (path  *
 *           hash) command to set the number of entries in this path hash      *
 *           table.  For longer searches, a larger table avoids path table     *
 *           collisions which will produce those short paths that end in <HT>. *
 *           If <HT> appears in too many PVs, phash should be increased.  The  *
 *           "Trojan Horse" code has been completely removed, which also       *
 *           resulted in the removal of the last bit of "pre-processing code"  *
 *           in preeval.c, so it has been completely removed as well.  Current *
 *           search depths are such that the Trojan Horse code is simply not   *
 *           needed any longer.  A minor bug in TimeSet() fixed where on rare  *
 *           occasions, near the end of a time control, time_limit could be    *
 *           larger than the max time allowed (absolute_time_limit).  We never *
 *           check against this limit until we exceed the nominal time limit.  *
 *           Cleanup on the draw by repetition code to greatly simplify the    *
 *           code as well as speed it up.                                      *
 *                                                                             *
 *    23.5   Several pieces of code cleanup, both for readability and speed.   *
 *           We are now using stdint.h, which lets us specific the exact size  *
 *           of an integer value which gets rid of the int vs long issues that *
 *           exist between MSVC and the rest of the world on X86_64.  We still *
 *           use things like "int" if we want to let the compiler choose the   *
 *           most efficient size, but when we need a specific number of bits,  *
 *           such as 64, we use a specific type (uint64_t for 64 bits).  Minor *
 *           change to forward pruning that does not toss out passed pawn      *
 *           moves if they are advanced enough (compared to remaining depth)   *
 *           that they might be worth searching.  Cleanup to passed pawn eval  *
 *           code to help efficiency and readability.  New options to the      *
 *           display command.  "display nothing" turns all output off except   *
 *           for the move Crafty chooses to play.  "display everything" will   *
 *           produce a veritable flood of information before it makes a move.  *
 *           Cleanup of personality code to fix some values that were not      *
 *           included, as well as clean up the formatting for simplicity.      *
 *                                                                             *
 *    23.6   Minor tweak to "adaptive hash" code + a fix to the usage warning  *
 *           that failed to explain how many parameters are required.  New way *
 *           of timing the search, replacing the old "easy move" code that was *
 *           simply too ineffective.  Now Crafty computes a "difficulty" level *
 *           after each iteration.  If the best move doesn't change, this      *
 *           level is reduced by 10% of the current value, until it reaches    *
 *           60% where it stops dropping.  If the best move changes during an  *
 *           iteration, it adjusts in one of two ways.  If the current         *
 *           difficulty level is less than 100%, it reverts to 100% + the      *
 *           number of different best moves minus 1 times 20%.  If the current *
 *           difficulty level is already >= 100% it is set to 80% of the       *
 *           current value + (again) the number of different best moves - 1    *
 *           times 20%.  Repeated changes can run this up to 180% max.  As the *
 *           search progresses this difficulty level represents the percentage *
 *           of the nominal target time limit it should use.  It still tries   *
 *           to complete the current iteration before quitting, so this limit  *
 *           is a lower bound on the time it will use.  Restored an old idea   *
 *           from earlier Crafty versions (and even Cray Blitz), that of       *
 *           trying the killers from two plies earlier in the tree, once the   *
 *           killers for the current ply have been tried.  Was a +10 Elo gain, *
 *           added to about +10 for the new time control logic.  Old fail-high *
 *           restriction removed.  At one point in time, a fail-high on the    *
 *           null-window search at the root would cause problems.  Crafty was  *
 *           modified so that the fail-high was ignored if the re-search       *
 *           failed low.  Removing this produced an 8 Elo gain.                *
 *                                                                             *
 *    23.7   Minor fix to time check code.  Was particularly broken at very    *
 *           low skill level settings, but it had a small impact everywhere.   *
 *           Some simplification with the reduction code, and how moves are    *
 *           counted as they are searched, which would not count any moves     *
 *           that were pruned, which could delay triggering reductions at a    *
 *           ply.  Fixed significant killer move bug that would only surface   *
 *           during a parallel search.  NextMove (remaining moves phase) culls *
 *           killer moves as it selects, because they have already been tried. *
 *           Unfortunately, in a parallel search, the killers could get        *
 *           modified by other threads, which could erroneously cause a move   *
 *           to be excluded that had not been searched.  I discovered this bug *
 *           on wac #266 where Crafty normally finds a mate at depth=11, but   *
 *           a parallel search would often miss the mate completely.  Minor    *
 *           fix to new "difficulty" time management.  It was possible for a   *
 *           move to look "easy" (several iterations without finding a new     *
 *           best move) but it could fail low at the beginning of the next     *
 *           iteration.  Difficulty would be reduced due to not changing the   *
 *           best move at previous iterations, so the search could time out    *
 *           quicker than expected.  Simple fix was to reset difficulty to     *
 *           100% on a fail low, which makes sense anyway if it was supposed   *
 *           to be "easy" but the score is mysteriously dropping anyway.       *
 *                                                                             *
 *    23.8   MAXPLY changed to 128, which increases the max length of the PV's *
 *           that can be displayed.  The hash path stuff works so well that it *
 *           often STILL has PVs with <HT> on the end, after 64 moves are      *
 *           displayed.  While this can still probably be reached, it should   *
 *           be much less frequent.  This does mean the search can go for 128  *
 *           iterations, rather than the old 64 limit, of course.  Bug in      *
 *           Search() that failed to update the PV on an EGTB hit, which would *
 *           cause the hashed PV to contain illegal moves.  When Crafty then   *
 *           tried to display this PV, it would promptly crash.                *
 *                                                                             *
 *    24.0   Major changes to portability/platform specific code.  Crafty now  *
 *           supports Unix and Windows platforms.  All the other spaghetti     *
 *           code has been removed, with an effort to make the code readable.  *
 *           Couple of bugs in EvaluateWinningChances() fixed, one in          *
 *           particular that makes the krp vs kr draw detection unreachable.   *
 *           strcpy() replaced with memmove() to fix problems with Apple OS X  *
 *           Mavericks where strcpy() with operands that overlap now causes an *
 *           application to abort on the spot.  Bug in Search() that caused an *
 *           inefficiency in the parallel search.  At some point the way moves *
 *           are counted within a ply was changed, which changed the split     *
 *           algorithm unintentionally.  The original intent was to search     *
 *           just one move and then allow splits.  The bug changed this to     *
 *           search TWO moves before splitting is allowed, which slowed things *
 *           down quite a bit.  thread[i] is now an array of structures so     *
 *           that the thread pointers are separated from each other to fit     *
 *           into separate cache blocks, this avoids excessive cache traffic   *
 *           since they get changed regularly and were sitting in a single     *
 *           cache block with 8 cpus on 64 bit architectures.  Bug in new      *
 *           "difficulty" time usage code would let the program run up against *
 *           "absolute_time_limit" before stopping the search, even if we      *
 *           finished an iteration using beyond the normal target.  This       *
 *           really burned a lot of unnecessary time.  I had previously fixed  *
 *           the PVS window fail high (at the root) to force such a move to be *
 *           played in the game.  One unfortunate oversight however was that   *
 *           when splitting at the root, N different processors could fail     *
 *           high on N different moves, and they were searching those in       *
 *           parallel.  I've modified this so that the PVS fail high at the    *
 *           root exits from Search() (after stopping other busy threads) and  *
 *           then returns to Iterate() which will print the fail-high notice   *
 *           and re-start the search with all threads searching just one fail  *
 *           high move to get a score quicker.  Fail-high/fail-low code clean- *
 *           up.  In particular, the "delta" value to increment beta or        *
 *           decrement alpha is now two variables, so that an early fail-low   *
 *           won't increment the delta value and then the subsequent (hope-    *
 *           fully) fail-high starts off with a bad increment and jumps beta   *
 *           too quickly.  All fail-highs at the root return to Iterate(),     *
 *           even the PVS null-window fail-highs.  This re-starts the search   *
 *           with all processors working on that one move to get it resolved   *
 *           much quicker.  Other minor fixes such as when failing low on the  *
 *           first move, we pop out of search and go back to Iterate() to      *
 *           lower alpha and start a new search.  Unfortunately, after the     *
 *           first move is searched, a parallel split at the root would occur, *
 *           only to be instantly aborted so that we could back out of Search()*
 *           and then re-start.  Wasted the split time unnecessarily although  *
 *           it was not a huge amount of time.  Another simplification to the  *
 *           repetition-detection code while chasing a parallel search bug     *
 *           was missing repetitions.                                          *
 *                                                                             *
 *    24.1   LMR reductions changed to allow a more dynamic reduction amount   *
 *           rather than just 1 or 2.  An array LMR_reductions[depth][moves]   *
 *           is initialized at start-up and sets the reduction amount for a    *
 *           specific depth remaining (larger reduction with more depth left)  *
 *           and the total moves searched at this node (again a larger         *
 *           reduction as more moves are searched.)  This can easily be        *
 *           changed with the new "lmr" command (help lmr for details).  The   *
 *           old "noise" stuff has been changed.  Now, rather than noise       *
 *           specifying the number of nodes that have to be searched before    *
 *           any output is displayed (PV, fail high, etc) it now specifies a   *
 *           time that must pass before the first output is produced.  This    *
 *           can still be displayed with noise=0, otherwise noise=n says no    *
 *           output until n seconds have passed (n can be a fraction of a      *
 *           second if wanted.)  I restored the old adaptive null-move code,   *
 *           but changed the reduction (used to be 2 to 3 depending on the     *
 *           depth remaining) so that it is 3 + depth / 6.  For each 6 plies   *
 *           of remaining depth, we reduce by another ply, with no real bound  *
 *           other than the max depth we can reasonably reach.  The "6" in     *
 *           the above can be changed with the personality command or the new  *
 *           null m n command.  When the changes to noise were made, Iterate() *
 *           was also changed so that if the search terminates for any reason  *
 *           and no PV was displayed, whether because the noise limit was not  *
 *           satisfied or this search did not produce a PV because it started  *
 *           at an excessively deep search depth, Crafty now always displays   *
 *           at least one PV in the log file to show how the game is going to  *
 *           continue.  One case where this was an issue was where Crafty did  *
 *           a LONG ponder search because the opponent took an excessive       *
 *           amount of time to make a move.  If it finishes (say) a 30 ply     *
 *           search while pondering, when the opponent moves, Crafty will move *
 *           instantly, but when it starts the next search, we start pondering *
 *           at iteration 29, which might take a long time to finish.  But we  *
 *           do still have the PV from the last search (the 30 ply one) and    *
 *           main() thoughtfully removed the first two moves (the move played  *
 *           and the move we are now pondering, so we always have something to *
 *           display, and now we no longer terminate a search with no output   *
 *           (PV, time, etc) at all.  Generation II of the parallel split code *
 *           (see Thread() in the thread.c source file comments for specifics) *
 *           that is a lighter-weight split/copy plus the individual threads   *
 *           do most of the copying rather than the thread that initiates the  *
 *           split.  More work on the skill command.  For each 10 units the    *
 *           skill level is reduced, the search speed is reduced by 50%.       *
 *           Additionally the null-move and LMR reduction values are also      *
 *           ramped down so that by the time skill drops below 10, there are   *
 *           no reductions left.                                               *
 *                                                                             *
 *    24.2   Bug in SetTime() fixed.  The absolute time limit was set way too  *
 *           large in sudden death games (games with no secondary time control *
 *           to add time on the clock) with an increment.  It was intended to  *
 *           allow the search to use up to 5x the normal target time, unless   *
 *           that was larger than 1/2 of the total time remaining, but some-   *
 *           where along the way that 5x was removed and it always set the     *
 *           absolute time limit to 1/2 the remaining clock, which would burn  *
 *           clock time way too quickly.  Complete re-factor of Search() to    *
 *           take the main loop and move that to a new procedure               *
 *           SearchMoveList() to eliminate the duplicated search code between  *
 *           Search() and SearchParallel() which has been replaced by          *
 *           SearchMoveList().  New addition to NextMove().  It was cleaned up *
 *           to eliminate the old NextEvasion() code since one was a sub-set   *
 *           of the other.  A minor change to limit the NPS impact on history  *
 *           ordering.  We do not do history ordering within 6 plies of a      *
 *           terminal node, and we only try to order 1/2 of the moves before   *
 *           giving up and assuming this is an ALL node where ordering does    *
 *           not matter.                                                       *
 *                                                                             *
 *    25.0   Complete re-factor of pawn evaluation code.  There were too many  *
 *           overlapping terms that made tuning difficult.  Now a pawn is      *
 *           classified as one specific class, there is no overlap between     *
 *           classes, which simplifies the code significantly.  The code is    *
 *           now easier to understand and modify.  In addition, the passed     *
 *           pawn evaluation was rewritten and consolidates all the passed     *
 *           pawn evaluation in one place.  The evaluation used to add a bonus *
 *           for rooks behind passed pawns in rook scoring, blockading some-   *
 *           where else, etc.  All of this was moved to the passed pawn code   *
 *           to make it easier to understand and modify.  A limited version of *
 *           late move pruning (LMP) is used in the last two plies.  Once a    *
 *           set number of moves have been searched with no fail high, non-    *
 *           interesting moves are simply skipped in a way similar to futility *
 *           pruning.  This version also contains a major rewrite of the       *
 *           parallel search code, now referred to as Generation II.  It has a *
 *           more lightweight split algorithm, that costs the parent MUCH less *
 *           effort to split the search.  The key is that now the individual   *
 *           "helper" threads do all the work, allocating a split block,       *
 *           copying the data from the parent, etc., rather than the parent    *
 *           doing it all.  Gen II based on the DTS "late-join" idea so that a *
 *           thread will try to join existing split points before it goes to   *
 *           the idle wait loop waiting for some other thread to split with    *
 *           it.  In fact, this is now the basis for the new split method      *
 *           where the parent simply creates a split point by allocating a     *
 *           split block for itself, and then continuing the search, after the *
 *           parent split block is marked as "joinable".  Now any idle threads *
 *           just "jump in" without the parent doing anything else, which      *
 *           means that currently idle threads will "join" this split block if *
 *           they are in the "wait-for-work" spin loop, and threads that be-   *
 *           come idle also join in exactly the same way.  This is MUCH more   *
 *           efficient, and also significantly reduces the number of split     *
 *           blocks needed during the search.  We also now pre-split the       *
 *           search when we are reasonably close to the root of the tree,      *
 *           which is called a "gratuitous split.  This leaves joinable split  *
 *           points laying around so that whenever a thread becomes idle, it   *
 *           can join in at these pre-existing split points immediately.  We   *
 *           now use a much more conservative approach when dealing with fail  *
 *           highs at the root.  Since LMR and such have introduced greater    *
 *           levels of search instability, we no longer trust a fail-high at   *
 *           the root if it fails low on the re-search.  We maintain the last  *
 *           score returned for every root move, along with the PV.  Either an *
 *           exact score or the bound score that was returned.  At the end of  *
 *           the iteration, we sort the root move list using the backed-up     *
 *           score as the sort key, and we play the move with the best score.  *
 *           This solves a particularly ugly issue where we get a score for    *
 *           the first move, then another move fails high, but then fails low  *
 *           and the re-search produces a score that is actually WORSE than    *
 *           the original best move.  We still see that, but we always play    *
 *           the best move now.  One other efficiency trick is that when the   *
 *           above happens, the search would tend to be less efficient since   *
 *           the best score for that fail-high/fail-low move is actually worse *
 *           than the best move/score found so far.  If this happens, the      *
 *           score is restored to the original best move score (in Search())   *
 *           so that we continue searching with a good lower bound, not having *
 *           to deal with moves that would fail high with this worse value,    *
 *           but not with the original best move's value.  Minor change to     *
 *           history counters that now rely on a "saturating counter" idea.  I *
 *           wanted to avoid the aging idea, and it seemed to not be so clear  *
 *           that preferring history moves by the depth at which they were     *
 *           good was the way to go.  I returned to a history counter idea I   *
 *           tested around 2005 but discarded, namely using a saturating       *
 *           counter.  The idea is that a center value (at present 1024)       *
 *           represents a zero score.  Decrementing it makes it worse,         *
 *           incrementing it makes it better.  But to make it saturate at      *
 *           either end, I only reduce the counter by a fraction of its        *
 *           distance from the saturation point so that once it gets to either *
 *           extreme value, it will not be modified further avoiding wrap-     *
 *           around.  This basic idea was originally reported by Mark Winands  *
 *           in 2005.  It seems to provide better results (slightly) on very   *
 *           deep searches.  One impetus for this was an intent to fold this   *
 *           into a move so that I could sort the moves rather than doing the  *
 *           selection approach I currently use.  However, this had a bad      *
 *           effect on testing, since history information is dynamic and is    *
 *           constantly changing, between two moves at the same ply in fact.   *
 *           The sort fixed the history counters to the value at the start of  *
 *           that ply.  This was discarded after testing, but the history      *
 *           counter based on the saturating counter idea seemed to be OK and  *
 *           was left in even though it produced minimal Elo gain during       *
 *           testing.  Change to the way moves are counted, to add a little    *
 *           more consistency to LMR.  Now Next*() returns an order number     *
 *           that starts with 1 and monotonically increases, this order number *
 *           is used for LMR and such decisions that vary things based on how  *
 *           far down the move list something occurs.  Root move counting was  *
 *           particularly problematic with parallel searching, now things are  *
 *           at least "more consistent".  The only negative impact is that now *
 *           the move counter gets incremented even for illegal moves, but     *
 *           testing showed this was a no-change change with one thread, and   *
 *           the consistency with multiple threads made it useful.  New method *
 *           to automatically tune SMP parameters.  The command is autotune    *
 *           and "help autotune" will explain how to run it.  Added the        *
 *           "counter-move" heuristic for move ordering (Jos Uiterwijk, JICCA) *
 *           which simply remembers a fail high move and the move at the       *
 *           previous ply.  If the hash, captures or killer moves don't result *
 *           in a fail high, this move is tried next.  No significant cost,    *
 *           seems to reduce tree size noticeably.  Added a follow-up idea     *
 *           based on the same idea, except we pair a move that fails high     *
 *           with the move two plies back, introducing a sort of "connection"  *
 *           between them.  This is a sort of "plan" idea where the first move *
 *           of the pair enables the second move to fail high.  The benefit is *
 *           that this introduces yet another pair of good moves that get      *
 *           ordered before history moves, and is therefore not subject to     *
 *           reduction.  I have been unable to come up with a reference for    *
 *           this idea, but I believe I first saw it somewhere around the time *
 *           Fruit showed up, I am thinking perhaps in the JICCA/JICGA.  Any   *
 *           reference would be appreciated.  Minor change to the way the PV   *
 *           and fail-hi/fail-low moves are displayed when pondering.  Crafty  *
 *           now adds the ponder move to the front of the PV enclosed in ( )   *
 *           so that it is always visible in console mode.  The depths are     *
 *           reaching such extreme numbers the ponder move scrolls off the top *
 *           of the screen when running in console mode or when "tail -f" is   *
 *           used to watch the log file while a game is in progress.  This is  *
 *           a bit trickier than you might think since Crafty displays the     *
 *           game move numbers in the PV.  Penalty for pawns on same color as  *
 *           bishop now only applies when there is one bishop.                 *
 *                                                                             *
 *    25.1   Cleanup of NextMove() plus a minor ordering bug fix that would    *
 *           skip counter moves at ply = 2. Added NUMA code to force the hash  *
 *           tables to be spread across the numa nodes as equally as possible  *
 *           rather than all of the data sitting on just onenode.  This makes  *
 *           one specific user policy important.  BEFORE you set the hash size *
 *           for any of the four hash tables, you should ALWAYS set the max    *
 *           threads limit first, so that the NUMA trick works correctly.  Of  *
 *           course, if you do not use -DAFFINITY this is all irrelevant.  The *
 *           -DNUMA option has been removed.  I no longer use any libnuma      *
 *           routines.  A new "smpnuma" command is now used to enable/disable  *
 *           NUMA mode (which really only affects how the hash tables are      *
 *           cleared, all the other NUMA code works just fine no matter        *
 *           whether this is enabled or disabled.  Fixed a bug with the xboard *
 *           memory command that could overflow and cause preposterous malloc  *
 *           requests.  Change to LMP that now enables it in the last 16 plies *
 *           of search depth, although only the last 8-10 plies really have    *
 *           a chance for this to kick in unless there are more than 100 legal *
 *           moves to try.  Minor change to hash path in HashStore() that made *
 *           it hard to store entries on the first search after the table was  *
 *           cleared.  Removed Nalimov DTM EGTB code and converted to SYZYGY   *
 *           WDL/DTC tables instead (courtesy of Ronald de Man).  This         *
 *           correctly handles the 50 move rule  where the Nalimov tables      *
 *           would walk into forced draws (by 50 move rule) while convincing   *
 *           the search it was winning.  Swindle mode now also activates when  *
 *           in a drawn ending with a material plus for the side on move, as   *
 *           well as when the best root move is a "cursed win" (forced win,    *
 *           but drawn because of the 50 move rule).  This gives the non-EGTB  *
 *           opponent the opportunity to turn that 50 move draw into a loss.   *
 *           There are some changes in the scoring output as a result of this. *
 *           The usual +/-MatNN scores show up for real mates, but when in     *
 *           EGTB endings, the scores are of the form Win or Lose with the     *
 *           appropriate sign correction (-Win means black is winning, +Lose   *
 *           means white is losing.)  Basil Falcinelli contributed to the new  *
 *           syzygy code used in this version.  Minor change to skill code to  *
 *           avoid altering search parameters since the speed reduction and    *
 *           randomness in the eval is more than enough to reduce the Elo.     *
 *           Minor change to HashProbe() where I now only update the AGE of an *
 *           entry that matches the current hash signature if the entry        *
 *           actually causes a search termination, rather than updating it     *
 *           each time there is a signature match.  If the search is not       *
 *           terminated on the spot, we have to store an entry when the search *
 *           ends which will also overwrite the current exact match and update *
 *           the age as well.  Suggested by J. Wesley Cleveland on CCC.        *
 *                                                                             *
 *    25.2   Minor bug in the fail-high / fail-low code.  Crafty is supposed   *
 *           to deal with the case where the first move produces a score, then *
 *           a later move fails high but then produces a worse score.  We are  *
 *           supposed to revert to the better move.  An "optimization" made    *
 *           this fail, but it has been fixed here.  "new" command removed as  *
 *           it is pretty difficult to restore everything once a game has been *
 *           started.  To start a new game, quit crafty and restart.  Crafty   *
 *           now notifies xboard/winboard to do this automatically so using    *
 *           those interfaces requires no changes to anything.                 *
 *                                                                             *
 *******************************************************************************
 */
int main(int argc, char **argv) {
  TREE *tree;
  FILE *personality;
  int move, readstat, value = 0, i, v, result, draw_type;
  char announce[128];

#if defined(UNIX)
  char path[1024];
  struct passwd *pwd;
#else
  char crafty_rc_file_spec[FILENAME_MAX];
  SYSTEM_INFO sysinfo;
#endif
#if defined(UNIX)
  hardware_processors = sysconf(_SC_NPROCESSORS_ONLN);
#else
  GetSystemInfo(&sysinfo);
  hardware_processors = sysinfo.dwNumberOfProcessors;
#endif
/* Collect environmental variables */
  char *directory_spec = getenv("CRAFTY_BOOK_PATH");

  if (directory_spec)
    strncpy(book_path, directory_spec, sizeof book_path);
  directory_spec = getenv("CRAFTY_LOG_PATH");
  if (directory_spec)
    strncpy(log_path, directory_spec, sizeof log_path);
  directory_spec = getenv("CRAFTY_TB_PATH");
  if (directory_spec)
    strncpy(tb_path, directory_spec, sizeof tb_path);
  directory_spec = getenv("CRAFTY_RC_PATH");
  if (directory_spec)
    strncpy(rc_path, directory_spec, sizeof rc_path);
  if (getenv("CRAFTY_XBOARD")) {
    Print(-1, "feature done=0\n");
    xboard_done = 0;
  } else if (argc > 1 && !strcmp(argv[1], "xboard")) {
    Print(-1, "feature done=0\n");
    xboard_done = 0;
  }
/*
 ************************************************************
 *                                                          *
 *  First, parse the command-line options and pick off the  *
 *  ones that need to be handled before any initialization  *
 *  is attempted (mainly path commands at present.)         *
 *                                                          *
 ************************************************************
 */
  AlignedMalloc((void *) ((void *) &tree), 2048, (size_t) sizeof(TREE));
  block[0] = tree;
  memset((void *) block[0], 0, sizeof(TREE));
  tree->ply = 1;
  input_stream = stdin;
  for (i = 0; i < 512; i++)
    args[i] = (char *) malloc(256);
  if (argc > 1) {
    for (i = 1; i < argc; i++) {
      if (strstr(argv[i], "path") || strstr(argv[i], "log") ||
          strstr(argv[1], "affinity")) {
        strcpy(buffer, argv[i]);
        result = Option(tree);
        if (result == 0)
          Print(2048, "ERROR \"%s\" is unknown command-line option\n",
              buffer);
        display = tree->position;
      }
    }
  }
/*
 ************************************************************
 *                                                          *
 *  Now, read the crafty.rc/.craftyrc initialization file   *
 *  and process the commands that need to be handled prior  *
 *  to initializing things (path commands).                 *
 *                                                          *
 ************************************************************
 */
#if defined(UNIX)
  input_stream = fopen(".craftyrc", "r");
  if (!input_stream)
    if ((pwd = getpwuid(getuid()))) {
      sprintf(path, "%s/.craftyrc", pwd->pw_dir);
      input_stream = fopen(path, "r");
    }
  if (input_stream)
#else
  sprintf(crafty_rc_file_spec, "%s/crafty.rc", rc_path);
  if ((input_stream = fopen(crafty_rc_file_spec, "r")))
#endif
    while (1) {
      readstat = Read(1, buffer);
      if (readstat < 0)
        break;
      if (!strstr(buffer, "path"))
        continue;
      result = Option(tree);
      if (result == 0)
        Print(2048, "ERROR \"%s\" is unknown rc-file option\n", buffer);
      if (input_stream == stdin)
        break;
    }
/*
 ************************************************************
 *                                                          *
 *   Initialize all data arrays and chess board.            *
 *                                                          *
 ************************************************************
 */
  Initialize();
/*
 ************************************************************
 *                                                          *
 *  Now, parse the command-line options and pick off the    *
 *  ones that need to be handled after initialization is    *
 *  completed.                                              *
 *                                                          *
 ************************************************************
 */
  if (argc > 1) {
    for (i = 1; i < argc; i++)
      if (strcmp(argv[i], "c"))
        if (!strstr(argv[i], "path")) {
          if (strlen(argv[i]) > 255)
            Print(-1, "ERROR ignoring token %s, 255 character max\n",
                argv[i]);
          else {
            strcpy(buffer, argv[i]);
            Print(32, "(info) command line option \"%s\"\n", buffer);
            result = Option(tree);
            if (result == 0)
              Print(2048, "ERROR \"%s\" is unknown command-line option\n",
                  buffer);
          }
        }
  }
  display = tree->position;
  initialized = 1;
  move_actually_played = 0;
/*
 ************************************************************
 *                                                          *
 *  Next, read the crafty.rc/.craftyrc initialization file  *
 *  and process the commands that need to be handled after  *
 *  engine initialization has been completed.               *
 *                                                          *
 ************************************************************
 */
#if defined(UNIX)
  input_stream = fopen(".craftyrc", "r");
  if (!input_stream)
    if ((pwd = getpwuid(getuid()))) {
      sprintf(path, "%s/.craftyrc", pwd->pw_dir);
      input_stream = fopen(path, "r");
    }
  if (input_stream) {
#else
  sprintf(crafty_rc_file_spec, "%s/crafty.rc", rc_path);
  if ((input_stream = fopen(crafty_rc_file_spec, "r"))) {
#endif
    while (1) {
      readstat = Read(1, buffer);
      if (readstat < 0)
        break;
      if (strstr(buffer, "path"))
        continue;
      result = Option(tree);
      if (result == 0)
        Print(2048, "ERROR \"%s\" is unknown rc-file option\n", buffer);
      if (input_stream == stdin)
        break;
    }
  }
  input_stream = stdin;
#if defined(UNIX)
  if (xboard)
    signal(SIGINT, SIG_IGN);
#endif
  if (smp_max_threads)
    Print(32, "\nCrafty v%s (%d threads)\n\n", version, smp_max_threads);
  else
    Print(32, "\nCrafty v%s\n\n", version);
  if (hardware_processors > 0)
    Print(32, "machine has %d processors\n\n", hardware_processors);

/*
 ************************************************************
 *                                                          *
 *  Check to see if we can find a "crafty.cpf" personality  *
 *  file which contains the default personality settings to *
 *  be used unless overridden by the user.                  *
 *                                                          *
 ************************************************************
 */
  if ((personality = fopen("crafty.cpf", "r"))) {
    fclose(personality);
    Print(-1, "using default personality file \"crafty.cpf\"\n");
    sprintf(buffer, "personality load crafty.cpf");
    Option(tree);
  }
/*
 ************************************************************
 *                                                          *
 *  This is the "main loop" that never ends unless the user *
 *  tells Crafty to "quit".  We read commands/moves,        *
 *  execute them, and when a move is entered we change      *
 *  sides and execute a search to find a reply.  We repeat  *
 *  this until the game ends or the opponent gets tired and *
 *  tells us to terminate the engine.                       *
 *                                                          *
 *  Prompt user and read input string for processing.  As   *
 *  long as Option() returns a non-zero value, continue     *
 *  reading lines and calling Option().  When Option()      *
 *  fails to recogize a command, then try InputMove() to    *
 *  determine if this is a legal move.                      *
 *                                                          *
 *  While we are waiting on a move, we call Ponder() which  *
 *  will find a move that it assumes the opponent will      *
 *  make, and then it will call Iterate() to find a reply   *
 *  for that move while we are waiting to see what is       *
 *  actually played in the game.  Ponder() will return one  *
 *  of the following "results" (stored in "presult"):       *
 *                                                          *
 *  (0) This indicates that Ponder() did not run, either    *
 *      because pondering is disabled, or we are not in a   *
 *      state where pondering is allowed, such as using the *
 *      xboard "force" command to set up a game that will   *
 *      be continued.                                       *
 *                                                          *
 *  (1) This indicates that we pondered a move, the         *
 *      opponent actually played this move, and we were     *
 *      able to complete a normal search for the proper     *
 *      amount of time and have a move ready to use.        *
 *                                                          *
 *  (2) This indicates that we pondered a move, but that    *
 *      for some reason, we did not need to continue until  *
 *      time ran out.  For example, we found a forced mate  *
 *      and further searching is not needed.  The search is *
 *      not "in progress" but we still have a valid move    *
 *      to play here.                                       *
 *                                                          *
 *  (3) We pondered, but the opponent either played a       *
 *      different move, or else entered a command that      *
 *      forces us to unwind the search so that the command  *
 *      can be executed.  In this case, we will re-start    *
 *      pondering, otherwise we make the correct move and   *
 *      then start a normal search.                         *
 *                                                          *
 ************************************************************
 */
/*
 ************************************************************
 *                                                          *
 *  From this point forward, we are in a state where it is  *
 *                                                          *
 *             O P P O N E N T ' S turn to move.            *
 *                                                          *
 ************************************************************
 */
  while (1) {
    presult = 0;
    do {
      opponent_start_time = ReadClock();
      input_status = 0;
      display = tree->position;
      move = 0;
      presult = 0;
      if (xboard_done == 0 && xboard) {
        xboard_done = 1;
        Print(-1, "feature done=1\n");
      }
      do {
        if (presult != 2)
          presult = 0;
        result = 0;
        if (pong) {
          Print(-1, "pong %d\n", pong);
          pong = 0;
        }
        display = tree->position;
        if (presult != 2 && (move_number != 1 || !game_wtm))
          presult = Ponder(game_wtm);
        if (presult == 1)
          value = last_root_value;
        else if (presult == 2)
          value = ponder_value;
        if (presult == 0 || presult == 2) {
          if (!xboard) {
            printf("%s(%d): ", SideToMove(game_wtm), move_number);
            fflush(stdout);
          }
          readstat = Read(1, buffer);
          if (log_file)
            fprintf(log_file, "%s(%d): %s\n", SideToMove(game_wtm),
                move_number, buffer);
          if (readstat < 0 && input_stream == stdin) {
            strcpy(buffer, "end");
            Option(tree);
          }
        }
        if (presult == 1)
          break;
        opponent_end_time = ReadClock();
        result = Option(tree);
        if (result == 0) {
          nargs = ReadParse(buffer, args, " \t;");
          move = InputMove(tree, 0, game_wtm, 0, 0, args[0]);
          result = !move;
          if (move)
            last_pv.path[1] = 0;
        } else {
          input_status = 0;
          if (result == 3)
            presult = 0;
        }
      } while (result > 0);
      if (presult == 1)
        move = ponder_move;
/*
 ************************************************************
 *                                                          *
 *  We have a move.  Make the move (returned by InputMove)  *
 *  and then change the side on move (wtm).                 *
 *                                                          *
 ************************************************************
 */
      if (result == 0) {
        fseek(history_file, ((move_number - 1) * 2 + 1 - game_wtm) * 10,
            SEEK_SET);
        fprintf(history_file, "%9s\n", OutputMove(tree, 0, game_wtm, move));
        MakeMoveRoot(tree, game_wtm, move);
        tree->curmv[0] = move;
        time_used_opponent = opponent_end_time - opponent_start_time;
        if (!force)
          Print(1, "              time used: %s\n",
              DisplayTime(time_used_opponent));
        TimeAdjust(game_wtm, time_used_opponent);
        game_wtm = Flip(game_wtm);
        if (game_wtm)
          move_number++;
        move_actually_played = 1;
        last_opponent_move = move;
/*
 ************************************************************
 *                                                          *
 *  From this point forward, we are in a state where it is  *
 *                                                          *
 *              C R A F T Y ' S turn to move.               *
 *                                                          *
 ************************************************************
 */
/*
 ************************************************************
 *                                                          *
 *  We have made the indicated move, before we do a search  *
 *  we need to determine if the present position is a draw  *
 *  by rule.  If so, Crafty allowed it to happen and must   *
 *  be satisfied with a draw, so we will claim it and end   *
 *  the current game.                                       *
 *                                                          *
 ************************************************************
 */
        if ((draw_type = Repeat3x(tree)) == 1) {
          Print(1, "I claim a draw by 3-fold repetition.\n");
          value = DrawScore(game_wtm);
          if (xboard)
            Print(-1, "1/2-1/2 {Drawn by 3-fold repetition}\n");
        }
        if (draw_type == 2) {
          if (!Mated(tree, 0, game_wtm)) {
            Print(1, "I claim a draw by the 50 move rule.\n");
            value = DrawScore(game_wtm);
            if (xboard)
              Print(-1, "1/2-1/2 {Drawn by 50-move rule}\n");
          }
        }
        if (Drawn(tree, last_search_value) == 2) {
          Print(1, "I claim a draw due to insufficient material.\n");
          if (xboard)
            Print(-1, "1/2-1/2 {Insufficient material}\n");
        }
      } else
        presult = 0;
#if defined(DEBUG)
      ValidatePosition(tree, 0, move, "Main(1)");
#endif
    } while (force);
/*
 ************************************************************
 *                                                          *
 *  Now call Iterate() to compute a move for the current    *
 *  position.  (Note this is not done if Ponder() has al-   *
 *  ready computed a move.)                                 *
 *                                                          *
 ************************************************************
 */
    crafty_is_white = game_wtm;
    if (presult == 2) {
      if (From(ponder_move) == From(move) && To(ponder_move) == To(move)
          && Piece(ponder_move) == Piece(move)
          && Captured(ponder_move) == Captured(move)
          && Promote(ponder_move) == Promote(move)) {
        presult = 1;
        if (!book_move)
          predicted++;
      } else
        presult = 0;
    }
    ponder_move = 0;
    thinking = 1;
    if (presult != 1) {
      strcpy(kibitz_text, "n/a");
      last_pv.pathd = 0;
      last_pv.pathl = 0;
      display = tree->position;
      tree->status[1] = tree->status[0];
      value = Iterate(game_wtm, think, 0);
    }
/*
 ************************************************************
 *                                                          *
 *  We've now completed a search and need to handle a       *
 *  pending draw offer based on what the search found.      *
 *                                                          *
 *  When we get a draw offer, we make a note, but we do not *
 *  accept it until after we have a chance to see what      *
 *  happens after making the opponent's move that came with *
 *  the draw offer.  If he played a blunder, we are not     *
 *  going to mistakenly accept a draw when we are now       *
 *  winning.  We make this decision to accept or decline    *
 *  since we have completed as search and now have a real   *
 *  score.                                                  *
 *                                                          *
 ************************************************************
 */
    if (draw_offer_pending) {
      int drawsc = abs_draw_score;

      draw_offer_pending = 0;
      if (move_number < 40 || !accept_draws)
        drawsc = -300;
      if (value <= drawsc && (tc_increment != 0 ||
              tc_time_remaining[Flip(game_wtm)] >= 1000)) {
        if (xboard)
          Print(-1, "offer draw\n");
        else {
          Print(1, "Draw accepted.\n");
          if (audible_alarm)
            printf("%c", audible_alarm);
          if (speech) {
            strcpy(announce, "./speak ");
            strcat(announce, "Drawaccept");
            v = system(announce);
            if (v != 0)
              perror("main() system() error: ");
          }
        }
        Print(-1, "1/2-1/2 {Draw agreed}\n");
        strcpy(pgn_result, "1/2-1/2");
      } else {
        if (!xboard) {
          Print(1, "Draw declined.\n");
          if (speech) {
            strcpy(announce, "./speak ");
            strcat(announce, "Drawdecline");
            v = system(announce);
            if (v != 0)
              perror("main() system() error: ");
          }
        }
      }
    }
/*
 ************************************************************
 *                                                          *
 *  If the last_pv.path is null, then we were either        *
 *  checkmated or stalemated.  We need to determine which   *
 *  and end the game appropriately.                         *
 *                                                          *
 *  If we do have a PV, we will also check to see if it     *
 *  leads to mate and make the proper announcement if so.   *
 *                                                          *
 ************************************************************
 */
    last_pv = tree->pv[0];
    last_value = value;
    if (MateScore(last_value))
      last_mate_score = last_value;
    thinking = 0;
    if (!last_pv.pathl) {
      if (Check(game_wtm)) {
        over = 1;
        if (game_wtm) {
          Print(-1, "0-1 {Black mates}\n");
          strcpy(pgn_result, "0-1");
        } else {
          Print(-1, "1-0 {White mates}\n");
          strcpy(pgn_result, "1-0");
        }
        if (speech) {
          strcpy(announce, "./speak ");
          strcat(announce, "Checkmate");
          v = system(announce);
          if (v != 0)
            perror("main() system() error: ");
        }
      } else {
        over = 1;
        if (!xboard) {
          Print(1, "stalemate\n");
          if (speech) {
            strcpy(announce, "./speak ");
            strcat(announce, "Stalemate");
            v = system(announce);
            if (v != 0)
              perror("main() system() error: ");
          }
        } else
          Print(-1, "1/2-1/2 {stalemate}\n");
      }
    } else {
      if (value > 32000 && value < MATE - 2) {
        Print(1, "\nmate in %d moves.\n\n", (MATE - value) / 2);
        Kibitz(1, game_wtm, 0, 0, (MATE - value) / 2, tree->nodes_searched, 0,
            0, " ");
      } else if (-value > 32000 && -value < MATE - 1) {
        Print(1, "\nmated in %d moves.\n\n", (MATE + value) / 2);
        Kibitz(1, game_wtm, 0, 0, -(MATE + value) / 2, tree->nodes_searched,
            0, 0, " ");
      }
/*
 ************************************************************
 *                                                          *
 *  See if we want to offer a draw based on the recent      *
 *  scores that have been returned.  See resign.c for more  *
 *  details, but this is where we offer a draw, or resign,  *
 *  if appropriate.  This has nothing to do with claiming a *
 *  draw by rule, which is done later.                      *
 *                                                          *
 ************************************************************
 */
      tree->status[MAXPLY] = tree->status[0];
      ResignOrDraw(tree, value);
/*
 ************************************************************
 *                                                          *
 *  Now output the move chosen by the search, and the       *
 *  "result" string if this move checkmates our opponent.   *
 *                                                          *
 ************************************************************
 */
      if (speech) {
        char *moveptr = OutputMove(tree, 0, game_wtm, last_pv.path[1]);

        strcpy(announce, "./speak ");
        strcat(announce, moveptr);
        strcat(announce, " &");
        v = system(announce);
        if (v != 0)
          perror("main() system() error: ");
      }
      if (!xboard && audible_alarm)
        printf("%c", audible_alarm);
      Print(1, "%s(%d): %s\n", SideToMove(game_wtm), move_number,
          OutputMove(tree, 0, game_wtm, last_pv.path[1]));
      if (xboard)
        printf("move %s\n", OutputMove(tree, 0, game_wtm, last_pv.path[1]));
      fflush(stdout);
      if (value == MATE - 2) {
        if (game_wtm) {
          Print(-1, "1-0 {White mates}\n");
          strcpy(pgn_result, "1-0");
        } else {
          Print(-1, "0-1 {Black mates}\n");
          strcpy(pgn_result, "0-1");
        }
      }
      time_used = program_end_time - program_start_time;
      Print(1, "              time used: %s\n", DisplayTime(time_used));
      TimeAdjust(game_wtm, time_used);
      fseek(history_file, ((move_number - 1) * 2 + 1 - game_wtm) * 10,
          SEEK_SET);
      fprintf(history_file, "%9s\n", OutputMove(tree, 0, game_wtm,
              last_pv.path[1]));
      last_search_value = value;
      if (kibitz) {
        if (kibitz_depth)
          Kibitz(2, game_wtm, kibitz_depth, end_time - start_time, value,
              tree->nodes_searched, busy_percent, tree->egtb_hits,
              kibitz_text);
        else
          Kibitz(4, game_wtm, 0, 0, 0, 0, 0, 0, kibitz_text);
      }
      MakeMoveRoot(tree, game_wtm, last_pv.path[1]);
      tree->curmv[0] = move;
/*
 ************************************************************
 *                                                          *
 *  From this point forward, we are in a state where it is  *
 *                                                          *
 *          O P P O N E N T ' S turn to move.               *
 *                                                          *
 *  We have made the indicated move, we need to determine   *
 *  if the present position is a draw by rule.  If so, we   *
 *  need to send the appropriate game result to xboard      *
 *  and/or inform the operator/opponent.                    *
 *                                                          *
 ************************************************************
 */
      game_wtm = Flip(game_wtm);
      if (game_wtm)
        move_number++;
      move_actually_played = 1;
      if ((draw_type = Repeat3x(tree)) == 1) {
        Print(1, "I claim a draw by 3-fold repetition after my move.\n");
        if (xboard)
          Print(-1, "1/2-1/2 {Drawn by 3-fold repetition}\n");
        value = DrawScore(game_wtm);
      }
      if (draw_type == 2 && last_search_value < 32000) {
        Print(1, "I claim a draw by the 50 move rule after my move.\n");
        if (xboard)
          Print(-1, "1/2-1/2 {Drawn by 50-move rule}\n");
        value = DrawScore(game_wtm);
      }
      if (Drawn(tree, last_search_value) == 2) {
        Print(1,
            "I claim a draw due to insufficient material after my move.\n");
        if (xboard)
          Print(-1, "1/2-1/2 {Insufficient material}\n");
      }
#if !defined(TEST)
      if (time_limit > 300)
#endif
        if (log_file)
          DisplayChessBoard(log_file, tree->position);
/*
 ************************************************************
 *                                                          *
 *  Save the ponder_move from the current principal         *
 *  variation, then shift it left two moves to use as the   *
 *  starting point for the next search.  Adjust the depth   *
 *  to start the next search at the right iteration.        *
 *                                                          *
 ************************************************************
 */
      if (last_pv.pathl > 2 && VerifyMove(tree, 0, game_wtm, last_pv.path[2])) {
        ponder_move = last_pv.path[2];
        for (i = 1; i < (int) last_pv.pathl - 2; i++)
          last_pv.path[i] = last_pv.path[i + 2];
        last_pv.pathl = (last_pv.pathl > 2) ? last_pv.pathl - 2 : 0;
        last_pv.pathd -= 2;
        if (last_pv.pathd > last_pv.pathl)
          last_pv.pathd = last_pv.pathl;
        if (last_pv.pathl == 0)
          last_pv.pathd = 0;
        tree->pv[0] = last_pv;
      } else {
        last_pv.pathd = 0;
        last_pv.pathl = 0;
        ponder_move = 0;
        tree->pv[0] = last_pv;
      }
    }
#if defined(TEST)
    strcpy(buffer, "score");
    Option(tree);
#endif
    if ((i = GameOver(game_wtm))) {
      if (i == 1)
        Print(-1, "1/2-1/2 {stalemate}\n");
    }
    if (book_move) {
      moves_out_of_book = 0;
      predicted++;
      if (ponder_move)
        sprintf(book_hint, "%s", OutputMove(tree, 0, game_wtm, ponder_move));
    } else
      moves_out_of_book++;
#if defined(DEBUG)
    ValidatePosition(tree, 0, last_pv.path[1], "Main(2)");
#endif
/*
 ************************************************************
 *                                                          *
 *  Now execute LearnValue() to record the scores for the   *
 *  first N searches out of book.  see learn.c for details  *
 *  on how this is used and when.                           *
 *                                                          *
 ************************************************************
 */
    if (learning && moves_out_of_book && !learn_value)
      LearnValue(last_value, last_pv.pathd + 2);
    if (learn_positions_count < 63) {
      learn_seekto[learn_positions_count] = book_learn_seekto;
      learn_key[learn_positions_count] = book_learn_key;
      learn_nmoves[learn_positions_count++] = book_learn_nmoves;
    }
    if (mode == tournament_mode) {
      strcpy(buffer, "clock");
      Option(tree);
      Print(32, "if clocks are wrong, use 'settc' command to adjust them\n");
    }
  }
}

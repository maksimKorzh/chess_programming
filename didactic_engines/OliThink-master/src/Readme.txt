OliThink5 (c) Oliver Brausch 17.Sep.2020, ob112@web.de, http://brausch.org

Version: 5.7.9
Protocol: Winboard 2
HashSize: 128MB
Ponder: Yes
OpeningBook: Small
EndgameTables: No
AnalyzeMode: Basic
SearchMethods: Nullmove, Internal Iterative Reduction, Check Extension, LMR
Evaluation: Just mobility and a very simple pawnprogressing evaluation
LinesOfCode: 1606
Stability: 100%
Special thanks to Dann Corbit for his support and contribution.

v5.7.9: changes since 5.7.8:
Refactor movelist in generate. History heuristics color dependent.

v5.7.8: changes since 5.7.7:
Reverse futility pruning. Quiesce alpha buffer to 85.

v5.7.7: changes since 5.7.6:
SEE fix.

v5.7.6: changes since 5.7.5:
No reduction of queen attacks.

v5.7.5: changes since 5.7.4:
Remove matekillers. Dynamic reduction depth.

v5.7.4: changes since 5.7.3:
Purge hash when new game. Exact Bound. End game factor.

v5.7.3: changes since 5.7.2:
Increase queen mobility, change pawn eval. Implement ping command.

v5.7.2: changes since 5.7.1:
Use intrinsic function _tzcnt to get least significant bit.

v5.7.1: changes since 5.7.0:
Fix pinnned-pawn-under-promotion bug. Change in time management.

v5.7.0: changes since 5.6.9:
Simplify pawn and king eval. Improve mate detection.

v5.6.9: changes since 5.6.8:
Increase mobile eval of knights. Use built-in popcount.

v5.6.8: changes since 5.6.7:
Pawns reduce attack bonus on king.

v5.6.7: changes since 5.6.6:
More bonus for attack on king's squares. No nullmove on pvnode. Quiesce puffer to 125. Simplify pawn evaluation.

v5.6.6: changes since 5.6.5:
Implementing Internal Iterative Reduction instead of Deepening. Idea by Ed Schröder.

v5.6.5: changes since 5.6.4:
Changes to hashtable.

v5.6.4: changes since 5.6.3:
Simplify hashtable and gain strength.

v5.6.3: changes since 5.6.2:
Simpler hashkey. Remove unsued draw/resign option. Other mini changes.

v5.6.2: changes since 5.6.1:
Matekillers only prevent reduction. pv refactor and fix time management when pondering.

v5.6.1: changes since 5.6.0:
Matekillers. Progress to 7th rank is not quiet.

v5.6.0: changes since 5.5.9:
Minor changes that yield better results in tests.

v5.5.9: changes since 5.5.8:
Redo some former changes in order to gain strength.

v5.5.8: changes since 5.5.7:
Bugfix in time management.

v5.5.7: changes since 5.5.6:
Increasing of static evaluation buffer in quiesce.

v5.5.6: changes since 5.5.5:
More aggressive null move pruning. Fix fen reading bug.

v5.5.5: changes since 5.5.4:
L RN and RB vs R is draw. Lazy evaluation after implementing incremential none-pawn counting. RN and RB vs R is draw.

v5.5.4: changes since 5.5.3:
Simplify mobility evaluation, display node counter in 64bit.

v5.5.3: changes since 5.5.2:
Simplify var declarations, NBK vs k eval and LMR. Null-move pruning for PVnodes.   

v5.5.2: changes since 5.5.1:
Extend Queen Promotions, don't reduce any promotion. Win NBK vs k.

v5.5.1: changes since 5.5.0:
Substantial changes in pawn evaluation. Fix value bug on seach abort.

v5.5.0: changes since 5.4.13a:
Refactor time management.

v5.4.13a: changes since 5.4.13:
Refactor hashtable storage lower bound in search.

v5.4.13: changes since 5.4.12:
Clear the depth-dependent hashtable after each move. Increase the size of this table.

v5.4.12: changes since 5.4.11:
Reduce search for captures with negative SEE.

v5.4.11: changes since 5.4.10:
Several protocol fixes. Support for resign and offer draw.

v5.4.10: changes since 5.4.9:
Reusing data when undoing a move in analyze mode or after pondering.

v5.4.9: changes since 5.4.8:
Mobilitiy change: No blocking of queen in the same direction. Less score for attacking squares near king.

v5.4.8: changes since 5.4.7:
Queen doesn't block Rook or Bishop mobility. Change eval for no mating material.  

v5.4.7: changes since 5.4.6:
Option "-reset" instead of "-forceponder". Reinstate hanging pawn eval.

v5.4.6: changes since 5.4.5:
Adaption in printouts, protocols and engine reuse. Remove asymetrical eval.

v5.4.5: changes since 5.4.4:
Some issues about the printout of moves and evaluations have been fixed. It could fix a bug, where a strange move has been made.

v5.4.4: changes since 5.4.3:
Fixed "Bus Error 10" for plies greater than 63. Max depth increased to 64.

v5.4.3: changes since 5.4.2:
Aggressive pruning with history heuristic. Bugfix when claiming draw for insufficient material. Reads fen-string without command.

v5.4.2: changes since 5.4.1:
History heuristics increase with depth

v5.4.1: changes since 5.4.0:
Implementing a very simple Move Count Pruning

v5.4.0: changes since 5.3.5f:
Refactoring of "No mating material" which yields gains in strength

v5.3.5f: changes since 5.3.5c:
Option -forceponder. Pvlength = 128. Preparing code for corrected "No mating matieral"

v5.3.5c: changes since 5.3.5:
Reducing Hashsize again to 48MB. More flexibel reading of opening book. Selective openings for win and loss.

v5.3.5: changes since 5.3.4:
Analyze mode and Mate score.

v5.3.4: changes since 5.3.3:
Expanding HashSize to 512MB

v5.3.3: changes since 5.3.2:
Eliminating bug in getDir thanks to Francisto Modesto

v5.3.2: changes since 5.3.1:
Splitting of move generation in non-captures and captures.
Removed small bug in move-generator

v5.3.1: changes since 5.3.0:
Removed bug in quiesce

v5.3.0: changes since 5.2.9:
No LMR for passed pawns

v5.2.9: changes since 5.2.8:
More flexible reducing in Null Move Pruning

v5.2.8: changes since 5.2.7:
Removal of Singular Reply Extension
Changes in the endgame mobility evaluation

v5.2.7: changes since 5.2.6:
Added a discount for blocked pawn in the evaluation

v5.2.6: changes since 5.2.5:
Successful implementation of Late Move Reduction (LMR)

v5.2.5: changes since 5.2.4:
Storing moves history and flags for whole match
Bugfix in Internal Iterative Deepening
Bugfix when finishing matches with ponder=on

v5.2.4: changes since 5.2.3:
Pondering changed to save more time during search
Additional commands of Xboard protocol

v5.2.3: changes since 5.2.2:
At root the first move searched is from the pv and not a hash move

v5.2.2: changes since 5.2.1:
HashSize reduced from 128MB to 48MB

v5.2.1: changes since 5.2.0:
buggy search extension line removed (following the bug correction)
small bug in move-parsing corrected

v5.2.0: changes since 5.1.9:
Bug for search extensions corrected

v5.1.9: changes since 5.1.8:
Singular Reply Extension added

v5.1.8: changes since 5.1.7:
Minor change in the king mobility evaluation that has a notable effect
Opening book removed from the standard package

v5.1.7: changes since 5.1.6:
Knight mobility evaluation reduced
Constant for "draw for insufficiant material" reduced 
Minor infrastructual changes and little bugs removed (setboard, moveparser)

v5.1.6: changes since 5.1.5:
Rewrite of the pawn-progress evalution. It was a factor 2 too high. This made the engine stronger

v5.1.5: changes since 5.1.4:
Evaluation consideres the fact that with just one minor piece it can't win
Small changes in punishing hanging pawns (see 5.1.4)

v5.1.4: changes since 5.1.3:
Fixed minor bug that position setup by GUI interfered with opening book
Added strategical evaluation: Free pawns are rewarded, hanging pawns punished

V5.1.3: changes since 5.1.2:
Support for opening books, including a small one
Parsing of PGN Moves
Minor restructure in quiesce, thus increasing strength

V5.1.2: changes since 5.1.1:
PV-Search, getting rid of Alpha-Beta Windows
Minor changes in time control
Draw Check for insufficient Material
Pruning Window in Quiescence reduced
Search extension moved into the move-loop

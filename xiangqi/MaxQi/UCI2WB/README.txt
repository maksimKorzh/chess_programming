                         UCI2WB relelease notes

UCI2WB is an adapter for running USI (Shogi) engines under WinBoard.
Install USI2WB in WinBoard as if it was the engine, with one or two arguments
on its command line, to indicate the name of the real engine executable,
and (optionally) its working directory. 

E.g. when you have placed USI2WB.exe in a sub-folder 'Adapters' inside the WinBoard folder,
you could include the following line in winboard.ini amongst the firstChessPrograms:

"UCI2WB -s USI_ENGINE.exe ENGINE_FOLDER" /fd="./Adapters" /variant=shogi

UCI2WB was developed with gcc under Cygwin. It can also be used as a (dumb) UCI2WB adapter. 
"Dumb" here means that the adapter does not know anything about the game state, 
and just passes on the moves and position FENs as it receives them from engine or GUI. 
As this can be done without any knowledge of the game rules, or even of the board size, 
such a dumb adapter can in principle be used for any variant. To use it for UCI protocol
(both the Chess or Xiangqi dialects), use it without the -s flag (or with a -c flag).

This package includes the source code. To compile, use the command

gcc -O2 -s -mno-cygwin UCI2WB.c -o UCI2WB.exe

Have fun,
H.G.Muller




Change log:

31/7/2010 v1.1
Add WB remove command

30/7/2010 v1.0
Allow spaces in option names.
Refactor StopPonder into separate subroutine.
Refactor LoadPos into separate subroutine.
Send stop-ponder commands on exit and force.
Added icon.

29/7/2010 v0.9
Fixed analysis, which was broken after refacoring (newline after 'go infinite')

27/7/2010 v0.8
Refactored sendng of go command into separate routine
Send times with 'go ponder'.
Measure time spend on own move, and correct time left for it (2% safety margin).
Do adjust time left for new session or move time.

26/7/2010 v0.7
Fix bug w.r.t. side to move on setboard.
Print version number with -v option.

25/7/2010 v0.6
Undo implemented.
Analyze mode implemented. Seems to work for Glaurung 2.2 and Cyclone 2.1.1.
Periodic updates still use fictitious total move count of 100.

18/7/2010 v0.5
Switching between USI and UCI is now done at run time nased on a -s flag argument
Recognize WB variant command
In Xiangqi the position keyword is omitted, and a FEN is sent even for the start pos
Recognize 'null' as best move
Recognize scores without cp
Corrrect thinking time to centi-sec

17/7/2010 v0.4:
Introduced  compiler switch that enables some macros for everything that is different
in USI compared to UCI.
Fixed pondering.
Fixed setboard (for UCI).
Added result claims on checkmate / stalemate (for engines that say 'bestmove (none)').

16/7/2010 v0.3:
This is the first version for which the basics seem to work.
It could play a game of Blunder against itself, ending in resign.
Options of all types should work now.
Only classical time control tested.
Pondering not tested. (Blunder does not give a ponder move?)
Setboard not tested. (Probably does not work due to FEN format discrepancy.)
No analyze mode yet.
No SMP yet.
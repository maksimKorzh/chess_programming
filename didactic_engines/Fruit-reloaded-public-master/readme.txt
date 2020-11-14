
Legal details
-------------

Fruit 2.1      Copyright 2004-2005 Fabien Letouzey.
Fruit reloaded Copyright 2012-2014 Daniel Mehrmann
Fruit reloaded Copyright 2013-2014 Ryan Benitez


Preface
-------

Fruit reloaded was born through a "random case".
I was using Fruit 2.1 for analyzing my own games. 
At some points I noticed Fruit doesn't fit to my
own game style and had some weak parts.

Just for fun i was starting to tune Fruit a little
bit for myself. From time to time i made some changes
without a special target.
But Fruit was slow in search, i wanted a multi
core engine. I added a MP-Search and at this point
i thought it might be a good idea to send the code
to Fabien and Ryan to get any kind of feedback.
Maybe they could use it for there own engines.

The feedback was overwhelming good and both of
them liked my work. :-)
I was pretty happy that Ryan helps me now and 
so we started a team project.

Fruit reloaded is an independent fork of Fruit 2.1.

I want to say thank you to Fabien for his clean
written Fruit and Ryan for his nice support and code
work so far.

Fruit reloaded target is to follow the clean style
of Fabien work. It should help other programmers
how to write an simple clean engine.

Of course we want make Fruit reloaded stronger, so
we're searching other developers and testing user
to help Fruit reloaded.


UCI options
-----------

- "Aspiration Search" (true/false, default true)

Enable aspiration search. Fruit reloaded will use a smaller
search window, which makes him faster on depth, but will
overlooked good moves sometimes.

- "Aspiration Depth" (3-10 plies, default:4)

Defines the minimum depth for using aspiration search.

- "Aspiration Window" (1-100, default:16)

Specifies the window size used by aspiration search.
I expect some tuning possibilities with other values.

- "BookFile" (name, default book.bin)

Specifies the path to the book which should be used.

- "EGTB" (true/false, default true)

Enables the usage of Nalimov Endgame Tablebases in the search.

- "EGTB Depth" (0-20 plies, default:8)

Defines the minimum remaining depth (distance to horizon) for using
EGTBs.  Lower values result in a more aggressive use of EGTBs but a
significant slowdown in endgame positions (due to heave disk access).
For analysis or fast SCSI Drives one can choose lower values.

- "EGBB" (true/false, default false)

Enables the usage of Daniel Shawul endgame bitbases in the search

- "EGBB Depth" (0-20 plies, default:4)

Defines the factor depth for using EGBB. Lower values result in
a more aggressive use of EGBB.

- "EGBB path" (directory, default <empty>)

Specifies the path to the bitbase files. Not the path to the DLL!
The DLL must be in the same dir like Fruit reloaded binaries.

- "EGBB cache"

Defines the cache size in megabytes for the bitbase files


- evaluation options (percentage, default: 100%)

These options are evaluation-feature multipliers.  You can modify
Fruit's playing style to an extent or make Fruit reloaded weaker 
for instance by setting "Material" to a low value.

"Material" is obvious.  It also includes the bishop-pair bonus.
"Piece Activity": piece placement and mobility.
"King Safety": mixed features related to the king during early phases
"Pawn Structure": all pawn-only features (not passed pawns).
"Pawn Activity": .pawn attack and pawn square control

I think "Pawn Structure" is not an important parameter.
Who knows what you can obtain by playing with others?

- material options (percentage, default 100%)

These options set the material value of the different pieces. 
It is unlikely to gain much by changing them but who knows.

- contempt factor (centipawns, default: 0)

This is the value Fruit reloaded will assign to a draw (repetition, stalemate,
perpetual, 50-move-rule).  It is from Fruits viewpoint.  With negative
values Fruit will avoid draws with positive values it will seek draws.
The bonus is constantly reduced when material comes off and 0 if all
material is exchanged.


History
-------

2004/03/17 Fruit 1.0, first stable release
------------------------------------------

Fruit was written in early 2003, then hibernated for many months.
I suddenly decided to remove some dust from it and release it after
seeing the great WBEC web site by Leo Dijksman!  Note that Fruit is
nowhere near ready to compete there because of the lack of xboard
support and opening book.  Note from the future: these limitations
seem not to be a problem anymore.

Fruit 1.0 is as close to the original program as possible, with the
main exception of added UCI-handling code (Fruit was using a private
protocol before).  It is a very incomplete program, released "as is",
before I start heavily modifying the code (for good or bad).

You can find a succinct description of some algorithms that Fruit uses
in the file "technical_10.txt" (don't expect much).


2004/06/04 Fruit 1.5, halfway through the code cleanup
------------------------------------------------------

In chronological order:

- added mobility in evaluation (makes Fruit play more actively)

- added drawish-material heuristics (makes Fruit look a bit less stupid
  in some dead-draw endgames)

- tweaked the piece/square tables (especially for knights)

- added time management (play easy moves more quickly, take more time
  when unsure)

- enabled the single-reply extension (to partly compensate for the lack
  of king safety)

- some speed up (but bear in mind mobility is a costly feature, when
  implemented in a straightforward way as I did)


2004/12/24 Fruit 2.0, the new departure
---------------------------------------

The main characteristic of Fruit 2.0 is the "completion" of the
evaluation function (addition of previously-missing major features).

In chronological order:

- separated passed-pawn evaluation from the pawn hash table,
  interaction with pieces can now be taken into account

- added a pawn-shelter penalty; with king placement this forms
  some sort of a simplistic king-safety feature

- added incremental move generation (Fruit was starting to be too slow
  for my taste)

- added futility and delta pruning (not tested in conjunction with
  history pruning and hence not activated by default)

- improved move ordering (bad captures are now postponed)

- added history pruning (not tested seriously at the time I write
  this yet enabled by default, I must be really dumb)

- cleaned up a large amount of code (IMO anyway), this should allow
  easier development in the future


2005/06/17 Fruit 2.1, the unexpected
------------------------------------

Unexpected because participation in the Massy tournament had not been
planned.  What you see is a picture of Fruit right in the middle of
development. There may even be bugs (but this is a rumor)!

I have completed the eval "even more", not that it's ever complete
anyway.  I have to admit that I had always been too lazy to include
king attacks in previous versions.  However, some programs had fun
trashing Fruit 2.0 mercilessly in 20 moves, no doubt in order to make
me angry.  Now they should need at least 25 moves, don't bother me
again!

- added rook-on-open file bonus; thanks to Vincent Diepeveen for
  reminding me to add this.  Some games look less pathetic now.

- added pawn storms; they don't increase strength but they are so
  ridiculous that I was unable to deactivate them afterwards!

- added PV-node extensions (this is from Toga), e.g. extending
  recaptures only at PV nodes.  Not sure if these extensions help; if
  they do, we all need to recognize Thomas Gaksch's contribution to
  the community!

- added (small) king-attack bonus, the last *huge* hole in the eval;
  now only large holes remain, "be prepared" says he (to himself)!

- added history-pruning re-search; does not help in my blitz tests,
  but might at longer time control; it's also safer in theory,
  everybody else is using it and I was feeling lonely not doing like
  them.  OK, Tord told me that it helped in his programs ...

- added opening book (compatible with PolyGlot 1.3 ".bin" files)

- fixed hash-size UCI option, it should now be easy to configure using
  all interfaces (there used to be problems with Arena, entirely by my
  fault)


2012/05/26 Fruit reloaded 1.0, reborn the Fruit
-----------------------------------------------

This version is a Fruit 2.2.1 + new pruning technics.

- added side-to-move bonus

- added extended futility pruning

- added Razor pruning (not yet full tested)

- added static null-move pruning (not yet full tested)

- tuned history pruning

- minor tuning of different parameters

- added Tablebase support

- added Pawn endgame extension

- added Pruning exception of passed pawns

- added full evaluation every node

- added "Tune Eval" based on search/eval

- improved move ordering of late moves

- added modern futility pruning


2012/05/27 Fruit reloaded 1.1, the null guy
-------------------------------------------

- added UseNullRestricted

- added Null-Move reduction


2012/05/31 Fruit reloaded 1.1.1
-------------------------------

- added Extended History Pruning

- tuned Null-Move reduction (if null restrict used)


2012/07/01 Fruit reloaded 1.2.0
-------------------------------

- added evaluation stored in hashtable

- fixed UCI options

- added UCI option Contempt Factor


2012/07/01 Fruit reloaded 1.3.0
-------------------------------

- added quiescence hash

- tuned Evasion move order 

- added test/debug sets

- added Log


2013/03/07 Fruit reloaded 1.3.1
-------------------------------

- fixed QS check extend only if eval near beta


2013/03/08 Fruit reloaded 1.3.2
-------------------------------

- changed "dangerous moves" no longer history reduce 



2013/10/30 Fruit reloaded 1.4.0
-------------------------------

- added eval:   pawn attack by piece

- added eval:   mobility piece attack

- added search: aspiration search

- added search: pawn passed extension

- added search: Delta-Delta pruning (but disabled so far)

- added search: internal depth doubling

- added search: move eval used by ordering 

- tuned search: Redesign of null-move pruning

- tuned search: quiescence nodePV captures pass through

- tuned hash:   quiescence hash for all and new depths

- changed search: search stop with vars



2013/11/23 Fruit reloaded 2.0
-----------------------------

- added MP-Search

- added Pass Pawn extension

- tuned History pruning

- Null-Move fail (but disabled so far)



2013/12/03 Fruit reloaded 2.0.1
-------------------------------

- tuned: Hash performance with MP-Search



2013/12/14 Fruit reloaded 2.0.2
-------------------------------

- tuned History pruning/reduce

- added discover checks to Null-Move fail

- fixed various typos

- fixed cleanup piece attack code



2013/12/14 Fruit reloaded 2.0.3
-------------------------------

- tuned History pruning/reduce


2014/03/17 Fruit reloaded 2.1
-----------------------------

-added multipv support

-added X-Ray mobility attack

-added pawn mobility attack

-added pawn mobility

-added pawn safe mobility

-added LMR & LMR pruning

-added eval_win endings

-added eval promote

-tuned eval

-removed history pruning


Known bugs
----------

Fruit reloaded always claims that CPU is 100% used.  This is apparently a
problem in the standard C libraries on Windows.  Mailbomb me if fixing
this would save lives (especially children)!  I prefer waiting for
late users to throw away Windows 95/98/ME before adding an
NT/2000/XP/... solution.


Contact
-------

E-Mail: daniel.mehrmann@gmx.de


The GPL v3 based source files can be found here: https://github.com/Akusari/Fruit-reloaded-public

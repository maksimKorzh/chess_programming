                                FAIRY-MAX 4.8A RELEASE NOTES

MaxQi is an engine for playing Xiang Qi, using WinBoard protocol to communicate its moves.
This means it can run under the WinBoard GUI, version 4.3 of which has Xiang Qi mongst the
supported variants.

MaxQi is a derivative of the Fairy-Max 4.8F engine, which is a gneral variant-playing system.
Xiang Qi falls a bit outside its normal parameter range, though, due to the board subdivision.
Hence a version specializd for Xiang qi was made.

MaxQi should be possible to compile it with any C compiler. It was developed using gcc under cygwin. 
On this platform, it can simply be compiled with the command:

gcc -O2 -mno-cygwin MaxQi.c -o MaxQi.exe

This gives you a perfectly functional executable, which you can shrink in size somewhat by using

strip MaxQi.exe

MaxQi needs the qmax.ini file in its directory to describe the pieces and their values.

The remainder of this README file applies to Fairy-Max in general.

For compiling under Linux, you have to define the compiler swithch LINUX. This can be done by adding the
command-line option "-D LINUX" (without te quotes) in the compilation command.

If you want the executable to show up in Windows as a nice icon, rather than the standard symbol for an executable, you have to link it to an icon pictogram. I do this by the sequence of commands:

windres --use-temp-file --include-dir . fmax.rc -O coff -o fres.o
gcc -O2 -mno-cygwin -c fmax4_8.c
gcc -mno-cygwin *.o -o fmax.exe
strip fmax.exe

I have no idea if this is a good or a stupid way to do it, I have zero experience in programming Windows applications, and this was a copy-cat solution based on the WinBoard source code that seemed to work.

Have fun,
H.G. Muller


New features compared to previous releases:

* Support for a cylindrical board (used in predefined variant Cylinder Chess)
* Support for lame leapers and multi-path lame leapers.
* Support for e.p. capture of Berolina Pawns.

The supplied fmax.ini file now includes definitions for Cylinder Chess, Berolina Chess,
Falcon Chess (patented!) and Super-Chess (TM) next to the usual set (normal Chess,
Capablanca Chess, Gothic Chess, Shatranj, Courier Chess and Knightmate). Note that
Cylinder Chess and Berolina Chess can only be played in WinBoard 4.3.15 with legality testing off.

SOME NOTES ON SUPERCHESS

Super Chess is defined with the standard pieces used in the Dutch Open of this variant:
Archbishop (A), Marshall (M), Centaur (C) and Amazon (Z) (which in Super Chess are referred to as
Princess, Empress, Veteran and Amazon, respectively). But it also has definitions for the
Cannon (O), Nightrider (H), Dragon Horse (D) and and Grasshopper (G). If you include any of those
pieces in the initial setup, Fairy-Max will use them. You can instruct WinBoard to use these pieces
in stead of the default AMCZ by redefining the pieceToCharTable of WinBoard with a command-line option.
Note that the promotion rules for SuperChess are not yet implemented in this version of Fairy-Max:
the opponent can promote to anything (but if it is not Queen this will come as a complete surprise
to Fairy-Max), and Fairy-Max itself only promotes to Queen. (Even if it already has one...)
The Grasshopper can currently only be used in WinBoard when Legality testing is off.

SOME NOTES ON FALCON CHESS

Falcon Chess features a piece that is covered by a U.S. patent (#5,690,334), the Falcon, 
which is a non-leaping piece that can follow three alternative paths to any (1,3) or (2,3) 
destination. (In the sme terminology as where a Knight reaches any (1,2) destination.) 
The current version of Fairy-Max is licenced to use this piece in the way it does now. 
This does NOT imply, however, that any code you derive from Fairy-Max, (something that in 
itself you are perfectly allowed to make) is also licensed to play Falcon Chess, or use the
Falcon in another Chess variant, if any of the modification you made reflect in any way on 
its skill in playing Falcon Chess or these Falcon-containing variants.
In such a case you would have to seek a new license from the patent holder, George W. Duke.
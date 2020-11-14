A few notes on compiling and running Gerbil

Bruce Moreland (brucemo@seanet.com)
14-Jun-2001

CONTENTS:

0) SOME NOTES REGARDING COPYRIGHT AND LICENSING
1) COMPILING THE PROJECT
2) RUNNING GERBIL
3) RUNNING EPD2WB
4) SOME NOTES REGARDING EDITING SOURCE CODE
5) EDITING THE OPENING BOOK
6) PROBLEMS


0) SOME NOTES REGARDING COPYRIGHT AND LICENSING

All of the sources contained within this project are covered by the GNU GPL,
a copy of which is also included along with this project.

The basic idea is that if you can't sell the project executables without
providing source code, even if you modify the source code, and the source code
must be more or less freely provided and be redistributable.  So you can't
turn this project into proprietary software, even if you make a lot of cool
changes to it.

I plan that the chess engine (the contents of the "engine" subdirectory)
remain covered by the GPL.

At some future date I may put the rest of the source code into the public
domain, so it can be used by anyone for any purpose, without emcumberment.
This is not the case yet.

If you have any comments regarding free software, the GPL, my interpretation
of the GPL, or if you have anything to ask or suggest regarding my plans for
this software, please drop me an email.


1) COMPILING THE PROJECT

If you want to compile this project, first edit "paths.mak" so that your
compiler knows where to find this project, and its own binaries, libraries,
and include files.

Once you do that, you will be able to sit in the "src" directory and type ...

    nmake

... to make the executables.  They will be put in the "ship" subdirectory,
along with the object modules and so on.

If you type ...

    nmake DEBUG=1

... it will make a debug version, which will go into the "debug" directory.
A debug version has asserts (debug checks) enabled in the engine, and it is
compiled without compiler optimizations so it is easier to debug.

The debug version shouldn't be used to play serious chess, since it is very
much slower than the ship version.


2) RUNNING GERBIL

Gerbil is a winboard engine.  It will not use old versions of Winboard, at
least not reliably.  If you have an ancient version, get a new one from Tim
Mann at:

    http://www.tim-mann.org/xboard.html

The oldest version I've tried to run with is version 4.2.3, which at time of
writing is the current version.

To play against the program, under winboard, the command line is:

    winboard /cp /fcp gerbil.exe

To tell it to play on a chess server such as ICC, the command line is:

    winboard /cp /zp /fcp gerbil.exe /ics /icshost chessclub.com /icshelper timestamp.exe

To get it to work, you might have to move "gerbil.exe" and "gerbil.opn" into
the same directory as Winboard, or mess with Winboard's command line and/or
Gerbil's INI file ("gerbil.ini").

The INI file expects to be in the same directory as "gerbil.exe".  Documented
variables in the INI file are:

    "BookFile".  Accepts a filename.  The file where the book is.  Currently,
    the file must exist and contain a valid book, even if "UseBook" is zero.
    The default is "gerbil.opn".

    "UseBook".  Accepts 0 or 1.  If non-zero, it will make moves from its
    opening book.  The default is 1.

    "MaxHash".  Accepts a positive integer.  Maximum hash table size, in
    bytes.  It will use less than or equal to this number of bytes for
    transposition hash table.  The default is 10,000,000 bytes.

    "MaxBook".  Accepts a positive integer.  The book is read from disk and
    stored in memory.  If you edit the opening book and make it huge, you
    may have to change this value.  The default is 65536 bytes.  The program
    can use 75% of book storage, and each unique position takes 8 bytes.  It
    will output an error if you try to make the program eat a book that is
    larger than it can handle.

    "BookDump".  This is describe in the section on editing the opening book.

If you can't get the program to boot, try passing the "/debug" switch to
Winboard, then look at the "winboard.debug" log file after trying to start
Winboard.  There might be a clue in there.


3) RUNNING EPD2WB

Epd2Wb is a utility that allows you to run test suites with Winboard engines.
Specifically it works with Gerbil, but it may work with others.

To run it, put the utility in the same directory as the winboard engine you
are going to test, and type:

    epd2wb <engine.exe> <suite.epd> <seconds to think> [skip]

The optional "skip" parameter will be describe shortly.  The others should be
self-explanatory.

The program should start spitting out analysis immediately.  If it doesn't, it
could be because of a bug in how my utility or the target engine interprets
the Winboard protocol, or it could be a problem with configuration, or it
could be that the engine does not support the Winboard commands that I need to
use.  I use several protover 2 commands, including:

    analyze
    ping
    setboard

If your engine doesn't support these, Epd2Wb won't work.

How the thing works is that it starts the target engine in analysis mode and
sits there and eats analysis lines from the engine.  The format of analysis
likes is undefined, but many engines, including Gerbil, return the lines in
SAN (standard algebraic notation).  If the engine does that, everything should
work fine.

Some engines, like Yace, prefix their thinking output with strange characters.
Yace always emits one blob of strange characters, so the "skip" argument for
Yace should be set to 1, meaning skip over one blob of strange characters.

Some other engines return their output in e2e4 notation, or e2-e4 notation,
and in these cases the only way you are going to get Epd2Wb to work well is if
you edit the EPD file so the solution ("bm" field in the EPD record) is in the
same format.

It is possible that other engines will work with Epd2Wb, and it is possible
that none of them will.

If you are writing your own Winboard engine, you may wish to explore Epd2Wb as
a means of avoiding writing your own test suite harness.

If you need to pass parameters to the engine, you can do this:

    epd2wb "<engine.exe> params" <suite.epd> <seconds to think> [skip]


4) SOME NOTES REGARDING EDITING SOURCE CODE

Tabs in the sources are set at 4 characters.  If you try to use an editor that
has tabs set at some other number of characters, everything will look like
hell.

Please see my "hungary.txt" document for some notes regarding my variable
naming convention.


5) EDITING THE OPENING BOOK

The opening book is in text format.  You may wish to use something like
notepad to edit it, since the lines are greater than 80 characters long.

Any edits you make will take effect next time the program is booted.  If you
look at the file format, it should be obvious what kinds of edits are allowed
and make sense.

If you want to "turn off" a book move, for instance "1. e4", change it to
"1. e4?", and it won't play it, nor will it try to transpose via the opening
book into any of the lines (for white) following 1. e4.  Meaning, if you turn
off 1. e4, the program will still play 1. ... c5 or 1. ... e5 as black, after
the opponent plays 1. e4, but if the program plays 1. d4 and the opponent
replies with 1. ... e6, the program will not play 2. e4, since that position
is has essentially been deleted from the program's list of possibilities.

You can probably do alright just by editing the book that is there, or making
your own book that is in the same format, but there are a few tools that will
help you, although they may be hard to use.

If you set "BookDump" in gerbil.ini to 1 or 2, you can dump the book when the
program starts.  To do this, edit the INI file, then type ...

    gerbil >foo

... from the command line, and when the program sits there looking blankly at
you, type the very intuitive command line:

    protover 2

You can redirect to some other file than "foo" if you want, but *do not* try
to redirect onto the book file.  That will delete the book file before the
program has a chance to read it, as I found out the hard way.

If you've set "BookDump" to 1, your file will contain full opening book lines,
meaning that the program won't try to diff each line with the previous book
line.

You can edit this all you want, then copy it over the old book.  That will
make a fine book, but you can set "BookDump" to 2 and go through the dump
process with your new book, and it will output a file that is in the more
compact format.

My own suggestion as to how to best create a book is as follows:

    1) Dump the book in format 1.
    2) Edit it all you want.
    3) Sort the new book in ascii order.
    4) Copy it over the old book.
    5) Dump the new book in format 2.
    6) Copy it over the old book.

"BookDump" should be a command-line switch, but I have strange architectural
problems with this, so I didn't implement it like that yet.


6) PROBLEMS

If you have problems with this project, please take a good solid shot at
working through them, but if you have real trouble, please let me know.  Don't
expect me to do a lot of work for you or teach you C or something.  This
program is supposed to be an opportunity for you to have some harmless and
semi-educational fun for free.  It is not supposed to be a giant rock that I
have to balance on my head for the rest of my life.

--
Copyright (C) Bruce Moreland, 2001.  All rights reserved.
Please look in "gpl.txt" for information on the GNU General Public License.

# To build crafty:
#
#  You want to set up for maximum optimization, but typically you will
#  need to experiment to see which options provide the fastest code.  The
#  following option descriptions explain each option.  To use one or more
#  of these options, to the "opt =" line that follows the explanations and
#  add the options you want, being careful to stay inside the single quote
#  marks.
#   
#   -DAFFINITY     Include code to set processor/thread affinity on Unix
#                  systems.  Note this will not work on Apple OS X as it does
#                  not support any sort of processor affinity mechanism.
#                  WARNING:  know what you are doing before using this.  If
#                  you test multiple copies of Crafty (each copy using just
#                  one thread) you can have bogus results because if compiled
#                  with -DAFFINITY, even a single-cpu execution will lock led
#                  itself onto processor zero (since it only has thread #0)  
#                  which is probably NOT what you want.  This is intended to 
#                  be used when you run Crafty using a complete dedicated
#                  machine with nothing else running at the same time.
#   -DBOOKDIR      Path to the directory containing the book binary files.
#                  The default for all such path values is "." if you don't
#                  specify a path with this macro definition.
#   -DCPUS=n       Defines the maximum number of CPUS Crafty will be able
#                  to use in a SMP system.  Note that this is the max you
#                  will be able to use.  You need to use the smpmt=n command
#                  to make crafty use more than the default 1 process.
#   -DDEBUG        This is used for testing changes.  Enabling this option
#                  greatly slows Crafty down, but every time a move is made,
#                  the corresponding position is checked to make sure all of
#                  the bitboards are set correctly.
#   -DEPD          If you want full EPD support built in.
#   -DLOGDIR       Path to the directory where Crafty puts the log.nnn and
#                  game.nnn files.
#   -DNODES        This enables the sn=x command.  Crafty will search until
#                  exactly X nodes have been searched, then the search 
#                  terminates as if time ran out.
#   -DPOSITIONS    Causes Crafty to emit FEN strings, one per book line, as
#                  it creates a book.  I use this to create positions to use
#                  for cluster testing.
#   -DRCDIR        Path to the directory where we look for the .craftyrc or
#                  crafty.rc (windows) file.
#   -DSKILL        Enables the "skill" command which allows you to arbitrarily
#                  lower Crafty's playing skill so it does not seem so
#                  invincible to non-GM-level players.
#   -DSYZYGY       This enables syzygy endgame tables (both WDL and DTC)
#   -DTBDIR        Path to the directory where the endgame tablebase files
#                  are found.  default = "./TB"
#   -DTEST         Displays evaluation table after each move (in the logfile)
#   -DTRACE        This enables the "trace" command so that the search tree
#                  can be dumped while running.
#   -DUNIX         This identifies the target O/S as being Unix-based, if this
#                  option is omitted, windows is assumed.
#
#    Note:         If you compile on a machine with the hardware popcnt
#                  instruction, you should use the -mpopcnt compiler option
#                  to make the built-in intrinsic use the hardware rather than
#                  the "folding" approach.  That is the default in this
#                  Makefile so if you do NOT have hardware popcnt, remove all
#                  of the -mpopcnt strings in the Makefile lines below.
#
default:
	$(MAKE) -j unix-clang
help:
	@echo "You must specify the system which you want to compile for:"
	@echo ""
	@echo "make unix-clang       Unix w/clang compiler (MacOS usually)"
	@echo "make unix-gcc         Unix w/gcc compiler"
	@echo "make unix-icc         Unix w/icc compiler"
	@echo "make profile          profile-guided-optimizations"
	@echo "                      (edit Makefile to make the profile"
	@echo "                      option use the right compiler)"
	@echo ""


quick:
	$(MAKE) target=UNIX \
		CC=clang \
 		opt='-DSYZYGY -DTEST -DTRACE -DCPUS=4' \
		CFLAGS='-mpopcnt -Wall -Wno-array-bounds -pipe -O3' \
		LDFLAGS='$(LDFLAGS) -lstdc++' \
		crafty-make

unix-gcc:
	$(MAKE) -j target=UNIX \
		CC=gcc \
		opt='-DSYZYGY -DTEST -DCPUS=4' \
		CFLAGS='-Wall -Wno-array-bounds -pipe -O3 -fprofile-use \
		-mpopcnt -fprofile-correction -pthread' \
		LDFLAGS='$(LDFLAGS) -fprofile-use -pthread -lstdc++' \
		crafty-make

unix-gcc-profile:
	$(MAKE) -j target=UNIX \
		CC=gcc \
		opt='-DSYZYGY -DTEST -DCPUS=4' \
		CFLAGS='-Wall -Wno-array-bounds -pipe -O3 -fprofile-arcs \
		-mpopcnt -pthread' \
		LDFLAGS='$(LDFLAGS) -fprofile-arcs -pthread -lstdc++ ' \
		crafty-make

unix-clang:
	@/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/llvm-profdata merge -output=crafty.profdata *.profraw
	$(MAKE) -j target=UNIX \
		CC=clang \
		opt='-DSYZYGY -DTEST -DCPUS=4' \
		CFLAGS='-Wall -Wno-array-bounds -pipe -O3 \
			-mpopcnt -fprofile-instr-use=crafty.profdata' \
		LDFLAGS='$(LDFLAGS) -fprofile-use -lstdc++' \
		crafty-make

unix-clang-profile:
	$(MAKE) -j target=UNIX \
		CC=clang \
		opt='-DSYZYGY -DTEST -DCPUS=4' \
		CFLAGS='-Wall -Wno-array-bounds -pipe -O3 \
			-mpopcnt -fprofile-instr-generate' \
		LDFLAGS='$(LDFLAGS) -fprofile-instr-generate -lstdc++ ' \
		crafty-make

unix-icc:
	$(MAKE) -j target=UNIX \
		CC=icc \
		opt='-DSYZYGY -DTEST -DCPUS=4' \
		CFLAGS='-Wall -w -O2 -prof_use -prof_dir ./prof -fno-alias \
                        -mpopcnt -pthread' \
		LDFLAGS='$(LDFLAGS) -pthread -lstdc++' \
		crafty-make

unix-icc-profile:
	$(MAKE) -j target=UNIX \
		CC=icc \
		opt='-DSYZYGY -DTEST -DCPUS=4' \
		CFLAGS='-Wall -w -O2 -prof_gen -prof_dir ./prof -fno-alias \
                        -mpopcnt -pthread' \
		LDFLAGS='$(LDFLAGS) -pthread -lstdc++ ' \
		crafty-make

profile:
	@rm -rf *.o
	@rm -rf log.*
	@rm -rf game.*
	@rm -rf prof
	@rm -rf *.gcda
	@mkdir prof
	@touch *.c *.h
	$(MAKE) -j unix-clang-profile
	@echo "#!/bin/csh" > runprof
	@echo "./crafty <<EOF" >>runprof
	@echo "bench" >>runprof
	@echo "mt=0" >>runprof
	@echo "quit" >>runprof
	@echo "EOF" >>runprof
	@chmod +x runprof
	@./runprof
	@rm runprof
	@touch *.c *.h
	$(MAKE) -j unix-clang


#
#  one of the two following definitions for "objects" should be used.  The
#  default is to compile everything separately.  However, if you use the 
#  definition that refers to crafty.o, that will compile using the file crafty.c
#  which #includes every source file into one large glob.  This gives the
#  compiler max opportunity to inline functions as appropriate.  You should try
#  compiling both ways to see which way produces the fastest code.
#

#objects = main.o iterate.o time.o search.o quiesce.o evaluate.o thread.o \
        repeat.o hash.o next.o history.o movgen.o make.o unmake.o attacks.o \
        see.o tbprobe.o boolean.o utility.o book.o drawn.o epd.o \
        epdglue.o init.o input.o autotune.o interrupt.o option.o output.o \
        ponder.o resign.o root.o learn.o setboard.o test.o validate.o \
        annotate.o analyze.o evtest.o bench.o edit.o data.o

objects = crafty.o

# Do not change anything below this line!

opts = $(opt) -D$(target)

crafty-make:
	@$(MAKE) opt='$(opt)' CFLAGS='$(CFLAGS)' crafty

crafty.o: *.c *.h

crafty:	$(objects)
	$(CC) $(LDFLAGS) -g -o crafty $(objects) -lm  $(LIBS)

evaluate.o: evaluate.h

clean:
	-rm -f *.o crafty

$(objects): chess.h data.h

.c.o:
	$(CC) $(CFLAGS) $(opts) -c $*.c

.s.o:
	$(AS) $(AFLAGS) -o $*.o $*.s

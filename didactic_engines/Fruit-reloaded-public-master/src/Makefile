
# files

EXE = fruit-reloaded

OBJS = attack.o board.o book.o egbb.o engine.o eval.o fen.o hash.o list.o main.o \
       material.o move.o move_check.o move_do.o move_evasion.o move_gen.o move_legal.o \
       option.o pawn.o piece.o posix.o protocol.o pst.o pv.o random.o recog.o search.o \
       search_full.o see.o sort.o square.o tb.o test.o thread.o trans.o util.o \
       value.o vector.o

# rules

all: $(EXE) .depend

clean:
	$(RM) *.o .depend gmon.out fruit-reloaded

# general

CXX      = g++
CXXFLAGS = -Wall -pipe
LDFLAGS  = -lm

# C++

CXXFLAGS += -fno-exceptions -fno-rtti

# optimisation

CXXFLAGS += -O3 -fstrict-aliasing 
#CXXFLAGS += -fomit-frame-pointer

# cpu

#CXXFLAGS += -march=core2

# strip

LDFLAGS += -s -pthread

# dependencies

$(EXE): $(OBJS)
	$(CXX) $(LDFLAGS) -o $@ $(OBJS) -ldl

.depend:
	$(CXX) -MM $(OBJS:.o=.cpp) > $@

include .depend


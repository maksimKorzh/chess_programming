
AUTOMAKE_OPTIONS = foreign subdir-objects
AM_CXXFLAGS = -std=gnu++11
CXXFLAGS = -m64 -g

CPPFLAGS = -I../src
LDADD = -lpthread -lrt

bin_PROGRAMS = chameleon

chameleon_SOURCES = ../src/bitboard.cpp \
					../src/book.cpp \
					../src/endgame.cpp \
					../src/evaluate.cpp \
					../src/init.cpp \
					../src/main.cpp \
					../src/material.cpp \
					../src/misc.cpp \
					../src/movegen.cpp \
					../src/movepick.cpp \
					../src/pawns.cpp \
					../src/position.cpp \
					../src/psqt.cpp \
					../src/search.cpp \
					../src/thread.cpp \
					../src/timeman.cpp \
					../src/tt.cpp \
					../src/uci.cpp \
					../src/ucioption.cpp

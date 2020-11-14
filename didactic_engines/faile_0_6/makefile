CC = gcc
FLAGS = -O2 -Wall

all:	faile.exe

faile.exe:	eval.o faile.o moves.o search.o utils.o xboard.o book.o
	$(CC) -o faile.exe eval.o faile.o moves.o search.o utils.o xboard.o book.o

eval.o:	eval.c extvars.h faile.h protos.h
	$(CC) $(FLAGS) -c -o eval.o eval.c

faile.o:	faile.c extvars.h faile.h protos.h
	$(CC) $(FLAGS) -c -o faile.o faile.c

moves.o:	moves.c extvars.h faile.h protos.h
	$(CC) $(FLAGS) -c -o moves.o moves.c

search.o:	search.c extvars.h faile.h protos.h
	$(CC) $(FLAGS) -c -o search.o search.c

utils.o:	utils.c extvars.h faile.h protos.h
	$(CC) $(FLAGS) -c -o utils.o utils.c

xboard.o:	xboard.c extvars.h faile.h protos.h
	$(CC) $(FLAGS) -c -o xboard.o xboard.c

book.o:	book.c extvars.h faile.h protos.h
	$(CC) $(FLAGS) -c -o book.o book.c
#EOF


#CC=mpicc
CC=gcc
CFLAGS= -ansi -Wall -pedantic -g -std=c99

CFILES=nqueens.c myQueue.c
HFILES=nqueens.h myQueue.h
OFILES=nqueens.o myQueue.o
BIN=nqueens

all:	nqueens

%.o:	%.c $(HFILES)
	$(CC) -c $(CFLAGS) $< -o $@

nqueens:	$(OFILES) $(HFILES)
	        $(CC) $(CFLAGS) $(OFILES) -o ./bin/$(BIN)

clean:	
	rm -f *~ $(OFILES) ./bin/$(BIN)

mpirun: 
	mpirun -hostfile myhosts ./bin/$(BIN)

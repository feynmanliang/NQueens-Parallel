CC=mpicc
CFLAGS= -ansi -Wall -pedantic -g -std=c99

CFILES=nqueens.c mpi_manager_worker.c nqueens_parallel.c myQueue.c
HFILES=nqueens.h mpi_manager_worker.h myQueue.h
OFILES=nqueens.o mpi_manager_worker.o nqueens_parallel.o myQueue.o
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

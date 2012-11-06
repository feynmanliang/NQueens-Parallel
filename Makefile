CC=mpicc
CFLAGS= -ansi -Wall -pedantic -g -std=c99

CFILES=myQueue.c nqueens.c nqueens_parallel.c mpi_manager_worker.c
HFILES=myQueue.h nqueens.h nqueens_parallel.h mpi_manager_worker.h
OFILES=myQueue.o nqueens.o nqueens_parallel.o mpi_manager_worker.o
BIN=nqueens

all:	nqueens

%.o:	%.c $(HFILES)
	$(CC) -c $(CFLAGS) $< -o $@

nqueens:	$(OFILES) $(HFILES)
	        $(CC) $(CFLAGS) $(OFILES) -o ./bin/$(BIN)

clean:	
	rm -f *~ $(OFILES) ./bin/$(BIN)

mpirun: 
	mpirun -hostfile myhosts ./bin/$(BIN) 5

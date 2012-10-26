CC=mpicc
CFLAGS= -ansi -Wall -pedantic -g -std=c99

CFILES=nqueens.c mpi_manager_worker.c nqueens_parallel.c
HFILES=nqueens.h mpi_manager_worker.h
OFILES=nqueens.o mpi_manager_worker.c nqueens_parallel.c
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

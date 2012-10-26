#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include "./mpi_manager_worker.h"

#define MYTAG 42

#define msginit(argcp, argvp, rankp, totalp) {\
   MPI_Init(argcp, argvp);\
   MPI_Comm_rank(MPI_COMM_WORLD, rankp);\
   MPI_Comm_size(MPI_COMM_WORLD, totalp);\
}  

#define msgsend(buffp, to) \
  MPI_Send(buffp, sizeof(buffp), MPI_BYTE, to, MYTAG, MPI_COMM_WORLD);

#define msgrecv(buffp, from) \
  MPI_Recv(buffp, sizeof(buffp), MPI_BYTE, from, MYTAG, MPI_COMM_WORLD, &status);

#define MANAGER 0

int mpi_compute(int argc, char **argv) {
   int rank, total;
   int active_node;
   char buff;
   MPI_Status status;

   msginit(&argc,&argv,&rank,&total);

   if (rank == 0) {
      for (active_node = 1; active_node < total - 1; active_node++) {
         msgsend(&buff, active_node);
         msgrecv(&buff, active_node);
      }
   }

   else {
      msgrecv(&buff, MANAGER); 
      msgsend(&buff, MANAGER); 
   }

   fflush(stdout);
   MPI_Finalize();
   return 0;
}

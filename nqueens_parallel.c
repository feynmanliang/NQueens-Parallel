#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include "./nqueens.h"
#include "./myQueue.h"


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

void generate_initial();
int N;
void *stateQueue;


int main(int argc, char **argv) {
   void* buff = NULL;
   /*int rank, total;
   int active_node;
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
   }   */

   if (argc < 2) {
      printf("usage: nqueens_parallel <board dimension>\n");
      exit(1);
   }

   N = atoi(argv[1]);
   stateQueue = qopen();
   generate_initial();

   for (int i=0; i < N; ++i) {
      qget(stateQueue, &buff);
      State x = (State) buff;
      NextRow tmp = solve_next_layer(x);
      for (int j=0; j < tmp->numSolutions; ++j) printf("%d\n",tmp->validColumns[j]);
   }


   /*solve_state(N, states);*/
   return 0;
}

void generate_initial() {
   State currentState;
   for (int i=0; i < N; ++i) {
      currentState = malloc(sizeof(SState));
      currentState->N = N;
      currentState->numQueens = 1;
      currentState->queenPos = malloc(sizeof(int) * N);
      currentState->queenPos[0] = i;
      qput(stateQueue, (void*) currentState);
   }
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>
#include "./nqueens.h"
#include "./myQueue.h"
#include "./mpi_manager_worker.h"
#include "./nqueens_parallel.h"

#define WORKTAG 42
#define DIETAG 20
#define MANAGER 0

#define msgInit(argc, argv) {\
   MPI_Init(&argc, &argv);\
   MPI_Comm_rank(MPI_COMM_WORLD, &myrank);\
   MPI_Comm_size(MPI_COMM_WORLD, &numTasks);\
}

#define msgKillAll() {\
   for (int rank = 1; rank < numTasks; ++rank) {\
      MPI_Send(0, 0, MPI_INT, rank, DIETAG, MPI_COMM_WORLD);\
   }\
}

#define DEBUG 0

int myrank, numTasks;
MPI_Status status;

int N;

int main(int argc, char **argv) {
   int myrank;

   if (argc < 2) {
      printf("usage: nqueens_parallel <board dimension>\n");
      exit(1);
   }
   N = atoi(argv[1]);

   msgInit(argc, argv);

   if (myrank == 0) {
      manager();
   }
   else {
      worker();
   }

   MPI_Finalize();
   return 0;
}

void manager() {
   State work;
   Queue workQueue;
   int numOutstanding; // track outstanding messages to know when to terminate

   workQueue = qopen();

   numOutstanding = 0;

   SuccessorStates result;
   result = malloc(sizeof(SSuccessorStates));
   result->successorStates = malloc(sizeof(State) * N);

   generate_initial_workQueue(N, workQueue);

   // continue serving requests untill all have returned and there is no work left
   do {
      // distribute work 
      if (numOutstanding == 0 && workQueue->size != 0) {
         for (int rank = 1; rank < numTasks; ++rank) {
            work = get_next_work_item(workQueue);
            if (work == NULL) break;

            MPI_Send(&(work->N), 1, MPI_INT, rank, WORKTAG, MPI_COMM_WORLD);
            MPI_Send(&(work->numQueens), 1, MPI_INT, rank, WORKTAG, MPI_COMM_WORLD);
            MPI_Send(work->queenPos, N, MPI_INT, rank, WORKTAG, MPI_COMM_WORLD);

            numOutstanding++;

            free(work->queenPos);
            free(work);
         }
      }

      work = get_next_work_item(workQueue);

      MPI_Recv(&(result->numStates), 1, MPI_INT, MPI_ANY_SOURCE, WORKTAG, MPI_COMM_WORLD, &status);
      for (int i = 0; i < result->numStates; ++i) {
         State resultState = malloc(sizeof(SState));
         resultState->queenPos = malloc(sizeof(int) * N);

         MPI_Recv(&(resultState->N), 1, MPI_INT, status.MPI_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
         MPI_Recv(&(resultState->numQueens), 1, MPI_INT, status.MPI_SOURCE, WORKTAG, MPI_COMM_WORLD, &status);
         MPI_Recv(resultState->queenPos, N, MPI_INT, status.MPI_SOURCE, WORKTAG, MPI_COMM_WORLD, &status);

         result->successorStates[i] = resultState;
      }
      numOutstanding--;

      process_results(result, workQueue);

      if (work != NULL) {
         MPI_Send(&(work->N), 1, MPI_INT, status.MPI_SOURCE, WORKTAG, MPI_COMM_WORLD);
         MPI_Send(&(work->numQueens), 1, MPI_INT, status.MPI_SOURCE, WORKTAG, MPI_COMM_WORLD);
         MPI_Send(work->queenPos, N, MPI_INT, status.MPI_SOURCE, WORKTAG, MPI_COMM_WORLD);

         numOutstanding++;
      }

      free(work);

      //printf("queueSize: %i, numOutstanding: %i\n", workQueue->size, numOutstanding);
   } while (workQueue->size != 0 || numOutstanding != 0);

   msgKillAll();
     

   for (int i = 0; i < result->numStates; ++i) { 
      free(result->successorStates[i]->queenPos);
      free(result->successorStates[i]); 
   }
   free(result->successorStates);
   free(result);
   qclose(workQueue);

   return;
}

void worker() {
   MPI_Status status;
   SuccessorStates result;
   State work;

   work = malloc(sizeof(SState));
   work->queenPos = malloc(sizeof(int) * N);

   for(;;) {
      MPI_Recv(&(work->N), 1, MPI_INT, MANAGER, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
      if (status.MPI_TAG == DIETAG) { break; }
      else {
         MPI_Recv(&(work->numQueens), 1, MPI_INT, MANAGER, WORKTAG, MPI_COMM_WORLD, &status);
         MPI_Recv(work->queenPos, N, MPI_INT, MANAGER, WORKTAG, MPI_COMM_WORLD, &status);
      }

      result = do_work(work);

      MPI_Send(&(result->numStates), 1, MPI_INT, MANAGER, WORKTAG, MPI_COMM_WORLD);
      for (int i=0; i < result->numStates; ++i) {
         State resultState = result->successorStates[i];
         MPI_Send(&(resultState->N), 1, MPI_INT, MANAGER, WORKTAG, MPI_COMM_WORLD);
         MPI_Send(&(resultState->numQueens), 1, MPI_INT, MANAGER, WORKTAG, MPI_COMM_WORLD);
         MPI_Send(resultState->queenPos, N, MPI_INT, MANAGER, WORKTAG, MPI_COMM_WORLD);

         free(resultState);
      }
   }

   free(work->queenPos);
   free(work);
   //free(result);
}

State get_next_work_item(Queue workQueue) {
   State workItem;
   int result;
   void* buff;

   if (workQueue->size == 0) return NULL;

   result = qget(workQueue, &buff);
   workItem = (State) buff;
   if (result) return NULL;
   else return workItem;
}

void process_results(SuccessorStates result, Queue workQueue) {
   if (result->numStates == 0) return;
   for (int i=0; i < result->numStates; ++i) {
      State resultState = result->successorStates[i];

      if (resultState->numQueens == N) print_state(resultState);
      else {
         qput(workQueue, (void *) resultState);
      }
   }
}

SuccessorStates do_work(State work) {
   if (DEBUG) printf("Doing work...\n");
   SuccessorStates successors = solve_next_row(work);
   return successors;
}

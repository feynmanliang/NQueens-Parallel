#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>
#include "./nqueens.h"
#include "./myQueue.h"
#include "./nqueens_parallel.h"
#include "./mpi_manager_worker.h"

void generate_initial_workQueue(Queue workQueue);
result_t do_work(work_t work);
void process_results(result_t result, Queue workQueue);
void* pack_work(work_t);
work_t unpack_work(void*);

int main(int argc, char **argv) {
   if (argc < 2) {
      printf("usage: nqueens_parallel <board dimension>\n");
      exit(1);
   }
   N = atoi(argv[1]);
   mpi_main(argc,argv, &generate_initial_workQueue, &do_work, 
     &pack_work, &unpack_work, &process_results);
   return 0;
}

void generate_initial_workQueue(Queue workQueue) {
   State* initialStates;
   initialStates = generate_initial_states(N);
   for (int i = 0; i < N; ++i) { qput(workQueue, (void *) initialStates[i]); }
   free(initialStates);
}

result_t do_work(State work) {
   SuccessorStates successors = solve_next_row(work);
   return successors;
}

void process_results(SuccessorStates result, Queue workQueue) {
   if (result->numStates == 0) return;
   for (int i=0; i < result->numStates; ++i) {
      State resultState = result->successorStates[i];
      if (resultState->numQueens == resultState->N) print_state(resultState);
      else {
         qput(workQueue, (void *) resultState);
      }
   }
}

// first int indicates size of packed block
void* pack_work(work_t work) {
   int blockSize = sizeof(int) * (1 + 2 + work->numQueens);
   int* packedWork = malloc(blockSize);
   packedWork[0] = blockSize;
   packedWork[1] = work->N;
   packedWork[2] = work->numQueens;
   for (int i=0; i < work->numQueens; ++i) packedWork[3+i] = work->queenPos[i];
   return packedWork;
}

work_t unpack_work(void* packedWork) {
   int* intPackedWork = (int*) packedWork;
   work_t work = malloc(sizeof(SState));

   work->N = intPackedWork[1];
   work->numQueens = intPackedWork[2];

   work->queenPos = malloc(sizeof(int) * work->N);
   for (int i=0; i < work->numQueens; ++i) work->queenPos[i] = intPackedWork[3+i];

   return work;
}

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
void* pack_result(result_t);
result_t unpack_result(void*);

int main(int argc, char **argv) {
   if (argc < 2) {
      printf("usage: nqueens_parallel <board dimension>\n");
      exit(1);
   }
   N = atoi(argv[1]);
   mpi_main(argc,argv, &generate_initial_workQueue, &do_work, 
     &pack_work, &unpack_work, &pack_result, &unpack_result, &process_results);
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

void* pack_result(result_t result) {
   int blockSize = sizeof(int) * (1 + 1);
   for (int i=0; i < result->numStates; ++i) 
      blockSize += sizeof(int) * (3 + result->successorStates[i]->numQueens);
   
   int* packedResult = malloc(blockSize);
   int offset = 0;
   packedResult[offset++] = blockSize;
   packedResult[offset++] = result->numStates;
   for (int i=0; i < result->numStates; ++i) {
      int* packedWork = pack_work(result->successorStates[i]);
      for (int j=0; j<(packedWork[0]/sizeof(int)); ++j) 
         packedResult[offset++] = packedWork[j];
   }
   return packedResult;
}

result_t unpack_result(void* packedResult) {
   int* intPackedResult = (int*) packedResult;
   result_t result = malloc(sizeof(SSuccessorStates));

   int offset = 1;
   result->numStates = intPackedResult[offset++];
   result->successorStates = malloc(sizeof(State) * (result->numStates));
   
   for (int i=0; i<result->numStates; ++i) {
      State succ = unpack_work(intPackedResult+offset);
      result->successorStates[i] = succ;
      offset += (intPackedResult[offset]/sizeof(int));
   }

   return result;
}

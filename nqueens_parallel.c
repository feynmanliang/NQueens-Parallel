#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>
#include "./nqueens.h"
#include "./myQueue.h"
#include "./mpi_manager_worker.h"

typedef State work_t;
typedef SuccessorStates result_t;

void generate_initial_workQueue(Queue);
void* do_work(void* work);
void process_results(void* result, Queue workQueue);
void* pack_work(void* work);
void* unpack_work(void* packedWork);
void* pack_result(void* result);
void* unpack_result(void* packedResult);

int N;
int depthPerNode = 1;

int main(int argc, char **argv) {
   if (argc < 2) {
      printf("usage: nqueens_parallel <board dimension> <depth per node>\n");
      exit(1);
   }
   N = atoi(argv[1]);
   if (argc > 2) depthPerNode = atoi(argv[2]);

   mpi_main(argc, argv, depthPerNode, &generate_initial_workQueue, &do_work, 
     &pack_work, &unpack_work, &pack_result, &unpack_result, &process_results);
   return 0;
}

void generate_initial_workQueue(Queue workQueue) {
   State* initialStates;
   initialStates = generate_initial_states(N);
   for (int i = 0; i < N; ++i) { 
      qput(workQueue, (void *) initialStates[i]); }
   free(initialStates);
}

void* do_work(void* workptr) {
   work_t work = (work_t) workptr;
   result_t successors = solve_next_row(work);
   return successors;
}

void process_results(void* resultptr, Queue workQueue) {
   result_t result = (result_t) resultptr;
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
void* pack_work(void* workptr) {
   work_t work = (work_t) workptr;
   int blockSize = sizeof(int) * (1 + 2 + work->numQueens);
   int* packedWork = malloc(blockSize);
   packedWork[0] = blockSize;
   packedWork[1] = work->N;
   packedWork[2] = work->numQueens;
   for (int i=0; i < work->numQueens; ++i) packedWork[3+i] = work->queenPos[i];
   return packedWork;
}

void* unpack_work(void* packedWork) {
   int* intPackedWork = (int*) packedWork;
   work_t work = malloc(sizeof(SState));

   work->N = intPackedWork[1];
   work->numQueens = intPackedWork[2];

   work->queenPos = malloc(sizeof(int) * work->N);
   for (int i=0; i < work->numQueens; ++i) work->queenPos[i] = intPackedWork[3+i];

   return work;
}

void* pack_result(void* resultptr) {
   result_t result = (result_t) resultptr;
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

void* unpack_result(void* packedResult) {
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

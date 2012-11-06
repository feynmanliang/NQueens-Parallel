#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>
#include "./nqueens.h"
#include "./myQueue.h"
#include "./nqueens_parallel.h"
#include "./mpi_manager_worker.h"

int main(int argc, char **argv) {
   if (argc < 2) {
      printf("usage: nqueens_parallel <board dimension>\n");
      exit(1);
   }
   N = atoi(argv[1]);
   mpi_main(argc,argv);
   return 0;
}

void generate_initial_workQueue(Queue workQueue) {
   State* initialStates;
   initialStates = generate_initial_states(N);
   for (int i = 0; i < N; ++i) { qput(workQueue, (void *) initialStates[i]); }
   free(initialStates);
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

SuccessorStates do_work(State work) {
   SuccessorStates successors = solve_next_row(work);
   return successors;
}

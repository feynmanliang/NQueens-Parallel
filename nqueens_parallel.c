#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>
#include "./nqueens.h"
#include "./myQueue.h"
#include "./nqueens_parallel.h"
#include "./mpi_manager_worker.h"

// generate initial states
void generate_initial_workQueue(int N, Queue workQueue) {
   State* initialStates;

   initialStates = generate_initial_states(N);
   for (int i = 0; i < N; ++i) { qput(workQueue, (void *) initialStates[i]); }

   free(initialStates);
}

void map_work_to_tasks(int numTasks, Queue workQueue, int* numOutstanding) {
   State work;

   for (int rank = 1; rank < numTasks; ++rank) {
      work = get_next_work_item(workQueue);
      if (work == NULL) break;

      MPI_Send(&(work->N), 1, MPI_INT, rank, WORKTAG, MPI_COMM_WORLD);
      MPI_Send(&(work->numQueens), 1, MPI_INT, rank, WORKTAG, MPI_COMM_WORLD);
      MPI_Send(work->queenPos, work->N, MPI_INT, rank, WORKTAG, MPI_COMM_WORLD);

      *numOutstanding++;

      free(work->queenPos);
   }

   free(work);
}

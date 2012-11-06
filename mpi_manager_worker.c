#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "./myQueue.h"
#include "./nqueens.h" // problem specific header
#include "./nqueens_parallel.h" // problem specific header
#include "./mpi_manager_worker.h"

#define msgInit(argc, argv) {\
   MPI_Init(&argc, &argv);\
   MPI_Comm_rank(MPI_COMM_WORLD, &myrank);\
   MPI_Comm_size(MPI_COMM_WORLD, &numTasks);\
}

#define msgKillAll() {\
   for (int rank = 1; rank < numTasks; ++rank) {\
      int load;\
      MPI_Send(0, 0, MPI_INT, rank, DIETAG, MPI_COMM_WORLD);\
      MPI_Recv(&load, 1, MPI_INT, rank, DIETAG, MPI_COMM_WORLD, &status);\
      printf("Node %i computed load of %i\n", rank, load);\
   }\
}

#define msgKillNode() {\
   MPI_Send(&myLoad, 1, MPI_INT, MANAGER, DIETAG, MPI_COMM_WORLD);\
   return;\
}

#define msgSendWait(to) {\
   MPI_Send(0, 0, MPI_INT, to, WAITTAG, MPI_COMM_WORLD);\
   continue;\
}

int myrank, numTasks, myLoad;
MPI_Status status;

int mpi_main(int argc, char **argv) {
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
   Queue workQueue = qopen();
   int numOutstanding = 0; // track outstanding messages to know when to terminate

   generate_initial_workQueue(workQueue);

   // keep going until workQueue is exhausted and no outstanding nodes
   do {
      // map tasks from queue out if all workers idle
      if (numOutstanding == 0 && workQueue->size != 0) {
         for (int rank = 1; rank < numTasks; ++rank) {
            work_t work = get_next_work_item(workQueue);
            if (work == NULL) { break; }
            else {
               msgSendWork(rank, work)
               numOutstanding++;
               free(work);
            }
         }
      }

      // receive a result from an outstanding node
      result_t result;
      init_result_t(result);
      msgRecvResult(result);
      numOutstanding--;

      // process the result (also adding to workQueue)
      process_results(result, workQueue);

      // respond with new work item (if available)
      work_t work = get_next_work_item(workQueue);
      if (work != NULL) {
         int nextNode = (status.MPI_SOURCE + 1) % numTasks;
         if (nextNode == 0) nextNode++;
         msgSendWork(nextNode, work);
         numOutstanding++;
      }
      free(work);
      //printf("queueSize: %i, numOutstanding: %i\n", workQueue->size, numOutstanding);
   } while (workQueue->size != 0 || numOutstanding != 0);

   // send DIETAG to all workers
   msgKillAll();

   qclose(workQueue);
   return;
}

void worker() {
   MPI_Status status;
   work_t work;
   init_work_t(work);
   while (1) {
      msgRecvWork(MANAGER, work);
      result_t result = do_work(work);
      myLoad += result->numStates;
      msgSendResult(MANAGER, result);
      free(result);
   }
   free(work);
}

work_t get_next_work_item(Queue workQueue) {
   if (workQueue->size == 0) return NULL;

   work_t workItem;
   int result;
   void* buff;

   result = qget(workQueue, &buff);
   workItem = (State) buff;
   if (result) return NULL;
   else return workItem;
}

#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "./myQueue.h"
#include "./mpi_manager_worker.h"

#define WORKTAG 42
#define DIETAG 20
#define MANAGER 0

typedef void* work_t;
typedef void* result_t;

#define msgInit(argc, argv) {\
   MPI_Init(&argc, &argv);\
   MPI_Comm_rank(MPI_COMM_WORLD, &myrank);\
   MPI_Comm_size(MPI_COMM_WORLD, &numTasks);\
}
#define msgSendWork(to, work) {\
   void* packedWork = pack_work(work);\
   MPI_Send(packedWork, *((int*)packedWork), MPI_BYTE, to, WORKTAG, MPI_COMM_WORLD);\
}
#define msgRecvWork(from, work) {\
   free(work);\
   MPI_Probe(from, MPI_ANY_TAG, MPI_COMM_WORLD, &status);\
   if (status.MPI_TAG == DIETAG) {\
      MPI_Recv(0, 0, MPI_INT, from, MPI_ANY_TAG, MPI_COMM_WORLD, &status);\
      msgKillNode();\
   }\
   else {\
      MPI_Get_count(&status, MPI_BYTE, &msgSize);\
      int* packedWork = malloc(msgSize);\
      MPI_Recv(packedWork, msgSize, MPI_BYTE, from, WORKTAG, MPI_COMM_WORLD, &status);\
      work = unpack_work(packedWork);\
   }\
}
#define msgSendResult(to, result) {\
   void* packedResult = pack_result(result);\
   MPI_Send(packedResult, *((int*)packedResult), MPI_BYTE, to, WORKTAG, MPI_COMM_WORLD);\
}
#define msgRecvResult(result) {\
   free(result);\
   MPI_Probe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);\
   MPI_Get_count(&status, MPI_BYTE, &msgSize);\
   int* packedResult = malloc(msgSize);\
   MPI_Recv(packedResult, msgSize, MPI_BYTE, status.MPI_SOURCE, WORKTAG, MPI_COMM_WORLD, &status);\
   result = unpack_result(packedResult);\
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

int myrank, numTasks, myLoad, msgSize;
double startTime;
MPI_Status status;

int mpi_main(int argc, char **argv, void generate_initial_workQueue(Queue),
  result_t do_work(work_t), void* pack_work(work_t), work_t unpack_work(void*),
  void* pack_result(result_t), result_t unpack_result(void*), void process_results(result_t, Queue)) {
   startTime = MPI_Wtime();
   msgInit(argc, argv);

   if (myrank == 0) {
      manager(generate_initial_workQueue, pack_work, unpack_work, unpack_result, process_results);
   }
   else {
      worker(do_work, pack_work, unpack_work, pack_result);
   }

   MPI_Finalize();
   return 0;
}

void manager(void generate_initial_workQueue(Queue), void* pack_work(work_t), work_t unpack_work(void*),
  result_t unpack_result (void*), void process_results(result_t, Queue)) {
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
      result_t result = malloc(sizeof(result_t));
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
   printf("Total run time: %f\n", MPI_Wtime() - startTime);
   return;
}

void worker(result_t do_work(work_t), void* pack_work(work_t), work_t unpack_work(void*), 
  void* pack_result(result_t)) {
   MPI_Status status;
   work_t work = malloc(sizeof(work_t));
   while (1) {
      msgRecvWork(MANAGER, work);
      myLoad += 1;
      result_t result = do_work(work);

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
   workItem = (work_t) buff;
   if (result) return NULL;
   else return workItem;
}

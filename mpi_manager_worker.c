#include "nqueens.h"
#include <mpi.h>
#include <pthread.h>
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
#define msgRecvResult(from, result) {\
   free(result);\
   MPI_Probe(from, MPI_ANY_TAG, MPI_COMM_WORLD, &status);\
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

typedef struct sThreadData {
   Queue workQueue;
   void (* generate_initial_workQueue)(Queue);
   result_t (* do_work)(work_t); 
   void* (* pack_work)(work_t); 
   work_t (* unpack_work)(void*);
   void* (* pack_result)(result_t);
   result_t (* unpack_result)(void*);
   void (* process_results)(result_t, Queue);
} SThreadData;
typedef SThreadData* ThreadData;
void* generator_thread(void* threadDataptr);
void* listener_thread(void* threadDataptr);

pthread_t callThd[2];
int myrank, numTasks, myLoad, msgSize;
double startTime;
MPI_Status status;

int generatorComplete = 0;
int depthPerNode;

int mpi_main(int argc, char **argv, int depthPerNode, void generate_initial_workQueue(Queue),
  result_t do_work(work_t), void* pack_work(work_t), work_t unpack_work(void*),
  void* pack_result(result_t), result_t unpack_result(void*), void process_results(result_t, Queue)) {
   startTime = MPI_Wtime();
   msgInit(argc, argv);

   if (myrank == 0) {
      manager(generate_initial_workQueue, pack_work, unpack_work, unpack_result, process_results);
   }
   else {
      worker(do_work, pack_work, unpack_work, process_results, pack_result);
   }

   MPI_Finalize();
   pthread_exit(NULL);
}


void* generator_thread(void* threadDataptr) {
   ThreadData threadData = (ThreadData) threadDataptr;
   (threadData->generate_initial_workQueue)(threadData->workQueue);
   generatorComplete=1;
   pthread_exit(NULL);
}

void* listener_thread(void* threadDataptr) {
   ThreadData threadData = (ThreadData) threadDataptr;
   Queue workQueue = threadData->workQueue;
   void* (*pack_work)(void*) = threadData->pack_work;
   void* (*unpack_result)(void*) = threadData->unpack_result;
   void (*process_results)(void*, Queue) = threadData->process_results;

   int numOutstanding = 0; // track outstanding messages to know when to terminate
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

      if (numOutstanding != 0) {
         int numResults;
         MPI_Recv(&numResults, 1, MPI_INT, MPI_ANY_SOURCE, WORKTAG, MPI_COMM_WORLD, &status);
         numOutstanding--;
         for (int i=0; i<numResults; ++i) {
            // receive a result from an outstanding node
            result_t result = malloc(sizeof(result_t));
            msgRecvResult(status.MPI_SOURCE, result);

            // process the result (also adding to workQueue)
            process_results(result, workQueue);
         }

         // respond with new work item (if available)
         work_t work = get_next_work_item(workQueue);
         if (work != NULL) {
            int nextNode = (status.MPI_SOURCE + 1) % numTasks;
            if (nextNode == 0) nextNode++;
            msgSendWork(nextNode, work);
            numOutstanding++;
         }
         free(work);
      }
   } while (!generatorComplete || workQueue->size != 0 || numOutstanding != 0);
   pthread_exit(NULL);
}

void manager(void generate_initial_workQueue(Queue), void* pack_work(work_t), work_t unpack_work(void*),
  result_t unpack_result (void*), void process_results(result_t, Queue)) {
   Queue workQueue = qopen();

   ThreadData threadData = malloc(sizeof(SThreadData));
   threadData->workQueue = workQueue;
   threadData->generate_initial_workQueue = generate_initial_workQueue;
   threadData->pack_work = pack_work;
   threadData->unpack_work = unpack_work;
   threadData->unpack_result = unpack_result;
   threadData->process_results = process_results;

   pthread_create(&callThd[0], NULL, generator_thread, (void*) threadData);
   pthread_create(&callThd[1], NULL, listener_thread, (void*) threadData);

   pthread_join(callThd[0], NULL);
   pthread_join(callThd[1], NULL);

   // send DIETAG to all workers
   msgKillAll();

   qclose(workQueue);
   printf("Total run time: %f\n", MPI_Wtime() - startTime);
   return;
}

void worker(result_t do_work(work_t), void* pack_work(work_t), work_t unpack_work(void*), 
  void process_results(result_t, Queue), void* pack_result(result_t)) {
   MPI_Status status;
   Queue workQueue = qopen();
   Queue resultQueue = qopen();
   work_t work = malloc(sizeof(work_t));
   result_t result = malloc(sizeof(result_t));

   while (1) {
      int numResults = 0;
      msgRecvWork(MANAGER, work);
      qput(workQueue, work);

      for (int depth=0; depth < depthPerNode-1; ++depth) {
         qget(workQueue, &work);
         if (work != NULL) {
            myLoad += 1;
            result = do_work(work);
            process_results(result, workQueue);
         }
      }

      while (workQueue->size != 0) {
         qget(workQueue, &work);
         if (work != NULL) {
            myLoad += 1;
            result = do_work(work);
            qput(resultQueue, result);
            numResults++;
         }
      }

      MPI_Send(&numResults, 1, MPI_INT, MANAGER, WORKTAG, MPI_COMM_WORLD);
      for (int i=0; i<numResults; ++i) {
         qget(resultQueue, &result);
         msgSendResult(MANAGER, result);
      }
   }
   free(work);
   qclose(workQueue);
   qclose(resultQueue);
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

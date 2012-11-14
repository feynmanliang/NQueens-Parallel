#include <mpi.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "./myQueue.h"
#include "./mpi_manager_worker.h"

#define WORKTAG 42
#define WORKREQ 33
#define DELAYTAG 23
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
   void* packedWork = p_pack_work(work);\
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
      work = p_unpack_work(packedWork);\
   }\
}
#define msgSendResult(to, result) {\
   void* packedResult = p_pack_result(result);\
   MPI_Send(packedResult, *((int*)packedResult), MPI_BYTE, to, WORKTAG, MPI_COMM_WORLD);\
}
#define msgRecvResult(from, result) {\
   free(result);\
   MPI_Probe(from, MPI_ANY_TAG, MPI_COMM_WORLD, &status);\
   MPI_Get_count(&status, MPI_BYTE, &msgSize);\
   int* packedResult = malloc(msgSize);\
   MPI_Recv(packedResult, msgSize, MPI_BYTE, status.MPI_SOURCE, WORKTAG, MPI_COMM_WORLD, &status);\
   result = p_unpack_result(packedResult);\
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

MPI_Status status;
int myrank, numTasks, myLoad, msgSize;
double startTime;

pthread_t callThd[2];
void* generator_thread(void* threadDataptr);
int generatorComplete = 0;
void* listener_thread(void* threadDataptr);
int depthPerNode;

void (*p_generate_initial_workQueue)(Queue);
result_t (*p_do_work)(work_t);
void* (*p_pack_work)(work_t);
work_t (*p_unpack_work)(void*);
void* (*p_pack_result)(result_t);
result_t (*p_unpack_result)(void*);
void (*p_process_results)(result_t, Queue);

int mpi_main(int argc, char **argv, int depthPerNode, void generate_initial_workQueue(Queue),
  result_t do_work(work_t), void* pack_work(work_t), work_t unpack_work(void*),
  void* pack_result(result_t), result_t unpack_result(void*), void process_results(result_t, Queue)) {
   startTime = MPI_Wtime();
   msgInit(argc, argv);

   // set global function pointers to be used by subroutines
   p_generate_initial_workQueue = generate_initial_workQueue;
   p_do_work = do_work;
   p_pack_work = pack_work;
   p_unpack_work = unpack_work;
   p_pack_result = pack_result;
   p_unpack_result = unpack_result;
   p_process_results = process_results;

   if (myrank == 0) {
      manager();
   }
   else {
      worker();
   }

   MPI_Finalize();
   pthread_exit(NULL);
}


// this thread generates initial subproblems and sets a boolean when done
void* generator_thread(void* p_queue) {
   p_generate_initial_workQueue((Queue) p_queue);
   generatorComplete=1;
   pthread_exit(NULL);
}

// this thread serves work to incoming connections and processes results
void* listener_thread(void* p_queue) {
   Queue workQueue = (Queue) p_queue;
   int numOutstanding = 0; // track outstanding nodes to know when to terminate

   do {
      MPI_Probe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
      if (status.MPI_TAG == WORKREQ) {
         MPI_Recv(0, 0, MPI_INT, status.MPI_SOURCE, WORKREQ, MPI_COMM_WORLD, &status);
         work_t work = get_next_work_item(workQueue);
         if (work != NULL) { // we have a work item for you!
            msgSendWork(status.MPI_SOURCE, work);
            free(work);
            numOutstanding++;
         }
         else if (!generatorComplete || numOutstanding != 0) { // still chance for new work, come back later
            MPI_Send(0, 0, MPI_INT, status.MPI_SOURCE, DELAYTAG, MPI_COMM_WORLD);
         }
      }

      else {
         int numResults;
         MPI_Recv(&numResults, 1, MPI_INT, status.MPI_SOURCE, WORKTAG, MPI_COMM_WORLD, &status);
         numOutstanding--;
         for (int i=0; i<numResults; ++i) {
            // receive a result from an outstanding node
            result_t result = malloc(sizeof(result_t));
            msgRecvResult(status.MPI_SOURCE, result);

            // process the result (also adding to workQueue)
            p_process_results(result, workQueue);
         }

         // respond with new work item (if available)
         work_t work = get_next_work_item(workQueue);
         if (work != NULL) {
            int nextNode = (status.MPI_SOURCE);
            if (nextNode == 0) nextNode++;
            msgSendWork(nextNode, work);
            numOutstanding++;
         }
         free(work);
      }
   } while (!generatorComplete || workQueue->size != 0 || numOutstanding != 0);
   pthread_exit(NULL);
}

void manager() {
   Queue workQueue = qopen();

   pthread_create(&callThd[0], NULL, generator_thread, (void*) workQueue);
   pthread_create(&callThd[1], NULL, listener_thread, (void*) workQueue);

   pthread_join(callThd[0], NULL);
   pthread_join(callThd[1], NULL);

   // send DIETAG to all workers
   msgKillAll();

   qclose(workQueue);
   printf("Total run time: %f\n", MPI_Wtime() - startTime);
   return;
}

void worker() {
   MPI_Status status;
   Queue workQueue = qopen();
   Queue resultQueue = qopen();
   work_t work = malloc(sizeof(work_t));
   result_t result = malloc(sizeof(result_t));

   MPI_Send(0, 0, MPI_INT, MANAGER, WORKREQ, MPI_COMM_WORLD);
   while (1) {
      MPI_Probe(MANAGER, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
      if (status.MPI_TAG == DELAYTAG) {
         MPI_Recv(0, 0, MPI_INT, MANAGER, DELAYTAG, MPI_COMM_WORLD, &status);
         MPI_Send(0, 0, MPI_INT, MANAGER, WORKREQ, MPI_COMM_WORLD);
         continue;
      }
      msgRecvWork(MANAGER, work);
      MPI_Send(0, 0, MPI_INT, MANAGER, WORKREQ, MPI_COMM_WORLD);
      qput(workQueue, work);

      for (int depth=0; depth < depthPerNode-1; ++depth) {
         while (workQueue->size != 0) {
            qget(workQueue, &work);
            myLoad += 1;
            result = p_do_work(work);
            qput(resultQueue, result);
         }
         while (resultQueue->size != 0) {
            qget(resultQueue, &result);
            p_process_results(result, workQueue);
         }
      }

      int numResults = 0;
      while (workQueue->size != 0) {
         qget(workQueue, &work);
         if (work != NULL) {
            myLoad += 1;
            result = p_do_work(work);
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>
#include "./nqueens.h"
#include "./myQueue.h"

#define WORKTAG 42
#define DIETAG 20

#define msgsendwork(data, to) {\
   MPI_Send(&(data->N), 1, MPI_INT, to, WORKTAG, MPI_COMM_WORLD);\
   MPI_Send(&(data->numQueens), 1, MPI_INT, to, WORKTAG, MPI_COMM_WORLD);\
   MPI_Send(data->queenPos, N, MPI_INT, to, WORKTAG, MPI_COMM_WORLD);\
}

#define msgsendresult(data, to) {\
   MPI_Send(&(data->numStates), 1, MPI_INT, to, WORKTAG, MPI_COMM_WORLD);\
   for (int m=0; m<data->numStates; ++m) {\
      msgsendwork(data->successorStates[m], to);\
   }\
}

#define msgrecvwork(data, from) {\
   MPI_Recv(&(data->N), 1, MPI_INT, from, WORKTAG, MPI_COMM_WORLD, &status);\
   MPI_Recv(&(data->numQueens), 1, MPI_INT, from, WORKTAG, MPI_COMM_WORLD, &status);\
   MPI_Recv(data->queenPos, N, MPI_INT, from, WORKTAG, MPI_COMM_WORLD, &status);\
}

#define msgrecvresult(buffp) \
  MPI_Recv(buffp, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

#define msgrespondwork(buffp) \
  MPI_Send(buffp, 1, MPI_INT, status.MPI_SOURCE, WORKTAG, MPI_COMM_WORLD);

#define msgsenddie(to) \
  MPI_Send(0, 0, MPI_INT, to, DIETAG, MPI_COMM_WORLD);

#define MANAGER 0

void manager(void);
void worker(void);
State get_next_work_item(Queue workQueue);
void process_results(SuccessorStates result, Queue workQueue);
SuccessorStates do_work(State work);
void define_workitem_derivedtype();
void define_result_derivedtype();

int N;
MPI_Status status;

int main(int argc, char **argv) {
   int myrank;

   // set up vars used by all nodes
   if (argc < 2) {
      printf("usage: nqueens_parallel <board dimension>\n");
      exit(1);
   }
   N = atoi(argv[1]);

   MPI_Init(&argc, &argv);

   MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
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
   int numTasks, rank;
   State work;
   SuccessorStates result = malloc(sizeof(SSuccessorStates));

   MPI_Comm_size(MPI_COMM_WORLD, &numTasks);

   Queue workQueue = qopen();
   printf("Queue opened\n");

   // generate initial states
   State* initialStates = generate_initial_states(N);
   for (int i=0; i < N; ++i) {
      printf("Generating initial state %i\n", i);
      qput(workQueue, (void *) initialStates[i]);
   }

   printf("numTasks: %i\n", numTasks);
   // Seed the workers
   for (int rank = 1; rank<numTasks; ++rank) {
      work = get_next_work_item(workQueue);
      printf("Sending to worker %i the following state:\n", rank);
      print_state(work);

      MPI_Send(&(work->N), 1, MPI_INT, rank, WORKTAG, MPI_COMM_WORLD);
      MPI_Send(&(work->numQueens), 1, MPI_INT, rank, WORKTAG, MPI_COMM_WORLD);
      MPI_Send(work->queenPos, N, MPI_INT, rank, WORKTAG, MPI_COMM_WORLD);
   }

   work = get_next_work_item(workQueue);
   // TODO: This currently depends on seeding the workers to not exhaust the initial queue
   while (work != NULL) {
      MPI_Recv(buffp, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
      MPI_Recv(&(result->numStates), 1, MPI_INT, MANAGER, WORKTAG, MPI_COMM_WORLD, &status);
      for (int i=0; i < result->numStates; ++i) {
         State resultState = result->successorStates[i];
         MPI_Recv(&(work->N), 1, MPI_INT, MANAGER, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
         MPI_Recv(&(work->numQueens), 1, MPI_INT, MANAGER, WORKTAG, MPI_COMM_WORLD, &status);
         MPI_Recv(work->queenPos, N, MPI_INT, MANAGER, WORKTAG, MPI_COMM_WORLD, &status);
      }

      printf("Received from worker %i\n", status.MPI_SOURCE);
      printf("numStates: %i", result->numStates);

      // process the received message?
      process_results(result, workQueue);

      msgrespondwork(work);
      printf("Sending to worker %i\n", status.MPI_SOURCE);
      work = get_next_work_item(workQueue);
   }

   // receive all outstanding results (should be all complete solutions)
   for (rank = 1; rank < numTasks; ++rank) {
      msgrecvresult(result);
      process_results(result, workQueue);
   }

   // tell slaves to die
   for (rank = 1; rank < numTasks; ++rank) {
      msgsenddie(rank);
   }
}

void worker() {
   State work = malloc(sizeof(SState));
   work->queenPos= malloc(sizeof(int) * N);

   SuccessorStates result;
   for(;;) {
      MPI_Recv(&(work->N), 1, MPI_INT, MANAGER, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
      if (status.MPI_TAG == DIETAG) {
         free(work);
         return;
      }
      else {
         MPI_Recv(&(work->numQueens), 1, MPI_INT, MANAGER, WORKTAG, MPI_COMM_WORLD, &status);
         MPI_Recv(work->queenPos, N, MPI_INT, MANAGER, WORKTAG, MPI_COMM_WORLD, &status);
      }

      printf("Received state with %i queens:\n", work->numQueens);
      print_state(work);

      result = do_work(work);

      printf("Sending to master the following successors:\n");
      for (int i=0;i<result->numStates;++i) {
         print_state(result->successorStates[i]);
      }

      MPI_Send(&(result->numStates), 1, MPI_INT, MANAGER, WORKTAG, MPI_COMM_WORLD);
      for (int i=0; i < result->numStates; ++i) {
         State resultState = result->successorStates[i];
         MPI_Send(&(resultState->N), 1, MPI_INT, MANAGER, WORKTAG, MPI_COMM_WORLD);
         MPI_Send(&(resultState->numQueens), 1, MPI_INT, MANAGER, WORKTAG, MPI_COMM_WORLD);
         MPI_Send(resultState->queenPos, N, MPI_INT, MANAGER, WORKTAG, MPI_COMM_WORLD);
      }
   }
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
   for (int i=0; i < result->numStates; ++i) {
      State resultState = result->successorStates[i];

      if (resultState->numQueens == N) print_state(resultState);
      else {
         qput(workQueue, (void *) resultState);
      }
   }
}

SuccessorStates do_work(State work) {
   printf("Doing work...\n");
   SuccessorStates successors = solve_next_row(work);
   return successors;
}

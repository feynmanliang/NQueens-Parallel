// Custom datatypes
#define work_t State
#define result_t SuccessorStates

// Helpers to initialize/free custom datatypes
#define init_result_t(result) {\
   extern int N;\
   result = malloc(sizeof(SSuccessorStates));\
   result->successorStates = malloc(sizeof(State) * N);\
}

#define init_work_t(work) {\
   extern int N;\
   work = malloc(sizeof(SState));\
   work->queenPos = malloc(sizeof(int) * N);\
}

// MPI Messaging Macros
#define msgSendWork(to, work) {\
   MPI_Send(&(work->N), 1, MPI_INT, to, WORKTAG, MPI_COMM_WORLD);\
   MPI_Send(&(work->numQueens), 1, MPI_INT, to, WORKTAG, MPI_COMM_WORLD);\
   MPI_Send(work->queenPos, work->N, MPI_INT, to, WORKTAG, MPI_COMM_WORLD);\
}

#define msgRecvWork(from, work) {\
   MPI_Recv(&(work->N), 1, MPI_INT, from, MPI_ANY_TAG, MPI_COMM_WORLD, &status);\
   if (status.MPI_TAG == DIETAG) { break; }\
   else {\
      MPI_Recv(&(work->numQueens), 1, MPI_INT, from, WORKTAG, MPI_COMM_WORLD, &status);\
      MPI_Recv(work->queenPos, work->N, MPI_INT, from, WORKTAG, MPI_COMM_WORLD, &status);\
   }\
}

#define msgSendResult(to, result) {\
   MPI_Send(&(result->numStates), 1, MPI_INT, to, WORKTAG, MPI_COMM_WORLD);\
   for (int i=0; i < result->numStates; ++i) {\
      State resultState = result->successorStates[i];\
      msgSendWork(to, resultState);\
      free(resultState);\
   }\
}

#define msgRecvResult(result) {\
   extern int N;\
   MPI_Recv(&(result->numStates), 1, MPI_INT, MPI_ANY_SOURCE, WORKTAG, MPI_COMM_WORLD, &status);\
   for (int i = 0; i < result->numStates; ++i) {\
      State resultState = malloc(sizeof(SState));\
      resultState->queenPos = malloc(sizeof(int) * N);\
      msgRecvWork(status.MPI_SOURCE, resultState);\
      result->successorStates[i] = resultState;\
   }\
}

public int N;

public void generate_initial_workQueue(Queue workQueue);
public void process_results(SuccessorStates result, Queue workQueue);
public SuccessorStates do_work(State work);

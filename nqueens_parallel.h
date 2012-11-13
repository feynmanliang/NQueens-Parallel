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

//#define msgSendResult(to, result) {\
   MPI_Send(&(result->numStates), 1, MPI_INT, to, WORKTAG, MPI_COMM_WORLD);\
   for (int i=0; i < result->numStates; ++i) {\
      State resultState = result->successorStates[i];\
      msgSendWork(to, resultState);\
      free(resultState);\
   }\
}

//#define msgRecvResult(result) {\
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

#include <stdio.h>
#include <stdlib.h>
#include "nqueens.h"
#include "myQueue.h"

#define NO_QUEEN -1

int main(int argc, char *argv[]) {
  int N;
  Queue stateQueue;
  State* initialStates;
  void* buff;

  if (argc < 2) {
    printf("usage: nqueens <board size>\n");
    exit(1);
  }

  N = atoi(argv[1]);
  stateQueue = qopen();

  initialStates = generate_initial_states(N);
  for (int i=0; i < N; ++i) {
    qput(stateQueue, (void *) initialStates[i]);
  }


  while(stateQueue->size != 0) {
    qget(stateQueue, &buff);
    State stateToSolve = (State) buff;
    if (stateToSolve->numQueens == N) print_state(stateToSolve);
    else {
      SuccessorStates successors = solve_next_row(stateToSolve);
      for (int i=0; i < successors->numStates; ++i) 
        qput(stateQueue, (void *) successors->successorStates[i]);
    }
  }

  free(initialStates);
  qclose(stateQueue);
  return 0;
}

State* generate_initial_states(int N) {
  State* states;

  states = malloc(sizeof(State) * N);
  for (int i=0; i < N; ++i) {
    State currentState;
    currentState = malloc(sizeof(SState));
    currentState->N = N;
    currentState->numQueens = 1;
    currentState->queenPos = generate_empty_board(N);
    currentState->queenPos[0] = i;
    states[i] = currentState;
  }
  return states;
}

int* generate_empty_board(int N) {
  int* row;

  row = malloc(sizeof(int) * N);
  for (int i=0; i < N; ++i) row[i] = NO_QUEEN;
  return row;
}

void print_state(State state) {
  int N = state->N;
  printf("-------\n");
  for (int i=0; i < N; ++i) {
    int rowQueenPos = state->queenPos[i];
    for (int j=0; j < N; ++j) {
      if ((rowQueenPos == NO_QUEEN) || (rowQueenPos != j)) printf(" . ");
      else printf(" Q ");
    }
    printf("\n");
  }
  printf("-------\n");
}

SuccessorStates solve_next_row(State state) {
  int N = state->N;
  SuccessorStates successors = malloc(sizeof(SSuccessorStates));
  State* nextStates = malloc(sizeof(State) * N);
  int numSolutions = 0;
  int nextRow = state->numQueens;

  // all possible N col positions
  for (int nextCol =0; nextCol<N; ++nextCol) {
    int validMove = 1;

    for (int row=0; row<nextRow; ++row) {
      int col = state->queenPos[row];
      if ((state->queenPos[row] == nextCol) || (abs(row-nextRow) == abs(col-nextCol)))
        validMove = 0;
    }

    if (validMove) {
      State nextState = copy_state(state);
      nextState->numQueens = state->numQueens + 1;
      nextState->queenPos[nextRow] = nextCol;
      nextStates[numSolutions] = nextState;
      numSolutions++;
    }
  }

  successors->numStates = numSolutions;
  successors->successorStates = nextStates;
  return successors;
}

/* copies the state instance and returns pointer to new instance */
State copy_state(State state) {
  State stateCopy = malloc(sizeof(SState));
  stateCopy->N = state->N;
  stateCopy->numQueens = state->numQueens;
  stateCopy->queenPos = generate_empty_board(state->N);
  for (int i=0; i<(state->N); ++i) {
    stateCopy->queenPos[i] = state->queenPos[i];
  }
  return stateCopy;
}

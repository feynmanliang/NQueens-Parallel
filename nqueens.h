#define public
#define private

/* queenPos[i] = j implies row i has queen at column j*/
typedef struct sState {
   int N;
   int numQueens;
   int* queenPos;
} SState;
typedef SState* State;

typedef struct sSuccessorStates {
   int numStates ;
   State* successorStates;
} SSuccessorStates;

typedef SSuccessorStates* SuccessorStates;

public SuccessorStates solve_next_row(State);
public State* generate_initial_states(int);
public void print_state(State);

private int* generate_empty_board(int);
private State copy_state(State);

#define public
#define private

/* queenPos[i] = j implies row i has queen at column j*/
typedef struct sState {
   int N;
   int numQueens;
   int* queenPos;
} SState;

typedef SState* State;

typedef struct sNextRow {
   int numSolutions;
   int* validColumns;
} SNextRow;

typedef SNextRow* NextRow;

public void solve_state(int, State);
NextRow solve_next_layer(State);
private void setup_state(State);
private void place_queen(int, int);
private int legal_move(int, int);
private int on_board(int, int);
private void initialize_board();
private void clear_board();
private void print_board();

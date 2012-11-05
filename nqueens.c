#include <stdio.h>
#include <stdlib.h>
#include "./nqueens.h"
int N;
int queens_left;
char **board;

#define DEBUG 0

/*int main(int argc, char *argv[]) {
  if (argc < 2) {
  printf("usage: nqueens <board size>\n");
  exit(1);
  }
  int state[] = {0, 2};
  solve_state(atoi(argv[1]), state);

  return 0;
  }*/

void solve_state(int board_dim, State state) {
   N = board_dim;
   queens_left = N;

   initialize_board();
   setup_state(state);

   place_queen(N - queens_left,0);
}


NextRow solve_next_layer(State state) {
   NextRow next = malloc(sizeof(SNextRow));
   int numSolutions;
   int rowToPlace = state->numQueens;
   N = state->N;
   queens_left = N;

   initialize_board();
   setup_state(state);

   int* validCols = malloc(sizeof(int) * N);
   for (int i=0; i < N; ++i) {
      if (legal_move(rowToPlace, i)) {
         validCols[numSolutions] = i;
         numSolutions++;
      }
   }

   validCols = realloc(validCols, sizeof(int)*numSolutions);
   next->numSolutions = numSolutions;
   next->validColumns = validCols;
   return next;
}

void place_queen(int row, int col) {
   if (on_board(row, col)) {
      if (legal_move(row, col)) {
         board[row][col] = 'Q';
         queens_left--;

         if (queens_left == 0) { print_board(); }

         place_queen(row+1, 0);

         board[row][col] = '*';
         queens_left++;
      }
      place_queen(row, col+1);
   }
}

int legal_move(int row, int col) {
   int queen_found = 0;

   /* check row and col */
   for (int i=0; i < N; ++i) {
      if (board[row][i] == 'Q' || board[i][col] == 'Q') { queen_found = 1; }
   }
   /* check diagonals */
   for (int i=0; i < N; ++i) {
      for (int j=0; j < N; ++j) {
         if (abs(i-row) == abs(j-col)) {
            if (board[i][j] == 'Q') { queen_found = 1; }
         }
      }
   }

   if (queen_found == 1) { return 0; }
   else { return 1; }
}

int on_board(int row, int col) {
   if (0 <= row && 0 <= col && N > row && N > col) { return 1; }
   else { return 0; }
}

void initialize_board() {
   board = malloc(sizeof(char *) * N);
   for (int i=0; i < N; ++i) {
      board[i] = malloc( sizeof(char) );
   }
   clear_board();
}

void setup_state(State state) {
   int row, col;
   int * queen_positions;

   clear_board();
   if (state != NULL) {
      queen_positions = (int *) state->queenPos;
      for (int i=0; i < state->numQueens; i++) {
         row = i;
         col = queen_positions[i];
         board[row][col] = 'Q';
         queens_left--;
      }
   }
}

void clear_board() {
   for (int i=0; i < N; ++i) {
      for (int j=0; j < N; ++j) {
         board[i][j] = '*';
      }
   }
}

void print_board(){
   printf("-solution-\n");
   for (int i=0; i < N; ++i) {
      for (int j=0; j < N; ++j) {
         printf("%c ", board[i][j]);
      }
      printf("\n");
   }
   printf("---------\n");
}

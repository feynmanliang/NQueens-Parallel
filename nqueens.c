#include <stdio.h>
#include <stdlib.h>
void place_queen(int, int);
int legal_move(int, int);
int on_board(int, int);
void initialize_board();
void print_board();

int N;
char **board;

int main(int argc, char *argv[]) {

   if (argc < 2) {
      printf("usage: nqueens <board size>\n");
      exit(1);
   }
   N = atoi(argv[1]);
   initialize_board();
   print_board();


   return 0;
}

void place_queen(int row, int col) {
   if (on_board(row, col)) {
      if (legal_move(row, col)) {
         board[row][col] = 'Q';
         place_queen(row+1, 0);
         board[row][col] = '*';
      }
      place_queen(row, col+1);
   }
}

int legal_move(int row, int col) {
   int queen_found = 0;
   /* check col */
   /* check left diag */
   /* check diag */
   if (queen_found) { return 0; }
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
   for (int i=0; i < N; ++i) {
      for (int j=0; j < N; ++j) {
         board[i][j] = '*';
      }
   }
}

void print_board(){
   for (int i=0; i < N; ++i) {
      for (int j=0; j < N; ++j) {
         printf("%c ", board[i][j]);
      }
      printf("\n");
   }
}

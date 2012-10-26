#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include "./nqueens.h"
#include "./mpi_manager_worker.h"
void generate_states(int, int*);

int main(int argc, char **argv) {
   int N;
   int *states;

   if (argc < 2) {
      printf("usage: nqueens_parallel <board dimension>\n");
      exit(1);
   }
   N = atoi(argv[1]);

   /*states = malloc(sizeof(int *) * N);
   generate_states(N, states);*/

   /*states = malloc(sizeof(int *) * 2);
   states[0] = 0;
   states[1] = 1;*/
   states = NULL;

   solve_state(N, states);
}

void generate_states(int N, int* states) {
   for (int i=0; i < N; ++i) {
   }
}

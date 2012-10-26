#define public
#define private

/* takes a list of ints, reads two at a time and places queens in corresponding row,col pairs */
public void solve_state(int, int[]);

private void setup_state(void *);
private void place_queen(int, int);
private int legal_move(int, int);
private int on_board(int, int);
private void initialize_board();
private void print_board();


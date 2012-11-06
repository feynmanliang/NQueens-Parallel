#define WORKTAG 42
#define DIETAG 20
#define MANAGER 0

public int mpi_main(int argc, char **argv);

private void manager();
private void worker();
private work_t get_next_work_item(Queue workQueue);

#define WORKTAG 42
#define DIETAG 20
#define MANAGER 0

public int mpi_main(int argc, char **argv, void generate_initial_workQueue(Queue),
  result_t do_work(work_t), void* pack_work(work_t), work_t unpack_work(void*),
  void* pack_result(result_t), result_t unpack_result(void*), void process_results(result_t, Queue));

private void manager(void generate_initial_workQueue(Queue), void* pack_work(work_t), work_t unpack_work(void*),
  result_t unpack_result(void*), void process_results(result_t, Queue));
private void worker(result_t do_work(work_t), void* pack_work(work_t), work_t unpack_work(void*),
  void* pack_result(result_t));
private work_t get_next_work_item(Queue workQueue);

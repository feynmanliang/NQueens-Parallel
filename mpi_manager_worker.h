public int mpi_main(int argc, char **argv, int depthPerNode, void generate_initial_workQueue(Queue),
  void* do_work(void*), void* pack_work(void*), void* unpack_work(void*),
  void* pack_result(void*), void* unpack_result(void*), void process_results(void*, Queue));

private void manager();
private void worker();
private void* get_next_work_item(Queue workQueue);

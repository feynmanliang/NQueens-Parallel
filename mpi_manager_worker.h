public int mpi_main(int argc, char **argv, void* generate_initial_workQueue(void*),
  void* do_work(void*), void* pack_work(void*), void* unpack_work(void*),
  void* pack_result(void*), void* unpack_result(void*), void process_results(void*, Queue));

private void manager(void* generate_initial_workQueue(void*), void* pack_work(void*), void* unpack_work(void*),
  void* unpack_result(void*), void process_results(void*, Queue));
private void worker(void* do_work(void*), void* pack_work(void*), void* unpack_work(void*),
  void* pack_result(void*));
private void* get_next_work_item(Queue workQueue);

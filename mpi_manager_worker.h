#define WORKTAG 42
#define DIETAG 20
#define MANAGER 0

void manager();
void worker();
State get_next_work_item(Queue workQueue);
void process_results(SuccessorStates result, Queue workQueue);
SuccessorStates do_work(State work);

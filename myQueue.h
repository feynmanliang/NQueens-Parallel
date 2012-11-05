/* 
 * queue.h -- public interface to the queue module
 */
#define public
#define private

typedef struct sNode {
   void* data;
   struct sNode* next;
} SNode;

typedef SNode* Node;

typedef struct SQueue {
   int size;
   Node head;
   Node last;
} SQueue;

typedef SQueue* Queue;

/* create an empty queue */
public Queue qopen();        

/* deallocate a queue, assuming every element has been removed and deallocated */
public int qclose(Queue queue);   

/* put element at end of queue */
public int qput(Queue queue, void *elementp); 

/* get first element from a queue */
public int qget(Queue queue, void** elementp);

/* apply a void function (e.g. a printing fn) to every element of a queue */
public int qapply(Queue queue, void (*fn)(void* elementp));

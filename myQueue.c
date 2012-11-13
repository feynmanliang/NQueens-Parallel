/*
 * myQueue.c - Linked list impementation of a queue
 */
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include "./myQueue.h"

Queue qopen(void) {
   Queue queue;
   queue = malloc(sizeof(SQueue));
   pthread_mutex_init(&(queue->queueLock), NULL);
   queue->size = 0;
   queue->head = queue->last = NULL;
   return queue;
}

int qclose(Queue queue) {
   pthread_mutex_lock(&(queue->queueLock));
   if (!queue->size) {
      free(queue);
      pthread_mutex_unlock(&(queue->queueLock));
      return 0;
   }
   else {
      printf("Queue still has elements, can't deallocate!");
      pthread_mutex_unlock(&(queue->queueLock));
      return 1;
   }
}

int qput(Queue queue, void *elementp) {
   int result = 0;
   Node newNode;

   newNode = malloc(sizeof(SNode));
   pthread_mutex_lock(&(queue->queueLock));
   if (newNode) {
      newNode->data = elementp;
      newNode->next = NULL;
      if (!queue->size) queue->head = newNode;
      else queue->last->next = newNode;
      queue->last = newNode;
      queue->last = newNode;
      queue->size++;
   }
   else result=1;
   pthread_mutex_unlock(&(queue->queueLock));

   return result;
}

int qget(Queue queue, void** elementp) {
   int result = 0;
   Node aux;
   pthread_mutex_lock(&(queue->queueLock));
   if(!queue->size) {
      if (elementp) *elementp= NULL;
      result = 1;
   }
   else {
      if (elementp) *elementp = queue->head->data;
      if (queue->size == 1) {
         free(queue->head);
         queue->last=NULL;
         queue->head=NULL;
      }
      else {
         aux=queue->head;
         queue->head = queue->head->next;
         free(aux);
      }
      queue->size--;
   }
   pthread_mutex_unlock(&(queue->queueLock));
   return result;
}

/* apply a void function (e.g. a printing fn) to every element of a queue */
int qapply(Queue queue, void (*fn)(void* elementp)) {
   int result = 0;
   Node aux;

   if (queue->size==0) result = 1;
   else {
      for(aux=queue->head;aux;aux=aux->next)
         fn(aux->data);
   }
   return result;
}

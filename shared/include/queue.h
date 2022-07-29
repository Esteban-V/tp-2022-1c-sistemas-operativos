#ifndef P_QUEUE_H_
#define P_QUEUE_H_

#include <commons/collections/queue.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdlib.h>

typedef struct pQueue
{
	t_queue *lib_queue;
	pthread_mutex_t mutex;
	sem_t sem;
} t_pQueue;

t_pQueue *pQueue_create();

void pQueue_destroy(t_pQueue *queue, void (*destroyer)(void *));

void pQueue_put(t_pQueue *queue, void *elem);
int pQueue_size(t_pQueue *queue);
void *pQueue_take(t_pQueue *queue);

bool pQueue_isEmpty(t_pQueue *queue);

void pQueue_iterate(t_pQueue *queue, void (*closure)(void *));

void pQueue_sort(t_pQueue *queue, bool (*algorithm)(void *, void *));

void *pQueue_removeBy(t_pQueue *queue, bool (*condition)(void *));

void pQueue_lock(t_pQueue *queue);

void pQueue_unlock(t_pQueue *queue);

void *pQueue_takeLast(t_pQueue *queue);

void *pQueue_peek(t_pQueue *queue);

#endif /* P_QUEUE_H_ */

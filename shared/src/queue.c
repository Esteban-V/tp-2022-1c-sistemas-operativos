#include "queue.h"

t_pQueue *pQueue_create()
{
	t_pQueue *queue = malloc(sizeof(t_pQueue));
	queue->lib_queue = queue_create();
	pthread_mutexattr_t mta;
	pthread_mutexattr_init(&mta);
	pthread_mutexattr_settype(&mta, PTHREAD_MUTEX_RECURSIVE);
	pthread_mutex_init(&queue->mutex, &mta);
	pthread_mutexattr_destroy(&mta);
	sem_init(&queue->sem, 0, 0);
	return queue;
}

void pQueue_destroy(t_pQueue *queue)
{
	pthread_mutex_lock(&queue->mutex);
	queue_destroy(queue->lib_queue);
	pthread_mutex_unlock(&queue->mutex);
	pthread_mutex_destroy(&queue->mutex);
	sem_destroy(&queue->sem);
	free(queue);
}

int pQueue_size(t_pQueue *queue)
{
	pthread_mutex_lock(&queue->mutex);
	int size = queue_size(queue->lib_queue);
	pthread_mutex_unlock(&queue->mutex);

	return size;
}

void pQueue_put(t_pQueue *queue, void *elem)
{
	pthread_mutex_lock(&queue->mutex);
	queue_push(queue->lib_queue, (void *)elem);
	pthread_mutex_unlock(&queue->mutex);
	sem_post(&queue->sem);
}

void *pQueue_take(t_pQueue *queue)
{
	sem_wait(&queue->sem);
	pthread_mutex_lock(&queue->mutex);
	void *elem = queue_pop(queue->lib_queue);
	pthread_mutex_unlock(&queue->mutex);
	return elem;
}

bool pQueue_is_empty(t_pQueue *queue)
{
	pthread_mutex_lock(&queue->mutex);
	int result = queue_is_empty(queue->lib_queue);
	pthread_mutex_unlock(&queue->mutex);
	return result;
}

void pQueue_iterate(t_pQueue *queue, void (*closure)(void *))
{
	pthread_mutex_lock(&queue->mutex);
	list_iterate(queue->lib_queue->elements, closure);
	pthread_mutex_unlock(&queue->mutex);
}

void pQueue_sort(t_pQueue *queue, bool (*algorithm)(void *, void *))
{
	pthread_mutex_lock(&queue->mutex);
	list_sort(queue->lib_queue->elements, algorithm);
	pthread_mutex_unlock(&queue->mutex);
}

void *pQueue_remove_by_condition(t_pQueue *queue, bool (*condition)(void *))
{
	pthread_mutex_lock(&queue->mutex);
	void *removed_elem = list_remove_by_condition(queue->lib_queue->elements, condition);
	pthread_mutex_unlock(&queue->mutex);
	return removed_elem;
}

void pQueue_lock(t_pQueue *queue)
{
	pthread_mutex_lock(&queue->mutex);
}

void pQueue_unlock(t_pQueue *queue)
{
	pthread_mutex_unlock(&queue->mutex);
}

void *pQueue_peek(t_pQueue *queue)
{
	sem_wait(&queue->sem);
	pthread_mutex_lock(&queue->mutex);
	void *elem = queue_peek(queue->lib_queue);
	pthread_mutex_unlock(&queue->mutex);
	return elem;
}

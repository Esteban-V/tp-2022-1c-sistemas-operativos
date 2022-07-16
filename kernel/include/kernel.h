#ifndef KERNEL_H_
#define KERNEL_H_

#include <stdio.h>
#include <stdlib.h>
#include <commons/log.h>
#include <pthread.h>
#include <commons/collections/list.h>
#include <commons/collections/queue.h>
#include <commons/collections/dictionary.h>
#include <commons/config.h>
#include <readline/readline.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>
#include <assert.h>
#include "networking.h"
#include "serialization.h"
#include "socket_headers.h"
#include "process_utils.h"
#include "pcb_utils.h"
#include "utils.h"

enum e_sortingAlgorithm
{
	FIFO = 0,
	SRT = 1
};

enum e_sortingAlgorithm sortingAlgorithm = FIFO;

struct timespec start_exec_time, last_burst_estimate, now_time;

t_pQueue *newQ, *readyQ, *memoryWaitQ, *blockedQ, *suspended_readyQ, *suspended_blockQ, *exitQ;
pthread_t newToReadyThread, suspendedToReadyThread, readyToExecThread, thread_mediumTerm,
	suspendProcessThread, cpuDispatchThread, memoryThread, io_thread, exitProcessThread;
sem_t sem_multiprogram, new_for_ready, suspended_for_ready, ready_for_exec, longTermSemCall, freeCpu, exec_to_ready, any_blocked, pcb_table_ready, bloquear;
pthread_mutex_t mutex_mediumTerm;
// pthread_mutex_t mutex_cupos;
pthread_cond_t cond_mediumTerm;
int server_socket;
struct timespec now;

int cpu_interrupt_socket;
int cpu_dispatch_socket;
int memory_socket;

int pid = 0;
t_log *logger;

void *header_handler(void *_client_socket);
bool receive_process(t_packet *petition, int console_socket);
bool receive_table_index(t_packet *petition, int mem_socket);

void putToReady(t_pcb *pcb);
void *io_t(void *args);
int getIO(t_pcb *pcb);
void blocked_to_ready(t_pQueue *origin, t_pQueue *destination);

bool exit_op(t_packet *petition, int console_socket);
bool io_op(t_packet *petition, int console_socket);

void *newToReady(void *args);
void *suspendedToReady(void *args);
void *cpu_dispatch_listener(void *args);
void *memory_listener(void *args);
void *suspend_process(void *args);
void *exit_process(void *args);

void terminate_kernel(bool error);

#endif /* KERNEL_H_ */

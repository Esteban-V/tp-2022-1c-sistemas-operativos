#ifndef KERNEL_H_
#define KERNEL_H_
#include<stdio.h>
#include<stdlib.h>
#include<commons/log.h>
#include<pthread.h>
#include<commons/collections/list.h>
#include<commons/collections/queue.h>
#include<commons/collections/dictionary.h>
#include<commons/config.h>
#include<readline/readline.h>
#include<sys/socket.h>
#include<unistd.h>
#include<netdb.h>
#include<assert.h>
#include"networking.h"
#include"serialization.h"
#include"socket_headers.h"
#include"process_utils.h"
#include"pcb_utils.h"
#include"utils.h"

enum e_sortingAlgorithm {
	FIFO = 0, SRT = 1
};
#define BILLION 1E9

enum e_sortingAlgorithm sortingAlgorithm = FIFO;

struct timespec start_exec_time, last_burst_estimate, now_time;

t_pQueue *newQ, *readyQ, *blockedQ, *suspended_readyQ, *suspended_blockQ,
		*exitQ;
pthread_t newToReadyThread, readyToExecThread, thread_mediumTerm,
		suspendProcessThread, cpu_listener, io_thread, exitProcessThread;
sem_t sem_multiprogram, any_for_ready, longTermSemCall, freeCpu, exec_to_ready,
		any_blocked, bloquear;
pthread_mutex_t mutex_mediumTerm, mutex_cupos;
pthread_cond_t cond_mediumTerm;

struct timespec now;

int cpu_interrupt_socket;
int cpu_dispatch_socket;
int memory_server_socket;

char *ip;
char *port;

int pid = 0;
t_log *logger;

void* header_handler(void *_client_socket);
bool receive_process(t_packet *petition, int console_socket);
void putToReady(t_pcb *pcb);
void terminate_kernel();
void* thread_mediumTermFunc();
void* io_t(void *args);
int getIO(t_pcb *pcb);
float toMiliSec(struct timespec time);

#endif /* KERNEL_H_ */

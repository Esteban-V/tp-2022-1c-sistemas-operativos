#ifndef UTILS_H_
#define UTILS_H_

#include<stdio.h>
#include<stdlib.h>

#include<string.h>
#include<commons/log.h>
#include<commons/config.h>
#include<commons/string.h>
#include<commons/collections/list.h>
#include<commons/collections/queue.h>
#include<commons/collections/dictionary.h>

#include<sys/socket.h>
#include<unistd.h>
#include<netdb.h>

#include<assert.h>

#include"p_queue.h"
#include"process_utils.h"
#include"networking.h"
#include"socket_headers.h"
#include"serialization.h"

enum e_states {
	NEW, READY, RUNNING, EXIT, BLOCKED, SUSPENDED_READY, SUSPENDED_BLOCKED
};

enum e_sortingAlgorithm {
	FIFO = 0, SRT = 1
};
#define BILLION 1E9

enum e_sortingAlgorithm sortingAlgorithm;

struct timespec start_exec_time, last_burst_estimate, now_time;

t_pQueue *newQ, *readyQ, *blockedQ, *suspended_readyQ, *suspended_blockQ;
pthread_t thread_longTerm, thread_mediumTerm, thread_mediumTermUnsuspender;
sem_t sem_multiprogram, sem_newProcess, longTermSemCall;
pthread_mutex_t mutex_mediumTerm, mutex_cupos;
pthread_cond_t cond_mediumTerm;

typedef struct pcb {
	int id;
	int size;
	t_list *instructions;
	int program_counter;
	//t_ptbr page_table;
	double burst_estimation;
} t_pcb;

typedef struct kernelConfig {
	t_config *config;
	char *kernelIP;
	char *kernelPort;
	char *memoryIP;
	int memoryPort;
	char *cpuIP;
	int cpuPortDispatch;
	int cpuPortInterrupt;
	int listenPort;
	char *schedulerAlgorithm;
	int initialEstimate;
	char *alpha;
	int multiprogrammingLevel;
	int maxBlockedTime;
} t_kernelConfig;

t_kernelConfig *config;
t_process *process;

t_kernelConfig* getKernelConfig(char *path);
void destroyKernelConfig(t_kernelConfig *kernelConfig);

t_pcb* create_pcb(t_process *process);
void destroy_pcb(t_pcb *pcb);

void* thread_mediumTermUnsuspenderFunc(void *args);

void* thread_mediumTermFunc(void *args); //faltaba declarar
void* thread_longTermFunc();
bool SFJAlg(void *elem1, void *elem2);

void* thread_mediumTermFunc(void *args);
void* thread_longTermFunc();

int cupos_libres;

#endif /* UTILS_H_ */

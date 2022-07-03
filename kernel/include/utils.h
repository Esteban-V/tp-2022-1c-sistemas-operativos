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
#include"queue.h"
#include"pcb_utils.h"
#include"networking.h"
#include"socket_headers.h"
#include"serialization.h"

#define BILLION 1E9

typedef struct kernelConfig {
	t_config *config;
	char *kernelIP;
	char *kernelPort;
	char *memoryIP;
	char *memoryPort;
	char *cpuIP;
	char *cpuPortDispatch;
	char *cpuPortInterrupt;
	char *listenPort;
	char *schedulerAlgorithm;
	uint32_t initialEstimate;
	char *alpha;
	int multiprogrammingLevel;
	int maxBlockedTime;
} t_kernelConfig;

t_kernelConfig *config;
t_process *process;

t_kernelConfig* getKernelConfig(char *path);
void destroyKernelConfig(t_kernelConfig *kernelConfig);

typedef struct pageTableEntry {
	bool present;
	uint32_t frame;
} t_pageTableEntry;

typedef struct t_ptbr {
	int32_t pageQuantity;
	t_pageTableEntry *entries;
} t_ptbr;

void* readyToExec(void *args);
void* thread_longTermFunc();

// int cupos_libres;

float time_to_ms(struct timespec time);
bool SJF_sort(void *elem1, void *elem2);

#endif /* UTILS_H_ */

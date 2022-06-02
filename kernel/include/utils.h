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

#include"kernel.h"

#include"networking.h"
#include"socket_headers.h"
#include"serialization.h"

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

t_kernelConfig* getKernelConfig(char *path);
void destroyKernelConfig(t_kernelConfig *kernelConfig);

void* thread_mediumTermUnsuspenderFunc(void* args);

void* thread_mediumTermFunc(void* args); //faltaba declarar
void* thread_longTermFunc();
bool SFJAlg(void*elem1, void*elem2);


#endif /* UTILS_H_ */

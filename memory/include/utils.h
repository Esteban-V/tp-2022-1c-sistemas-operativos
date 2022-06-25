#ifndef MEMORY_INCLUDE_UTILS_H_
#define MEMORY_INCLUDE_UTILS_H_

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<pthread.h>
#include<commons/log.h>
#include<commons/config.h>
#include<commons/string.h>
#include<commons/collections/list.h>
#include"serialization.h"
#include"networking.h"
#include"socket_headers.h"
#include<commons/config.h>

typedef struct memoryConfig{
	t_config *config;
    int listenPort;
    int memorySize;
    int pageSize;
    int entriesPerTable;
    int memoryDelay;
    char* replaceAlgorithm;
    int framesPerProcess;
    int swapDelay;
    char* swapPath;
} t_memoryConfig;

t_log *logger;
t_memoryConfig* getMemoryConfig(char *path);
void destroyMemoryConfig(t_memoryConfig *memoryConfig);
pthread_mutex_t memoryMut, metadataMut, pageTablesMut;
// Algoritmo (clock-m o LRU) toma un frame de inicio y un frame final y eligen la victima dentro del rango.
uint32_t (*algoritmo)(int32_t start, int32_t end);

// Asignacion fija o global, devuelven en los parametros un rango de frames entre los cuales se puede elegir una victima.
bool (*asignacion)(int32_t *start, int32_t *end, uint32_t PID);

#endif /* MEMORY_INCLUDE_UTILS_H_ */

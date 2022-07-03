#ifndef UTILS_H_
#define UTILS_H_

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
    t_config* config;
    char* listenPort;
    int memorySize;
    int pageSize;
    int entriesPerTable;
    int memoryDelay;
    char* replaceAlgorithm;
    int framesPerProcess;
    int swapDelay;
    char* swapPath;
} t_memoryConfig;

typedef struct t_fr_metadata {
    bool isFree;
    bool modified;
    bool u;         // Para Clock-M
    uint32_t PID;
    uint32_t page;
    uint32_t timeStamp;
} t_fr_metadata;

typedef struct t_mem_metadata{
    uint32_t entryQty;
    uint32_t *firstFrame; // Array de PIDS donde el indice es el numero de "bloque" asignado en asig fija.
    uint32_t *clock_m_counter;
    uint32_t counter;
    t_fr_metadata *entries;
} t_mem_metadata;

typedef struct swapInterface {
    int pageSize;
    int socket;
    pthread_mutex_t mutex;
} t_swapInterface;

typedef struct mem {
    void *memory;
} t_memory;

t_memoryConfig *memoryConfig;
t_log *logger;
t_mem_metadata* metadata;

t_memoryConfig* getMemoryConfig(char *path);
void destroyMemoryConfig(t_memoryConfig *memoryConfig);

pthread_mutex_t memoryMut, metadataMut, pageTablesMut;

// Algoritmo toma un frame de inicio y un frame final y eligen la victima dentro del rango.
uint32_t (*algoritmo)(uint32_t start, uint32_t end);

// Asignacion fija o global, devuelven en los parametros un rango de frames entre los cuales se puede elegir una victima.
bool (*asignacion)(uint32_t *start, uint32_t *end, uint32_t PID);

#endif /* UTILS_H_ */

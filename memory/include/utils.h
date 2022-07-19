#ifndef UTILS_H_
#define UTILS_H_

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <commons/log.h>
#include <commons/config.h>
#include <commons/string.h>
#include <commons/collections/list.h>
#include "serialization.h"
#include "networking.h"
#include "pcb_utils.h"
#include "socket_headers.h"
#include <sys/mman.h>
#include <commons/config.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "semaphore.h"

typedef struct t_memoryConfig
{
    t_config *config;
    char *listenPort;
    // En bytes
    int memorySize;
    // En bytes
    int pageSize;
    // Cantidad de entradas en cada tabla
    int entriesPerTable;
    // En milisegundos
    int memoryDelay;
    // Reemplazo de paginas, CLOCK | CLOCK-M
    char *replaceAlgorithm;
    // Cantidad de marcos por proceso (asig. fija)
    int framesPerProcess;
    // En milisegundos
    int swapDelay;
    // Carpeta de archivos .swap
    char *swapPath;
    // Cantidad de frames en memoria, memorySize/pageSize
    int framesInMemory;
} t_memoryConfig;

typedef struct t_frame_metadata
{
    bool isFree;
    bool modified; // Clock-M
    bool u;        // Clock / Clock-M
    uint32_t PID;
    uint32_t page;
    uint32_t timeStamp;
} t_frame_metadata;

typedef struct t_mem_metadata
{
    uint32_t entryQty;
    uint32_t *firstFrame; // Array de PIDS donde el indice es el numero de "bloque" asignado en asig fija.
    uint32_t *clock_m_counter;
    uint32_t clock_counter;
    t_frame_metadata *entries;
} t_mem_metadata;

typedef struct swapInterface
{
    int pageSize;
    int socket;
    pthread_mutex_t mutex;
} t_swapInterface;

typedef struct mem
{
    void *memory;
} t_memory;

typedef struct pageMetadata
{
    uint32_t pid;
    uint32_t pageNumber;
    bool used;
} t_pageMetadata;

t_memory *memory;
t_memoryConfig *config;
t_log *logger;
t_mem_metadata *metadata;
uint32_t clock_m_counter;
uint32_t clock_counter;

uint32_t getFreeFrame(uint32_t start, uint32_t end);
t_memoryConfig *getMemoryConfig(char *path);
void destroyMemoryConfig(t_memoryConfig *config);
sem_t writeRead;
pthread_mutex_t memoryMut, metadataMut;

// Algoritmo de Reemplazo
uint32_t (*replace_algo)(uint32_t start, uint32_t end);

#endif /* UTILS_H_ */

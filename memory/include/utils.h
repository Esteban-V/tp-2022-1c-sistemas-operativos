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
#include <commons/bitarray.h>
#include "serialization.h"
#include "networking.h"
#include "pcb_utils.h"
#include "socket_headers.h"
#include <sys/mman.h>
#include <commons/config.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "semaphore.h"

enum e_replaceAlgorithm
{
    CLOCK = 0,
    CLOCK_M = 1
};

enum e_replaceAlgorithm replaceAlgorithm = CLOCK;

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

typedef struct t_frame_entry
{
    // A que frame (index) de la memoria (de void*) corresponde
    int frame;
    // Lista de t_page_entry
    t_page_entry *page_data;
} t_frame_entry;

typedef struct t_process_frame
{
    // Lista de t_frame_entry
    t_list *frames;
    // A que index de frames apunta
    int clock_hand;
} t_process_frame;

// Lista de t_process_frames
t_list *process_frames;

typedef struct mem
{
    void *memory;
} t_memory;

t_bitarray *frames_bitmap;

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

int ceil_div(int a, int b);

int find_first_unassigned_frame(t_bitarray *frames_bitmap);

bool has_free_frame(t_process_frame *process_frames);
int find_first_free_frame(t_process_frame *process_frames);

int frame_set_assigned(t_bitarray *frames_bitmap, int index);
int frame_clear_assigned(t_bitarray *frames_bitmap, int index);

void increment_clock_hand(int *clock_hand);

void sync_bitmap(t_bitarray *bitmap);

#endif /* UTILS_H_ */

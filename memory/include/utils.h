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

typedef struct t_page_entry
{
    // A que frame (index) de la memoria (de void*) corresponde (y con presencia en false estaria desactualizado)
    int frame;
    // Bit de presencia
    bool present;
    // Bit de uso
    bool used;
    // Bit de modificado
    bool modified;
    // Que pagina (index) en su tabla de paginas nivel 2 es
    int page;
} t_page_entry;

typedef struct t_ptbr2
{
    // Lista de t_page_entry
    t_list *entries;
} t_ptbr2;

typedef struct t_ptbr1
{
    // Lista de int de level2_tables
    t_list *entries;
} t_ptbr1;

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
t_list *processes_frames;

typedef struct mem
{
    void *memory;
} t_memory;

t_bitarray *frames_bitmap;

t_memory *memory;
t_memoryConfig *config;
t_log *logger;

sem_t writeRead;
pthread_mutex_t memoryMut, metadataMut;

uint32_t clock_m_counter;
uint32_t clock_counter;

t_memoryConfig *getMemoryConfig(char *path);
void destroyMemoryConfig(t_memoryConfig *config);

int ceil_div(int a, int b);

int get_frame_number(int pt2_index, int entry_index, int pid, int frames_index);
void *get_frame(uint32_t frame_number);
uint32_t read_frame_value(void *frame_ptr, uint32_t offset);
void *get_frame_value(void *frame_ptr);

int find_first_unassigned_frame(t_bitarray *frames_bitmap);

bool has_free_frame(t_process_frame *process_frames);
t_frame_entry * find_first_free_frame(t_process_frame *process_frames);

void frame_set_assigned(t_bitarray *frames_bitmap, int index);
void frame_clear_assigned(t_bitarray *frames_bitmap, int index);
void write_frame_value(void *frame_ptr, void *value);
void increment_clock_hand(int *clock_hand);

void sync_bitmap(t_bitarray *bitmap);

#endif /* UTILS_H_ */

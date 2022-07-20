#ifndef INCLUDE_CPU_H_
#define INCLUDE_CPU_H_

#include <stdio.h>
#include <stdlib.h>
#include <commons/log.h>
#include <pthread.h>
#include <commons/collections/list.h>
#include <commons/collections/queue.h>
#include <commons/collections/dictionary.h>
#include <commons/config.h>
#include <readline/readline.h>
#include <commons/collections/queue.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>
#include <assert.h>
#include "utils.h"
#include "networking.h"
#include "serialization.h"
#include "socket_headers.h"
#include "process_utils.h"
#include "semaphore.h"

#define CPU_MEMORY_SECRET = "CMSMC"

typedef struct tlbEntry {
    uint32_t page;
    int32_t frame;
    bool isFree;
} t_tlbEntry;

typedef struct tlb {
    t_tlbEntry* entries;
    uint32_t entryQty;
    t_list* victimQueue;
} t_tlb;

sem_t interruption_counter;

t_pcb *pcb;
t_cpu_config *config;
t_tlb * tlb;

bool receive_pcb(t_packet *petition, int console_socket);
bool receive_interruption(t_packet *petition, int console_socket);

void *header_handler(void *_kernel_client_socket);
void *cpu_cycle();
enum operation fetch_and_decode(t_instruction **instruction);
void *listen_interruption();
void execute_no_op();
void execute_io(t_list *params);
void execute_read(t_list *params);
void execute_copy(t_list *params);
void execute_write(t_list *params);
void execute_exit();
void pcb_to_kernel(kernel_headers header);

pthread_mutex_t mutex_kernel_socket, mutex_has_interruption, tlb_mutex;
bool new_interruption;
int kernel_client_socket;
int memory_server_socket;
sem_t pcb_loaded;
t_log *logger;
int kernel_dispatch_socket;
int kernel_interrupt_socket;

void stats();
pthread_t interruptionThread, execThread;
void terminate_cpu(bool error);
void memory_handshake();

// TLB

int tlb_hit_counter = 0;
int tlb_miss_counter = 0;
t_tlb* create_tlb();
void lru_tlb(t_tlbEntry* entry);
void fifo_tlb(t_tlbEntry* entry);
void (*update_victim_queue)(t_tlbEntry*);
void destroy_tlb();
void clean_tlb();

#endif /* INCLUDE_CPU_H_ */

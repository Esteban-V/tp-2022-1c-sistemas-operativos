#ifndef INCLUDE_CPU_H_
#define INCLUDE_CPU_H_

#include <stdio.h>
#include <stdlib.h>

#include <commons/collections/list.h>
#include <commons/collections/queue.h>
#include <commons/collections/dictionary.h>
#include <commons/config.h>
#include <commons/log.h>
#include <commons/collections/queue.h>

#include <pthread.h>

#include <readline/readline.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>
#include <assert.h>
#include <math.h>
#include <semaphore.h>

#include "networking.h"
#include "serialization.h"
#include "tlb.h"

t_pcb *pcb;
t_log *logger;

void memory_handshake();

void *listen_interruption();
void *get_read_value();
void *dispatch_header_handler(void *_kernel_client_socket);
void *header_handler(void *_kernel_client_socket);
void *packet_handler(void *_kernel_client_socket);

bool receive_pcb(t_packet *petition, int kernel_socket);
bool receive_interruption(t_packet *petition, int kernel_socket);
bool receive_value(t_packet *petition, int mem_socket);

void *cpu_cycle();
enum operation fetch_and_decode(t_instruction **instruction);
void check_interrupt();

void execute_no_op();
void execute_io(t_list *params);
void execute_read(t_list *params);
void execute_copy(t_list *params);
void execute_write(t_list *params);
void execute_exit();

void pcb_to_kernel(kernel_headers header);

uint32_t get_frame(uint32_t page_number);
void memory_op(enum memory_headers header, uint32_t frame, uint32_t offset, uint32_t value);

pthread_t interruptionThread, execThread, memoryThread;
pthread_mutex_t mutex_kernel_socket, mutex_has_interruption, mutex_value, mutex_pcb;

sem_t pcb_loaded, value_loaded;
bool new_interruption;
uint32_t read_value;

int kernel_client_socket;
int memory_server_socket;
int kernel_dispatch_socket;
int kernel_interrupt_socket;

void stats();
void terminate_cpu(int x);

#endif /* INCLUDE_CPU_H_ */

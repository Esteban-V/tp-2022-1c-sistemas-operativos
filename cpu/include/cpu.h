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
#include "socket_headers.h"
#include "tlb.h"

t_pcb *pcb;
t_log *logger;

void memory_handshake();
void *packet_handler(void *_client_socket);
void *listen_interruption();
void *dispatch_header_handler(void *_kernel_client_socket);
void *header_handler(void *_kernel_client_socket);

bool receive_pcb(t_packet *petition, int console_socket);
bool receive_interruption(t_packet *petition, int console_socket);

void *cpu_cycle();
enum operation fetch_and_decode(t_instruction **instruction);

void execute_no_op();
void execute_io(t_list *params);
void execute_read(t_list *params);
void execute_copy(t_list *params);
void execute_write(t_list *params);
void execute_exit();

void pcb_to_kernel(kernel_headers header);

pthread_t interruptionThread, execThread;
pthread_mutex_t mutex_kernel_socket, mutex_has_interruption;

sem_t pcb_loaded,cpu_bussy;
bool new_interruption;

int kernel_client_socket;
int memory_server_socket;
int kernel_dispatch_socket;
int kernel_interrupt_socket;

bool bussyCpu();

void stats();
void terminate_cpu(int x);

#endif /* INCLUDE_CPU_H_ */

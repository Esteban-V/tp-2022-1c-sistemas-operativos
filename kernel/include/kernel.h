#ifndef KERNEL_H_
#define KERNEL_H_

#include <stdio.h>
#include <stdlib.h>
#include <commons/log.h>
#include <pthread.h>
#include <commons/collections/list.h>
#include <commons/collections/queue.h>
#include <commons/collections/dictionary.h>
#include <commons/config.h>
#include <readline/readline.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>
#include <assert.h>
#include "networking.h"
#include "serialization.h"
#include "socket_headers.h"
#include "process_utils.h"
#include "pcb_utils.h"
#include "utils.h"

enum e_sortingAlgorithm
{
	FIFO = 0,
	SRT = 1
};

enum e_sortingAlgorithm sortingAlgorithm = FIFO; // TODO. Ta bien esto?

int pid = 0;
t_log *logger;

struct timespec now, toExec, fromExec;

t_pQueue *new_q, *ready_q, *memory_init_q, *memory_exit_q, *blocked_q, *suspended_ready_q, *suspended_block_q, *exit_q;

pthread_t any_to_ready_t, ready_to_exec_t,
	cpu_dispatch_t, memory_t, io_t, exit_process_t;

sem_t sem_multiprogram, interrupt_ready, any_for_ready, process_for_IO, ready_for_exec, cpu_free, pcb_table_ready;

int server_socket;
int cpu_interrupt_socket;
int cpu_dispatch_socket;
int memory_socket;

void *header_handler(void *_client_socket);
bool receive_process(t_packet *petition, int console_socket);
bool table_index_success(t_packet *petition, int mem_socket);
bool exit_process_success(t_packet *petition, int mem_socket);

bool exit_op(t_packet *petition, int console_socket);
bool io_op(t_packet *petition, int console_socket);

void *new_to_ready();
void *suspended_to_ready();
void blocked_to_ready(t_pQueue *origin, t_pQueue *destination);
void put_to_ready(t_pcb *pcb);

void *to_exec();
void *to_ready();

void *cpu_dispatch_listener(void *args);
void *memory_listener(void *args);
void *io_listener();

bool handle_interruption(t_packet *petition, int cpu_socket);

void *suspend_process(void *args);
void *exit_process(void *args);

void terminate_kernel(bool error);

#endif /* KERNEL_H_ */

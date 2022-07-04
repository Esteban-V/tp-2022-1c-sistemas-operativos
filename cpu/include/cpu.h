/*
 * cpu.h
 *
 *  Created on: 15 jun. 2022
 *      Author: utnso
 */

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

t_pcb *pcb;
t_cpu_config *config;

bool receivedPcb(t_packet *petition, int console_socket);
bool receivedInterruption(t_packet *petition, int console_socket);

void *header_handler(void *_kernel_client_socket);
void *cpu_cycle();
enum operation fetch_and_decode(t_instruction **instruction);

void *interruption();

void execute_no_op();
void execute_io(t_instruction *instruction);
void execute_read(t_instruction *instruction);
void execute_copy(t_instruction *instruction);
void execute_write(t_instruction *instruction);
void execute_exit();

void pcb_to_kernel(kernel_headers header);

pthread_mutex_t mutex_kernel_socket;
int kernel_client_socket;

sem_t pcb_loaded;
t_log *logger;

int kernel_dispatch_socket;
int kernel_interrupt_socket;

pthread_t interruptionThread, execThread;

#endif /* INCLUDE_CPU_H_ */

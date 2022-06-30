/*
 * cpu.h
 *
 *  Created on: 15 jun. 2022
 *      Author: utnso
 */

#ifndef INCLUDE_CPU_H_
#define INCLUDE_CPU_H_

#include<stdio.h>
#include<stdlib.h>
#include<commons/log.h>
#include<pthread.h>
#include<commons/collections/list.h>
#include<commons/collections/queue.h>
#include<commons/collections/dictionary.h>
#include<commons/config.h>
#include<readline/readline.h>
#include<sys/socket.h>
#include<unistd.h>
#include<netdb.h>
#include<assert.h>
#include <utils.h>
#include"networking.h"
#include"serialization.h"
#include"socket_headers.h"
#include"process_utils.h"
#include"pcb_utils.h"
#include "semaphore.h"

enum operation {
	NO_OP, IO_OP, READ, COPY, WRITE, EXIT_OP, DEAD
};

enum operation getOperation(char*);

t_pcb *pcb;
t_cpu_config *config;

bool receivedPcb(t_packet *petition, int console_socket);
bool receivedInterruption(t_packet *petition, int console_socket);

void* header_handler(void *_client_socket);
void* execution();
void* interruption();

void execute_exit();
void execute_no_op(uint32_t time);
void execute_io(uint32_t time);

void sendPcbToKernel(headers header);

sem_t pcb_loaded;

int kernel_dispatch_socket;
int kernel_interrupt_socket;

pthread_t interruptionThread, execThread;

#endif /* INCLUDE_CPU_H_ */

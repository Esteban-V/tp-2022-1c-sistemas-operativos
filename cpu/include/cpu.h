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
#include"networking.h"
#include"serialization.h"
#include"socket_headers.h"
#include"process_utils.h"
#include"pcb.h"
#include "cpuConfig.h"
#include "semaphore.h"


enum operation{NO_OPOP,IOOP,READOP,COPYOP,WRITEOP,EXITOP,DEAD};

enum operation getOperation(char*);
t_pcb *pcb;
t_cpuConfig *config;
void* interruption();
bool receivedPcb(t_packet *petition, int console_socket);
bool receivedInterruption(t_packet *petition, int console_socket);
void* header_handler(void *_client_socket);
void* execution();
sem_t pcb_loaded;
int server_dispatch_socket;
int server_interrupt_socket;

pthread_t interruptionThread, execThread;

#endif /* INCLUDE_CPU_H_ */

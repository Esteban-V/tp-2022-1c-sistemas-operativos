#ifndef KERNEL_H_
#define KERNEL_H_

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

#include"utils.h"

char *ip;
char *port;

t_log *logger;

enum states {
	NEW, READY, EXECUTE, EXIT, BLOCKED, SUSPENDED_READY, SUSPENDED_BLOCK
};

t_queue *newQ;
t_queue *readyQ;
t_queue *executeQ;
t_queue *exitQ;
t_queue *blockedQ;
t_queue *suspended_readyQ;
t_queue *suspended_blockQ;

typedef struct pcb {
	int id;
	int size;
	t_list *instructions;
	int program_counter;
	//t_ptbr page_table;
	int burst_estimation;
} t_pcb;

t_kernelConfig *config;

t_log* create_logger();
t_config* create_config();

void* header_handler(void *_client_socket);
bool receive_process(t_packet *petition, int console_socket);

void stream_take_process(t_packet *packet, t_process *process);
void stream_take_instruction(t_stream_buffer *stream, void **elem);

void terminate_kernel();

#endif /* KERNEL_H_ */
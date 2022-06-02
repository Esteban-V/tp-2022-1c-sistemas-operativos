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
#include"p_queue.h"

#include"utils.h"

char *ip;
char *port;

t_log *logger;

enum e_states {
	NEW, READY, RUNNING, EXIT, BLOCKED, SUSPENDED_READY, SUSPENDED_BLOCKED
};

enum e_sortingAlgorithm {
	FIFO=0, SRT=1
};
#define BILLION 1E9

enum e_sortingAlgorithm sortingAlgorithm;

t_pQueue *newQ,*readyQ,*blockedQ,*suspended_readyQ,*suspended_blockQ;
pthread_t thread_longTerm, thread_mediumTerm, thread_mediumTermUnsuspender;
sem_t sem_multiprogram, sem_newProcess, longTermSemCall;
pthread_mutex_t mutex_mediumTerm, mutex_cupos;
pthread_cond_t cond_mediumTerm;

typedef struct pcb {
	int id;
	int size;
	t_list *instructions;
	int program_counter;
	//t_ptbr page_table;
	double burst_estimation;
} t_pcb;


t_kernelConfig *config;
t_process *process;
t_pcb* create_pcb(t_process *process);
void destroy_pcb(t_pcb *pcb);


t_kernelConfig *config;
int pid = 0;

struct timespec start_exec_time, last_burst_estimate, now_time;
t_log *logger;
t_config* create_config();

void* header_handler(void *_client_socket);
bool receive_process(t_packet *petition, int console_socket);

void stream_take_process(t_packet *packet, t_process *process);
void stream_take_instruction(t_stream_buffer *stream, t_instruction **elem);

void* thread_mediumTermFunc(void *args);
void* thread_longTermFunc();

void terminate_kernel();






#endif /* KERNEL_H_ */

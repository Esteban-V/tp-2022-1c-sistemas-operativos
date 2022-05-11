#ifndef INCLUDE_PROCESS_H_
#define INCLUDE_PROCESS_H_

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

typedef struct instruction {
	char *id;
	t_list *params;
} t_instruction;

typedef struct process {
	uint32_t size;
	t_list *instructions;
} t_process;

t_process* create_process(uint32_t size);

#endif /* INCLUDE_PROCESS_H_ */
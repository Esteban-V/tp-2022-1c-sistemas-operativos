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

char* ip;
char* port;

t_config* config;

typedef enum
{
	MESSAGE,
	PACKAGE
} op_code;

void iterator(char* value);

t_log* create_logger();
t_config* create_config();
void receive_client(void* thread_args);

void terminate_kernel();

#endif /* KERNEL_H_ */

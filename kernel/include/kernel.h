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

#include"socket_headers.h"

char* ip;
char* port;

t_config* config;

t_log* create_logger();
t_config* create_config();

void* op_code_handler(void *_client_socket);
bool receive_process(t_packet *petition, int console_socket);

void terminate_kernel();

#endif /* KERNEL_H_ */

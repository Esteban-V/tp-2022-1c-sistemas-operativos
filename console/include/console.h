#ifndef CONSOLE_H_
#define CONSOLE_H_

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<commons/log.h>
#include<commons/config.h>
#include<pthread.h>
#include<commons/string.h>
#include<commons/collections/list.h>
#include<pthread.h>
#include"serialization.h"
#include"networking.h"
#include"socket_headers.h"
#include"utils.h"
#include"process_utils.h"

t_list *instruction_list;
t_process *process;

t_config *config;
t_log *logger;
int server_socket;

char *ip;
char *port;

void get_code(FILE *file);
t_instruction* parse_instruction(char *string);

void stream_add_process(t_packet *packet);
void stream_add_instruction(t_stream_buffer *stream, void *elem);

void terminate_console();

// void serializacion_process(t_process* procces);

#endif /* CONSOLE_H_ */

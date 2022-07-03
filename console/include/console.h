#ifndef CONSOLE_H_
#define CONSOLE_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <commons/log.h>
#include <commons/config.h>
#include <pthread.h>
#include <commons/string.h>
#include <commons/collections/list.h>
#include <pthread.h>
#include "serialization.h"
#include "networking.h"
#include "socket_headers.h"
#include "utils.h"
#include "process_utils.h"

t_list *instruction_list;
t_process *process;

t_config *config;
t_log *logger;
int kernel_socket;

char *kernel_ip;
char *kernel_port;

void get_code(FILE *file);
void error_opening_file();
t_instruction *parse_instruction(char *string);

void terminate_console(bool error);

#endif /* CONSOLE_H_ */

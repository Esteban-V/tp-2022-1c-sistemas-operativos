/*
 * utils.h
 *
 *  Created on: May 9, 2022
 *      Author: utn-so
 */

#ifndef INCLUDE_UTILS_H_
#define INCLUDE_UTILS_H_

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<stdint.h>

#include<commons/config.h>
#include<commons/log.h>

#include<commons/string.h>
#include<commons/collections/list.h>

t_log *new_logger;
t_config *new_config;

typedef struct instruction {
	char *id;
	t_list *params;
} t_instruction;

typedef struct process {
	uint32_t size;
	t_list *instructions;
} t_process;

void log_instruction_params(int param);
void log_instruction(t_instruction *inst);

FILE* open_file(char *path);

t_instruction* create_instruction(size_t id_size);
void instruction_destroy(t_instruction *instruction);

t_process* create_process(uint32_t size);
void process_destroy(t_process *process);

t_log* create_logger();
t_config* create_config();

#endif /* INCLUDE_UTILS_H_ */

#ifndef INCLUDE_PROCESS_UTILS_H_
#define INCLUDE_PROCESS_UTILS_H_

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<stdint.h>

#include<commons/log.h>

#include<commons/string.h>
#include<commons/collections/list.h>

typedef struct instruction {
	char *id;
	t_list *params;
} t_instruction;

typedef struct process {
	uint32_t size;
	t_list *instructions;
} t_process;

void log_instruction_param(t_log* logger, int param);
void log_instruction(t_log* logger, t_instruction *inst);

t_instruction* create_instruction(size_t id_size);
void instruction_destroy(t_instruction *instruction);

t_process* create_process();
void process_destroy(t_process *process);

#endif /* INCLUDE_PROCESS_UTILS_H_ */

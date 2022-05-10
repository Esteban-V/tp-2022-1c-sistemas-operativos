/*
 * utils.c
 *
 *  Created on: May 9, 2022
 *      Author: utn-so
 */

#include"utils.h"

void log_instruction_params(int param) {
	log_info(new_logger, " %d", param);
}

void log_instruction(t_instruction *inst) {
	log_info(new_logger, inst->id);
	list_iterate(inst->params, (void*) log_instruction_params);
}

FILE* open_file(char *path) {
	FILE *file = fopen(path, "r");
	if (file == NULL) {
		exit(-1);
	}
	return file;
}

t_log* create_logger() {
	new_logger = log_create("console.log", "CONSOLE", 1, LOG_LEVEL_INFO);
	return new_logger;
}

t_config* create_config() {
	new_config = config_create("console.config");
	return new_config;
}

t_instruction* create_instruction(size_t id_size) {
	// Try to allocate instruction structure.
	t_instruction *instruction = malloc(sizeof(t_instruction));
	if (instruction == NULL)
		return NULL;

	// Try to allocate instruction id and params, free structure if fail.
	instruction->id = malloc(id_size * sizeof(char));
	if (instruction->id == NULL) {
		free(instruction);
		return NULL;
	}

	instruction->params = list_create();
	if (instruction->params == NULL) {
		free(instruction->id);
		free(instruction);
		return NULL;
	}

	return instruction;
}

void instruction_destroy(t_instruction *instruction) {
	if (instruction != NULL) {
		free(instruction->id);
		free(instruction->params);
		free(instruction);
	}
}

t_process* create_process(uint32_t size) {
	// Try to allocate process structure.
	t_process *process = malloc(sizeof(t_process));
	if (process == NULL) {
		return NULL;
	}

	// Try to allocate process size and instructions, free structure if fail.
	process->size = malloc(sizeof(uint32_t));
	if (process->size == NULL) {
		free(process);
		return NULL;
	}
	process->size = size;

	process->instructions = list_create();
	if (process->instructions == NULL) {
		free(process->size);
		free(process);
		return NULL;
	}

	if (process->size == NULL) {
		free(process);
		return NULL;
	}

	return process;
}

void destroy_instruction_iteratee(t_instruction *elem) {
	instruction_destroy(elem);
}

void process_destroy(t_process *process) {
	if (process != NULL) {
		free(process->size);
		list_iterate(process->instructions,
				(void*) destroy_instruction_iteratee);
		free(process->instructions);
	}
}

/*
 * process_utils.c
 *
 *  Created on: May 11, 2022
 *      Author: utn-so
 */

#include"process_utils.h"

void log_instruction_param(t_log *logger, uint32_t *param) {
	log_info(logger, " %d", *param);
}

void log_instruction(t_log *logger, t_instruction *inst) {
	void _log_instruction_param(void *elem) {
		log_instruction_param(logger, elem);
	}

	log_info(logger, inst->id);
	list_iterate(inst->params, _log_instruction_param);
}

void log_process(t_log *logger, t_process *proc) {
	void _log_instruction(void *elem) {
		log_instruction(logger, elem);
	}

	log_info(logger, "Size: %d\n", proc->size);
	log_info(logger, "Instructions:\n");

	list_iterate(proc->instructions, _log_instruction);
}

t_instruction* create_instruction(size_t id_size) {
	// Try to allocate instruction structure.
	t_instruction *instruction = malloc(sizeof(t_instruction));
	if (instruction == NULL)
		return NULL;

	// Try to allocate instruction id and params, free structure if fail.
	instruction->id = malloc((id_size + 1) * sizeof(char));
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

t_process* create_process() {
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


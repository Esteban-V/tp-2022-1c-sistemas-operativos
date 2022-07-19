#include "process_utils.h"

void log_instruction_param(t_log *logger, uint32_t *param)
{
	pthread_mutex_lock(&mutex_log);
	log_info(logger, " %d", *param);
	pthread_mutex_unlock(&mutex_log);
}

void log_instruction(t_log *logger, t_instruction *inst)
{
	void _log_instruction_param(void *elem)
	{
		log_instruction_param(logger, elem);
	}

	pthread_mutex_lock(&mutex_log);
	log_info(logger, inst->id);
	pthread_mutex_unlock(&mutex_log);

	list_iterate(inst->params, _log_instruction_param);
}

void log_process(t_log *logger, t_process *proc)
{
	void _log_instruction(void *elem)
	{
		log_instruction(logger, elem);
	}

	pthread_mutex_lock(&mutex_log);
	log_info(logger, "Size: %d\n", proc->size);
	log_info(logger, "Instructions:\n");
	pthread_mutex_unlock(&mutex_log);
}

t_instruction *create_instruction(size_t id_size)
{
	// Try to allocate instruction structure.
	t_instruction *instruction = malloc(sizeof(t_instruction));
	if (instruction == NULL)
		return NULL;

	// Try to allocate instruction id and params, free structure if fail.
	instruction->id = malloc((id_size + 1) * sizeof(char));
	if (instruction->id == NULL)
	{
		free(instruction);
		return NULL;
	}

	instruction->params = list_create();
	if (instruction->params == NULL)
	{
		free(instruction->id);
		free(instruction);
		return NULL;
	}

	return instruction;
}

void instruction_destroy(t_instruction *instruction)
{
	if (instruction != NULL)
	{
		free(instruction->id);
		free(instruction->params);
		free(instruction);
	}
}

t_process *create_process()
{
	// Try to allocate process structure.
	t_process *process = malloc(sizeof(t_process));
	if (process == NULL)
	{
		return NULL;
	}

	// Try to allocate process size and instructions, free structure if fail.
	process->size = malloc(sizeof(uint32_t));
	if (process->size == NULL)
	{
		free(process);
		return NULL;
	}

	process->instructions = list_create();
	if (process->instructions == NULL)
	{
		free(process->size);
		free(process);
		return NULL;
	}

	return process;
}

void process_destroy(t_process *process)
{
	if (process != NULL)
	{
		// Tira Warning
		// free(process->size);
		list_iterate(process->instructions,
					 (t_instruction *)instruction_destroy);
		list_destroy(process->instructions);
	}
}

void stream_add_process(t_packet *packet, t_process *process)
{
	stream_add_UINT32(packet->payload, process->size);
	stream_add_LIST(packet->payload, process->instructions,
					stream_add_instruction);
}

void stream_take_process(t_packet *packet, t_process *process)
{
	uint32_t *size = &(process->size);
	stream_take_UINT32P(packet->payload, &size);

	t_list *instructions = stream_take_LIST(packet->payload,
											stream_take_instruction);
	memcpy(process->instructions, instructions, sizeof(t_list));
}

void stream_take_instruction(t_stream_buffer *stream, t_instruction **elem)
{
	char *id = stream_take_STRING(stream);
	if ((*elem) == NULL)
	{
		*elem = create_instruction(string_length(id));
	}

	memcpy((*elem)->id, id, string_length(id) + 1);

	t_list *params = stream_take_LIST(stream, stream_take_UINT32P);
	memcpy((*elem)->params, params, sizeof(t_list));
}

void stream_add_instruction(t_stream_buffer *stream, void *elem)
{
	t_instruction *instruction = (t_instruction *)elem;
	stream_add_STRING(stream, instruction->id);
	stream_add_LIST(stream, instruction->params, stream_add_UINT32P);
}

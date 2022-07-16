#include "pcb_utils.h"

void pcb_destroy(t_pcb *pcb)
{
	if (pcb != NULL)
	{
		list_destroy(pcb->instructions);
		free(pcb->instructions);
		free(pcb);
	}
	pcb = NULL;
}

t_pcb *create_pcb()
{
	// Try to allocate pcb structure.
	t_pcb *pcb = malloc(sizeof(t_pcb));
	if (pcb == NULL)
	{
		return NULL;
	}

	// Try to allocate pcb properties and instructions, free structure if fail.
	pcb->pid = malloc(sizeof(uint32_t));
	if (pcb->pid == NULL)
	{
		free(pcb);
		return NULL;
	}

	pcb->size = malloc(sizeof(uint32_t));
	if (pcb->size == NULL)
	{
		free(pcb->pid);
		free(pcb);
		return NULL;
	}

	pcb->client_socket = malloc(sizeof(uint32_t));
	if (pcb->client_socket == NULL)
	{
		free(pcb->pid);
		free(pcb->size);
		free(pcb);
		return NULL;
	}

	pcb->instructions = list_create();
	if (pcb->instructions == NULL)
	{
		free(pcb->pid);
		free(pcb->size);
		free(pcb->client_socket);
		free(pcb);
		return NULL;
	}

	pcb->program_counter = malloc(sizeof(uint32_t));
	if (pcb->program_counter == NULL)
	{
		free(pcb->pid);
		free(pcb->size);
		free(pcb->client_socket);
		free(pcb->instructions);
		free(pcb);
		return NULL;
	}

	pcb->burst_estimation = malloc(sizeof(uint32_t));
	if (pcb->burst_estimation == NULL)
	{
		free(pcb->pid);
		free(pcb->size);
		free(pcb->client_socket);
		free(pcb->instructions);
		free(pcb->program_counter);
		free(pcb);
		return NULL;
	}

	pcb->blocked_time = malloc(sizeof(uint32_t));
	if (pcb->blocked_time == NULL)
	{
		free(pcb->pid);
		free(pcb->size);
		free(pcb->client_socket);
		free(pcb->instructions);
		free(pcb->program_counter);
		free(pcb->burst_estimation);
		free(pcb);
		return NULL;
	}

	pcb->pending_io_time = malloc(sizeof(uint32_t));
	if (pcb->pending_io_time == NULL)
	{
		free(pcb->pid);
		free(pcb->size);
		free(pcb->client_socket);
		free(pcb->instructions);
		free(pcb->program_counter);
		free(pcb->burst_estimation);
		free(pcb->blocked_time);
		free(pcb);
		return NULL;
	}

	return pcb;
}

void stream_take_pcb(t_packet *packet, t_pcb *pcb)
{
	uint32_t *pid = &(pcb->pid);
	stream_take_UINT32P(packet->payload, &pid);

	uint32_t *size = &(pcb->size);
	stream_take_UINT32P(packet->payload, &size);

	uint32_t *client_socket = &(pcb->client_socket);
	stream_take_UINT32P(packet->payload, &client_socket);

	t_list *instructions = stream_take_LIST(packet->payload,
											stream_take_instruction);
	memcpy(pcb->instructions, instructions, sizeof(t_list));

	uint32_t *program_counter = &(pcb->program_counter);
	stream_take_UINT32P(packet->payload, &program_counter);

	// paginas

	uint32_t *burst_estimation = &(pcb->burst_estimation);
	stream_take_UINT32P(packet->payload, &burst_estimation);
}

void stream_add_pcb(t_packet *packet, t_pcb *pcb)
{
	stream_add_UINT32(packet->payload, pcb->pid);
	stream_add_UINT32(packet->payload, pcb->size);
	stream_add_UINT32(packet->payload, pcb->client_socket);

	// ESTO ROMPERIA PORQUE LAS INSTRUCTIONS SE COPIAN MAL
	stream_add_LIST(packet->payload, pcb->instructions, stream_add_instruction);

	stream_add_UINT32(packet->payload, pcb->program_counter);
	// paginas
	stream_add_UINT32(packet->payload, pcb->burst_estimation);
}

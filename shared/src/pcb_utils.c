#include "pcb_utils.h"

void pcb_destroy(t_pcb *pcb)
{
	void _delete_instrucion(void *elem)
	{
		instruction_destroy(elem);
	}

	if (pcb != NULL)
	{
		list_destroy_and_destroy_elements(pcb->instructions, _delete_instrucion);
		free(pcb);
	}
}

t_pcb *create_pcb()
{
	// Try to allocate pcb structure.
	t_pcb *pcb = malloc(sizeof(t_pcb));
	if (pcb == NULL)
	{
		return NULL;
	}

	pcb->instructions = list_create();

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

	t_list *instructions = stream_take_LIST(packet->payload, stream_take_instruction);
	memcpy(pcb->instructions, instructions, sizeof(t_list));

	uint32_t *program_counter = &(pcb->program_counter);
	stream_take_UINT32P(packet->payload, &program_counter);

	uint32_t *page_table = &(pcb->page_table);
	stream_take_UINT32P(packet->payload, &page_table);

	uint32_t *assigned_frames = &(pcb->process_frames_index);
	stream_take_UINT32P(packet->payload, &assigned_frames);

	uint32_t *burst_estimation = &(pcb->burst_estimation);
	stream_take_UINT32P(packet->payload, &burst_estimation);

	uint32_t *left_burst_estimation = &(pcb->left_burst_estimation);
	stream_take_UINT32P(packet->payload, &left_burst_estimation);

	uint32_t *blocked_time = &(pcb->blocked_time);
	stream_take_UINT32P(packet->payload, &blocked_time);

	uint32_t *pending_io_time = &(pcb->pending_io_time);
	stream_take_UINT32P(packet->payload, &pending_io_time);
}

void stream_add_pcb(t_packet *packet, t_pcb *pcb)
{
	stream_add_UINT32(packet->payload, pcb->pid);
	stream_add_UINT32(packet->payload, pcb->size);
	stream_add_UINT32(packet->payload, pcb->client_socket);
	stream_add_LIST(packet->payload, pcb->instructions, stream_add_instruction);
	stream_add_UINT32(packet->payload, pcb->program_counter);
	stream_add_UINT32(packet->payload, pcb->page_table);
	stream_add_UINT32(packet->payload, pcb->process_frames_index);
	stream_add_UINT32(packet->payload, pcb->burst_estimation);
	stream_add_UINT32(packet->payload, pcb->left_burst_estimation);
	stream_add_UINT32(packet->payload, pcb->blocked_time);
	stream_add_UINT32(packet->payload, pcb->pending_io_time);
}

#include"pcb.h"

/*
 * pcb.c
 *
 *  Created on: 3 jun. 2022
 *      Author: utnso
 */



void destroy_pcb(t_pcb *pcb) {
	if (pcb != NULL) {
		free(pcb->id);
		free(pcb->size);
		free(pcb->program_counter);
		free(pcb->burst_estimation);
		list_destroy(pcb->instructions);

		free(pcb->instructions);
		free(pcb);
	}
}

t_pcb* create_pcb() {
	t_pcb *pcb = malloc(sizeof(t_pcb));
	pcb->instructions = list_create();
	pcb->id = malloc(sizeof(int));
	pcb->size = malloc(sizeof(int));
	pcb->program_counter = malloc(sizeof(int));
	pcb->burst_estimation = malloc(sizeof(int));

	return pcb;
}

void stream_take_pcb(t_packet *packet, t_pcb *pcb) {
	uint32_t *id = &(pcb->id);
	stream_take_UINT32P(packet->payload, &id);

	uint32_t *size = &(pcb->size);
	stream_take_UINT32P(packet->payload, &size);

	t_list *instructions = stream_take_LIST(packet->payload, stream_take_instruction);
	memcpy(pcb->instructions, instructions, sizeof(t_list));

	uint32_t *program_counter = &(pcb->program_counter);
	stream_take_UINT32P(packet->payload, &program_counter);

	//paginas

	uint32_t *burst_estimation = &(pcb->burst_estimation);
	stream_take_UINT32P(packet->payload, &burst_estimation);

}

void stream_add_process(t_packet *packet,t_pcb *pcb) {
	stream_add_UINT32(packet->payload, pcb->id);
	stream_add_UINT32(packet->payload, pcb->size);
	stream_add_LIST(packet->payload, pcb->instructions, stream_add_instruction);
	stream_add_UINT32(packet->payload, pcb->program_counter);
	stream_add_UINT32(packet->payload, pcb->burst_estimation);

}


typedef struct pcb {
	int id;
	int size;
	t_list *instructions;
	int program_counter;
	//t_ptbr page_table;
	int burst_estimation;
} t_pcb;

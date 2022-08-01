/*
 * pcb.h
 *
 *  Created on: 3 jun. 2022
 *      Author: utnso
 */

#ifndef INCLUDE_PCB_UTILS_H_
#define INCLUDE_PCB_UTILS_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <commons/log.h>
#include <commons/string.h>
#include <commons/collections/list.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <pthread.h>
#include <commons/log.h>
#include <errno.h>

#include "process_utils.h"

#include "serialization.h"
#include "networking.h"

typedef struct pcb
{
	uint32_t pid;
	uint32_t size;
	uint32_t client_socket;
	t_list *instructions;
	uint32_t program_counter;
	uint32_t page_table;
	uint32_t frames_index;
	uint32_t burst_estimation;
	uint32_t blocked_time;
	uint32_t pending_io_time;
	uint32_t left_burst_estimation;
} t_pcb;

void stream_take_pcb(t_packet *, t_pcb *);
void stream_add_pcb(t_packet *packet, t_pcb *pcb);

t_pcb *create_pcb();
void pcb_destroy(t_pcb *pcb);

#endif /* INCLUDE_PCB_UTILS_H_ */

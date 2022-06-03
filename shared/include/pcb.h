/*
 * pcb.h
 *
 *  Created on: 3 jun. 2022
 *      Author: utnso
 */

#ifndef INCLUDE_PCB_H_
#define INCLUDE_PCB_H_


#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<stdint.h>
#include<commons/log.h>
#include<commons/string.h>
#include<commons/collections/list.h>
#include<sys/socket.h>
#include<sys/types.h>
#include<netdb.h>
#include<pthread.h>
#include<commons/log.h>
#include<errno.h>

#include"serialization.h"
#include"networking.h"
#include"process_utils.h"



typedef struct pcb {
	int id;
	int size;
	t_list *instructions;
	int program_counter;
	//t_ptbr page_table;
	int burst_estimation;
} t_pcb;

void stream_take_pcb(t_packet *, t_pcb *);
void stream_add_process(t_packet *,t_pcb *);
t_pcb* create_pcb();
void destroy_pcb(t_pcb *);


#endif /* INCLUDE_PCB_H_ */
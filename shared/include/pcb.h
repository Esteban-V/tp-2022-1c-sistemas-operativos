/*
 * pcb.h
 *
 *  Created on: 3 jun. 2022
 *      Author: utnso
 */

#ifndef INCLUDE_PCB_H_
#define INCLUDE_PCB_H_

#include<sys/socket.h>
#include<sys/types.h>
#include<netdb.h>
#include<pthread.h>
#include<commons/log.h>
#include<errno.h>

#include"serialization.h"


typedef struct pcb {
	int id;
	int size;
	t_list *instructions;
	int program_counter;
	//t_ptbr page_table;
	int burst_estimation;
} t_pcb;

t_pcb* create_pcb();
void destroy_pcb(t_pcb *pcb);


#endif /* INCLUDE_PCB_H_ */

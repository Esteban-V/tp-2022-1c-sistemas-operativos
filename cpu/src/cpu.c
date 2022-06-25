/*
 * cpu.c
 *
 *  Created on: 15 jun. 2022
 *      Author: utnso
 */
#include "cpu.h"

int main(){
	// Initialize logger
		logger = log_create("./cpu.log", "CPU", 1, LOG_LEVEL_TRACE);
		log_info(logger,"LOGGER CREADO");
		config = getcpuConfig("cpu.config");


		// Creacion de server
		server_dispatch_socket = create_server(config->ip,config->dispatchListenPort);
		server_interrupt_socket = create_server(config->ip,config->interruptListenPort);

		sem_init(&pcb_loaded,0,0);

		log_info(logger, "Servidor de cpu creado");

		pthread_create(&interruptionThread, 0, interruption, NULL);
		pthread_detach(interruptionThread);

		pthread_create(&execThread, 0, execution, NULL);
		pthread_detach(execThread);

		while (1) {
			server_listen(server_dispatch_socket, header_handler);
		}

		log_destroy(logger);

}

void* execution(){
	while(1){
		sem_wait(&pcb_loaded);
		log_info(logger,"QUE HAGO ACA");
		while(pcb->program_counter<list_size(pcb->instructions)){
		t_instruction* instruccion;

		instruccion = list_get(pcb->instructions, pcb->program_counter) ;
		pcb->program_counter=pcb->program_counter+1;
		char* instru = instruccion->id;
		uint32_t n;
		uint32_t m;
		log_info(logger,"%s",instru);
		enum operation entry = getOperation(instru);
		switch(entry){
		case NO_OPOP:{
			log_info(logger, "EJECUTA NO OP");
			n = *((uint32_t*) list_get(instruccion->params, 0));
			noOperation(n);
			break;
			}
			case IOOP:{
				log_info(logger, "EJECUTA I/O");
				n = *((uint32_t*) list_get(instruccion->params, 0));
				inAndOut(n);
				break;
			}
			case READOP:{
				log_info(logger, "EJECUTA READ");
				n = *((uint32_t*) list_get(instruccion->params, 0));
				break;
			}
			case COPYOP:{
				log_info(logger, "EJECUTA COPY");
				n = *((uint32_t*) list_get(instruccion->params, 0));
				m = *((uint32_t*) list_get(instruccion->params, 0));
				break;
			}
			case WRITEOP:{
				log_info(logger, "EJECUTA WRITE");
				n = *((uint32_t*) list_get(instruccion->params, 0));
				m = *((uint32_t*) list_get(instruccion->params, 0));
				break;
			}
			case EXITOP:{
				log_info(logger, "EJECUTA EXIT");
				exitiando();
				break;
			}
			case DEAD:{
				log_info(logger, "MAL");
				break;
			}
			}
		log_info(logger,"CUANTA VECES");
		}

	}
}

void* interruption(){

	while(1){
		log_info(logger, "Servidor de interrupciones creado");
		server_listen(server_interrupt_socket, header_handler);
	}
}

bool receivedPcb(t_packet *petition, int console_socket){
	pcb = create_pcb();
	stream_take_pcb(petition, pcb);
	log_info(logger,"RECIBO PCB %d",pcb->id);
	sem_post(&pcb_loaded);
	return false;
}

bool receivedInterruption(t_packet *petition, int console_socket){
	return false;
}

bool (*kernel_handlers[7])(t_packet *petition, int console_socket) =
{
		false,
		false,
		false,
		false,
		receivedPcb,
		receivedInterruption,
		false
};

void* header_handler(void *_client_socket) {
	int client_socket = (int) _client_socket;
	bool serve = true;
	while (serve) {
		t_packet *packet = socket_receive_packet(client_socket);
		if (packet == NULL) {
			if (!socket_retry_packet(client_socket, &packet)) {
				close(client_socket);
				break;
			}
		}
		serve = kernel_handlers[packet->header](packet, client_socket);
		packet_destroy(packet);
	}
	return 0;
}

enum operation getOperation(char* op){
	log_info(logger, "%s",op);

	if(!strcmp(op,"NO_OP")){
		return NO_OPOP;
	}else if(!strcmp(op,"I/O")){
		return IOOP;
	}else if(!strcmp(op,"READ")){
		return READOP;
	}else if(!strcmp(op,"WRITE")){
		return WRITEOP;
	}else if(!strcmp(op,"COPY")){
		return COPYOP;
	}else if(!strcmp(op,"EXIT")){
		return EXITOP;
	}else return DEAD;

}

void noOperation(time){
	usleep(time);
	return;
}
void exitiando(){/*
	t_packet *pcb_packet = create_packet(PCB_TO_CPU, 64);//implementar PCBTOCPU
	stream_add_pcb(pcb_packet,pcb);
	if (cpu_server_socket != -1) {
			socket_send_packet(cpu_server_socket, pcb_packet);
	}
	*/
}
void inAndOut(){

}





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

		sem_init(&pcb_loaded,0,1);

		log_info(logger, "Servidor de cpu creado");

		pthread_create(&interruptionThread, 0, interruption, NULL);
		pthread_detach(interruptionThread);
		server_listen(server_dispatch_socket, header_handler);
		while (1) {
			log_info(logger, "UNO");
			server_listen(server_dispatch_socket, header_handler);
			log_info(logger,"QEEE");
			sem_wait(&pcb_loaded);
			while(pcb->program_counter<list_size(pcb->instructions)){

				t_instruction* instruccion;
				instruccion = list_get(pcb->instructions, pcb->program_counter) ;
				char* instru = instruccion->id;
				uint32_t n = *((uint32_t*) list_get(instruccion->params, 0));

				switch(getOperation(instru)){
				case NO_OPOP:{
					log_info(logger, "EJECUTA NO OP");
					break;
				}
				case IOOP:{
					log_info(logger, "EJECUTA I/O");
					break;
				}
				case READOP:{
					log_info(logger, "EJECUTA READ");
					break;
				}
				case COPYOP:{
					log_info(logger, "EJECUTA COPY");
					break;
				}
				case WRITEOP:{
					log_info(logger, "EJECUTA WRITE");
					break;
				}
				case EXITOP:{
					log_info(logger, "EJECUTA EXIT");
					break;
				}
				}
			pcb->program_counter=pcb->program_counter+1;
			}

		}

		log_destroy(logger);

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

enum operation getOperation(char* operation){
	if(operation=="NO_OP"){
		return NO_OPOP;
	}
	if(operation=="I/O"){
		return IOOP;
	}
	if(operation=="READ"){
		return READOP;
	}
	if(operation=="WRITE"){
		return WRITEOP;
	}
	if(operation=="COPY"){
		return COPYOP;
	}
	if(operation=="COPY"){
		return COPYOP;
	}
	if(operation=="EXIT"){
		return EXITOP;
	}
}



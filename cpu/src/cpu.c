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
		int server_dispatch_socket = create_server(config->ip,config->dispatchListenPort);

		pthread_cond_init(&pcb_loaded, NULL);

		log_info(logger, "Servidor de cpu creado");

		pthread_create(&interruptionThread, 0, interruption, NULL);
		pthread_detach(interruptionThread);

		while (1) {
			log_info(logger, "UNO");
			server_listen(server_dispatch_socket, header_handler);
			pthread_cond_wait(&pcb_loaded);

			t_instruction* instruccion;
			instruccion = list_get(pcb->instructions, pcb->program_counter) ;
			char* instru = instruccion->id;

			switch(instru){
			case:

			}

			uint32_t n = *((uint32_t*) list_get(instruccion->params, 0));
				return n;

		}

		log_destroy(logger);

}

void* interruption(){
	log_info(logger, "Servidor de interrupciones creado");
	int server_interrupt_socket = create_server(config->ip,config->interruptListenPort);
	while(1){
		server_listen(server_interrupt_socket, header_handler);
	}
}

bool receivedPcb(t_packet *petition, int console_socket){
	pcb = create_pcb();
	stream_take_pcb(petition, pcb);
	log_info(logger,"RECIBO PCB %d",pcb->id);
	pthread_cond_signal(&pcb_loaded);
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



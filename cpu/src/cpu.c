/*
 * cpu.c
 *
 *  Created on: 15 jun. 2022
 *      Author: utnso
 */
#include "cpu.h"

int main() {
	// Initialize logger
	logger = log_create("./cpu.log", "CPU", 1, LOG_LEVEL_TRACE);
	config = get_cpu_config("cpu.config");

	// Creacion de server
	kernel_dispatch_socket = create_server(config->ip,
			config->dispatchListenPort);
	kernel_interrupt_socket = create_server(config->ip,
			config->interruptListenPort);
	log_info(logger, "CPU ready for kernel");

	sem_init(&pcb_loaded, 0, 0);

	pthread_create(&interruptionThread, 0, interruption, NULL);
	pthread_detach(interruptionThread);

	pthread_create(&execThread, 0, execution, NULL);
	pthread_detach(execThread);

	while (1) {
		server_listen(kernel_dispatch_socket, header_handler);
	}

	log_destroy(logger);

}

void* execution() {
	while (1) {
		sem_wait(&pcb_loaded);
		while (pcb->program_counter < list_size(pcb->instructions)) {
			t_instruction *instruccion;

			instruccion = list_get(pcb->instructions, pcb->program_counter);
			pcb->program_counter = pcb->program_counter + 1;

			char *op_code = instruccion->id;
			uint32_t fst_param = -1, snd_param = -1;

			log_info(logger, "%s", op_code);
			enum operation entry = getOperation(op_code);
			switch (entry) {
			case NO_OP: {
				log_info(logger, "EJECUTA NO OP");
				fst_param = *((uint32_t*) list_get(instruccion->params, 0));
				execute_no_op(fst_param);
				break;
			}
			case IO_OP: {
				log_info(logger, "EJECUTA I/O");
				fst_param = *((uint32_t*) list_get(instruccion->params, 0));
				execute_io(fst_param);
				break;
			}
			case READ: {
				log_info(logger, "EJECUTA READ");
				fst_param = *((uint32_t*) list_get(instruccion->params, 0));
				break;
			}
			case COPY: {
				log_info(logger, "EJECUTA COPY");
				fst_param = *((uint32_t*) list_get(instruccion->params, 0));
				snd_param = *((uint32_t*) list_get(instruccion->params, 1));
				break;
			}
			case WRITE: {
				log_info(logger, "EJECUTA WRITE");
				fst_param = *((uint32_t*) list_get(instruccion->params, 0));
				snd_param = *((uint32_t*) list_get(instruccion->params, 1));
				break;
			}
			case EXIT_OP: {
				execute_exit();
				break;
			}
			case DEAD: {
				log_info(logger, "MAL");
				break;
			}
			}
		}

	}
}

void* interruption() {
	while (1) {
		server_listen(kernel_interrupt_socket, header_handler);
	}
}

bool receivedPcb(t_packet *petition, int console_socket) {
	pcb = create_pcb();
	stream_take_pcb(petition, pcb);
	log_info(logger, "RECIBO PCB %d", pcb->pid);
	sem_post(&pcb_loaded);
	return false;
}

bool receivedInterruption(t_packet *petition, int console_socket) {
	// recibir header interrupcion
	// "desalojar" actual --> guardar contexto de ejecucion
	// devolver pcb actualizado por dispatch
	return false;
}

bool (*kernel_handlers[7])(t_packet *petition, int console_socket) =
{
	NULL,
	NULL,
	NULL,
	NULL,
	receivedPcb,
	receivedInterruption,
	NULL
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

enum operation getOperation(char *op) {
	log_info(logger, "%s", op);

	if (!strcmp(op, "NO_OP")) {
		return NO_OP;
	} else if (!strcmp(op, "I/O")) {
		return IO_OP;
	} else if (!strcmp(op, "READ")) {
		return READ;
	} else if (!strcmp(op, "WRITE")) {
		return WRITE;
	} else if (!strcmp(op, "COPY")) {
		return COPY;
	} else if (!strcmp(op, "EXIT")) {
		return EXIT_OP;
	} else
		return DEAD;

}

void execute_no_op(uint32_t time) {
	usleep((time_t) time);
	return;
}

void execute_exit() {/*
 t_packet *pcb_packet = create_packet(PCB_TO_CPU, 64);//implementar PCBTOCPU
 stream_add_pcb(pcb_packet,pcb);
 if (cpu_server_socket != -1) {
 socket_send_packet(cpu_server_socket, pcb_packet);
 }
 */
}

void execute_io(uint32_t time) {

}


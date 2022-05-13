#include "kernel.h"

void stream_take_process(t_packet *packet, t_process *process) {
	uint32_t *size = &(process->size);
	stream_take_UINT32P(packet->payload, &size);
	printf("size %d\n", process->size);

	t_list *instructions = stream_take_LIST(packet->payload,
			stream_take_instruction);
	memcpy(&(process->instructions), &instructions, sizeof(t_instruction));
}

void log_param(void *param) {
	uint32_t p = (uint32_t) param;
	printf("param %d\n", param);
}

void stream_take_instruction(t_stream_buffer *stream, void **elem) {

	t_instruction *instruction = (t_instruction*) elem;

	char **id = &(instruction->id);
	stream_take_STRINGP(stream, &id);
	printf("id %s\n", id);

	t_list *params = stream_take_LIST(stream, stream_take_UINT32P);
	// memcpy(&(instruction->params), &params, sizeof(uint32_t));
	list_iterate(params, log_param);
}

///////////////////////////////////////////////////////////////////////////////
t_pcb* create_pcb(t_process *process) {
	t_pcb *pcb = malloc(sizeof(t_pcb));
	pcb->instructions = list_create();
	pcb->id = malloc(sizeof(int));
	pcb->size = malloc(sizeof(int));
	pcb->program_counter = malloc(sizeof(int));
	pcb->burst_estimation = malloc(sizeof(int));
	//pcb->page_table=malloc(sizeof(t_ptbr));

	memcpy(pcb->instructions, process->instructions, sizeof(t_list));
	//pcb->id = generate_pcb_id();
	pcb->size = process->size;
	pcb->program_counter = 0;
	//pcb->burst_estimation = estimate_cpu_burst();

	return pcb;
}

/*int estimate_cpu_burst(){
 int alfa = config_get_int_value(config,"ALFA");
 return alfa* + (1-alfa);
 }*/

void destroy_pcb(t_pcb *pcb) {
	if (pcb != NULL) {
		free(pcb->id);
		free(pcb->size);
		free(pcb->program_counter);
		free(pcb->burst_estimation);

		//list_destroy(pcb->instructions);

		free(pcb->instructions);
		free(pcb);
	}

}

///////////////////////////////////////////////////////////////////////////////

bool receive_process(t_packet *petition, int console_socket) {
	t_process *received_process = create_process();
	puts("empieza");
	stream_take_process(petition, received_process);
	puts("termina");
	log_process(logger, received_process);

	//char *file_name = stream_take_STRING(petition->payload);
	return false;
}

bool (*kernel_handlers[1])(t_packet *petition, int console_socket) =
{
	receive_process,
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

int main(void) {
	logger = create_logger();
	log_info(logger, "Loggin");
	config = getKernelConfig("kernel.config");
	log_info(logger, "No Rompio Paco");
	if (config == NULL) {
		log_info(logger, "Rompio Paco");
		return EXIT_FAILURE;
	}

	// Creacion de Server
	int server_socket = create_server(config->kernelIP, config->kernelPort);
	log_info(logger, "Servidor listo para recibir al cliente");

	// Poner en Modo Escucha
	while (1) {
		server_listen(server_socket, header_handler);

		t_process *process;
		//Proceso a PCB

		//Planificador a Largo Plazo
		//NEW
		//queue_push(newQ,pcb);

		//t_pcb pcb = create_pcb(process);

		//READY

		if (queue_size(readyQ) < config->multiprogrammingLevel) {
			t_queue *ready_process = queue_pop(newQ);
			//queue_push(readyQ, ready_process);

			//MSJ a Memoria
		} else {

		}

		// if finalizacion -> exit -> msj a memoria -> msj a consola

		//plan med
		/*int max_blocked_time = config_get_int_value(config,"TIEMPO_MAXIMO_BLOQUEADO");
		 if(X->blocked_time>max_blocked_time){
		 //Suspender
		 //msj a memoria
		 }*/

		//plan corto
		/*if(){
		 //estimacion

		 }*/

		/*if(){
		 //interrupt a CPU
		 //CPU desaloja proceso
		 //PCB se recibe por dispatch

		 }*/

	}

	destroyKernelConfig(config);

	return EXIT_SUCCESS;
}

t_log* create_logger() {
	t_log *new_logger = log_create("kernel.log", "KERNEL", 1, LOG_LEVEL_INFO);
	return new_logger;
}

t_config* create_config() {
	t_config *new_config = config_create("kernel.config");
	return new_config;
}

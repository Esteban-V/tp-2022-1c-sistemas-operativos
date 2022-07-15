#include "cpu.h"

int main() {
	// Initialize logger
	logger = log_create("./cfg/cpu-final.log", "CPU", 1, LOG_LEVEL_TRACE);
	config = get_cpu_config("./cfg/cpu.config");

	// Creacion de server
	kernel_dispatch_socket = create_server(config->dispatchListenPort);
	kernel_interrupt_socket = create_server(config->interruptListenPort);

	pthread_mutex_lock(&mutex_log);
	log_info(logger, "CPU Ready for Kernel");
	pthread_mutex_unlock(&mutex_log);

	sem_init(&pcb_loaded, 0, 0);
	pthread_mutex_init(&mutex_kernel_socket, NULL);

	pthread_create(&interruptionThread, 0, listen_interruption, NULL);
	pthread_detach(interruptionThread);

	// Inicializar memory listener
	pthread_create(&memoryThread, 0, memory_listener, NULL);
	pthread_detach(memoryThread);

	pthread_create(&execThread, 0, cpu_cycle, NULL);
	pthread_detach(execThread);

	memory_socket = connect_to(config->memoryIP, config->memoryPort);

	// Handshake con Memoria
	memory_handshake();

	while (1) {
		server_listen(kernel_dispatch_socket, header_handler);
	}

	log_destroy(logger);
}

void* listen_interruption() {
	while (1) {
		server_listen(kernel_interrupt_socket, header_handler);
	}
}

void *memory_listener()
{
	while (1)
	{
		header_handler(memory_socket);
	}
}

void pcb_to_kernel(kernel_headers header) {
	t_packet *pcb_packet = create_packet(header, INITIAL_STREAM_SIZE);
	stream_add_pcb(pcb_packet, pcb);

	pthread_mutex_lock(&mutex_kernel_socket);
	if (kernel_client_socket != -1) {
		socket_send_packet(kernel_client_socket, pcb_packet);
		pthread_mutex_unlock(&mutex_kernel_socket);

		pthread_mutex_lock(&mutex_log);
		log_info(logger, "PID #%d CPU --> Kernel", pcb->pid);
		pthread_mutex_unlock(&mutex_log);
	}
	packet_destroy(pcb_packet);
}

bool (*cpu_handlers[3])(t_packet *petition, int console_socket) =
{
	receive_pcb, // PCB_TO_CPU
	receive_interruption, // INTERRUPT
};

void* header_handler(void *_client_socket) {
	pthread_mutex_lock(&mutex_kernel_socket);
	kernel_client_socket = (int) _client_socket;
	pthread_mutex_unlock(&mutex_kernel_socket);

	bool serve = true;
	while (serve) {
		t_packet *packet = socket_receive_packet(kernel_client_socket);
		if (packet == NULL) {
			if (!socket_retry_packet(kernel_client_socket, &packet)) {
				close(kernel_client_socket);
				break;
			}
		}
		serve = cpu_handlers[packet->header](packet, kernel_client_socket);
		packet_destroy(packet);
	}
	return 0;
}

bool receive_pcb(t_packet *petition, int kernel_socket) {
	pcb = create_pcb();
	stream_take_pcb(petition, pcb);
	if (!!pcb) {
		pthread_mutex_lock(&mutex_log);
		log_info(logger, "Received PID: #%d ; %d Instructions", pcb->pid,
				list_size(pcb->instructions));
		pthread_mutex_unlock(&mutex_log);
		sem_post(&pcb_loaded);
		return true;
	}
	return false;
}

bool receive_interruption(t_packet *petition, int kernel_socket) {
	// recibir header interrupcion
	// meter interrupcion en queue o variable ya q se puede 1 a la vez
	// activar sem de interrupcion
	return false;
}

void (*execute[6])(t_list *params) =
{
	execute_no_op,
	execute_io,
	execute_read,
	execute_copy,
	execute_write,
	execute_exit,
};

void* cpu_cycle() {
	while (1) {
		sem_wait(&pcb_loaded);
		while (pcb->program_counter < list_size(pcb->instructions)) {
			t_instruction *instruction;
			enum operation op = fetch_and_decode(&instruction);
			execute[op](instruction->params);
		}
	}
}

enum operation fetch_and_decode(t_instruction **instruction) {
	*instruction = list_get(pcb->instructions, pcb->program_counter);

	pcb->program_counter = pcb->program_counter + 1;

	char *op_code = (*instruction)->id;
	enum operation op = get_op(op_code);

	pthread_mutex_lock(&mutex_log);
	log_info(logger, "PID #%d / Instruction %d --> %s", pcb->pid,
			pcb->program_counter, op_code);
	pthread_mutex_unlock(&mutex_log);

	return op;
}

void execute_no_op() {
	usleep((time_t) config->delayNoOp);
}

void execute_io(t_list *params) {
	uint32_t *time = list_get(params, 0);
	pcb->pending_io_time = time;
	pcb_to_kernel(IO_CALL);
}

void execute_read(t_list *params) {
	uint32_t *l_address = list_get(params, 0);
}

void execute_copy(t_list *params) {
	uint32_t *fst_param = list_get(params, 0);
	uint32_t *snd_param = list_get(params, 1);
}

void execute_write(t_list *params) {
	uint32_t *l_address = list_get(params, 0);
	uint32_t *value = list_get(params, 1);
}

void execute_exit() {
	pcb_to_kernel(EXIT_CALL);
}

void memory_handshake() {
	if (memory_socket != -1) {
		socket_send_header(memory_socket, MEM_HANDSHAKE);
	}

	pthread_mutex_lock(&mutex_log);
	log_info(logger, "Handshake with memory requested");
	pthread_mutex_unlock(&mutex_log);

	t_packet *mem_data = socket_receive_packet(memory_socket);

	// TODO? Errores

	// Recibir datos
	config->pageSize = stream_take_UINT32(mem_data->payload);
	config->memoryEntriesPerTable = stream_take_UINT32(mem_data->payload);

	pthread_mutex_lock(&mutex_log);
	log_info(logger, "Handshake with memory successful");
	log_info(logger, "Page size: %d | Entries per table: %d", config->pageSize,
			config->memoryEntriesPerTable);
	pthread_mutex_unlock(&mutex_log);

	packet_destroy(mem_data);
}

/*
 ============================================================================
 Name        : kernel.c
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

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
	config = create_config();

	ip = config_get_string_value(config, "IP");
	port = config_get_string_value(config, "PUERTO");

	int server_socket = create_server(ip, port);
	log_info(logger, "Servidor listo para recibir al cliente");

	while (1) {
		server_listen(server_socket, header_handler);
	}
}

t_log* create_logger() {
	t_log *new_logger = log_create("kernel.log", "KERNEL", 1, LOG_LEVEL_INFO);
	return new_logger;
}

t_config* create_config() {
	t_config *new_config = config_create("kernel.config");
	return new_config;
}


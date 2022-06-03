#include"memory.h"

int main() {
	// Initialize logger
	logger = log_create("./memory.log", "MEMORY", 1, LOG_LEVEL_TRACE);
	config = getMemoryConfig("memory.config");

	// Creacion de server
	int server_socket = create_server(config->memoryIP, config->memoryPort);
	log_info(logger, "Servidor de Memoria creado");

	//tlb = createTLB();

	while (1) {
		server_listen(server_socket, header_handler);
	}

	log_destroy(logger);
	//destroyTLB(tlb);

	return EXIT_SUCCESS;
}

bool receive_memory_info(t_packet *petition, int console_socket) {
	uint32_t *size;
	stream_take_UINT32P(petition->payload, size);
	log_info(logger, "UINT STREAM, %d", size);

	if (!!size) {
		log_info(logger, "UINT RECEIVED, %d", size);
	}

	return false;
}

bool (*memory_handlers[1])(t_packet *petition, int console_socket) =
{
	receive_memory_info,
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
		serve = memory_handlers[packet->header](packet, client_socket);
		packet_destroy(packet);
	}
	return 0;
}

#include"memory.h"

int main() {
	// Initialize logger
	logger = log_create("./memory.log", "MEMORY", 1, LOG_LEVEL_TRACE);
	config = getMemoryConfig("memory.config");

	// Creacion de server
	int server_socket = create_server("127.0.0.1", "8002");
	log_info(logger, "Servidor de Memoria creado");

	memory = initializeMemory(config);
	clock_m_counter = 0;
	pageTables = dictionary_create();

	while (1) {
		log_info(logger, "TEST");
		server_listen(server_socket, header_handler);
	}

	destroyMemoryConfig(config);
    dictionary_destroy_and_destroy_elements(pageTables, _destroyPageTable);
    log_destroy(logger);

	return EXIT_SUCCESS;
}

bool receive_memory_info(t_packet *petition, int console_socket) {
	log_info(logger, "DOS");
	int size = (int)stream_take_UINT32(petition->payload);
	log_info(logger, "UINT STREAM, %d", size);

	if (!!size) {
		log_info(logger, "UINT RECEIVED, %d", size);
	}

	return false;
}

bool (*memory_handlers[7])(t_packet *petition, int console_socket) =
{
	true,
	true,
	true,
	receive_memory_info,
	true,
	true,
	true
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

/////////////////////

t_memory *initializeMemory(t_memoryConfig *config){
    t_memory *newMemory = malloc(sizeof(t_memory));
    newMemory->memory = calloc(1,config->memorySize);

    return newMemory;
}

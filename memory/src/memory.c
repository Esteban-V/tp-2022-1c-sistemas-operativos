#include "imemory.h"

int main() {
	// Initialize logger
	logger = log_create("./cfg/memory-final.log", "MEMORY", 1, LOG_LEVEL_TRACE);
	memoryConfig = getMemoryConfig("./cfg/memory.config");

	metadata = metadata_init();

	// Creacion de server
	server_socket = create_server(memoryConfig->listenPort);

	pthread_mutex_lock(&mutex_log);
	log_info(logger, "Memory server ready");
	pthread_mutex_unlock(&mutex_log);

	fsLevelTables = list_create();
	sdLevelTables = list_create();

	swap_files = list_create();
	/* hay que ver como determinar swapfiles y filesize
	 agregar donde se swappean
	 list_add(swap_files, swapFile_create(memoryConfig->swap_files[i], memoryConfig->fileSize, memoryConfig->pageSize));
	 */
	sem_init(&writeRead, 0, 2); // TODO Ver si estan vien los semaforos, o si van en otras funciones tmb

	memory = memory_init();
	metadata->clock_m_counter = 0;
	pageTables = dictionary_create();

	if (!strcmp(memoryConfig->replaceAlgorithm, "CLOCK")) {
		replace_algo = clock_alg;
	} else if (!strcmp(memoryConfig->replaceAlgorithm, "CLOCK-M")) {
		replace_algo = clock_m_alg;
	} else {
		pthread_mutex_lock(&mutex_log);
		log_warning(logger,
				"Wrong replacing algorithm set in config --> Using CLOCK");
		pthread_mutex_unlock(&mutex_log);
	}

	clock_m_counter = 0;

	while (1) {
		server_listen(server_socket, header_handler);
	}

	terminate_memory(false);
}

// READY
bool process_new(t_packet *petition, int kernel_socket) {
	uint32_t pid = stream_take_UINT32(petition->payload);
	uint32_t size = stream_take_UINT32(petition->payload);

	if (!!pid) {
		pthread_mutex_lock(&mutex_log);
		log_info(logger, "Initializing memory structures for PID #%d", pid);
		pthread_mutex_unlock(&mutex_log);

		// char* swap_filename = swap_init(pid);
		int pt1_index = page_table_init(size);

		t_packet *response = create_packet(PROCESS_MEMORY_READY,
		INITIAL_STREAM_SIZE);
		stream_add_UINT32(response->payload, pt1_index);
		socket_send_packet(kernel_socket, response);

		packet_destroy(response);
	}

	return true;
}

// READY
bool process_suspend(t_packet *petition, int kernel_socket) {
	uint32_t pid = stream_take_UINT32(petition->payload);
	uint32_t pt1_entry = stream_take_UINT32(petition->payload);

	if (!!pid) {
		pthread_mutex_lock(&mutex_log);
		log_info(logger, "Process suspension requested for PID #%d", pid);
		pthread_mutex_unlock(&mutex_log);

		pthread_mutex_lock(&pageTablesMut);
		t_ptbr2 *pt2 = getPageTable2(pid, pt1_entry, pageTables);
		uint32_t page_cant = list_size(pt2->entries);

		// Swappear Pages
		for (uint32_t i = 0; i < page_cant; i++) {
			if (((t_page_entry*) list_get(pt2->entries, i))->present) {
				void *pageContent = (void*) memory_getFrame(memory,
						((t_page_entry*) list_get(pt2->entries, i))->frame);
				savePage(pid, i, pageContent);
				pthread_mutex_lock(&metadataMut);
				metadata->entries[((t_page_entry*) list_get(pt2->entries, i))->frame].isFree =
				true;
				pthread_mutex_unlock(&metadataMut);
				((t_page_entry*) list_get(pt2->entries, i))->present = false;
			}
		}
		pthread_mutex_unlock(&pageTablesMut);

		// Cambiar metadata
		if (metadata->firstFrame) {
			pthread_mutex_lock(&metadataMut);
			for (uint32_t i = 0;
					i < memoryConfig->frameQty / memoryConfig->framesPerProcess;
					i++) {
				if (metadata->firstFrame[i] == pid)
					metadata->firstFrame[i] = -1;
			}
			pthread_mutex_unlock(&metadataMut);
		}

		// freeProcessTLBEntries(pid); TODO CPU
	}

	return false;
}

// TODO no se si falta o no intervenir en el t_memory
bool process_exit(t_packet *petition, int cpu_socket) {
	uint32_t pid = stream_take_UINT32(petition->payload);
	uint32_t pt1_entry = stream_take_UINT32(petition->payload);

	if (!!pid) {
		pthread_mutex_lock(&mutex_log);
		log_info(logger, "Destroying PID #%d", pid);
		pthread_mutex_unlock(&mutex_log);

		pthread_mutex_lock(&pageTablesMut);
		t_ptbr2 *pt2 = getPageTable2(pid, pt1_entry, pageTables);
		uint32_t pages_cant = list_size(pt2->entries);
		pthread_mutex_unlock(&pageTablesMut);

		for (uint32_t i = pages_cant - 1; i >= 0; i--) {
			// Borrar swap
			bool error = destroy_swap_page(pid, i);

			t_packet *response_packet = create_packet(
					error ? SWAP_ERROR : SWAP_OK, 0);
			socket_send_packet(cpu_socket, response_packet);
			packet_destroy(response_packet);

			// Cambiar valores en metadata
			pthread_mutex_lock(&pageTablesMut);
			if (((t_page_entry*) list_get(pt2->entries, i))->present == true) {
				uint32_t frame =
						((t_page_entry*) list_get(pt2->entries, i))->frame;
				pthread_mutex_lock(&metadataMut);
				(metadata->entries)[frame].isFree = true;
				pthread_mutex_unlock(&metadataMut);
			}
			pthread_mutex_unlock(&pageTablesMut);
		}

		char *_PID = string_itoa(pid);
		dictionary_remove_and_destroy(pageTables, _PID, page_table_destroy);
		free(_PID);

		/*t_packet *response = create_packet(OK, 0);
		 socket_send_packet(cpu_socket, response);
		 packet_destroy(response);*/

		pthread_mutex_lock(&mutex_log);
		log_info(logger, "Destroyed PID #%d successfully", pid);
		pthread_mutex_unlock(&mutex_log);

		// sigUsr1HandlerTLB(0);

		// freeProcessEntries(pid);
	}

	return true;
}

// READY
bool access_lvl1_table(t_packet *petition, int cpu_socket) {
	uint32_t pt1_index = stream_take_UINT32(petition->payload);
	uint32_t index = stream_take_UINT32(petition->payload);

	if (pt1_index != NULL) {
		pthread_mutex_lock(&mutex_log);
		log_info(logger, "Getting level 2 table index");
		pthread_mutex_unlock(&mutex_log);

		int pt2_index = getPageTable2(pt1_index, index);
		t_packet *response = create_packet(TABLE2_TO_CPU, INITIAL_STREAM_SIZE);
		stream_add_UINT32(response->payload, pt2_index);

		socket_send_packet(cpu_socket, response);
		packet_destroy(response);
	}

	return false;
}

// READY?
bool access_lvl2_table(t_packet *petition, int cpu_socket) {
	uint32_t pt2_index = stream_take_UINT32(petition->payload);
	uint32_t index = stream_take_UINT32(petition->payload);

	if (pt2_index != NULL) {
		pthread_mutex_lock(&mutex_log);
		log_info(logger, "Getting frame number");
		pthread_mutex_unlock(&mutex_log);

		uint32_t frame_num = pageTable_getFrame(pt2_index, index);

		t_packet *response = create_packet(FRAME_TO_CPU,
		INITIAL_STREAM_SIZE);
		stream_add_UINT32(response->payload, frame_num);
		socket_send_packet(cpu_socket, response);
		packet_destroy(response);
	}

	return false;
}

// ver si se cumple lo del OK, chequear que se pueda crear pag

// READY?
bool memory_write(t_packet *petition, int cpu_socket) {
	sem_wait(&writeRead);

	uint32_t dir = stream_take_UINT32(petition->payload);
	uint32_t toWrite = stream_take_UINT32(petition->payload);

	if (!!toWrite) {

		pthread_mutex_lock(&mutex_log);
		log_info(logger, "Memory Write Request");
		pthread_mutex_unlock(&mutex_log);

		// Cargar página
		uint32_t frameVictima = swapPage(pid, pt1_entry, pt2_entry, page);

		t_packet *response;
		if (frameVictima == -1) {
			response = create_packet(SWAP_ERROR, 0);
		} else {
			response = create_packet(SWAP_OK, INITIAL_STREAM_SIZE);
		}

		stream_add_UINT32(response->payload, frameVictima);
		socket_send_packet(cpu_socket, response);
		packet_destroy(response);
	}

	sem_post(&writeRead);

	return false;
}

bool memory_read(t_packet *petition, int cpu_socket) {
	sem_wait(&writeRead);

	uint32_t dir = stream_take_UINT32(petition->payload);

	pthread_mutex_lock(&mutex_log);
	log_info(logger, "Reading memory");
	pthread_mutex_unlock(&mutex_log);

	sem_post(&writeRead);

	return false;
}

bool cpu_handshake(t_packet *petition, int cpu_socket) {
	pthread_mutex_lock(&mutex_log);
	log_info(logger, "Received handshake petition from CPU");
	pthread_mutex_unlock(&mutex_log);

	t_packet *mem_data = create_packet(TABLE_INFO_TO_CPU, INITIAL_STREAM_SIZE);
	stream_add_UINT32(mem_data->payload, memoryConfig->pageSize);
	stream_add_UINT32(mem_data->payload, memoryConfig->entriesPerTable);
	socket_send_packet(cpu_socket, mem_data);
	packet_destroy(mem_data);

	pthread_mutex_lock(&mutex_log);
	log_info(logger, "Relayed relevant memory data to CPU");
	pthread_mutex_unlock(&mutex_log);

	return 0;
}

t_mem_metadata* metadata_init() {
	t_mem_metadata *metadata = malloc(sizeof(t_mem_metadata));
	metadata->entryQty = memoryConfig->frameQty;
	metadata->clock_counter = 0;
	metadata->entries = calloc(metadata->entryQty, sizeof(t_frame_metadata));
	metadata->clock_m_counter = NULL;
	metadata->firstFrame = NULL;

	uint32_t blockQuantity = memoryConfig->frameQty
			/ memoryConfig->framesPerProcess;

	metadata->firstFrame = calloc(blockQuantity, sizeof(uint32_t));
	memset(metadata->firstFrame, -1, sizeof(uint32_t) * blockQuantity);

	metadata->clock_m_counter = calloc(blockQuantity, sizeof(uint32_t));

	for (int i = 0; i < blockQuantity; i++) {
		metadata->clock_m_counter[i] = i * memoryConfig->framesPerProcess;
	}

	for (int i = 0; i < metadata->entryQty; i++) {
		((metadata->entries)[i]).isFree = true;
		((metadata->entries)[i]).timeStamp = 0;
	}

	return metadata;
}

void metadata_destroy(t_mem_metadata *meta) {
	if (meta->firstFrame) {
		free(meta->firstFrame);
		free(meta->clock_m_counter);
	}

	free(meta->entries);
	free(meta);
}

t_memory* memory_init() {
	t_memory *mem = malloc(sizeof(t_memory));
	mem->memory = calloc(memoryConfig->frameQty, sizeof(uint32_t));
	return mem;
}

bool (*memory_handlers[8])(t_packet *petition, int socket) =
{
	process_new,
	cpu_handshake,
	access_lvl1_table,
	access_lvl2_table,
	memory_read,
	memory_write,
	process_suspend,
	process_exit };

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
		usleep(memoryConfig->memoryDelay);
		serve = memory_handlers[packet->header](packet, client_socket);
		packet_destroy(packet);
	}
	return 0;
}

void terminate_memory(bool error) {
	log_destroy(logger);
	destroyMemoryConfig(memoryConfig);
	dictionary_destroy_and_destroy_elements(pageTables, page_table_destroy);

	if (server_socket)
		close(server_socket);
	exit(error ? EXIT_FAILURE : EXIT_SUCCESS);
}

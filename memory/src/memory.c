#include"memmory.h"

int main()
{
	// Initialize logger
	logger = log_create("./cfg/memory.log", "MEMORY", 1, LOG_LEVEL_TRACE);
	// Initialize Config
	memoryConfig = getMemoryConfig("./cfg/memory.config");
	// Initialize Metadata
	metadata = initializeMemoryMetadata(memoryConfig);

	// Creacion de server
	server_socket = create_server(memoryConfig->listenPort);
	log_info(logger, "Servidor de Memoria creado");

	t_list *swapFiles = list_create();
	// Initialize Variables
	memory = initializeMemory(memoryConfig);
	metadata->clock_m_counter = 0;
	pageTables = dictionary_create();
	algoritmo = strcmp(memoryConfig->replaceAlgorithm, "CLOCK") ? clock_alg : clock_m_alg;
	clock_m_counter = 0;

	while (1)
	{
		server_listen(server_socket, header_handler);
	}

	// Destroy
	destroyMemoryConfig(memoryConfig);
	dictionary_destroy_and_destroy_elements(pageTables, page_table_destroy);
	log_destroy(logger);

	return EXIT_SUCCESS;
}

//Ready
bool process_suspension(t_packet *petition, int console_socket)
{
	uint32_t PID = stream_take_UINT32(petition->payload);
	uint32_t pt1_entry = stream_take_UINT32(petition->payload);

	pthread_mutex_lock(&pageTablesMut);
		t_ptbr2 *pt2 = getPageTable2(PID, pt1_entry, pageTables);
		uint32_t pages = pt2->pageQuantity;

		//Swappear Pages
		for (uint32_t i = 0; i < pages; i++){
			if (((t_page_entry*)list_get(pt2->entries, i))->present){
				void *pageContent = (void*) memory_getFrame(memory, ((t_page_entry*)list_get(pt2->entries, i))->frame);
				savePage(PID, i, pageContent);
				pthread_mutex_lock(&metadataMut);
					metadata->entries[((t_page_entry*)list_get(pt2->entries, i))->frame].isFree = true;
				pthread_mutex_unlock(&metadataMut);
				((t_page_entry*)list_get(pt2->entries, i))->present = false;
			 }
		}

	//Cambiar metadata
	pthread_mutex_unlock(&pageTablesMut);
	if(metadata->firstFrame){
		pthread_mutex_lock(&metadataMut);
			for (uint32_t i = 0; i < memoryConfig->frameQty / memoryConfig->framesPerProcess; i++){
				 if(metadata->firstFrame[i] == PID) metadata->firstFrame[i] = -1;
			}
		pthread_mutex_unlock(&metadataMut);
	}

	//freeProcessTLBEntries(PID); TODO CPU

	return false;
 }


//READY
bool receive_pid(t_packet *petition, int kernel_socket)
{

	uint32_t pid = stream_take_UINT32(petition->payload);

	if (!!pid)
	{
		pthread_mutex_lock(&mutex_log);
		log_info(logger, "Memory: PID #%d Received",pid);
		pthread_mutex_unlock(&mutex_log);

		t_ptbr1 *newPageTable = initializePageTable();

		log_info(logger, "PID #%d", pid);
		char *pid_key = string_itoa(pid);

		pthread_mutex_lock(&pageTablesMut);
			dictionary_put(pageTables, pid_key, (void *)newPageTable);
		pthread_mutex_unlock(&pageTablesMut);

		t_packet* response;
		response = create_packet(ERROR, sizeof(uint32_t));
		stream_add_UINT32(response->payload, newPageTable->tableNumber);
		socket_send_packet(kernel_socket, response);
		packet_destroy(response);

		free(pid_key);
	}

	return false;
}

//READY
bool access_lvl1_table(t_packet *petition, int cpu_socket)
{
	uint32_t pid = stream_take_UINT32(petition->payload);
	uint32_t pt1_entry = stream_take_UINT32(petition->payload);

	if (!!pid)
	{
		t_ptbr2 *pt2 = getPageTable2(pid, pt1_entry, pageTables);
		t_packet* response = create_packet(FRAME, sizeof(uint32_t));
		stream_add_UINT32(response->payload, pt2->tableNumber);
		socket_send_packet(cpu_socket, response);
		packet_destroy(response);

	}
	return false;
}

//READY?
bool access_lvl2_table(t_packet *petition, int cpu_socket)
{
	uint32_t pid = stream_take_UINT32(petition->payload);
	uint32_t pt1_entry = stream_take_UINT32(petition->payload);
	uint32_t pt2_entry = stream_take_UINT32(petition->payload);
	uint32_t page = stream_take_UINT32(petition->payload);

	if (!!pid)
	{
		uint32_t frame = pageTable_getFrame(pid, pt1_entry, pt2_entry);

		if(frame == -1){
			uint32_t frameVictima = swapPage(pid, pt1_entry, pt2_entry, page);
			t_packet* response = create_packet(FRAME, sizeof(uint32_t));
			stream_add_UINT32(response->payload, frameVictima);
			socket_send_packet(cpu_socket, response);
			packet_destroy(response);
		} else {
			//esta presente y no esta libre
			t_packet* response = create_packet(FRAME, sizeof(uint32_t));
			stream_add_UINT32(response->payload, frame);
			socket_send_packet(cpu_socket, response);
			packet_destroy(response);

		}
	}

	return false;
}

// READY?
bool memory_write(t_packet *petition, int cpu_socket)
{
	uint32_t pid = stream_take_UINT32(petition->payload);
	uint32_t pt1_entry = stream_take_UINT32(petition->payload);
	uint32_t pt2_entry = stream_take_UINT32(petition->payload);
	uint32_t page = stream_take_UINT32(petition->payload);

	//cargar pagina
	uint32_t frameVictima = swapPage(pid, pt1_entry, pt2_entry, page);

	t_packet* response;
	if(frameVictima == -1) {
		response = create_packet(SWAP_ERROR, 0);
	} else {
		response = create_packet(SWAP_OK, sizeof(uint32_t));
	}

	stream_add_UINT32(response->payload, frameVictima);
	socket_send_packet(cpu_socket, response);
	packet_destroy(response);
	return false;
}

bool memory_read(t_packet *petition, int cpu_socket)
{
	uint32_t pid = stream_take_UINT32(petition->payload);
	uint32_t pt1_entry = stream_take_UINT32(petition->payload);
	uint32_t pt2_entry = stream_take_UINT32(petition->payload);
	uint32_t page = stream_take_UINT32(petition->payload);
	t_packet* response;

	if (!!pid)
	{
		uint32_t frame = pageTable_getFrame(pid, pt1_entry, pt2_entry);
		if(frame == -1 || isFree(frame)){
			response = create_packet(ERROR, 0);
			socket_send_packet(cpu_socket, response);
			packet_destroy(response);
			return false;
		}

		for (int i = 0; i < memoryConfig->swapDelay; i++) {
			usleep(1000);
		}

		readPage(pid, page);

		response = create_packet(ERROR, 0);
		socket_send_packet(cpu_socket, response);
		packet_destroy(response);
	}
	return false;
}

//TODO no se si falta o no intervenir en el t_memory
bool end_process(t_packet *petition, int cpu_socket)
{
    uint32_t PID = stream_take_UINT32(petition->payload);
    uint32_t pt1_entry = stream_take_UINT32(petition->payload);

    pthread_mutex_lock(&pageTablesMut);
        t_ptbr2 *pt2 = getPageTable2(PID, pt1_entry, pageTables);
        uint32_t pageQty = pt2->pageQuantity;
    pthread_mutex_unlock(&pageTablesMut);

    for (uint32_t i = pageQty - 1; i >= 0; i--){
    	// Borrar File de Swap
    	destroy_swap_page(PID, i);

    	// Cambiar Valores en Metadata
        pthread_mutex_lock(&pageTablesMut);
        if (((t_page_entry*)list_get(pt2->entries, i))->present == true){
            uint32_t frame = ((t_page_entry*)list_get(pt2->entries, i))->frame;
            pthread_mutex_lock(&metadataMut);
                (metadata->entries)[frame].isFree = true;
            pthread_mutex_unlock(&metadataMut);
        }
        pthread_mutex_unlock(&pageTablesMut);
    }

    char *_PID = string_itoa(PID);
    dictionary_remove_and_destroy(pageTables, _PID, _destroyPageTable);
    free(_PID);

    t_packet *response = create_packet(OK, 0);
    socket_send_packet(cpu_socket, response);
    packet_destroy(response);

    pthread_mutex_lock(&mutex_log);
        log_info(logger, "Process %u Ended", PID);
    pthread_mutex_unlock(&mutex_log);

    //sigUsr1HandlerTLB(0);

    //freeProcessEntries(PID);

    return true;
}

t_mem_metadata *initializeMemoryMetadata(t_memoryConfig *config)
{
	t_mem_metadata *metadata = malloc(sizeof(t_mem_metadata));
	metadata->entryQty = memoryConfig->frameQty;
	metadata->counter = 0;
	metadata->entries = calloc(metadata->entryQty, sizeof(t_frame_metadata));
	metadata->clock_m_counter = NULL;
	metadata->firstFrame = NULL;

	uint32_t blockQuantity = memoryConfig->frameQty / config->framesPerProcess;

	metadata->firstFrame = calloc(blockQuantity, sizeof(uint32_t));
	memset(metadata->firstFrame, -1, sizeof(uint32_t) * blockQuantity);

	metadata->clock_m_counter = calloc(blockQuantity, sizeof(uint32_t));

	for (int i = 0; i < blockQuantity; i++)
	{
		metadata->clock_m_counter[i] = i * config->framesPerProcess;
	}

	for (int i = 0; i < metadata->entryQty; i++)
	{
		((metadata->entries)[i]).isFree = true;
		((metadata->entries)[i]).timeStamp = 0;
	}

	return metadata;
}

void memory_metadata_destroy(t_mem_metadata *meta)
{
	if (meta->firstFrame)
	{
		free(meta->firstFrame);
		free(meta->clock_m_counter);
	}

	free(meta->entries);
	free(meta);
}

t_memory *initializeMemory(t_memoryConfig *config)
{
	t_memory *newMemory = malloc(sizeof(t_memory));
	newMemory->memory = calloc(1, config->memorySize);

	return newMemory;
}

bool (*memory_handlers[6])(t_packet *petition, int socket) =
{
	receive_pid,
	access_lvl1_table,
	access_lvl2_table,
	memory_read,
	process_suspension,
	memory_write,
	end_process
};

void *header_handler(void *_client_socket)
{
	int client_socket = (int)_client_socket;
	bool serve = true;
	while (serve)
	{
		t_packet *packet = socket_receive_packet(client_socket);
		if (packet == NULL)
		{
			if (!socket_retry_packet(client_socket, &packet))
			{
				close(client_socket);
				break;
			}
		}
		serve = memory_handlers[packet->header](packet, client_socket);
		packet_destroy(packet);
	}
	return 0;
}

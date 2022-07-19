#include "imemory.h"

int main()
{
	// Initialize logger
	logger = log_create("./cfg/memory-final.log", "MEMORY", 1, LOG_LEVEL_TRACE);
	config = getMemoryConfig("./cfg/memory.config");

	metadata = metadata_init();

	// Creacion de server
	server_socket = create_server(config->listenPort);

	pthread_mutex_lock(&mutex_log);
	log_info(logger, "Memory server ready");
	pthread_mutex_unlock(&mutex_log);

	level1_tables = list_create();
	level2_tables = list_create();

	swap_files = list_create();
	/* hay que ver como determinar swapfiles y filesize
	 agregar donde se swappean
	 list_add(swap_files, swapFile_create(config->swap_files[i], config->fileSize, config->pageSize));
	 */
	sem_init(&writeRead, 0, 2); // TODO Ver si estan bien los semaforos, o si van en otras funciones tmb

	memory = memory_init();
	metadata->clock_m_counter = 0;

	clock_m_counter = 0;

	cpu_handshake_listener();

	while (1)
	{
		server_listen(server_socket, header_handler);
	}

	terminate_memory(false);
}

bool (*memory_handlers[8])(t_packet *petition, int socket) =
	{
		// PROCESS_NEW
		process_new,
		NULL,
		// LVL1_TABLE
		access_lvl1_table,
		// LVL2_TABLE
		access_lvl2_table,
		// READ_CALL
		memory_read,
		// WRITE_CALL
		memory_write,
		// PROCESS_SUSPEND
		process_suspend,
		// PROCESS_EXIT
		process_exit};

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
		usleep(config->memoryDelay * 1000);

		memory_access_counter++;

		serve = memory_handlers[packet->header](packet, client_socket);
		packet_destroy(packet);
	}
	return 0;
}

void cpu_handshake_listener()
{
	int client_socket = accept_client(server_socket);
	bool serve = true;
	while (serve)
	{
		uint8_t header = socket_receive_header(client_socket);
		if (header == MEM_HANDSHAKE)
		{
			serve = cpu_handshake(client_socket);
		}
	}
}

bool cpu_handshake(int cpu_socket)
{
	pthread_mutex_lock(&mutex_log);
	log_info(logger, "Received handshake petition from CPU");
	pthread_mutex_unlock(&mutex_log);

	t_packet *mem_data = create_packet(TABLE_INFO_TO_CPU, INITIAL_STREAM_SIZE);
	stream_add_UINT32(mem_data->payload, config->pageSize);
	stream_add_UINT32(mem_data->payload, config->entriesPerTable);
	socket_send_packet(cpu_socket, mem_data);
	packet_destroy(mem_data);

	pthread_mutex_lock(&mutex_log);
	log_info(logger, "Relayed relevant memory data to CPU");
	pthread_mutex_unlock(&mutex_log);

	return false;
}

bool process_new(t_packet *petition, int kernel_socket)
{
	uint32_t pid = stream_take_UINT32(petition->payload);
	uint32_t size = stream_take_UINT32(petition->payload);

	if (pid != NULL)
	{
		pthread_mutex_lock(&mutex_log);
		log_info(logger, "Initializing memory structures for PID #%d", pid);
		pthread_mutex_unlock(&mutex_log);

		uint32_t pt1_index = (uint32_t)page_table_init(size);
		dictionary_put(clock_pointers_dictionary,string_itoa(pid),NULL);

		// char* swap_filename = swap_init(pid);

		t_packet *response = create_packet(PROCESS_MEMORY_READY,
										   INITIAL_STREAM_SIZE);

		stream_add_UINT32(response->payload, pid);
		stream_add_UINT32(response->payload, pt1_index);
		socket_send_packet(kernel_socket, response);

		packet_destroy(response);
	}

	return true;
}

bool process_suspend(t_packet *petition, int kernel_socket)
{
	uint32_t pid = stream_take_UINT32(petition->payload);
	uint32_t pt1_index = stream_take_UINT32(petition->payload);

	if (pid != NULL)
	{
		pthread_mutex_lock(&mutex_log);
		log_info(logger, "Process suspension requested for PID #%d", pid);
		pthread_mutex_unlock(&mutex_log);

		// TODO: Definir si se deben recorrer todas las entradas de la pt1 del proceso
		t_ptbr1 *pt1 = get_page_table1((int)pt1_index);

		// Se iteran las entries (indices a tablas nivel 2) de pt1 y se ejecuta por cada una:
		/*
		t_ptbr2 *pt2 = get_page_table2(pt2_index);
		int page_cant = list_size(pt2->entries);
		// Swappear Pages
		// No page_cant, usar list_iterate
		for (int i = 0; i < page_cant; i++)
		{
			t_page_entry *entry = (t_page_entry *)list_get(pt2->entries, i);
			// Se actualiza en "disco" unicamente si la pagina estaba en RAM y fue modificada
			if (entry->present && entry->modified)
			{
				int frame = entry->frame;
				void *pageContent = (void *)memory_getFrame(memory,
															frame);
				savePage(pid, i, pageContent);
				pthread_mutex_lock(&metadataMut);
				metadata->entries[frame].isFree =
					true;
				pthread_mutex_unlock(&metadataMut);
				entry->present = false;
			}
		}*/

		// TODO: Investigar que hace y si es necesario
		// Cambiar metadata
		if (metadata->firstFrame)
		{
			pthread_mutex_lock(&metadataMut);
			for (uint32_t i = 0;
				 i < config->framesInMemory / config->framesPerProcess;
				 i++)
			{
				if (metadata->firstFrame[i] == pid)
					metadata->firstFrame[i] = -1;
			}
			pthread_mutex_unlock(&metadataMut);
		}
	}

	return false;
}

bool process_exit(t_packet *petition, int kernel_socket)
{
	uint32_t pid = stream_take_UINT32(petition->payload);
	uint32_t pt1_index = stream_take_UINT32(petition->payload);

	if (pid != NULL)
	{
		pthread_mutex_lock(&mutex_log);
		log_info(logger, "Destroying PID #%d", pid);
		pthread_mutex_unlock(&mutex_log);

		printf("pt1 %d\n", pt1_index);
		// TODO: Definir si se deben recorrer todas las entradas de la pt1 del proceso
		t_ptbr1 *pt1 = get_page_table1((int)pt1_index);

		// Se iteran las entries (indices a tablas nivel 2) de pt1 y se ejecuta por cada una:
		/*
		t_ptbr2 *pt2 = get_page_table2(pt2_index);
		int page_cant = list_size(pt2->entries);
		// No page_cant, usar list_iterate
		for (int i = 0; i < page_cant; i++)
		{
			// Borrar swap
			bool error = destroy_swap_page(pid, i);

			t_packet *response_packet = create_packet(
				error ? SWAP_ERROR : SWAP_OK, 0);
			socket_send_packet(cpu_socket, response_packet);
			packet_destroy(response_packet);

			// Cambiar valores en metadata
			if (((t_page_entry *)list_get(pt2->entries, i))->present == true)
			{
				uint32_t frame =
					((t_page_entry *)list_get(pt2->entries, i))->frame;
				pthread_mutex_lock(&metadataMut);
				(metadata->entries)[frame].isFree = true;
				pthread_mutex_unlock(&metadataMut);
			}
		}*/

		/*t_packet *response = create_packet(OK, 0);
		 socket_send_packet(cpu_socket, response);
		 packet_destroy(response);*/

		pthread_mutex_lock(&mutex_log);
		log_info(logger, "Destroyed PID #%d successfully", pid);
		pthread_mutex_unlock(&mutex_log);

		t_packet *response = create_packet(PROCESS_EXIT_READY,
										   INITIAL_STREAM_SIZE);

		stream_add_UINT32(response->payload, pid);
		socket_send_packet(kernel_socket, response);

		packet_destroy(response);
	}

	return true;
}

// Recibe index de pt1 en lista global (guardado del pcb) y entrada de la tabla 1 a la que acceder
bool access_lvl1_table(t_packet *petition, int cpu_socket)
{
	uint32_t pt1_index = stream_take_UINT32(petition->payload);

	// CPU debe calcular:
	// page_number = floor(instruction_param_address / config->pageSize)
	// entry_index = floor(page_number / config->entriesPerTable)
	uint32_t entry_index = stream_take_UINT32(petition->payload);

	if (pt1_index != -1)
	{
		pthread_mutex_lock(&mutex_log);
		log_info(logger, "Getting level 2 table index");
		pthread_mutex_unlock(&mutex_log);

		uint32_t pt2_index = (uint32_t)get_page_table2_index(pt1_index, entry_index);
		t_packet *response = create_packet(TABLE2_TO_CPU, INITIAL_STREAM_SIZE);
		stream_add_UINT32(response->payload, pt2_index);

		socket_send_packet(cpu_socket, response);
		packet_destroy(response);
	}

	return false;
}

// Recibe index de pt2 en lista global y entrada de la tabla 2 a la que acceder
bool access_lvl2_table(t_packet *petition, int cpu_socket)
{
	uint32_t pt2_index = stream_take_UINT32(petition->payload);

	// CPU debe calcular:
	// page_number = floor(instruction_param_address / config->pageSize)
	// entry_index =  page_number % config->entriesPerTable
	uint32_t entry_index = stream_take_UINT32(petition->payload);
	uint32_t pid = stream_take_UINT32(petition->payload);

	if (pt2_index != -1)
	{
		pthread_mutex_lock(&mutex_log);
		log_info(logger, "Getting frame number");
		pthread_mutex_unlock(&mutex_log);

		uint32_t frame_num = (uint32_t)get_frame_number(pt2_index, entry_index,pid);

		t_packet *response = create_packet(FRAME_TO_CPU,
										   INITIAL_STREAM_SIZE);
		stream_add_UINT32(response->payload, frame_num);
		socket_send_packet(cpu_socket, response);
		packet_destroy(response);
	}

	return false;
}

bool memory_write(t_packet *petition, int cpu_socket)
{
	memory_write_counter++;

	sem_wait(&writeRead);

	uint32_t dir = stream_take_UINT32(petition->payload);
	uint32_t toWrite = stream_take_UINT32(petition->payload);
	/*
		if (!!toWrite)
		{

			pthread_mutex_lock(&mutex_log);
			log_info(logger, "Memory Write Request");
			pthread_mutex_unlock(&mutex_log);

			// Cargar página
			uint32_t frameVictima = swapPage(pid, pt1_entry, pt2_entry, page);

			t_packet *response;
			if (frameVictima == -1)
			{
				response = create_packet(SWAP_ERROR, 0);
			}
			else
			{
				response = create_packet(SWAP_OK, INITIAL_STREAM_SIZE);
			}

			stream_add_UINT32(response->payload, frameVictima);
			socket_send_packet(cpu_socket, response);
			packet_destroy(response);
		}
	 */
	sem_post(&writeRead);

	return false;
}

bool memory_read(t_packet *petition, int cpu_socket)
{
	memory_read_counter++;

	sem_wait(&writeRead);

	uint32_t dir = stream_take_UINT32(petition->payload);

	pthread_mutex_lock(&mutex_log);
	log_info(logger, "Reading Memory");
	pthread_mutex_unlock(&mutex_log);

	sem_post(&writeRead);

	return false;
}

t_memory *memory_init()
{
	t_memory *mem = malloc(sizeof(t_memory));
	mem->memory = calloc(config->framesInMemory, sizeof(uint32_t));
	return mem;
}

// TODO: Checkear que sirva/tenga sentido
t_mem_metadata *metadata_init()
{
	t_mem_metadata *metadata = malloc(sizeof(t_mem_metadata));
	metadata->entryQty = config->framesInMemory;
	metadata->clock_counter = 0;
	metadata->entries = calloc(metadata->entryQty, sizeof(t_frame_metadata));
	metadata->clock_m_counter = NULL;
	metadata->firstFrame = NULL;

	uint32_t blockQuantity = config->framesInMemory / config->framesPerProcess;

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

void metadata_destroy(t_mem_metadata *meta)
{
	if (meta->firstFrame)
	{
		free(meta->firstFrame);
		free(meta->clock_m_counter);
	}

	free(meta->entries);
	free(meta);
}
//

void terminate_memory(bool error)
{
	pthread_mutex_lock(&mutex_log);
	log_info(logger, "Memory accesses: %d", memory_access_counter);
	log_info(logger, "Memory reads: %d", memory_read_counter);
	log_info(logger, "Memory writes: %d", memory_write_counter);
	/*log_info(logger, "Page Assignments: %d", page_assignment_counter);
	log_info(logger, "Page Replacements: %d", page_replacement_counter);
	log_info(logger, "Page Faults: %d", page_faults_counter);*/
	pthread_mutex_unlock(&mutex_log);

	log_destroy(logger);
	destroyMemoryConfig(config);

	if (server_socket)
		close(server_socket);
	exit(error ? EXIT_FAILURE : EXIT_SUCCESS);
}

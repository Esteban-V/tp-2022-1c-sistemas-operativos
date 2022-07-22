#include "imemory.h"

int main()
{
	// Initialize logger
	logger = log_create("./cfg/memory-final.log", "MEMORY", 1, LOG_LEVEL_TRACE);
	config = getMemoryConfig("./cfg/memory.config");

	if (!strcmp(config->replaceAlgorithm, "CLOCK"))
	{
		replaceAlgorithm = CLOCK;
	}
	else if (!strcmp(config->replaceAlgorithm, "CLOCK-M"))
	{
		replaceAlgorithm = CLOCK_M;
	}
	else
	{
		log_warning(logger,
					"Wrong replace algorithm set in config --> Using CLOCK");
	}

	// Creacion de server
	server_socket = create_server(config->listenPort);

	pthread_mutex_lock(&mutex_log);
	log_info(logger, "Memory Server Ready");
	pthread_mutex_unlock(&mutex_log);

	level1_tables = list_create();
	level2_tables = list_create();

	swap_files = list_create();
	process_frames = list_create();

	sem_init(&writeRead, 0, 2); // TODO Ver si estan bien los semaforos, o si van en otras funciones tmb

	memory = memory_init();

	clock_m_counter = 0;

	cpu_handshake_listener();

	// TODO hilo de swap

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
		usleep(config->memoryDelay);

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
	log_info(logger, "Received Handshake Petition from CPU");
	pthread_mutex_unlock(&mutex_log);

	t_packet *mem_data = create_packet(TABLE_INFO_TO_CPU, INITIAL_STREAM_SIZE);
	stream_add_UINT32(mem_data->payload, config->pageSize);
	stream_add_UINT32(mem_data->payload, config->entriesPerTable);
	socket_send_packet(cpu_socket, mem_data);
	packet_destroy(mem_data);

	pthread_mutex_lock(&mutex_log);
	log_info(logger, "Relayed Relevant Memory Data to CPU");
	pthread_mutex_unlock(&mutex_log);

	return false;
}

bool process_new(t_packet *petition, int kernel_socket)
{
	uint32_t pid = stream_take_UINT32(petition->payload);
	uint32_t process_size = stream_take_UINT32(petition->payload);

	if (pid != NULL)
	{
		pthread_mutex_lock(&mutex_log);
		log_info(logger, "Initializing Memory Structures for PID #%d", pid);
		pthread_mutex_unlock(&mutex_log);

		int pt1_index, process_frames_index;
		page_table_init(process_size, &pt1_index, &process_frames_index);
		
		list_add(swap_files, swapFile_create(pid, process_size));

		t_packet *response = create_packet(PROCESS_MEMORY_READY, INITIAL_STREAM_SIZE);

		stream_add_UINT32(response->payload, pid);
		stream_add_UINT32(response->payload, (uint32_t)pt1_index);
		stream_add_UINT32(response->payload, (uint32_t)process_frames_index);
		socket_send_packet(kernel_socket, response);

		packet_destroy(response);
	}

	return true;
}

bool process_suspend(t_packet *petition, int kernel_socket)
{
	uint32_t pid = stream_take_UINT32(petition->payload);
	uint32_t pt1_index = stream_take_UINT32(petition->payload);
	uint32_t process_frames_index = stream_take_UINT32(petition->payload);

	if (pid != NULL)
	{
		pthread_mutex_lock(&mutex_log);
		log_info(logger, "Process Suspension Requested for PID #%d", pid);
		pthread_mutex_unlock(&mutex_log);

		t_ptbr1 * pt1 = get_page_table1(pt1_index);

		// Se iteran las entries (indices a tablas nivel 2) de pt1 y se ejecuta por cada una:
		// t_ptbr2 *pt2 = get_page_table2(pt2_index);

		// Swappear Pages
		void _swap(void *elem){
			t_page_entry *entry = (t_page_entry *) elem;

			// Se actualiza en "disco" unicamente si la pagina estaba en RAM y fue modificada
			if (entry->present && entry->modified)
			{
				int frame = entry->frame;
				void *pageContent = (void *)memory_getFrame(frame);

				// savePage(pid, pageNumber, pageContent);

				// TODO settear frame en free en process_frames (o en las tablas globales, nidea)
				pthread_mutex_lock(&metadataMut);
				//metadata->entries[frame].isFree = true;
				pthread_mutex_unlock(&metadataMut);
				entry->present = false;
			}
		}

		// list_iterate(pt2->entries, _swap);

		}



	return false;
}

bool process_exit(t_packet *petition, int kernel_socket)
{
	uint32_t pid = stream_take_UINT32(petition->payload);
	uint32_t pt1_index = stream_take_UINT32(petition->payload);
	uint32_t process_frames_index = stream_take_UINT32(petition->payload);

	if (pid != NULL)
	{
		pthread_mutex_lock(&mutex_log);
		log_info(logger, "Destroying PID #%d Memory Structures", pid);
		pthread_mutex_unlock(&mutex_log);

		// TODO: Definir si se deben recorrer todas las entradas de la pt1 del proceso
		t_ptbr1 *pt1 = get_page_table1((int)pt1_index);

		// Se iteran las entries (indices a tablas nivel 2) de pt1 y se ejecuta por cada una:
		//t_ptbr2 *pt2 = get_page_table2(pt2_index);
		//int page_cant = list_size(pt2->entries);
		// No page_cant, usar list_iterate
		/*for (int i = 0; i < page_cant; i++)
		{
			// Borrar swap
			bool error = destroy_swap_page(pid, i);

			t_packet *response_packet = create_packet(
				error ? SWAP_ERROR : SWAP_OK, 0);
			socket_send_packet(kernel_socket, response_packet);
			packet_destroy(response_packet);

			// Cambiar valores
			if (((t_page_entry *)list_get(pt2->entries, i))->present == true)
			{
				uint32_t frame = ((t_page_entry *)list_get(pt2->entries, i))->frame;

				pthread_mutex_lock(&metadataMut); // TODO cambiar en process_frames (o en las tablas globales, nidea)
				(metadata->entries)[frame].isFree = true;
				pthread_mutex_unlock(&metadataMut);
			}
		}*/

		pthread_mutex_lock(&mutex_log);
		log_info(logger, "Memory Structures Destroyed Successfully");
		pthread_mutex_unlock(&mutex_log);

		t_packet *response = create_packet(PROCESS_EXIT_READY, INITIAL_STREAM_SIZE);

		stream_add_UINT32(response->payload, pid);
		socket_send_packet(kernel_socket, response);

		packet_destroy(response);
	}

	return true;
}

void *create_swap(uint32_t pid, int frame_index, size_t psize)
{
	pthread_mutex_lock(&mutex_log);
	log_info(logger, "PID #%d - Creating Swap File - Size: %d", pid, psize);
	pthread_mutex_lock(&mutex_log);

	char *swap_file_path = string_from_format("%s/%d.swap", config->swapPath, pid);

	uint32_t frame_start = frame_index * sizeof(uint32_t);
	uint32_t frame_end = frame_start + config->pageSize;

	// Crear archivo
	usleep(config->swapDelay);

	int swap_file = open(swap_file_path, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
	if (swap_file == -1)
	{
		pthread_mutex_lock(&mutex_log);
		log_error(logger, "Error al abrir el archivo swap: %s", strerror(errno));
		pthread_mutex_unlock(&mutex_log);
	}

    if (ftruncate(swap_file, psize) == -1) {
		pthread_mutex_lock(&mutex_log);
        log_error(logger, "Error al truncar el archivo swap: %s", strerror(errno));
		pthread_mutex_unlock(&mutex_log);
    }

	free(swap_file_path);
}

void *delete_swap(uint32_t pid, int psize)
{
	pthread_mutex_lock(&mutex_log);
	log_info(logger, "PID #%d - Deleting Swap File", pid);
	pthread_mutex_unlock(&mutex_log);

	char *swap_file_path = string_from_format("%s/%d.swap", config->swapPath, pid);

	uint32_t pages = psize / config->pageSize;
	if (psize % config->pageSize != 0)
		pages++;

	uint32_t nro_tablas_segundo_nivel = pages / config->entriesPerTable;
	if (pages % config->entriesPerTable != 0)
		nro_tablas_segundo_nivel++;

	usleep(config->swapDelay);
	remove(swap_file_path);
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
		log_info(logger, "Getting Level 2 Table Index");
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
	// CPU debe calcular:
	// page_number = floor(instruction_param_address / config->pageSize)
	// entry_index =  page_number % config->entriesPerTable

	uint32_t pt2_index = stream_take_UINT32(petition->payload);
	uint32_t entry_index = stream_take_UINT32(petition->payload);
	uint32_t pid = stream_take_UINT32(petition->payload);
	uint32_t framesIndex = stream_take_UINT32(petition->payload);

	if (pt2_index != -1)
	{
		pthread_mutex_lock(&mutex_log);
		log_info(logger, "Getting Frame Number");
		pthread_mutex_unlock(&mutex_log);

		uint32_t frame_num = (uint32_t)get_frame_number(pt2_index, entry_index, pid,framesIndex);

		t_packet *response = create_packet(FRAME_TO_CPU, INITIAL_STREAM_SIZE);
		stream_add_UINT32(response->payload, frame_num);
		socket_send_packet(cpu_socket, response);
		packet_destroy(response);
	}

	return false;
}

bool memory_write(t_packet *petition, int cpu_socket)
{
	// Q corno es cada cosa????
	uint32_t dir = stream_take_UINT32(petition->payload);
	uint32_t toWrite = stream_take_UINT32(petition->payload);

	if (!!toWrite)
	{
		memory_write_counter++;
		sem_wait(&writeRead);

		pthread_mutex_lock(&mutex_log);
		log_info(logger, "Memory Write Request");
		pthread_mutex_unlock(&mutex_log);

		// Cargar página
		uint32_t frameVictima; // = replace(frameVictima, PID, pt2_index, page, cpu_socket);
		//writeFrame(PID, victim_frame, pageFromSwap);

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

		sem_post(&writeRead);
	}

	return false;
}

bool memory_read(t_packet *petition, int cpu_socket)
{
	uint32_t dir = stream_take_UINT32(petition->payload);

	if(!!dir) {
		memory_read_counter++;
		sem_wait(&writeRead);

		pthread_mutex_lock(&mutex_log);
		log_info(logger, "Reading Memory");
		pthread_mutex_unlock(&mutex_log);

		usleep(config->memoryDelay);

		//void *pageFromSwap = readPage(PID, page);

		sem_post(&writeRead);
	}

	return false;
}

t_memory *memory_init()
{
	int cant_frames = config->framesInMemory;

	// Crea espacio de memoria contiguo
	t_memory *mem = malloc(sizeof(t_memory));
	mem->memory = calloc(cant_frames, sizeof(uint32_t));

	// Crea bitmap de frames libres/ocupados
	void *ptr = malloc(cant_frames);
	frames_bitmap = bitarray_create_with_mode(ptr, ceil_div(cant_frames, 8), LSB_FIRST);
	msync(frames_bitmap->bitarray, cant_frames, MS_SYNC);

	for (int i = 0; i < cant_frames; i++)
	{
		// Inicializa todos los frames como vacios/no asignados
		bitarray_clean_bit(frames_bitmap, i);
	}

	return mem;
}

void terminate_memory(bool error)
{
	pthread_mutex_lock(&mutex_log);
	log_info(logger, "Memory Accesses: %d", memory_access_counter);
	log_info(logger, "Memory Reads: %d", memory_read_counter);
	log_info(logger, "Memory Writes: %d", memory_write_counter);
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

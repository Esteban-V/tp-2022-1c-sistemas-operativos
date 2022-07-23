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

	sem_init(&writeRead, 0, 1); // TODO Ver si estan bien los semaforos, o si van en otras funciones tmb

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

		memory_access_counter++;

		usleep(config->memoryDelay * 1000);
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

bool process_new(t_packet *petition, int kernel_socket) // Listo
{
	uint32_t pid = stream_take_UINT32(petition->payload);
	uint32_t process_size = stream_take_UINT32(petition->payload);

	if (pid != NULL)
	{
		pthread_mutex_lock(&mutex_log);
		log_info(logger, "Initializing Memory Structures for PID #%d", pid);
		pthread_mutex_unlock(&mutex_log);

		int pt1_index = page_table_init(process_size);
		int process_frames_index = assign_process_frames();

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
	uint32_t pt2_index = stream_take_UINT32(petition->payload);

	if (pid != NULL)
	{
		pthread_mutex_lock(&mutex_log);
		log_info(logger, "Process suspension requested for PID #%d", pid);
		pthread_mutex_unlock(&mutex_log);

		t_ptbr1 *pt1 = get_page_table1((int)pt1_index);

		for (int i = 0; i < list_size(pt1->entries); i++)
		{
			int pt2_index = *((int *)list_get(pt1->entries, i));
			t_ptbr2 *pt2 = get_page_table2(pt2_index);

			for (int j = 0; j < list_size(pt2->entries); j++)
			{
				t_page_entry *entry = (t_page_entry *)list_get(pt2->entries, j);

				// Se actualiza en "disco" unicamente si la pagina estaba en RAM y fue modificada
				if (entry->present && entry->modified)
				{
					int frame = entry->frame;
					void *pageContent = (void *)memory_getFrame(memory, frame);
					savePage(pid, j, pageContent);

					/*pthread_mutex_lock(&metadataMut);
					metadata->entries[frame].isFree =
						true;
					pthread_mutex_unlock(&metadataMut);*/

					entry->present = false;
				}
			}
		}
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
		log_info(logger, "Destroying PID #%d memory structures", pid);
		pthread_mutex_unlock(&mutex_log);

		t_ptbr1 *pt1 = get_page_table1((int)pt1_index);

		void _destroy_pt2(void *elem)
		{
			int pt2_index = *((int *)elem);
			t_ptbr2 *pt2 = get_page_table2(pt2_index);
			int page_cant = list_size(pt2->entries);

			// Borrar swap
			for (int i = 0; i < page_cant; i++)
			{
				bool error = destroy_swap_page(pid, i);

				// Cambiar valores
				if (((t_page_entry *)list_get(pt2->entries, i))->present == true)
				{
					uint32_t frame = ((t_page_entry *)list_get(pt2->entries, i))->frame;
				}
			}
		}

		list_iterate(pt1->entries, _destroy_pt2);

		pthread_mutex_lock(&mutex_log);
		log_info(logger, "Memory structures destroyed successfully");
		pthread_mutex_unlock(&mutex_log);

		t_packet *response = create_packet(PROCESS_EXIT_READY, INITIAL_STREAM_SIZE);

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
	uint32_t pid = stream_take_UINT32(petition->payload);
	uint32_t pt2_index = stream_take_UINT32(petition->payload);

	// CPU debe calcular:
	// page_number = floor(instruction_param_address / config->pageSize)
	// entry_index =  page_number % config->entriesPerTable

	uint32_t entry_index = stream_take_UINT32(petition->payload);

	uint32_t process_frames_index = stream_take_UINT32(petition->payload);

	if (pt2_index != -1)
	{
		int frame = get_frame_number(pt2_index, entry_index, pid, process_frames_index);

		// Lo tendria que hacer adentro de get_frame_number cuando se *confirma* que hay que reemplazar
		/*
		// Traer contenido de pagina pedida de swap.
		void *pageFromSwap = read_swap_page(pid, pageNumber);

		// Chequear que se haya podido traer
		if (pageFromSwap == NULL)
		{
			log_error(logger, "Cannot load page #%u", pageNumber, pid);

			return -1;
		}

		// Escribir pagina traida de swap a memoria.
		writeFrame(pid, victim_frame, pageFromSwap);
		free(pageFromSwap);

		// Modificar tabla de paginas del proceso cuya pagina entra a memoria.
		t_ptbr2 *ptReemplaza = get_page_table2(pt2_index);
		((t_page_entry *)list_get(ptReemplaza->entries, pt2_index))->present = true;
		((t_page_entry *)list_get(ptReemplaza->entries, pt2_index))->frame = victim_frame;

		// add_tlb_entry(PID, page, victim); TODO pasar a CPU
		t_packet *add_tlb_entry = create_packet(TLB_ADD, INITIAL_STREAM_SIZE);
		stream_add_UINT32(add_tlb_entry->payload, pid);
		stream_add_UINT32(add_tlb_entry->payload, pageNumber);
		stream_add_UINT32(add_tlb_entry->payload, victim_frame);
		socket_send_packet(cpu_socket, add_tlb_entry);
		packet_destroy(add_tlb_entry);

		// Modificar frame "metadata"
		pthread_mutex_lock(&metadataMut);					 // TODO cambiar en process_frames (o en las tablas globales, nidea), quitar metadata
		(metadata->entries)[victim_frame].page = pageNumber; // process frames y page entry

		(metadata->entries)[victim_frame].modified = false; // page entry
		(metadata->entries)[victim_frame].u = true;			// page entry

		(metadata->entries)[victim_frame].isFree = false; // bitmap
		pthread_mutex_unlock(&metadataMut);

		t_packet *drop_tlb_entry = create_packet(FRAME_TO_CPU, INITIAL_STREAM_SIZE);
		stream_add_UINT32(drop_tlb_entry->payload, victim_frame);
		socket_send_packet(cpu_socket, drop_tlb_entry);
		packet_destroy(drop_tlb_entry);
		*/
	}

	return false;
}

bool memory_write(t_packet *petition, int cpu_socket)
{
	uint32_t address = stream_take_UINT32(petition->payload);
	uint32_t value = stream_take_UINT32(petition->payload);

	if (address != NULL)
	{
		memory_write_counter++;
		sem_wait(&writeRead);

		pthread_mutex_lock(&mutex_log);
		log_info(logger, "Memory write request");
		pthread_mutex_unlock(&mutex_log);

		// Write
		sem_post(&writeRead);
	}

	return false;
}

bool memory_read(t_packet *petition, int cpu_socket)
{
	uint32_t address = stream_take_UINT32(petition->payload);

	if (!!address)
	{
		memory_read_counter++;
		sem_wait(&writeRead);

		pthread_mutex_lock(&mutex_log);
		log_info(logger, "Memory read request");
		pthread_mutex_unlock(&mutex_log);

		// Read
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

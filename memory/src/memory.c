#include "imemory.h"

int main()
{
	signal(SIGINT, terminate_memory);
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
		replaceAlgorithm = CLOCK;
		log_warning(logger, "Wrong replace algorithm set in config --> Using CLOCK");
	}

	// Creacion de server
	server_socket = create_server(config->listenPort);

	pthread_mutex_lock(&mutex_log);
	log_info(logger, "Memory server ready");
	pthread_mutex_unlock(&mutex_log);

	level1_tables = list_create();
	level2_tables = list_create();

	global_frames = list_create();

	sem_init(&writeRead, 0, 1);

	memory = memory_init();

	clock_m_counter = 0;
	// Stats
	memory_access_counter = 0;
	memory_read_counter = 0;
	memory_write_counter = 0;
	page_fault_counter = 0;
	page_assignment_counter = 0;
	page_replacement_counter = 0;

	int cpu_client_socket = cpu_handshake_listener();

	pthread_create(&cpuThread, NULL, packet_handler, cpu_client_socket);
	pthread_detach(cpuThread);

	while (1)
	{
		server_listen(server_socket, packet_handler);
	}
}

bool (*memory_handlers[8])(t_packet *petition, int socket) =
	{
		// PROCESS_NEW
		process_new,
		NULL, // Handshake
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
		// PROCESS_UNSUSPEND
		process_unsuspend,
		// PROCESS_EXIT
		process_exit};

void *packet_handler(void *_client_socket)
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

int cpu_handshake_listener()
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
	return client_socket;
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

bool process_new(t_packet *petition, int kernel_socket) // Listo
{
	uint32_t pid = stream_take_UINT32(petition->payload);
	uint32_t process_size = stream_take_UINT32(petition->payload);

	if (pid != NULL)
	{
		pthread_mutex_lock(&mutex_log);
		log_info(logger, "Initializing memory structures for PID #%d", pid);
		pthread_mutex_unlock(&mutex_log);

		int pt1_index = page_table_init(process_size);
		int process_frames_index = assign_process_frames();

		create_swap(pid, process_size);

		t_packet *response = create_packet(PROCESS_MEMORY_READY, INITIAL_STREAM_SIZE);

		stream_add_UINT32(response->payload, pid);
		stream_add_UINT32(response->payload, (uint32_t)pt1_index);
		stream_add_UINT32(response->payload, (uint32_t)process_frames_index);
		socket_send_packet(kernel_socket, response);

		packet_destroy(response);
	}

	return true;
}

bool process_unsuspend(t_packet *petition, int kernel_socket)
{
	uint32_t pid = stream_take_UINT32(petition->payload);

	if (pid != NULL)
	{
		pthread_mutex_lock(&mutex_log);
		log_info(logger, "Resuming process #%d", pid);
		pthread_mutex_unlock(&mutex_log);

		// TODO: assign_process_frames y get_swap

		t_packet *unsuspend_response = create_packet(PROCESS_SUSPENSION_READY, INITIAL_STREAM_SIZE);
		stream_add_UINT32(unsuspend_response->payload, pid);
		if (kernel_socket != -1)
		{
			socket_send_packet(kernel_socket, unsuspend_response);
		}
		packet_destroy(unsuspend_response);
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
		log_info(logger, "Process suspension requested for PID #%d", pid);
		pthread_mutex_unlock(&mutex_log);

		t_ptbr1 *pt1 = get_page_table1((int)pt1_index);

		for (int i = 0; i < list_size(pt1->entries); i++)
		{
			int pt2_index = (int *)list_get(pt1->entries, i);
			t_ptbr2 *pt2 = get_page_table2(pt2_index);

			for (int j = 0; j < list_size(pt2->entries); j++)
			{
				t_page_entry *entry = (t_page_entry *)list_get(pt2->entries, j);

				// Se actualiza en "disco" unicamente si la pagina estaba en RAM y fue modificada
				if (entry->present && entry->modified)
				{
					// Actualiza la pagina en el swap
					save_swap(entry->frame, entry->page, pid);
					entry->modified = false;
				}

				entry->present = false;
				entry->frame = -1;

				// Liberar los frames asignados en memoria
				unassign_process_frames((int)process_frames_index);
			}
		}

		t_packet *suspend_response = create_packet(PROCESS_SUSPENSION_READY, INITIAL_STREAM_SIZE);
		stream_add_UINT32(suspend_response->payload, pid);
		if (kernel_socket != -1)
		{
			socket_send_packet(kernel_socket, suspend_response);
		}
		packet_destroy(suspend_response);
	}

	return true;
}

bool process_exit(t_packet *petition, int kernel_socket)
{
	/* void _free_frames(void *elem)
	{
		int pt2_index = *((int *)elem);
		t_ptbr2 *pt2 = get_page_table2(pt2_index);
		int page_cant = list_size(pt2->entries);

		for (int i = 0; i < page_cant; i++)
		{
			t_page_entry *entry = (t_page_entry *)list_get(pt2->entries, i);
			// Cambiar valores
			if (entry->present == true)
			{
				uint32_t frame = entry->frame;
			}
		}
	}; */

	uint32_t pid = stream_take_UINT32(petition->payload);
	uint32_t pt1_index = stream_take_UINT32(petition->payload);
	uint32_t process_frames_index = stream_take_UINT32(petition->payload);

	if (pid != NULL)
	{
		pthread_mutex_lock(&mutex_log);
		log_warning(logger, "Destroying process #%d memory structures", pid);
		pthread_mutex_unlock(&mutex_log);

		t_ptbr1 *pt1 = get_page_table1((int)pt1_index);
		// list_iterate(pt1->entries, _free_frames);

		unassign_process_frames((int)process_frames_index);

		delete_swap(pid);

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
	uint32_t entry_index = stream_take_UINT32(petition->payload);

	if (pt1_index != -1)
	{
		uint32_t pt2_index = (uint32_t)get_page_table2_index(pt1_index, entry_index);
		t_packet *response = create_packet(TABLE2_TO_CPU, INITIAL_STREAM_SIZE);
		stream_add_UINT32(response->payload, pt2_index);

		socket_send_packet(cpu_socket, response);
		packet_destroy(response);
	}

	return true;
}

// Recibe index de pt2 en lista global y entrada de la tabla 2 a la que acceder
bool access_lvl2_table(t_packet *petition, int cpu_socket)
{
	uint32_t pid = stream_take_UINT32(petition->payload);
	uint32_t pt2_index = stream_take_UINT32(petition->payload);
	uint32_t entry_index = stream_take_UINT32(petition->payload);

	uint32_t process_frames_index = stream_take_UINT32(petition->payload);

	if (pt2_index != -1)
	{
		int frame = get_frame_number((int)pt2_index, (int)entry_index, (int)pid, (int)process_frames_index);

		t_packet *response = create_packet(FRAME_TO_CPU, INITIAL_STREAM_SIZE);
		stream_add_UINT32(response->payload, frame);

		socket_send_packet(cpu_socket, response);
		packet_destroy(response);
	}

	return true;
}

bool memory_write(t_packet *petition, int cpu_socket)
{
	uint32_t frame = stream_take_UINT32(petition->payload);
	uint32_t offset = stream_take_UINT32(petition->payload);
	uint32_t value = stream_take_UINT32(petition->payload);
	uint32_t frames_index = stream_take_UINT32(petition->payload);

	if (frame != NULL)
	{
		memory_write_counter++;
		sem_wait(&writeRead);

		pthread_mutex_lock(&mutex_log);
		log_info(logger, "Memory write request");
		pthread_mutex_unlock(&mutex_log);

		void *frame_ptr = get_frame(frame);
		write_frame_value(frame_ptr, offset, value);

		set_page_bits((int)frames_index, (int)frame, false);
		// used = true / modified = true

		pthread_mutex_lock(&mutex_log);
		log_info(logger, "Memory wrote at frame %d + offset %d / value %d", frame, offset, value);
		pthread_mutex_unlock(&mutex_log);

		sem_post(&writeRead);
	}

	return true;
}

bool memory_read(t_packet *petition, int cpu_socket)
{
	uint32_t frame = stream_take_UINT32(petition->payload);
	uint32_t offset = stream_take_UINT32(petition->payload);
	uint32_t frames_index = stream_take_UINT32(petition->payload);

	if (frame != NULL)
	{
		memory_read_counter++;
		sem_wait(&writeRead);

		pthread_mutex_lock(&mutex_log);
		log_info(logger, "Memory read request");
		pthread_mutex_unlock(&mutex_log);

		void *frame_ptr = get_frame(frame);
		uint32_t value = read_frame_value(frame_ptr, offset);

		set_page_bits((int)frames_index, (int)frame, false);
		// used = true

		pthread_mutex_lock(&mutex_log);
		log_info(logger, "Memory read at frame %d + offset %d", frame, offset);
		pthread_mutex_unlock(&mutex_log);

		t_packet *response = create_packet(VALUE_TO_CPU, INITIAL_STREAM_SIZE);
		stream_add_UINT32(response->payload, value);

		socket_send_packet(cpu_socket, response);
		packet_destroy(response);

		sem_post(&writeRead);
	}

	return true;
}

t_memory *memory_init()
{
	int cant_frames = config->framesInMemory;

	// Crea espacio de memoria contiguo
	t_memory *mem = malloc(sizeof(t_memory));
	mem->memory = malloc(config->memorySize);
	memset(mem->memory, 0, config->memorySize);

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

void stats()
{
	pthread_mutex_lock(&mutex_log);
	log_info(logger, "- - - Stats - - -");
	log_info(logger, "Memory accesses: %d", memory_access_counter);
	log_info(logger, "Memory reads: %d", memory_read_counter);
	log_info(logger, "Memory writes: %d", memory_write_counter);
	log_info(logger, "Page assignments: %d", page_assignment_counter);
	log_info(logger, "Page replacements: %d", page_replacement_counter);
	log_info(logger, "Page faults: %d", page_fault_counter);
	pthread_mutex_unlock(&mutex_log);
}

void terminate_memory(int x)
{
	free(memory);
	bitarray_destroy(frames_bitmap);
	list_destroy(level1_tables);
	list_destroy(level2_tables);
	list_destroy(global_frames);
	destroyMemoryConfig(config);

	if (server_socket)
		close(server_socket);

	switch (x)
	{
	case 1:
		log_destroy(logger);
		exit(EXIT_FAILURE);
	case SIGINT:
		stats();
		log_destroy(logger);
		exit(EXIT_SUCCESS);
	}
}

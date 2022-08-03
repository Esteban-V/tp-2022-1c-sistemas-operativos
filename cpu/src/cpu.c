#include "cpu.h"

int main()
{

	signal(SIGINT, terminate_cpu);

	// Initialize logger
	logger = log_create("./cfg/cpu-final.log", "CPU", 1, LOG_LEVEL_TRACE);
	config = get_cpu_config("./cfg/cpu.config");

	// Creacion de server
	kernel_dispatch_socket = create_server(config->dispatchListenPort);
	kernel_interrupt_socket = create_server(config->interruptListenPort);

	pthread_mutex_init(&mutex_kernel_socket, NULL);

	pthread_mutex_lock(&mutex_log);
	log_info(logger, "CPU server ready for kernel");
	pthread_mutex_unlock(&mutex_log);

	sem_init(&pcb_loaded, 0, 0);
	pthread_mutex_init(&mutex_has_interruption, NULL);
	sem_init(&value_loaded, 0, 0);
	pthread_mutex_init(&mutex_value, NULL);

	new_interruption = false;
	read_value = NULL;

	memory_server_socket = connect_to(config->memoryIP, config->memoryPort);

	if (memory_server_socket == -1)
	{
		terminate_cpu(true);
	}

	// pthread_create(&memoryThread, 0, get_read_value, NULL);
	// pthread_detach(memoryThread);

	// Handshake con memoria
	memory_handshake();
	tlb = create_tlb();

	pthread_create(&interruptionThread, 0, listen_interruption, NULL);
	pthread_detach(interruptionThread);

	pthread_create(&execThread, 0, cpu_cycle, NULL);
	pthread_detach(execThread);

	while (1)
	{
		server_listen(kernel_dispatch_socket, dispatch_header_handler);
	}
}

void *get_read_value()
{
	while (1)
	{
		server_listen(memory_server_socket, packet_handler);
	}
}

void *listen_interruption()
{
	while (1)
	{
		server_listen(kernel_interrupt_socket, header_handler);
	}
}

void pcb_to_kernel(kernel_headers header)
{
	t_packet *pcb_packet = create_packet(header, INITIAL_STREAM_SIZE);
	if (header == INTERRUPT_DISPATCH)
		stream_add_UINT32(pcb_packet->payload, 1);
	stream_add_pcb(pcb_packet, pcb);

	pthread_mutex_lock(&mutex_kernel_socket);
	if (kernel_client_socket != -1)
	{
		socket_send_packet(kernel_client_socket, pcb_packet);

		pthread_mutex_lock(&mutex_log);
		log_info(logger, "PID #%d CPU --> Kernel", pcb->pid);
		pthread_mutex_unlock(&mutex_log);
	}
	pthread_mutex_unlock(&mutex_kernel_socket);

	packet_destroy(pcb_packet);
	clean_tlb(tlb);
	pcb_destroy(pcb);

	// Resetea la interrupcion
	pthread_mutex_lock(&mutex_has_interruption);
	new_interruption = false;
	pthread_mutex_unlock(&mutex_has_interruption);
}

void release_interruption()
{
	pthread_mutex_lock(&mutex_log);
	log_warning(logger, "CPU is not loaded, releasing interruption");
	pthread_mutex_unlock(&mutex_log);

	t_packet *no_pcb_packet = create_packet(INTERRUPT_DISPATCH, INITIAL_STREAM_SIZE);
	stream_add_UINT32(no_pcb_packet->payload, (uint32_t)0);

	pthread_mutex_lock(&mutex_kernel_socket);
	if (kernel_client_socket != -1)
	{
		printf("send packettt\n");
		socket_send_packet(kernel_client_socket, no_pcb_packet);
	}
	pthread_mutex_unlock(&mutex_kernel_socket);

	packet_destroy(no_pcb_packet);
}

bool (*cpu_handlers[3])(t_packet *petition, int console_socket) =
	{
		// PCB_TO_CPU
		receive_pcb,
		// INTERRUPT
		receive_interruption,
		// VALUE_TO_CPU
		// receive_value,
};

void *dispatch_header_handler(void *_client_socket)
{
	pthread_mutex_lock(&mutex_kernel_socket);
	kernel_client_socket = (int)_client_socket;
	pthread_mutex_unlock(&mutex_kernel_socket);

	return packet_handler(_client_socket);
}

void *header_handler(void *_client_socket)
{
	bool serve = true;
	while (serve)
	{
		uint8_t header = socket_receive_header(_client_socket);
		if (header == INTERRUPT)
		{
			serve = cpu_handlers[header](NULL, (int)_client_socket);
		}
	}
}

void *packet_handler(void *_client_socket)
{
	bool serve = true;
	while (serve)
	{
		t_packet *packet = socket_receive_packet((int)_client_socket);

		if (packet == NULL)
		{
			if (!socket_retry_packet((int)_client_socket, &packet))
			{
				close((int)_client_socket);
				break;
			}
		}

		if (packet->header != TABLE_INFO_TO_CPU)
		{
			serve = cpu_handlers[packet->header](packet, (int)_client_socket);
		}
		packet_destroy(packet);
	}
	return 0;
}

bool receive_pcb(t_packet *petition, int kernel_socket)
{
	pcb = create_pcb();
	stream_take_pcb(petition, pcb);
	if (!!pcb)
	{
		pthread_mutex_lock(&mutex_log);
		log_info(logger, "Received PID #%d with %d instructions", pcb->pid,
				 list_size(pcb->instructions));
		pthread_mutex_unlock(&mutex_log);
		sem_post(&pcb_loaded);
		return true;
	}
	return true;
}

bool receive_interruption(t_packet *_petition, int kernel_socket)
{
	if (!!pcb && !!pcb->pid)
	{
		pthread_mutex_lock(&mutex_has_interruption);
		new_interruption = true;
		pthread_mutex_unlock(&mutex_has_interruption);

		pthread_mutex_lock(&mutex_log);
		log_info(logger, "Interruption received");
		pthread_mutex_unlock(&mutex_log);
	}
	else
	{
		release_interruption();
	}

	return true;
}

/* bool receive_value(t_packet *petition, int mem_socket)
{
	uint32_t value = stream_take_UINT32(petition->payload);
	if (value != NULL)
	{
		pthread_mutex_lock(&mutex_log);
		log_info(logger, "Read %d from memory", value);
		pthread_mutex_unlock(&mutex_log);

		pthread_mutex_lock(&mutex_value);
		read_value = value;
		pthread_mutex_unlock(&mutex_value);

		sem_post(&value_loaded);
		return true;
	}
	return false;
} */

void (*execute[6])(t_list *params) =
	{
		execute_no_op,
		execute_io,
		execute_read,
		execute_copy,
		execute_write,
		execute_exit,
};

void *cpu_cycle()
{
	while (1)
	{
		check_interrupt();

		sem_wait(&pcb_loaded);
		while (!!pcb && !!pcb->pid && pcb->program_counter < list_size(pcb->instructions))
		{
			t_instruction *instruction;
			enum operation op = fetch_and_decode(&instruction);
			execute[op](instruction->params);
			check_interrupt();
		}
		// TODO: Definir si mandar el proceso al kernel en caso de que no haya un EXIT
		// pcb_to_kernel(EXIT_CALL);
	}
	return 0;
}

void check_interrupt()
{
	// Checkea interrupcion
	pthread_mutex_lock(&mutex_has_interruption);
	if (new_interruption)
	{
		// Resetea la interrupcion
		new_interruption = false;
		pthread_mutex_unlock(&mutex_has_interruption);

		if (!!pcb && !!pcb->pid)
		{
			pthread_mutex_lock(&mutex_log);
			log_warning(logger, "Encountered interruption, kicking out process #%d", pcb->pid);
			pthread_mutex_unlock(&mutex_log);

			// Desalojar proceso actual
			pcb_to_kernel(INTERRUPT_DISPATCH);
		}
		else
		{
			// Avisa a kernel que se libero la CPU previo al chequeo de la interrupcion
			// y que no debe esperar el desalojo
			release_interruption();
		}
	}
	else
	{
		pthread_mutex_unlock(&mutex_has_interruption);
	}
}

enum operation fetch_and_decode(t_instruction **instruction)
{
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

void execute_no_op(t_list *params)
{
	uint32_t times = *((uint32_t *)list_get(params, 0));

	pthread_mutex_lock(&mutex_log);
	log_info(logger, "Executing NO OP for %dms %d times", config->delayNoOp, times);
	pthread_mutex_unlock(&mutex_log);

	usleep(config->delayNoOp * 1000 * times);
}

void execute_io(t_list *params)
{
	uint32_t time = *((uint32_t *)list_get(params, 0));

	pthread_mutex_lock(&mutex_log);
	log_info(logger, "Executing I/O for %dms", time);
	pthread_mutex_unlock(&mutex_log);

	pcb->pending_io_time = time;
	pcb_to_kernel(IO_CALL);
}

void execute_read(t_list *params)
{
	uint32_t l_address = *((uint32_t *)list_get(params, 0));

	pthread_mutex_lock(&mutex_log);
	log_info(logger, "Executing read of value in %d", l_address);
	pthread_mutex_unlock(&mutex_log);

	uint32_t page_number = get_page_number(l_address);
	uint32_t frame = get_frame(page_number);
	uint32_t offset = get_offset(l_address);

	pthread_mutex_lock(&mutex_log);
	log_info(logger, "Request to read frame %d", frame);
	pthread_mutex_unlock(&mutex_log);

	memory_op(READ_CALL, frame, offset, NULL);
	sem_wait(&value_loaded);

	pthread_mutex_lock(&mutex_value);
	pthread_mutex_lock(&mutex_log);
	log_info(logger, "Read %d in memory", read_value);
	pthread_mutex_unlock(&mutex_log);
	pthread_mutex_unlock(&mutex_value);
}

void execute_copy(t_list *params)
{
	uint32_t l_address = *((uint32_t *)list_get(params, 0));
	uint32_t l_value_address = *((uint32_t *)list_get(params, 1));

	pthread_mutex_lock(&mutex_log);
	log_info(logger, "Executing copy from %d to %d", l_value_address, l_address);
	pthread_mutex_unlock(&mutex_log);

	uint32_t val_page_number = get_page_number(l_value_address);
	uint32_t val_frame = get_frame(val_page_number);
	uint32_t val_offset = get_offset(l_value_address);

	pthread_mutex_lock(&mutex_log);
	log_info(logger, "Request to read frame %d", val_frame);
	pthread_mutex_unlock(&mutex_log);

	memory_op(READ_CALL, val_frame, val_offset, NULL);
	sem_wait(&value_loaded);

	pthread_mutex_lock(&mutex_value);
	pthread_mutex_lock(&mutex_log);
	log_info(logger, "Read %d in memory", read_value);
	pthread_mutex_unlock(&mutex_log);
	pthread_mutex_unlock(&mutex_value);

	uint32_t page_number = get_page_number(l_address);
	uint32_t frame = get_frame(page_number);
	uint32_t offset = get_offset(l_address);

	pthread_mutex_lock(&mutex_log);
	log_info(logger, "Request to write at frame %d", frame);
	pthread_mutex_unlock(&mutex_log);

	pthread_mutex_lock(&mutex_value);
	memory_op(WRITE_CALL, frame, offset, read_value);
	pthread_mutex_unlock(&mutex_value);
}

void execute_write(t_list *params)
{
	uint32_t l_address = *((uint32_t *)list_get(params, 0));
	uint32_t value = *((uint32_t *)list_get(params, 1));

	pthread_mutex_lock(&mutex_log);
	log_info(logger, "Executing write of %d in %d", value, l_address);
	pthread_mutex_unlock(&mutex_log);

	uint32_t page_number = get_page_number(l_address);
	uint32_t frame = get_frame(page_number);
	uint32_t offset = get_offset(l_address);

	pthread_mutex_lock(&mutex_log);
	log_info(logger, "Request to write at frame %d", frame);
	pthread_mutex_unlock(&mutex_log);

	memory_op(WRITE_CALL, frame, offset, value);
}

void execute_exit()
{
	pthread_mutex_lock(&mutex_log);
	log_info(logger, "Executing exit");
	pthread_mutex_unlock(&mutex_log);

	pcb_to_kernel(EXIT_CALL);
}

void memory_handshake()
{
	socket_send_header(memory_server_socket, MEM_HANDSHAKE);

	pthread_mutex_lock(&mutex_log);
	log_info(logger, "Handshake with memory requested");
	pthread_mutex_unlock(&mutex_log);

	t_packet *mem_data = socket_receive_packet(memory_server_socket);
	if (mem_data->header == TABLE_INFO_TO_CPU)
	{
		// Recibir datos
		config->pageSize = stream_take_UINT32(mem_data->payload);
		config->entriesPerTable = stream_take_UINT32(mem_data->payload);

		pthread_mutex_lock(&mutex_log);
		log_info(logger, "Handshake with memory successful");
		log_info(logger, "Page size: %d | Entries per table: %d", config->pageSize,
				 config->entriesPerTable);
		pthread_mutex_unlock(&mutex_log);
	}
	packet_destroy(mem_data);
}

uint32_t get_frame(uint32_t page_number)
{
	uint32_t frame = find_tlb_entry(page_number);
	if (frame != -1)
	{
		return frame;
	}

	uint32_t lvl1_entry_index = floor(page_number / config->entriesPerTable);

	t_packet *lvl1_table_request = create_packet(LVL1_TABLE, INITIAL_STREAM_SIZE);
	stream_add_UINT32(lvl1_table_request->payload, pcb->page_table);
	stream_add_UINT32(lvl1_table_request->payload, lvl1_entry_index);
	socket_send_packet(memory_server_socket, lvl1_table_request);

	packet_destroy(lvl1_table_request);

	t_packet *lvl2_data = socket_receive_packet(memory_server_socket);
	if (lvl2_data->header == TABLE2_TO_CPU)
	{
		uint32_t pt2_index = stream_take_UINT32(lvl2_data->payload);

		packet_destroy(lvl2_data);

		uint32_t lvl2_entry_index = page_number % config->entriesPerTable;

		t_packet *lvl2_table_request = create_packet(LVL2_TABLE, INITIAL_STREAM_SIZE);
		stream_add_UINT32(lvl2_table_request->payload, pcb->pid);
		stream_add_UINT32(lvl2_table_request->payload, pt2_index);
		stream_add_UINT32(lvl2_table_request->payload, lvl2_entry_index);
		stream_add_UINT32(lvl2_table_request->payload, pcb->frames_index);
		socket_send_packet(memory_server_socket, lvl2_table_request);

		packet_destroy(lvl2_table_request);

		t_packet *frame_data = socket_receive_packet(memory_server_socket);
		if (frame_data->header == FRAME_TO_CPU)
		{
			frame = stream_take_UINT32(frame_data->payload);
			packet_destroy(frame_data);

			add_tlb_entry(page_number, frame);
			return frame;
		}
	}

	return frame;
}

void memory_op(enum memory_headers header, uint32_t frame, uint32_t offset, uint32_t value)
{
	t_packet *request = create_packet(header, INITIAL_STREAM_SIZE);
	stream_add_UINT32(request->payload, frame);
	stream_add_UINT32(request->payload, offset);
	if (value != NULL)
		stream_add_UINT32(request->payload, value);
	stream_add_UINT32(request->payload, pcb->frames_index);

	socket_send_packet(memory_server_socket, request);

	packet_destroy(request);

	if (header == READ_CALL)
	{
		t_packet *value_response = socket_receive_packet(memory_server_socket);
		if (value_response->header == VALUE_TO_CPU)
		{
			pthread_mutex_lock(&mutex_value);
			read_value = stream_take_UINT32(value_response->payload);
			pthread_mutex_unlock(&mutex_value);
			sem_post(&value_loaded);
		}
		packet_destroy(value_response);
	}
}

void stats()
{
	pthread_mutex_lock(&mutex_log);
	log_info(logger, "- - - Stats - - -");
	log_info(logger, "TLB hits: %d", tlb_hit_counter);
	log_info(logger, "TLB misses: %d", tlb_miss_counter);
	pthread_mutex_unlock(&mutex_log);
}

void terminate_cpu(int x)
{
	tlb_destroy();
	destroy_cpu_config(config);

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
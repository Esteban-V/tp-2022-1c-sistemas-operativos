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

	pthread_mutex_lock(&mutex_log);
	log_info(logger, "CPU server ready for Kernel");
	pthread_mutex_unlock(&mutex_log);

	sem_init(&pcb_loaded, 0, 0);
	pthread_mutex_init(&mutex_has_interruption, NULL);

	new_interruption = false;

	pthread_create(&interruptionThread, 0, listen_interruption, NULL);
	pthread_detach(interruptionThread);

	pthread_create(&execThread, 0, cpu_cycle, NULL);
	pthread_detach(execThread);

	sem_init(&interruption_counter, 0, 0);

	memory_server_socket = connect_to(config->memoryIP, config->memoryPort);

	if (memory_server_socket == -1)
	{
		terminate_cpu(true);
	}

	// Handshake con memoria
	memory_handshake();

	pthread_mutex_init(&tlb_mutex, NULL);
	tlb = create_tlb();

	while (1)
	{
		server_listen(kernel_dispatch_socket, dispatch_header_handler);
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
	clean_tlb(tlb);

	t_packet *pcb_packet = create_packet(header, INITIAL_STREAM_SIZE);
	stream_add_pcb(pcb_packet, pcb);

	socket_send_packet(kernel_client_socket, pcb_packet);

	pthread_mutex_lock(&mutex_log);
	log_info(logger, "PID #%d CPU --> Kernel", pcb->pid);
	pthread_mutex_unlock(&mutex_log);

	packet_destroy(pcb_packet);
	pcb_destroy(pcb);
}

bool (*cpu_handlers[2])(t_packet *petition, int console_socket) =
	{
		// PCB_TO_CPU
		receive_pcb,
		// INTERRUPT
		receive_interruption,
};

void *dispatch_header_handler(void *_client_socket)
{
	kernel_client_socket = (int)_client_socket;

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

		serve = receive_pcb(packet, (int)_client_socket);
		packet_destroy(packet);
	}
	return 0;
}

void *header_handler(void *_client_socket)
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

		serve = receive_interruption(packet, (int)_client_socket);
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
	return false;
}

bool receive_interruption(t_packet *petition, int kernel_socket)
{
	pthread_mutex_lock(&mutex_log);
	log_info(logger, "Interruption received");
	pthread_mutex_unlock(&mutex_log);

	pthread_mutex_lock(&mutex_has_interruption);
	new_interruption = true;
	pthread_mutex_unlock(&mutex_has_interruption);

	return true;
}

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
		sem_wait(&pcb_loaded);
		while (!!pcb && pcb->program_counter < list_size(pcb->instructions))
		{
			t_instruction *instruction;
			enum operation op = fetch_and_decode(&instruction);
			execute[op](instruction->params);

			// Checkea interrupcion
			pthread_mutex_lock(&mutex_has_interruption);
			if (new_interruption)
			{
				log_info(logger, "Encountered interruption, sending to kernel");
				// Desalojar proceso actual
				pcb_to_kernel(INTERRUPT_DISPATCH);

				// Resetea la interrupcion
				new_interruption = false;
				pthread_mutex_unlock(&mutex_has_interruption);
			}
			else
			{
				pthread_mutex_unlock(&mutex_has_interruption);
			}
		}

		// Mandar el proceso al kernel en caso de que no haya un EXIT?
		// pcb_to_kernel(EXIT_CALL);
	}
	return 0;
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
	pthread_mutex_lock(&mutex_log);
	log_info(logger, "Executing NO OP");
	pthread_mutex_unlock(&mutex_log);

	uint32_t times = *((uint32_t *)list_get(params, 0));
	usleep(config->delayNoOp * 1000 * times);
}

void execute_io(t_list *params)
{

	pthread_mutex_lock(&mutex_log);
	log_info(logger, "Executing IO");
	pthread_mutex_unlock(&mutex_log);

	uint32_t time = *((uint32_t *)list_get(params, 0));
	pcb->pending_io_time = time;
	pcb_to_kernel(IO_CALL);
}

void execute_read(t_list *params)
{
	pthread_mutex_lock(&mutex_log);
	log_info(logger, "Execute read");
	pthread_mutex_unlock(&mutex_log);

	uint32_t l_address = *((uint32_t *)list_get(params, 0));

	// MMU debe calcular:
	uint32_t page_number = floor(l_address / config->pageSize);
	uint32_t offset = l_address - (page_number * config->pageSize);

	uint32_t frame = get_frame(page_number);
	// uint32_t lvl1_entry_index = floor(page_number / config->entriesPerTable);
	// uint32_t lvl2_entry_index = page_number % config->entriesPerTable;
	// read(frame, offset);

	// Pedir LVL1_TABLE con pcb->page_table
}

void execute_copy(t_list *params)
{
	pthread_mutex_lock(&mutex_log);
	log_info(logger, "Execute copy");

	pthread_mutex_unlock(&mutex_log);

	uint32_t l_address = *((uint32_t *)list_get(params, 0));
	uint32_t l_value_address = *((uint32_t *)list_get(params, 1));
	// uint32_t value = fetch_operand(l_value_address);
	//  write(l_address, value);
}

void execute_write(t_list *params)
{
	pthread_mutex_lock(&mutex_log);
	log_info(logger, "Execute write");
	pthread_mutex_unlock(&mutex_log);

	uint32_t l_address = *((uint32_t *)list_get(params, 0));
	uint32_t value = *((uint32_t *)list_get(params, 1));
	// write(l_address, value);
}

void execute_exit()
{
	pthread_mutex_lock(&mutex_log);
	log_info(logger, "Execute exit");
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

void stats()
{
	pthread_mutex_lock(&mutex_log);
	log_info(logger, "- - - Stats - - -");
	log_info(logger, "TLB Hits: %d", tlb_hit_counter);
	log_info(logger, "TLB Misses: %d", tlb_miss_counter);
	pthread_mutex_unlock(&mutex_log);
}

void terminate_cpu(int x)
{
	destroy_tlb();
	log_destroy(logger);
	destroy_cpu_config(config);

	switch (x)
	{
	case 1:
		exit(EXIT_FAILURE);
	case SIGINT:
		stats();
		exit(EXIT_SUCCESS);
	}
}
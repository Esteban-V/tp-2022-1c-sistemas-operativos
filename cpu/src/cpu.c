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
	pthread_mutex_init(&mutex_kernel_socket, NULL);
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
	t_packet *pcb_packet = create_packet(header, INITIAL_STREAM_SIZE);
	stream_add_pcb(pcb_packet, pcb);

	pthread_mutex_lock(&mutex_kernel_socket);
	if (kernel_client_socket != -1)
	{
		socket_send_packet(kernel_client_socket, pcb_packet);
		pthread_mutex_unlock(&mutex_kernel_socket);

		pthread_mutex_lock(&mutex_log);
		log_info(logger, "PID #%d CPU --> Kernel", pcb->pid);
		pthread_mutex_unlock(&mutex_log);
	}
	else
	{
		pthread_mutex_unlock(&mutex_kernel_socket);
	}

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
	pthread_mutex_lock(&mutex_kernel_socket);
	kernel_client_socket = (int)_client_socket;
	pthread_mutex_unlock(&mutex_kernel_socket);

	return header_handler(_client_socket);
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
		serve = cpu_handlers[packet->header](packet, (int)_client_socket);
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
	usleep(config->delayNoOp * 1000 * times);
}

void execute_io(t_list *params)
{
	uint32_t time = *((uint32_t *)list_get(params, 0));
	pcb->pending_io_time = time;
	pcb_to_kernel(IO_CALL);
}

void execute_read(t_list *params)
{
	uint32_t l_address = *((uint32_t *)list_get(params, 0));

	// MMU debe calcular:

		uint32_t page_number = floor(l_address / config->pageSize);
			uint32_t entry_index = floor(page_number / config->entriesPerTable);
	// Pedir LVL1_TABLE con pcb->page_table

}

void execute_copy(t_list *params)
{
	uint32_t l_address = *((uint32_t *)list_get(params, 0));
	uint32_t l_value_address = *((uint32_t *)list_get(params, 1));
	// uint32_t value = fetch_operand(l_value_address);
	//  write(l_address, value);
}

void execute_write(t_list *params)
{
	uint32_t l_address = *((uint32_t *)list_get(params, 0));
	uint32_t value = *((uint32_t *)list_get(params, 1));
	// write(l_address, value);
}

void execute_exit()
{
	clean_tlb(tlb);
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
	log_info(logger, "\n- - - Stats - - -\n");
	log_info(logger, "TLB Hits: %d", tlb_hit_counter);
	log_info(logger, "TLB Misses: %d", tlb_miss_counter);
	pthread_mutex_unlock(&mutex_log);
}

void terminate_cpu(int x)
{
	switch(x)
	{
	case SIGINT:
		stats();
		destroy_tlb();
		log_destroy(logger);
		destroy_cpu_config(config);
		exit(EXIT_SUCCESS);
	}
}

t_tlb *create_tlb()
{
	// Inicializo estructura de TLB
	tlb = malloc(sizeof(t_tlb));
	tlb->entryQty = config->tlbEntryQty; // cantidad de entradas
	tlb->entries = (t_tlbEntry *)calloc(tlb->entryQty, sizeof(t_tlbEntry));
	tlb->victimQueue = list_create();

	if (strcmp(config->tlb_alg, "LRU") == 0)
	{
		update_victim_queue = lru_tlb;
	}
	else
	{
		update_victim_queue = fifo_tlb;
	}

	// Seteo todas las entradas como libres
	for (int i = 0; i < tlb->entryQty; i++)
	{
		tlb->entries[i].isFree = true;
	}

	return tlb;
}

// -1 si no esta en tlb
// TODO ejecutar en memoria cuando se  pide frame
int32_t get_tlb_frame(uint32_t pid, uint32_t page)
{
	pthread_mutex_lock(&tlb_mutex);
	int32_t frame = -1;
	for (int i = 0; i < tlb->entryQty; i++)
	{
		if (tlb->entries[i].page == page && tlb->entries[i].isFree == false)
		{
			update_victim_queue(&(tlb->entries[i]));
			frame = tlb->entries[i].frame;
			tlb_hit_counter++;
		}
	}
	pthread_mutex_unlock(&tlb_mutex);

	tlb_miss_counter++;

	return frame;
}

// TODO ejecutar en memoria cuando se reemplaza / asigna pagina a TP
void add_tlb_entry(uint32_t pid, uint32_t page, int32_t frame)
{
	pthread_mutex_lock(&tlb_mutex);

	// Busco si hay una entrada libre
	bool any_free_entry = false;
	for (int i = 0; i < tlb->entryQty && !any_free_entry; i++)
	{
		if (tlb->entries[i].isFree)
		{
			tlb->entries[i].page = page;
			tlb->entries[i].frame = frame;
			tlb->entries[i].isFree = false;

			pthread_mutex_lock(&mutex_log);
			log_info(logger, "TLB Assignment: Free Entry Used %d ; PID: %u, Page: %u, Frame: %u", i, pid, page, frame);
			pthread_mutex_unlock(&mutex_log);

			any_free_entry = true;
			list_add(tlb->victimQueue, &(tlb->entries[i]));
			break;
		}
	}

	// Si no hay entrada libre, reemplazo
	if (!any_free_entry)
	{
		if (!list_is_empty(tlb->victimQueue))
		{
			t_tlbEntry *victim = list_remove(tlb->victimQueue, 0);

			pthread_mutex_lock(&mutex_log);
			log_info(logger, "TLB Replacement: Replacement in Entry %d ; Old Entry Data: Page: %u, Frame: %u ; New Entry Data: Page: %u, Frame: %u ; PID: %u",
					 (victim - tlb->entries), victim->page, victim->frame, pid, page, frame);
			pthread_mutex_unlock(&mutex_log);

			victim->page = page;
			victim->frame = frame;
			victim->isFree = false;
			list_add(tlb->victimQueue, victim);
		}
	}

	pthread_mutex_unlock(&tlb_mutex);
}

void lru_tlb(t_tlbEntry *entry)
{
	bool isVictim(t_tlbEntry * victim)
	{
		return victim == entry;
	}

	t_tlbEntry *entryToBeMoved = list_remove_by_condition(tlb->victimQueue, (void *)isVictim);

	list_add(tlb->victimQueue, entryToBeMoved);
}

void fifo_tlb(t_tlbEntry *entry)
{
	// Intencionalmente vacío
}

// TODO ejecutar en memoria cuando se va a reemplazar pag
void drop_tlb_entry(uint32_t page, uint32_t frame)
{
	pthread_mutex_lock(&tlb_mutex);

	for (int i = 0; i < tlb->entryQty; i++)
	{
		if (tlb->entries[i].page == page && tlb->entries[i].frame == frame)
		{
			tlb->entries[i].isFree = true;
			pthread_mutex_unlock(&tlb_mutex);
			return;
		}
	}

	pthread_mutex_unlock(&tlb_mutex);
}

void destroy_tlb()
{
	free(tlb->entries);
	list_destroy(tlb->victimQueue);
	free(tlb);
}

// Corre cada vez que se hace execute_exit()
void clean_tlb()
{
	pthread_mutex_lock(&tlb_mutex);
	for (int i = 0; i < tlb->entryQty; i++)
	{
		tlb->entries[i].isFree = true;
	}
	pthread_mutex_unlock(&tlb_mutex);
}

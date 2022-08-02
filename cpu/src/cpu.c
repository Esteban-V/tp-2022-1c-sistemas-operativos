#include "cpu.h"

int main()
{

	signal(SIGINT, terminate_cpu);

	// Initialize logger
	logger = log_create("./cfg/cpu-final.log", "CPU", 1, LOG_LEVEL_TRACE);
	config = get_cpu_config("./cfg/cpu.config");

	i=0;

	// Creacion de server
	kernel_dispatch_socket = create_server(config->dispatchListenPort);
	kernel_interrupt_socket = create_server(config->interruptListenPort);

	pcb = create_pcb();

	pthread_mutex_init(&mutex_kernel_socket, NULL);

	pthread_mutex_lock(&mutex_log);
	log_info(logger, "CPU server ready for Kernel");
	pthread_mutex_unlock(&mutex_log);
	sem_init(&waiting_frame, 0, 0);
	sem_init(&waiting_second_table_number, 0, 0);
	sem_init(&waiting_write_answer, 0, 0);
	sem_init(&waiting_read_answer, 0, 0);
	
	
	sem_init(&pcb_loaded, 0, 0);
	pthread_mutex_init(&mutex_has_interruption, NULL);
	sem_init(&cpu_bussy, 0,0);

	new_interruption = false;

	pthread_create(&interruptionThread, 0, listen_interruption, NULL);
	pthread_detach(interruptionThread);

	pthread_create(&execThread, 0, cpu_cycle, NULL);
	pthread_detach(execThread);

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

	pthread_mutex_lock(&mutex_kernel_socket);
	if (kernel_client_socket != -1)
	{
		socket_send_packet(kernel_client_socket, pcb_packet);

		pthread_mutex_lock(&mutex_log);
		log_info(logger, "PID #%d CPU --> Kernel", pcb->pid);
		pthread_mutex_unlock(&mutex_log);
	}
	sem_wait(&cpu_bussy);
	pthread_mutex_unlock(&mutex_kernel_socket);

	packet_destroy(pcb_packet);
}

bool (*cpu_handlers[11])(t_packet *petition, int console_socket) =
	{
		// PCB_TO_CPU
		receive_pcb,
		// INTERRUPT
		receive_interruption,
		//FRAME_TO_CPU = 2,
		receive_frame,
		//TABLE2_TO_CPU = 3,
		receive_ptTwoIndex,
	//TABLE_INFO_TO_CPU = 4,
		NULL,
	//SWAP_OK = 5,
	NULL,
	//SWAP_ERROR = 6,
	NULL,
	//TLB_ADD = 7,
	NULL,
	//TLB_DROP = 8,
	NULL,
	//WRITE_SUCCESS = 9,
	write_answer,
	//READ_ANSWER = 10,
	read_answer,
};

void *dispatch_header_handler(void *_client_socket)
{
	if(i==0){
	pthread_mutex_lock(&mutex_kernel_socket);
	kernel_client_socket = (int)_client_socket;
	pthread_mutex_unlock(&mutex_kernel_socket);
	}
	i++;
	return packet_handler(_client_socket);
}

void *header_handler(void *_client_socket)
{
	bool serve = true;
	while (serve)
	{
		uint8_t header = socket_receive_header(_client_socket);
		if (header == MEM_HANDSHAKE)
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

		serve = cpu_handlers[packet->header](packet, (int)_client_socket);
		packet_destroy(packet);
	}
	return 0;
}

bool receive_pcb(t_packet *petition, int kernel_socket)
{
	stream_take_pcb(petition, pcb);
	if (!!pcb)
	{
		pthread_mutex_lock(&mutex_log);
		log_info(logger, "Received PID #%d with %d instructions", pcb->pid,
				 list_size(pcb->instructions));
		pthread_mutex_unlock(&mutex_log);
		if(bussyCpu()){
			pthread_mutex_lock(&mutex_log);
			log_warning(logger, "CPU was used");
			pthread_mutex_unlock(&mutex_log);
		}
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
		sem_post(&cpu_bussy);
		while (bussyCpu() && pcb->program_counter < list_size(pcb->instructions))
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
	uint32_t fisicDir = mmu(l_address);
	readMem(fisicDir);

	// Pedir LVL1_TABLE con pcb->page_table
}

void execute_copy(t_list *params)
{
	pthread_mutex_lock(&mutex_log);
	log_info(logger, "Execute copy");

	pthread_mutex_unlock(&mutex_log);

	uint32_t l_address = *((uint32_t *)list_get(params, 0));
	uint32_t l_value_address = *((uint32_t *)list_get(params, 1));
	uint32_t fisicDirWrite = mmu(l_address);
	uint32_t fisicDirRead = mmu(l_value_address);
	uint32_t value = readMem(fisicDirRead);
	writeMem(fisicDirWrite, value);
}

uint32_t readMem(uint32_t fisicDir){
	t_packet *readPack = create_packet(READ_CALL, INITIAL_STREAM_SIZE);
	stream_add_UINT32(readPack->payload, fisicDir);

	if (memory_server_socket != -1)
	{
				socket_send_packet(memory_server_socket, readPack);
				pthread_mutex_lock(&mutex_log);
				log_info(logger, "Read request");
				pthread_mutex_unlock(&mutex_log);
				sem_wait(&waiting_read_answer);
	}
	return memRead;
}

void writeMem(uint32_t fisicDir,uint32_t value){
	t_packet *write = create_packet(WRITE_CALL, INITIAL_STREAM_SIZE);
	stream_add_UINT32(write->payload, fisicDir);
	stream_add_UINT32(write->payload, value);

	if (memory_server_socket != -1)
	{
				socket_send_packet(memory_server_socket, write);
				pthread_mutex_lock(&mutex_log);
				log_info(logger, "Write request");
				pthread_mutex_unlock(&mutex_log);
				sem_wait(&waiting_write_answer);
	}
	return true;
}

bool write_answer(t_packet *petition, int kernel_socket)
{//WRITE_SUCCESS
	pthread_mutex_lock(&mutex_log);
	log_info(logger, "Write succed");
	pthread_mutex_unlock(&mutex_log);
	sem_post(&waiting_write_answer);
	return true;
}

bool read_answer(t_packet *petition, int kernel_socket)
{//READ_ANSWER
	memRead = stream_take_UINT32(petition->payload);
	pthread_mutex_lock(&mutex_log);
	log_info(logger, "Read %d succed",memRead);
	pthread_mutex_unlock(&mutex_log);
	sem_post(&waiting_read_answer);
	return true;
}

void execute_write(t_list *params)
{
	pthread_mutex_lock(&mutex_log);
	log_info(logger, "Execute write");
	pthread_mutex_unlock(&mutex_log);

	uint32_t l_address = *((uint32_t *)list_get(params, 0));
	uint32_t value = *((uint32_t *)list_get(params, 1));
	uint32_t fisicDir = mmu(l_address);

	writeMem(fisicDir, value);
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
	tlb_destroy();
	log_destroy(logger);
	destroy_cpu_config(config);
	pcb_destroy(pcb);

	switch (x)
	{
	case 1:
		exit(EXIT_FAILURE);
	case SIGINT:
		stats();
		exit(EXIT_SUCCESS);
	}
}
bool bussyCpu(){
	int isBussy;
	sem_getvalue(&cpu_bussy,&isBussy);
	return isBussy;
}

uint32_t mmu(int logicDir){
	int page_number = floor(logicDir/config->pageSize);
	int fstLevelEntry = floor(page_number/config->entriesPerTable);
	int scdLevelEntry = page_number%config->entriesPerTable;
	int offset = logicDir - page_number*config->pageSize;
	int framesIndex = pcb->process_frames_index;
	int pid = pcb->pid;
	int ptIndex = pcb->page_table;

	t_packet *fsPageTable = create_packet(LVL1_TABLE, INITIAL_STREAM_SIZE);
	stream_add_UINT32(fsPageTable->payload, ptIndex);
	stream_add_UINT32(fsPageTable->payload, fstLevelEntry);


	if (memory_server_socket != -1)
	{
				socket_send_packet(memory_server_socket, fsPageTable);
				sem_wait(&waiting_second_table_number);
	}
	t_packet *sdPageTable = create_packet(LVL2_TABLE, INITIAL_STREAM_SIZE);
	stream_add_UINT32(fsPageTable->payload, pid);
	stream_add_UINT32(fsPageTable->payload, tableTwoIndex);
	stream_add_UINT32(fsPageTable->payload, scdLevelEntry);
	stream_add_UINT32(fsPageTable->payload, framesIndex);


	if (memory_server_socket != -1)
	{
				socket_send_packet(memory_server_socket, sdPageTable);
				sem_wait(&waiting_frame);
	}
	uint32_t fisicDir = received_frame+offset;
	
	return fisicDir;
}

bool receive_frame(t_packet *petition, int kernel_socket)
{//FRAME_TO_CPU
	received_frame = (int) stream_take_UINT32(petition->payload);
	pthread_mutex_lock(&mutex_log);
	log_info(logger, "Frame received");
	pthread_mutex_unlock(&mutex_log);
	sem_post(&waiting_frame);
	return true;
}

bool receive_ptTwoIndex(t_packet *petition, int kernel_socket)
{//TABLE2_TO_CPU
	tableTwoIndex = (int) stream_take_UINT32(petition->payload);
	pthread_mutex_lock(&mutex_log);
	log_info(logger, "Page Table 2 index received");
	pthread_mutex_unlock(&mutex_log);
	sem_post(&waiting_second_table_number);
	return true;
}
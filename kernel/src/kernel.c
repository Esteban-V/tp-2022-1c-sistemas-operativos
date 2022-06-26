#include "kernel.h"

int main(void) {
	logger = log_create("kernel.log", "KERNEL", 1, LOG_LEVEL_INFO);
	config = getKernelConfig("kernel.config");

	// Inicializar estructuras de estado
	newQ = pQueue_create();
	readyQ = pQueue_create();
	blockedQ = pQueue_create();
	suspended_blockQ = pQueue_create();
	suspended_readyQ = pQueue_create();
	exitQ = pQueue_create();

	if (config == NULL) {
		log_error(logger, "Config failed to load");
		terminate_kernel(true);
	}

	// Setteo de Algoritmo de Planificacion
	if (!strcmp(config->schedulerAlgorithm, "SRT")) {
		sortingAlgorithm = SRT;
	} else if (!strcmp(config->schedulerAlgorithm, "FIFO")) {
		sortingAlgorithm = FIFO;
	} else {
		log_warning(logger,
				"Wrong scheduler algorithm set in config --> Using FIFO");
	}

	// Inicializar Planificador de largo plazo
	pthread_create(&newToReadyThread, 0, newToReady, NULL);
	pthread_detach(newToReadyThread);

	// Inicializar Planificador de largo plazo
	pthread_create(&readyToExecThread, 0, readyToExec, NULL);
	pthread_detach(readyToExecThread);

	// Finalizador de procesos
	pthread_create(&exitProcessThread, 0, exit_process, NULL);
	pthread_detach(exitProcessThread);

	// Inicializar cpu dispatch listener
	pthread_create(&cpuDispatchThread, 0, cpu_dispatch_listener, NULL);
	pthread_detach(cpuDispatchThread);

	pthread_create(&io_thread, 0, io_t, NULL);
	pthread_detach(io_thread);

	// Creacion de server
	int server_socket = create_server(config->kernelIP, config->kernelPort);
	log_info(logger, "Kernel ready for console");

	// Inicializar semaforo de multiprocesamiento
	sem_init(&sem_multiprogram, 0, config->multiprogrammingLevel);
	sem_init(&freeCpu, 0, 1);
	cupos_libres = config->multiprogrammingLevel;
	pthread_mutex_init(&mutex_cupos, NULL);

	sem_init(&any_for_ready, 0, 0);
	sem_init(&longTermSemCall, 0, 0);

	sem_init(&exec_to_ready, 0, 0);

	sem_init(&any_blocked, 0, 0);

	sem_init(&bloquear, 0, 0);

	// Inicializo condition variable para despertar al planificador de mediano plazo
	pthread_cond_init(&cond_mediumTerm, NULL);
	pthread_mutex_init(&mutex_mediumTerm, NULL);

	cpu_dispatch_socket = connect_to(config->cpuIP, config->cpuPortDispatch);
	cpu_interrupt_socket = connect_to(config->cpuIP, config->cpuPortInterrupt);
	if (!cpu_dispatch_socket || !cpu_interrupt_socket) {
		terminate_kernel(true);
	}

	log_info(logger, "Kernel connected to CPU");

	/*
	 memory_server_socket = connect_to(config->memoryIP, config->memoryPort);
	 */

	//pid = 0;
	while (1) {
		server_listen(server_socket, header_handler);
	}

	destroyKernelConfig(config);
	terminate_kernel(false);
}

void* cpu_dispatch_listener() {
	sem_wait(&bloquear);
	//listen cpu_dispatch_socket
	/*
	 int server_cpu_socket = create_server(config->kernelIP, config->cpuPortDispatch);
	 while(1){
	 server_listen(server_cpu_socket, header_handler);
	 }
	 */
}

void* exit_process() {
	t_pcb *pcb;
	while (1) {
		pcb = pQueue_take(exitQ);
		//avisar a consola que exit
	}
}

void* readyToExec(void *args) {
	t_pcb *pcb = NULL;
	while (1) {
		sem_wait(&freeCpu);
		pcb = pQueue_take(readyQ);

		t_packet *pcb_packet = create_packet(PCB_TO_CPU, 64); //implementar PCBTOCPU
		stream_add_pcb(pcb_packet, pcb);
		if (cpu_dispatch_socket != -1) {
			socket_send_packet(cpu_dispatch_socket, pcb_packet);
		}

		packet_destroy(pcb_packet);
		pthread_mutex_lock(&mutex_log);
		log_info(logger, "Process %d to CPU", pcb->pid);
		pthread_mutex_unlock(&mutex_log);

	}
}

void* newToReady() { // Hilo del largo plazo, toma un proceso de new y lo pasa a ready
	t_pcb *pcb;
	while (1) {
		sem_wait(&sem_multiprogram);
		sem_wait(&any_for_ready);
		if (!pQueue_isEmpty(suspended_readyQ)) {
			pcb = pQueue_take(suspended_readyQ);
			pQueue_put(readyQ, pcb);
		}
		pcb = (t_pcb*) pQueue_take(newQ);

		//sem_wait(&longTermSemCall);
		//pthread_mutex_lock(&mutex_mediumTerm);
		//pcb = (t_pcb*) pQueue_take(newQ);

		// Message a Memoria para que cree estructuras

		/*
		 t_packet *memory_info = create_packet(MEMORY_INFO, 64);
		 stream_add_UINT32(memory_info->payload, pcb->size);

		 if (memory_server_socket != -1) {
		 socket_send_packet(memory_server_socket, memory_info);
		 }

		 packet_destroy(memory_info);

		 // Recibir valo de Tabla

		 // Actualizar PCB
		 */
		putToReady(pcb);

		pthread_mutex_lock(&mutex_log);
		log_info(logger, "Long Term Scheduler: process %u from New to Ready",
				pcb->pid);
		pthread_mutex_unlock(&mutex_log);

		pthread_mutex_lock(&mutex_cupos);
		cupos_libres--; // TODO Chequear bien donde se modifica
		pthread_mutex_unlock(&mutex_cupos);

		pthread_cond_signal(&cond_mediumTerm);
		pthread_mutex_unlock(&mutex_mediumTerm);
	}
}

void* io_t(void *args) {
	t_pcb *pcb;
	int milisecs;
	int sobra;

	//int memorySocket = connectToServer(config->memoryIP, config->memoryPort);

	//t_packet* suspendRequest;

	while (1) {
		sem_wait(&any_blocked);
		if (!pQueue_isEmpty(blockedQ)) {
			pcb = pQueue_peek(blockedQ); //retorna el primer elemento de

			clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &now);
			milisecs = (config->maxBlockedTime)
					- (time_to_ms(now) - (pcb->blocked_time));
			sobra = milisecs - pcb->nextIO;

			if (sobra >= 0) {
				usleep(pcb->nextIO);
				pcb = pQueue_take(blockedQ);
				putToReady(pcb);
			} else {
				usleep(milisecs);
				pcb = pQueue_take(blockedQ);
				pcb->nextIO = pcb->nextIO - milisecs;
				//Notifica a memoria de la suspension
				//suspendRequest = createPacket(SUSPEND, INITIAL_STREAM_SIZE);
				//streamAdd_UINT32(suspendRequest->payload, process->pid);
				//socket_sendPacket(memorySocket, suspendRequest);
				//destroyPacket(suspendRequest);
				pQueue_put(suspended_blockQ, pcb);
				sem_post(&sem_multiprogram);
				pthread_mutex_lock(&mutex_cupos);
				cupos_libres++;
				pthread_mutex_unlock(&mutex_cupos);
				pthread_mutex_lock(&mutex_log);
				log_info(logger,
						"Medium Term Scheduler: process %u to Suspended Blocked",
						pcb->pid);
				pthread_mutex_unlock(&mutex_log);
			}
		} else {
			pcb = pQueue_peek(suspended_blockQ);
			usleep(pcb->nextIO);
			pcb = pQueue_take(suspended_blockQ);
			pQueue_put(suspended_readyQ, pcb);
			sem_post(&any_for_ready);
		}

	}
}

void putToReady(t_pcb *pcb) {

	pQueue_put(readyQ, (void*) pcb);

	if (sortingAlgorithm) {

		t_packet *int_packet = create_packet(INTERRUPT, 64);
		stream_add_UINT32(int_packet->payload, 1);
		if (cpu_interrupt_socket != -1) {
			socket_send_packet(cpu_interrupt_socket, int_packet);
		}

		packet_destroy(int_packet);

		sem_wait(&exec_to_ready);

		pQueue_sort(readyQ, SFJ_algorithm);

		pthread_mutex_lock(&mutex_log);
		log_info(logger, "Corto Plazo: Cola Ready replanificada:");
		pthread_mutex_unlock(&mutex_log);
	}
}

bool receive_process(t_packet *petition, int console_socket) {

	t_process *received_process = create_process();
	stream_take_process(petition, received_process);

	if (!!received_process) {
		t_list *instructions = received_process->instructions;
		log_info(logger, "Received process with %d instructions",
				instructions->elements_count);

		t_pcb *pcb = create_pcb();
		memcpy(pcb->instructions, instructions, sizeof(t_list));

		pid++;
		pcb->pid = pid;
		pcb->size = received_process->size;
		pcb->program_counter = 0;
		pcb->burst_estimation = config->initialEstimate;

		pQueue_put(newQ, (void*) pcb);
		pthread_mutex_lock(&mutex_log);
		log_info(logger, "PID #%d --> New queue", pcb->pid);
		pthread_mutex_unlock(&mutex_log);

		sem_post(&any_for_ready);
	}

	process_destroy(received_process);
	return false;
}

bool io_op(t_packet *petition, int console_socket) {
	sem_post(&freeCpu);
	t_pcb *received_pcb = create_pcb();
	stream_take_pcb(petition, received_pcb);
	clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &now);
	received_pcb->blocked_time = time_to_ms(now);
	pQueue_put(blockedQ, (void*) received_pcb);
	sem_post(&any_blocked);
	return false;
}

bool exit_op(t_packet *petition, int console_socket) {
	sem_post(&freeCpu);
	t_pcb *received_pcb = create_pcb();
	stream_take_pcb(petition, received_pcb);
	pQueue_put(exitQ, (void*) received_pcb);
	return false;
}

bool interrupt_ready(t_packet *petition, int console_socket) {
	t_pcb *received_pcb = create_pcb();
	stream_take_pcb(petition, received_pcb);
	putToReady(received_pcb);
	sem_post(&exec_to_ready);
	return false;
}

bool (*kernel_handlers[7])(t_packet *petition, int console_socket) =
{
	receive_process,
	io_op,
	exit_op,
	NULL,
	NULL,
	NULL,
	interrupt_ready

};

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
		serve = kernel_handlers[packet->header](packet, client_socket);
		packet_destroy(packet);
	}
}

int getIO(t_pcb *pcb) {
	t_instruction *instruccion;
	instruccion = list_get(pcb->instructions, pcb->program_counter);
	uint32_t n = *((uint32_t*) list_get(instruccion->params, 0));
	return n;
}

void terminate_kernel(bool error) {
	log_destroy(logger);
	config_destroy(config);
	close(cpu_interrupt_socket);
	close(cpu_dispatch_socket);
	close(memory_server_socket);
	exit(error ? EXIT_FAILURE : EXIT_SUCCESS);
}

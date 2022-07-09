#include "kernel.h"

int main(void)
{
	logger = log_create("./cfg/kernel.log", "KERNEL", 1, LOG_LEVEL_INFO);
	config = getKernelConfig("./cfg/kernel.config");

	// Inicializar estructuras de estado
	newQ = pQueue_create();
	readyQ = pQueue_create();
	blockedQ = pQueue_create();
	suspended_blockQ = pQueue_create();
	suspended_readyQ = pQueue_create();
	exitQ = pQueue_create();

	// Inicializar semaforo de multiprocesamiento
	sem_init(&sem_multiprogram, 0, config->multiprogrammingLevel);
	sem_init(&freeCpu, 0, 1);
	// cupos_libres = config->multiprogrammingLevel;
	// pthread_mutex_init(&mutex_cupos, NULL);

	sem_init(&new_for_ready, 0, 0);
	sem_init(&suspended_for_ready, 0, 0);
	sem_init(&ready_for_exec, 0, 0);
	sem_init(&longTermSemCall, 0, 0);

	sem_init(&any_blocked, 0, 0);

	sem_init(&bloquear, 0, 0);

	if (config == NULL)
	{
		log_error(logger, "Config failed to load");
		terminate_kernel(true);
	}

	// Setteo de Algoritmo de Planificacion
	if (!strcmp(config->schedulerAlgorithm, "SRT"))
	{
		sortingAlgorithm = SRT;
	}
	else if (!strcmp(config->schedulerAlgorithm, "FIFO"))
	{
		sortingAlgorithm = FIFO;
	}
	else
	{
		log_warning(logger,
					"Wrong scheduler algorithm set in config --> Using FIFO");
	}

	cpu_dispatch_socket = connect_to(config->cpuIP, config->cpuPortDispatch);
	cpu_interrupt_socket = connect_to(config->cpuIP, config->cpuPortInterrupt);

	if (!cpu_dispatch_socket || !cpu_interrupt_socket)
	{
		terminate_kernel(true);
	}

	log_info(logger, "Kernel connected to CPU");

	// Creacion de server
	int server_socket = create_server(config->kernelPort);
	if (!server_socket)
	{
		terminate_kernel(true);
	}
	log_info(logger, "Kernel ready for console");

	// Inicializar hilos
	// Inicializar Planificador de largo plazo
	pthread_create(&newToReadyThread, 0, newToReady, NULL);
	pthread_detach(newToReadyThread);
	pthread_create(&suspendedToReadyThread, 0, newToReady, NULL);
	pthread_detach(suspendedToReadyThread);

	// Inicializar Planificador de corto plazo
	pthread_create(&readyToExecThread, 0, readyToExec, NULL);
	pthread_detach(readyToExecThread);

	// Finalizador de procesos
	pthread_create(&exitProcessThread, 0, exit_process, NULL);
	pthread_detach(exitProcessThread);

	// Inicializar cpu dispatch listener
	pthread_create(&cpuDispatchThread, 0, cpu_dispatch_listener, NULL);
	pthread_detach(cpuDispatchThread);

	pthread_create(&io_thread, NULL, io_t, NULL);
	pthread_detach(io_thread);

	// Inicializo condition variable para despertar al planificador de mediano plazo
	pthread_cond_init(&cond_mediumTerm, NULL);
	pthread_mutex_init(&mutex_mediumTerm, NULL);

	/*
	 memory_server_socket = connect_to(config->memoryIP, config->memoryPort);
	 */

	while (1)
	{
		server_listen(server_socket, header_handler);
	}

	terminate_kernel(false);
}

void *cpu_dispatch_listener(void *args)
{
	// sem_wait(&bloquear);
	while (1)
	{
		// printf("a\n");
		header_handler(cpu_dispatch_socket);
	}
}

void *exit_process(void *args)
{
	t_pcb *pcb = NULL;
	while (1)
	{
		pcb = pQueue_take(exitQ);
		// avisar a consola que exit
	}
}

void *readyToExec(void *args)
{
	t_pcb *pcb = NULL;
	while (1)
	{
		sem_wait(&ready_for_exec);
		sem_wait(&freeCpu);
		pcb = pQueue_take(readyQ);

		t_packet *pcb_packet = create_packet(PCB_TO_CPU, 64);
		stream_add_pcb(pcb_packet, pcb);

		if (cpu_dispatch_socket != -1)
		{
			socket_send_packet(cpu_dispatch_socket, pcb_packet);
		}

		packet_destroy(pcb_packet);
		pthread_mutex_lock(&mutex_log);
		log_info(logger, "PID #%d [READY] --> CPU", pcb->pid);
		pthread_mutex_unlock(&mutex_log);
	}
}

void *newToReady(void *args)
{ // Hilo del largo plazo, toma un proceso de new y lo pasa a ready
	t_pcb *pcb = NULL;
	while (1)
	{
		sem_wait(&new_for_ready);
		sem_wait(&sem_multiprogram);

		pcb = (t_pcb *)pQueue_take(newQ);

		// sem_wait(&longTermSemCall);
		// pthread_mutex_lock(&mutex_mediumTerm);
		// pcb = (t_pcb*) pQueue_take(newQ);

		// manejar memoria, creacion de estructuras

		/*
		 t_packet *memory_info = create_packet(MEMORY_PID, 64);
		 stream_add_UINT32(memory_info->payload, pcb->size);

		 if (memory_server_socket != -1) {
		 socket_send_packet(memory_server_socket, memory_info);
		 }

		 packet_destroy(memory_info);

		 // Recibir valor de Tabla
		 // Actualizar PCB
		 */

		putToReady(pcb);

		pthread_mutex_lock(&mutex_log);
		log_info(logger, "Long Term Scheduler: PID #%d [NEW] --> Ready queue", pcb->pid);
		pthread_mutex_unlock(&mutex_log);

		pthread_cond_signal(&cond_mediumTerm);
		pthread_mutex_unlock(&mutex_mediumTerm);
	}
}

void *suspendedToReady(void *args)
{
	// Hilo del largo plazo, toma un proceso de new y lo pasa a ready
	t_pcb *pcb = NULL;
	while (1)
	{
		sem_wait(&suspended_for_ready);
		sem_wait(&sem_multiprogram);

		pcb = pQueue_take(suspended_readyQ);
		// manejar memoria, sacar de suspendido y traer a "ram"
		putToReady(pcb);

		pthread_mutex_lock(&mutex_log);
		log_info(logger, "Long Term Scheduler: PID #%d [SUSPENDED READY] --> Ready queue", pcb->pid);
		pthread_mutex_unlock(&mutex_log);

		pthread_cond_signal(&cond_mediumTerm);
		pthread_mutex_unlock(&mutex_mediumTerm);
	}
}

void *io_t(void *args)
{
	t_pcb *pcb;
	int milisecs;
	int sobra;

	// int memorySocket = connectToServer(config->memoryIP, config->memoryPort);

	// t_packet* suspendRequest;

	while (1)
	{
		sem_wait(&any_blocked);
		if (!pQueue_isEmpty(blockedQ))
		{
			pcb = pQueue_peek(blockedQ);

			clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &now);
			milisecs = (config->maxBlockedTime) - (time_to_ms(now) - (pcb->blocked_time));
			sobra = milisecs - pcb->nextIO;

			if (sobra >= 0)
			{
				usleep(pcb->nextIO);
				pcb = pQueue_take(blockedQ);
				putToReady(pcb);
			}
			else
			{
				usleep(milisecs);
				pcb = pQueue_take(blockedQ);
				pcb->nextIO = pcb->nextIO - milisecs;
				// Notifica a memoria de la suspension
				/*t_packet* suspendRequest = create_packet(SUSPEND, INITIAL_STREAM_SIZE);
				stream_add_UINT32(suspendRequest->payload, pcb->pid);
				socket_send_packet(memory_server_socket, suspendRequest);
				packet_destroy(suspendRequest);*/
				pQueue_put(suspended_blockQ, pcb);
				sem_post(&sem_multiprogram);
				// pthread_mutex_lock(&mutex_cupos);
				// cupos_libres++;
				// pthread_mutex_unlock(&mutex_cupos);
				pthread_mutex_lock(&mutex_log);

				log_info(logger,
						 "Medium Term Scheduler: PID #%d [BLOCKED] --> Suspended Blocked queue",
						 pcb->pid);
				pthread_mutex_unlock(&mutex_log);
			}
		}
		else
		{
			pcb = pQueue_peek(suspended_blockQ);
			usleep(pcb->nextIO);
			blocked_to_ready(suspended_blockQ, suspended_readyQ);
		}
	}
}

void blocked_to_ready(t_pQueue *origin, t_pQueue *destination)
{
	t_pcb *pcb = pQueue_take(origin);
	pQueue_put(destination, pcb);
	sem_post(&suspended_for_ready);
}

void putToReady(t_pcb *pcb)
{
	pQueue_put(readyQ, (void *)pcb);
	if (sortingAlgorithm == FIFO)
	{
		pthread_mutex_lock(&mutex_log);
		log_info(logger, "Short Term Scheduler: FIFO, skipping replan...");
		pthread_mutex_unlock(&mutex_log);
	}

	if (sortingAlgorithm == SRT)
	{
		if (cpu_interrupt_socket != -1)
		{
			socket_send_header(cpu_interrupt_socket, INTERRUPT);
		}

		pthread_mutex_lock(&mutex_log);
		log_info(logger, "Short Term Scheduler: SJF, replanning...");
		pthread_mutex_unlock(&mutex_log);

		pQueue_sort(readyQ, SJF_sort);
	}
	sem_post(&ready_for_exec);
}

bool receive_process(t_packet *petition, int console_socket)
{
	t_process *received_process = create_process();
	stream_take_process(petition, received_process);

	if (!!received_process)
	{
		t_pcb *pcb = create_pcb();

		list_add_all(pcb->instructions, received_process->instructions);
		pcb->size = received_process->size;
		// NO DESTRUIR HASTA QUE ENCONTREMOS LA FORMA DE COPIAR CORRECTAMENTE
		// process_destroy(received_process);

		log_info(logger, "Received process sized %d with %d instructions",
				 pcb->size, list_size(pcb->instructions));

		// ESTO ROMPERIA SI DESTRUIMOS EL RECEIVED_PROCESS PORQUE LAS INSTRUCTIONS SE COPIAN MAL
		// list_iterate(pcb->instructions, _log_instruction);

		pid++;
		pcb->pid = pid;
		pcb->client_socket = console_socket;
		pcb->program_counter = 0;
		pcb->burst_estimation = config->initialEstimate;

		pQueue_put(newQ, (void *)pcb);
		pthread_mutex_lock(&mutex_log);
		log_info(logger, "PID #%d --> New queue", pcb->pid);
		pthread_mutex_unlock(&mutex_log);

		sem_post(&new_for_ready);
	}

	return true;
}

bool io_op(t_packet *petition, int cpu_socket)
{
	sem_post(&freeCpu);
	t_pcb *received_pcb = create_pcb();
	stream_take_pcb(petition, received_pcb);

	if (!!received_pcb)
	{
		clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &now);
		received_pcb->blocked_time = time_to_ms(now);
		pQueue_put(blockedQ, (void *)received_pcb);
		sem_post(&any_blocked);
	}
	return true;
}

bool exit_op(t_packet *petition, int cpu_socket)
{
	t_pcb *received_pcb = create_pcb();
	stream_take_pcb(petition, received_pcb);

	if (!!received_pcb)
	{
		log_info(logger, "PID #%d CPU --> Exit queue", received_pcb->pid);
		pQueue_put(exitQ, (void *)received_pcb);
		sem_post(&freeCpu);
		// debe haber un post al sem de exit, donde se limpien verdaderamente los espacios de memoria
		// y recien ahi se libere el multiprogram
		sem_post(&sem_multiprogram);

		socket_send_header(received_pcb->client_socket, PROCESS_OK);
		return true;
	}
	return false;
}

bool (*kernel_handlers[3])(t_packet *petition, int console_socket) =
	{
		receive_process,
		io_op,
		exit_op,
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
		serve = kernel_handlers[packet->header](packet, client_socket);
		packet_destroy(packet);
	}
	return 0;
}

int getIO(t_pcb *pcb)
{
	t_instruction *instruccion;
	instruccion = list_get(pcb->instructions, pcb->program_counter);
	uint32_t n = *((uint32_t *)list_get(instruccion->params, 0));
	return n;
}

void terminate_kernel(bool error)
{
	log_destroy(logger);
	destroyKernelConfig(config);
	if (cpu_interrupt_socket)
		close(cpu_interrupt_socket);
	if (cpu_dispatch_socket)
		close(cpu_dispatch_socket);
	if (memory_server_socket)
		close(memory_server_socket);
	exit(error ? EXIT_FAILURE : EXIT_SUCCESS);
}

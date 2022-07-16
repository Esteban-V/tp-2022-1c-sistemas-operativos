#include "kernel.h"

int main(void)
{
	logger = log_create("./cfg/kernel-final.log", "KERNEL", 1, LOG_LEVEL_INFO);
	kernelConfig = getKernelConfig("./cfg/kernel.config");

	// Inicializar estructuras de estado
	newQ = pQueue_create();
	readyQ = pQueue_create();
	memoryWaitQ = pQueue_create();
	blockedQ = pQueue_create();
	suspended_blockQ = pQueue_create();
	suspended_readyQ = pQueue_create();
	exitQ = pQueue_create();

	// multiproc
	sem_init(&sem_multiprogram, 0, kernelConfig->multiprogrammingLevel);
	sem_init(&freeCpu, 0, 1);

	sem_init(&somethingToReadyInitialCondition, 0, 0);

	sem_init(&new_for_ready, 0, 0);

	sem_init(&suspended_for_ready, 0, 0);
	sem_init(&ready_for_exec, 0, 0);

	sem_init(&pcb_table_ready, 0, 0);

	sem_init(&longTermSemCall, 0, 0);
	sem_init(&any_blocked, 0, 0);
	sem_init(&bloquear, 0, 0);

	if (kernelConfig == NULL)
	{
		pthread_mutex_lock(&mutex_log);
		log_error(logger, "Config Failed to Load");
		pthread_mutex_unlock(&mutex_log);

		terminate_kernel(true);
	}

	if (!strcmp(kernelConfig->schedulerAlgorithm, "SRT"))
	{
		sortingAlgorithm = SRT;
	}
	else if (!strcmp(kernelConfig->schedulerAlgorithm, "FIFO"))
	{
		sortingAlgorithm = FIFO;
	}
	else
	{
		pthread_mutex_lock(&mutex_log);
		log_warning(logger,
					"Wrong scheduler algorithm set in config --> Using FIFO");
		pthread_mutex_unlock(&mutex_log);
	}

	cpu_dispatch_socket = connect_to(kernelConfig->cpuIP, kernelConfig->cpuPortDispatch);
	cpu_interrupt_socket = connect_to(kernelConfig->cpuIP, kernelConfig->cpuPortInterrupt);
	memory_socket = connect_to(kernelConfig->memoryIP, kernelConfig->memoryPort);

	if (!cpu_dispatch_socket || !cpu_interrupt_socket || !memory_socket)
	{
		terminate_kernel(true);
	}

	pthread_mutex_lock(&mutex_log);
	log_info(logger, "Kernel connected to CPU and memory");
	pthread_mutex_unlock(&mutex_log);

	// Creacion de server
	server_socket = create_server(kernelConfig->listenPort);

	if (!server_socket)
	{
		terminate_kernel(true);
	}

	pthread_mutex_lock(&mutex_log);
	log_info(logger, "Kernel ready for console");
	pthread_mutex_unlock(&mutex_log);

	// Inicializar hilos
	// Inicializar Planificador de largo plazo

	// Inicializar Planificador de corto plazo
	pthread_create(&readyToExecThread, 0, readyToExec, NULL);
	pthread_detach(readyToExecThread);

	// Finalizador de procesos
	pthread_create(&exitProcessThread, 0, exit_process, NULL);
	pthread_detach(exitProcessThread);

	// Inicializar cpu dispatch listener
	pthread_create(&cpuDispatchThread, 0, cpu_dispatch_listener, NULL);
	pthread_detach(cpuDispatchThread);

	// Inicializar memory listener
	pthread_create(&memoryThread, 0, memory_listener, NULL);
	pthread_detach(memoryThread);

	pthread_create(&io_thread, NULL, io_t, NULL);
	pthread_detach(io_thread);

	// Inicializo condition variable para despertar al planificador de mediano plazo
	pthread_mutex_init(&mutexToReady, NULL);

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
		header_handler(cpu_dispatch_socket);
	}
}

void *memory_listener(void *args)
{
	// sem_wait(&bloquear);
	while (1)
	{
		header_handler(memory_socket);
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

		t_packet *pcb_packet = create_packet(PCB_TO_CPU, INITIAL_STREAM_SIZE);
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

void *newToReady()
{
	t_pcb *pcb = NULL;
	sem_wait(&sem_multiprogram);

	pcb = (t_pcb *)pQueue_take(newQ);

	// Pide a memoria creacion de estructuras segun pid y size
	t_packet *pid_packet = create_packet(PROCESS_NEW, INITIAL_STREAM_SIZE);
	stream_add_UINT32(pid_packet->payload, pcb->pid);
	stream_add_UINT32(pid_packet->payload, pcb->size);

	pQueue_put(memoryWaitQ, (void *)pcb);

	if (memory_socket != -1)
	{
		socket_send_packet(memory_socket, pid_packet);
	}

	packet_destroy(pid_packet);

	sem_wait(&pcb_table_ready);

	put_to_ready(pcb);

	pthread_mutex_lock(&mutex_log);
	log_info(logger, "Long Term Scheduler: PID #%d [NEW] --> Ready queue", pcb->pid);
	pthread_mutex_unlock(&mutex_log);

	pthread_mutex_unlock(&mutexToReady);

	return 0;
}

void *suspendedToReady()
{
	t_pcb *pcb = NULL;
	sem_wait(&sem_multiprogram);
	pcb = pQueue_take(suspended_readyQ);

	// Manejar memoria, sacar de suspendido y traer a "ram"

	put_to_ready(pcb);

	pthread_mutex_lock(&mutex_log);
	log_info(logger, "Long Term Scheduler: PID #%d [SUSPENDED READY] --> Ready queue", pcb->pid);
	pthread_mutex_unlock(&mutex_log);

	pthread_mutex_unlock(&mutexToReady);
	return 0;
}

void *toReady(void *args)
{
	while (1)
	{
		pthread_mutex_lock(&mutexToReady);

		sem_wait(&somethingToReadyInitialCondition); // falta agregarlo a cuando algo esta listo en bloqueado o suspendido listo
		int freeSpots;
		sem_getvalue(&sem_multiprogram, &freeSpots);
		if (freeSpots != 0)
		{
			pthread_mutex_unlock(&mutexToReady);
			continue;
		}

		if (!pQueue_isEmpty(suspended_readyQ))
		{
			suspendedToReady();
		}
		else if (!pQueue_isEmpty(newQ))
		{
			newToReady();
		}
		else
		{
			pthread_mutex_unlock(&mutexToReady);
			continue;
		}
	}
}

void *io_t(void *args)
{
	t_pcb *pcb;
	int time_blocked;
	int sleep_ms;
	int remaining_io_time;

	// t_packet* suspendRequest;

	while (1)
	{
		sem_wait(&any_blocked);
		if (!pQueue_isEmpty(blockedQ))
		{
			pcb = pQueue_peek(blockedQ);

			clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &now);

			// Tiempo desde su bloqueo
			time_blocked = time_to_ms(now) - pcb->blocked_time;

			// Tiempo a bloquear en caso de que
			sleep_ms = kernelConfig->maxBlockedTime - time_blocked;

			// Tiempo que le faltaria de IO
			remaining_io_time = sleep_ms - pcb->pending_io_time;

			// Tiempo faltante supera tiempo maximo seteado en config
			if (remaining_io_time >= 0)
			{
				usleep(pcb->pending_io_time);
				pcb = pQueue_take(blockedQ);
				put_to_ready(pcb);
			}
			else
			{
				usleep(sleep_ms);
				pcb = pQueue_take(blockedQ);
				pcb->pending_io_time = pcb->pending_io_time - sleep_ms;

				// Notifica a memoria de la suspension

				t_packet *suspendRequest = create_packet(SUSPEND, INITIAL_STREAM_SIZE);
				stream_add_UINT32(suspendRequest->payload, pcb->pid);
				socket_send_packet(memory_socket, suspendRequest);
				packet_destroy(suspendRequest);

				pQueue_put(suspended_blockQ, pcb);
				sem_post(&any_blocked);
				sem_post(&sem_multiprogram);
				sem_post(&somethingToReadyInitialCondition);
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
			usleep(pcb->pending_io_time);
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

void put_to_ready(t_pcb *pcb)
{
	pQueue_put(readyQ, (void *)pcb);
	if (sortingAlgorithm == FIFO)
	{
		pthread_mutex_lock(&mutex_log);
		log_info(logger, "Short Term Scheduler: FIFO ; Skipping Replan...");
		pthread_mutex_unlock(&mutex_log);
	}

	if (sortingAlgorithm == SRT)
	{
		if (cpu_interrupt_socket != -1)
		{
			socket_send_header(cpu_interrupt_socket, INTERRUPT);
		}

		pthread_mutex_lock(&mutex_log);
		log_info(logger, "Short Term Scheduler: SJF -> Replanning...");
		pthread_mutex_unlock(&mutex_log);

		pQueue_sort(readyQ, SJF_sort);
	}

	sem_post(&ready_for_exec);
}

bool receive_table_index(t_packet *petition, int mem_socket)
{
	uint32_t pid = stream_take_UINT32(petition->payload);
	uint32_t level1_table_index = stream_take_UINT32(petition->payload);

	void _page_table_to_pid(void *elem)
	{
		t_pcb *pcb = (t_pcb *)elem;
		// Encontrar el pcb correspondiente al pid
		if (pcb->pid == pid)
		{
			// Almacenar puntero a tabla de paginas nivel 1 dado por memoria
			pcb->page_table = level1_table_index;
		}
	};

	pQueue_iterate(memoryWaitQ, _page_table_to_pid);

	// Avisar de pcb listo para memoria
	sem_post(&pcb_table_ready);
	return false;
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

		pthread_mutex_lock(&mutex_log);
		log_info(logger, "Received %d sized process with %d instructions",
				 pcb->size, list_size(pcb->instructions));
		pthread_mutex_unlock(&mutex_log);

		// ESTO ROMPERIA SI DESTRUIMOS EL RECEIVED_PROCESS PORQUE LAS INSTRUCTIONS SE COPIAN MAL
		// list_iterate(pcb->instructions, _log_instruction);

		pid++;
		pcb->pid = pid;
		pcb->client_socket = console_socket;
		pcb->program_counter = 0;
		pcb->burst_estimation = kernelConfig->initialEstimate;

		pQueue_put(newQ, (void *)pcb);

		pthread_mutex_lock(&mutex_log);
		log_info(logger, "PID #%d --> New queue", pcb->pid);
		pthread_mutex_unlock(&mutex_log);
		sem_post(&somethingToReadyInitialCondition);
		sem_post(&new_for_ready); // se saca
	}

	return true;
}

bool io_op(t_packet *petition, int cpu_socket)
{
	t_pcb *received_pcb = create_pcb();
	stream_take_pcb(petition, received_pcb);

	if (!!received_pcb)
	{
		clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &now);
		received_pcb->blocked_time = time_to_ms(now);
		pQueue_put(blockedQ, (void *)received_pcb);
		sem_post(&freeCpu);
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
		pthread_mutex_lock(&mutex_log);
		log_info(logger, "PID #%d CPU --> Exit queue", received_pcb->pid);
		pthread_mutex_unlock(&mutex_log);

		pQueue_put(exitQ, (void *)received_pcb);
		sem_post(&freeCpu);
		// debe haber un post al sem de exit, donde se limpien verdaderamente los espacios de memoria
		// y recien ahi se libere el multiprogram
		sem_post(&sem_multiprogram);
		sem_wait(&somethingToReadyInitialCondition);

		socket_send_header(received_pcb->client_socket, PROCESS_OK);
		return true;
	}
	return false;
}

bool handle_interruption(t_packet *petition, int cpu_socket)
{
	t_pcb *received_pcb = create_pcb();
	stream_take_pcb(petition, received_pcb);

	if (!!received_pcb)
	{
		pthread_mutex_lock(&mutex_log);
		log_info(logger, "PID #%d --> Desalojado de CPU", received_pcb->pid);
		pthread_mutex_unlock(&mutex_log);

		put_to_ready(received_pcb);
		sem_post(&freeCpu);
		// debe haber un post al sem de exit, donde se limpien verdaderamente los espacios de memoria
		// y recien ahi se libere el multiprogram
		sem_post(&sem_multiprogram);
		sem_wait(&somethingToReadyInitialCondition);
	}

	return true;
}

bool (*kernel_handlers[6])(t_packet *petition, int console_socket) =
	{
		// NEW_PROCESS
		receive_process,
		// IO_CALL
		io_op,
		// EXIT_CALL
		exit_op,
		// INTERRUPT_DISPATCH
		handle_interruption,
		// SUSPEND
		NULL,
		// PROCESS_MEMORY_READY
		receive_table_index,
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
	destroyKernelConfig(kernelConfig);
	if (cpu_interrupt_socket)
		close(cpu_interrupt_socket);
	if (cpu_dispatch_socket)
		close(cpu_dispatch_socket);
	if (memory_socket)
		close(memory_socket);
	exit(error ? EXIT_FAILURE : EXIT_SUCCESS);
}

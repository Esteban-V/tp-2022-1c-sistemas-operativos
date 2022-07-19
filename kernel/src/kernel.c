#include "kernel.h"

int main(void)
{
	logger = log_create("./cfg/kernel-final.log", "KERNEL", 1, LOG_LEVEL_INFO);
	kernelConfig = getKernelConfig("./cfg/kernel.config");

	// Inicializar estructuras de estado
	new_q = pQueue_create();
	ready_q = pQueue_create();
	memory_init_q = pQueue_create();
	memory_exit_q = pQueue_create();
	blocked_q = pQueue_create();
	suspended_block_q = pQueue_create();
	suspended_ready_q = pQueue_create();
	exit_q = pQueue_create();

	sem_init(&sem_multiprogram, 0, kernelConfig->multiprogrammingLevel);
	sem_init(&cpu_free, 0, 1);
	sem_init(&interrupt_ready, 0, 0);
	sem_init(&process_for_IO, 0, 0);
	sem_init(&any_for_ready, 0, 0);
	sem_init(&ready_for_exec, 0, 0);

	sem_init(&pcb_table_ready, 0, 0);

	if (kernelConfig == NULL)
	{
		log_error(logger, "Config failed to load");
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
		log_warning(logger,
					"Wrong scheduler algorithm set in config --> Using FIFO");
	}

	cpu_dispatch_socket = connect_to(kernelConfig->cpuIP, kernelConfig->cpuPortDispatch);
	cpu_interrupt_socket = connect_to(kernelConfig->cpuIP, kernelConfig->cpuPortInterrupt);
	memory_socket = connect_to(kernelConfig->memoryIP, kernelConfig->memoryPort);

	if (cpu_dispatch_socket == -1 || cpu_interrupt_socket == -1 || memory_socket == -1)
	{
		terminate_kernel(true);
	}

	log_info(logger, "Kernel connected to CPU and memory");

	// Creacion de server
	server_socket = create_server(kernelConfig->listenPort);

	if (server_socket == -1)
	{
		terminate_kernel(true);
	}

	log_info(logger, "Kernel server ready for console");

	// Planificador de mediano plazo
	pthread_create(&any_to_ready_t, 0, to_ready, NULL);
	pthread_detach(any_to_ready_t);

	// Planificador de corto plazo
	pthread_create(&ready_to_exec_t, 0, to_exec, NULL);
	pthread_detach(ready_to_exec_t);

	// Planificador de largo plazo
	pthread_create(&exit_process_t, 0, exit_process, NULL);
	pthread_detach(exit_process_t);

	// CPU dispatch listener
	pthread_create(&cpu_dispatch_t, 0, cpu_dispatch_listener, NULL);
	pthread_detach(cpu_dispatch_t);

	// Memory listener
	pthread_create(&memory_t, 0, memory_listener, NULL);
	pthread_detach(memory_t);

	pthread_create(&io_t, 0, io_listener, NULL);
	pthread_detach(io_t);

	while (1)
	{
		server_listen(server_socket, header_handler);
	}

	terminate_kernel(false);
}

void *cpu_dispatch_listener(void *args)
{
	while (1)
	{
		header_handler(cpu_dispatch_socket);
	}
}

void *memory_listener(void *args)
{
	while (1)
	{
		header_handler(memory_socket);
	}
}

void *io_listener()
{
	t_pcb *pcb;
	int time_blocked;
	int sleep_ms;
	int remaining_io_time;

	while (1)
	{
		sem_wait(&process_for_IO);
		if (!pQueue_isEmpty(blocked_q))
		{
			pcb = pQueue_take(blocked_q);

			// Tiempo que ya estuvo bloqueado
			struct timespec curr_time;
			clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &curr_time);
			time_blocked = time_to_ms(curr_time) - pcb->blocked_time;

			// Tiempo que puede seguir bloqueado (restando del maximo lo que "ya estuvo")
			sleep_ms = kernelConfig->maxBlockedTime - time_blocked;

			// Tiempo extra luego de hacer su io
			remaining_io_time = sleep_ms - pcb->pending_io_time;

			if (remaining_io_time >= 0)
			{
				pthread_mutex_lock(&mutex_log);
				log_info(logger, "PID #%d --> I/O burst %dms", pcb->pid, pcb->pending_io_time);
				pthread_mutex_unlock(&mutex_log);

				// Duerme por el tiempo faltante, sin superar el maximo
				usleep(pcb->pending_io_time * 1000);

				put_to_ready(pcb);
			}
			else
			{
				// Tiempo faltante supera tiempo maximo
				pthread_mutex_lock(&mutex_log);
				log_warning(logger, "Burst exceeds %dms max", kernelConfig->maxBlockedTime);
				log_info(logger, "PID #%d --> I/O burst %dms", sleep_ms);
				pthread_mutex_unlock(&mutex_log);

				// Duerme el maximo
				usleep(sleep_ms * 1000);

				pthread_mutex_lock(&mutex_log);
				log_info(logger, "Finished I/O burst");
				pthread_mutex_unlock(&mutex_log);

				// Se actualiza lo que le queda (por instruccion I/O) restandole lo que ya "durmio"
				pcb->pending_io_time = pcb->pending_io_time - sleep_ms;

				// Pide suspension a memoria
				t_packet *suspend_packet = create_packet(PROCESS_SUSPEND, INITIAL_STREAM_SIZE);
				stream_add_UINT32(suspend_packet->payload, pcb->pid);
				stream_add_UINT32(suspend_packet->payload, pcb->page_table);

				if (memory_socket != -1)
				{
					socket_send_packet(memory_socket, suspend_packet);
				}

				packet_destroy(suspend_packet);

				// Esperar suspension exitosa
				// Se libera la memoria (sube multiprogramacion)
				pthread_mutex_lock(&mutex_log);
				log_info(logger, "%dms left", pcb->pending_io_time);
				pthread_mutex_unlock(&mutex_log);

				pQueue_put(suspended_block_q, (void *)pcb);
				sem_post(&sem_multiprogram);

				sem_post(&process_for_IO);

				pthread_mutex_lock(&mutex_log);
				log_info(logger, "Medium Term Scheduler: PID #%d [BLOCKED] --> Suspended blocked queue", pcb->pid);
				pthread_mutex_unlock(&mutex_log);
			}
		}
		else
		{
			pthread_mutex_lock(&mutex_log);
			log_info(logger, "Medium Term Scheduler: PID #%d [BLOCKED] --> Suspended blocked queue", pcb->pid);
			pthread_mutex_unlock(&mutex_log);

			pcb = pQueue_take(suspended_block_q);

			usleep(pcb->pending_io_time * 1000);

			pthread_mutex_lock(&mutex_log);
			log_info(logger, "Finished I/O burst");
			log_info(logger, "Medium Term Scheduler: PID #%d [SUSPENDED BLOCKED] --> Suspended ready queue", pcb->pid);
			pthread_mutex_unlock(&mutex_log);

			pQueue_put(suspended_ready_q, (void *)pcb);

			// blocked_to_ready(suspended_block_q, suspended_ready_q);
			sem_post(&any_for_ready);
		}
	}
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

		pthread_mutex_lock(&mutex_log);
		log_info(logger, "Received %d sized process with %d instructions",
				 pcb->size, list_size(pcb->instructions));
		pthread_mutex_unlock(&mutex_log);

		pid++;
		pcb->pid = pid;
		pcb->client_socket = console_socket;
		pcb->program_counter = 0;
		pcb->burst_estimation = kernelConfig->initialEstimate;
		pcb->left_burst_estimation = kernelConfig->initialEstimate;

		pQueue_put(new_q, (void *)pcb);

		pthread_mutex_lock(&mutex_log);
		log_info(logger, "PID #%d --> New queue", pcb->pid);
		pthread_mutex_unlock(&mutex_log);

		sem_post(&any_for_ready);
	}

	return true;
}

void *exit_process(void *args)
{
	t_pcb *pcb = NULL;
	while (1)
	{
		pcb = (t_pcb *)pQueue_take(exit_q);

		t_packet *exit_request = create_packet(PROCESS_EXIT, INITIAL_STREAM_SIZE);
		stream_add_UINT32(exit_request->payload, pcb->pid);
		stream_add_UINT32(exit_request->payload, pcb->page_table);

		if (memory_socket != -1)
		{
			socket_send_packet(memory_socket, exit_request);
		}
		packet_destroy(exit_request);

		pQueue_put(memory_exit_q, (void *)pcb);
		sem_post(&sem_multiprogram);
	}
}

void *to_exec()
{
	t_pcb *pcb = NULL;
	while (1)
	{
		sem_wait(&ready_for_exec);
		sem_wait(&cpu_free);
		pcb = pQueue_take(ready_q);

		t_packet *pcb_packet = create_packet(PCB_TO_CPU, INITIAL_STREAM_SIZE);
		stream_add_pcb(pcb_packet, pcb);

		if (cpu_dispatch_socket != -1)
		{
			socket_send_packet(cpu_dispatch_socket, pcb_packet);
		}
		clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &toExec);

		packet_destroy(pcb_packet);

		pthread_mutex_lock(&mutex_log);
		log_info(logger, "PID #%d [READY] --> CPU", pcb->pid);
		pthread_mutex_unlock(&mutex_log);
	}
}

void *new_to_ready()
{
	t_pcb *pcb = NULL;

	pcb = (t_pcb *)pQueue_take(new_q);

	// Pide a memoria creacion de estructuras segun pid y size
	t_packet *pid_packet = create_packet(PROCESS_NEW, INITIAL_STREAM_SIZE);
	stream_add_UINT32(pid_packet->payload, pcb->pid);
	stream_add_UINT32(pid_packet->payload, pcb->size);

	if (memory_socket != -1)
	{
		socket_send_packet(memory_socket, pid_packet);
	}

	packet_destroy(pid_packet);

	pQueue_put(memory_init_q, (void *)pcb);
	sem_wait(&pcb_table_ready);

	put_to_ready(pcb);

	pthread_mutex_lock(&mutex_log);
	log_info(logger, "Long Term Scheduler: PID #%d [NEW] --> Ready queue", pcb->pid);
	pthread_mutex_unlock(&mutex_log);

	return 0;
}

void *suspended_to_ready()
{
	t_pcb *pcb = NULL;
	pcb = pQueue_take(suspended_ready_q);

	pthread_mutex_lock(&mutex_log);
	log_info(logger, "Long Term Scheduler: PID #%d [SUSPENDED READY] --> Ready queue", pcb->pid);
	pthread_mutex_unlock(&mutex_log);

	// Manejar memoria, sacar de suspendido y traer a "ram"

	put_to_ready(pcb);

	return 0;
}

void blocked_to_ready(t_pQueue *origin, t_pQueue *destination)
{
	t_pcb *pcb = pQueue_take(origin);
	pQueue_put(destination, (void *)pcb);
}

void put_to_ready(t_pcb *pcb)
{
	pQueue_put(ready_q, (void *)pcb);
	if (sortingAlgorithm == FIFO)
	{
		pthread_mutex_lock(&mutex_log);
		log_info(logger, "Short Term Scheduler: FIFO --> Skipping replan...");
		pthread_mutex_unlock(&mutex_log);
	}

	if (sortingAlgorithm == SRT)
	{
		// Se envia la interrupcion
		// Si se manda otra por "encima" de la anterior, la CPU siempre usa la mas nueva
		if (cpu_interrupt_socket != -1)
		{
			socket_send_header(cpu_interrupt_socket, INTERRUPT);
		}

		// Se espera a que vuelva de CPU
		sem_wait(&interrupt_ready);

		pthread_mutex_lock(&mutex_log);
		log_info(logger, "Short Term Scheduler: SJF --> Replanning...");
		pthread_mutex_unlock(&mutex_log);

		pQueue_sort(ready_q, SJF_sort);
	}

	sem_post(&ready_for_exec);
}

void *to_ready()
{
	while (1)
	{
		sem_wait(&any_for_ready);

		sem_wait(&sem_multiprogram);

		if (!pQueue_isEmpty(suspended_ready_q))
		{
			suspended_to_ready();
		}
		else if (!pQueue_isEmpty(new_q))
		{
			new_to_ready();
		}
	}
}

bool table_index_success(t_packet *petition, int mem_socket)
{
	uint32_t pid = stream_take_UINT32(petition->payload);
	uint32_t level1_table_index = stream_take_UINT32(petition->payload);

	bool found = false;
	void _page_table_to_pid(void *elem)
	{
		t_pcb *pcb = (t_pcb *)elem;
		// Encontrar el pcb correspondiente al pid
		if (pcb->pid == pid)
		{
			// Almacenar puntero a tabla de paginas nivel 1 dado por memoria
			pcb->page_table = (int)level1_table_index;
			found = true;
		}
	};

	pQueue_iterate(memory_init_q, _page_table_to_pid);
	if (found)
	{
		// Avisar de pcb listo para memoria
		sem_post(&pcb_table_ready);
	}
	else
	{
		pthread_mutex_lock(&mutex_log);
		log_error(logger, "Failed to load page table data for PID #%d", pid);
		pthread_mutex_unlock(&mutex_log);

		terminate_kernel(true);
	}

	return false;
}

bool exit_process_success(t_packet *petition, int mem_socket)
{
	uint32_t pid = stream_take_UINT32(petition->payload);

	int console_socket = NULL;
	void _page_table_to_pid(void *elem)
	{
		t_pcb *pcb = (t_pcb *)elem;
		// Encontrar el pcb correspondiente al pid
		if (pcb->pid == pid)
		{
			console_socket = pcb->client_socket;
		}
	};

	pQueue_iterate(memory_exit_q, _page_table_to_pid);
	if (console_socket != NULL)
	{
		socket_send_header(console_socket, PROCESS_OK);
	}
	else
	{
		pthread_mutex_lock(&mutex_log);
		log_error(logger, "Failed to exit PID #%d", pid);
		pthread_mutex_unlock(&mutex_log);

		terminate_kernel(true);
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

		clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &fromExec);

		received_pcb->left_burst_estimation = received_pcb->left_burst_estimation - (time_to_ms(toExec) - time_to_ms(fromExec));

		pQueue_put(ready_q, received_pcb);
		sem_post(&cpu_free);
		sem_post(&interrupt_ready);
	}

	return true;
}

bool io_op(t_packet *petition, int cpu_socket)
{
	// No borrar printf
	printf("Process requested I/O call\n");
	pthread_mutex_lock(&mutex_log);
	log_info(logger, "Process requested I/O call");
	pthread_mutex_unlock(&mutex_log);

	t_pcb *received_pcb = create_pcb();
	stream_take_pcb(petition, received_pcb);

	if (!!received_pcb)
	{
		pthread_mutex_lock(&mutex_log);
		log_info(logger, "PID #%d --> Blocked queue", received_pcb->pid);
		pthread_mutex_unlock(&mutex_log);

		clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &fromExec);

		received_pcb->burst_estimation = (kernelConfig->alpha * (time_to_ms(toExec) - time_to_ms(fromExec))) + ((1 - kernelConfig->alpha) * received_pcb->burst_estimation);
		received_pcb->left_burst_estimation = (kernelConfig->alpha * (time_to_ms(toExec) - time_to_ms(fromExec))) + ((1 - kernelConfig->alpha) * received_pcb->burst_estimation);

		struct timespec blocked_time;
		clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &blocked_time);
		received_pcb->blocked_time = time_to_ms(blocked_time);

		pQueue_put(blocked_q, (void *)received_pcb);

		sem_post(&cpu_free);
		sem_post(&process_for_IO);
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

		pQueue_put(exit_q, (void *)received_pcb);
		sem_post(&cpu_free);
		return true;
	}
	return false;
}

bool (*kernel_handlers[7])(t_packet *petition, int console_socket) =
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
		table_index_success,
		// PROCESS_EXIT_READY
		exit_process_success,
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

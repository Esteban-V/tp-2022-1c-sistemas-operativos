#include "kernel.h"

int main(void)
{
	signal(SIGINT, terminate_kernel);

	logger = log_create("./cfg/kernel-final.log", "KERNEL", 1, LOG_LEVEL_INFO);
	config = getKernelConfig("./cfg/kernel.config");

	// Inicializar estructuras de estado
	new_q = pQueue_create();
	ready_q = pQueue_create();
	memory_init_q = pQueue_create();
	memory_exit_q = pQueue_create();
	blocked_q = pQueue_create();
	memory_suspension_q = pQueue_create();
	suspended_block_q = pQueue_create();
	suspended_ready_q = pQueue_create();
	exit_q = pQueue_create();

	sem_init(&sem_multiprogram, 0, config->multiprogrammingLevel);
	sem_init(&cpu_free, 0, 1);
	sem_init(&interrupt_ready, 0, 0);
	sem_init(&process_for_IO, 0, 0);
	sem_init(&any_for_ready, 0, 0);
	sem_init(&ready_for_exec, 0, 0);
	sem_init(&pcb_table_ready, 0, 0);

	sem_init(&suspension_ready, 0, 1);

	pthread_mutex_init(&execution_mutex, NULL);
	if (config == NULL)
	{
		log_error(logger, "Config failed to load");
		terminate_kernel(true);
	}

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
	memory_socket = connect_to(config->memoryIP, config->memoryPort);

	if (cpu_dispatch_socket == -1 || cpu_interrupt_socket == -1 || memory_socket == -1)
	{
		terminate_kernel(true);
	}

	log_info(logger, "Kernel connected to CPU and memory");

	// Creacion de server
	server_socket = create_server(config->listenPort);

	if (server_socket == -1)
	{
		terminate_kernel(true);
	}

	log_info(logger, "Kernel server ready for console");

	// Planificador de mediano plazo
	pthread_create(&any_to_ready_t, NULL, to_ready, NULL);
	pthread_detach(any_to_ready_t);

	// Planificador de corto plazo
	pthread_create(&ready_to_exec_t, NULL, to_exec, NULL);
	pthread_detach(ready_to_exec_t);

	// Planificador de largo plazo
	pthread_create(&exit_process_t, NULL, exit_process, NULL);
	pthread_detach(exit_process_t);

	// CPU dispatch listener
	pthread_create(&cpu_dispatch_t, NULL, cpu_dispatch_listener, NULL);
	pthread_detach(cpu_dispatch_t);

	// Memory listener
	pthread_create(&memory_t, NULL, memory_listener, NULL);
	pthread_detach(memory_t);

	pthread_create(&io_t, NULL, io_listener, NULL);
	pthread_detach(io_t);

	while (1)
	{
		server_listen(server_socket, packet_handler);
	}
}

void *cpu_dispatch_listener(void *args)
{
	while (1)
	{
		packet_handler((void *)cpu_dispatch_socket);
	}
}

void *memory_listener(void *args)
{
	while (1)
	{
		packet_handler((void *)memory_socket);
	}
}

void *io_listener(void *args)
{
	t_pcb *pcb;
	int time_blocked;
	int sleep_ms;
	int remaining_io_time;

	while (1)
	{
		sem_wait(&process_for_IO);
		if (!pQueue_is_empty(blocked_q))
		{
			pcb = pQueue_take(blocked_q);

			// Tiempo que ya estuvo bloqueado
			struct timespec curr_time;
			clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &curr_time);
			time_blocked = time_to_ms(curr_time) - pcb->blocked_time;

			// Tiempo que puede seguir bloqueado (restando del maximo lo que "ya estuvo")
			sleep_ms = config->maxBlockedTime - time_blocked;

			// Tiempo extra luego de hacer su io
			remaining_io_time = sleep_ms - pcb->pending_io_time;

			if (remaining_io_time >= 0)
			{
				pthread_mutex_lock(&mutex_log);
				log_info(logger, "PID #%d --> I/O burst %dms", pcb->pid, pcb->pending_io_time);
				pthread_mutex_unlock(&mutex_log);

				// Duerme por el tiempo faltante, sin superar el maximo
				usleep(pcb->pending_io_time * 1000);

				pthread_mutex_lock(&mutex_log);
				log_info(logger, "PID #%d [BLOCKED] --> [READY]", pcb->pid);
				pthread_mutex_unlock(&mutex_log);

				put_to_ready(pcb);
			}
			else
			{
				// Tiempo faltante supera tiempo maximo
				pthread_mutex_lock(&mutex_log);
				log_warning(logger, "Process #%d's %dms burst exceeds %dms max", pcb->pid, pcb->pending_io_time, config->maxBlockedTime);
				log_info(logger, "PID #%d --> I/O burst %dms", pcb->pid, sleep_ms);
				pthread_mutex_unlock(&mutex_log);

				// Duerme el maximo
				usleep(sleep_ms * 1000);

				pthread_mutex_lock(&mutex_log);
				log_info(logger, "Process #%d finished I/O burst", pcb->pid);
				pthread_mutex_unlock(&mutex_log);

				// Se actualiza lo que le queda (por instruccion I/O) restandole lo que ya "durmio"
				pcb->pending_io_time = pcb->pending_io_time - sleep_ms;

				// Pide suspension a memoria
				t_packet *suspend_packet = create_packet(PROCESS_SUSPEND, INITIAL_STREAM_SIZE);
				stream_add_UINT32(suspend_packet->payload, pcb->pid);
				stream_add_UINT32(suspend_packet->payload, pcb->page_table);
				stream_add_UINT32(suspend_packet->payload, pcb->frames_index);

				if (memory_socket != -1)
				{
					socket_send_packet(memory_socket, suspend_packet);
				}

				packet_destroy(suspend_packet);

				pQueue_put(memory_suspension_q, (void *)pcb);

				// Esperar suspension exitosa
				sem_wait(&suspension_ready);

				pQueue_put(suspended_block_q, (void *)pcb);

				pthread_mutex_lock(&mutex_log);
				log_info(logger, "Process suspended, %dms left", pcb->pending_io_time);
				pthread_mutex_unlock(&mutex_log);

				sem_post(&process_for_IO);
			}
		}
		else
		{
			pcb = pQueue_take(suspended_block_q);

			usleep(pcb->pending_io_time * 1000);

			pthread_mutex_lock(&mutex_log);
			log_info(logger, "Process #%d finished I/O burst in suspension", pcb->pid);
			log_info(logger, "PID #%d [SUSPENDED BLOCKED] --> [SUSPENDED READY]", pcb->pid);
			pthread_mutex_unlock(&mutex_log);

			pQueue_put(suspended_ready_q, (void *)pcb);

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
		pcb->burst_estimation = config->initialEstimate;
		pcb->left_burst_estimation = config->initialEstimate;

		pQueue_put(new_q, (void *)pcb);

		pthread_mutex_lock(&mutex_log);
		log_info(logger, "PID #%d --> [NEW]", pcb->pid);
		pthread_mutex_unlock(&mutex_log);

		sem_post(&any_for_ready);
	}

	// NO HACER este destroy, porque siguen atados el proceso recibido y el pcb creado (de alguna forma)
	// Ya mande un issue al foro en su momento preguntando y me respondieron "Por que querrias destruir el recibido?"
	// process_destroy(received_process);

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
		stream_add_UINT32(exit_request->payload, pcb->frames_index);

		if (memory_socket != -1)
		{
			socket_send_packet(memory_socket, exit_request);
		}
		packet_destroy(exit_request);

		pQueue_put(memory_exit_q, (void *)pcb);
	}
}

void *to_exec()
{
	t_pcb *pcb = NULL;
	while (1)
	{
		sem_wait(&ready_for_exec);
		sem_wait(&cpu_free);

		pthread_mutex_lock(&execution_mutex);

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
		pcb_destroy(pcb);

		pthread_mutex_unlock(&execution_mutex);
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

	pthread_mutex_lock(&mutex_log);
	log_info(logger, "PID #%d [NEW] --> [READY]", pcb->pid);
	pthread_mutex_unlock(&mutex_log);

	put_to_ready(pcb);
	return 0;
}

void *suspended_to_ready()
{
	t_pcb *pcb = NULL;
	pcb = pQueue_take(suspended_ready_q);

	pthread_mutex_lock(&mutex_log);
	log_info(logger, "TODO: PID #%d [SUSPENDED READY] --> [READY]", pcb->pid);
	pthread_mutex_unlock(&mutex_log);

	// Manejar memoria, sacar de suspendido y traer a "ram"
	put_to_ready(pcb);
}

void put_to_ready(t_pcb *pcb)
{
	pthread_mutex_lock(&execution_mutex);

	pQueue_put(ready_q, (void *)pcb);

	if (sortingAlgorithm == FIFO)
	{
		pthread_mutex_lock(&mutex_log);
		log_info(logger, "FIFO --> Skipping replan...");
		pthread_mutex_unlock(&mutex_log);
	}
	else
	{
		// Se envia la interrupcion
		// Si se manda otra por "encima" de la anterior, la CPU siempre usa la mas nueva
		int freeCpu;
		sem_getvalue(&cpu_free, &freeCpu);

		pthread_mutex_lock(&mutex_log);
		log_warning(logger, "SJF --> Replanning...");
		pthread_mutex_unlock(&mutex_log);

		pQueue_sort(ready_q, SJF_sort);

		if (freeCpu == 0)
		{
			pthread_mutex_lock(&mutex_log);
			log_info(logger, "PID #%d Interruption", pcb->pid);
			pthread_mutex_unlock(&mutex_log);

			socket_send_header(cpu_interrupt_socket, INTERRUPT);

			// Se espera a que vuelva de CPU
			sem_wait(&interrupt_ready);
		}
	}

	pthread_mutex_unlock(&execution_mutex);
	sem_post(&ready_for_exec);
}

void *to_ready()
{
	while (1)
	{

		sem_wait(&any_for_ready);
		sem_wait(&sem_multiprogram);
		if (!pQueue_is_empty(suspended_ready_q))
		{
			suspended_to_ready();
		}
		else if (!pQueue_is_empty(new_q))
		{
			new_to_ready();
		}
	}

	return 0;
}

bool table_index_success(t_packet *petition, int mem_socket)
{
	uint32_t pid = stream_take_UINT32(petition->payload);
	uint32_t level1_table_index = stream_take_UINT32(petition->payload);
	uint32_t process_frame_index = stream_take_UINT32(petition->payload);

	bool found = false;
	void _memory_data_to_pid(void *elem)
	{
		t_pcb *pcb = (t_pcb *)elem;
		// Encontrar el pcb correspondiente al pid
		if (pcb->pid == pid)
		{
			// Almacenar index a tabla de paginas nivel 1 y listado de framess dados por memoria
			pcb->page_table = level1_table_index;
			pcb->frames_index = process_frame_index;
			found = true;
			pQueue_take(memory_init_q);
		}
	};

	pQueue_iterate(memory_init_q, _memory_data_to_pid);

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

bool suspension_success(t_packet *petition, int mem_socket)
{
	uint32_t pid = stream_take_UINT32(petition->payload);

	bool found = false;
	void _find_suspended_pid(void *elem)
	{
		t_pcb *pcb = (t_pcb *)elem;
		// Encontrar el pcb correspondiente al pid
		if (pcb->pid == pid)
		{
			found = true;
			pQueue_take(memory_suspension_q);
		}
	};

	pQueue_iterate(memory_suspension_q, _find_suspended_pid);

	if (found)
	{
		// Avisar de proceso suspendido
		sem_post(&suspension_ready);
		// Se libera la memoria, sube multiprogramacion
		sem_post(&sem_multiprogram);
	}
	else
	{
		pthread_mutex_lock(&mutex_log);
		log_error(logger, "Failed to confirm suspension of PID #%d", pid);
		pthread_mutex_unlock(&mutex_log);

		terminate_kernel(true);
	}

	return false;
}

bool exit_process_success(t_packet *petition, int mem_socket) // posible problema de sems aca
{
	uint32_t pid = stream_take_UINT32(petition->payload);

	int console_socket = 0;
	void _page_table_to_pid(void *elem)
	{
		t_pcb *pcb = (t_pcb *)elem;
		// Encontrar el pcb correspondiente al pid
		if (pcb->pid == pid)
		{
			console_socket = pcb->client_socket;
			pcb_destroy(pcb);
		}
	};

	pQueue_iterate(memory_exit_q, _page_table_to_pid);
	if (console_socket != 0)
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

	sem_post(&sem_multiprogram);
	return false;
}

bool handle_interruption(t_packet *petition, int cpu_socket)
{
	t_pcb *received_pcb = create_pcb();
	stream_take_pcb(petition, received_pcb);

	if (!!received_pcb)
	{
		pthread_mutex_lock(&mutex_log);
		log_info(logger, "Successfully kicked out process #%d from CPU", received_pcb->pid);
		pthread_mutex_unlock(&mutex_log);

		clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &fromExec);

		int ms_passed = time_to_ms(toExec) - time_to_ms(fromExec);
		received_pcb->left_burst_estimation = received_pcb->left_burst_estimation - ms_passed;

		pthread_mutex_lock(&mutex_log);
		log_info(logger, "PID #%d CPU --> [READY] with updated estimate of %dms", received_pcb->pid, received_pcb->left_burst_estimation);
		pthread_mutex_unlock(&mutex_log);

		pQueue_put(ready_q, received_pcb);

		sem_post(&ready_for_exec);
	}

	sem_post(&cpu_free);
	sem_post(&interrupt_ready);

	return true;
}

bool io_op(t_packet *petition, int cpu_socket)
{

	t_pcb *received_pcb = create_pcb();
	stream_take_pcb(petition, received_pcb);

	if (!!received_pcb)
	{
		pthread_mutex_lock(&mutex_log);
		log_info(logger, "Process #%d requested I/O call", received_pcb->pid);
		pthread_mutex_unlock(&mutex_log);

		clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &fromExec);

		int ms_passed = (time_to_ms(toExec) - time_to_ms(fromExec));
		int estimate = (config->alpha * ms_passed) + ((1 - config->alpha) * received_pcb->burst_estimation);

		received_pcb->burst_estimation = estimate;
		received_pcb->left_burst_estimation = estimate;

		struct timespec blocked_time;
		clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &blocked_time);
		received_pcb->blocked_time = time_to_ms(blocked_time);

		pthread_mutex_lock(&mutex_log);
		log_info(logger, "PID #%d CPU --> [BLOCKED] with updated estimate of %dms", received_pcb->pid, received_pcb->left_burst_estimation);
		pthread_mutex_unlock(&mutex_log);

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
		log_info(logger, "PID #%d CPU --> [EXIT]", received_pcb->pid);
		pthread_mutex_unlock(&mutex_log);

		pQueue_put(exit_q, (void *)received_pcb);

		sem_post(&cpu_free);

		return true;
	}
	return false;
}

bool (*kernel_handlers[8])(t_packet *petition, int console_socket) =
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
		// PROCESS_SUSPENSION_READY
		suspension_success,
};

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

		serve = kernel_handlers[packet->header](packet, client_socket);
		packet_destroy(packet);
	}
	return 0;
}

void terminate_kernel(int x)
{
	pQueue_destroy(new_q);
	pQueue_destroy(ready_q);
	pQueue_destroy(memory_init_q);
	pQueue_destroy(memory_exit_q);
	pQueue_destroy(blocked_q);
	pQueue_destroy(memory_suspension_q);
	pQueue_destroy(suspended_block_q);
	pQueue_destroy(suspended_ready_q);
	pQueue_destroy(exit_q);

	pthread_mutex_destroy(&execution_mutex);

	destroyKernelConfig(config);
	if (cpu_interrupt_socket)
		close(cpu_interrupt_socket);
	if (cpu_dispatch_socket)
		close(cpu_dispatch_socket);
	if (memory_socket)
		close(memory_socket);

	log_destroy(logger);

	switch (x)
	{
	case 1:
		exit(EXIT_FAILURE);
	case SIGINT:
		exit(EXIT_SUCCESS);
	}
}

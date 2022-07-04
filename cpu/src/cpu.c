/*
 * cpu.c
 *
 *  Created on: 15 jun. 2022
 *      Author: utnso
 */
#include "cpu.h"

int main()
{
	// Initialize logger
	logger = log_create("./cfg/cpu.log", "CPU", 1, LOG_LEVEL_TRACE);
	config = get_cpu_config("./cfg/cpu.config");

	// Creacion de server
	kernel_dispatch_socket = create_server(config->dispatchListenPort);
	kernel_interrupt_socket = create_server(config->interruptListenPort);
	log_info(logger, "CPU ready for kernel");

	sem_init(&pcb_loaded, 0, 0);
	pthread_mutex_init(&mutex_kernel_socket, NULL);

	pthread_create(&interruptionThread, 0, interruption, NULL);
	pthread_detach(interruptionThread);

	pthread_create(&execThread, 0, cpu_cycle, NULL);
	pthread_detach(execThread);

	while (1)
	{
		server_listen(kernel_dispatch_socket, header_handler);
	}

	log_destroy(logger);
}

void *interruption()
{
	while (1)
	{
		server_listen(kernel_interrupt_socket, header_handler);
	}
}

void pcb_to_kernel(kernel_headers header)
{
	t_packet *pcb_packet = create_packet(header, 64);
	stream_add_pcb(pcb_packet, pcb);

	pthread_mutex_lock(&mutex_kernel_socket);
	if (kernel_client_socket != -1)
	{
		socket_send_packet(kernel_client_socket, pcb_packet);
		pthread_mutex_unlock(&mutex_kernel_socket);

		log_info(logger, "PID #%d CPU --> Kernel", pcb->pid);
	}
	packet_destroy(pcb_packet);
}

bool (*cpu_handlers[2])(t_packet *petition, int console_socket) =
	{
		receivedPcb,
		receivedInterruption,
};

void *header_handler(void *_kernel_client_socket)
{
	pthread_mutex_lock(&mutex_kernel_socket);
	kernel_client_socket = (int)_kernel_client_socket;
	pthread_mutex_unlock(&mutex_kernel_socket);

	bool serve = true;
	while (serve)
	{
		t_packet *packet = socket_receive_packet(kernel_client_socket);
		if (packet == NULL)
		{
			if (!socket_retry_packet(kernel_client_socket, &packet))
			{
				close(kernel_client_socket);
				break;
			}
		}
		serve = cpu_handlers[packet->header](packet, kernel_client_socket);
		packet_destroy(packet);
	}
	return 0;
}

bool receivedPcb(t_packet *petition, int kernel_socket)
{
	pcb = create_pcb();
	stream_take_pcb(petition, pcb);
	if (!!pcb)
	{
		log_info(logger, "Received PID #%d with %d instructions", pcb->pid,
				 list_size(pcb->instructions));
		sem_post(&pcb_loaded);
		return true;
	}
	return false;
}

bool receivedInterruption(t_packet *petition, int kernel_socket)
{
	// recibir header interrupcion
	// meter interrupcion en queue o variable ya q se puede 1 a la vez
	// activar sem de interrupcion
	return false;
}

void (*execute[6])(t_instruction *instruction) =
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
		while (pcb->program_counter < list_size(pcb->instructions))
		{
			t_instruction *instruction;
			enum operation op = fetch_and_decode(&instruction);
			execute[op](instruction);
		}
	}
}

enum operation fetch_and_decode(t_instruction **instruction)
{
	*instruction = list_get(pcb->instructions, pcb->program_counter);

	pcb->program_counter = pcb->program_counter + 1;

	char *op_code = (*instruction)->id;
	enum operation op = get_op(op_code);

	log_info(logger, "PID #%d / Instruction %d --> %s", pcb->pid,
			 pcb->program_counter, op_code);
	return op;
}

void execute_no_op()
{
	usleep((time_t)config->delayNoOp);
}

void execute_io(t_instruction *instruction)
{
	uint32_t *time = list_get(instruction->params, 0);
	pcb_to_kernel(IO_CALL);
}

void execute_read(t_instruction *instruction)
{
	uint32_t *fst_param = list_get(instruction->params, 0);
}

void execute_copy(t_instruction *instruction)
{
	uint32_t *fst_param = list_get(instruction->params, 0);
	uint32_t *snd_param = list_get(instruction->params, 1);
}

void execute_write(t_instruction *instruction)
{
	uint32_t *fst_param = list_get(instruction->params, 0);
	uint32_t *snd_param = list_get(instruction->params, 1);
}

void execute_exit()
{
	pcb_to_kernel(EXIT_CALL);
}
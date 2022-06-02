#include "kernel.h"

/*crear thread
 * int pthread_create(pthread_t * thread_id,
 const pthread_attr_t * attr,
 void * (*start_routine)(void *),
 void * arg);

 (Esperar a la) Terminación:
 int pthread_join(pthread_t thread_id, void **value_ptr);

 recv (int __fd, void *__buf, size_t __n, int __flags);
 recv (client socket, message, strlen(message), 0);

 send (int __fd, const void *__buf, size_t __n, int __flags);
 send (client socket, message, strlen(message), 0);
 */

int main(void) {
	logger = create_logger();
	log_info(logger, "Logger started");

	config = getKernelConfig("kernel.config");

	// Inicializar estructuras de estado
	/*	newQ = pQueue_create();
	 readyQ = pQueue_create();

	 //Reloj de espera de ready
	 //clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start);

	 blockedQ = pQueue_create();
	 suspended_blockQ = pQueue_create();
	 suspended_readyQ = pQueue_create();

	 if (config == NULL) {
	 log_error(logger, "Config failed to load");
	 return EXIT_FAILURE;
	 }

	 // Setteo de Algoritmo de Planificacion
	 if (!strcmp(config->schedulerAlgorithm, "SJF"))
	 sortingAlgoritm = SRT;
	 if (!strcmp(config->schedulerAlgorithm, "FIFO"))
	 sortingAlgoritm = FIFO;

	 // Inicializar Planificador de largo plazo
	 pthread_create(&thread_longTerm, 0, thread_longTermFunc, NULL);
	 pthread_detach(thread_longTerm);

	 pthread_create(&thread_shortTermUnsuspender, 0,
	 thread_shortTermUnsuspenderFunc, NULL);
	 pthread_detach(thread_shortTermUnsuspender);

	 // Inicializar Planificador de mediano plazo
	 pthread_create(&thread_mediumTerm, 0, thread_mediumTermFunc, NULL);
	 pthread_detach(thread_mediumTerm);*/

	// Creacion de server
	int server_socket = create_server(config->kernelIP, config->kernelPort);
	log_info(logger, "Servidor listo para recibir al cliente");

	// Inicilizacion de semaforo
	//sem_init(&longTermSemCall, 0, 0);

	//pid = 0;
	while (1) {
		// pid++;
		server_listen(server_socket, header_handler);

		// t_pcb *pcb = create_pcb(process);
		// pQueue_put(newQ, (void*) pcb);
		// pthread_mutex_lock(&mutex_log);
		// log_info(logger, "Adding process to new, &d", pcb->id);
		// pthread_mutex_unlock(&mutex_log);

		// agregar una funcion que loggee con mutex en el logger
		// sem_post(&longTermSemCall); //despertar largo plazo
	}

	//Planificador de Largo Plazo

	//NEW

	//queue_push(newQ,pcb);

	//READY

	if (cupos_libres < config->multiprogrammingLevel) {
		t_pcb *ready_process = (t_pcb*) pQueue_take(newQ);
		pQueue_put(readyQ, (void*) ready_process);

		//Mensaje a memoria
		//send(client socket, message, strlen(message), 0);

		//Recibir tabla de paginas
		//recv(client socket, message, strlen(message), 0);

		//actualizar pcb
	}

	// if finalizacion -> exit -> msj a memoria -> msj a consola

	//Planificador de Mediano Plazo

	/*int max_blocked_time = config_get_int_value(config,"TIEMPO_MAXIMO_BLOQUEADO");

	 if(X->blocked_time>max_blocked_time){

	 //Suspender

	 //Mensaje a Memoria

	 }*/

	//Planificador de Corto Plazo
	/*if(){

	 //estimacion

	 }*/

	/*if(){
	 *
	 //interrupt a CPU

	 //CPU Desaloja Proceso

	 //Se recibe PCB por dispatch

	 }*/

	destroyKernelConfig(config);

	return EXIT_SUCCESS;
}

// Hilo del largo plazo, toma un proceso de new o suspended_ready y lo pasa a ready
void* thread_longTermFunc() {
	t_pcb *pcb;
	while (1) {
		sem_wait(&longTermSemCall);
		pthread_mutex_lock(&mutex_mediumTerm);
		pcb = (t_pcb*) pQueue_take(newQ);

		pthread_mutex_lock(&mutex_log);
		log_info(logger, "Long Term Scheduler: process %u from NEW to READY",
				pcb->id);
		pthread_mutex_unlock(&mutex_log);

		//putToReady(pcb);

		pthread_mutex_lock(&mutex_cupos);
		cupos_libres--; // TODO Chequear bien donde se modifica
		pthread_mutex_unlock(&mutex_cupos);

		pthread_cond_signal(&cond_mediumTerm);
		pthread_mutex_unlock(&mutex_mediumTerm);
	}
}

//Hilo del mediano plazo que pasa a Ready a aquellos procesos en Suspended-Ready
void* thread_mediumTermUnsuspenderFunc(void *args) {
	t_pcb *pcb;
	while (1) {
		sem_wait(&sem_multiprogram);
		sem_wait(&sem_newProcess);
		pthread_mutex_lock(&mutex_mediumTerm);
		if (pQueue_isEmpty(suspended_readyQ)) {
			sem_post(&longTermSemCall);
			pthread_mutex_unlock(&mutex_mediumTerm);
			continue;
		}
		pcb = (t_pcb*) pQueue_take(suspended_readyQ);

		pthread_mutex_lock(&mutex_log);
		log_info(logger,
				"Medium Term Scheduler: process %u from SUSPENDED-READY to READY",
				pcb->id);
		pthread_mutex_unlock(&mutex_log);

		//putToReady(process);

		pthread_mutex_lock(&mutex_cupos);
		cupos_libres--;
		pthread_mutex_unlock(&mutex_cupos);

		pthread_cond_signal(&cond_mediumTerm);
		pthread_mutex_unlock(&mutex_mediumTerm);
	}
}

// Hilo de mediano plazo, se despierta solo cuando el grado de multiprogramacion esta copado de procesos en blocked
// Agarra un proceso de blocked, lo pasa a suspended blocked y sube el grado de multiprogramacion
void* thread_mediumTermFunc(void *args) {
	t_pcb *pcb;

	//int memorySocket = connectToServer(config->memoryIP, config->memoryPort);

	//t_packet* suspendRequest;

	while (1) {
		pthread_mutex_lock(&mutex_mediumTerm);
		//Espera a que se cumpla la condicion para despertarse
		pthread_mutex_lock(&mutex_cupos);
		while (cupos_libres >= 1 || pQueue_isEmpty(newQ)
				|| !pQueue_isEmpty(readyQ) || pQueue_isEmpty(blockedQ)) {
			pthread_mutex_unlock(&mutex_cupos);
			pthread_cond_wait(&cond_mediumTerm, &mutex_mediumTerm);
			pthread_mutex_lock(&mutex_cupos);
		}
		pthread_mutex_unlock(&mutex_cupos);
		//Sacamos al proceso de la cola de blocked y lo metemos a suspended blocked
		pcb = (t_pcb*) pQueue_takeLast(blockedQ);

		pQueue_put(suspended_readyQ, (void*) process);

		sem_post(&sem_multiprogram);
		pthread_mutex_lock(&mutex_cupos);
		cupos_libres++;
		pthread_mutex_unlock(&mutex_cupos);

		//Notifica a memoria de la suspension
		//suspendRequest = createPacket(SUSPEND, INITIAL_STREAM_SIZE);
		//streamAdd_UINT32(suspendRequest->payload, process->pid);
		//socket_sendPacket(memorySocket, suspendRequest);
		//destroyPacket(suspendRequest);
		pthread_mutex_unlock(&mutex_mediumTerm);

		pthread_mutex_lock(&mutex_log);
		log_info(logger,
				"Medium Term Scheduler: process %u to Suspended Blocked",
				pcb->id);
		pthread_mutex_unlock(&mutex_log);
	}
}

// Hilo CPU, toma un proceso de ready y ejecuta todas sus peticiones hasta que se termine o pase a blocked
// Cuando lo pasa a blocked, recalcula el estimador de rafaga del proceso segun la cantidad de rafagas que duro
void* thread_shortTermFunc(void *args) {
	//intptr_t CPUid = (intptr_t)args;
	t_pcb *pcb = NULL;
	//t_packet *request = NULL;
	bool keepServing = true;
	struct timespec rafagaStart, rafagaStop;

	//int memorySocket = connectToServer(config->memoryIP, config->memoryPort);

	while (1) {
		pcb = NULL;
		keepServing = true;
		pcb = pQueue_take(readyQ);

		pthread_mutex_lock(&mutex_mediumTerm);
		pthread_cond_signal(&cond_mediumTerm);
		pthread_mutex_unlock(&mutex_mediumTerm);

		//t_packet* ok_packet = createPacket(OK, 0);
		//socket_sendPacket(pcb->socket, ok_packet);
		//destroyPacket(ok_packet);

		clock_gettime(CLOCK_MONOTONIC, &rafagaStart);

		pthread_mutex_lock(&mutex_log);
		//log_info(logger, "Short Term Scheduler %i: el proceso %u pasa de READY a EXEC", CPUid, pcb->id);
		pthread_mutex_unlock(&mutex_log);

		while (keepServing) {
			/*request = socket_getPacket(pcb->socket);
			 if(request == NULL){
			 if(!retry_getPacket(process->socket, &request)){
			 t_packet* abruptTerm = createPacket(CAPI_TERM, INITIAL_STREAM_SIZE);
			 streamAdd_UINT32(abruptTerm->payload, pcb->id);
			 petitionHandlers[CAPI_TERM](process, abruptTerm, memorySocket);
			 destroyPacket(abruptTerm);
			 petitionHandlers[DISCONNECTED](process, request, memorySocket);
			 break;
			 }
			 }*/
			//keepServing = petitionHandlers[request->header](process, request, memorySocket);
			/*if(request->header == SEM_WAIT || request->header == CALL_IO){
			 clock_gettime(CLOCK_MONOTONIC, &rafagaStop);
			 double rafagaMs = (double)(rafagaStop.tv_sec - rafagaStart.tv_sec) * 1000
			 + (double)(rafagaStop.tv_nsec - rafagaStart.tv_nsec) / 1000000;
			 double oldEstimate = process->estimate;
			 pcb->estimate = config->alpha * rafagaMs + (1 - config->alpha) * pcb->estimate;
			 pthread_mutex_lock(&mutex_log);
			 log_info(logger, "Proceso %u: Nueva estimacion - rafaga real finalizada: %f, Old Estimator: %f, New Estimator: %f", pcb->id, rafagaMs, oldEstimate, pcb->estimate);
			 pthread_mutex_unlock(&mutex_log);
			 }

			 destroyPacket(request);*/
		}
	}
}

void stream_take_process(t_packet *packet, t_process *process) {
	uint32_t *size = &(process->size);
	stream_take_UINT32P(packet->payload, &size);

	t_list *instructions = stream_take_LIST(packet->payload,
			stream_take_instruction);
	memcpy(process->instructions, instructions, sizeof(t_list));
}

void stream_take_instruction(t_stream_buffer *stream, t_instruction **elem) {
	char *id = stream_take_STRING(stream);
	if ((*elem) == NULL) {
		*elem = create_instruction(string_length(id));
	}

	memcpy((*elem)->id, id, string_length(id) + 1);

	t_list *params = stream_take_LIST(stream, stream_take_UINT32P);
	memcpy((*elem)->params, params, sizeof(t_list));

}

///////////////////////////////////////////////////////////////////////////////

t_pcb* create_pcb(t_process *process) {
	pid++;
	t_pcb *pcb = malloc(sizeof(t_pcb));
	pcb->instructions = list_create();
	//Tiran Warnings con Malloc
	//pcb->id; = malloc(sizeof(int));
	//pcb->size; = malloc(sizeof(int));
	//pcb->program_counter; = malloc(sizeof(int));
	//pcb->burst_estimation; = malloc(sizeof(int));
	//pcb->page_table=malloc(sizeof(t_ptbr));

	memcpy(pcb->instructions, process->instructions, sizeof(t_list));
	pcb->id = pid;
	pcb->size = process->size;
	pcb->program_counter = 0;
	pcb->burst_estimation = config->initialEstimate;

	return pcb;
}

void destroy_pcb(t_pcb *pcb) {
	if (pcb != NULL) {
		//Tiran Warnings
		//free(pcb->id);
		//free(pcb->size);
		//free(pcb->program_counter);
		//free(pcb->burst_estimation);

		//list_destroy(pcb->instructions);

		free(pcb->instructions);
		free(pcb);
	}
}

///////////////////////////////////////////////////////////////////////////////

bool receive_process(t_packet *petition, int console_socket) {
	t_process *received_process = create_process();
	stream_take_process(petition, received_process);
	log_process(logger, received_process);

	return false;
}

bool (*kernel_handlers[1])(t_packet *petition, int console_socket) =
{
	receive_process,
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
	return 0;
}

t_log* create_logger() {
	t_log *new_logger = log_create("kernel.log", "KERNEL", 1, LOG_LEVEL_INFO);
	return new_logger;
}

t_config* create_config() {
	t_config *new_config = config_create("kernel.config");
	return new_config;
}


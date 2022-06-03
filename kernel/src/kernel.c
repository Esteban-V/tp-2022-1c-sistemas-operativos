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
	logger = log_create("kernel.log", "KERNEL", 1, LOG_LEVEL_INFO);
	log_info(logger, "Logger started");

	config = getKernelConfig("kernel.config");

	// Inicializar estructuras de estado
	newQ = pQueue_create();
	readyQ = pQueue_create();
	blockedQ = pQueue_create();
	suspended_blockQ = pQueue_create();
	suspended_readyQ = pQueue_create();

	 if (config == NULL) {
		 log_error(logger, "Config failed to load");
		 return EXIT_FAILURE;
	 }

	 // Setteo de Algoritmo de Planificacion
	 if (!strcmp(config->schedulerAlgorithm, "SJF"))
	 sortingAlgorithm = SRT;
	 if (!strcmp(config->schedulerAlgorithm, "FIFO"))
	 sortingAlgorithm = FIFO;

	 // Inicializar Planificador de largo plazo
	 pthread_create(&thread_longTerm, 0, thread_longTermFunc, NULL);
	 pthread_detach(thread_longTerm);

	 pthread_create(&thread_mediumTermUnsuspender, 0, thread_mediumTermUnsuspenderFunc, NULL);
	 pthread_detach(thread_mediumTermUnsuspender);

	 // Inicializar Planificador de mediano plazo
	 pthread_create(&thread_mediumTerm, 0, thread_mediumTermFunc, NULL);
	 pthread_detach(thread_mediumTerm);

	// Creacion de server
	int server_socket = create_server(config->kernelIP, config->kernelPort);
	log_info(logger, "Servidor listo para recibir al cliente");

	// Inicializar semaforo de multiprocesamiento
	sem_init(&sem_multiprogram, 0, config->multiprogrammingLevel);
	sem_init(&freeCpu, 0, 1);
	cupos_libres = config->multiprogrammingLevel;
	pthread_mutex_init(&mutex_cupos, NULL);

	sem_init(&sem_newProcess, 0, 0);
	sem_init(&longTermSemCall, 0, 0);

	// Inicializo condition variable para despertar al planificador de mediano plazo
	pthread_cond_init(&cond_mediumTerm, NULL);
	pthread_mutex_init(&mutex_mediumTerm, NULL);

	//memmory_server_socket = connect_to(config->memoryIP, config->memoryPort);CONEXION CON MEMORIA
	//pid = 0;
	while (1) {
		server_listen(server_socket, header_handler);

	}

	//Planificador de Largo Plazo
	//NEW
	//queue_push(newQ,pcb);
	//READY
	/*if (cupos_libres < config->multiprogrammingLevel) {
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
	 //int max_blocked_time = config_get_int_value(config,"TIEMPO_MAXIMO_BLOQUEADO");
	 //if(X->blocked_time>max_blocked_time){
	 //Suspender
	 //Mensaje a Memoria
	 //}
	//Planificador de Corto Plazo
	//if(){
	 //estimacion
	 //}
	//if(){
	 //interrupt a CPU
	 //CPU Desaloja Proceso
	 //Se recibe PCB por dispatch
	 }*/

	destroyKernelConfig(config);

	return EXIT_SUCCESS;
}

// Hilo CPU, toma un proceso de ready y ejecuta todas sus peticiones hasta que se termine o pase a blocked
// Cuando lo pasa a blocked, recalcula el estimador de rafaga del proceso segun la cantidad de rafagas que duro
void* thread_shortTermFunc(void *args) {
	t_pcb *pcb = NULL;
	while(1){

		sem_wait(&freeCpu);
		pcb = pQueue_take(readyQ);

		pthread_mutex_lock(&mutex_log);
			log_info(logger, "Process %d to CPU", pcb->id);
		pthread_mutex_unlock(&mutex_log);



		//enviar pcb a cpu
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

t_pcb* create_pcb() {
	pid++;
	t_pcb *pcb = malloc(sizeof(t_pcb));
	pcb->instructions = list_create();
	pcb->id = malloc(sizeof(int));
	pcb->size = malloc(sizeof(int));
	pcb->program_counter = malloc(sizeof(int));
	pcb->burst_estimation = malloc(sizeof(int));

	return pcb;
}

/*	memcpy(pcb->instructions, process->instructions, sizeof(t_list));
	pcb->id = pid;
	pcb->size = process->size;
	pcb->program_counter = 0;
	pcb->burst_estimation = config->initialEstimate;*/

void destroy_pcb(t_pcb *pcb) {
	if (pcb != NULL) {
		free(pcb->id);
		free(pcb->size);
		free(pcb->program_counter);
		free(pcb->burst_estimation);
		list_destroy(pcb->instructions);

		free(pcb->instructions);
		free(pcb);
	}
}

void* thread_longTermFunc() { // Hilo del largo plazo, toma un proceso de new y lo pasa a ready
	t_pcb *pcb;
	while (1) {
		sem_wait(&longTermSemCall);
		pthread_mutex_lock(&mutex_mediumTerm);
		pcb = (t_pcb*) pQueue_take(newQ);
		putToReady(pcb);

		pthread_mutex_lock(&mutex_log);
			log_info(logger, "Long Term Scheduler: process %u from New to Ready",pcb->id);
		pthread_mutex_unlock(&mutex_log);

		pthread_mutex_lock(&mutex_cupos);
			cupos_libres--; // TODO Chequear bien donde se modifica
		pthread_mutex_unlock(&mutex_cupos);

		// Message a Memoria para que cree estructuras
		t_packet *memory_info = create_packet(MEMORY_INFO, 64);
		stream_add_UINT32(memory_info->payload, pcb->size);

		if (memory_server_socket != -1) {
			socket_send_packet(memory_server_socket, memory_info);
		}

		// Recibir valo de Tabla

		// Actualizar PCB



		//pthread_cond_signal(&cond_mediumTerm);
		pthread_mutex_unlock(&mutex_mediumTerm);
	}
}

void* thread_mediumTermUnsuspenderFunc(void *args) { // Hilo del mediano plazo que pasa a Ready a aquellos procesos en Suspended-Ready
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
		log_info(logger, "Medium Term Scheduler: process %u from Suspended Ready to Ready",pcb->id);
		pthread_mutex_unlock(&mutex_log);

		putToReady(pcb);

		pthread_mutex_lock(&mutex_cupos);
		cupos_libres--;
		pthread_mutex_unlock(&mutex_cupos);

		pthread_cond_signal(&cond_mediumTerm);
		pthread_mutex_unlock(&mutex_mediumTerm);
	}
}

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
		log_info(logger, "Medium Term Scheduler: process %u to Suspended Blocked", pcb->id);
		pthread_mutex_unlock(&mutex_log);
	}
}

// Funcion para poner un proceso a ready, actualiza la cola de ready y la reordena segun algoritmo
// No hay hilo de corto plazo ya que esta funcion hace exactamente eso de un saque
void putToReady(t_pcb *pcb) {

	pQueue_put(readyQ, (void*) pcb);

	if (sortingAlgorithm) {
		pQueue_sort(readyQ, SFJAlg);

		clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &now_time);
		/*	if(
		 //pcb->burst_estimation <
		 //((double)(now_time->tv_sec - start_exec_time->tv_sec)*BILLION + ((double)(now_time->tv_nsec - start_exec_time->tv_nsec)))

		 )*/
		{
			//enviar interrupcion a cpu

		}

		pthread_mutex_lock(&mutex_log);
		log_info(logger, "Corto Plazo: Cola Ready replanificada:");
		pthread_mutex_unlock(&mutex_log);
	}
}

bool SFJAlg(void *elem1, void *elem2) {
	return ((t_pcb*) elem1)->burst_estimation
			<= ((t_pcb*) elem2)->burst_estimation;
}


bool receive_process(t_packet *petition, int console_socket) {
	t_process *received_process = create_process();
	stream_take_process(petition, received_process);
	log_process(logger, received_process);

	if(!!received_process){
		t_pcb *pcb = create_pcb(received_process);
		pQueue_put(newQ, (void*) pcb);
		log_info(logger, "Adding process to New, %d", pcb->id);
		sem_post(&sem_newProcess);
	}

	process_destroy(received_process);

	return false;
}

bool io(t_packet *petition, int console_socket){
	sem_post(&freeCpu);
	t_pcb *received_pcb = create_pcb(); //cambiar create pcb para primero crearlo y dsp iniciarlo
	//stream_take_pcb(petition,received_pcb);
	pQueue_put(blockedQ,(void*) received_pcb);//faltaria poner en que momento entro en bloqueado?
	return false;
}

bool exitt(t_packet *petition, int console_socket){
	sem_post(&freeCpu);
	t_pcb *received_pcb = create_pcb();
	//stream_take_pcb(petition,received_pcb);
	return false;
}

bool (*kernel_handlers[3])(t_packet *petition, int console_socket) =
{
	receive_process,
	io,
	exitt
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

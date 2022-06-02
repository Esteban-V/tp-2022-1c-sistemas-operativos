#include"utils.h"

void putToReady(t_pcb *pcb);
t_kernelConfig* getKernelConfig(char *path) {
	t_kernelConfig *kernelConfig = malloc(sizeof(t_kernelConfig));
	kernelConfig->config = config_create(path);

	kernelConfig->kernelIP = config_get_string_value(kernelConfig->config,
			"IP");
	kernelConfig->kernelPort = config_get_string_value(kernelConfig->config,
			"PUERTO");
	kernelConfig->memoryIP = config_get_string_value(kernelConfig->config,
			"IP_MEMORIA");
	kernelConfig->memoryPort = config_get_int_value(kernelConfig->config,
			"PUERTO_MEMORIA");
	kernelConfig->cpuIP = config_get_string_value(kernelConfig->config,
			"IP_CPU");
	kernelConfig->cpuPortDispatch = config_get_int_value(kernelConfig->config,
			"PUERTO_CPU_DISPATCH");
	kernelConfig->cpuPortInterrupt = config_get_int_value(kernelConfig->config,
			"PUERTO_CPU_INTERRUPT");
	kernelConfig->listenPort = config_get_int_value(kernelConfig->config,
			"PUERTO_ESCUCHA");
	kernelConfig->schedulerAlgorithm = config_get_string_value(
			kernelConfig->config, "ALGORITMO_PLANIFICACION");
	kernelConfig->initialEstimate = config_get_int_value(kernelConfig->config,
			"ESTIMACION_INICIAL");
	kernelConfig->alpha = config_get_string_value(kernelConfig->config, "ALFA");
	kernelConfig->multiprogrammingLevel = config_get_int_value(
			kernelConfig->config, "GRADO_MULTIPROGRAMACION");
	kernelConfig->maxBlockedTime = config_get_int_value(kernelConfig->config,
			"TIEMPO_MAXIMO_BLOQUEADO");
	return kernelConfig;
}

void destroyKernelConfig(t_kernelConfig *kernelConfig) {
	//Tiran Warnings
	free(kernelConfig->memoryIP);
	//free(kernelConfig->memoryPort);
	free(kernelConfig->cpuIP);
	//free(kernelConfig->cpuPortDispatch);
	//free(kernelConfig->cpuPortInterrupt);
	//free(kernelConfig->listenPort);
	free(kernelConfig->schedulerAlgorithm);
	//free(kernelConfig->initialEstimate);
	free(kernelConfig->alpha);
	//free(kernelConfig->multiprogrammingLevel);
	//free(kernelConfig->maxBlockedTime);
	config_destroy(kernelConfig->config);
	free(kernelConfig);
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

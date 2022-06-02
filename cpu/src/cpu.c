#include"cpu.h"

int main(){

    // Initialize logger
    logger = log_create("./cpu.log", "CPU", 1, LOG_LEVEL_TRACE);
    config = getCPUConfig("cpu.config");

    while(1){

    }

    log_destroy(logger);

    return EXIT_SUCCESS;
}


bool receive_pcb(t_packet *petition, int console_socket) {
	t_pcb* received_pcb = create_pcb();
	stream_take_pcb(petition, received_pcb);
	pthread_mutex_lock(&mutex_log);
		log_pcb(logger, received_pcb);
	pthread_mutex_unlock(&mutex_log);

	if(!!received_pcb){
		pthread_mutex_lock(&mutex_log);
			//log_info(logger, "Adding process to New, %d", pcb->id);
		pthread_mutex_unlock(&mutex_log);
	}

	destroy_pcb(received_pcb);

	return false;
}

bool (*cpu_handlers[1])(t_packet *petition, int console_socket) =
{
	receive_pcb,
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
		serve = cpu_handlers[packet->header](packet, client_socket);
		packet_destroy(packet);
	}
	return 0;
}

/*
 ============================================================================
 Name        : kernel.c
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include "kernel.h"

int main(void) {
	logger = create_logger();
	config = create_config();

	ip = config_get_string_value(config, "IP");
	port = config_get_string_value(config, "PUERTO");

	//int server_socket = create_server(ip, port);
	log_info(logger, "Servidor listo para recibir al cliente");

	/*
	while (1) {
		pthread_t thread;
		int *client_socket = accept_client(server_socket);

		int thread_error = pthread_create(&thread, NULL, (void*) receive_client,
				client_socket);
		if (thread_error != 0) {
			log_error(logger, "Error creating thread");
		}

		pthread_detach(thread);
	}
	*/
	return EXIT_SUCCESS;
}

/*
void receive_client(void *thread_args) {
	t_list *lista;

	int *client_socket = *((int*) thread_args);
	while (1) {
		int cod_op = receive_op(client_socket);
		switch (cod_op) {
		case MESSAGE:
			receive_message(client_socket);
			break;
		case PACKAGE:
			lista = receive_package(client_socket);
			log_info(logger, "Me llegaron los siguientes valores:\n");
			list_iterate(lista, (void*) iterator);
			break;
		case -1:
			log_error(logger, "el cliente se desconecto.");
			pthread_exit(NULL);
		default:
			log_warning(logger,
					"Operacion desconocida. No quieras meter la pata");
			break;
		}
	}
}
*/

t_log* create_logger() {
	t_log *new_logger = log_create("kernel.log", "KERNEL", 1, LOG_LEVEL_INFO);
	return new_logger;
}

t_config* create_config() {
	t_config *new_config = config_create("kernel.config");
	return new_config;
}

void iterator(char *value) {
	log_info(logger, "%s", value);
}


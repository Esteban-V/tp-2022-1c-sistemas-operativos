#include"utils.h"

/*
int create_server(char *ip, char *port) {
	// Quitar esta línea cuando hayamos terminado de implementar la funcion
	// assert(!"no implementado!");

	int socket_servidor;

	struct addrinfo hints, *server_info, *p;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	getaddrinfo(ip, port, &hints, &server_info);

	socket_servidor = socket(server_info->ai_family, server_info->ai_socktype,
			server_info->ai_protocol);
	// Creamos el socket de escucha del servidor

	bind(socket_servidor, server_info->ai_addr, server_info->ai_addrlen);
	// Asociamos el socket a un puerto

	listen(socket_servidor, SOMAXCONN);
	// Escuchamos las conexiones entrantes

	freeaddrinfo(server_info);
	log_trace(logger, "Listo para escuchar a mi cliente");

	return socket_servidor;
}

int* accept_client(int server_socket) {
	int *client_socket = malloc(sizeof(int));
	*client_socket = accept(server_socket, NULL, NULL);
	// Aceptamos un nuevo cliente
	log_info(logger, "Se conecto un cliente!");

	return client_socket;
}

int receive_op(int socket_cliente) {
	int cod_op;
	if (recv(socket_cliente, &cod_op, sizeof(int), MSG_WAITALL) > 0)
		return cod_op;
	else {
		close(socket_cliente);
		return -1;
	}
}

void* receive_buffer(int *size, int socket_cliente) {
	void *buffer;

	recv(socket_cliente, size, sizeof(int), MSG_WAITALL);
	buffer = malloc(*size);
	recv(socket_cliente, buffer, *size, MSG_WAITALL);

	return buffer;
}

void receive_message(int socket_cliente) {
	int size;
	char *buffer = receive_buffer(&size, socket_cliente);
	log_info(logger, "Me llego el mensaje %s", buffer);
	free(buffer);
}

t_list* receive_package(int socket_cliente) {
	int size;
	int desplazamiento = 0;
	void *buffer;
	t_list *valores = list_create();
	int tamanio;

	buffer = receive_buffer(&size, socket_cliente);
	while (desplazamiento < size) {
		memcpy(&tamanio, buffer + desplazamiento, sizeof(int));
		desplazamiento += sizeof(int);
		char *valor = malloc(tamanio);
		memcpy(valor, buffer + desplazamiento, tamanio);
		desplazamiento += tamanio;
		list_add(valores, valor);
	}
	free(buffer);
	return valores;
}
*/

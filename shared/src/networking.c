#include"networking.h"

void catch_syscall_err(int code) {
	if (code == -1) {
		int error = errno;
		char *buf = malloc(100);
		strerror_r(error, buf, 100);
		pthread_mutex_lock(&mutex_log);
		log_error(logger, "Error: %s", buf);
		pthread_mutex_unlock(&mutex_log);
		free(buf);
	}
}

int connect_to(char *server_ip, char *server_port) {
    struct addrinfo hints;
    struct addrinfo *server_info;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    catch_syscall_err(getaddrinfo(server_ip, server_port, &hints, &server_info));

    // Ahora vamos a crear el socket.
    int socket_cliente = 0;
    catch_syscall_err(socket_cliente =socket(AF_INET, SOCK_STREAM, 0));
    // Ahora que tenemos el socket, vamos a conectarlo
    catch_syscall_err(connect(socket_cliente, server_info->ai_addr, server_info->ai_addrlen) !=0);
    freeaddrinfo(server_info);
    return socket_cliente;
}

int create_server(char* server_port) {
	int servidor=0;
    struct sockaddr_in direccionServidor;
    direccionServidor.sin_family = AF_INET;
    direccionServidor.sin_addr.s_addr = INADDR_ANY;
    direccionServidor.sin_port = htons(atoi(server_port));

    catch_syscall_err(servidor = socket(AF_INET, SOCK_STREAM, 0));

    //Sirve para que se puedan reutilizar los puertos mal cerrados
    int activado = 1;
    catch_syscall_err(setsockopt(servidor, SOL_SOCKET, SO_REUSEADDR, &activado, sizeof(activado)));

    catch_syscall_err(bind (servidor, (void*) &direccionServidor, sizeof(direccionServidor)) != 0);
    catch_syscall_err(listen(servidor, SOMAXCONN));
    log_info(logger, "Servidor listo para recibir al cliente.");
    return servidor;
}

int accept_client(int server_socket) {
	int client_socket = 0; // = malloc(sizeof(int));

	struct sockaddr_in client_address;
	socklen_t address_size = sizeof(struct sockaddr_in);
	catch_syscall_err(client_socket = accept(server_socket, (struct sockaddr*) &client_address, &address_size));
	return client_socket;
}

void server_listen(int server_socket, void* (*client_handler)(void*)) {

	int client_socket = accept_client(server_socket);
	pthread_t client_handler_thread = 0;
	catch_syscall_err(pthread_create(&client_handler_thread, NULL, client_handler, (void*) client_socket));
	pthread_detach(client_handler_thread);
}

//uso
/*
 while(1) {
 server_listen(server_socket, handler);
 }

 void* handler(void *_client_socket) {
 int client_socket = (int) _client_socket;
 bool serve = true;
 while (serve) {
 t_packet *petition = socket_receive_packet(client_socket);
 if (petition == NULL) {
 if (!socket_retry_packet(client_socket, &petition)) {
 close(client_socket);
 break;
 }
 }
 serve = petitionHandlers[petition->header](petition,
 client_socket);
 packet_destroy(petition);
 }
 return 0;
 }
 */

t_packet* create_packet(uint8_t header, size_t size) {
	t_packet *packet = malloc(sizeof(t_packet));
	if (packet == NULL) {
		return NULL;
	}

	packet->header = header;
	packet->payload = create_stream(size);
	if (packet->payload == NULL) {
		free(packet);
		return NULL;
	}

	return packet;
}

void packet_destroy(t_packet *packet) {
	stream_destroy(packet->payload);
	free(packet);
}

int receive_wrapper(int socket, void *dest, size_t size) {
	while (size > 0) {
		int i = recv(socket, dest, size, 0);
		if (i == 0)
			return 0;
		if (i < 0)
			return -1;
		dest += i;
		size -= i;
	}
	return 1;
}

bool socket_receive(int socket, void *dest, size_t size) {
	if (size != 0) {
		int rcv;
		catch_syscall_err(rcv = receive_wrapper(socket, dest, size));
		if (rcv < 1)
			return false;
	}
	return true;
}

uint8_t socket_receive_header(int socket) {
	uint8_t header;
	socket_receive(socket, &header, sizeof(uint8_t));
	return header;
}

t_packet* socket_receive_packet(int socket) {
	uint8_t header = socket_receive_header(socket);
	uint32_t size;

	if (!socket_receive(socket, &size, sizeof(uint32_t))) {
		return NULL;
	}

	t_packet *packet = create_packet(header, size);
	if (!socket_receive(socket, packet->payload->stream, size)) {
		packet_destroy(packet);
		return NULL;
	}

	return packet;
}

bool socket_retry_packet(int socket, t_packet **packet) {
	int tries = 0;

	while (*packet == NULL && tries <= 5) {
		sleep(1);
		*packet = socket_receive_packet(socket);
		if (*packet != NULL)
			return true;
		tries++;
	}

	return false;
}

int send_wrapper(int socket, void *buffer, size_t size) {
	while (size > 0) {
		int i = send(socket, buffer, size, 0);
		if (i == 0)
			return 0;
		if (i < 0)
			return -1;
		buffer += i;
		size -= i;
	}
	return 1;
}

void socket_send(int socket, void *source, size_t size) {
	catch_syscall_err(send_wrapper(socket, source, size));
}

void socket_send_header(int socket, uint8_t header) {
	uint8_t tmpHeader = header;
	socket_send(socket, (void*) &tmpHeader, sizeof(uint8_t));
}

void socket_send_packet(int socket, t_packet *packet) {
	socket_send_header(socket, packet->header);
	socket_send(socket, (void*) &packet->payload->offset, sizeof(uint32_t));
	socket_send(socket, (void*) packet->payload->stream,
			packet->payload->offset);
}

void socket_relay(int socket, t_packet *packet) {
	packet->payload->offset = (uint32_t) packet->payload->malloc_size;
	socket_send_packet(socket, packet);
}


#ifndef NETWORKING_H_
#define NETWORKING_H_

#include<sys/socket.h>
#include<sys/types.h>
#include<netdb.h>
#include<pthread.h>
#include<commons/log.h>
#include<errno.h>

#include"serialization.h"

typedef struct t_packet {
	uint8_t header;
	t_stream_buffer *payload;
} t_packet;


pthread_mutex_t mutex_log;
bool catch_syscall_err(int code);

int accept_client(int server_socket);
void server_listen(int server_socket, void* (*client_handler)(void*));

t_packet* create_packet(uint8_t header, size_t size);
void packet_destroy(t_packet *packet);

// int receive_wrapper(int socket, void *dest, size_t size);
// bool socket_receive(int socket, void *dest, size_t size);

uint8_t socket_receive_header(int socket);
t_packet* socket_receive_packet(int socket);
bool socket_retry_packet(int socket, t_packet **packet);

// int send_wrapper(int socket, void *buffer, size_t size);
// void socket_send(int socket, void *source, size_t size);

void socket_send_header(int socket, uint8_t header);
void socket_send_packet(int socket, t_packet *packet);

#endif /* NETWORKING_H_ */

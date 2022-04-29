#ifndef UTILSH
#define UTILSH

#include<stdio.h>
#include<stdlib.h>
#include<signal.h>
#include<unistd.h>
#include<sys/socket.h>
#include<netdb.h>
#include<string.h>
#include<commons/log.h>

typedef enum
{
    MENSAJE,
    PAQUETE
}op_code;

typedef struct
{
    int size;
    void* stream;
} t_buffer;

typedef struct
{
    op_code op_code;
    t_buffer* buffer;
} t_paquete;



int create_connection(char* ip, char* port);
void send_message(char* message, int socket_client);
t_paquete* create_package(void);
t_paquete* create_super_package(void);
void add_to_package(t_paquete* package, void* value, int size);
void send_package(t_paquete* package, int socket_client);
void destroy_connection(int socket_client);
void destroy_package(t_paquete* package);

#endif /* UTILSH */

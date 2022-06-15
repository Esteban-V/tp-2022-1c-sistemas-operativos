#ifndef MEMMORY_H_
#define MEMMORY_H_

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<pthread.h>
#include<commons/log.h>
#include<commons/config.h>
#include<commons/string.h>
#include<commons/collections/list.h>
#include"serialization.h"
#include"networking.h"
#include"socket_headers.h"
#include"swap.h"
#include"pageTable.h"
#include"utils.h"

typedef struct mem {
    void *memory;
} t_memory;

void* header_handler(void *_client_socket);
t_log *logger;
t_memory *memory;
t_memoryConfig *config;
uint32_t clock_m_counter;
t_memory *initializeMemory(t_memoryConfig *config);

#endif /* MEMMORY_H_ */

#ifndef MEMORY_H_
#define MEMORY_H_

#include"page_table.h"

typedef struct mem {
	void *memory;
} t_memory;

void* header_handler(void *_client_socket);
t_memory *memory;
uint32_t clock_m_counter;
t_memory* initializeMemory(t_memoryConfig *config);

#endif /* MEMORY_H_ */

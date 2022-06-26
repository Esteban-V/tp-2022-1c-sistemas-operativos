#ifndef MEMORY_H_
#define MEMORY_H_

#include"page_table.h"

void* header_handler(void *_client_socket);
t_memory *memory;
uint32_t clock_m_counter;
t_memory* initializeMemory(t_memoryConfig *config);

t_memoryMetadata *initializeMemoryMetadata(t_memoryConfig *config);
void destroyMemoryMetadata(t_memoryMetadata *meta);
bool swapInterface_savePage(t_swapInterface* self, uint32_t pid, int32_t pageNumber, void* pageContent);

#endif /* MEMMORY_H_ */

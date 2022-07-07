#ifndef MEMORY_H_
#define MEMORY_H_

#include"utils.h"
#include"page_table.h"

bool receive_pid(t_packet *petition, int kernel_socket);
bool memory_write(t_packet *petition, int cpu_socket);
bool memory_read(t_packet *petition, int cpu_socket);
bool end_process(t_packet *petition, int cpu_socket);

void *header_handler(void *_client_socket);

t_mem_metadata *create_memory_metadata(t_memoryConfig *config);
void memory_metadata_destroy(t_mem_metadata *meta);

t_memory *initializeMemory(t_memoryConfig *config);

#endif /* MEMORY_H_ */

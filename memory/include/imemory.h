#ifndef MEMORY_H_
#define MEMORY_H_

#include"utils.h"
#include"page_table.h"

bool access_lvl1_table(t_packet *petition, int cpu_socket);
bool access_lvl2_table(t_packet *petition, int cpu_socket);
bool receive_pid(t_packet *petition, int kernel_socket);
bool memory_write(t_packet *petition, int cpu_socket);
bool memory_read(t_packet *petition, int cpu_socket);
bool end_process(t_packet *petition, int cpu_socket);
bool process_suspension(t_packet *petition, int cpu_socket);

void *header_handler(void *_client_socket);

t_mem_metadata *initializeMemoryMetadata();
void memory_metadata_destroy(t_mem_metadata *meta);
t_memory *initializeMemory(t_memoryConfig *config);
bool handshake(t_packet *petition, int cpu_socket);

#endif /* MEMORY_H_ */

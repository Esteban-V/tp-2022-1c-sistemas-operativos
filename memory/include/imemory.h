#ifndef MEMORY_H_
#define MEMORY_H_

#include "utils.h"
#include "page_table.h"

bool access_lvl1_table(t_packet *petition, int cpu_socket);
bool access_lvl2_table(t_packet *petition, int cpu_socket);
bool process_new(t_packet *petition, int kernel_socket);
bool memory_write(t_packet *petition, int cpu_socket);
bool memory_read(t_packet *petition, int cpu_socket);
bool process_exit(t_packet *petition, int cpu_socket);
bool process_suspend(t_packet *petition, int cpu_socket);

bool cpu_handshake(t_packet *petition, int cpu_socket);
void *header_handler(void *_client_socket);

int server_socket;
t_list *swap_files;
int memory_access_counter = 0;
int memory_read_counter = 0;
int memory_write_counter = 0;

t_mem_metadata *metadata_init();
void metadata_destroy(t_mem_metadata *meta);
t_memory *memory_init();
void terminate_memory(bool error);

#endif /* MEMORY_H_ */

#ifndef MEMORY_H_
#define MEMORY_H_

#include "page_table.h"

bool access_lvl1_table(t_packet *petition, int cpu_socket);
bool access_lvl2_table(t_packet *petition, int cpu_socket);
bool process_new(t_packet *petition, int kernel_socket);
bool memory_write(t_packet *petition, int cpu_socket);
bool memory_read(t_packet *petition, int cpu_socket);
bool process_exit(t_packet *petition, int cpu_socket);
bool process_suspend(t_packet *petition, int cpu_socket);
bool process_unsuspend(t_packet *petition, int cpu_socket);
bool cpu_handshake(int cpu_socket);
void *packet_handler(void *_client_socket);

int server_socket;
pthread_t cpuThread, swapThread;

t_memory *memory_init();
void terminate_memory(int x);
int cpu_handshake_listener();

#endif /* MEMORY_H_ */

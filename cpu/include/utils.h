/*
 * cpu_config.h
 *
 *  Created on: 15 jun. 2022
 *      Author: utnso
 */

#ifndef INCLUDE_UTILS_H_
#define INCLUDE_UTILS_H_

#include<commons/config.h>

typedef struct cpu_config{
    t_config* config;
    int tlbEntries;
    char* tlbReplace;
    int delayNoOp;
    char* memoryIP;
    char* memoryPort;
    char* dispatchListenPort;
    char* interruptListenPort;
    char* ip;
} t_cpu_config;

void destroy_cpu_config(t_cpu_config* cpu_config);

t_cpu_config* get_cpu_config(char* path);

#endif /* INCLUDE_UTILS_H_ */

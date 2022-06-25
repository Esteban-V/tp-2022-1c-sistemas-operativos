/*
 * cpuConfig.h
 *
 *  Created on: 15 jun. 2022
 *      Author: utnso
 */

#ifndef INCLUDE_CPUCONFIG_H_
#define INCLUDE_CPUCONFIG_H_

#include<commons/config.h>

typedef struct cpuConfig{
    t_config* config;
    int tlbEntries;
    char* tlbReplace;
    int delayNoOp;
    char* memoryIP;
    char* memoryPort;
    char* dispatchListenPort;
    char* interruptListenPort;
    char* ip;
} t_cpuConfig;

void destroycpuConfig(t_cpuConfig* cpuConfig);

t_cpuConfig* getcpuConfig(char* path);

#endif /* INCLUDE_CPUCONFIG_H_ */

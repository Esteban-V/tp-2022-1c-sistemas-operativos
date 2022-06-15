/*
 * cpuConfig.c
 *
 *  Created on: 15 jun. 2022
 *      Author: utnso
 */
#include "cpuConfig.h"


t_cpuConfig* getcpuConfig(char* path){
    t_cpuConfig* cpuConfig = malloc(sizeof(t_cpuConfig));
    cpuConfig->config = config_create(path);
    cpuConfig->ip = config_get_string_value(cpuConfig->config, "CPU_IP");
    cpuConfig->tlbEntries = config_get_int_value(cpuConfig->config, "ENTRADAS_TLB");
    cpuConfig->tlbReplace = config_get_string_value(cpuConfig->config, "REEMPLAZO_TLB");
    cpuConfig->delayNoOp = config_get_int_value(cpuConfig->config, "RETARDO_NOOP");
    cpuConfig->memoryIP = config_get_string_value(cpuConfig->config, "IP_MEMORIA");
    cpuConfig->memoryPort = config_get_string_value(cpuConfig->config, "PUERTO_MEMORIA");
    cpuConfig->dispatchListenPort = config_get_string_value(cpuConfig->config, "PUERTO_ESCUCHA_DISPATCH");
    cpuConfig->interruptListenPort = config_get_string_value(cpuConfig->config, "PUERTO_ESCUCHA_INTERRUPT");
    return cpuConfig;
}

void destroycpuConfig(t_cpuConfig* cpuConfig){
	free(cpuConfig->ip);
    free(cpuConfig->tlbEntries);
    free(cpuConfig->tlbReplace);
    free(cpuConfig->delayNoOp);
    free(cpuConfig->memoryIP);
    free(cpuConfig->memoryPort);
    free(cpuConfig->dispatchListenPort);
    free(cpuConfig->interruptListenPort);
    config_destroy(cpuConfig->config);
    free(cpuConfig);
}

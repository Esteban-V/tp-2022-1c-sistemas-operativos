/*
 * utils.c
 *
 *  Created on: 15 jun. 2022
 *      Author: utnso
 */
#include <utils.h>

t_cpu_config* get_cpu_config(char *path) {
	t_cpu_config *cpu_config = malloc(sizeof(t_cpu_config));
	cpu_config->config = config_create(path);
	cpu_config->ip = config_get_string_value(cpu_config->config, "CPU_IP");
	cpu_config->tlbEntries = config_get_int_value(cpu_config->config,
			"ENTRADAS_TLB");
	cpu_config->tlbReplace = config_get_string_value(cpu_config->config,
			"REEMPLAZO_TLB");
	cpu_config->delayNoOp = config_get_int_value(cpu_config->config,
			"RETARDO_NOOP");
	cpu_config->memoryIP = config_get_string_value(cpu_config->config,
			"IP_MEMORIA");
	cpu_config->memoryPort = config_get_string_value(cpu_config->config,
			"PUERTO_MEMORIA");
	cpu_config->dispatchListenPort = config_get_string_value(cpu_config->config,
			"PUERTO_ESCUCHA_DISPATCH");
	cpu_config->interruptListenPort = config_get_string_value(cpu_config->config,
			"PUERTO_ESCUCHA_INTERRUPT");
	return cpu_config;
}

void destroy_cpu_config(t_cpu_config *cpu_config) {
	free(cpu_config->ip);
	free(cpu_config->tlbEntries);
	free(cpu_config->tlbReplace);
	free(cpu_config->delayNoOp);
	free(cpu_config->memoryIP);
	free(cpu_config->memoryPort);
	free(cpu_config->dispatchListenPort);
	free(cpu_config->interruptListenPort);
	config_destroy(cpu_config->config);
	free(cpu_config);
}

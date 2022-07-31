#include "../include/utils.h"

t_cpu_config *get_cpu_config(char *path)
{
	t_cpu_config *cpu_config = malloc(sizeof(t_cpu_config));
	cpu_config->config = config_create(path);
	cpu_config->tlbEntryQty = config_get_int_value(cpu_config->config,
												  "ENTRADAS_TLB");
	cpu_config->tlb_alg = config_get_string_value(cpu_config->config,
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

void destroy_cpu_config(t_cpu_config *cpu_config)
{
	free(cpu_config->tlb_alg);
	free(cpu_config->memoryIP);
	free(cpu_config->memoryPort);
	free(cpu_config->dispatchListenPort);
	free(cpu_config->interruptListenPort);
	config_destroy(cpu_config->config);
	free(cpu_config);
}

enum operation get_op(char *op)
{
	if (!strcmp(op, "NO_OP"))
	{
		return NO_OP;
	}
	else if (!strcmp(op, "I/O"))
	{
		return IO_OP;
	}
	else if (!strcmp(op, "READ"))
	{
		return READ;
	}
	else if (!strcmp(op, "WRITE"))
	{
		return WRITE;
	}
	else if (!strcmp(op, "COPY"))
	{
		return COPY;
	}
	else if (!strcmp(op, "EXIT"))
	{
		return EXIT_OP;
	}
	else
		return DEAD;
}


uint32_t get_frame(uint32_t page_number){
	
}
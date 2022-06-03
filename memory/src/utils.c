#include"utils.h"

t_memoryConfig* getMemoryConfig(char *path) {
	t_memoryConfig *memoryConfig = malloc(sizeof(t_memoryConfig));
	memoryConfig->config = config_create(path);
	memoryConfig->tlbEntries = config_get_int_value(memoryConfig->config,
			"ENTRADAS_TLB");
	memoryConfig->tlbReplace = config_get_string_value(memoryConfig->config,
			"REEMPLAZO_TLB");
	memoryConfig->delayNoOp = config_get_int_value(memoryConfig->config,
			"RETARDO_NOOP");
	memoryConfig->memoryIP = config_get_string_value(memoryConfig->config,
			"IP_MEMORIA");
	memoryConfig->memoryPort = config_get_string_value(memoryConfig->config,
			"PUERTO_MEMORIA");
	memoryConfig->dispatchListenPort = config_get_int_value(
			memoryConfig->config, "PUERTO_ESCUCHA_DISPATCH");
	memoryConfig->interruptListenPort = config_get_int_value(
			memoryConfig->config, "PUERTO_ESCUCHA_INTERRUPT");
	return memoryConfig;
}

void destroyMemoryConfig(t_memoryConfig *memoryConfig) {
	free(memoryConfig->tlbEntries);
	free(memoryConfig->tlbReplace);
	free(memoryConfig->delayNoOp);
	free(memoryConfig->memoryIP);
	free(memoryConfig->memoryPort);
	free(memoryConfig->dispatchListenPort);
	free(memoryConfig->interruptListenPort);
	config_destroy(memoryConfig->config);
	free(memoryConfig);
}

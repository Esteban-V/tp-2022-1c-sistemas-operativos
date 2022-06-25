#include"utils.h"

t_kernelConfig* getKernelConfig(char *path) {
	t_kernelConfig *kernelConfig = malloc(sizeof(t_kernelConfig));
	kernelConfig->config = config_create(path);
	kernelConfig->kernelIP = config_get_string_value(kernelConfig->config,
			"IP");
	kernelConfig->kernelPort = config_get_string_value(kernelConfig->config,
			"PUERTO");
	kernelConfig->memoryIP = config_get_string_value(kernelConfig->config,
			"IP_MEMORIA");
	kernelConfig->memoryPort = config_get_string_value(kernelConfig->config,
			"PUERTO_MEMORIA");
	kernelConfig->cpuIP = config_get_string_value(kernelConfig->config,
			"IP_CPU");
	kernelConfig->cpuPortDispatch = config_get_string_value(
			kernelConfig->config, "PUERTO_CPU_DISPATCH");
	kernelConfig->cpuPortInterrupt = config_get_string_value(
			kernelConfig->config, "PUERTO_CPU_INTERRUPT");
	kernelConfig->listenPort = config_get_string_value(kernelConfig->config,
			"PUERTO_ESCUCHA");
	kernelConfig->schedulerAlgorithm = config_get_string_value(
			kernelConfig->config, "ALGORITMO_PLANIFICACION");
	kernelConfig->initialEstimate = config_get_int_value(kernelConfig->config,
			"ESTIMACION_INICIAL");
	kernelConfig->alpha = config_get_string_value(kernelConfig->config, "ALFA");
	kernelConfig->multiprogrammingLevel = config_get_int_value(
			kernelConfig->config, "GRADO_MULTIPROGRAMACION");
	kernelConfig->maxBlockedTime = config_get_int_value(kernelConfig->config,
			"TIEMPO_MAXIMO_BLOQUEADO");
	return kernelConfig;
}

void destroyKernelConfig(t_kernelConfig *kernelConfig) {
	//Tiran Warnings
	free(kernelConfig->memoryIP);
	free(kernelConfig->memoryPort);
	free(kernelConfig->cpuIP);
	free(kernelConfig->cpuPortDispatch);
	free(kernelConfig->cpuPortInterrupt);
	free(kernelConfig->listenPort);
	free(kernelConfig->schedulerAlgorithm);
	//free(kernelConfig->initialEstimate);
	free(kernelConfig->alpha);
	//free(kernelConfig->multiprogrammingLevel);
	//free(kernelConfig->maxBlockedTime);
	config_destroy(kernelConfig->config);
	free(kernelConfig);
}

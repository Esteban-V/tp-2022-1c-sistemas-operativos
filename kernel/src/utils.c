#include "utils.h"

t_kernelConfig *getKernelConfig(char *path)
{
	t_kernelConfig *kernelConfig = malloc(sizeof(t_kernelConfig));
	kernelConfig->config = config_create(path);
	kernelConfig->memoryIP = config_get_string_value(kernelConfig->config, "IP_MEMORIA");
	kernelConfig->memoryPort = config_get_string_value(kernelConfig->config, "PUERTO_MEMORIA");
	kernelConfig->cpuIP = config_get_string_value(kernelConfig->config, "IP_CPU");
	kernelConfig->cpuPortDispatch = config_get_string_value(kernelConfig->config, "PUERTO_CPU_DISPATCH");
	kernelConfig->cpuPortInterrupt = config_get_string_value(kernelConfig->config, "PUERTO_CPU_INTERRUPT");
	kernelConfig->listenPort = config_get_string_value(kernelConfig->config, "PUERTO_ESCUCHA");
	kernelConfig->schedulerAlgorithm = config_get_string_value(kernelConfig->config, "ALGORITMO_PLANIFICACION");
	kernelConfig->initialEstimate = config_get_int_value(kernelConfig->config, "ESTIMACION_INICIAL");
	kernelConfig->alpha = config_get_double_value(kernelConfig->config, "ALFA");
	kernelConfig->multiprogrammingLevel = config_get_int_value(kernelConfig->config, "GRADO_MULTIPROGRAMACION");
	kernelConfig->maxBlockedTime = config_get_int_value(kernelConfig->config, "TIEMPO_MAXIMO_BLOQUEADO");
	return kernelConfig;
}

void destroyKernelConfig(t_kernelConfig *kernelConfig)
{
	config_destroy(kernelConfig->config);
	free(kernelConfig);
}

float time_to_ms(struct timespec time)
{
	return (time.tv_sec) * 1000 + (time.tv_nsec) / 1000000;
}

bool SJF_sort(void *elem1, void *elem2)
{
	t_pcb *pcb1 = (t_pcb *)elem1;
	t_pcb *pcb2 = (t_pcb *)elem2;
	return (pcb1->burst_estimation) <= (pcb2->burst_estimation);
}

#include"utils.h"

t_CPUConfig* getCPUConfig(char* path){
    t_CPUConfig* cpuConfig = malloc(sizeof(t_CPUConfig));
    cpuConfig->config = config_create(path);
    cpuConfig->listenPort = config_get_int_value(cpuConfig->config, "PUERTO_ESCUCHA");
    cpuConfig->memorySize = config_get_int_value(cpuConfig->config, "TAM_MEMORIA");
    cpuConfig->pageSize = config_get_int_value(cpuConfig->config, "TAM_PAGINA");
    cpuConfig->entriesPerPage = config_get_int_value(cpuConfig->config, "ENTRADAS_POR_TABLA");
    cpuConfig->memoryDelay = config_get_int_value(cpuConfig->config, "RETARDO_MEMORIA");
    cpuConfig->replaceAlgorithm = config_get_string_value(cpuConfig->config, "ALGORITMO_REEMPLAZO");
    cpuConfig->framesPerProcess = config_get_int_value(cpuConfig->config, "MARCOS_POR_PROCESO");
    cpuConfig->swapDelay = config_get_int_value(cpuConfig->config, "RETARDO_SWAP");
    cpuConfig->swapPath = config_get_string_value(cpuConfig->config, "PATH_SWAP");
    return cpuConfig;
}

void destroyCPUConfig(t_CPUConfig* cpuConfig){
    free(cpuConfig->listenPort);
    free(cpuConfig->memorySize);
    free(cpuConfig->pageSize);
    free(cpuConfig->entriesPerPage);
    free(cpuConfig->memoryDelay);
    free(cpuConfig->replaceAlgorithm);
    free(cpuConfig->framesPerProcess);
    free(cpuConfig->swapDelay);
    free(cpuConfig->swapPath);
    config_destroy(cpuConfig->config);
    free(cpuConfig);
}
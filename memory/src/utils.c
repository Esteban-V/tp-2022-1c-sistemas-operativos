#include"utils.h"

t_memoryConfig* getMemoryConfig(char* path){
    t_memoryConfig* memoryConfig = malloc(sizeof(t_memoryConfig));
    memoryConfig->config = config_create(path);
    memoryConfig->listenPort = config_get_string_value(memoryConfig->config, "PUERTO_ESCUCHA");
    memoryConfig->memorySize = config_get_int_value(memoryConfig->config, "TAM_MEMORIA");
    memoryConfig->pageSize = config_get_int_value(memoryConfig->config, "TAM_PAGINA");
    memoryConfig->entriesPerTable = config_get_int_value(memoryConfig->config, "ENTRADAS_POR_TABLA");
    memoryConfig->memoryDelay = config_get_int_value(memoryConfig->config, "RETARDO_MEMORIA");
    memoryConfig->replaceAlgorithm = config_get_string_value(memoryConfig->config, "ALGORITMO_REEMPLAZO");
    memoryConfig->framesPerProcess = config_get_int_value(memoryConfig->config, "MARCOS_POR_PROCESO");
    memoryConfig->swapDelay = config_get_int_value(memoryConfig->config, "RETARDO_SWAP");
    memoryConfig->swapPath = config_get_string_value(memoryConfig->config, "PATH_SWAP");
    memoryConfig->framesInMemory = memoryConfig->memorySize / memoryConfig->pageSize;
    return memoryConfig;
}

void destroyMemoryConfig(t_memoryConfig* memoryConfig){
    config_destroy(memoryConfig->config);
    free(memoryConfig);
}

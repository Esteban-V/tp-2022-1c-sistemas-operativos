#ifndef MEMORY_INCLUDE_UTILS_H_
#define MEMORY_INCLUDE_UTILS_H_

typedef struct memoryConfig{
    t_config* config;
    int tlbEntries;
    char* tlbReplace;
    int delayNoOp;
    int memoryIP;
    int memoryPort;
    int dispatchListenPort;
    char* interruptListenPort;
} t_memoryConfig;

t_memoryConfig* getMemoryConfig(char* path);
void destroyMemoryConfig(t_memoryConfig* memoryConfig);

#endif /* MEMORY_INCLUDE_UTILS_H_ */

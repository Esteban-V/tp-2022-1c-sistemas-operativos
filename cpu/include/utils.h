#ifndef CPU_INCLUDE_UTILS_H_
#define CPU_INCLUDE_UTILS_H_

typedef struct t_CPUConfig{
    t_config* config;
    int listenPort;
    int memorySize;
    int pageSize;
    int entriesPerPage;
    int memoryDelay;
    char* replaceAlgorithm;
    int framesPerProcess;
    int swapDelay;
    char* swapPath;
} t_CPUConfig;

t_CPUConfig* getCPUConfig(char* path);

void destroyCPUConfig(t_CPUConfig* cpuConfig);

#endif /* CPU_INCLUDE_UTILS_H_ */

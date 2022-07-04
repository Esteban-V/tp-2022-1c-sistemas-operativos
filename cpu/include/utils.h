#ifndef UTILS_H_
#define UTILS_H_

#include <commons/config.h>
#include "pcb_utils.h"

enum operation
{
    NO_OP,
    IO_OP,
    READ,
    COPY,
    WRITE,
    EXIT_OP,
    DEAD
};

enum operation get_op(char *);

typedef struct cpu_config{
    t_config* config;
    int tlbEntries;
    char* tlbReplace;
    int delayNoOp;
    char* memoryIP;
    char* memoryPort;
    char* dispatchListenPort;
    char* interruptListenPort;
    char* ip;
} t_cpu_config;

void destroy_cpu_config(t_cpu_config* cpu_config);

t_cpu_config* get_cpu_config(char* path);

#endif /* UTILS_H_ */

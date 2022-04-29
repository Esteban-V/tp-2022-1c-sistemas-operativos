#ifndef CONSOLE_H_
#define CONSOLE_H_

#include<stdio.h>
#include<stdlib.h>
#include<string.h>

#include<commons/log.h>

#include<commons/string.h>
#include<commons/collections/list.h>
#include<commons/collections/queue.h>
#include<commons/collections/dictionary.h>

#include<commons/config.h>
#include<readline/readline.h>

typedef struct instruction {
    char* id;
    int* params;
} t_instruction;

typedef struct process {
    int size;
    t_list* instructions;
} t_process;

t_list* instruction_list;
t_process* process;

t_config* config;
t_log* logger;
int conexion;

FILE* open_file(char *path);
void get_code(FILE *file);
t_instruction* parse_instruction(char *string);

t_instruction* instruction_create(size_t id_size);
void instruction_destroy(t_instruction *instruction);

t_process* process_create();
void process_destroy(t_process *process);

t_log* iniciar_logger(void);
t_config* iniciar_config(void);

void terminate_console();

#endif /* CONSOLE_H_ */

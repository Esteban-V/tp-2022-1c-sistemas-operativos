#ifndef CONSOLE_H_
#define CONSOLE_H_

#include<stdio.h>
#include<stdlib.h>

#include<commons/log.h>

#include<commons/string.h>
#include<commons/collections/list.h>
#include<commons/collections/queue.h>
#include<commons/collections/dictionary.h>

#include<commons/config.h>
#include<readline/readline.h>

//#include <utils.c>

#endif /* CONSOLE_H_ */


typedef struct instruction {
    char* instruction_id;
    int* instruction_params;
} t_instruction;


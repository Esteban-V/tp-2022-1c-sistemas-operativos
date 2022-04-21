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

#endif /* CONSOLE_H_ */

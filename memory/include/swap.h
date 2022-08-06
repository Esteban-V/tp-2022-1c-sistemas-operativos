#ifndef SWAP_H_
#define SWAP_H_

#include <stdint.h>
#include <stdbool.h>
#include <commons/collections/dictionary.h>
#include <pthread.h>
#include "utils.h"

typedef struct relation_t
{
    uint32_t pid;
    void *dir;
    uint32_t size;
} relation_t;

t_list *relations;
void *dir;
void *value_swap;
void *read_from_swap;
void create_swap(uint32_t pid, uint32_t process_size);
void swap_write_page(uint32_t pid, int page, void *data);
void *swap_get_page(uint32_t pid, int page);
void delete_swap(uint32_t pid);
void swap();
typedef enum
{
    CREATE_SWAPPP,
    WRITE_SWAP,
    READ_SWAP
} swap_instrusct;
swap_instrusct swap_instruct;
sem_t sem_swap, swap_end;
uint32_t pid_swap, page_num_swap, pid_size_swap;
bool create_swapp(uint32_t pid, uint32_t size);
void read_swap(void *result, uint32_t pid, uint32_t page_num);
void write_swapp(uint32_t pid, void *value, uint32_t page_num);
relation_t *findRela(uint32_t pid);

#endif /* SWAP_H_ */
